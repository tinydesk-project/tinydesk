"""tdapp - the TinyDesk app library: what most tdui apps have in common.

An app imports it and keeps only its own logic: its windows and widgets,
its artwork and what happens when something is clicked. tdapp brings the
rest, the same code App Builder's apps use:

  App            a window with pages, timers, a serial link and bindings
  require        stop with a clear message when the firmware is too old
  batch          draw several things as one frame
  panel, gauge, slider, Field
                 canvas elements that redraw themselves when a value changes
  make_canvas, register_canvas, show_canvases, canvas_page, ...
                 canvases that show a different page per window page
  Link, crc16    Modbus RTU master / slave and plain UART protocols, with
                 widgets bound to registers

Frozen into the firmware next to tdui (import tdapp); its version is
tdapp.VERSION. Public names only grow within a major version
(tools/check_tdui_contract.py checks them like tdui's). The documentation
is the TinyDesk site's "Python apps library (tdapp)" page.
"""
import tdui
import time as _time
import struct as _struct

VERSION = (1, 0)

# Canvases with pages (draw_to / show) on this firmware; without them each
# page's canvas is a canvas of its own, moved off the window while hidden.
PAGED = hasattr(tdui.Canvas, "draw_to")
PARK = 1000


def require(cells=0, pixels=0, rgb=False, cache=None, pictures=0, visible_pictures=0,
            fonts=False, gradients=False, pixel_widgets=False):
    """Raise OSError naming what to update when the firmware lacks a
    feature the app needs; turn on RGB pictures and the render cache when
    asked (cache: KiB, 0 off, None leave as is)."""
    if cells and getattr(tdui, "MAX_CELLS", 2000) < cells:
        raise OSError("This app needs %d canvas cells; update TinyDesk firmware." % cells)
    if rgb:
        if not getattr(tdui, "SUPPORTS_RGB_PICTURES", False):
            raise OSError("Update TinyDesk firmware for RGB565 pictures.")
        tdui.enable_rgb_pictures()
        if cache is not None and hasattr(tdui, "render_cache"):
            tdui.render_cache(cache, bool(cache))
    if getattr(tdui, "MAX_PICTURES", 192) < pictures or getattr(tdui, "MAX_VISIBLE_PICTURES", 64) < visible_pictures:
        raise OSError("Update TinyDesk firmware for this many smooth surfaces.")
    if fonts and not hasattr(tdui, "Font"):
        raise OSError("Update TinyDesk firmware for generated bitmap fonts.")
    if gradients and not getattr(tdui, "SUPPORTS_GRADIENT_FILLS", False):
        raise OSError("Update TinyDesk firmware for gradient fills.")
    if pixel_widgets and not hasattr(tdui.Canvas, "bar"):
        raise OSError("This app needs TinyDesk with batch and pixel widgets.")
    if pixels and getattr(tdui, "MAX_PIXELS", 0) < pixels:
        raise OSError("This app needs %d pixel bytes." % pixels)


def preparing(on):
    """While on, batch() draws at once instead of in one frame: for pages
    that are prepared hidden at start-up."""
    global _preparing
    _preparing = bool(on)

def panel(cv, img, x, y, w, h, src=None, border=16, text="", fg=231, scale=0, font=None):
    if w < 5 or h < 3 or border < 1 or (src and (src[2] <= border*2 or src[3] <= border*2)):
        cv.picture(img, x, y, w, h, src=src, border=border, text=text, fg=fg, scale=scale, font=font)
        return
    sx, sy, sw, sh = src or (0, 0, img.width, img.height)
    b = min(border, sw // 2, sh // 2)
    cols, rows = (2, w-4, 2), (1, h-2, 1)
    source_x, source_y = (sx, sx+b, sx+sw-b), (sy, sy+b, sy+sh-b)
    source_w, source_h = (b, sw-2*b, b), (b, sh-2*b, b)
    yy = y
    with batch():
        for row in range(3):
            xx = x
            for col in range(3):
                label = text if row == 1 and col == 1 else ""
                cv.picture(img, xx, yy, cols[col], rows[row], src=(source_x[col], source_y[row], source_w[col], source_h[row]), text=label, fg=fg, scale=scale, font=font)
                xx += cols[col]
            yy += rows[row]

_controls = []
_held = None

def gauge(cv, x, y, w, h, value, maximum=100, color=220, track=244, bg=17, gradient=None, radius=0):
    points = ((-9,5),(-10,2),(-10,-2),(-8,-6),(-5,-9),(0,-10),(5,-9),(8,-6),(10,-2),(10,2),(9,5))
    rx = max(1, min(w // 2 - 1, h - 7))
    ry = max(1, rx // 2)  # Terminal pixels are twice as tall as wide.
    cx, cy = x + w // 2, y + ry
    with batch():
        cv.rect(x, y, w, h, bg, radius=radius, gradient=gradient)
        for i in range(10):
            a, b = points[i], points[i+1]
            cv.line(cx+a[0]*rx//10, cy+a[1]*ry//10, cx+b[0]*rx//10, cy+b[1]*ry//10, track)
        position = max(0,min(100,int(value)*100//maximum))
        index = min(9,position//10)
        fraction = position-index*10
        a, b = points[index], points[index+1]
        px, py = a[0]*10+(b[0]-a[0])*fraction, a[1]*10+(b[1]-a[1])*fraction
        cv.line(cx, cy, cx+px*rx//100, cy+py*ry//100, color)
        cv.draw_text(x+1, y+h-5, str(value), color, bg=tdui.CLEAR, small=True)

def slider(cv, x, y, w, h, value, maximum=100, color=220, track=17, knob=231, gradient=None, radius=0):
    with batch():
        cv.rect(x*2, y*2, w*2, h*2, track, radius=radius, gradient=gradient)
        cv.line(x*2, y*2+h, (x+w)*2-1, y*2+h, color)
        at = max(0,min(w-1,int(value)*(w-1)//maximum))
        cv.rect((x+at)*2, y*2, 2, h*2, knob, radius=1)

def control_mouse(cv, x, y, button, action):
    global _held
    held = _held
    if action == 0 and button == 0:
        for field in reversed(_controls):
            p = field.values
            if field.cv is cv and field.page == cv.showing and field.enabled and p['x']<=x<p['x']+p['w'] and p['y']<=y<p['y']+p['h']:
                held = field
                _held = held
                break
    if held is None or held.cv is not cv:
        return False
    if held.page != cv.showing or not held.enabled:
        _held = None
        return True
    p = held.values
    value = max(0,min(p['maximum'],(x-p['x'])*p['maximum']//max(1,p['w']-1)))
    if value != p['value']:
        held.set(value=value)
        if held.on_change:
            held.on_change(held)
    if action == 1:
        _held = None
    return True

class Field:
    # Geometry and appearance come from the editable drawing operation.
    def __init__(self, cv, page, method, values, image=None, on_change=None, font=None):
        self.cv, self.page, self.method = cv, page, method
        self.values = values
        if image is not None:
            self.values['img'] = image
        if font is not None:
            self.values['font'] = font
        if method == 'slider':
            self.on_change, self.enabled = on_change, True
            _controls.append(self)

    def set(self, **changes):
        values = self.values
        for k in changes:
            if values.get(k) != changes[k]:
                values.update(changes)
                self.draw()
                return

    def draw(self):
        # One batch: no frame shows the field half drawn (a fill, then the
        # track, then the value would otherwise flicker when values move fast).
        with batch():
            other = PAGED and self.cv.drawing != self.page
            if other:
                previous = self.cv.drawing
                self.cv.draw_to(self.page)
            try:
                if self.method == 'gauge':
                    gauge(self.cv, **self.values)
                elif self.method == 'slider':
                    slider(self.cv, **self.values)
                elif self.method == 'panel':
                    panel(self.cv, **self.values)
                else:
                    getattr(self.cv, self.method)(**self.values)
            finally:
                if other:
                    self.cv.draw_to(previous)

class _NoBatch:
    def __enter__(self):
        return self

    def __exit__(self, kind, value, traceback):
        return False

_no_batch = _NoBatch()
_preparing = False


def batch():
    return tdui.batch() if not _preparing and hasattr(tdui, "batch") else _no_batch


_groups = {}
_window_groups = {}

def make_canvas(win, x, y, w, h, pages):
    if PAGED:
        cv = win.canvas(x, y, w, h, pages=pages)
    else:
        cv = win.canvas(x, y, w, h)
    cv._tdapp_parked = False
    return cv

def alias_canvas(base, win, x, y, w, h):
    if PAGED:
        return base
    return make_canvas(win, x, y, w, h, 1)

def hide_canvas(cv, clear=False):
    if clear:
        cv.fill(0, 0, cv.width, cv.height, " ", -1, -1)
    if not cv._tdapp_parked:
        cv.move(PARK, PARK)
        cv._tdapp_parked = True

def place_canvas(cv, x, y):
    if cv._tdapp_parked:
        cv.move(x, y)
        cv._tdapp_parked = False

def select_canvas(cv, page):
    # _PAGE supports draw and show together: one atomic request, not two.
    cv.drawing = page
    cv.showing = page
    cv._req2(19, page, page, 0, 0, 0, 0, None)

def register_canvas(win, group, base, cv, own, page, x, y, paint, visible, mouse):
    record = _groups.get(group)
    if record is None:
        record = [base, x, y, []]
        _groups[group] = record
        _window_groups.setdefault(win, []).append(record)
    record[3].append((cv, own, page, paint, visible, mouse))
    if PAGED:
        cv.draw_to(page)
        paint()

def show_canvases(win, name):
    for base, x, y, entries in _window_groups.get(win, ()):
        active = None
        for entry in entries:
            cv, own, page, paint, visible, mouse = entry
            if not PAGED:
                hide_canvas(cv, True)
            if visible and (own is None or own == name):
                active = entry
        if active is None:
            if PAGED:
                hide_canvas(base)
            continue
        cv, own, page, paint, visible, mouse = active
        place_canvas(cv, x, y)
        if PAGED:
            if len(entries) > 1:
                select_canvas(cv, page)
        else:
            paint()
        cv.on_mouse = mouse

def canvas_page(cv, name):
    # Use the returned canvas: aliases share an object only on newer firmware.
    for base, x, y, entries in _groups.values():
        found = False
        for entry in entries:
            if entry[0] is cv:
                found = True
                break
        if found:
            for target, own, page, paint, visible, mouse in entries:
                if own is None or own == name:
                    if PAGED:
                        target.draw_to(page)
                    return target
    raise ValueError("unknown canvas page")

# ---------------------------------------------------- serial link

def crc16(data):
    crc = 65535
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ (40961 if crc & 1 else 0)
    return crc


def _rtu(data):
    crc = crc16(data)
    return bytes(data) + bytes((crc & 255, crc >> 8))


# What a link config or a binding leaves out (App Builder's defaults).
_LINK_DEFAULTS = {"version": 1, "mode": "none", "uart": 1, "tx": None, "rx": None, "baud": 9600, "bits": 8,
                  "parity": "none", "stop": 1, "de": -1, "deActive": 1, "unit": 1, "pollMs": 250,
                  "timeoutMs": 300, "retries": 1, "staleMs": 2000, "codec": "rtu", "codecModule": "",
                  "framing": "newline", "statusElement": "", "registerCount": 0, "inputAlias": False}
_BIND_DEFAULTS = {"direction": "read", "table": "hr", "address": 0, "dataType": "u16", "wordOrder": "high_first",
                  "gain": 1, "offset": 0, "decimals": 0, "prefix": "", "suffix": "", "event": "click",
                  "action": "value", "writeValue": 1, "releaseValue": 0, "min": 0, "max": 65535, "initial": 0}


class Link:
    """A serial link: Modbus RTU master / slave or a UART protocol, with
    widgets bound to registers (the "Serial links" part of the tdapp
    documentation). Keys config leaves out take App Builder's defaults."""

    def __init__(self, config, simulated=False):
        config = dict(_LINK_DEFAULTS, **config)
        if config["mode"].startswith("modbus"):
            config["codec"] = "rtu"
        self.config, self.simulated = config, simulated
        self.master = config['mode'].endswith('master')
        self.codec = config['codec']
        self.uart = self.de = self.custom = None
        self.bank, self.bindings, self.queue = {}, [], []
        for i in range(config.get('registerCount',0)):
            self.bank[('hr',i)] = 0
        self.on_write = self.before_read = None
        self.frames = self.errors = 0
        self.rx_bytes = self.tx_bytes = 0
        self.rendered = {}
        self.rx = bytearray()
        self.pending = None
        self.index = self.sequence = self.last_rx = self.last_sent = 0
        self.last_good = None
        self.next_poll = 0
        self.state, self.last_error = 'STARTING', ''
        self.status_target = None
        self.closed = False
        c = config
        bits = 1 + c['bits'] + c['stop'] + (c['parity'] != 'none')
        self.gap = max(2, (bits * 3500 + c['baud'] - 1) // c['baud'])

    def _status(self, state, error=''):
        changed = (state, error) != (self.state, self.last_error)
        self.state, self.last_error = state, str(error)[:120]
        if changed and self.status_target is not None:
            if hasattr(self.status_target, 'set'):
                self.status_target.set(text=state)
            else:
                self.status_target.text = state

    def start(self):
        now = _time.ticks_ms()
        self.last_rx = self.last_sent = self.next_poll = now
        if self.simulated:
            self._status('SIMULATED')
        else:
            try:
                from machine import UART, Pin
                c = self.config
                if c['de'] >= 0:
                    self.de = Pin(c['de'], Pin.OUT, value=1-c['deActive'])
                self.uart = UART(c['uart'], baudrate=c['baud'], bits=c['bits'],
                    parity={'none': None, 'even': 0, 'odd': 1}[c['parity']],
                    stop=c['stop'], tx=c['tx'], rx=c['rx'], timeout=0,
                    timeout_char=0, rxbuf=512)
                if self.codec in ('json', 'custom'):
                    import json
                    self.json = json
                if self.codec == 'custom':
                    self.custom = __import__(c['codecModule'])
                    if not callable(getattr(self.custom, 'encode', None)) or not callable(getattr(self.custom, 'decode', None)):
                        raise ValueError('UART codec needs encode(message) and decode(frame)')
                self._status('WAIT' if self.master else 'LISTENING')
            except Exception as error:
                if self.uart:
                    self.uart.deinit()
                self.uart = None
                if self.de:
                    self.de.value(1-self.config['deActive'])
                self._status('ERROR', error)
        for element, spec, kind in self.bindings:
            if spec['direction'] != 'write':
                if self.simulated or not self.master:
                    self._display(element, spec, kind, self._value(spec))
                else:
                    self._display(element, spec, kind, None)

    def bind(self, element, spec, kind):
        spec = dict(_BIND_DEFAULTS, **spec)
        self.bindings.append((element, spec, kind))
        for i, value in enumerate(self._words(spec, spec['initial'])):
            self.bank.setdefault((spec['table'], spec['address']+i), value)
        if spec['direction'] == 'read':
            return
        if spec['action'] == 'momentary':
            for attr, release in (('on_press', False), ('on_release', True)):
                self._wrap(element, spec, kind, attr, release)
        else:
            attr = {'click': 'on_click', 'press': 'on_press', 'release': 'on_release',
                    'change': 'on_change', 'enter': 'on_enter'}[spec['event']]
            self._wrap(element, spec, kind, attr, False)

    def _wrap(self, element, spec, kind, attr, release):
        old = getattr(element, attr, None)
        def event(control):
            if old:
                old(control)
            try:
                action = spec['action']
                if action == 'momentary':
                    value = spec['releaseValue'] if release else spec['writeValue']
                elif action == 'constant':
                    value = spec['writeValue']
                elif action == 'toggle':
                    value = spec['releaseValue'] if self._value(spec) == spec['writeValue'] else spec['writeValue']
                elif kind == 'checkbox':
                    value = int(control.checked)
                elif kind == 'slider':
                    value = control.values['value']
                else:
                    value = float(control.text)
                self.write(spec, value, coalesce=kind == 'slider')
            except Exception as error:
                self._status('WRITE ERROR', error)
        setattr(element, attr, event)

    def _words(self, spec, value):
        if not spec['min'] <= value <= spec['max']:
            raise ValueError('Value outside configured limits')
        raw = (value-spec['offset']) / spec['gain']
        dtype = spec['dataType']
        if spec['table'] in ('co', 'di'):
            if raw not in (0, 1):
                raise ValueError('Bit must be 0 or 1')
            return [int(raw)]
        if dtype == 'f32':
            data = _struct.pack('>f', raw)
            words = [data[0]*256+data[1], data[2]*256+data[3]]
        else:
            raw = int(round(raw))
            low, high = {'u16': (0,65535), 'i16': (-32768,32767),
                         'u32': (0,4294967295), 'i32': (-2147483648,2147483647)}[dtype]
            if not low <= raw <= high:
                raise ValueError('Value does not fit register data type')
            words = [raw & 65535] if dtype.endswith('16') else [(raw >> 16) & 65535, raw & 65535]
        return words[::-1] if spec['wordOrder'] == 'low_first' else words

    def _value(self, spec):
        count = 2 if spec['dataType'] in ('u32','i32','f32') else 1
        words = [self.bank[(spec['table'],spec['address']+i)] for i in range(count)]
        if spec['wordOrder'] == 'low_first':
            words.reverse()
        raw = words[0] if count == 1 else words[0]*65536+words[1]
        if spec['dataType'] == 'i16' and raw >= 32768:
            raw -= 65536
        elif spec['dataType'] == 'i32' and raw >= 2147483648:
            raw -= 4294967296
        elif spec['dataType'] == 'f32':
            raw = _struct.unpack('>f', bytes((words[0]>>8,words[0]&255,words[1]>>8,words[1]&255)))[0]
        value = raw*spec['gain']+spec['offset']
        if value != value or value in (float('inf'), -float('inf')):
            raise ValueError('Non-finite register value')
        return value

    def _display(self, element, spec, kind, value):
        key = id(element)
        if key in self.rendered and self.rendered[key] == value:
            return
        if value is None:
            if kind not in ('progress','checkbox','slider','bar','gauge'):
                if hasattr(element, 'set'):
                    element.set(text='--')
                else:
                    element.text = '--'
            self.rendered[key] = value
            return
        if kind in ('progress','slider','bar','gauge'):
            if hasattr(element, 'set'):
                element.set(value=int(value))
            else:
                element.value = max(0,min(100,int(value)))
        elif kind == 'checkbox':
            element.checked = bool(value)
        else:
            text = spec['prefix'] + ('%.*f' % (spec['decimals'],value)) + spec['suffix']
            if hasattr(element, 'set'):
                element.set(text=text)
            else:
                element.text = text
        self.rendered[key] = value

    def _refresh(self):
        # All bound values in one drawing batch where the app has one, so no
        # frame shows some of them new and some old or half drawn.
        with batch():
            self._refresh_all()

    def _refresh_all(self):
        for element, spec, kind in self.bindings:
            if spec['direction'] != 'write':
                self._display(element,spec,kind,self._value(spec))

    def write(self, spec, value, coalesce=False):
        words = self._words(spec, value)
        if spec['table'] not in ('co','hr'):
            raise ValueError('Read-only register table')
        if self.simulated or not self.master:
            self._store(spec['table'],spec['address'],words)
            self._refresh()
            return
        now = _time.ticks_ms()
        if self.last_good is None or _time.ticks_diff(now,self.last_good) > self.config['staleMs']:
            raise OSError('Write blocked: no fresh reply from device')
        req = self._request(spec['table'],spec['address'],words=words)
        if coalesce:
            self.queue = [r for r in self.queue if (r['table'],r['address']) != (req['table'],req['address'])]
        if len(self.queue) >= 16:
            raise OSError('Register write queue full')
        self.queue.append(req)

    def put(self, table, address, *words):
        """Set register words in the link's own bank (table "hr", "ir",
        "co" or "di", zero-based address) and redraw what is bound to
        them: what the app itself measured, for a master to read."""
        self._store(table, address, words)
        self._refresh()

    def get(self, table, address, count=1):
        """The bank's words from address (a list of count)."""
        return [self.bank.get((table, address + i), 0) for i in range(count)]

    def _store(self, table, address, words):
        for i, word in enumerate(words):
            self.bank[(table,address+i)] = word

    def _checked_store(self, table, address, words, enforce_limits=False):
        old = [self.bank.get((table,address+i),0) for i in range(len(words))]
        self._store(table,address,words)
        try:
            for element,spec,kind in self.bindings:
                size = 2 if spec['dataType'] in ('u32','i32','f32') else 1
                if spec['table']==table and spec['address']<address+len(words) and spec['address']+size>address:
                    value=self._value(spec)
                    if enforce_limits and spec['direction']!='read' and not spec['min']<=value<=spec['max']:
                        raise ValueError('Remote write outside configured limits')
        except Exception:
            self._store(table,address,old)
            raise

    def _request(self, table, address, count=1, words=None):
        self.sequence = self.sequence % 65535 + 1
        return {'id': self.sequence, 'unit': self.config['unit'], 'op': 'read' if words is None else 'write',
                'table': table, 'address': address, 'count': count if words is None else len(words),
                'values': words or []}

    def _encode(self, req):
        if self.codec != 'rtu':
            payload = self.custom.encode(req) if self.custom else self.json.dumps(req).encode()
            return bytes(payload) + (b'\n' if self.codec == 'json' or self.config['framing'] == 'newline' else b'')
        fc = {'co':1,'di':2,'hr':3,'ir':4}[req['table']]
        address, count = req['address'], req['count']
        head = bytes((req['unit'],fc,address>>8,address&255,count>>8,count&255))
        if req['op'] == 'write':
            words = req['values']
            if count == 1:
                value = (65280 if words[0] else 0) if req['table'] == 'co' else words[0]
                head = bytes((req['unit'],5 if req['table']=='co' else 6,address>>8,address&255,value>>8,value&255))
            elif req['table'] == 'co':
                data = bytearray((count+7)//8)
                for i, word in enumerate(words):
                    data[i//8] |= bool(word) << (i%8)
                head = bytes((req['unit'],15,address>>8,address&255,count>>8,count&255,len(data)))+data
            else:
                head = bytes((req['unit'],16,address>>8,address&255,count>>8,count&255,count*2))
                head += bytes(v for word in words for v in (word>>8,word&255))
        return _rtu(head)

    def _send(self, data, now):
        if len(data) > 512:
            raise ValueError('Serial frame exceeds 512 bytes')
        if self.de:
            self.de.value(self.config['deActive'])
        try:
            if self.uart.write(data) != len(data):
                raise OSError('Incomplete UART write')
            self.tx_bytes += len(data)
            if self.de:
                self.uart.flush()
        finally:
            if self.de:
                self.de.value(1-self.config['deActive'])
        self.last_sent = _time.ticks_ms()

    def _master_reply(self, data, now):
        if self.pending is None:
            return
        req, sent, retry = self.pending
        if self.codec != 'rtu':
            reply = self.custom.decode(data) if self.custom else self.json.loads(data)
            if not reply or not isinstance(reply,dict):
                return
            if reply.get('id') != req['id'] or reply.get('unit') != req['unit']:
                return
            if reply.get('error'):
                raise OSError('Remote exception %s' % reply['error'])
            words = reply.get('values',[]) if req['op']=='read' else req['values']
            if req['op']=='write' and reply.get('ok') is not True:
                raise ValueError('Write acknowledgment missing')
        else:
            if len(data)<5 or crc16(data[:-2]) != data[-2]+data[-1]*256 or data[0]!=req['unit']:
                return
            fc = {'co':1,'di':2,'hr':3,'ir':4}[req['table']] if req['op']=='read' else (5 if req['table']=='co' else 6) if req['count']==1 else (15 if req['table']=='co' else 16)
            if data[1] == fc|128:
                if len(data) != 5:
                    return
                raise OSError('Modbus exception %d' % data[2])
            if data[1]!=fc:
                return
            if req['op']=='write':
                if data != self._encode(req)[:6]+_rtu(self._encode(req)[:6])[-2:]:
                    return
                words = req['values']
            else:
                size = (req['count']+7)//8 if fc<3 else req['count']*2
                if len(data)!=size+5 or data[2]!=size:
                    return
                words = [(data[3+i//8]>>(i%8))&1 for i in range(req['count'])] if fc<3 else [data[3+i*2]*256+data[4+i*2] for i in range(req['count'])]
        if len(words) != req['count'] or any(not isinstance(v,int) or not 0<=v<=(1 if req['table'] in ('co','di') else 65535) for v in words):
            raise ValueError('Invalid register response')
        self._checked_store(req['table'],req['address'],words)
        self.pending = None
        self.last_good = now
        self._status('ONLINE')
        self._refresh()

    def _serve(self, req):
        table, address, count = req['table'],req['address'],req['count']
        if table=='ir' and self.config.get('inputAlias',False):
            table='hr'
        if table not in ('co','di','hr','ir') or not 1<=count<=125 or not 0<=address<=65535 or address+count>65536:
            return {'error':3}
        if any((table,address+i) not in self.bank for i in range(count)):
            return {'error':2}
        if req['op'] == 'write':
            words = req['values']
            if table not in ('co','hr') or len(words)!=count or any(not isinstance(v,int) or not 0<=v<=(1 if table=='co' else 65535) for v in words):
                return {'error':3}
            try:
                self._checked_store(table,address,words,True)
            except (ValueError, OverflowError):
                return {'error':3}
            self._refresh()
            if self.on_write:
                self.on_write(table,address,words)
            return {'ok':True}
        if req['op'] != 'read':
            return {'error':1}
        if self.before_read:
            self.before_read(table,address,count)
        return {'values':[self.bank[(table,address+i)] for i in range(count)]}

    def _slave_reply(self, data, now):
        if self.codec != 'rtu':
            req = self.custom.decode(data) if self.custom else self.json.loads(data)
            if not req or not isinstance(req,dict):
                return
            if req.get('unit') != self.config['unit']:
                return
            result = self._serve(req)
            result.update(id=req['id'],unit=req['unit'])
            self._send(self.custom.encode(result)+(b'\n' if self.config['framing']=='newline' else b'') if self.custom else self.json.dumps(result).encode()+b'\n',now)
        else:
            if len(data)<8 or crc16(data[:-2])!=data[-2]+data[-1]*256 or data[0] not in (0,self.config['unit']):
                return
            unit, fc = data[0],data[1]
            address, count = data[2]*256+data[3],data[4]*256+data[5]
            table = {1:'co',2:'di',3:'hr',4:'ir',5:'co',6:'hr',15:'co',16:'hr'}.get(fc)
            words = []
            error = 0
            if fc in (1,2,3,4) and len(data)==8:
                op='read'
            elif fc in (5,6) and len(data)==8:
                op='write'
                if fc==5 and count not in (0,65280):
                    error=3
                words=[int(bool(count)) if fc==5 else count];count=1
            elif fc in (15,16) and len(data)>=9 and len(data)==data[6]+9 and data[6]==((count+7)//8 if fc==15 else count*2):
                op='write'
                words=[(data[7+i//8]>>(i%8))&1 for i in range(count)] if fc==15 else [data[7+i*2]*256+data[8+i*2] for i in range(count)]
            else:
                op='invalid';error=1 if table is None else 3
            if unit==0 and op!='write':
                return
            result={'error':error} if error else self._serve({'table':table,'address':address,'count':count,'op':op,'values':words})
            if unit==0:
                if not result.get('error'):
                    self.last_good=now;self.frames+=1;self._status('ONLINE')
                return
            if result.get('error'):
                reply=bytes((unit,fc|128,result['error']))
            elif op=='write':
                reply=data[:6]
            elif fc<3:
                packed=bytearray((count+7)//8)
                for i,value in enumerate(result['values']):
                    packed[i//8] |= bool(value) << (i%8)
                reply=bytes((unit,fc,len(packed)))+packed
            else:
                reply=bytes((unit,fc,count*2))+bytes(v for word in result['values'] for v in (word>>8,word&255))
            self._send(_rtu(reply),now)
        self.last_good=now
        self.frames+=1
        self._status('ONLINE')

    def _rtu_frames(self,now):
        # Explicit lengths tolerate UART FIFO chunks without treating a pause
        # between chunks as the end of a request. Bound resynchronization work.
        # MicroPython bytearray does not implement item/slice deletion.
        # Scan with a cursor, then copy only the unconsumed tail once.
        b,pos=self.rx,0
        try:
            for _ in range(64):
                left=len(b)-pos
                if left<2:break
                if b[pos] not in ((self.config['unit'],) if self.master else (0,self.config['unit'])):
                    pos+=1;continue
                fc=b[pos+1]
                if self.master and fc&128:
                    size=5
                elif self.master and fc in (1,2,3,4):
                    if left<3:break
                    size=b[pos+2]+5
                elif not self.master and fc in (15,16):
                    if left<7:break
                    size=b[pos+6]+9
                else:
                    size=8
                if size>256:
                    pos+=1;self.errors+=1;continue
                if left<size:break
                frame=bytes(b[pos:pos+size])
                if crc16(frame[:-2])!=frame[-2]+frame[-1]*256:
                    pos+=1;self.errors+=1;continue
                pos+=size
                if self.master:self._master_reply(frame,now)
                else:self._slave_reply(frame,now)
        finally:
            if pos:self.rx=b[pos:]

    def tick(self):
        if self.closed or self.simulated or not self.uart:
            return
        now=_time.ticks_ms()
        try:
            n=min(512,self.uart.any())
            if n:
                data=self.uart.read(n)
                if data:
                    self.rx.extend(data);self.rx_bytes+=len(data);self.last_rx=now
                if len(self.rx)>512:
                    self.rx=bytearray();raise ValueError('RX frame overflow')
            newline=self.codec=='json' or self.codec=='custom' and self.config['framing']=='newline'
            if self.codec=='rtu':
                self._rtu_frames(now)
                if self.rx and _time.ticks_diff(now,self.last_rx)>max(350,self.config['timeoutMs']):
                    self.rx=bytearray();self.errors+=1
            elif self.rx and (b'\n' in self.rx if newline else _time.ticks_diff(now,self.last_rx)>=self.gap):
                if newline:
                    frame,rest=bytes(self.rx).split(b'\n',1);self.rx=bytearray(rest)
                else:
                    frame=bytes(self.rx);self.rx=bytearray()
                if self.master:
                    self._master_reply(frame,now)
                else:
                    self._slave_reply(frame,now)
            if self.pending and _time.ticks_diff(now,self.pending[1])>=self.config['timeoutMs']:
                req,sent,retry=self.pending
                if req['op']=='read' and retry<self.config['retries']:
                    self._send(self._encode(req),now);self.pending=(req,now,retry+1)
                else:
                    self.pending=None;self._status('TIMEOUT','Device did not acknowledge request')
                    self.next_poll=_time.ticks_add(now,self.config['pollMs'])
            if self.last_good is not None and _time.ticks_diff(now,self.last_good)>self.config['staleMs']:
                if self.state!='STALE':
                    self._status('STALE')
                    for element,spec,kind in self.bindings:
                        if spec['direction']!='write':
                            self._display(element,spec,kind,None)
                self.queue=[]
            if self.master and not self.pending and not self.rx and _time.ticks_diff(now,self.last_sent)>=self.gap and _time.ticks_diff(now,self.last_rx)>=self.gap:
                req=self.queue.pop(0) if self.queue else None
                if req is None and self.bindings and _time.ticks_diff(now,self.next_poll)>=0:
                    element,spec,kind=self.bindings[self.index%len(self.bindings)]
                    self.index+=1
                    req=self._request(spec['table'],spec['address'],2 if spec['dataType'] in ('u32','i32','f32') else 1)
                    self.next_poll=_time.ticks_add(now,self.config['pollMs'])
                if req:
                    self._send(self._encode(req),now);self.pending=(req,now,0)
        except Exception as error:
            self.errors+=1
            self.pending=None
            self._status('ERROR',error)
            self.next_poll=_time.ticks_add(now,self.config['pollMs'])

    def close(self):
        self.closed=True
        self.queue=[]
        if self.de:
            self.de.value(1-self.config['deActive'])
        if self.uart:
            self.uart.deinit()
            self.uart=None


# ------------------------------------------------------------------ App

class App:
    """A window with pages: the frame of a hand-written app.

        app = tdapp.App("Oven", 60, 16)
        main = app.page("Main")
        main.add(app.win.label(2, 1, "Temperature"))
        app.show("Main")
        app.run()

    Pages hold tdui widgets (anything with .visible); show() makes one
    page's widgets visible and hides the others'. Canvases that need a
    different picture per page use register_canvas() / show_canvases().
    link() starts a serial link (Link) ticked by the window; bind() ties a
    widget to a register."""

    def __init__(self, title, w, h, x=None, y=None, resizable=False, fullscreen=False):
        self.win = tdui.Window(title, w, h, x, y, resizable=resizable)
        if fullscreen:
            self.win.fullscreen(True)
        self.pages = {}
        self.current = None
        self.on_page = None
        self._link = None

    def page(self, name):
        """The page called name (made the first time)."""
        p = self.pages.get(name)
        if p is None:
            p = Page(self, name)
            self.pages[name] = p
        return p

    def show(self, name):
        """Show page name: its widgets and canvases, nobody else's."""
        if name not in self.pages:
            raise ValueError("unknown page")
        with batch():
            for n, p in self.pages.items():
                if n != name:
                    p._visible(False)
            self.pages[name]._visible(True)
            show_canvases(self.win, name)
        self.current = name
        if self.on_page:
            self.on_page(name)

    def every(self, ms, fn):
        return self.win.every(ms, fn)

    def after(self, ms, fn):
        return self.win.after(ms, fn)

    def link(self, config, simulated=False):
        """Start a serial link (see Link) and tick it every 10 ms."""
        self._link = Link(config, simulated)
        self._link.start()
        self.win.every(10, self._link.tick)
        return self._link

    def bind(self, widget, spec, kind=None):
        """Show (or write) a register with a widget: see Link.bind."""
        if self._link is None:
            raise OSError("call link() first")
        if kind is None:
            kind = type(widget).__name__.lower()
        self._link.bind(widget, spec, kind)

    def run(self):
        """Run until the windows are closed; close the link afterwards."""
        try:
            tdui.run()
        finally:
            if self._link:
                self._link.close()


class Page:
    """Widgets that are shown together (App.page). Labels, buttons and the
    other widgets are hidden with .visible; canvases are moved off the
    window, so make them with page.canvas()."""

    def __init__(self, app, name):
        self.app, self.name, self.widgets = app, name, []

    def add(self, *widgets):
        for w in widgets:
            if isinstance(w, tdui.Canvas) and not hasattr(w, "_tdapp_at"):
                raise ValueError("make a page's canvas with page.canvas()")
            self.widgets.append(w)
            if self.app.current is not None and self.app.current != self.name:
                self._show(w, False)
        return widgets[0] if len(widgets) == 1 else widgets

    def canvas(self, x, y, w, h, on_mouse=None, pages=1):
        """A canvas on this page (tdui.Window.canvas), hidden with it."""
        cv = self.app.win.canvas(x, y, w, h, on_mouse=on_mouse, pages=pages)
        cv._tdapp_at = (x, y)
        cv._tdapp_parked = False
        return self.add(cv)

    def _show(self, w, on):
        at = getattr(w, "_tdapp_at", None)
        if at is None:
            w.visible = on
        elif on:
            place_canvas(w, at[0], at[1])
        else:
            hide_canvas(w)

    def _visible(self, on):
        for w in self.widgets:
            self._show(w, on)
