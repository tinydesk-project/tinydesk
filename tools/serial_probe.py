#!/usr/bin/env python3
"""serial_probe.py - drive tinydesk on real hardware from a script.

Opens the board's serial port like a terminal would, answers the terminal
size query (as an 80x25 terminal unless --size=COLSxROWS is given; --baud=N
sets the line speed, 921600 for the classic ESP32 port), sends keys and
mouse clicks, and saves everything the board sent. Render a capture with
tools/vtshot:  vtshot capture_1.bin 80 25

Use --sixel=8x16 (or 10x20) to answer Sixel and cell-pixel queries. Status
queries are answered in both modes so a long capture stays connected.
For CrowPanel use the LCD's cell size: font 16 is 8x16, font 20 is 10x20.

  python serial_probe.py COM3 out wait=1500 key=Enter wait=500 shot type="ls\\r" wait=800 shot

Actions: wait=MS, type=TEXT (\\r \\n \\t \\e \\xHH escapes), key=NAME,
click=X,Y (0-based cells), shot (write out_<n>.bin with all output so far),
reset (pulse the chip reset through the RTS/DTR control lines).
maintenance (reset normally, then hold BOOT through DTR for maintenance startup).
Needs pyserial (included in the ESP-IDF Python environment).
"""
import sys
import time
import re

import serial

KEYS = {
    "Enter": b"\r", "Esc": b"\x1b", "Tab": b"\t", "Up": b"\x1b[A", "Down": b"\x1b[B",
    "Right": b"\x1b[C", "Left": b"\x1b[D", "F6": b"\x1b[17~", "F10": b"\x1b[21~",
    "F11": b"\x1b[23~", "Bksp": b"\x7f", "CtrlL": b"\x0c", "CtrlC": b"\x03",
    "CtrlQ": b"\x11", "F4": b"\x1b[14~", "Del": b"\x1b[3~",
}


def unescape(text):
    return text.replace("\\e", "\\x1b").encode().decode("unicode_escape").encode("latin-1")


def main():
    args = sys.argv[1:]
    cols, rows = 80, 25
    cell_px = None
    baud = 115200        # the C6's USB Serial/JTAG ignores it; the classic ESP32's UART0 runs at 921600
    while args and args[0].startswith("--"):
        opt = args.pop(0)
        if opt.startswith("--size="):
            cols, rows = (int(v) for v in opt[7:].split("x"))
        elif opt.startswith("--baud="):
            baud = int(opt[7:])
        elif opt.startswith("--sixel="):
            cell_px = tuple(int(v) for v in opt[8:].split("x"))
            if len(cell_px) != 2 or min(cell_px) < 1:
                sys.exit("use --sixel=CELL_WIDTHxCELL_HEIGHT")
        else:
            sys.exit("unknown option " + opt)
    port, prefix, actions = args[0], args[1], args[2:]

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0
    ser.dtr = False      # equal DTR/RTS levels never reset the chip
    ser.rts = False
    ser.open()

    capture = bytearray()
    shots = 0
    tail = b""

    def pump(seconds):
        nonlocal tail
        end = time.time() + seconds
        while time.time() < end:
            data = ser.read(8192)
            if data:
                capture.extend(data)
                # Queries can straddle serial reads. A status reply keeps
                # the link alive; optional Sixel mimics a picture terminal.
                joined = tail + data
                for query in re.finditer(rb"\x1b\[(6n|5n|c|16t)", joined):
                    if query.end() <= len(tail):
                        continue
                    kind = query.group(1)
                    if kind == b"6n":
                        ser.write(b"\x1b[%d;%dR" % (rows, cols))
                    elif kind == b"5n":
                        ser.write(b"\x1b[0n")
                    elif kind == b"c":
                        ser.write(b"\x1b[?1;2;4c" if cell_px else b"\x1b[?1;2c")
                    elif cell_px:
                        ser.write(b"\x1b[6;%d;%dt" % (cell_px[1], cell_px[0]))
                tail = joined[-6:]
            else:
                time.sleep(0.005)

    pump(0.2)
    for a in actions:
        if a.startswith("wait="):
            pump(int(a[5:]) / 1000.0)
        elif a.startswith("type="):
            ser.write(unescape(a[5:]))
            pump(0.1)
        elif a.startswith("key="):
            ser.write(KEYS[a[4:]])
            pump(0.1)
        elif a.startswith("click="):
            x, y = (int(v) for v in a[6:].split(","))
            ser.write(b"\x1b[<0;%d;%dM" % (x + 1, y + 1))
            pump(0.03)
            ser.write(b"\x1b[<0;%d;%dm" % (x + 1, y + 1))
            pump(0.1)
        elif a in ("reset","maintenance"):
            # RTS high with DTR low resets the chip. Windows' usbser.sys only
            # passes an RTS change on when DTR is written too (as esptool does).
            ser.dtr = False
            ser.rts = True
            ser.dtr = ser.dtr
            time.sleep(0.1)
            ser.rts = False
            ser.dtr = ser.dtr
            pump(0.1)
            if a=="maintenance":
                # BOOT goes low only after reset has sampled normal boot.
                ser.dtr=True
                ser.rts=ser.rts
                pump(2.5)
                ser.dtr=False
                ser.rts=ser.rts
        elif a == "shot":
            shots += 1
            name = "%s_%d.bin" % (prefix, shots)
            with open(name, "wb") as f:
                f.write(capture)
            print("%s: %d bytes" % (name, len(capture)))
        else:
            sys.exit("unknown action " + a)
    ser.close()


if __name__ == "__main__":
    main()
