#!/usr/bin/env python3
"""make_ui_skin.py - a UI skin for pictures (canvas.picture()): buttons,
tabs and panels drawn smooth at the screen's own resolution.

Writes examples/python/ui_skin.ppm (binary PPM, 256 x 128 pixels): eight
64 x 64 pieces on magenta (255, 0, 255), which scripts load as
transparent (Image.load(..., key=(255, 0, 255), dither=False)). Each is drawn
four times larger and averaged within the shape. Uncovered pixels stay
transparent; no edge is blended with a guessed background. Draw with a nine-slice
border of 16:

    x   0  y  0  button          x   0  y 64  panel
    x  64  y  0  button pressed  x  64  y 64  alarm (red)
    x 128  y  0  tab             x 128  y 64  OK (green)
    x 192  y  0  tab, active     x 192  y 64  Cancel (grey)

    python tools/make_ui_skin.py
"""
import os

SIZE, SS, RADIUS = 64, 4, 14
KEY = (255, 0, 255)


def mix(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def inside(x, y, w, h, r):
    cx = min(max(x, r), w - r)
    cy = min(max(y, r), h - r)
    return (x - cx) ** 2 + (y - cy) ** 2 <= r * r


def piece(top, bottom, rim, shine, inset=False):
    """A rounded glossy piece: a gradient, a rim and a lit top edge."""
    n = SIZE * SS
    r = RADIUS * SS
    rim_w = 2 * SS
    out = []
    for py in range(SIZE):
        row = []
        for px in range(SIZE):
            acc, cover = [0.0, 0.0, 0.0], 0
            for sy in range(SS):
                for sx in range(SS):
                    x, y = px * SS + sx + 0.5, py * SS + sy + 0.5
                    if not inside(x, y, n, n, r):
                        continue
                    cover += 1
                    t = y / n
                    if inset:
                        t = 1 - t
                    c = mix(top, bottom, t)
                    if not inside(x, y, n, n, r) or not all(
                            inside(x + dx, y + dy, n, n, r) for dx, dy in ((rim_w, 0), (-rim_w, 0), (0, rim_w), (0, -rim_w))):
                        c = rim
                    elif y < rim_w + 2 * SS and not inset:
                        c = mix(c, shine, 0.6)       # the light along the top
                    elif y > n * 0.5 and not inset:
                        c = mix(c, bottom, 0.3)      # a little deeper below the middle
                    for i in range(3):
                        acc[i] += c[i]
            if cover < SS * SS // 2:
                row.append(KEY)
            else:
                c = tuple(v / cover for v in acc)
                row.append(tuple(int(round(v)) for v in c))
        out.append(row)
    return out


PIECES = [
    piece((96, 160, 255), (24, 78, 180), (14, 40, 100), (200, 230, 255)),              # button
    piece((96, 160, 255), (20, 60, 150), (10, 30, 80), (150, 190, 255), inset=True),   # pressed
    piece((70, 92, 140), (34, 48, 86), (20, 28, 54), (150, 170, 210)),                 # tab
    piece((255, 226, 110), (222, 160, 20), (130, 90, 0), (255, 248, 200)),             # tab, active
    piece((38, 62, 118), (20, 34, 74), (80, 120, 200), (90, 130, 210)),                # panel
    piece((255, 110, 100), (190, 24, 24), (110, 10, 10), (255, 200, 200)),             # alarm
    piece((110, 235, 130), (24, 150, 60), (10, 80, 30), (210, 255, 220)),              # OK
    piece((190, 196, 206), (104, 110, 122), (60, 64, 72), (240, 242, 246)),            # Cancel
]


def main():
    w, h = 4 * SIZE, 2 * SIZE
    out = bytearray(b"P6\n%d %d\n255\n" % (w, h))
    for y in range(h):
        for x in range(w):
            out += bytes(PIECES[(y // SIZE) * 4 + x // SIZE][y % SIZE][x % SIZE])
    path = os.path.join(os.path.dirname(__file__), "..", "examples", "python", "ui_skin.ppm")
    with open(path, "wb") as f:
        f.write(out)
    print("wrote", os.path.normpath(path), len(out), "bytes")


if __name__ == "__main__":
    main()
