/*
 * qstrdefs_tinydesk.h - names TinyDesk's own Python modules (tdui, tdapp,
 * tinydesk) need in the generated tables that the generator cannot take
 * from the sources (tools/micropython/gen.sh passes it as EXT_QSTR_DEFS).
 * The shell's own are in third_party/tdsh/python/qstrdefsport.h.
 */

// clang-format off

// Native bitmap-font resource loader.
Q(font_load)
Q(enable_rgb_pictures)
Q(graphics_info)
Q(render_cache)
Q(render_cache_info)
Q(scanout_info)
Q(scanout_timing)
Q(SUPPORTS_RGB_PICTURES)
