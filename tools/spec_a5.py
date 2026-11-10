"""Boxes and texts of the Avernum 5 pictures with English text (used by make_images.py).

Coordinates were measured on the original pictures (_backup_original/Data or orig_tree/Data).
Most of these BMPs are 8-bit with pure white as the transparent colour; they are drawn over
parchment, so letters are antialiased against PARCH and the white around them is kept
("key_white").
"""
import os

from PIL import Image

FONTS = 'C:/Windows/Fonts/'
MINCHO_B = FONTS + 'yumindb.ttf'       # Yu Mincho Demibold
GOTHIC_B = FONTS + 'YuGothB.ttc'       # Yu Gothic Bold
GOTHIC = FONTS + 'YuGothM.ttc'         # Yu Gothic Medium

PARCH = (242, 237, 230)    # parchment behind the pictures
INK = (0, 0, 0)            # help panels: black text
TITLE_INK = (44, 0, 0)     # help panel titles: very dark red-brown

PAGES = {}
PAGE_BOX = (0, 0, 0, 0)
PAGE_INK = (0, 0, 0)

SRC = None   # set by make_images (orig dir) when needed


def _size(name):
    import os
    for d in ('orig_tree/Data', '../_backup_original/Data', '../Data'):
        p = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d, name)
        if os.path.exists(p):
            return Image.open(p).size
    raise FileNotFoundError(name)


A5_HEAD = {'style': 'a4head', 'edge': (238, 238, 230), 'top': (112, 112, 106), 'bottom': (38, 38, 38)}
A5_BUTTON = {'style': 'a4button', 'edge': (238, 238, 230), 'top': (96, 96, 92), 'bottom': (30, 30, 30)}
A4_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'Avernum 4', '_jp_mod', 'img_out')
COPY = {'G188.bmp': os.path.join(A4_DIR, 'G188.bmp'), 'G216.bmp': os.path.join(A4_DIR, 'G216.bmp')}


def heading(name, text, pad=1):
    w, h = _size(name)
    return dict(A5_HEAD, erase='white', key_white=True,
                texts=[((pad, pad, w - pad - 1, h - pad - 1), [text], MINCHO_B, h - 3, None, 'left')])


SPEC = {
    'G150.bmp': heading('G150.bmp', 'パーティーの編成'),
    'G151.bmp': heading('G151.bmp', '新しいキャラクター'),
    'G152.bmp': heading('G152.bmp', '能力値'),
    'G153.bmp': heading('G153.bmp', '呪文・能力'),
    'G155.bmp': heading('G155.bmp', 'ゲームをセーブ'),
    'G156.bmp': heading('G156.bmp', 'セーブデータのロード'),
    'G159.bmp': heading('G159.bmp', '記録'),
    'G160.bmp': heading('G160.bmp', 'アヴァーナム5 操作説明'),
    'G162.bmp': heading('G162.bmp', 'パーティーの作成'),
    'G164.bmp': heading('G164.bmp', '製品登録'),
    'G183.bmp': heading('G183.bmp', 'ゲーム設定'),
    'G184.bmp': heading('G184.bmp', '転移門の使用'),
}


def rows(texts, x0, x1, y0, step, align='left', size=20):
    out, fills = [], []
    for i, t in enumerate(texts):
        y = y0 + i * step
        fills.append(((x0, y + 1, x1, y + step), (255, 255, 255, 255)))
        if t:
            out.append(((x0 + 2, y + 2, x1 - 2, y + step - 1), [t], MINCHO_B, size, None, align))
    return out, fills


_t202, _f202 = rows(['職業の変更', '名前の変更', '能力値の編集', '特性の編集', '削除', '新しいキャラクター',
                     '難易度：イージー', '難易度：ノーマル', '難易度：ハード', '難易度：トーメント'], 1, 223, 0, 26, size=20)
_t203l, _f203l = rows(['兵士', '狂戦士', '司祭', '妖術師', 'ならず者', '射手', '斥候', '田舎魔術師', 'シャーマン',
                       'カスタム'], 1, 223, 0, 24, align='center', size=19)
_t203r, _f203r = rows(['ゲームに戻る', '', 'ゲームをセーブ', 'セーブデータのロード', '', '設定', 'アヴァーナム5について', '',
                       'タイトル画面に戻る', '終了'], 224, 446, 0, 24, align='center', size=19)
_menu = [(34, 60, '新しいゲーム'), (76, 103, 'ゲームをロード'), (117, 144, '購入方法'), (160, 193, '製品登録'),
         (202, 228, '操作説明'), (244, 271, 'ゲーム設定'), (286, 312, 'キャラクターエディター'), (328, 360, '終了')]

SPEC.update({
    'G202.bmp': dict(A5_BUTTON, erase='none', key_white=True, fill_rects=_f202, texts=_t202),
    'G203.bmp': dict(A5_BUTTON, erase='none', key_white=True, fill_rects=_f203l + _f203r, texts=_t203l + _t203r),
    'G158.bmp': {'erase': 'none', 'key_white': True, 'style': 'a4menu', 'parch': (40, 30, 25),
                 'edge': (49, 35, 30), 'top': (246, 226, 202), 'bottom': (206, 166, 132), 'bold': 1,
                 'fill_rects': [((24, y0 - 3, 336, y1 + 6), (255, 255, 255, 255)) for y0, y1, _ in _menu],
                 'texts': [((26, y0 - 2, 334, y1 + 2), [t], MINCHO_B, 30, None, 'center') for y0, y1, t in _menu]},
    'G163.bmp': dict(A5_HEAD, erase='none', bg_from=('G141.bmp', (24, 40, 292, 90)),
                     texts=[((4, 2, 272, 44), ['依頼掲示板'], MINCHO_B, 40, None, 'left')]),
})

HELP_INK = (44, 0, 0)


def panel(title_box, title, blocks, body=13):
    texts = [(title_box, [title], MINCHO_B, 21, HELP_INK, 'left')]
    for b in blocks:
        texts.append((b[0], b[1], GOTHIC_B, b[2] if len(b) > 2 else body, HELP_INK, 'left'))
    return {'erase': 'fill', 'fill_colour': (255, 255, 255, 255), 'key_white': True, 'no_grid': True, 'texts': texts}


SPEC.update({
    'G185.bmp': panel((2, 4, 215, 28), 'クエストを見る', [
        ((2, 37, 212, 84), ['受け取ったクエストを', '見るには……', '1. 日誌ボタンを押す。']),
        ((160, 100, 296, 130), ['2. 上の「現在のクエスト」', 'を選ぶ。'])]),
    'G187.bmp': panel((2, 4, 150, 26), '移動のしかた', [
        ((2, 49, 262, 66), ['行きたい場所をクリックする。']),
        ((2, 89, 262, 122), ['テンキーや矢印キーでも', '移動できる。'])]),
    'G190.bmp': panel((2, 4, 150, 28), 'アイテムを拾う', [
        ((2, 49, 294, 114), ['1. アイテムをクリックして拾う。', '2. 装備するにはキャラクターをクリック。',
                             '3. 背負い袋に入れるには、', '　空いているマスをクリック。'])]),
    'G191.bmp': panel((2, 4, 296, 28), 'クイック使用欄・アイテムの使用', [
        ((2, 49, 296, 95), ['ここに置くとクイック使用に設定できる。', 'メイン画面でクリックすると', '使える。']),
        ((2, 113, 196, 143), ['アイテムを使うには……', '1. 使用ボタンをクリック：'])]),
    'G192.bmp': panel((2, 4, 212, 28), '呪文の唱え方', [
        ((2, 37, 205, 70), ['1. 魔術か祈祷の', 'ボタンを押す。']),
        ((156, 68, 318, 149), ['2. 唱えたい呪文を', 'クリックする。', '（呪文にカーソルを', '合わせると説明が', '出る。）'])]),
    'G193.bmp': panel((2, 4, 150, 28), '物を使う', [
        ((2, 49, 240, 98), ['扉をクリックすると開く。', '箱をクリックすると中を見る。', 'レバーなら引く、など。'])]),
    'G196.bmp': panel((2, 4, 150, 28), '戦い方', [
        ((12, 28, 205, 74), ['1. このボタンで攻撃の', '種類を切り替える', '（近接武器か飛び道具）。']),
        ((194, 84, 318, 116), ['2. 敵をクリックして', '攻撃する。'])]),
    'G197.bmp': panel((2, 4, 200, 30), '戦闘修練', [
        ((2, 37, 150, 70), ['1. 戦闘修練の', 'ボタンを押す。']),
        ((152, 68, 318, 150), ['2. 使う修練を選ぶ。', '（疲労は、次の修練を', '使えるようになるまでの', 'ターン数。）'])]),
    'G210.bmp': panel((2, 4, 200, 28), 'ゲームのセーブ', [
        ((2, 37, 188, 52), ['1. ディスクボタンを押す。']),
        ((146, 69, 298, 115), ['3. 枠を選び、名前を', '入れて OK ボタンを', '押す。']),
        ((2, 132, 158, 148), ['2. 「セーブ」を選ぶ。'])]),
    'G212.bmp': panel((2, 4, 216, 28), '新しいゲームの始め方', [
        ((2, 29, 170, 62), ['手軽に新しいゲームを', '始めるには……']),
        ((2, 89, 232, 104), ['1. 新しい職業を選ぶ']), ((2, 105, 136, 120), ['（必要なら）。']),
        ((2, 121, 124, 136), ['2. 難易度を設定。']), ((2, 137, 196, 152), ['3. OK ボタンを押す。'])]),
    'G213.bmp': panel((2, 4, 216, 28), 'キャラクターの訓練', [
        ((2, 37, 194, 52), ['1. 訓練ボタンを押す。']),
        ((140, 69, 298, 150), ['2. 技能の横の＋ボタン', 'を押して上げる。', '技能の名前をクリック', 'すると説明が出る。'])]),
    'G214.bmp': panel((2, 4, 250, 28), 'アイテムを置く・使う', [
        ((2, 49, 284, 98), ['アイテムを置くには……', '1. クリックして持ち上げる。', '2. 地面の欄をクリックして置く。']),
        ((2, 113, 196, 143), ['アイテムを使うには……', '1. 使用ボタンをクリック：'])]),
    'G217.bmp': panel((2, 6, 214, 44), 'クイック呪文ボタン', [
        ((2, 80, 372, 145), ['クイック呪文ボタンを設定するには……', '1. 設定したいクイック呪文ボタンを押す。',
                             '2. 知っている呪文の一覧が出るので、', '　1つ選ぶ。'])]),
    'G215.bmp': panel((2, 4, 180, 28), '戦闘の終了', [((20, 47, 210, 78), ['戦闘を終えるには、', '戦闘終了ボタンを押す。'])]),
    'G218.bmp': panel((2, 4, 180, 28), '世界地図', [((20, 47, 205, 96), ['いまいる場所を見るには、', '世界地図ボタンを押す。'])]),
})

LEG_INK = (62, 22, 22)


def labels(items, size=12, font=None, ink=LEG_INK, align='left'):
    return [(box, lines, font or GOTHIC_B, size, ink, align) for box, lines in items]


def white(texts):
    return {'erase': 'fill', 'fill_colour': (255, 255, 255, 255), 'key_white': True, 'no_grid': True, 'texts': texts}


_g166 = []
for (x0, x1), names in zip([(20, 84), (108, 172), (192, 268), (284, 364)],
                           [['呪い', '祝福', '減速', '加速', '毒', '棘の盾', '死の呪い'],
                            ['気絶', '魅了', '酸', '恐慌', '盾', '再生', '衰弱'],
                            ['精神の盾', '凍結', '持久の盾', '持久の鎧', '防護', '神の怒り'],
                            ['抵抗力', '強化', '分厚い皮膚', '朦朧', '無敵', '雷']]):
    for k, n in enumerate(names):
        y = int(21 + k * 14.6)
        _g166.append(((x0, y, x1, y + 14), [n]))

SPEC.update({
    'G166.bmp': white([((100, 2, 260, 18), ['魔法の効果'], MINCHO_B, 16, HELP_INK, 'center')] + labels(_g166, 12)),
    'G167.bmp': white(labels([((234, int(35 + k * 17.2), 400, int(50 + k * 17.2)), [t]) for k, t in enumerate(
        ['A - 地形', 'B - パーティー一覧', 'C - 自動地図', 'D - ゲームボタン', 'E - サイドボタン', 'F - 文字欄'])], 12)),
    'G168.bmp': white(labels([((82, 4, 178, 38), ['既定の攻撃', '（sで切替）']), ((214, 4, 268, 38), ['拾う', '（g）']),
                              ((308, 4, 374, 38), ['クイック呪文', '（F5〜F8）']), ((82, 42, 178, 76), ['魔術・祈祷', '（m / p）']),
                              ((214, 42, 268, 76), ['戦闘修練', '（d）']), ((308, 42, 374, 76), ['クイック使用', '（F9〜F12）'])], 12)),
    'G169.bmp': white([((2, 2, 192, 19), ['既定の攻撃ボタン：'], MINCHO_B, 16, HELP_INK, 'left')] +
                      labels([((40, 86, 94, 101), ['近接']), ((96, 86, 160, 101), ['飛び道具'])], 13, align='center')),
    'G288.bmp': white([((2, 3, 300, 24), ['冒険は続く……'], MINCHO_B, 18, HELP_INK, 'left')]),
})

GOLD = (255, 186, 40)
SPEC.update({
    'G132.bmp': {'erase': 'inpaint', 'texts': [
        ((266, 150, 322, 170), ['訓練'], GOTHIC_B, 15, GOLD, 'center', 'outline', 'orange'),
        ((332, 150, 372, 170), ['情報'], GOTHIC_B, 15, GOLD, 'center', 'outline', 'orange')]},
    'G133.bmp': {'erase': 'inpaint', 'texts': [
        ((41, 1, 80, 17), ['購入'], GOTHIC_B, 13, GOLD, 'center', 'outline', 'orange'),
        ((81, 1, 120, 17), ['購入'], GOTHIC_B, 13, (226, 200, 170), 'center', 'outline', 'orange'),
        ((0, 110, 11, 127), [''], GOTHIC_B, 12, (255, 255, 255), 'center', 'light'),
        ((0, 106, 46, 127), ['人間'], GOTHIC_B, 12, (255, 255, 255), 'center', 'outline', 'bw'),
        ((46, 106, 92, 127), ['ネフィル'], GOTHIC_B, 11, (255, 255, 255), 'center', 'outline', 'bw'),
        ((92, 106, 138, 127), ['スリス'], GOTHIC_B, 12, (255, 255, 255), 'center', 'outline', 'bw')]},
})
