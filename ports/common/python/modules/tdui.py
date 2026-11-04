# tdui.py - windows on the TinyDesk desktop from a Python script.
#
#     import tdui
#     w = tdui.Window("Counter", 30, 7)
#     n = w.label(2, 1, "0")
#     def inc(b):
#         n.text = str(int(n.text) + 1)
#     w.button(2, 3, "+1", inc)
#     tdui.run()    # until the window is closed
#
# The desktop makes the windows on its own loop (apps/pyui.c); this module
# sends it requests through _tdui and runs the script's handlers for what
# the user does, in run() or poll(). Requests never wait for an answer:
# objects are numbered here, and widget values come back as events.

import _tdui
import os
import time
from micropython import const
SUPPORTS_GRADIENT_FILLS = const(1)
SUPPORTS_RGB_PICTURES = bool(getattr(_tdui, "SUPPORTS_RGB_PICTURES", False))

# The contract apps are made for (ports/common/python/tdui_contract.json).
# The first number changes only when apps made for the old one would break,
# the second when something is added. CAPABILITIES names the optional parts
# this firmware has; an app package lists the ones it needs and stops with
# a clear message on firmware without them.
API = (1, 0)
CAPABILITIES = ("gradient_fills", "rgb_pictures") if SUPPORTS_RGB_PICTURES else ("gradient_fills",)

def enable_rgb_pictures():
    # Opt in before creating images/canvases. Existing apps keep their budget.
    if not SUPPORTS_RGB_PICTURES:
        return False
    return _tdui.enable_rgb_pictures()

def graphics_info():
    # Cumulative renders/allocation failures/local-sink failures, free PSRAM,
    # largest free PSRAM block, and the last completed frame time in ms.
    return _tdui.graphics_info()

def scanout_info():
    # Local RGB scanout pressure counters, separate from drawing statistics.
    info = getattr(_tdui, "scanout_info", None)
    return info() if info is not None else (0,) * 8

def scanout_timing():
    # Last LCD submission time (monotonic ms), last/peak/total paint work us.
    info = getattr(_tdui, "scanout_timing", None)
    return info() if info is not None else (0,) * 4

def render_cache(kb=128, warm=True):
    # Optional bounded, lossless RGB artwork cache. Zero disables it.
    configure = getattr(_tdui, "render_cache", None)
    if configure is not None:
        return configure(kb, warm)

def render_cache_info():
    # capacity, bytes used, hits, misses, evictions, warmed pictures,
    # raster ms, decode ms, picture frames, total/peak/last picture-frame ms.
    info = getattr(_tdui, "render_cache_info", None)
    return info() if info is not None else (0,) * 12

# Requests and events (td_apps.h TD_PYUI_*).
_OPEN = const(1)
_CLOSE = const(2)
_ADD = const(3)
_TEXT = const(4)
_VALUE = const(5)
_VISIBLE = const(6)
_COLOR = const(7)
_FOCUS = const(8)
_LCLEAR = const(9)
_LADD = const(10)
_PUT = const(11)
_FILL = const(12)
_MSGBOX = const(13)
_INPUT = const(14)
_MENU = const(15)
_RESIZE = const(16)
_MOVE = const(17)
_SHOW = const(18)
_PAGE = const(19)
_PAGE_COPY = const(20)
_BIG = const(21)
_IMAGE = const(22)
_IMAGE_DATA = const(23)
_DRAW_IMAGE = const(24)
_RECT = const(25)
_LINE = const(26)
_GRADIENT = const(27)
_CIRCLE = const(28)
_PTEXT = const(29)
_PICTURE = const(30)
_BATCH_BEGIN = const(31)
_BATCH_END = const(32)
_BAR = const(33)
_FONT = const(34)
_FONT_TEXT = const(35)

_W_LABEL = const(1)
_W_BUTTON = const(2)
_W_CHECK = const(3)
_W_TEXT = const(4)
_W_PROGRESS = const(5)
_W_LIST = const(6)
_W_CANVAS = const(7)

_EV_CLICK = const(1)
_EV_CHANGE = const(2)
_EV_CLOSE = const(3)
_EV_KEY = const(4)
_EV_MOUSE = const(5)
_EV_RESULT = const(6)
_EV_ERROR = const(7)
_EV_RESIZE = const(8)

MAX_WINDOWS = const(4)
MAX_WIDGETS = const(48)
MAX_CELLS = _tdui.MAX_CELLS   # all canvases together, pages counted: 2000, more with PSRAM
MAX_PAGES = const(16)         # pages of one canvas
MAX_PIXELS = _tdui.MAX_PIXELS # bytes of images and canvas pixels together
MAX_PICTURES = getattr(_tdui, "MAX_PICTURES", 192 if MAX_CELLS > 2000 else 48)
MAX_VISIBLE_PICTURES = getattr(_tdui, "MAX_VISIBLE_PICTURES", 64)
CLEAR = const(-2)             # a colour: none of its own (text and blanks over a
                              # canvas's pixels show them; in an image, transparent)

_KEYS = ("enter", "tab", "backspace", "esc", "up", "down", "right", "left", "home", "end",
         "insert", "delete", "pgup", "pgdn", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
         "f9", "f10", "f11", "f12")

_objs = {}       # id -> Window or widget
_results = {}    # dialog id -> (number, text)
_timers = []
_next = 0
_windows = 0
_widgets = 0
_cells = 0


def _new_id():
    global _next
    while True:
        _next = _next % 65535 + 1
        if _next not in _objs and _next not in _results:
            return _next


def _send(op, id, win=0, flags=0, a=0, b=0, c=0, d=0, e=0, f=0, text=None):
    _tdui.send(op, id, win, flags, a, b, c, d, e, f, text)


def _key_name(key, mods):
    if key > 0x110000 and key - 0x110001 < len(_KEYS):
        name = _KEYS[key - 0x110001]
        if mods & 1:
            name = "shift+" + name
    else:
        name = chr(key)
    if mods & 2:
        name = "alt+" + name
    if mods & 4:
        name = "ctrl+" + name
    return name


class Timer:
    # Made by Window.every/after or tdui.every/after; runs in run()/poll().
    def __init__(self, ms, fn, repeat, win=None):
        self.ms = ms
        self.fn = fn
        self.repeat = repeat
        self.win = win
        self.due = time.ticks_add(time.ticks_ms(), ms)
        _timers.append(self)

    def cancel(self):
        if self in _timers:
            _timers.remove(self)


def every(ms, fn):
    return Timer(ms, fn, True)


def after(ms, fn):
    return Timer(ms, fn, False)


class Window:
    def __init__(self, title, w, h, x=None, y=None, resizable=False):
        global _windows
        _tdui.open()
        if _windows >= MAX_WINDOWS:
            raise OSError("tdui: at most %d windows" % MAX_WINDOWS)
        if not (1 <= w <= 300 and 1 <= h <= 150):
            raise ValueError("window size")
        self.id = _new_id()
        self._title = str(title)
        self.width = w
        self.height = h
        self.is_open = True
        self.on_close = None
        self.on_key = None
        self.on_resize = None
        self._kids = []
        _objs[self.id] = self
        _windows += 1
        _send(_OPEN, self.id, 0, 1 if resizable else 0, w, h,
              -1 if x is None else x, -1 if y is None else y, text=self._title)

    @property
    def title(self):
        return self._title

    @title.setter
    def title(self, t):
        self._title = str(t)
        if self.is_open:
            _send(_TEXT, self.id, text=self._title)

    def focus(self):
        if self.is_open:
            _send(_FOCUS, self.id)

    # Size and place. width and height follow what the desktop really gave
    # (the screen may be smaller); on_resize(win, w, h) hears every change,
    # also the user's.
    def resize(self, w, h):
        if self.is_open:
            _send(_RESIZE, self.id, 0, 0, w, h)

    def move(self, x, y):
        if self.is_open:
            _send(_MOVE, self.id, 0, 0, x, y)

    def restore(self):
        if self.is_open:
            _send(_SHOW, self.id, 0, 0, 0)

    def maximize(self):
        if self.is_open:
            _send(_SHOW, self.id, 0, 0, 1)

    def minimize(self):
        if self.is_open:
            _send(_SHOW, self.id, 0, 0, 2)

    def fullscreen(self):
        if self.is_open:
            _send(_SHOW, self.id, 0, 0, 3)

    def close(self):
        if self.is_open:
            _send(_CLOSE, self.id)
            self._gone()

    def _gone(self):
        global _windows, _widgets, _cells
        self.is_open = False
        _windows -= 1
        _objs.pop(self.id, None)
        for k in self._kids:
            _objs.pop(k.id, None)
            _widgets -= k._n
            _cells -= k._cells
        self._kids = []
        for t in _timers[:]:
            if t.win is self:
                _timers.remove(t)

    def _event(self, op, a, b, c, d, t):
        if op == _EV_CLOSE:
            self._gone()
            if self.on_close:
                self.on_close(self)
        elif op == _EV_KEY and self.on_key:
            self.on_key(self, _key_name((a & 0xFFFF) | (b << 16), c))
        elif op == _EV_RESIZE:
            self.width = a
            self.height = b
            if self.on_resize:
                self.on_resize(self, a, b)

    def every(self, ms, fn):
        return Timer(ms, fn, True, self)

    def after(self, ms, fn):
        return Timer(ms, fn, False, self)

    def label(self, x, y, text="", width=0):
        return Label(self, x, y, text, width)

    def button(self, x, y, text, on_click=None):
        return Button(self, x, y, text, on_click)

    def checkbox(self, x, y, text, checked=False, on_change=None):
        return Checkbox(self, x, y, text, checked, on_change)

    def textbox(self, x, y, width, text="", secret=False, on_enter=None, on_change=None):
        return Textbox(self, x, y, width, text, secret, on_enter, on_change)

    def progress(self, x, y, width, value=0):
        return Progress(self, x, y, width, value)

    def list(self, x, y, w, h, items=(), on_select=None, on_activate=None):
        return List(self, x, y, w, h, items, on_select, on_activate)

    def canvas(self, x, y, w, h, on_mouse=None, pages=1):
        return Canvas(self, x, y, w, h, on_mouse, pages)


class _Widget:
    _n = 1        # desktop widgets it takes
    _cells = 0

    def __init__(self, win, kind, x, y, w, h, e, text):
        global _widgets
        if not win.is_open:
            raise OSError("tdui: the window is closed")
        if _widgets + self._n > MAX_WIDGETS:
            raise OSError("tdui: at most %d widgets" % MAX_WIDGETS)
        _widgets += self._n
        self.win = win
        self.id = _new_id()
        self._text = text
        self._visible = True
        _objs[self.id] = self
        win._kids.append(self)
        _send(_ADD, self.id, win.id, kind, x, y, w, h, e, 0, text)

    def _req(self, op, a=0, b=0, text=None):
        if self.win.is_open:
            _send(op, self.id, 0, 0, a, b, text=text)

    @property
    def text(self):
        return self._text

    @text.setter
    def text(self, t):
        self._text = str(t)
        self._req(_TEXT, text=self._text)

    @property
    def visible(self):
        return self._visible

    @visible.setter
    def visible(self, v):
        self._visible = bool(v)
        self._req(_VISIBLE, 1 if v else 0)

    def color(self, fg=-1, bg=-1):
        self._req(_COLOR, fg, bg)

    def focus(self):
        self._req(_FOCUS)

    def _event(self, op, a, b, c, d, t):
        pass


class Label(_Widget):
    def __init__(self, win, x, y, text, width):
        super().__init__(win, _W_LABEL, x, y, width, 0, 0, str(text))


class Button(_Widget):
    def __init__(self, win, x, y, text, on_click):
        self.on_click = on_click
        super().__init__(win, _W_BUTTON, x, y, 0, 0, 0, str(text))

    def _event(self, op, a, b, c, d, t):
        if op == _EV_CLICK and self.on_click:
            self.on_click(self)


class Checkbox(_Widget):
    def __init__(self, win, x, y, text, checked, on_change):
        self._checked = bool(checked)
        self.on_change = on_change
        super().__init__(win, _W_CHECK, x, y, 0, 0, 1 if checked else 0, str(text))

    @property
    def checked(self):
        return self._checked

    @checked.setter
    def checked(self, v):
        self._checked = bool(v)
        self._req(_VALUE, 1 if v else 0)

    def _event(self, op, a, b, c, d, t):
        if op == _EV_CHANGE:
            self._checked = bool(a)
            if self.on_change:
                self.on_change(self)


class Textbox(_Widget):
    def __init__(self, win, x, y, width, text, secret, on_enter, on_change):
        self.on_enter = on_enter
        self.on_change = on_change
        super().__init__(win, _W_TEXT, x, y, width, 0, 1 if secret else 0, str(text))

    def _event(self, op, a, b, c, d, t):
        if op == _EV_CHANGE:
            self._text = t or ""
            if self.on_change:
                self.on_change(self)
        elif op == _EV_CLICK and self.on_enter:
            self.on_enter(self)


class Progress(_Widget):
    def __init__(self, win, x, y, width, value):
        self._value = value
        super().__init__(win, _W_PROGRESS, x, y, width, 0, value, "")

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, v):
        self._value = max(0, min(100, int(v)))
        self._req(_VALUE, self._value)


class List(_Widget):
    _n = 2        # the list and its scrollbar

    def __init__(self, win, x, y, w, h, items, on_select, on_activate):
        if w == 1:                     # <= 0: to the edge, following resizes
            raise ValueError("list size")
        self.on_select = on_select
        self.on_activate = on_activate
        self._selected = 0
        super().__init__(win, _W_LIST, x, y, w, h, 0, "")
        self._items = []
        self.items = items

    @property
    def items(self):
        return self._items

    @items.setter
    def items(self, items):
        self._items = [str(i) for i in items]
        self._req(_LCLEAR)
        self._add(self._items)

    def append(self, item):
        item = str(item)
        self._items.append(item)
        self._add([item])

    def _add(self, items):
        chunk = []
        size = 0
        for i in items:
            i = i.replace("\n", " ")
            if chunk and size + len(i) > 900:
                self._req(_LADD, text="\n".join(chunk))
                chunk = []
                size = 0
            chunk.append(i)
            size += len(i) + 1
        if chunk:
            self._req(_LADD, text="\n".join(chunk))

    @property
    def selected(self):
        return self._selected

    @selected.setter
    def selected(self, i):
        self._selected = int(i)
        self._req(_VALUE, self._selected)

    def _event(self, op, a, b, c, d, t):
        if op == _EV_CHANGE:
            self._selected = a
            if self.on_select:
                self.on_select(self)
        elif op == _EV_CLICK:
            self._selected = a
            if self.on_activate:
                self.on_activate(self)


def rgb(r, g, b):
    # The palette colour nearest to an RGB colour (0..255 each).
    return _tdui.rgb(r, g, b)


def _part(w, h, border):
    return bytes((w & 255, (w >> 8) & 255, h & 255, (h >> 8) & 255, border & 255, (border >> 8) & 255))


class _Batch:
    # One reusable context, no list of requests or per-update context object.
    # Nested uses share the session's outer boundary on the desktop.
    def __enter__(self):
        _tdui.send(_BATCH_BEGIN, 0)
        return self

    def __exit__(self, kind, value, traceback):
        _tdui.send(_BATCH_END, 0)
        return False


_batch = _Batch()


def batch():
    # All tdui drawing in this session is held until the outer end, at most
    # 1000 ms (5000 ms for first-window construction). Keep its work bounded.
    # Exceptions release the drawing; this is not a rollback transaction.
    return _batch


class _Pixels:
    def _gradient_data(self, gradient):
        if gradient is None:
            return b''
        values=tuple(gradient['rgb1'])+tuple(gradient['rgb2'])
        if len(values)!=6 or any(not 0<=v<=255 for v in values):
            raise ValueError('gradient RGB range')
        return bytes(values+(int(bool(gradient.get('across',False))),))
    # Drawing in pixels, done on the desktop: one call draws a whole shape,
    # picture or panel, however large. A canvas has 2 x 2 pixels to a cell
    # (pwidth x pheight in all); a cell drawn in shows its pixels until text
    # is put there. Colours are palette numbers 0..255 (rgb() finds one);
    # CLEAR in an image is transparent.

    def batch(self):
        return _batch

    def bar(self, x, y, w, h, value, maximum=100, color=40, track=237, edge=-1, radius=0, gradient=None):
        # Pixel coordinates; value is clamped to 0..maximum. One request
        # repaints the track and fill, without clearing its background.
        # Even x and width give symmetric whole columns in Normal pictures.
        if maximum <= 0 or radius < 0 or not 0 <= color <= 255 or not 0 <= track <= 255 or not -1 <= edge <= 255:
            raise ValueError("bar range or colour")
        if self._alive():
            amount = min(1000, max(0, int(value * 1000 // maximum)))
            colors = self._bar_colors
            if not colors or colors[0] != color or colors[1] != track or colors[2] != max(0, edge):
                colors = self._bar_colors = bytes((color, track, max(0, edge)))
            _tdui.send(_BAR, self.id, 0, 1 if edge >= 0 else 0, x, y, w, h, amount, radius,
                       colors+self._gradient_data(gradient))

    def rect(self, x, y, w, h, color, radius=0, outline=False, gradient=None):
        if self._alive():
            _tdui.send(_RECT, self.id, 0, (1 if outline else 0)|(2 if gradient else 0), x, y, w, h, color, radius,self._gradient_data(gradient))

    def line(self, x0, y0, x1, y1, color):
        if self._alive():
            _tdui.send(_LINE, self.id, 0, 0, x0, y0, x1, y1, color, 0)

    def circle(self, x, y, r, color, outline=False, gradient=None):
        if self._alive():
            _tdui.send(_CIRCLE, self.id, 0, (1 if outline else 0)|(2 if gradient else 0), x, y, r, 0, color, 0,self._gradient_data(gradient))

    def gradient(self, x, y, w, h, rgb1, rgb2, across=False):
        # From one (r, g, b) colour to another, top to bottom (across: left
        # to right), dithered to the palette.
        if self._alive():
            _tdui.send(_GRADIENT, self.id, 0, 1 if across else 0, x, y, w, h, 0, 0, bytes(tuple(rgb1) + tuple(rgb2)))

    def image(self, img, x, y, w=0, h=0, src=None):
        # img (or its part src = (sx, sy, sw, sh), say one picture of a
        # sprite sheet) at x, y; w, h stretch or shrink it.
        if self._alive():
            sx, sy, sw, sh = src or (0, 0, 0, 0)
            _tdui.send(_DRAW_IMAGE, self.id, img.id, 0, x, y, w, h, sx, sy, _part(sw, sh, 0))

    def nine(self, img, x, y, w, h, border, src=None):
        # img (or its part src) as a w x h panel: its corners of border
        # pixels as they are, the edges and the middle stretched. One small
        # picture makes buttons and frames of any size.
        if self._alive():
            sx, sy, sw, sh = src or (0, 0, 0, 0)
            _tdui.send(_DRAW_IMAGE, self.id, img.id, 1, x, y, w, h, sx, sy, _part(sw, sh, border))

    def draw_text(self, x, y, text, color, bg=CLEAR, scale=1, small=False):
        # Text in pixels at x, y: an 8 x 16 font (4 x 8 cells) times scale
        # (1-8), or small=True the 3 x 5 one of big() (digits, A-Z, - + %
        # / . : ?). bg CLEAR: only the letters are drawn, over whatever is
        # there. text_size() gives the size. (Not text(): a canvas's text is
        # its widget text.)
        if self._alive():
            _tdui.send(_PTEXT, self.id, 0, 1 if small else 0, x, y, color, bg, scale, 0, str(text))

    def pixels(self, x, y, w, h, data, rgb=False, dither=True):
        # w x h pixels from bytes: palette numbers, or (rgb) red, green, blue
        # for each, dithered or the nearest colour. One call for all of them.
        if self._alive():
            _tdui.pixels(self.id, x, y, w, h, data, (1 if dither else 2) if rgb else 0)


def text_size(text, scale=1, small=False):
    # Pixels wide and high that draw_text() draws text in ("\n" starts a line).
    lines = str(text).split("\n")
    if small:
        w = max(sum(1 if ch in ".:" else 3 for ch in l) + len(l) - 1 if l else 0 for l in lines)
        return w * scale, (6 * len(lines) - 1) * scale
    return 8 * scale * max(len(l) for l in lines), 16 * scale * len(lines)


class CanvasButton:
    # A button drawn in pixels on a canvas (canvas.button()): rounded, in
    # colours, or a piece of a sprite sheet stretched to its size
    # (nine-slice), with its text on it. It looks pressed while the mouse
    # button is held on it and calls on_click(button) when let go on it.
    # Cells: x, y, w, h. The look is a few desktop calls; nothing per pixel.

    def __init__(self, cv, x, y, w, h, text, on_click, color, down, rim, fg, radius, scale, sheet, src, src_down, border,
                 on_press=None, on_release=None, on_cancel=None, font=None, gradient=None):
        self.cv = cv
        self.page = cv.drawing
        self.x, self.y, self.w, self.h = x, y, w, h
        self._text = str(text)
        self.on_click = on_click
        self.on_press, self.on_release, self.on_cancel = on_press, on_release, on_cancel
        self.color, self.down, self.rim, self.fg = color, down, rim, fg
        self.gradient=gradient
        self.radius, self.scale = radius, scale
        self.font = font
        self.sheet, self.src, self.src_down, self.border = sheet, src, src_down, border
        self.enabled = True
        cv._buttons.append(self)
        self.draw()

    @property
    def text(self):
        return self._text

    @text.setter
    def text(self, t):
        self._text = str(t)
        self.draw()

    def draw(self, pressed=None):
        if pressed is None:
            pressed = self.cv._held is self
        with self.cv.batch():
            previous = self.cv.drawing
            if previous != self.page:
                self.cv.draw_to(self.page)
            try:
                self._draw(pressed)
            finally:
                if previous != self.page:
                    self.cv.draw_to(previous)

    def _draw(self, pressed=False):
        # Drawn again: after a background was drawn over it, say.
        cv = self.cv
        px, py, pw, ph = 2 * self.x, 2 * self.y, 2 * self.w, 2 * self.h
        if self.sheet:
            # A picture: at the terminal's own resolution where it can.
            src = self.src_down if pressed and self.src_down else self.src
            cv.picture(self.sheet, self.x, self.y, self.w, self.h, src=src, border=self.border, text=self._text,
                       fg=self.fg, scale=self.scale, font=self.font)
            return
        gradient=self.gradient
        if pressed and gradient:
            gradient={'rgb1':gradient['rgb2'],'rgb2':gradient['rgb1'],'across':gradient.get('across',False)}
        cv.rect(px, py, pw, ph, self.down if pressed else self.color, self.radius,gradient=gradient)
        if self.rim is not None:
            cv.rect(px, py, pw, ph, self.rim, self.radius, outline=True)
        if self.font:
            cv.font_text(self.font,self.x,self.y,self.w,self.h,self._text,fg=self.fg,align="center",valign="middle")
        elif self.scale:
            tw, th = text_size(self._text, self.scale)
            cv.draw_text(px + (pw - tw) // 2, py + (ph - th) // 2 + (1 if pressed else 0), self._text, self.fg,
                    scale=self.scale)
        else:
            cv.put(self.x + (self.w - len(self._text)) // 2, self.y + self.h // 2, self._text, self.fg, CLEAR)

    def remove(self):
        # It no longer takes clicks (what is drawn stays).
        if self in self.cv._buttons:
            self.cv._buttons.remove(self)
        if self.cv._held is self:
            self.cv._held = None
            if self.on_cancel:
                self.on_cancel(self)

    def _hit(self, x, y):
        return (self.enabled and self.page == self.cv.showing and
                self.x <= x < self.x + self.w and self.y <= y < self.y + self.h)


class Image(_Pixels):
    # A picture the desktop keeps for the script, drawn into canvases with
    # their image() and nine(). Image(w, h, color) is blank (color CLEAR:
    # transparent); Image.load() reads a PPM, PGM or BMP file.

    def __init__(self, w, h, color=0, _file=None):
        _tdui.open()
        self._bar_colors = None
        self.id = _new_id()
        _objs[self.id] = self
        try:
            if _file:
                self.width, self.height = _tdui.image_load(self.id, _file[0], _file[1], _file[2], w, h)
            else:
                _tdui.image(self.id, w, h, color)
                self.width, self.height = w, h
        except Exception:
            del _objs[self.id]
            raise

    @staticmethod
    def load(path, w=0, h=0, key=None, dither=True):
        # The picture in path, scaled to w x h if given (only one: the other
        # keeps its shape). key (r, g, b): that colour is transparent.
        # Decoded in C on the script's task; dither spreads colours the
        # palette lacks over neighbouring pixels.
        if not path.startswith("/"):
            path = os.getcwd().rstrip("/") + "/" + path
        k = -1 if key is None else key[0] << 16 | key[1] << 8 | key[2]
        return Image(w, h, 0, (path, k, dither))

    def free(self):
        # The desktop forgets it (it is also gone when the script ends).
        if self._alive():
            _send(_CLOSE, self.id)
            del _objs[self.id]

    def _alive(self):
        return self.id in _objs

    def _event(self, op, a, b, c, d, t):
        pass


class Font:
    # Generated bitmap resource, loaded once into native graphics memory.
    # Glyphs are rendered in C; no per-character Python drawing messages.
    def __init__(self, path):
        _tdui.open()
        self.id = _new_id()
        if not path.startswith("/"):
            path = os.getcwd().rstrip("/") + "/" + path
        self.height,self.ascent,self.glyphs,self.size,self.bpp = _tdui.font_load(self.id,path)
        _objs[self.id] = self

    @staticmethod
    def load(path):
        return Font(path)

    def free(self):
        if self.id in _objs:
            _send(_CLOSE,self.id)
            del _objs[self.id]

    def _event(self, op, a, b, c, d, t):
        pass


class Canvas(_Widget, _Pixels):
    # A grid of character cells. Colours are palette numbers 0..255, -1 for
    # the window's own. With pages > 1 it keeps that many pictures on the
    # desktop: draw into a hidden one (draw_to), then show it at once.
    # Pixels (_Pixels): 2 x 2 to a cell, in a layer under the cells; text
    # with the background CLEAR stands on them.
    _n = 0

    def __init__(self, win, x, y, w, h, on_mouse, pages=1):
        global _cells
        if not win.is_open:
            raise OSError("tdui: the window is closed")
        if x < 0 or y < 0 or w < 1 or h < 1:
            raise ValueError("canvas size")
        if not 1 <= pages <= MAX_PAGES:
            raise ValueError("canvas pages: 1 to %d" % MAX_PAGES)
        if _cells + w * h * pages > MAX_CELLS:
            raise OSError("tdui: canvases are limited to %d cells in all" % MAX_CELLS)
        _cells += w * h * pages
        self._cells = w * h * pages
        self.width = w
        self.height = h
        self.pages = pages
        self.drawing = 0      # the page put/fill/big and the pixels draw into
        self.showing = 0      # the page the window shows
        self.on_mouse = on_mouse
        self._buttons = []    # CanvasButton
        self._bar_colors = None
        self._held = None     # the one pressed
        super().__init__(win, _W_CANVAS, x, y, w, h, pages, "")

    # Drawing goes straight to _tdui.send: these are called most.
    def put(self, x, y, text, fg=-1, bg=-1):
        if self.win.is_open:
            _tdui.send(_PUT, self.id, 0, 0, x, y, fg, bg, 0, 0, str(text))

    def fill(self, x, y, w, h, ch=" ", fg=-1, bg=-1):
        if self.win.is_open:
            _tdui.send(_FILL, self.id, 0, 0, x, y, w, h, fg, bg, ch)

    def clear(self, fg=-1, bg=-1):
        self.fill(0, 0, self.width, self.height, " ", fg, bg)

    def big(self, x, y, text, fg=-1, bg=-1, scale=1, width=0):
        # Text in big characters (digits, A-Z, - + % / . : and space), drawn
        # on the desktop: 3 cells wide and 3 rows high a character at scale
        # 1, 6 x 5 at scale 2. width > 0: right-aligned in that many
        # characters, blanking what is left. big_size() gives the size.
        if self.win.is_open:
            _tdui.send(_BIG, self.id, 0, 0, x, y, fg, bg, scale, width, str(text))

    def _page(self, n):
        if not 0 <= n < self.pages:
            raise ValueError("canvas page: 0 to %d" % (self.pages - 1))
        return n

    def draw_to(self, page):
        # put/fill/big draw into this page from now on; the window keeps
        # showing its page.
        self.drawing = self._page(page)
        self._req2(_PAGE, page, -1, 0, 0, 0, 0, None)

    def show(self, page):
        # The window shows this page at once (all of it, not piece by piece).
        self.showing = self._page(page)
        self._req2(_PAGE, -1, page, 0, 0, 0, 0, None)

    def copy_page(self, src, dst):
        # Page src's picture onto page dst, for example a background drawn
        # once.
        self._req2(_PAGE_COPY, self._page(src), self._page(dst), 0, 0, 0, 0, None)

    def resize(self, w, h):
        # What fits of the picture stays; new cells are blank.
        global _cells
        if w < 1 or h < 1:
            raise ValueError("canvas size")
        if _cells - self._cells + w * h * self.pages > MAX_CELLS:
            raise OSError("tdui: canvases are limited to %d cells in all" % MAX_CELLS)
        _cells += w * h * self.pages - self._cells
        self._cells = w * h * self.pages
        self.width = w
        self.height = h
        self._req2(_RESIZE, w, h, 0, 0, 0, 0, None)

    def move(self, x, y):
        self._req2(_MOVE, x, y, 0, 0, 0, 0, None)

    def _req2(self, op, a, b, c, d, e, f, text):
        if self.win.is_open:
            _send(op, self.id, 0, 0, a, b, c, d, e, f, text)

    def _alive(self):
        return self.win.is_open

    def picture(self, img, x, y, w, h, src=None, border=0, text="", fg=231, scale=0, font=None):
        # img (or its part src = (sx, sy, sw, sh)) over cells x, y, w x h, at
        # the terminal's own resolution where it can show pictures (the
        # CrowPanel's LCD, the web terminal, Windows Terminal): smooth
        # corners and gradients, like an LCD panel's UI. border > 0:
        # nine-slice, keeping border pixels of the image at each edge.
        # text: a label drawn into the picture in fg (scale: its size, 0 as
        # large as fits). Other terminals (PuTTY) show the same in the
        # canvas's pixels. Drawing over its cells removes it.
        if self.win.is_open:
            sx, sy, sw, sh = src or (0, 0, 0, 0)
            head = bytes(v & 255 if i % 2 == 0 else (v >> 8) & 255
                         for v in (sw, sh, border, fg, scale) for i in (0, 1))
            if font is not None:
                head += bytes((font.id & 255,font.id >> 8))
            _tdui.send(_PICTURE, self.id, img.id, (1 if border else 0) | (2 if font else 0), x, y, w, h, sx, sy, head + str(text).encode())

    def font_text(self, font, x, y, w, h, text, fg=231, bg=CLEAR, align="left", valign="top", wrap=False):
        # Cell-bounded native-resolution text using a generated font's pixels.
        if not isinstance(font, Font) or font.id not in _objs:
            raise ValueError("font_text needs a loaded Font")
        horizontal={"left":0,"center":1,"right":2}
        vertical={"top":0,"middle":1,"bottom":2}
        if align not in horizontal or valign not in vertical or w<1 or h<1:
            raise ValueError("font_text bounds or alignment")
        if self.win.is_open:
            _tdui.send(_FONT_TEXT,self.id,font.id,horizontal[align] | vertical[valign]<<2 | (16 if wrap else 0),x,y,w,h,fg,bg,str(text))

    @property
    def pwidth(self):
        return 2 * self.width

    @property
    def pheight(self):
        return 2 * self.height

    def _event(self, op, a, b, c, d, t):
        if op != _EV_MOUSE:
            return
        # Buttons drawn in pixels first: a press on one is theirs, and so is
        # the release that follows (wherever it is).
        held = self._held
        if c == 0 and d == 0 and not held:
            for btn in self._buttons:
                if btn._hit(a, b):
                    self._held = btn
                    btn.draw(True)
                    if btn.on_press:
                        btn.on_press(btn)
                    return
        elif c == 0 and d == 1 and held:
            self._held = None
            held.draw()
            if held.on_release:
                held.on_release(held)
            if held._hit(a, b) and held.on_click:
                held.on_click(held)
            elif not held._hit(a, b) and held.on_cancel:
                held.on_cancel(held)
            return
        elif held:
            return
        if self.on_mouse:
            self.on_mouse(self, a, b, c, d)

    def button(self, x, y, w, h, text, on_click=None, color=None, down=None, rim=None, fg=231, radius=6, scale=0,
               sheet=None, src=None, src_down=None, border=4, on_press=None, on_release=None, on_cancel=None, font=None, gradient=None):
        # A CanvasButton at cell x, y, w x h cells. In colours: color (a
        # blue by default), down when pressed (darker), rim an outline;
        # radius rounds the corners (pixels). From a sprite sheet: sheet and
        # its pieces src and src_down, border pixels kept at the edges.
        # scale > 0: the text in pixels (draw_text()), that big.
        # All callbacks receive the button. Release fires even outside;
        # click follows release inside, cancel follows release outside.
        if color is None:
            color = rgb(40, 110, 210)
        if down is None:
            down = rgb(20, 60, 140)
        return CanvasButton(self, x, y, w, h, text, on_click, color, down, rim, fg, radius, scale, sheet, src,
                            src_down, border, on_press, on_release, on_cancel, font, gradient)


def big_size(text, scale=1):
    # Columns and rows Canvas.big() takes for text ('.' and ':' are narrow).
    n = len(text)
    if not n:
        return 0, 0
    cols = sum(1 if c in ".:" else 3 for c in text) * scale + (n - 1) * scale
    return cols, (5 * scale + 1) // 2


# ------------------------------------------------------------ the loop

def _run_timers(left):
    # Run the timers that are due; return how long poll() may wait.
    now = time.ticks_ms()
    for t in _timers[:]:
        if time.ticks_diff(t.due, now) <= 0:
            if t.repeat:
                t.due = time.ticks_add(now, t.ms)
            else:
                _timers.remove(t)
            t.fn()
    now = time.ticks_ms()
    for t in _timers:
        left = min(left, time.ticks_diff(t.due, now))
    return max(left, 0)


def _dispatch(ev):
    op, id, a, b, c, d, t = ev
    if op == _EV_RESULT:
        if id in _results:
            _results[id] = (a, t)
    elif op == _EV_ERROR:
        raise OSError("tdui: " + (t or "error"))
    else:
        o = _objs.get(id)
        if o is not None:
            o._event(op, a, b, c, d, t)


def poll(ms=0):
    # Handle what the user did and run due timers, for up to ms milliseconds.
    end = time.ticks_add(time.ticks_ms(), ms)
    while True:
        ev = _tdui.recv(_run_timers(time.ticks_diff(end, time.ticks_ms())))
        while ev:
            _dispatch(ev)
            ev = _tdui.recv(0)
        if time.ticks_diff(end, time.ticks_ms()) <= 0:
            return


def run():
    # Until every window is closed.
    while _windows > 0:
        poll(100)


async def run_async():
    # run() for asyncio: other tasks keep going between events.
    import asyncio
    while _windows > 0:
        poll(0)
        await asyncio.sleep_ms(20)


def quit():
    for o in list(_objs.values()):
        if isinstance(o, Window):
            o.close()


def _request(op, text, flags=0):
    _tdui.open()
    rid = _new_id()
    _results[rid] = None
    try:
        _send(op, rid, 0, flags, text=text)
        while _results[rid] is None:
            poll(20)
        return _results[rid]
    finally:
        del _results[rid]


# ------------------------------------------------------------ dialogs

def msgbox(title, text, buttons="OK"):
    # The number of the button chosen (from 0), -1 if closed with Esc.
    return _request(_MSGBOX, "%s\0%s\0%s" % (title, text, buttons))[0]


def confirm(title, text):
    return msgbox(title, text, "Yes|No") == 0


def inputbox(title, prompt, text="", secret=False):
    # The text entered, or None for Cancel.
    n, t = _request(_INPUT, "%s\0%s\0%s" % (title, prompt, text), 1 if secret else 0)
    return None if n < 0 else (t or "")


def menu(items):
    # A popup menu at the mouse; the number of the item chosen, -1 if none.
    # An item "-" is a separator.
    return _request(_MENU, "\0".join(str(i) for i in items[:20]))[0]
