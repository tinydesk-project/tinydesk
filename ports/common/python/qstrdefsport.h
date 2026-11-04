/*
 * qstrdefsport.h - strings MicroPython's generated headers must contain
 * that the generator (tools/micropython) cannot take from the sources:
 *
 * - "/lib" for sys.path (td_py.c): a name like MP_QSTR__slash_lib is only
 *   turned back into "/lib" when it is listed here;
 * - hashlib.sha1 and hashlib.md5, built only with mbedTLS, which the
 *   generator does not have (see mpconfigport.h).
 */

// clang-format off

Q(/lib)
Q(sha1)
Q(md5)

// Native bitmap-font resource loader.
Q(font_load)
Q(enable_rgb_pictures)
Q(graphics_info)
Q(render_cache)
Q(render_cache_info)
Q(scanout_info)
Q(scanout_timing)
Q(SUPPORTS_RGB_PICTURES)
