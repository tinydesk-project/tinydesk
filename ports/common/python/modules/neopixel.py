# neopixel.py - WS2812 / SK6812 RGB(W) LED strips through machine.bitstream,
# with the same interface as MicroPython's neopixel module:
#
#     np = NeoPixel(Pin(18), 8)    # 8 LEDs, GRB order (bpp=4 for RGBW)
#     np[0] = (255, 0, 0)
#     np.write()

from machine import bitstream


class NeoPixel:
    # Where R, G, B, W go in each LED's bytes (WS2812: G, R, B).
    ORDER = (1, 0, 2, 3)

    def __init__(self, pin, n, bpp=3, timing=1):
        self.pin = pin
        self.n = n
        self.bpp = bpp
        self.buf = bytearray(n * bpp)
        self.pin.init(pin.OUT)
        # high/low times in ns for a 0 bit and a 1 bit: 800 kHz, or 400 kHz
        if isinstance(timing, int):
            self.timing = (400, 850, 800, 450) if timing else (800, 1700, 1600, 900)
        else:
            self.timing = timing

    def __len__(self):
        return self.n

    def __setitem__(self, i, v):
        offset = i * self.bpp
        for c in range(self.bpp):
            self.buf[offset + self.ORDER[c]] = v[c]

    def __getitem__(self, i):
        offset = i * self.bpp
        return tuple(self.buf[offset + self.ORDER[c]] for c in range(self.bpp))

    def fill(self, v):
        b = self.buf
        n = len(b)
        for c in range(self.bpp):
            x = v[c]
            for i in range(self.ORDER[c], n, self.bpp):
                b[i] = x

    def write(self):
        bitstream(self.pin, 0, self.timing, self.buf)
