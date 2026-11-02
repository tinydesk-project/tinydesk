# pixels_demo.py - pictures and pixels in a tdui window.
#
#     py pixels_demo.py      (ui_sheet.ppm next to it)
#
# A gradient sky, a sun, a panel and three buttons drawn from one small
# sprite sheet (tools/make_ui_sheet.py), with text over the pixels. Every
# shape and picture is one call: the desktop draws the pixels, not Python.

import tdui

W, H = 56, 18                      # cells; pixels are 2 x 2 to a cell
KEY = (255, 0, 255)                # the sheet's transparent colour
BUTTON, DOWN, PANEL, LED = ((x, 0, 16, 16) for x in (0, 16, 32, 48))
WHITE, YELLOW = 231, 226

try:
    here = __file__.rsplit("/", 1)[0] if "/" in __file__ else "."
except NameError:
    here = "."
# Flat UI pieces load without dithering (it is for photos and gradients).
sheet = tdui.Image.load(here + "/ui_sheet.ppm", key=KEY, dither=False)

win = tdui.Window("Pixels", W, H)
count = 0
buttons = []                       # (x, y, w, h in cells, label, action)


def draw_button(b, pressed=False):
    x, y, w, h, label, _ = b
    cv.nine(sheet, 2 * x, 2 * y, 2 * w, 2 * h, 4, src=DOWN if pressed else BUTTON)
    cv.put(x + (w - len(label)) // 2, y + h // 2, label, WHITE, tdui.CLEAR)


def draw_count():
    cv.nine(sheet, 60, 10, 44, 18, 4, src=PANEL)
    cv.big(35, 8, str(count), YELLOW, tdui.CLEAR, width=3)


def change(n):
    global count
    count = max(0, min(999, count + n))
    draw_count()


pressed = None


def on_mouse(c, x, y, button, action):
    # A press shows the button down at once; letting go puts it back up,
    # and does what it says if the mouse is still on it.
    global pressed
    hit = None
    for b in buttons:
        bx, by, bw, bh = b[:4]
        if bx <= x < bx + bw and by <= y < by + bh:
            hit = b
    if action == 0 and button == 0 and hit:
        pressed = hit
        draw_button(hit, True)
    elif action == 1 and pressed:
        draw_button(pressed)
        if hit is pressed:
            pressed[5]()
        pressed = None


cv = win.canvas(0, 0, W, H, on_mouse=on_mouse)

# The sky: one gradient, then a sun of two circles and the ground.
cv.gradient(0, 0, cv.pwidth, cv.pheight, (20, 40, 110), (250, 150, 90))
cv.circle(90, 8, 6, tdui.rgb(255, 230, 120))
cv.circle(90, 8, 8, tdui.rgb(255, 200, 80), outline=True)
cv.rect(0, cv.pheight - 6, cv.pwidth, 6, tdui.rgb(30, 70, 40))
cv.put(2, 1, "Pictures and pixels in a tdui window", WHITE, tdui.CLEAR)
cv.image(sheet, 4, 6, 8, 8, src=LED)

buttons = [
    [4, 13, 10, 3, "- 1", lambda: change(-1)],
    [16, 13, 10, 3, "+ 1", lambda: change(1)],
    [42, 13, 10, 3, "Quit", win.close],
]
for b in buttons:
    draw_button(b)
draw_count()

tdui.run()
