# Generated bitmap fonts

`.tdfont` files can be generated from a desktop font. These
immutable files contain only the selected glyphs, so the device needs no TTF
parser and performs no font generation at runtime. A font is loaded once into
native graphics memory, shared by text boxes, pictures and buttons, and drawn
in C with one IPC message per text box. Existing apps without custom fonts
retain their original appearance and terminal fallback behavior.

```python
font = tdui.Font.load('/root/display_28.tdfont')
cv.font_text(font, 2, 2, 40, 4, 'Temperature\n123.4 °C',
             fg=231, bg=25, align='center', valign='middle', wrap=True)
button = cv.button(2, 10, 20, 3, 'Apply', font=font, on_click=apply_value)
# Image skins can use the same native label renderer:
cv.picture(skin, 2, 14, 20, 3, text='Ready', font=font)
```

The bounds are character cells; glyph dimensions are physical display pixels.
CrowPanel font 20 uses 10×20 pixels/cell, font 16 uses 8×16. Generated font size
does not change when the terminal font changes, but wrapping and clipping can.
`font.height`, `ascent`, `glyphs`, `size` and `bpp` describe the loaded file.
Call `font.free()` after removing text using it; script teardown releases all
native font/text allocations. Budget includes file bytes and retained labels.
Images and font labels can share a picture descriptor, avoiding a separately
drawn label replacing its skin. Text boxes use their chosen background or
`tdui.CLEAR`, with bounded clipping and optional word wrapping.

## TDFONT01 format

All numbers are little endian. A file is at most 256 KiB, with 1–512 sorted,
unique Unicode scalar glyphs. `?` is required as a fallback. Rendering supports
UTF-8, variable advance/bearings, newlines and left/center/right and
top/middle/bottom alignment. Complex-script shaping is not implemented.

The 32-byte header contains: magic `TDFONT01` (offset 0), uint16 header size
32 (8), uint8 coverage depth 1 or 4 (10), reserved zero (11), uint16 line
height 1–128 (12), ascent (14), glyph count (16), nominal pixel size 1–128
(18), uint32 table offset 32 (20), bitmap start (24), total file bytes (28).

Each 16-byte glyph record contains uint32 codepoint (0), uint32 absolute
bitmap offset (4), uint16 width ≤256 (8), int16 X bearing −128…128 (10),
uint16 advance 1–256 (12), reserved uint16 zero (14). Bitmap offsets must be
contiguous and consume the exact file. Every glyph uses the font's full line
height; row stride is ceil(width × bpp / 8). One-bit glyphs use MSB first;
4-bit glyphs use the high nibble first. Coverage blends with the destination
palette color using nearest-color conversion, without dithering.

`td_font_open`, `td_font_measure` and `td_font_draw` in `td_font.h` provide the
allocation-free C API. The Python loader validates the whole file before
transferring ownership to the desktop task. Malformed files, replaced labels,
copied pages and script teardown have native tests. Allocation failures report
an error and release the reserved graphics budget.

A text element's background setting applies transparency per element;
font files contain glyph coverage rather than a baked background. CLEAR leaves
the underlying artwork visible. The native terminal fallback is clipped to the
element rectangle, including long and multiline labels, so it cannot leave
stray characters outside the full-resolution picture.
