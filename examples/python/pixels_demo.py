# pixels_demo.py - pictures and pixels in a tdui window.
#
#     py pixels_demo.py      (ui_sheet.ppm next to it)
#
# A gradient sky, a sun, a panel with a number in big pixel text, round
# buttons drawn in colours and one from a small sprite sheet
# (tools/make_ui_sheet.py), and text over the pixels. Every shape, picture
# and text is one call: the desktop draws the pixels, not Python.

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
cv = win.canvas(0, 0, W, H)
count = 0


def draw_count():
    # The panel, then the number in the 8 x 16 font, centred on it.
    cv.nine(sheet, 60, 8, 44, 20, 4, src=PANEL)
    s = str(count)
    tw, th = tdui.text_size(s)
    cv.draw_text(82 - tw // 2, 18 - th // 2, s, YELLOW)


def change(n):
    global count
    count = max(0, min(999, count + n))
    draw_count()


# The sky: one gradient, then a sun of two circles and the ground.
cv.gradient(0, 0, cv.pwidth, cv.pheight, (20, 40, 110), (250, 150, 90))
cv.circle(96, 7, 5, tdui.rgb(255, 230, 120))
cv.circle(96, 7, 7, tdui.rgb(255, 200, 80), outline=True)
cv.rect(0, cv.pheight - 6, cv.pwidth, 6, tdui.rgb(30, 70, 40))
cv.put(2, 1, "Pictures and pixels in a tdui window", WHITE, tdui.CLEAR)
cv.image(sheet, 4, 6, 8, 8, src=LED)
# The small font at scale 2: with Normal pictures a cell shows one of its
# two pixel columns, which a 3-pixel letter does not survive at scale 1.
cv.draw_text(14, 6, "ONLINE", tdui.rgb(120, 255, 140), scale=2, small=True)
draw_count()

# Round buttons in colours (rim: an outline), and one from the sheet.
rim = tdui.rgb(170, 210, 255)
cv.button(3, 13, 10, 3, "- 1", lambda b: change(-1), rim=rim)
cv.button(15, 13, 10, 3, "+ 1", lambda b: change(1), rim=rim)
cv.button(42, 13, 10, 3, "Quit", lambda b: win.close(), sheet=sheet, src=BUTTON, src_down=DOWN)

tdui.run()
