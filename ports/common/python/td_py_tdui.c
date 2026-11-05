/*
 * td_py_tdui.c - the _tdui module under tdui (modules/tdui.py): messages
 * to and from the desktop, which makes the windows (apps/pyui.c).
 *
 *   _tdui.open()                          start; OSError without a desktop
 *   _tdui.send(op, id, win, flags, a, b, c, d, e, f, text)
 *   _tdui.recv(timeout_ms)                (op, id, a, b, c, d, text) or None
 *   _tdui.image(id, w, h, color)          a new image on the desktop
 *   _tdui.image_load(id, path, key, dither, w, h)   one from a file: (w, h)
 *   _tdui.pixels(id, x, y, w, h, data, fmt)         bytes into a canvas or image
 *   _tdui.rgb(r, g, b)                    the nearest palette colour
 *
 * Pictures are decoded and converted here, in C on the script's task, and
 * the desktop gets the finished pixels as one pointer: no Python call per
 * pixel, and nothing slow on the desktop's loop.
 *
 * Only a script run on the desktop's own console (the Terminal window, the
 * Editor, Run on the desktop) gets windows; one in an SSH session does not.
 * Its windows close when it ends (td_tdui_release).
 */
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "td_apps.h"
#include "td_py.h"
#include "tinydesk/td_canvas.h"
#include "tinydesk/td_picture.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

#define DETACH_WAIT_MS 3000

static bool (*s_desktop_check)(void);
static bool s_open;

/* Where recv() puts an event's text: on the script's heap, not in RAM for
 * good. */
MP_REGISTER_ROOT_POINTER(char *td_tdui_text);

void td_py_set_desktop_check(bool (*fn)(void))
{
    s_desktop_check = fn;
}

static mp_obj_t tdui_open(void)
{
    if (s_open)
        return mp_const_none;
    if (!s_desktop_check || !s_desktop_check())
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: no desktop in this session"));
    if (!td_pyui_attach())
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: the desktop is not ready (or memory is short)"));
    MP_STATE_VM(td_tdui_text) = m_new(char, TD_PYUI_TEXT_MAX + 1);
    s_open = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_open_obj, tdui_open);

static mp_obj_t tdui_enable_rgb(void)
{
    tdui_open();
    return mp_obj_new_bool(td_pyui_enable_rgb());
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_enable_rgb_obj, tdui_enable_rgb);

static mp_obj_t tdui_graphics_info(void)
{
    uint32_t renders, allocations, sinks, free = 0, largest = 0;
    td_picture_stats(&renders, &allocations, &sinks);
#ifdef ESP_PLATFORM
    free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
#endif
    mp_obj_t values[6] = {mp_obj_new_int_from_uint(renders), mp_obj_new_int_from_uint(allocations), mp_obj_new_int_from_uint(sinks), mp_obj_new_int_from_uint(free), mp_obj_new_int_from_uint(largest), mp_obj_new_int_from_uint(td_stats()->frame_ms)};
    return mp_obj_new_tuple(6, values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_graphics_info_obj, tdui_graphics_info);
static mp_obj_t tdui_scanout_info(void)
{
    uint32_t values[8];
    mp_obj_t result[8];
    td_picture_scanout_info(values);
    for (int i = 0; i < 8; i++)
        result[i] = mp_obj_new_int_from_uint(values[i]);
    return mp_obj_new_tuple(8, result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_scanout_info_obj, tdui_scanout_info);
static mp_obj_t tdui_scanout_timing(void)
{
    uint32_t values[4];
    mp_obj_t result[4];
    td_picture_scanout_timing(values);
    for (int i = 0; i < 4; i++)
        result[i] = mp_obj_new_int_from_uint(values[i]);
    return mp_obj_new_tuple(4, result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_scanout_timing_obj, tdui_scanout_timing);

static int16_t arg16(mp_obj_t o)
{
    mp_int_t v = mp_obj_get_int(o);
    return (int16_t)(v < -32768 ? -32768 : v > 32767 ? 32767
                                                     : v);
}

/* A full queue (the desktop is busy) is waited out; Ctrl+C still works. */
static mp_obj_t tdui_send(size_t n_args, const mp_obj_t *args)
{
    uint8_t op = (uint8_t)mp_obj_get_int(args[0]);
    /* A batch may own first-window construction, so it must be able to
     * attach before Window.__init__ opens the session. */
    if (!s_open && op == TD_PYUI_BATCH_BEGIN)
        tdui_open();
    if (!s_open)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: not open"));
    td_pyui_msg_t m = {.op = op, .id = (uint16_t)mp_obj_get_int(args[1])};
    if (n_args > 2)
        m.win = (uint16_t)mp_obj_get_int(args[2]);
    if (n_args > 3)
        m.flags = (uint8_t)mp_obj_get_int(args[3]);
    int16_t *v[] = {&m.a, &m.b, &m.c, &m.d, &m.e, &m.f};
    for (size_t i = 4; i < n_args && i < 10; i++)
        *v[i - 4] = arg16(args[i]);
    const char *text = "";
    size_t len = 0;
    if (n_args > 10 && args[10] != mp_const_none)
    {
        mp_buffer_info_t b;
        mp_get_buffer_raise(args[10], &b, MP_BUFFER_READ);   /* str or bytes */
        text = b.buf;
        len = b.len;
    }
    if (op == TD_PYUI_FONT_TEXT && (len > 900 || memchr(text, 0, len)))
        mp_raise_ValueError(MP_ERROR_TEXT("tdui: font text needs at most 900 UTF-8 bytes without NUL"));
    if (op == TD_PYUI_PICTURE && (m.flags & 2) && len > 912)
        mp_raise_ValueError(MP_ERROR_TEXT("tdui: font label needs at most 900 UTF-8 bytes"));
    m.len = (uint16_t)(len > TD_PYUI_TEXT_MAX ? TD_PYUI_TEXT_MAX : len);
    while (!td_pyui_send(&m, text))
    {
        mp_event_handle_nowait();
        tdsh_sleep_ms(2);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tdui_send_obj, 2, 11, tdui_send);

static mp_obj_t tdui_render_cache(mp_obj_t kb, mp_obj_t warm)
{
    int limit = mp_obj_get_int(kb);
    if (limit < 0 || limit > 128)
        mp_raise_ValueError(MP_ERROR_TEXT("render cache: 0..128 KiB"));
    tdui_open();
    mp_obj_t args[5] = {MP_OBJ_NEW_SMALL_INT(TD_PYUI_RENDER_CACHE), MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(mp_obj_is_true(warm) ? 1 : 0), MP_OBJ_NEW_SMALL_INT(limit)};
    return tdui_send(5, args);
}
static MP_DEFINE_CONST_FUN_OBJ_2(tdui_render_cache_obj, tdui_render_cache);
static mp_obj_t tdui_render_cache_info(void)
{
    uint32_t values[12];
    td_pyui_cache_stats(values);
    mp_obj_t result[12];
    for (int i = 0; i < 12; i++)
        result[i] = mp_obj_new_int_from_uint(values[i]);
    return mp_obj_new_tuple(12, result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_render_cache_info_obj, tdui_render_cache_info);

/* Wait up to timeout_ms (0: just look) for the next event. */
static mp_obj_t tdui_recv(mp_obj_t timeout_in)
{
    if (!s_open)
        return mp_const_none;
    mp_int_t timeout = mp_obj_get_int(timeout_in);
    uint64_t end = tdsh_monotonic_ms() + (uint64_t)(timeout > 0 ? timeout : 0);
    td_pyui_msg_t m;
    char *text = MP_STATE_VM(td_tdui_text);
    for (;;)
    {
        if (td_pyui_recv(&m, text, TD_PYUI_TEXT_MAX + 1))
            break;
        mp_event_handle_nowait();
        uint64_t now = tdsh_monotonic_ms();
        if (now >= end)
            return mp_const_none;
        uint64_t left = end - now;
        tdsh_sleep_ms(left < 5 ? (uint32_t)left : 5);
    }
    mp_obj_t t[7] = {
        MP_OBJ_NEW_SMALL_INT(m.op),
        MP_OBJ_NEW_SMALL_INT(m.id),
        MP_OBJ_NEW_SMALL_INT(m.a),
        MP_OBJ_NEW_SMALL_INT(m.b),
        MP_OBJ_NEW_SMALL_INT(m.c),
        MP_OBJ_NEW_SMALL_INT(m.d),
        m.len ? mp_obj_new_str(text, m.len) : mp_const_none,
    };
    return mp_obj_new_tuple(7, t);
}
static MP_DEFINE_CONST_FUN_OBJ_1(tdui_recv_obj, tdui_recv);

/* ------------------------------------------------------------ pixels */

static void check_open(void)
{
    if (!s_open)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: not open"));
}

/* While the desktop's queue is full (Ctrl+C is not looked at here: the
 * image would be lost). */
static void queue_wait(void)
{
    tdsh_sleep_ms(2);
}

/* A new image within the script's pixel bytes. */
static td_pyui_image_t *new_image(mp_int_t w, mp_int_t h, bool mask, bool rgb)
{
    if (w < 1 || h < 1 || w > 4096 || h > 4096)
        mp_raise_ValueError(MP_ERROR_TEXT("tdui: an image is 1 to 4096 pixels each way"));
    long bytes = (long)w * h * ((mask ? 2 : 1) + (rgb ? 2 : 0));
    if (!td_pyui_pixels_take(bytes))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("tdui: images and canvas pixels are limited to %d bytes in all"),
                          TD_PYUI_MAX_PIXELS);
    td_pyui_image_t *img = td_pyui_image_new_rgb((int)w, (int)h, mask, rgb);
    if (!img)
    {
        td_pyui_pixels_give(bytes);
        mp_raise_type(&mp_type_MemoryError);
    }
    return img;
}

/* _tdui.image(id, w, h, color): every pixel that colour (-2: transparent). */
static mp_obj_t tdui_image(size_t n_args, const mp_obj_t *args)
{
    (void)n_args;
    check_open();
    mp_int_t color = mp_obj_get_int(args[3]);
    td_pyui_image_t *img = new_image(mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), color == TD_PYUI_CLEAR, false);
    size_t n = (size_t)img->w * (size_t)img->h;
    memset(img->px, color >= 0 ? (int)(color & 255) : 0, n);
    if (img->mask)
        memset(img->mask, 1, n);
    td_pyui_send_image(TD_PYUI_IMAGE, (uint16_t)mp_obj_get_int(args[0]), 0, 0, img, queue_wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tdui_image_obj, 4, 4, tdui_image);

/* The image decoder (apps/imagefile.c) reads files in parts. */
static int real_read_at(const char *path, uint32_t offset, char *buf, int cap)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    int n = -1;
    if (fseek(f, (long)offset, SEEK_SET) == 0)
        n = (int)fread(buf, 1, (size_t)cap, f);
    fclose(f);
    return n;
}

static const td_fs_ops_t s_real_fs = {.read_at = real_read_at};

/* _tdui.image_load(id, path, key, dither, w, h) -> (w, h). path is an
 * absolute TinyDesk path; key 0xRRGGBB is transparent (-1: none); w, h
 * scale the picture (0 for both: as it is; one of them 0: keep the shape). */
static mp_obj_t tdui_image_load(size_t n_args, const mp_obj_t *args)
{
    (void)n_args;
    check_open();
    char real[TDSH_MAX_PATH];
    tdsh_session_t *session = td_py_session();
    int rc = session ? tdsh_path_to_real(session, mp_obj_str_get_str(args[1]), real, sizeof(real), NULL, 0) : -5;
    if (rc)
        mp_raise_OSError(-rc);
    mp_int_t key = mp_obj_get_int(args[2]);
    bool dither = mp_obj_is_true(args[3]);
    mp_int_t fw = mp_obj_get_int(args[4]), fh = mp_obj_get_int(args[5]);

    td_image_t im;
    char err[80];
    if (!td_image_load(&im, &s_real_fs, real, err, sizeof(err)))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("tdui: %s"), err);
    if (fw <= 0 && fh <= 0)
    {
        fw = im.w;
        fh = im.h;
    }
    else if (fw <= 0)
        fw = (mp_int_t)((long)im.w * fh / im.h);
    else if (fh <= 0)
        fh = (mp_int_t)((long)im.h * fw / im.w);
    fw = fw < 1 ? 1 : fw;
    fh = fh < 1 ? 1 : fh;
    uint8_t *rgb = fw <= 4096 && fh <= 4096 ? malloc((size_t)im.w * 3) : NULL;
    if (!rgb)
    {
        td_image_close(&im);
        if (fw > 4096 || fh > 4096)
            mp_raise_ValueError(MP_ERROR_TEXT("tdui: an image is 1 to 4096 pixels each way: give w or h"));
        mp_raise_type(&mp_type_MemoryError);
    }
    nlr_buf_t nlr;
    if (nlr_push(&nlr) != 0)
    {
        free(rgb);
        td_image_close(&im);
        nlr_jump(nlr.ret_val);
    }
    td_pyui_image_t *img = new_image(fw, fh, key >= 0, td_pyui_rgb_enabled());
    nlr_pop();

    /* Each stored row (in the file's order) fills the image rows that come
     * from it: nearest pixel either way. */
    int y;
    while (td_image_row(&im, &y, rgb))
    {
        int lo = (int)(((long)y * fh + im.h - 1) / im.h), hi = (int)(((long)(y + 1) * fh + im.h - 1) / im.h);
        for (int dy = lo; dy < hi && dy < fh; dy++)
        {
            uint8_t *px = img->px + (size_t)dy * (size_t)fw;
            uint8_t *mask = img->mask ? img->mask + (size_t)dy * (size_t)fw : NULL;
            for (int dx = 0; dx < fw; dx++)
            {
                const uint8_t *p = rgb + 3 * ((long)dx * im.w / fw);
                px[dx] = dither ? td_rgb256_dither(p[0], p[1], p[2], dx, dy) : td_rgb256(p[0], p[1], p[2]);
                if (img->rgb)
                    img->rgb[(size_t)dy * fw + dx] = td_rgb565(p[0], p[1], p[2]);
                if (mask)
                    mask[dx] = ((mp_int_t)p[0] << 16 | p[1] << 8 | p[2]) == key;
            }
        }
    }
    free(rgb);
    td_image_close(&im);
    td_pyui_send_image(TD_PYUI_IMAGE, (uint16_t)mp_obj_get_int(args[0]), 0, 0, img, queue_wait);
    mp_obj_t size[2] = {MP_OBJ_NEW_SMALL_INT(fw), MP_OBJ_NEW_SMALL_INT(fh)};
    return mp_obj_new_tuple(2, size);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tdui_image_load_obj, 6, 6, tdui_image_load);

/* Generated fonts stay in native memory, once per file, shared by labels. */
static mp_obj_t tdui_font_load(mp_obj_t id, mp_obj_t path)
{
    check_open();
    uint16_t font_id = (uint16_t)mp_obj_get_int(id);
    char real[TDSH_MAX_PATH];
    tdsh_session_t *session = td_py_session();
    int rc = session ? tdsh_path_to_real(session, mp_obj_str_get_str(path), real, sizeof(real), NULL, 0) : -5;
    if (rc)
        mp_raise_OSError(-rc);
    FILE *file = fopen(real, "rb");
    if (!file)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: font file cannot be read"));
    long length = -1;
    if (fseek(file, 0, SEEK_END) == 0)
        length = ftell(file);
    if (length < 32 || length > (long)TD_FONT_MAX_BYTES || fseek(file, 0, SEEK_SET))
    {
        fclose(file);
        mp_raise_ValueError(MP_ERROR_TEXT("tdui: invalid generated font size"));
    }
    long bytes = (long)sizeof(td_pyui_font_t) + length;
    if (!td_pyui_pixels_take(bytes))
    {
        fclose(file);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: font exceeds graphics memory budget"));
    }
    td_pyui_font_t *font = malloc((size_t)bytes);
    if (!font)
    {
        fclose(file);
        td_pyui_pixels_give(bytes);
        mp_raise_type(&mp_type_MemoryError);
    }
    font->bytes = (size_t)bytes;
    bool read = fread(font->data, 1, (size_t)length, file) == (size_t)length;
    fclose(file);
    if (!read || !td_font_open(&font->face, font->data, (size_t)length))
    {
        free(font);
        td_pyui_pixels_give(bytes);
        mp_raise_ValueError(MP_ERROR_TEXT("tdui: invalid TDFONT01 font"));
    }
    mp_obj_t info[5] = {MP_OBJ_NEW_SMALL_INT(font->face.height), MP_OBJ_NEW_SMALL_INT(font->face.ascent),
                        MP_OBJ_NEW_SMALL_INT(font->face.count), MP_OBJ_NEW_SMALL_INT(font->face.pixels), MP_OBJ_NEW_SMALL_INT(font->face.bpp)};
    nlr_buf_t nlr;
    if (nlr_push(&nlr) != 0)
    {
        free(font);
        td_pyui_pixels_give(bytes);
        nlr_jump(nlr.ret_val);
    }
    mp_obj_t result = mp_obj_new_tuple(5, info);
    td_pyui_send_font(font_id, font, queue_wait);
    nlr_pop();
    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_2(tdui_font_load_obj, tdui_font_load);

/* _tdui.pixels(id, x, y, w, h, data, fmt): data is w x h palette indexes
 * (fmt 0) or RGB triples, dithered (1) or the nearest colour (2), into the
 * canvas or image id at x, y. */
static mp_obj_t tdui_pixels(size_t n_args, const mp_obj_t *args)
{
    (void)n_args;
    check_open();
    mp_int_t w = mp_obj_get_int(args[3]), h = mp_obj_get_int(args[4]), fmt = mp_obj_get_int(args[6]);
    mp_int_t x0 = mp_obj_get_int(args[1]), y0 = mp_obj_get_int(args[2]);
    mp_buffer_info_t b;
    mp_get_buffer_raise(args[5], &b, MP_BUFFER_READ);
    if (w < 1 || h < 1 || w > 4096 || h > 4096 || b.len < (size_t)(w * h * (fmt ? 3 : 1)))
        mp_raise_ValueError(MP_ERROR_TEXT("tdui: pixels: data is w * h bytes (rgb: 3 a pixel)"));
    td_pyui_image_t *img = td_pyui_image_new_rgb((int)w, (int)h, false, fmt && td_pyui_rgb_enabled());
    if (!img)
        mp_raise_type(&mp_type_MemoryError);
    const uint8_t *src = b.buf;
    if (fmt == 0)
        memcpy(img->px, src, (size_t)(w * h));
    else
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++, src += 3)
            {
                img->px[y * w + x] = fmt == 1 ? td_rgb256_dither(src[0], src[1], src[2], (int)x0 + x, (int)y0 + y)
                                              : td_rgb256(src[0], src[1], src[2]);
                if (img->rgb)
                    img->rgb[y * w + x] = td_rgb565(src[0], src[1], src[2]);
            }
    td_pyui_send_image(TD_PYUI_IMAGE_DATA, (uint16_t)mp_obj_get_int(args[0]), (int)x0, (int)y0, img, queue_wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tdui_pixels_obj, 7, 7, tdui_pixels);

static mp_obj_t tdui_rgb(mp_obj_t r, mp_obj_t g, mp_obj_t b)
{
    return MP_OBJ_NEW_SMALL_INT(td_rgb256((uint8_t)mp_obj_get_int(r), (uint8_t)mp_obj_get_int(g),
                                          (uint8_t)mp_obj_get_int(b)));
}
static MP_DEFINE_CONST_FUN_OBJ_3(tdui_rgb_obj, tdui_rgb);

void td_tdui_release(void)
{
    if (!s_open)
        return;
    s_open = false;
    td_pyui_detach_begin();
    /* The desktop closes the windows on its next tick. If it does not let go
     * in time, the queues stay allocated until the next script asks. */
    uint64_t end = tdsh_monotonic_ms() + DETACH_WAIT_MS;
    while (!td_pyui_detach_done() && tdsh_monotonic_ms() < end)
        tdsh_sleep_ms(5);
}

static const mp_rom_map_elem_t tdui_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__tdui)},
    {MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&tdui_open_obj)},
    {MP_ROM_QSTR(MP_QSTR_enable_rgb_pictures), MP_ROM_PTR(&tdui_enable_rgb_obj)},
    {MP_ROM_QSTR(MP_QSTR_graphics_info), MP_ROM_PTR(&tdui_graphics_info_obj)},
    {MP_ROM_QSTR(MP_QSTR_scanout_info), MP_ROM_PTR(&tdui_scanout_info_obj)},
    {MP_ROM_QSTR(MP_QSTR_scanout_timing), MP_ROM_PTR(&tdui_scanout_timing_obj)},
    {MP_ROM_QSTR(MP_QSTR_render_cache), MP_ROM_PTR(&tdui_render_cache_obj)},
    {MP_ROM_QSTR(MP_QSTR_render_cache_info), MP_ROM_PTR(&tdui_render_cache_info_obj)},
    {MP_ROM_QSTR(MP_QSTR_SUPPORTS_RGB_PICTURES), MP_ROM_INT(TD_PYUI_MAX_CELLS > 2000)},
    {MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&tdui_send_obj)},
    {MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&tdui_recv_obj)},
    {MP_ROM_QSTR(MP_QSTR_MAX_CELLS), MP_ROM_INT(TD_PYUI_MAX_CELLS)},
    {MP_ROM_QSTR(MP_QSTR_MAX_PIXELS), MP_ROM_INT(TD_PYUI_MAX_PIXELS)},
    {MP_ROM_QSTR(MP_QSTR_MAX_PICTURES), MP_ROM_INT(TD_PYUI_MAX_CELLS > 2000 ? 512 : 48)},
    {MP_ROM_QSTR(MP_QSTR_MAX_VISIBLE_PICTURES), MP_ROM_INT(TD_PYUI_MAX_CELLS > 2000 ? 128 : 64)},
    {MP_ROM_QSTR(MP_QSTR_image), MP_ROM_PTR(&tdui_image_obj)},
    {MP_ROM_QSTR(MP_QSTR_image_load), MP_ROM_PTR(&tdui_image_load_obj)},
    {MP_ROM_QSTR(MP_QSTR_font_load), MP_ROM_PTR(&tdui_font_load_obj)},
    {MP_ROM_QSTR(MP_QSTR_pixels), MP_ROM_PTR(&tdui_pixels_obj)},
    {MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&tdui_rgb_obj)},
};
static MP_DEFINE_CONST_DICT(tdui_module_globals, tdui_module_globals_table);

const mp_obj_module_t td_tdui_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&tdui_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__tdui, td_tdui_module);
