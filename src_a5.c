
// ---------------------------------------------------------------- config
static char g_fontFace[64] = "MS UI Gothic"; // font for Japanese characters (the game's own bitmap fonts have none)
static int g_sizeAdjust = 0;                  // px added to the Japanese character height
static int g_yAdjust = 0;                     // px added to the Japanese vertical position
static int g_bold = 0;                        // 1 = bold Japanese
static int g_lineGap = 2;                     // px added to the line pitch of wrapped text (Japanese is taller)

static void load_config(const char *dir)
{
    char ini[MAX_PATH];
    _snprintf(ini, sizeof ini, "%s\\avernum_jp.ini", dir);
    GetPrivateProfileStringA("font", "face", g_fontFace, g_fontFace, sizeof g_fontFace, ini);
    g_sizeAdjust = GetPrivateProfileIntA("font", "size_adjust", g_sizeAdjust, ini);
    g_yAdjust = GetPrivateProfileIntA("font", "y_adjust", g_yAdjust, ini);
    g_bold = GetPrivateProfileIntA("font", "bold", g_bold, ini);
    g_lineGap = GetPrivateProfileIntA("font", "line_gap", g_lineGap, ini);
}

// ---------------------------------------------------------------- text encoding
// The game converts its text with CP_THREAD_ACP (3). Those conversions are switched to
// EUC-JP. CP_ACP (0) is used for file paths and is left alone, except for the one
// conversion of a string-table text right after LoadStringW (see my_LoadStringW).
typedef int(WINAPI *MB2WC_t)(UINT, DWORD, LPCCH, int, LPWSTR, int);
typedef int(WINAPI *WC2MB_t)(UINT, DWORD, LPCWCH, int, LPSTR, int, LPCCH, LPBOOL);
static MB2WC_t g_realMB2WC;
static WC2MB_t g_realWC2MB;
static volatile LONG g_nextAcpIsGame; // set by my_LoadStringW for the following CP_ACP conversion

static int WINAPI my_MultiByteToWideChar(UINT cp, DWORD flags, LPCCH s, int n, LPWSTR w, int wn)
{
    if (cp == CP_THREAD_ACP)
    {
        cp = GAME_CP;
        flags = 0; // MB_PRECOMPOSED etc. are not accepted for 20932
    }
    return g_realMB2WC(cp, flags, s, n, w, wn);
}

static int WINAPI my_WideCharToMultiByte(UINT cp, DWORD flags, LPCWCH w, int wn, LPSTR s, int n,
                                         LPCCH def, LPBOOL used)
{
    if (cp == CP_THREAD_ACP || (cp == CP_ACP && InterlockedExchange(&g_nextAcpIsGame, 0)))
    {
        cp = GAME_CP;
        flags = 0;
    }
    return g_realWC2MB(cp, flags, w, wn, s, n, def, used);
}

// ---------------------------------------------------------------- string table
// avernum_jp_strtab.tsv (UTF-8, tab separated): id, english, japanese.
typedef int(WINAPI *LoadStringW_t)(HINSTANCE, UINT, LPWSTR, int);
static LoadStringW_t g_realLoadStringW;
typedef struct
{
    UINT id;
    WCHAR *text;
} StrTab;
static StrTab *g_strtab;
static int g_strtabCount;

static int cmp_strtab(const void *a, const void *b)
{
    UINT x = ((const StrTab *)a)->id, y = ((const StrTab *)b)->id;
    return x < y ? -1 : x > y;
}

static const WCHAR *strtab_get(UINT id)
{
    int lo = 0, hi = g_strtabCount;
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        if (g_strtab[mid].id < id)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < g_strtabCount && g_strtab[lo].id == id ? g_strtab[lo].text : NULL;
}

static int WINAPI my_LoadStringW(HINSTANCE inst, UINT id, LPWSTR buf, int max)
{
    const WCHAR *t = strtab_get(id);
    int n;
    InterlockedExchange(&g_nextAcpIsGame, 1); // the game converts the result with CP_ACP next
    if (!t)
        return g_realLoadStringW(inst, id, buf, max);
    n = (int)wcslen(t);
    if (max == 0)
    {
        *(const WCHAR **)buf = t; // read-only pointer, as LoadStringW does with cchBufferMax 0
        return n;
    }
    if (max < 1)
        return 0;
    if (n >= max)
        n = max - 1;
    memcpy(buf, t, n * sizeof(WCHAR));
    buf[n] = 0;
    return n;
}

static void load_string_table(const char *dir)
{
    char path[MAX_PATH], *buf, *line, *next;
    DWORD size;
    int cap = 0, skipped = 0;
    _snprintf(path, sizeof path, "%s\\avernum_jp_strtab.tsv", dir);
    buf = (char *)read_all(path, &size);
    if (!buf)
    {
        jp_log("no %s (string table stays English)\n", path);
        return;
    }
    buf = (char *)realloc(buf, size + 1);
    buf[size] = 0;
    for (line = buf; line && *line; line = next)
    {
        char *col[3] = {0}, *p;
        int c = 0, wn;
        next = strchr(line, '\n');
        if (next)
            *next++ = 0;
        if ((p = strchr(line, '\r')) != NULL)
            *p = 0;
        col[0] = line;
        for (p = line; *p && c < 2; p++)
            if (*p == '\t')
            {
                *p = 0;
                col[++c] = p + 1;
            }
        if (c < 2 || !col[2][0] || !(col[0][0] >= '0' && col[0][0] <= '9'))
            continue;
        unescape(col[2]);
        wn = MultiByteToWideChar(CP_UTF8, 0, col[2], -1, NULL, 0);
        if (wn <= 0)
        {
            skipped++;
            continue;
        }
        if (g_strtabCount == cap)
        {
            cap = cap ? cap * 2 : 1024;
            g_strtab = (StrTab *)realloc(g_strtab, cap * sizeof(StrTab));
        }
        g_strtab[g_strtabCount].id = (UINT)strtoul(col[0], NULL, 10);
        g_strtab[g_strtabCount].text = (WCHAR *)malloc(wn * sizeof(WCHAR));
        MultiByteToWideChar(CP_UTF8, 0, col[2], -1, g_strtab[g_strtabCount].text, wn);
        g_strtabCount++;
    }
    free(buf);
    qsort(g_strtab, g_strtabCount, sizeof(StrTab), cmp_strtab);
    jp_log("string table: %d translated, %d skipped\n", g_strtabCount, skipped);
}

// ---------------------------------------------------------------- script token estimate
// Before tokenizing a script, 0x45c660 estimates the token count to size the token array.
// It counts every byte that is not a digit, letter, '_' or blank as its own token -- also
// inside string literals -- so Japanese adds 2 per character, a long dialogue file overflows
// the count, the allocation fails and the CRT aborts ("Runtime Error!" while loading
// dialogue scripts). A literal is still one token, so classify bytes >= 0x80 like letters.
//   0x45c695: cmp al,30h / jl 45c6a4 / cmp al,39h / jg 45c6a4   -> jmp tok_class_stub
#define A4_TOKCLASS 0x45c695
#define A4_TOK_DIGIT 0x45c69d
#define A4_TOK_OTHER 0x45c6a4
#define A4_TOK_LETTER 0x45c6c5
static const BYTE kTokClassOrig[8] = {0x3C, 0x30, 0x7C, 0x0B, 0x3C, 0x39, 0x7F, 0x07};
static DWORD g_tokDigit, g_tokOther, g_tokLetter;

__declspec(naked) static void tok_class_stub(void)
{
    __asm {
        test al, al
        js letter
        cmp al, 0x30
        jl other
        cmp al, 0x39
        jg other
        jmp dword ptr [g_tokDigit]
    other:
        jmp dword ptr [g_tokOther]
    letter:
        jmp dword ptr [g_tokLetter]
    }
}

static void install_token_estimate_fix(void)
{
    BYTE *p = (BYTE *)VA(A4_TOKCLASS);
    DWORD old;
    if (memcmp(p, kTokClassOrig, sizeof kTokClassOrig) != 0)
    {
        jp_log("token estimate fix skipped: unexpected bytes at %06x (not the Steam build?)\n", A4_TOKCLASS);
        return;
    }
    g_tokDigit = (DWORD)VA(A4_TOK_DIGIT);
    g_tokOther = (DWORD)VA(A4_TOK_OTHER);
    g_tokLetter = (DWORD)VA(A4_TOK_LETTER);
    VirtualProtect(p, 8, PAGE_EXECUTE_READWRITE, &old);
    p[0] = 0xE9;
    *(DWORD *)(p + 1) = (DWORD)tok_class_stub - (DWORD)(p + 5);
    p[5] = p[6] = p[7] = 0x90;
    VirtualProtect(p, 8, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 8);
    jp_log("token estimate fix installed\n");
}

// ---------------------------------------------------------------- text drawing
// Avernum 5 draws all text with its own bitmap fonts (G290-G294) through one routine:
//   0x46c890  draw_text(ecx = string; surface, mode, size, r, g, b, left, top, right, bottom)  cdecl
//             mode 0 = wrap in the rect, 1 = one line centred, 2 = one line at left/top
//             r, g, b are 0..31. Bytes above 0x7f are turned into blanks.
//   0x46d7d0  draw_line(ecx = string; surface, r, g, b)  cdecl, pen at the words 0x55b144 / 0x55b148
//   0x46c740  text_width(string)  cdecl, returns the start of the last character (ax)
// draw_text is replaced: the same layout, but EUC-JP characters are measured and drawn with GDI
// on the same DirectDraw surface (IDirectDrawSurface::GetDC); ASCII runs still go through
// draw_line so English looks the same.
#define A5_DRAW_TEXT 0x46c890
#define A5_DRAW_LINE 0x46d7d0
#define A5_TEXT_WIDTH 0x46c740
#define A5_CUR_FONT 0x55b140
#define A5_LINE_H 0x55b034
#define A5_PEN_X 0x55b144
#define A5_PEN_Y 0x55b148
#define A5_MAIN_SURF 0x55acc4
#define A5_OFS_X 0x55b138 /* draw_text offsets the rect on the main surface */
#define A5_OFS_Y 0x55b13c
#define A5_BLIT_OX 0x55acd4 /* the blit adds this on the main surface */
#define A5_BLIT_OY 0x55acd8
#define A5_CLIP 0x4faee8 /* left, top, right, bottom (dwords) */
#define A5_CHARMAP 0x49d2c8
static const BYTE kDrawTextOrig[] = {0x55, 0x8B, 0xEC, 0x81, 0xEC, 0x3C, 0x05, 0x00, 0x00};
static const BYTE kTextWidthOrig[] = {0x55, 0x8B, 0xEC, 0x81, 0xEC};

typedef struct
{
    WORD ch;      // ASCII byte (ch < 0x100) or UTF-16 code of a Japanese character
    BYTE cjk;     // 1 = drawn with GDI
    BYTE brk;     // 1 = a line may break before this unit
    short x;      // start position in the line
    short w;      // advance
} TUnit;

#define MAX_UNITS 512
static HFONT g_jfont[32]; // by game font size
static HDC g_measureDC;

static const BYTE *font_ptr(void) { return *(const BYTE **)VA(A5_CUR_FONT); }
static short fword(const BYTE *f, int off) { return *(const short *)(f + off); }
static int glyph_index(BYTE c) { return ((const short *)VA(A5_CHARMAP))[(signed char)c]; }

static HFONT jfont(int size)
{
    if (size < 8)
        size = 8;
    if (size > 31)
        size = 31;
    if (!g_jfont[size])
    {
        WCHAR face[64];
        MultiByteToWideChar(CP_ACP, 0, g_fontFace, -1, face, 64);
        g_jfont[size] = CreateFontW(-(size + g_sizeAdjust), 0, 0, 0, g_bold ? FW_BOLD : FW_NORMAL, 0, 0, 0,
                                    SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                    DEFAULT_PITCH, face);
    }
    return g_jfont[size];
}

static int jwidth(int size, WCHAR c)
{
    SIZE sz;
    if (!g_measureDC)
        g_measureDC = CreateCompatibleDC(NULL);
    SelectObject(g_measureDC, jfont(size));
    if (!GetTextExtentPoint32W(g_measureDC, &c, 1, &sz))
        return size;
    return sz.cx;
}

static int is_kinsoku_start(WCHAR c) // may not begin a line
{
    return wcschr(L"、。，．・：；？！ー―…‥）」』】〉》〕］｝ぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ々～〜", c) != NULL;
}
static int is_kinsoku_end(WCHAR c) // may not end a line
{
    return wcschr(L"（「『【〈《〔［｛", c) != NULL;
}

static unsigned char mac_fix(unsigned char c)
{
    switch (c)
    {
    case 0xc4: case 0xd0: return '-';
    case 0xd2: case 0xd3: return '"';
    case 0xd4: case 0xd5: return '\'';
    }
    return c > 0x7f ? ' ' : c;
}

// Split s into units; ASCII widths as the game measures them (spacing and kerning pairs).
static int make_units(const unsigned char *s, TUnit *u, int max, int size)
{
    const BYTE *f = font_ptr();
    int n = 0, pos = 0, prevAscii = -1;
    while (*s && n < max)
    {
        TUnit *t = &u[n];
        t->brk = 0;
        if (s[0] >= 0xa1 && s[0] <= 0xfe && s[1] >= 0xa1 && s[1] <= 0xfe)
        {
            WCHAR w;
            if (MultiByteToWideChar(GAME_CP, 0, (const char *)s, 2, &w, 1) != 1)
                w = L'?';
            t->ch = w;
            t->cjk = 1;
            if (n > 0)
                pos += fword(f, 0xa);
            t->x = (short)pos;
            t->w = (short)jwidth(size, w);
            pos += t->w;
            prevAscii = -1;
            s += 2;
        }
        else if (s[0] == 0x8e && s[1] >= 0xa1 && s[1] <= 0xdf) // half-width katakana
        {
            WCHAR w;
            if (MultiByteToWideChar(GAME_CP, 0, (const char *)s, 2, &w, 1) != 1)
                w = L'?';
            t->ch = w;
            t->cjk = 1;
            if (n > 0)
                pos += fword(f, 0xa);
            t->x = (short)pos;
            t->w = (short)jwidth(size, w);
            pos += t->w;
            prevAscii = -1;
            s += 2;
        }
        else
        {
            unsigned char c = mac_fix(s[0]);
            int g = glyph_index(c);
            t->ch = c;
            t->cjk = 0;
            if (n > 0)
            {
                if (prevAscii >= 0 && fword(f, 0xc + g * 8 + 6) > 0 && fword(f, 0xc + glyph_index((BYTE)prevAscii) * 8 + 4) > 0)
                    pos -= fword(f, 0xc + glyph_index((BYTE)prevAscii) * 8 + 4);
                else
                    pos += fword(f, 0xa);
            }
            t->x = (short)pos;
            t->w = (short)(fword(f, 0xc + g * 8 + 2) - fword(f, 0xc + g * 8));
            pos += t->w;
            prevAscii = c;
            s++;
        }
        n++;
    }
    // break opportunities
    {
        int i;
        for (i = 1; i < n; i++)
        {
            TUnit *a = &u[i - 1], *b = &u[i];
            if (b->ch == ' ' || b->ch == '|')
                b->brk = 1;
            else if (a->ch == ' ')
                b->brk = 1;
            else if ((a->cjk || b->cjk) && !(b->cjk && is_kinsoku_start(b->ch)) && !(a->cjk && is_kinsoku_end(a->ch)))
                b->brk = 1;
        }
    }
    return n;
}

static void call_draw_line(const char *s, DWORD surf, int r, int g, int b)
{
    DWORD fn = (DWORD)VA(A5_DRAW_LINE);
    __asm {
        push b
        push g
        push r
        push surf
        mov ecx, s
        call fn
        add esp, 16
    }
}

typedef HRESULT(STDMETHODCALLTYPE *SurfGetDC_t)(void *, HDC *);
typedef HRESULT(STDMETHODCALLTYPE *SurfReleaseDC_t)(void *, HDC);

// Draw units [i0, i1) of one line with the pen at (px, py) in surface coordinates of draw_line.
static void draw_units(const TUnit *u, int i0, int i1, int px, int py, DWORD surf, int r, int g, int b, int size)
{
    int i = i0;
    int base = u[i0].x;
    int isMain = (surf == *(DWORD *)VA(A5_MAIN_SURF));
    while (i < i1)
    {
        int j = i;
        if (!u[i].cjk)
        {
            char buf[MAX_UNITS + 1];
            int k = 0;
            while (j < i1 && !u[j].cjk)
                buf[k++] = (char)u[j++].ch;
            buf[k] = 0;
            *(WORD *)VA(A5_PEN_X) = (WORD)(px + u[i].x - base);
            *(WORD *)VA(A5_PEN_Y) = (WORD)py;
            call_draw_line(buf, surf, r, g, b);
        }
        else
        {
            void **vt = *(void ***)surf;
            HDC dc = NULL;
            WCHAR wbuf[MAX_UNITS];
            int k = 0, x = px + u[i].x - base, y = py;
            while (j < i1 && u[j].cjk)
                wbuf[k++] = u[j++].ch;
            if (isMain)
            {
                x += *(short *)VA(A5_BLIT_OX);
                y += *(short *)VA(A5_BLIT_OY);
            }
            if (((SurfGetDC_t)vt[17])((void *)surf, &dc) == S_OK && dc)
            {
                const int *clip = (const int *)VA(A5_CLIP);
                int cy = y + (fword(font_ptr(), 8) - fword(font_ptr(), 4) - size) / 2 + g_yAdjust;
                int kk, cox = 0, coy = 0;
                if (isMain) // the clip rectangle is in game coordinates too
                    cox = *(short *)VA(A5_BLIT_OX), coy = *(short *)VA(A5_BLIT_OY);
                IntersectClipRect(dc, clip[0] + cox, clip[1] + coy, clip[2] + cox, clip[3] + coy);
                SelectObject(dc, jfont(size));
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, RGB(r * 255 / 31, g * 255 / 31, b * 255 / 31));
                for (kk = 0; kk < k; kk++) // each at its own position (game spacing between characters)
                    TextOutW(dc, x + u[i + kk].x - u[i].x, cy, &wbuf[kk], 1);
                ((SurfReleaseDC_t)vt[26])((void *)surf, dc);
            }
        }
        i = j;
    }
}

static const char *g_dtString; // set by the stub

static void __cdecl my_draw_text(DWORD surf, int mode, int size, int r, int g, int b, int left, int top, int right, int bottom)
{
    static TUnit u[MAX_UNITS];
    char str[0x100];
    RECT rc;
    int n, lh;
    const char *src = g_dtString;
    if (!src || !*src)
        return;
    lstrcpynA(str, src, sizeof str);
    rc.left = left, rc.top = top, rc.right = right, rc.bottom = bottom;
    if (!surf)
        surf = *(DWORD *)VA(A5_MAIN_SURF);
    if (surf == *(DWORD *)VA(A5_MAIN_SURF))
        OffsetRect(&rc, *(short *)VA(A5_OFS_X), *(short *)VA(A5_OFS_Y));
    // font by size, as the original
    switch (size)
    {
    case 14: *(DWORD *)VA(A5_CUR_FONT) = 0x49c988 - 0x400000 + (DWORD)g_base; break;
    case 15: *(DWORD *)VA(A5_CUR_FONT) = 0x49c640 - 0x400000 + (DWORD)g_base; break;
    case 16: case 18: size = 16; *(DWORD *)VA(A5_CUR_FONT) = 0x49cc98 - 0x400000 + (DWORD)g_base; break;
    case 20: case 22: case 24: size = 24; *(DWORD *)VA(A5_CUR_FONT) = 0x49cfa8 - 0x400000 + (DWORD)g_base; break;
    default: size = 12; *(DWORD *)VA(A5_CUR_FONT) = 0x49c328 - 0x400000 + (DWORD)g_base; break;
    }
    lh = size + 1;
    *(WORD *)VA(A5_LINE_H) = (WORD)lh;
    if (mode == 2 && strchr(str, '|'))
        mode = 0;
    n = make_units((const unsigned char *)str, u, MAX_UNITS, size);
    if (!n)
        return;
    if (mode == 2)
    {
        draw_units(u, 0, n, rc.left + 2, rc.top + 2, surf, r, g, b, size);
        return;
    }
    if (mode == 1)
    {
        int w = u[n - 1].x; // the original centres on the start of the last character
        int x = (rc.left + rc.right) / 2 - w / 2 + 2;
        int y = (rc.bottom - lh + rc.top) / 2 + 2;
        draw_units(u, 0, n, x, y, surf, r, g, b, size);
        return;
    }
    if (mode == 0)
    {
        int limit = rc.right - rc.left - 6, start = 0, lastBrk = -1, i, y = rc.top + 2, lines = 0;
        for (i = 0; i <= n; i++)
        {
            int forced = (i < n && u[i].ch == '|');
            int over = (i < n && i > start && u[i].x + u[i].w - u[start].x > limit);
            if (i == n || forced || over)
            {
                int end = i;
                if (over && !forced && lastBrk > start)
                    end = lastBrk;
                // trim blanks at both ends of the line
                {
                    int a = start, z = end;
                    while (a < z && (u[a].ch == ' ' || u[a].ch == '|'))
                        a++;
                    while (z > a && u[z - 1].ch == ' ')
                        z--;
                    if (z > a)
                        draw_units(u, a, z, rc.left + 2, y, surf, r, g, b, size);
                }
                if (i == n)
                    break;
                lines++;
                y += size + g_lineGap;
                *(WORD *)VA(A5_LINE_H) = (WORD)(*(WORD *)VA(A5_LINE_H) + size + g_lineGap);
                start = end;
                while (start < n && (u[start].ch == ' ' || u[start].ch == '|'))
                    start++;
                lastBrk = -1;
                i = start;
                if (i >= n)
                    break;
            }
            if (i < n && u[i].brk && i > start)
                lastBrk = i;
        }
    }
}

__declspec(naked) static void draw_text_stub(void)
{
    __asm {
        mov g_dtString, ecx
        jmp my_draw_text
    }
}

static int __cdecl my_text_width(const char *s)
{
    static TUnit u[MAX_UNITS];
    int n;
    if (!s || !*s)
        return 0;
    n = make_units((const unsigned char *)s, u, MAX_UNITS, 12);
    return n ? u[n - 1].x : 0;
}

static void jump_to(DWORD va, void *fn, const BYTE *expect, int len, const char *name)
{
    BYTE *p = (BYTE *)VA(va);
    DWORD old;
    if (memcmp(p, expect, len) != 0)
    {
        jp_log("%s: unexpected bytes at %06x (not the Steam build?), Japanese text is NOT drawn\n", name, va);
        return;
    }
    VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old);
    p[0] = 0xE9;
    *(DWORD *)(p + 1) = (DWORD)fn - (DWORD)(p + 5);
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    jp_log("%s replaced\n", name);
}

static void install_text_draw(void)
{
    jump_to(A5_DRAW_TEXT, (void *)draw_text_stub, kDrawTextOrig, sizeof kDrawTextOrig, "draw_text");
    jump_to(A5_TEXT_WIDTH, (void *)my_text_width, kTextWidthOrig, sizeof kTextWidthOrig, "text_width");
}

// ---------------------------------------------------------------- start-up
static void install_text_hooks(void)
{
    g_realMB2WC = (MB2WC_t)patch_iat("kernel32.dll", "MultiByteToWideChar", (void *)my_MultiByteToWideChar);
    g_realWC2MB = (WC2MB_t)patch_iat("kernel32.dll", "WideCharToMultiByte", (void *)my_WideCharToMultiByte);
    g_realLoadStringW = (LoadStringW_t)patch_iat("user32.dll", "LoadStringW", (void *)my_LoadStringW);
    jp_log("hooks: MultiByteToWideChar %s, WideCharToMultiByte %s, LoadStringW %s\n",
           g_realMB2WC ? "ok" : "MISSING", g_realWC2MB ? "ok" : "MISSING", g_realLoadStringW ? "ok" : "MISSING");
    if (!g_realMB2WC)
        g_realMB2WC = MultiByteToWideChar;
    if (!g_realWC2MB)
        g_realWC2MB = WideCharToMultiByte;
    if (!g_realLoadStringW)
        g_realLoadStringW = LoadStringW;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        char dir[MAX_PATH], data[MAX_PATH], path[MAX_PATH], *p;
        DWORD attr;
        DisableThreadLibraryCalls(inst);
        GetModuleFileNameA(inst, dir, MAX_PATH);
        p = strrchr(dir, '\\');
        if (p)
            *p = 0;
        _snprintf(data, sizeof data, "%s\\avernum_jp", dir);
        attr = GetFileAttributesA(data);
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
            lstrcpynA(data, dir, MAX_PATH);
        _snprintf(path, sizeof path, "%s\\avernum_jp.log", data);
        g_log = fopen(path, "w");
        load_config(dir);
        g_base = (BYTE *)GetModuleHandleA(NULL);
        jp_log("Avernum 5 JP hook loaded. base=%p face=%s bold=%d size_adjust=%d y_adjust=%d line_gap=%d\n", g_base,
               g_fontFace, g_bold, g_sizeAdjust, g_yAdjust, g_lineGap);
        install_text_hooks();
        install_token_estimate_fix();
        install_text_draw();
        load_string_table(data);
        load_exe_strings(data);
        prepare_redirects(dir, data);
    }
    return TRUE;
}
