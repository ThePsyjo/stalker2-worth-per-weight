"""Render the 512x512 Workshop preview: a tooltip-style card using the game's own Oswald font.

    python3 -I tools/preview.py <Oswald-Medium.ufont> <out.jpg>
"""
import sys
from PIL import Image, ImageDraw, ImageFont

font_path, out = sys.argv[1], sys.argv[2]
W = H = 512
BG, PANEL, HEADER = (18, 19, 17), (27, 28, 25), (22, 38, 33)
TEXT, DIM, GOLD, ORANGE = (230, 222, 200), (150, 148, 138), (196, 160, 72), (232, 150, 40)
f = lambda s: ImageFont.truetype(font_path, s)

img = Image.new('RGB', (W, H), BG)
d = ImageDraw.Draw(img)
d.text((W // 2, 62), 'WORTH PER WEIGHT', font=f(52), fill=GOLD, anchor='mm')

# tooltip card
x0, y0, x1, y1 = 28, 112, W - 28, H - 28
d.rectangle([x0, y0, x1, y1], fill=PANEL, outline=(70, 70, 62), width=2)
d.rectangle([x0 + 2, y0 + 2, x1 - 2, y0 + 62], fill=HEADER)
d.text((W // 2, y0 + 33), 'SEVA-I SUIT', font=f(32), fill=TEXT, anchor='mm')

# weight left, "<value>/kg · <price> ₭" right, like the game's tooltip row
row = y0 + 118
d.text((x0 + 22, row), '7.0 kg', font=f(40), fill=TEXT, anchor='lm')
price_font = f(40)
coupon = '₭'
right = x1 - 22
cw = d.textlength(coupon, font=price_font)
d.text((right, row), coupon, font=price_font, fill=ORANGE, anchor='rm')
d.text((right - cw - 6, row), '45k/kg · 312750', font=price_font, fill=TEXT, anchor='rm')

# a few more rows to read as "every item"
rows = [('Medkit', '2k/kg · 189'), ('Flash', '40k/kg · 12000'), ('Bread', '<1k/kg · 28')]
y = row + 80
d.line([x0 + 22, y - 30, x1 - 22, y - 30], fill=(70, 70, 62), width=1)
for name, value in rows:
    d.text((x0 + 22, y), name, font=f(30), fill=DIM, anchor='lm')
    vw = d.textlength(coupon, font=f(30))
    d.text((right, y), coupon, font=f(30), fill=ORANGE, anchor='rm')
    d.text((right - vw - 5, y), value, font=f(30), fill=TEXT, anchor='rm')
    y += 54

img.save(out, quality=90)
