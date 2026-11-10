"""Redraw the place names on the Avernum 5 world map (G287.bmp overlay, G289.bmp title) in Japanese.

usage: python -I make_map_a5.py ORIGINAL_DIR OUTPUT_DIR [PREVIEW_DIR] [--boxes]

Boxes are rough areas in image coordinates; the letters inside are found by colour, erased by
inpainting (pure white stays pure white: it is the transparent colour of the overlay) and the
Japanese name is drawn in the same two styles as the original ("big" region names: dark brown
letters with a golden glow, "small" place names: grey-brown letters with a cream glow).
"|" breaks a label into lines.
"""
import os
import sys

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.stdout.reconfigure(encoding='utf-8')
FONT = 'C:/Windows/Fonts/yumindb.ttf'
STYLES = {
    'big': ((91, 77, 51), (188, 156, 92)),
    'small': ((108, 100, 84), (228, 220, 188)),
}
FILLS = [(91, 77, 51), (108, 100, 84), (108, 100, 76)]

LABELS = [
    ((556, 31, 618, 51), 'エクソダス'),
    ((258, 80, 312, 102), '急流'),
    ((466, 95, 524, 132), 'ドレイク|の柱'),
    ((408, 136, 476, 168), 'ハーキンの|船着き場'),
    ((688, 68, 785, 92), 'トランキリティ'),
    ((8, 192, 86, 228), '北方|諸島'),
    ((124, 174, 190, 203), 'シャンカー|の塔'),
    ((196, 229, 240, 246), '埠頭'),
    ((561, 243, 687, 265), '咆哮の深層'),
    ((750, 250, 824, 271), 'アホナリア'),
    ((394, 299, 481, 319), 'ハイグラウンド'),
    ((111, 359, 206, 379), '新ハーストン'),
    ((682, 372, 746, 406), 'アナマ|の地'),
    ((122, 440, 212, 475), '黒の大裂け目|前哨地'),
    ((459, 460, 524, 497), '紺碧の|回廊'),
    ((174, 497, 220, 517), 'マック'),
    ((78, 551, 151, 585), 'ヴァーナタイ|の地'),
    ((258, 589, 308, 625), '暗黒|の川'),
    ((9, 665, 77, 687), 'タラント'),
]

# G289: title in the lower right corner of the parchment
TITLE_BOX = (466, 652, 742, 698)
TITLE = '北アヴァーナムと|北部の辺境'


def near(a, cols, tol):
    m = np.zeros(a.shape[:2], bool)
    for c in cols:
        m |= (np.abs(a - np.array(c)).max(2) <= tol)
    return m


def fit(lines, w, h, start, lo=10):
    size = start
    while size > lo:
        f = ImageFont.truetype(FONT, size)
        lh = size * 1.12
        if max(f.getlength(t) for t in lines) <= w and lh * len(lines) <= h + 2:
            return f, size, lh
        size -= 1
    return ImageFont.truetype(FONT, lo), lo, lo * 1.12


def draw_label(layer, box, text, start=17):
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    lines = text.split('|')
    f, size, lh = fit(lines, w + 12, h + 4, start)
    d = ImageDraw.Draw(layer)
    y = y0 + (h - lh * len(lines)) / 2
    for t in lines:
        for dx in (0, 0.6):   # a little heavier than the font itself, like the original lettering
            d.text((x0 + (w - f.getlength(t)) / 2 + dx, y + (lh - size) / 2), t, font=f, fill=255, anchor='la')
        y += lh


def to_palette(img, pal, keep_white):
    palarr = np.array(pal.getpalette()[:768], dtype=np.int32).reshape(-1, 3)
    used = sorted(set(np.unique(np.array(pal)).tolist()))
    cand = palarr[used]
    px = np.array(img, dtype=np.int32).reshape(-1, 3)
    cols, inv = np.unique(px, axis=0, return_inverse=True)
    dist = ((cols[:, None, :] - cand[None, :, :]).astype(np.int64) ** 2).sum(2)
    if keep_white:   # only pure white may become the transparent white
        white = np.where((cand == 255).all(1))[0]
        notwhite = ~(cols == 255).all(1)
        dist[np.ix_(notwhite, white)] = 1 << 40
    idx = np.array(used)[dist.argmin(1)]
    res = Image.fromarray(idx[inv.reshape(-1)].reshape(img.size[1], img.size[0]).astype(np.uint8), 'P')
    res.putpalette(pal.getpalette())
    return res


def compose(img, layer, fill, glow, glow_px=2, blur=1.2):
    g = layer.filter(ImageFilter.MaxFilter(glow_px * 2 + 1)).filter(ImageFilter.GaussianBlur(blur))
    g = g.point(lambda v: 255 if v > 70 else 0)   # hard edge: soft edges turn into pale fringes over the parchment
    img = Image.composite(Image.new('RGB', img.size, glow), img, g)
    return Image.composite(Image.new('RGB', img.size, fill), img, layer)


def make_overlay(src, out, preview, show_boxes):
    pal = Image.open(os.path.join(src, 'G287.bmp'))
    rgb = np.array(pal.convert('RGB'))
    a = rgb.astype(int)
    mask = np.zeros(rgb.shape[:2], np.uint8)
    for L in LABELS:
        x0, y0, x1, y1 = L[0]
        hit = near(a[y0:y1, x0:x1], FILLS, 10)
        mask[y0:y1, x0:x1] |= hit.astype(np.uint8) * 255
    mask = cv2.dilate(mask, np.ones((3, 3), np.uint8), iterations=3)
    white = (rgb == 255).all(2)
    bgr = cv2.inpaint(cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR), mask, 4, cv2.INPAINT_TELEA)
    rgb2 = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    # erased pixels that come out almost white are background: make them the transparent white
    # whether an erased pixel is background is guessed separately from the white/non-white map
    wmap = cv2.inpaint(white.astype(np.uint8) * 255, mask, 4, cv2.INPAINT_TELEA)
    nearw = (wmap >= 128) & (mask > 0)
    rgb2[nearw] = 255
    img = Image.fromarray(rgb2)
    for style in STYLES:
        layer = Image.new('L', img.size, 0)
        for L in LABELS:
            x0, y0, x1, y1 = L[0]
            sub = a[y0:y1, x0:x1]
            big = near(sub, [STYLES['big'][0]], 6).sum()
            small = near(sub, [STYLES['small'][0], (108, 100, 76)], 6).sum()
            if ('big' if big > small else 'small') == style:
                draw_label(layer, L[0], L[1])
        img = compose(img, layer, *STYLES[style])
    res = to_palette(img, pal, keep_white=True)
    res.save(os.path.join(out, 'G287.bmp'))
    if preview:
        p = res.convert('RGB')
        if show_boxes:
            d = ImageDraw.Draw(p)
            for L in LABELS:
                d.rectangle(L[0], outline=(255, 0, 0))
        p.save(os.path.join(preview, 'G287_jp.png'))
        bg = Image.open(os.path.join(src, 'G289.bmp')).convert('RGB')
        ov = np.array(res.convert('RGB'))
        comp = np.array(bg)
        keep = ~(ov == 255).all(2)
        comp[keep] = ov[keep]
        Image.fromarray(comp).save(os.path.join(preview, 'G287_on_G289.png'))
    print('wrote G287.bmp')


def make_title(src, out, preview):
    pal = Image.open(os.path.join(src, 'G289.bmp'))
    rgb = np.array(pal.convert('RGB'))
    x0, y0, x1, y1 = TITLE_BOX
    sub = rgb[y0:y1, x0:x1].astype(int)
    lum = sub.mean(2)
    # letters are darker than the parchment around them
    bgl = np.median(lum)
    hit = (lum < bgl - 40).astype(np.uint8) * 255
    mask = np.zeros(rgb.shape[:2], np.uint8)
    mask[y0:y1, x0:x1] = cv2.dilate(hit, np.ones((3, 3), np.uint8), iterations=2)
    bgr = cv2.inpaint(cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR), mask, 6, cv2.INPAINT_TELEA)
    img = Image.fromarray(cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB))
    ink = tuple(int(v) for v in np.median(sub[hit > 0], axis=0)) if hit.any() else (92, 80, 64)
    layer = Image.new('L', img.size, 0)
    draw_label(layer, TITLE_BOX, TITLE, start=20)
    img = compose(img, layer, ink, (236, 226, 196), glow_px=1, blur=0.8)
    res = to_palette(img, pal, keep_white=False)
    res.save(os.path.join(out, 'G289.bmp'))
    if preview:
        res.convert('RGB').save(os.path.join(preview, 'G289_jp.png'))
    print('wrote G289.bmp ink', ink)


def main(argv):
    show = '--boxes' in argv
    argv = [x for x in argv if x != '--boxes']
    src, out = argv[0], argv[1]
    preview = argv[2] if len(argv) > 2 else None
    make_overlay(src, out, preview, show)
    make_title(src, out, preview)


if __name__ == '__main__':
    main(sys.argv[1:])
