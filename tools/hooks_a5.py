"""Hook points of Avernum 5.exe (Steam build) for gen_hooks.py."""

PATTERNS = [
    ('draw_text', 0x46c890, 0x46ca08),   # up to the font selection and the line height
    ('text_width', 0x46c740, 0x46c7a0),
    ('draw_line', 0x46d7d0, 0x46d838),
    ('blit', 0x4041a0, 0x404240),        # blit to the main surface: clip rectangle and offset
    ('tok_count', 0x45c660, 0x45c6d0),   # script token estimate: byte classes
]

VALUES = [
    ('draw_text', 'draw_text', 'start', 0x46c890),
    ('text_width', 'text_width', 'start', 0x46c740),
    ('draw_line', 'draw_line', 'start', 0x46d7d0),
    ('main_surf', 'draw_text', 'abs', 0x55acc4),
    ('ofs_x', 'draw_text', 'abs', 0x55b138),
    ('ofs_y', 'draw_text', 'abs', 0x55b13c),
    ('cur_font', 'draw_text', 'abs', 0x55b140),
    ('font14', 'draw_text', 'abs', 0x49c988),
    ('font15', 'draw_text', 'abs', 0x49c640),
    ('font16', 'draw_text', 'abs', 0x49cc98),
    ('font24', 'draw_text', 'abs', 0x49cfa8),
    ('font12', 'draw_text', 'abs', 0x49c328),
    ('line_h', 'draw_text', 'abs', 0x55b034),
    ('pen_x', 'draw_line', 'abs', 0x55b144),
    ('pen_y', 'draw_line', 'abs', 0x55b148),
    ('charmap', 'draw_line', 'abs', 0x49d2c8),
    ('clip', 'blit', 'abs', 0x4faee8),
    ('blit_ox', 'blit', 'abs', 0x55acd4),
    ('blit_oy', 'blit', 'abs', 0x55acd8),
    ('tok_class', 'tok_count', 'at', 0x45c695),
    ('tok_digit', 'tok_count', 'at', 0x45c69d),
    ('tok_other', 'tok_count', 'at', 0x45c6a4),
    ('tok_letter', 'tok_count', 'at', 0x45c6c5),
]
