"""Render the 512x512 Workshop preview image using the game's own Oswald font."""
import sys
from PIL import Image, ImageDraw, ImageFont
font_path, out = sys.argv[1], sys.argv[2]
img = Image.new('RGB', (512, 512), (24, 26, 22))
d = ImageDraw.Draw(img)
big, mid, small = (ImageFont.truetype(font_path, s) for s in (96, 44, 30))
d.rectangle([16, 16, 495, 495], outline=(196, 160, 72), width=4)
d.text((256, 140), 'WORTH', font=big, fill=(230, 222, 200), anchor='mm')
d.text((256, 240), 'PER WEIGHT', font=mid, fill=(196, 160, 72), anchor='mm')
for i, (n, v) in enumerate([('Medkit', '2k'), ('Bandage', '4k'), ('Flash', '40k')]):
    d.text((60, 320 + i * 48), n, font=small, fill=(200, 200, 190))
    d.text((452, 320 + i * 48), f'{v} ₭/kg', font=small, fill=(230, 222, 200), anchor='ra')
img.save(out, quality=90)
