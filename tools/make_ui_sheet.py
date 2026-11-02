#!/usr/bin/env python3
"""make_ui_sheet.py - a small sprite sheet of UI pieces for tdui scripts.

Writes examples/python/ui_sheet.ppm (binary PPM, 64 x 16 pixels): four
16 x 16 pieces side by side, on magenta (255, 0, 255), which scripts load
as transparent (Image.load(..., key=(255, 0, 255))):

    x  0  button        rounded, light at the top; nine-slice border 4
    x 16  button down   the same, darker: the pressed look
    x 32  panel         dark, rounded, a light rim; nine-slice border 4
    x 48  LED           a round light with a shine

A script draws a piece with canvas.nine(sheet, x, y, w, h, 4,
src=(0, 0, 16, 16)) at any size, or canvas.image(sheet, x, y,
src=(48, 0, 16, 16)). Change the pieces here and run:

    python tools/make_ui_sheet.py
"""
import os

W, H = 64, 16
KEY = (255, 0, 255)


def rounded(x, y, w, h, r):
    """Is pixel (x, y) inside a w x h rectangle with corners of radius r?"""
    cx = r if x < r else w - r - 1 if x >= w - r else x
    cy = r if y < r else h - r - 1 if y >= h - r else y
    return (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r


def mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def button(top, bottom, rim):
    def px(x, y):
        if not rounded(x, y, 16, 16, 4):
            return KEY
        if not all(rounded(x + dx, y + dy, 16, 16, 4) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            return rim
        return mix(top, bottom, y / 15)
    return px


def panel(x, y):
    if not rounded(x, y, 16, 16, 4):
        return KEY
    if not all(rounded(x + dx, y + dy, 16, 16, 4) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
        return (150, 160, 175)
    return mix((52, 58, 70), (36, 40, 50), y / 15)


def led(x, y):
    d2 = (x - 7.5) ** 2 + (y - 7.5) ** 2
    if d2 > 7.5 ** 2:
        return KEY
    if (x - 5.5) ** 2 + (y - 5) ** 2 < 4:
        return (210, 255, 210)          # the shine
    return mix((40, 230, 90), (10, 120, 40), d2 / 56)


PIECES = [
    button((120, 190, 255), (30, 100, 200), (15, 50, 110)),
    button((30, 90, 180), (20, 60, 130), (10, 35, 80)),
    panel,
    led,
]


def main():
    out = bytearray(b"P6\n%d %d\n255\n" % (W, H))
    for y in range(H):
        for x in range(W):
            out += bytes(PIECES[x // 16](x % 16, y))
    path = os.path.join(os.path.dirname(__file__), "..", "examples", "python", "ui_sheet.ppm")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(out)
    print("wrote", os.path.normpath(path), len(out), "bytes")


if __name__ == "__main__":
    main()
