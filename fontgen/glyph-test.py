import os
from collections import deque

from PIL import Image, ImageDraw, ImageFont  # pyright: ignore[reportMissingImports]

chars = deque([chr(i) for i in range(32, 127)])

font_filename = os.path.join(os.path.dirname(__file__), "Roboto-Medium.ttf")
font = ImageFont.truetype(font_filename, 48)

max_y = max((font.getbbox(char)[3] for char in chars))
box_width = max_y + 5
box_height = max_y + 5

image = Image.new("RGB", (box_width * 10, box_height * 10))
draw = ImageDraw.Draw(image)

for y in range(0, 10):
    for x in range(0, 10):
        if not chars:
            break
        char = chars.popleft()
        x_offset = x * box_width
        y_offset = y * box_height
        bbox = font.getbbox(char)
        draw.rectangle((x_offset, y_offset, x_offset + box_width, y_offset + box_height), outline="red")
        draw.rectangle(
            (x_offset + bbox[0], y_offset + bbox[1], x_offset + bbox[2], y_offset + bbox[3]), outline="yellow"
        )
        draw.text((x_offset, y_offset), char, font=font, fill="white")

image.save("canvas.jpg", "JPEG")
