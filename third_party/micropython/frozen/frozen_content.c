#include "py/mpconfig.h"
#include "py/objint.h"
#include "py/objstr.h"
#include "py/emitglue.h"
#include "py/nativeglue.h"

#if MICROPY_LONGINT_IMPL != 2
#error "incompatible MICROPY_LONGINT_IMPL"
#endif

#if MPZ_DIG_SIZE != 16
#error "incompatible MPZ_DIG_SIZE"
#endif

#if MICROPY_PY_BUILTINS_FLOAT
typedef struct _mp_obj_float_t {
    mp_obj_base_t base;
    mp_float_t value;
} mp_obj_float_t;
#endif

#if MICROPY_PY_BUILTINS_COMPLEX
typedef struct _mp_obj_complex_t {
    mp_obj_base_t base;
    mp_float_t real;
    mp_float_t imag;
} mp_obj_complex_t;
#endif

enum {
    MP_QSTR__0x00_ = MP_QSTRnumber_of,
    MP_QSTR__percent_s_0x00__percent_s_0x00__percent_s,
    MP_QSTR__dot__colon_,
    MP_QSTR_Button,
    MP_QSTR_CLEAR,
    MP_QSTR_Canvas,
    MP_QSTR_CanvasButton,
    MP_QSTR_Checkbox,
    MP_QSTR_Event,
    MP_QSTR_Font,
    MP_QSTR_IOQueue,
    MP_QSTR_Image,
    MP_QSTR_Label,
    MP_QSTR_List,
    MP_QSTR_Lock,
    MP_QSTR_Lock_space_not_space_acquired,
    MP_QSTR_Loop,
    MP_QSTR_MAX_PAGES,
    MP_QSTR_MAX_WIDGETS,
    MP_QSTR_MAX_WINDOWS,
    MP_QSTR_NeoPixel,
    MP_QSTR_OK,
    MP_QSTR_ORDER,
    MP_QSTR_PROTOCOL_TLS_CLIENT,
    MP_QSTR_Progress,
    MP_QSTR_SOCK_STREAM,
    MP_QSTR_SOL_SOCKET,
    MP_QSTR_SO_REUSEADDR,
    MP_QSTR_SSLContext,
    MP_QSTR_SUPPORTS_GRADIENT_FILLS,
    MP_QSTR_Server,
    MP_QSTR_SingletonGenerator,
    MP_QSTR_Stream,
    MP_QSTR_StreamReader,
    MP_QSTR_StreamWriter,
    MP_QSTR_Textbox,
    MP_QSTR_ThreadSafeFlag,
    MP_QSTR_TimeoutError,
    MP_QSTR_Timer,
    MP_QSTR_Window,
    MP_QSTR_Yes_pipe_No,
    MP_QSTR__Batch,
    MP_QSTR__KEYS,
    MP_QSTR__Pixels,
    MP_QSTR__Remove,
    MP_QSTR__Widget,
    MP_QSTR___version__,
    MP_QSTR__add,
    MP_QSTR__alive,
    MP_QSTR__attrs,
    MP_QSTR__bar_colors,
    MP_QSTR__batch,
    MP_QSTR__buttons,
    MP_QSTR__cells,
    MP_QSTR__checked,
    MP_QSTR__dequeue,
    MP_QSTR__dispatch,
    MP_QSTR__draw,
    MP_QSTR__enqueue,
    MP_QSTR__event,
    MP_QSTR__exc_context,
    MP_QSTR__exc_handler,
    MP_QSTR__file,
    MP_QSTR__gone,
    MP_QSTR__gradient_data,
    MP_QSTR__held,
    MP_QSTR__hit,
    MP_QSTR__io_queue,
    MP_QSTR__items,
    MP_QSTR__key_name,
    MP_QSTR__kids,
    MP_QSTR__n,
    MP_QSTR__new_id,
    MP_QSTR__next,
    MP_QSTR__objs,
    MP_QSTR__page,
    MP_QSTR__part,
    MP_QSTR__promote_to_task,
    MP_QSTR__req,
    MP_QSTR__req2,
    MP_QSTR__request,
    MP_QSTR__results,
    MP_QSTR__run,
    MP_QSTR__run_timers,
    MP_QSTR__selected,
    MP_QSTR__send,
    MP_QSTR__serve,
    MP_QSTR__stop_task,
    MP_QSTR__stopper,
    MP_QSTR__text,
    MP_QSTR__timers,
    MP_QSTR__title,
    MP_QSTR__value,
    MP_QSTR__visible,
    MP_QSTR__widgets,
    MP_QSTR__windows,
    MP_QSTR_a,
    MP_QSTR_accept,
    MP_QSTR_aclose,
    MP_QSTR_acquire,
    MP_QSTR_across,
    MP_QSTR_after,
    MP_QSTR_align,
    MP_QSTR_alt_plus_,
    MP_QSTR_ascent,
    MP_QSTR_asyncio,
    MP_QSTR_asyncio_slash___init___dot_py,
    MP_QSTR_asyncio_slash_core_dot_py,
    MP_QSTR_asyncio_slash_event_dot_py,
    MP_QSTR_asyncio_slash_funcs_dot_py,
    MP_QSTR_asyncio_slash_lock_dot_py,
    MP_QSTR_asyncio_slash_stream_dot_py,
    MP_QSTR_attr,
    MP_QSTR_aw,
    MP_QSTR_awrite,
    MP_QSTR_awritestr,
    MP_QSTR_b,
    MP_QSTR_backlog,
    MP_QSTR_backspace,
    MP_QSTR_bar,
    MP_QSTR_bar_space_range_space_or_space_colour,
    MP_QSTR_batch,
    MP_QSTR_bg,
    MP_QSTR_big_size,
    MP_QSTR_bind,
    MP_QSTR_border,
    MP_QSTR_bottom,
    MP_QSTR_bpp,
    MP_QSTR_buf,
    MP_QSTR_button,
    MP_QSTR_buttons,
    MP_QSTR_c,
    MP_QSTR_call_exception_handler,
    MP_QSTR_can_squot_t_space_gather,
    MP_QSTR_canvas,
    MP_QSTR_canvas_space_page_colon__space_0_space_to_space__percent_d,
    MP_QSTR_canvas_space_pages_colon__space_1_space_to_space__percent_d,
    MP_QSTR_canvas_space_size,
    MP_QSTR_cb,
    MP_QSTR_ch,
    MP_QSTR_checkbox,
    MP_QSTR_checked,
    MP_QSTR_circle,
    MP_QSTR_color,
    MP_QSTR_confirm,
    MP_QSTR_connect,
    MP_QSTR_context,
    MP_QSTR_copy_page,
    MP_QSTR_core,
    MP_QSTR_coro_equals_,
    MP_QSTR_coroutine_space_expected,
    MP_QSTR_create_task,
    MP_QSTR_ctrl_plus_,
    MP_QSTR_current_task,
    MP_QSTR_cv,
    MP_QSTR_d,
    MP_QSTR_default_exception_handler,
    MP_QSTR_delete,
    MP_QSTR_dither,
    MP_QSTR_do_handshake_on_connect,
    MP_QSTR_down,
    MP_QSTR_drain,
    MP_QSTR_draw,
    MP_QSTR_draw_text,
    MP_QSTR_draw_to,
    MP_QSTR_drawing,
    MP_QSTR_dst,
    MP_QSTR_dt,
    MP_QSTR_due,
    MP_QSTR_edge,
    MP_QSTR_enabled,
    MP_QSTR_enter,
    MP_QSTR_er,
    MP_QSTR_error,
    MP_QSTR_esc,
    MP_QSTR_ev,
    MP_QSTR_event,
    MP_QSTR_every,
    MP_QSTR_exc,
    MP_QSTR_exc_type,
    MP_QSTR_exception,
    MP_QSTR_f,
    MP_QSTR_f1,
    MP_QSTR_f10,
    MP_QSTR_f11,
    MP_QSTR_f12,
    MP_QSTR_f2,
    MP_QSTR_f3,
    MP_QSTR_f4,
    MP_QSTR_f5,
    MP_QSTR_f6,
    MP_QSTR_f7,
    MP_QSTR_f8,
    MP_QSTR_f9,
    MP_QSTR_fg,
    MP_QSTR_fill,
    MP_QSTR_flags,
    MP_QSTR_fn,
    MP_QSTR_focus,
    MP_QSTR_font,
    MP_QSTR_font_text,
    MP_QSTR_free,
    MP_QSTR_fullscreen,
    MP_QSTR_funcs,
    MP_QSTR_future,
    MP_QSTR_future_colon_,
    MP_QSTR_g,
    MP_QSTR_gather,
    MP_QSTR_get_event_loop,
    MP_QSTR_get_exception_handler,
    MP_QSTR_get_extra_info,
    MP_QSTR_getaddrinfo,
    MP_QSTR_glyphs,
    MP_QSTR_gradient,
    MP_QSTR_gradient_space_RGB_space_range,
    MP_QSTR_h,
    MP_QSTR_height,
    MP_QSTR_home,
    MP_QSTR_host,
    MP_QSTR_i,
    MP_QSTR_idx,
    MP_QSTR_img,
    MP_QSTR_inputbox,
    MP_QSTR_is_open,
    MP_QSTR_is_set,
    MP_QSTR_item,
    MP_QSTR_kb,
    MP_QSTR_kind,
    MP_QSTR_label,
    MP_QSTR_left,
    MP_QSTR_line,
    MP_QSTR_list_space_size,
    MP_QSTR_listen,
    MP_QSTR_lock,
    MP_QSTR_locked,
    MP_QSTR_loop,
    MP_QSTR_main_task,
    MP_QSTR_maximize,
    MP_QSTR_maximum,
    MP_QSTR_menu,
    MP_QSTR_message,
    MP_QSTR_middle,
    MP_QSTR_minimize,
    MP_QSTR_mods,
    MP_QSTR_move,
    MP_QSTR_ms,
    MP_QSTR_msgbox,
    MP_QSTR_n,
    MP_QSTR_neopixel_dot_py,
    MP_QSTR_new_event_loop,
    MP_QSTR_nine,
    MP_QSTR_no_space_running_space_event_space_loop,
    MP_QSTR_on_activate,
    MP_QSTR_on_cancel,
    MP_QSTR_on_change,
    MP_QSTR_on_click,
    MP_QSTR_on_close,
    MP_QSTR_on_enter,
    MP_QSTR_on_key,
    MP_QSTR_on_mouse,
    MP_QSTR_on_press,
    MP_QSTR_on_release,
    MP_QSTR_on_resize,
    MP_QSTR_on_select,
    MP_QSTR_op,
    MP_QSTR_open_connection,
    MP_QSTR_out_buf,
    MP_QSTR_outline,
    MP_QSTR_page,
    MP_QSTR_pages,
    MP_QSTR_peername,
    MP_QSTR_pgdn,
    MP_QSTR_pgup,
    MP_QSTR_pheight,
    MP_QSTR_picture,
    MP_QSTR_pin,
    MP_QSTR_poller,
    MP_QSTR_port,
    MP_QSTR_pressed,
    MP_QSTR_progress,
    MP_QSTR_prompt,
    MP_QSTR_put,
    MP_QSTR_pwidth,
    MP_QSTR_queue_read,
    MP_QSTR_queue_write,
    MP_QSTR_quit,
    MP_QSTR_radius,
    MP_QSTR_readexactly,
    MP_QSTR_repeat,
    MP_QSTR_req,
    MP_QSTR_resizable,
    MP_QSTR_resize,
    MP_QSTR_restore,
    MP_QSTR_return_exceptions,
    MP_QSTR_rgb1,
    MP_QSTR_rgb2,
    MP_QSTR_right,
    MP_QSTR_rim,
    MP_QSTR_run,
    MP_QSTR_run_async,
    MP_QSTR_run_forever,
    MP_QSTR_run_until_complete,
    MP_QSTR_runq_len,
    MP_QSTR_s,
    MP_QSTR_scale,
    MP_QSTR_secret,
    MP_QSTR_selected,
    MP_QSTR_server_hostname,
    MP_QSTR_server_side,
    MP_QSTR_set_exception_handler,
    MP_QSTR_setblocking,
    MP_QSTR_setsockopt,
    MP_QSTR_sgen,
    MP_QSTR_sheet,
    MP_QSTR_shift_plus_,
    MP_QSTR_show,
    MP_QSTR_showing,
    MP_QSTR_size,
    MP_QSTR_small,
    MP_QSTR_socket,
    MP_QSTR_src,
    MP_QSTR_src_down,
    MP_QSTR_ssl,
    MP_QSTR_start_server,
    MP_QSTR_stream,
    MP_QSTR_stream_awrite,
    MP_QSTR_sz,
    MP_QSTR_t,
    MP_QSTR_tab,
    MP_QSTR_task,
    MP_QSTR_tb,
    MP_QSTR_tdui_dot_py,
    MP_QSTR_tdui_colon__space_,
    MP_QSTR_tdui_colon__space_at_space_most_space__percent_d_space_widgets,
    MP_QSTR_tdui_colon__space_at_space_most_space__percent_d_space_windows,
    MP_QSTR_text,
    MP_QSTR_text_size,
    MP_QSTR_textbox,
    MP_QSTR_ticks,
    MP_QSTR_timing,
    MP_QSTR_title,
    MP_QSTR_top,
    MP_QSTR_traceback,
    MP_QSTR_track,
    MP_QSTR_up,
    MP_QSTR_v,
    MP_QSTR_valign,
    MP_QSTR_visible,
    MP_QSTR_w,
    MP_QSTR_wait,
    MP_QSTR_wait_closed,
    MP_QSTR_wait_for,
    MP_QSTR_wait_for_ms,
    MP_QSTR_wait_io_event,
    MP_QSTR_waiter,
    MP_QSTR_waiting,
    MP_QSTR_waitq_len,
    MP_QSTR_warm,
    MP_QSTR_width,
    MP_QSTR_win,
    MP_QSTR_window_space_size,
    MP_QSTR_wrap,
    MP_QSTR_wrap_socket,
    MP_QSTR_x,
    MP_QSTR_x0,
    MP_QSTR_x1,
    MP_QSTR_y,
    MP_QSTR_y0,
    MP_QSTR_y1,
};

const qstr_hash_t mp_qstr_frozen_const_hashes[] = {
    46501,
    9779,
    26321,
    58067,
    40860,
    56237,
    42171,
    33078,
    35561,
    45174,
    26002,
    41058,
    1891,
    49415,
    59534,
    43943,
    59673,
    17966,
    63985,
    55025,
    33641,
    29537,
    26955,
    10465,
    44216,
    48690,
    57103,
    21281,
    48598,
    29123,
    2976,
    12601,
    55353,
    39580,
    35638,
    3245,
    9558,
    19302,
    8098,
    51305,
    32663,
    41510,
    24574,
    8513,
    37244,
    4246,
    38975,
    15579,
    46221,
    24346,
    58426,
    41478,
    30719,
    54639,
    58973,
    49770,
    48522,
    8282,
    26816,
    2134,
    13092,
    52355,
    7740,
    35417,
    61587,
    63103,
    7983,
    43474,
    24380,
    26421,
    20879,
    30036,
    14164,
    51901,
    21966,
    33225,
    33581,
    24914,
    58300,
    23310,
    1451,
    52864,
    58803,
    55096,
    65235,
    56390,
    55469,
    5648,
    12837,
    41831,
    1614,
    62138,
    32465,
    19092,
    8709,
    49733,
    46532,
    35205,
    24658,
    54045,
    10906,
    2913,
    64424,
    40695,
    4555,
    44357,
    63415,
    61558,
    25345,
    46624,
    16486,
    35185,
    48310,
    28211,
    49657,
    2796,
    46535,
    43370,
    8042,
    18420,
    60286,
    4121,
    28352,
    40499,
    25732,
    23017,
    11626,
    18887,
    18804,
    24819,
    32544,
    46534,
    19688,
    4535,
    63693,
    26894,
    26460,
    33896,
    28388,
    28398,
    7766,
    4290,
    56759,
    1752,
    60919,
    15835,
    55546,
    25676,
    56254,
    21001,
    50473,
    21139,
    39559,
    8734,
    28400,
    46529,
    45889,
    59292,
    57187,
    8697,
    1591,
    18805,
    4517,
    61831,
    11905,
    33893,
    8102,
    28053,
    8305,
    27334,
    40672,
    20077,
    28082,
    4541,
    9328,
    28086,
    27081,
    26200,
    9435,
    52220,
    60370,
    46531,
    28178,
    12386,
    12387,
    12384,
    28177,
    28176,
    28183,
    28182,
    28181,
    28180,
    28187,
    28186,
    28228,
    13514,
    36858,
    28237,
    60745,
    11158,
    22932,
    15089,
    42202,
    27400,
    14592,
    22842,
    46530,
    2184,
    19971,
    60604,
    16583,
    6254,
    47708,
    43683,
    52651,
    46541,
    13306,
    13674,
    13605,
    46540,
    21584,
    21286,
    38406,
    31764,
    58562,
    8592,
    28652,
    429,
    59203,
    19934,
    7371,
    53826,
    3788,
    9134,
    47631,
    9273,
    31740,
    19395,
    27501,
    51446,
    48238,
    57512,
    22173,
    49200,
    49780,
    28347,
    61225,
    46539,
    49582,
    3209,
    21769,
    49107,
    53250,
    37053,
    51677,
    50389,
    43341,
    35219,
    25548,
    3770,
    14684,
    18962,
    55753,
    63283,
    28538,
    25560,
    30085,
    63333,
    53558,
    63365,
    8320,
    47256,
    46967,
    48810,
    49833,
    29682,
    42221,
    55388,
    47875,
    26584,
    40417,
    28788,
    43891,
    52601,
    8726,
    63836,
    4093,
    23753,
    2546,
    35683,
    35832,
    34487,
    40749,
    47572,
    57539,
    57536,
    47845,
    36083,
    35180,
    1493,
    25036,
    37871,
    52773,
    46550,
    20861,
    55987,
    56364,
    61272,
    61284,
    5288,
    6254,
    59448,
    32154,
    14442,
    54798,
    43654,
    36326,
    42272,
    31162,
    52320,
    36103,
    25354,
    36329,
    63679,
    54361,
    59482,
    28908,
    46545,
    25074,
    40488,
    28563,
    25454,
    56819,
    46557,
    19997,
    44952,
    62178,
    19597,
    2115,
    9013,
    49253,
    25390,
    45775,
    17322,
    28576,
    46547,
    61374,
    27275,
    46546,
    21902,
    29411,
    40746,
    25035,
    6948,
    60665,
    31214,
    21895,
    21164,
    29987,
    32693,
    42156,
    64593,
    62411,
    46557,
    29005,
    29004,
    46556,
    29036,
    29037,
};

const qstr_len_t mp_qstr_frozen_const_lengths[] = {
    1,
    8,
    2,
    6,
    5,
    6,
    12,
    8,
    5,
    4,
    7,
    5,
    5,
    4,
    4,
    17,
    4,
    9,
    11,
    11,
    8,
    2,
    5,
    19,
    8,
    11,
    10,
    12,
    10,
    23,
    6,
    18,
    6,
    12,
    12,
    7,
    14,
    12,
    5,
    6,
    6,
    6,
    5,
    7,
    7,
    7,
    11,
    4,
    6,
    6,
    11,
    6,
    8,
    6,
    8,
    8,
    9,
    5,
    8,
    6,
    12,
    12,
    5,
    5,
    14,
    5,
    4,
    9,
    6,
    9,
    5,
    2,
    7,
    5,
    5,
    5,
    5,
    16,
    4,
    5,
    8,
    8,
    4,
    11,
    9,
    5,
    6,
    10,
    8,
    5,
    7,
    6,
    6,
    8,
    8,
    8,
    1,
    6,
    6,
    7,
    6,
    5,
    5,
    4,
    6,
    7,
    19,
    15,
    16,
    16,
    15,
    17,
    4,
    2,
    6,
    9,
    1,
    7,
    9,
    3,
    19,
    5,
    2,
    8,
    4,
    6,
    6,
    3,
    3,
    6,
    7,
    1,
    22,
    12,
    6,
    20,
    21,
    11,
    2,
    2,
    8,
    7,
    6,
    5,
    7,
    7,
    7,
    9,
    4,
    5,
    18,
    11,
    5,
    12,
    2,
    1,
    25,
    6,
    6,
    23,
    4,
    5,
    4,
    9,
    7,
    7,
    3,
    2,
    3,
    4,
    7,
    5,
    2,
    5,
    3,
    2,
    5,
    5,
    3,
    8,
    9,
    1,
    2,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    4,
    5,
    2,
    5,
    4,
    9,
    4,
    10,
    5,
    6,
    7,
    1,
    6,
    14,
    21,
    14,
    11,
    6,
    8,
    18,
    1,
    6,
    4,
    4,
    1,
    3,
    3,
    8,
    7,
    6,
    4,
    2,
    4,
    5,
    4,
    4,
    9,
    6,
    4,
    6,
    4,
    9,
    8,
    7,
    4,
    7,
    6,
    8,
    4,
    4,
    2,
    6,
    1,
    11,
    14,
    4,
    21,
    11,
    9,
    9,
    8,
    8,
    8,
    6,
    8,
    8,
    10,
    9,
    9,
    2,
    15,
    7,
    7,
    4,
    5,
    8,
    4,
    4,
    7,
    7,
    3,
    6,
    4,
    7,
    8,
    6,
    3,
    6,
    10,
    11,
    4,
    6,
    11,
    6,
    3,
    9,
    6,
    7,
    17,
    4,
    4,
    5,
    3,
    3,
    9,
    11,
    18,
    8,
    1,
    5,
    6,
    8,
    15,
    11,
    21,
    11,
    10,
    4,
    5,
    6,
    4,
    7,
    4,
    5,
    6,
    3,
    8,
    3,
    12,
    6,
    13,
    2,
    1,
    3,
    4,
    2,
    7,
    6,
    24,
    24,
    4,
    9,
    7,
    5,
    6,
    5,
    3,
    9,
    5,
    2,
    1,
    6,
    7,
    1,
    4,
    11,
    8,
    11,
    13,
    6,
    7,
    9,
    4,
    5,
    3,
    11,
    4,
    11,
    1,
    2,
    2,
    1,
    2,
    2,
};

extern const qstr_pool_t mp_qstr_const_pool;
const qstr_pool_t mp_qstr_frozen_const_pool = {
    &mp_qstr_const_pool, // previous pool
    MP_QSTRnumber_of, // previous pool size
    true, // is_sorted
    10, // allocated entries
    369, // used entries
    (qstr_hash_t *)mp_qstr_frozen_const_hashes,
    (qstr_len_t *)mp_qstr_frozen_const_lengths,
    {
        "\x00",
        "\x25\x73\x00\x25\x73\x00\x25\x73",
        ".:",
        "Button",
        "CLEAR",
        "Canvas",
        "CanvasButton",
        "Checkbox",
        "Event",
        "Font",
        "IOQueue",
        "Image",
        "Label",
        "List",
        "Lock",
        "Lock not acquired",
        "Loop",
        "MAX_PAGES",
        "MAX_WIDGETS",
        "MAX_WINDOWS",
        "NeoPixel",
        "OK",
        "ORDER",
        "PROTOCOL_TLS_CLIENT",
        "Progress",
        "SOCK_STREAM",
        "SOL_SOCKET",
        "SO_REUSEADDR",
        "SSLContext",
        "SUPPORTS_GRADIENT_FILLS",
        "Server",
        "SingletonGenerator",
        "Stream",
        "StreamReader",
        "StreamWriter",
        "Textbox",
        "ThreadSafeFlag",
        "TimeoutError",
        "Timer",
        "Window",
        "Yes|No",
        "_Batch",
        "_KEYS",
        "_Pixels",
        "_Remove",
        "_Widget",
        "__version__",
        "_add",
        "_alive",
        "_attrs",
        "_bar_colors",
        "_batch",
        "_buttons",
        "_cells",
        "_checked",
        "_dequeue",
        "_dispatch",
        "_draw",
        "_enqueue",
        "_event",
        "_exc_context",
        "_exc_handler",
        "_file",
        "_gone",
        "_gradient_data",
        "_held",
        "_hit",
        "_io_queue",
        "_items",
        "_key_name",
        "_kids",
        "_n",
        "_new_id",
        "_next",
        "_objs",
        "_page",
        "_part",
        "_promote_to_task",
        "_req",
        "_req2",
        "_request",
        "_results",
        "_run",
        "_run_timers",
        "_selected",
        "_send",
        "_serve",
        "_stop_task",
        "_stopper",
        "_text",
        "_timers",
        "_title",
        "_value",
        "_visible",
        "_widgets",
        "_windows",
        "a",
        "accept",
        "aclose",
        "acquire",
        "across",
        "after",
        "align",
        "alt+",
        "ascent",
        "asyncio",
        "asyncio/__init__.py",
        "asyncio/core.py",
        "asyncio/event.py",
        "asyncio/funcs.py",
        "asyncio/lock.py",
        "asyncio/stream.py",
        "attr",
        "aw",
        "awrite",
        "awritestr",
        "b",
        "backlog",
        "backspace",
        "bar",
        "bar range or colour",
        "batch",
        "bg",
        "big_size",
        "bind",
        "border",
        "bottom",
        "bpp",
        "buf",
        "button",
        "buttons",
        "c",
        "call_exception_handler",
        "can't gather",
        "canvas",
        "canvas page: 0 to %d",
        "canvas pages: 1 to %d",
        "canvas size",
        "cb",
        "ch",
        "checkbox",
        "checked",
        "circle",
        "color",
        "confirm",
        "connect",
        "context",
        "copy_page",
        "core",
        "coro=",
        "coroutine expected",
        "create_task",
        "ctrl+",
        "current_task",
        "cv",
        "d",
        "default_exception_handler",
        "delete",
        "dither",
        "do_handshake_on_connect",
        "down",
        "drain",
        "draw",
        "draw_text",
        "draw_to",
        "drawing",
        "dst",
        "dt",
        "due",
        "edge",
        "enabled",
        "enter",
        "er",
        "error",
        "esc",
        "ev",
        "event",
        "every",
        "exc",
        "exc_type",
        "exception",
        "f",
        "f1",
        "f10",
        "f11",
        "f12",
        "f2",
        "f3",
        "f4",
        "f5",
        "f6",
        "f7",
        "f8",
        "f9",
        "fg",
        "fill",
        "flags",
        "fn",
        "focus",
        "font",
        "font_text",
        "free",
        "fullscreen",
        "funcs",
        "future",
        "future:",
        "g",
        "gather",
        "get_event_loop",
        "get_exception_handler",
        "get_extra_info",
        "getaddrinfo",
        "glyphs",
        "gradient",
        "gradient RGB range",
        "h",
        "height",
        "home",
        "host",
        "i",
        "idx",
        "img",
        "inputbox",
        "is_open",
        "is_set",
        "item",
        "kb",
        "kind",
        "label",
        "left",
        "line",
        "list size",
        "listen",
        "lock",
        "locked",
        "loop",
        "main_task",
        "maximize",
        "maximum",
        "menu",
        "message",
        "middle",
        "minimize",
        "mods",
        "move",
        "ms",
        "msgbox",
        "n",
        "neopixel.py",
        "new_event_loop",
        "nine",
        "no running event loop",
        "on_activate",
        "on_cancel",
        "on_change",
        "on_click",
        "on_close",
        "on_enter",
        "on_key",
        "on_mouse",
        "on_press",
        "on_release",
        "on_resize",
        "on_select",
        "op",
        "open_connection",
        "out_buf",
        "outline",
        "page",
        "pages",
        "peername",
        "pgdn",
        "pgup",
        "pheight",
        "picture",
        "pin",
        "poller",
        "port",
        "pressed",
        "progress",
        "prompt",
        "put",
        "pwidth",
        "queue_read",
        "queue_write",
        "quit",
        "radius",
        "readexactly",
        "repeat",
        "req",
        "resizable",
        "resize",
        "restore",
        "return_exceptions",
        "rgb1",
        "rgb2",
        "right",
        "rim",
        "run",
        "run_async",
        "run_forever",
        "run_until_complete",
        "runq_len",
        "s",
        "scale",
        "secret",
        "selected",
        "server_hostname",
        "server_side",
        "set_exception_handler",
        "setblocking",
        "setsockopt",
        "sgen",
        "sheet",
        "shift+",
        "show",
        "showing",
        "size",
        "small",
        "socket",
        "src",
        "src_down",
        "ssl",
        "start_server",
        "stream",
        "stream_awrite",
        "sz",
        "t",
        "tab",
        "task",
        "tb",
        "tdui.py",
        "tdui: ",
        "tdui: at most %d widgets",
        "tdui: at most %d windows",
        "text",
        "text_size",
        "textbox",
        "ticks",
        "timing",
        "title",
        "top",
        "traceback",
        "track",
        "up",
        "v",
        "valign",
        "visible",
        "w",
        "wait",
        "wait_closed",
        "wait_for",
        "wait_for_ms",
        "wait_io_event",
        "waiter",
        "waiting",
        "waitq_len",
        "warm",
        "width",
        "win",
        "window size",
        "wrap",
        "wrap_socket",
        "x",
        "x0",
        "x1",
        "y",
        "y0",
        "y1",
    },
};

////////////////////////////////////////////////////////////////////////////////
// frozen module asyncio___init__
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\asyncio\__init__.mpy
// - frozen file name: asyncio/__init__.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file asyncio/__init__.py, scope asyncio___init____lt_module_gt_
static const byte fun_data_asyncio___init____lt_module_gt_[75] = {
    0x10,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x02, // LOAD_CONST_STRING '*'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x03, // IMPORT_NAME 'core'
    0x69, // IMPORT_STAR
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x16,0x0e, // STORE_NAME '__version__'
    0x2c,0x0a, // BUILD_MAP 10
    0x10,0x04, // LOAD_CONST_STRING 'funcs'
    0x10,0x05, // LOAD_CONST_STRING 'wait_for'
    0x62, // STORE_MAP
    0x10,0x04, // LOAD_CONST_STRING 'funcs'
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x62, // STORE_MAP
    0x10,0x04, // LOAD_CONST_STRING 'funcs'
    0x10,0x06, // LOAD_CONST_STRING 'gather'
    0x62, // STORE_MAP
    0x10,0x07, // LOAD_CONST_STRING 'event'
    0x10,0x08, // LOAD_CONST_STRING 'Event'
    0x62, // STORE_MAP
    0x10,0x07, // LOAD_CONST_STRING 'event'
    0x23,0x02, // LOAD_CONST_OBJ 2
    0x62, // STORE_MAP
    0x10,0x09, // LOAD_CONST_STRING 'lock'
    0x10,0x0a, // LOAD_CONST_STRING 'Lock'
    0x62, // STORE_MAP
    0x10,0x0b, // LOAD_CONST_STRING 'stream'
    0x23,0x03, // LOAD_CONST_OBJ 3
    0x62, // STORE_MAP
    0x10,0x0b, // LOAD_CONST_STRING 'stream'
    0x23,0x04, // LOAD_CONST_OBJ 4
    0x62, // STORE_MAP
    0x10,0x0b, // LOAD_CONST_STRING 'stream'
    0x23,0x05, // LOAD_CONST_OBJ 5
    0x62, // STORE_MAP
    0x10,0x0b, // LOAD_CONST_STRING 'stream'
    0x23,0x06, // LOAD_CONST_OBJ 6
    0x62, // STORE_MAP
    0x16,0x0f, // STORE_NAME '_attrs'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x0c, // STORE_NAME '__getattr__'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio___init____lt_module_gt_
// frozen bytecode for file asyncio/__init__.py, scope asyncio___init_____getattr__
static const byte fun_data_asyncio___init_____getattr__[51] = {
    0x49,0x04, // prelude
    0x0c,0x10, // names: __getattr__, attr
     // code info
    0x12,0x0f, // LOAD_GLOBAL '_attrs'
    0x14,0x0d, // LOAD_METHOD 'get'
    0xb0, // LOAD_FAST 0
    0x51, // LOAD_CONST_NONE
    0x36,0x02, // CALL_METHOD 2
    0xc1, // STORE_FAST 1
    0xb1, // LOAD_FAST 1
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0x12,0x11, // LOAD_GLOBAL 'AttributeError'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x12, // LOAD_GLOBAL 'getattr'
    0x12,0x13, // LOAD_GLOBAL '__import__'
    0xb1, // LOAD_FAST 1
    0x12,0x14, // LOAD_GLOBAL 'globals'
    0x34,0x00, // CALL_FUNCTION 0
    0x51, // LOAD_CONST_NONE
    0x52, // LOAD_CONST_TRUE
    0x81, // LOAD_CONST_SMALL_INT 1
    0x34,0x05, // CALL_FUNCTION 5
    0xb0, // LOAD_FAST 0
    0x34,0x02, // CALL_FUNCTION 2
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x12,0x14, // LOAD_GLOBAL 'globals'
    0x34,0x00, // CALL_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x56, // STORE_SUBSCR
    0xb2, // LOAD_FAST 2
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio___init_____getattr__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio___init_____getattr__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 51,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 12,
        .line_info = fun_data_asyncio___init_____getattr__ + 4,
        .line_info_top = fun_data_asyncio___init_____getattr__ + 4,
        .opcodes = fun_data_asyncio___init_____getattr__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio___init_____getattr__ fun_data_asyncio___init_____getattr__[0]
#endif

static const mp_raw_code_t *const children_asyncio___init____lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio___init_____getattr__,
};

static const mp_raw_code_truncated_t proto_fun_asyncio___init____lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio___init____lt_module_gt_,
    .children = (void *)&children_asyncio___init____lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 75,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_asyncio___init____lt_module_gt_ + 3,
        .line_info_top = fun_data_asyncio___init____lt_module_gt_ + 3,
        .opcodes = fun_data_asyncio___init____lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_asyncio___init__[21] = {
    MP_QSTR_asyncio_slash___init___dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR__star_,
    MP_QSTR_core,
    MP_QSTR_funcs,
    MP_QSTR_wait_for,
    MP_QSTR_gather,
    MP_QSTR_event,
    MP_QSTR_Event,
    MP_QSTR_lock,
    MP_QSTR_Lock,
    MP_QSTR_stream,
    MP_QSTR___getattr__,
    MP_QSTR_get,
    MP_QSTR___version__,
    MP_QSTR__attrs,
    MP_QSTR_attr,
    MP_QSTR_AttributeError,
    MP_QSTR_getattr,
    MP_QSTR___import__,
    MP_QSTR_globals,
};

// constants
static const mp_rom_obj_tuple_t const_obj_asyncio___init___0 = {{&mp_type_tuple}, 3, {
    MP_ROM_INT(3),
    MP_ROM_INT(0),
    MP_ROM_INT(0),
}};

// constant table
static const mp_rom_obj_t const_obj_table_data_asyncio___init__[7] = {
    MP_ROM_PTR(&const_obj_asyncio___init___0),
    MP_ROM_QSTR(MP_QSTR_wait_for_ms),
    MP_ROM_QSTR(MP_QSTR_ThreadSafeFlag),
    MP_ROM_QSTR(MP_QSTR_open_connection),
    MP_ROM_QSTR(MP_QSTR_start_server),
    MP_ROM_QSTR(MP_QSTR_StreamReader),
    MP_ROM_QSTR(MP_QSTR_StreamWriter),
};

static const mp_frozen_module_t frozen_module_asyncio___init__ = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_asyncio___init__,
        .obj_table = (mp_obj_t *)&const_obj_table_data_asyncio___init__,
    },
    .proto_fun = &proto_fun_asyncio___init____lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module asyncio_core
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\asyncio\core.mpy
// - frozen file name: asyncio/core.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file asyncio/core.py, scope asyncio_core__lt_module_gt_
static const byte fun_data_asyncio_core__lt_module_gt_[218] = {
    0x2c,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x02, // LOAD_CONST_STRING 'ticks_ms'
    0x10,0x03, // LOAD_CONST_STRING 'ticks_diff'
    0x10,0x04, // LOAD_CONST_STRING 'ticks_add'
    0x2a,0x03, // BUILD_TUPLE 3
    0x1b,0x05, // IMPORT_NAME 'time'
    0x1c,0x02, // IMPORT_FROM 'ticks_ms'
    0x16,0x49, // STORE_NAME 'ticks'
    0x1c,0x03, // IMPORT_FROM 'ticks_diff'
    0x16,0x03, // STORE_NAME 'ticks_diff'
    0x1c,0x04, // IMPORT_FROM 'ticks_add'
    0x16,0x04, // STORE_NAME 'ticks_add'
    0x59, // POP_TOP
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x06, // IMPORT_NAME 'sys'
    0x16,0x06, // STORE_NAME 'sys'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x07, // IMPORT_NAME 'select'
    0x16,0x07, // STORE_NAME 'select'
    0x48,0x14, // SETUP_EXCEPT 20
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x08, // LOAD_CONST_STRING 'TaskQueue'
    0x10,0x09, // LOAD_CONST_STRING 'Task'
    0x2a,0x02, // BUILD_TUPLE 2
    0x1b,0x0a, // IMPORT_NAME '_asyncio'
    0x1c,0x08, // IMPORT_FROM 'TaskQueue'
    0x16,0x08, // STORE_NAME 'TaskQueue'
    0x1c,0x09, // IMPORT_FROM 'Task'
    0x16,0x09, // STORE_NAME 'Task'
    0x59, // POP_TOP
    0x4a,0x16, // POP_EXCEPT_JUMP 22
    0x59, // POP_TOP
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x08, // LOAD_CONST_STRING 'TaskQueue'
    0x10,0x09, // LOAD_CONST_STRING 'Task'
    0x2a,0x02, // BUILD_TUPLE 2
    0x1b,0x0b, // IMPORT_NAME 'task'
    0x1c,0x08, // IMPORT_FROM 'TaskQueue'
    0x16,0x08, // STORE_NAME 'TaskQueue'
    0x1c,0x09, // IMPORT_FROM 'Task'
    0x16,0x09, // STORE_NAME 'Task'
    0x59, // POP_TOP
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0x54, // LOAD_BUILD_CLASS
    0x32,0x00, // MAKE_FUNCTION 0
    0x10,0x0c, // LOAD_CONST_STRING 'CancelledError'
    0x11,0x4a, // LOAD_NAME 'BaseException'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x0c, // STORE_NAME 'CancelledError'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x01, // MAKE_FUNCTION 1
    0x10,0x0d, // LOAD_CONST_STRING 'TimeoutError'
    0x11,0x4b, // LOAD_NAME 'Exception'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x0d, // STORE_NAME 'TimeoutError'
    0x2c,0x03, // BUILD_MAP 3
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x10,0x0e, // LOAD_CONST_STRING 'message'
    0x62, // STORE_MAP
    0x51, // LOAD_CONST_NONE
    0x10,0x0f, // LOAD_CONST_STRING 'exception'
    0x62, // STORE_MAP
    0x51, // LOAD_CONST_NONE
    0x10,0x10, // LOAD_CONST_STRING 'future'
    0x62, // STORE_MAP
    0x16,0x4c, // STORE_NAME '_exc_context'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x02, // MAKE_FUNCTION 2
    0x10,0x11, // LOAD_CONST_STRING 'SingletonGenerator'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x11, // STORE_NAME 'SingletonGenerator'
    0x11,0x11, // LOAD_NAME 'SingletonGenerator'
    0x34,0x00, // CALL_FUNCTION 0
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x03, // MAKE_FUNCTION_DEFARGS 3
    0x16,0x14, // STORE_NAME 'sleep_ms'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x16, // STORE_NAME 'sleep'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x05, // MAKE_FUNCTION 5
    0x10,0x12, // LOAD_CONST_STRING 'IOQueue'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x12, // STORE_NAME 'IOQueue'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x17, // STORE_NAME '_promote_to_task'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x18, // STORE_NAME 'create_task'
    0x51, // LOAD_CONST_NONE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x08, // MAKE_FUNCTION_DEFARGS 8
    0x16,0x1b, // STORE_NAME 'run_until_complete'
    0x32,0x09, // MAKE_FUNCTION 9
    0x16,0x26, // STORE_NAME 'run'
    0x32,0x0a, // MAKE_FUNCTION 10
    0x16,0x27, // STORE_NAME '_stopper'
    0x51, // LOAD_CONST_NONE
    0x17,0x4d, // STORE_GLOBAL 'cur_task'
    0x51, // LOAD_CONST_NONE
    0x17,0x4e, // STORE_GLOBAL '_stop_task'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x0b, // MAKE_FUNCTION 11
    0x10,0x13, // LOAD_CONST_STRING 'Loop'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x13, // STORE_NAME 'Loop'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x0c, // MAKE_FUNCTION_DEFARGS 12
    0x16,0x28, // STORE_NAME 'get_event_loop'
    0x32,0x0d, // MAKE_FUNCTION 13
    0x16,0x29, // STORE_NAME 'current_task'
    0x32,0x0e, // MAKE_FUNCTION 14
    0x16,0x2a, // STORE_NAME 'new_event_loop'
    0x11,0x2a, // LOAD_NAME 'new_event_loop'
    0x34,0x00, // CALL_FUNCTION 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_CancelledError
static const byte fun_data_asyncio_core_CancelledError[13] = {
    0x00,0x02, // prelude
    0x0c, // names: CancelledError
     // code info
    0x11,0x4f, // LOAD_NAME '__name__'
    0x16,0x50, // STORE_NAME '__module__'
    0x10,0x0c, // LOAD_CONST_STRING 'CancelledError'
    0x16,0x51, // STORE_NAME '__qualname__'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_CancelledError = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_CancelledError,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 13,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 12,
        .line_info = fun_data_asyncio_core_CancelledError + 3,
        .line_info_top = fun_data_asyncio_core_CancelledError + 3,
        .opcodes = fun_data_asyncio_core_CancelledError + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_CancelledError fun_data_asyncio_core_CancelledError[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_TimeoutError
static const byte fun_data_asyncio_core_TimeoutError[13] = {
    0x00,0x02, // prelude
    0x0d, // names: TimeoutError
     // code info
    0x11,0x4f, // LOAD_NAME '__name__'
    0x16,0x50, // STORE_NAME '__module__'
    0x10,0x0d, // LOAD_CONST_STRING 'TimeoutError'
    0x16,0x51, // STORE_NAME '__qualname__'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_TimeoutError = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_TimeoutError,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 13,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 13,
        .line_info = fun_data_asyncio_core_TimeoutError + 3,
        .line_info_top = fun_data_asyncio_core_TimeoutError + 3,
        .opcodes = fun_data_asyncio_core_TimeoutError + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_TimeoutError fun_data_asyncio_core_TimeoutError[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_SingletonGenerator
static const byte fun_data_asyncio_core_SingletonGenerator[25] = {
    0x00,0x02, // prelude
    0x11, // names: SingletonGenerator
     // code info
    0x11,0x4f, // LOAD_NAME '__name__'
    0x16,0x50, // STORE_NAME '__module__'
    0x10,0x11, // LOAD_CONST_STRING 'SingletonGenerator'
    0x16,0x51, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x2b, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x2d, // STORE_NAME '__iter__'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x2e, // STORE_NAME '__next__'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_core_SingletonGenerator
// frozen bytecode for file asyncio/core.py, scope asyncio_core_SingletonGenerator___init__
static const byte fun_data_asyncio_core_SingletonGenerator___init__[17] = {
    0x11,0x04, // prelude
    0x2b,0x63, // names: __init__, self
     // code info
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x15, // STORE_ATTR 'state'
    0x12,0x5d, // LOAD_GLOBAL 'StopIteration'
    0x34,0x00, // CALL_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x18,0x2c, // STORE_ATTR 'exc'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_SingletonGenerator___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_SingletonGenerator___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 17,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 43,
        .line_info = fun_data_asyncio_core_SingletonGenerator___init__ + 4,
        .line_info_top = fun_data_asyncio_core_SingletonGenerator___init__ + 4,
        .opcodes = fun_data_asyncio_core_SingletonGenerator___init__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_SingletonGenerator___init__ fun_data_asyncio_core_SingletonGenerator___init__[0]
#endif

// child of asyncio_core_SingletonGenerator
// frozen bytecode for file asyncio/core.py, scope asyncio_core_SingletonGenerator___iter__
static const byte fun_data_asyncio_core_SingletonGenerator___iter__[6] = {
    0x09,0x04, // prelude
    0x2d,0x63, // names: __iter__, self
     // code info
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_SingletonGenerator___iter__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_SingletonGenerator___iter__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 6,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 45,
        .line_info = fun_data_asyncio_core_SingletonGenerator___iter__ + 4,
        .line_info_top = fun_data_asyncio_core_SingletonGenerator___iter__ + 4,
        .opcodes = fun_data_asyncio_core_SingletonGenerator___iter__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_SingletonGenerator___iter__ fun_data_asyncio_core_SingletonGenerator___iter__[0]
#endif

// child of asyncio_core_SingletonGenerator
// frozen bytecode for file asyncio/core.py, scope asyncio_core_SingletonGenerator___next__
static const byte fun_data_asyncio_core_SingletonGenerator___next__[42] = {
    0x21,0x04, // prelude
    0x2e,0x63, // names: __next__, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x15, // LOAD_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x52, // POP_JUMP_IF_FALSE 18
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0x12,0x4d, // LOAD_GLOBAL 'cur_task'
    0xb0, // LOAD_FAST 0
    0x13,0x15, // LOAD_ATTR 'state'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x15, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x13,0x2c, // LOAD_ATTR 'exc'
    0x18,0x2f, // STORE_ATTR '__traceback__'
    0xb0, // LOAD_FAST 0
    0x13,0x2c, // LOAD_ATTR 'exc'
    0x65, // RAISE_OBJ
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_SingletonGenerator___next__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_SingletonGenerator___next__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 42,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 46,
        .line_info = fun_data_asyncio_core_SingletonGenerator___next__ + 4,
        .line_info_top = fun_data_asyncio_core_SingletonGenerator___next__ + 4,
        .opcodes = fun_data_asyncio_core_SingletonGenerator___next__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_SingletonGenerator___next__ fun_data_asyncio_core_SingletonGenerator___next__[0]
#endif

static const mp_raw_code_t *const children_asyncio_core_SingletonGenerator[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_core_SingletonGenerator___init__,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_SingletonGenerator___iter__,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_SingletonGenerator___next__,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_core_SingletonGenerator = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_SingletonGenerator,
    .children = (void *)&children_asyncio_core_SingletonGenerator,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 25,
    .n_children = 3,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 17,
        .line_info = fun_data_asyncio_core_SingletonGenerator + 3,
        .line_info_top = fun_data_asyncio_core_SingletonGenerator + 3,
        .opcodes = fun_data_asyncio_core_SingletonGenerator + 3,
    },
    #endif
    #endif
};

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_sleep_ms
static const byte fun_data_asyncio_core_sleep_ms[25] = {
    0xb2,0x01,0x06, // prelude
    0x14,0x52,0x53, // names: sleep_ms, t, sgen
     // code info
    0x12,0x04, // LOAD_GLOBAL 'ticks_add'
    0x12,0x49, // LOAD_GLOBAL 'ticks'
    0x34,0x00, // CALL_FUNCTION 0
    0x12,0x54, // LOAD_GLOBAL 'max'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x34,0x02, // CALL_FUNCTION 2
    0x34,0x02, // CALL_FUNCTION 2
    0xb1, // LOAD_FAST 1
    0x18,0x15, // STORE_ATTR 'state'
    0xb1, // LOAD_FAST 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_sleep_ms = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_sleep_ms,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 25,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 20,
        .line_info = fun_data_asyncio_core_sleep_ms + 6,
        .line_info_top = fun_data_asyncio_core_sleep_ms + 6,
        .opcodes = fun_data_asyncio_core_sleep_ms + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_sleep_ms fun_data_asyncio_core_sleep_ms[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_sleep
static const byte fun_data_asyncio_core_sleep[18] = {
    0x21,0x04, // prelude
    0x16,0x52, // names: sleep, t
     // code info
    0x12,0x14, // LOAD_GLOBAL 'sleep_ms'
    0x12,0x55, // LOAD_GLOBAL 'int'
    0xb0, // LOAD_FAST 0
    0x22,0x87,0x68, // LOAD_CONST_SMALL_INT 1000
    0xf4, // BINARY_OP 29 __mul__
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_sleep = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_sleep,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 18,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 22,
        .line_info = fun_data_asyncio_core_sleep + 4,
        .line_info_top = fun_data_asyncio_core_sleep + 4,
        .opcodes = fun_data_asyncio_core_sleep + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_sleep fun_data_asyncio_core_sleep[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue
static const byte fun_data_asyncio_core_IOQueue[41] = {
    0x00,0x02, // prelude
    0x12, // names: IOQueue
     // code info
    0x11,0x4f, // LOAD_NAME '__name__'
    0x16,0x50, // STORE_NAME '__module__'
    0x10,0x12, // LOAD_CONST_STRING 'IOQueue'
    0x16,0x51, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x2b, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x32, // STORE_NAME '_enqueue'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x37, // STORE_NAME '_dequeue'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x39, // STORE_NAME 'queue_read'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x3a, // STORE_NAME 'queue_write'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x3b, // STORE_NAME 'remove'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x1f, // STORE_NAME 'wait_io_event'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue___init__
static const byte fun_data_asyncio_core_IOQueue___init__[20] = {
    0x11,0x04, // prelude
    0x2b,0x63, // names: __init__, self
     // code info
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x14,0x30, // LOAD_METHOD 'poll'
    0x36,0x00, // CALL_METHOD 0
    0xb0, // LOAD_FAST 0
    0x18,0x31, // STORE_ATTR 'poller'
    0x2c,0x00, // BUILD_MAP 0
    0xb0, // LOAD_FAST 0
    0x18,0x1e, // STORE_ATTR 'map'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 20,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 43,
        .line_info = fun_data_asyncio_core_IOQueue___init__ + 4,
        .line_info_top = fun_data_asyncio_core_IOQueue___init__ + 4,
        .opcodes = fun_data_asyncio_core_IOQueue___init__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue___init__ fun_data_asyncio_core_IOQueue___init__[0]
#endif

// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue__enqueue
static const byte fun_data_asyncio_core_IOQueue__enqueue[105] = {
    0x4b,0x08, // prelude
    0x32,0x63,0x64,0x65, // names: _enqueue, self, s, idx
     // code info
    0x12,0x66, // LOAD_GLOBAL 'id'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0xdd, // BINARY_OP 6 <in>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x6f, // POP_JUMP_IF_FALSE 47
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xb1, // LOAD_FAST 1
    0x2b,0x03, // BUILD_LIST 3
    0xc3, // STORE_FAST 3
    0x12,0x4d, // LOAD_GLOBAL 'cur_task'
    0xb3, // LOAD_FAST 3
    0xb2, // LOAD_FAST 2
    0x56, // STORE_SUBSCR
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0x12,0x66, // LOAD_GLOBAL 'id'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x56, // STORE_SUBSCR
    0xb0, // LOAD_FAST 0
    0x13,0x31, // LOAD_ATTR 'poller'
    0x14,0x33, // LOAD_METHOD 'register'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x34, // LOAD_ATTR 'POLLIN'
    0x42,0x44, // JUMP 4
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x35, // LOAD_ATTR 'POLLOUT'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x42,0x61, // JUMP 33
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0x12,0x66, // LOAD_GLOBAL 'id'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x55, // LOAD_SUBSCR
    0xc4, // STORE_FAST 4
    0x12,0x4d, // LOAD_GLOBAL 'cur_task'
    0xb4, // LOAD_FAST 4
    0xb2, // LOAD_FAST 2
    0x56, // STORE_SUBSCR
    0xb0, // LOAD_FAST 0
    0x13,0x31, // LOAD_ATTR 'poller'
    0x14,0x36, // LOAD_METHOD 'modify'
    0xb1, // LOAD_FAST 1
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x34, // LOAD_ATTR 'POLLIN'
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x35, // LOAD_ATTR 'POLLOUT'
    0xed, // BINARY_OP 22 __or__
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x12,0x4d, // LOAD_GLOBAL 'cur_task'
    0x18,0x21, // STORE_ATTR 'data'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue__enqueue = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue__enqueue,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 105,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 50,
        .line_info = fun_data_asyncio_core_IOQueue__enqueue + 6,
        .line_info_top = fun_data_asyncio_core_IOQueue__enqueue + 6,
        .opcodes = fun_data_asyncio_core_IOQueue__enqueue + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue__enqueue fun_data_asyncio_core_IOQueue__enqueue[0]
#endif

// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue__dequeue
static const byte fun_data_asyncio_core_IOQueue__dequeue[27] = {
    0x22,0x06, // prelude
    0x37,0x63,0x64, // names: _dequeue, self, s
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0x12,0x66, // LOAD_GLOBAL 'id'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x53, // LOAD_NULL
    0x5b, // ROT_THREE
    0x56, // STORE_SUBSCR
    0xb0, // LOAD_FAST 0
    0x13,0x31, // LOAD_ATTR 'poller'
    0x14,0x38, // LOAD_METHOD 'unregister'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue__dequeue = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue__dequeue,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 27,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 55,
        .line_info = fun_data_asyncio_core_IOQueue__dequeue + 5,
        .line_info_top = fun_data_asyncio_core_IOQueue__dequeue + 5,
        .opcodes = fun_data_asyncio_core_IOQueue__dequeue + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue__dequeue fun_data_asyncio_core_IOQueue__dequeue[0]
#endif

// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue_queue_read
static const byte fun_data_asyncio_core_IOQueue_queue_read[15] = {
    0x2a,0x06, // prelude
    0x39,0x63,0x64, // names: queue_read, self, s
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x32, // LOAD_METHOD '_enqueue'
    0xb1, // LOAD_FAST 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue_queue_read = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue_queue_read,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 15,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 57,
        .line_info = fun_data_asyncio_core_IOQueue_queue_read + 5,
        .line_info_top = fun_data_asyncio_core_IOQueue_queue_read + 5,
        .opcodes = fun_data_asyncio_core_IOQueue_queue_read + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue_queue_read fun_data_asyncio_core_IOQueue_queue_read[0]
#endif

// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue_queue_write
static const byte fun_data_asyncio_core_IOQueue_queue_write[15] = {
    0x2a,0x06, // prelude
    0x3a,0x63,0x64, // names: queue_write, self, s
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x32, // LOAD_METHOD '_enqueue'
    0xb1, // LOAD_FAST 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue_queue_write = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue_queue_write,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 15,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 58,
        .line_info = fun_data_asyncio_core_IOQueue_queue_write + 5,
        .line_info_top = fun_data_asyncio_core_IOQueue_queue_write + 5,
        .opcodes = fun_data_asyncio_core_IOQueue_queue_write + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue_queue_write fun_data_asyncio_core_IOQueue_queue_write[0]
#endif

// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue_remove
static const byte fun_data_asyncio_core_IOQueue_remove[65] = {
    0x6a,0x06, // prelude
    0x3b,0x63,0x0b, // names: remove, self, task
     // code info
    0x51, // LOAD_CONST_NONE
    0xc2, // STORE_FAST 2
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0x5f, // GET_ITER_STACK
    0x4b,0x1f, // FOR_ITER 31
    0xc3, // STORE_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0xb3, // LOAD_FAST 3
    0x55, // LOAD_SUBSCR
    0x30,0x03, // UNPACK_SEQUENCE 3
    0xc4, // STORE_FAST 4
    0xc5, // STORE_FAST 5
    0xc6, // STORE_FAST 6
    0xb4, // LOAD_FAST 4
    0xb1, // LOAD_FAST 1
    0xde, // BINARY_OP 7 <is>
    0x43,0x45, // POP_JUMP_IF_TRUE 5
    0xb5, // LOAD_FAST 5
    0xb1, // LOAD_FAST 1
    0xde, // BINARY_OP 7 <is>
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb6, // LOAD_FAST 6
    0xc2, // STORE_FAST 2
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x42,0x42, // JUMP 2
    0x42,0x1f, // JUMP -33
    0xb2, // LOAD_FAST 2
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb0, // LOAD_FAST 0
    0x14,0x37, // LOAD_METHOD '_dequeue'
    0xb6, // LOAD_FAST 6
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x42, // JUMP 2
    0x42,0x42, // JUMP 2
    0x42,0x06, // JUMP -58
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue_remove = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue_remove,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 65,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 14,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 59,
        .line_info = fun_data_asyncio_core_IOQueue_remove + 5,
        .line_info_top = fun_data_asyncio_core_IOQueue_remove + 5,
        .opcodes = fun_data_asyncio_core_IOQueue_remove + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue_remove fun_data_asyncio_core_IOQueue_remove[0]
#endif

// child of asyncio_core_IOQueue
// frozen bytecode for file asyncio/core.py, scope asyncio_core_IOQueue_wait_io_event
static const byte fun_data_asyncio_core_IOQueue_wait_io_event[156] = {
    0x62,0x06, // prelude
    0x1f,0x63,0x67, // names: wait_io_event, self, dt
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x31, // LOAD_ATTR 'poller'
    0x14,0x3c, // LOAD_METHOD 'ipoll'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x5f, // GET_ITER_STACK
    0x4b,0x89,0x01, // FOR_ITER 137
    0x30,0x02, // UNPACK_SEQUENCE 2
    0xc2, // STORE_FAST 2
    0xc3, // STORE_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x1e, // LOAD_ATTR 'map'
    0x12,0x66, // LOAD_GLOBAL 'id'
    0xb2, // LOAD_FAST 2
    0x34,0x01, // CALL_FUNCTION 1
    0x55, // LOAD_SUBSCR
    0xc4, // STORE_FAST 4
    0xb3, // LOAD_FAST 3
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x35, // LOAD_ATTR 'POLLOUT'
    0xd2, // UNARY_OP 2 __invert__
    0xef, // BINARY_OP 24 __and__
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x56, // STORE_SUBSCR
    0xb3, // LOAD_FAST 3
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x34, // LOAD_ATTR 'POLLIN'
    0xd2, // UNARY_OP 2 __invert__
    0xef, // BINARY_OP 24 __and__
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0xb4, // LOAD_FAST 4
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0xb4, // LOAD_FAST 4
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0xb4, // LOAD_FAST 4
    0x81, // LOAD_CONST_SMALL_INT 1
    0x56, // STORE_SUBSCR
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x50, // POP_JUMP_IF_FALSE 16
    0xb4, // LOAD_FAST 4
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb0, // LOAD_FAST 0
    0x14,0x37, // LOAD_METHOD '_dequeue'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x63, // JUMP 35
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4f, // POP_JUMP_IF_FALSE 15
    0xb0, // LOAD_FAST 0
    0x13,0x31, // LOAD_ATTR 'poller'
    0x14,0x36, // LOAD_METHOD 'modify'
    0xb2, // LOAD_FAST 2
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x35, // LOAD_ATTR 'POLLOUT'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x42,0x4d, // JUMP 13
    0xb0, // LOAD_FAST 0
    0x13,0x31, // LOAD_ATTR 'poller'
    0x14,0x36, // LOAD_METHOD 'modify'
    0xb2, // LOAD_FAST 2
    0x12,0x07, // LOAD_GLOBAL 'select'
    0x13,0x34, // LOAD_ATTR 'POLLIN'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x42,0xf4,0x7e, // JUMP -140
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue_wait_io_event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue_wait_io_event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 156,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 31,
        .line_info = fun_data_asyncio_core_IOQueue_wait_io_event + 5,
        .line_info_top = fun_data_asyncio_core_IOQueue_wait_io_event + 5,
        .opcodes = fun_data_asyncio_core_IOQueue_wait_io_event + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_IOQueue_wait_io_event fun_data_asyncio_core_IOQueue_wait_io_event[0]
#endif

static const mp_raw_code_t *const children_asyncio_core_IOQueue[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue___init__,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue__enqueue,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue__dequeue,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue_queue_read,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue_queue_write,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue_remove,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue_wait_io_event,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_core_IOQueue = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_IOQueue,
    .children = (void *)&children_asyncio_core_IOQueue,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 41,
    .n_children = 7,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 18,
        .line_info = fun_data_asyncio_core_IOQueue + 3,
        .line_info_top = fun_data_asyncio_core_IOQueue + 3,
        .opcodes = fun_data_asyncio_core_IOQueue + 3,
    },
    #endif
    #endif
};

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core__promote_to_task
static const byte fun_data_asyncio_core__promote_to_task[21] = {
    0x19,0x04, // prelude
    0x17,0x56, // names: _promote_to_task, aw
     // code info
    0x12,0x57, // LOAD_GLOBAL 'isinstance'
    0xb0, // LOAD_FAST 0
    0x12,0x09, // LOAD_GLOBAL 'Task'
    0x34,0x02, // CALL_FUNCTION 2
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
    0x12,0x18, // LOAD_GLOBAL 'create_task'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core__promote_to_task = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core__promote_to_task,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 21,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 23,
        .line_info = fun_data_asyncio_core__promote_to_task + 4,
        .line_info_top = fun_data_asyncio_core__promote_to_task + 4,
        .opcodes = fun_data_asyncio_core__promote_to_task + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core__promote_to_task fun_data_asyncio_core__promote_to_task[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_create_task
static const byte fun_data_asyncio_core_create_task[40] = {
    0x21,0x04, // prelude
    0x18,0x22, // names: create_task, coro
     // code info
    0x12,0x58, // LOAD_GLOBAL 'hasattr'
    0xb0, // LOAD_FAST 0
    0x10,0x19, // LOAD_CONST_STRING 'send'
    0x34,0x02, // CALL_FUNCTION 2
    0x43,0x47, // POP_JUMP_IF_TRUE 7
    0x12,0x59, // LOAD_GLOBAL 'TypeError'
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x09, // LOAD_GLOBAL 'Task'
    0xb0, // LOAD_FAST 0
    0x12,0x5a, // LOAD_GLOBAL 'globals'
    0x34,0x00, // CALL_FUNCTION 0
    0x34,0x02, // CALL_FUNCTION 2
    0xc1, // STORE_FAST 1
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb1, // LOAD_FAST 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_create_task = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_create_task,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 40,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 24,
        .line_info = fun_data_asyncio_core_create_task + 4,
        .line_info_top = fun_data_asyncio_core_create_task + 4,
        .opcodes = fun_data_asyncio_core_create_task + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_create_task fun_data_asyncio_core_create_task[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_run_until_complete
static const byte fun_data_asyncio_core_run_until_complete[346] = {
    0xf1,0x03,0x04, // prelude
    0x1b,0x5c, // names: run_until_complete, main_task
     // code info
    0x12,0x0c, // LOAD_GLOBAL 'CancelledError'
    0x12,0x4b, // LOAD_GLOBAL 'Exception'
    0x2a,0x02, // BUILD_TUPLE 2
    0xc1, // STORE_FAST 1
    0x12,0x0c, // LOAD_GLOBAL 'CancelledError'
    0x12,0x5d, // LOAD_GLOBAL 'StopIteration'
    0x2a,0x02, // BUILD_TUPLE 2
    0xc2, // STORE_FAST 2
    0x81, // LOAD_CONST_SMALL_INT 1
    0xc3, // STORE_FAST 3
    0x42,0x7e, // JUMP 62
    0x7f, // LOAD_CONST_SMALL_INT -1
    0xc3, // STORE_FAST 3
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1c, // LOAD_METHOD 'peek'
    0x36,0x00, // CALL_METHOD 0
    0xc4, // STORE_FAST 4
    0xb4, // LOAD_FAST 4
    0x44,0x53, // POP_JUMP_IF_FALSE 19
    0x12,0x54, // LOAD_GLOBAL 'max'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x03, // LOAD_GLOBAL 'ticks_diff'
    0xb4, // LOAD_FAST 4
    0x13,0x1d, // LOAD_ATTR 'ph_key'
    0x12,0x49, // LOAD_GLOBAL 'ticks'
    0x34,0x00, // CALL_FUNCTION 0
    0x34,0x02, // CALL_FUNCTION 2
    0x34,0x02, // CALL_FUNCTION 2
    0xc3, // STORE_FAST 3
    0x42,0x57, // JUMP 23
    0x12,0x5e, // LOAD_GLOBAL '_io_queue'
    0x13,0x1e, // LOAD_ATTR 'map'
    0x43,0x51, // POP_JUMP_IF_TRUE 17
    0x51, // LOAD_CONST_NONE
    0x17,0x4d, // STORE_GLOBAL 'cur_task'
    0xb0, // LOAD_FAST 0
    0x44,0x45, // POP_JUMP_IF_FALSE 5
    0xb0, // LOAD_FAST 0
    0x13,0x15, // LOAD_ATTR 'state'
    0x43,0x42, // POP_JUMP_IF_TRUE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0x83, // LOAD_CONST_SMALL_INT 3
    0xc3, // STORE_FAST 3
    0x42,0x40, // JUMP 0
    0x12,0x5e, // LOAD_GLOBAL '_io_queue'
    0x14,0x1f, // LOAD_METHOD 'wait_io_event'
    0xb3, // LOAD_FAST 3
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb3, // LOAD_FAST 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd8, // BINARY_OP 1 __gt__
    0x43,0xbc,0x7f, // POP_JUMP_IF_TRUE -68
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x20, // LOAD_METHOD 'pop'
    0x36,0x00, // CALL_METHOD 0
    0xc4, // STORE_FAST 4
    0xb4, // LOAD_FAST 4
    0x17,0x4d, // STORE_GLOBAL 'cur_task'
    0x48,0x22, // SETUP_EXCEPT 34
    0xb4, // LOAD_FAST 4
    0x13,0x21, // LOAD_ATTR 'data'
    0xc5, // STORE_FAST 5
    0xb5, // LOAD_FAST 5
    0x43,0x4b, // POP_JUMP_IF_TRUE 11
    0xb4, // LOAD_FAST 4
    0x13,0x22, // LOAD_ATTR 'coro'
    0x14,0x19, // LOAD_METHOD 'send'
    0x51, // LOAD_CONST_NONE
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x4d, // JUMP 13
    0x51, // LOAD_CONST_NONE
    0xb4, // LOAD_FAST 4
    0x18,0x21, // STORE_ATTR 'data'
    0xb4, // LOAD_FAST 4
    0x13,0x22, // LOAD_ATTR 'coro'
    0x14,0x23, // LOAD_METHOD 'throw'
    0xb5, // LOAD_FAST 5
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x4a,0xcc,0x01, // POP_EXCEPT_JUMP 204
    0x57, // DUP_TOP
    0xb1, // LOAD_FAST 1
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0xc5,0x81, // POP_JUMP_IF_FALSE 197
    0xc6, // STORE_FAST 6
    0x49,0xba,0x01, // SETUP_FINALLY 186
    0xb4, // LOAD_FAST 4
    0xb0, // LOAD_FAST 0
    0xde, // BINARY_OP 7 <is>
    0xc7, // STORE_FAST 7
    0xb7, // LOAD_FAST 7
    0x44,0x5d, // POP_JUMP_IF_FALSE 29
    0x51, // LOAD_CONST_NONE
    0x17,0x4d, // STORE_GLOBAL 'cur_task'
    0x12,0x57, // LOAD_GLOBAL 'isinstance'
    0xb6, // LOAD_FAST 6
    0x12,0x5d, // LOAD_GLOBAL 'StopIteration'
    0x34,0x02, // CALL_FUNCTION 2
    0x43,0x46, // POP_JUMP_IF_TRUE 6
    0x50, // LOAD_CONST_FALSE
    0xb4, // LOAD_FAST 4
    0x18,0x15, // STORE_ATTR 'state'
    0xb6, // LOAD_FAST 6
    0x65, // RAISE_OBJ
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0x50, // LOAD_CONST_FALSE
    0xb4, // LOAD_FAST 4
    0x18,0x15, // STORE_ATTR 'state'
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x44,0xe4,0x80, // POP_JUMP_IF_FALSE 100
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x52, // LOAD_CONST_TRUE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x50, // LOAD_CONST_FALSE
    0x42,0x41, // JUMP 1
    0x51, // LOAD_CONST_NONE
    0xb4, // LOAD_FAST 4
    0x18,0x15, // STORE_ATTR 'state'
    0x42,0x78, // JUMP 56
    0x12,0x5f, // LOAD_GLOBAL 'callable'
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x34,0x01, // CALL_FUNCTION 1
    0x44,0x50, // POP_JUMP_IF_FALSE 16
    0xb4, // LOAD_FAST 4
    0x14,0x15, // LOAD_METHOD 'state'
    0xb4, // LOAD_FAST 4
    0xb6, // LOAD_FAST 6
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x50, // LOAD_CONST_FALSE
    0xb4, // LOAD_FAST 4
    0x18,0x15, // STORE_ATTR 'state'
    0x52, // LOAD_CONST_TRUE
    0xc7, // STORE_FAST 7
    0x42,0x5f, // JUMP 31
    0x42,0x50, // JUMP 16
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x14,0x20, // LOAD_METHOD 'pop'
    0x36,0x00, // CALL_METHOD 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x52, // LOAD_CONST_TRUE
    0xc7, // STORE_FAST 7
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x14,0x1c, // LOAD_METHOD 'peek'
    0x36,0x00, // CALL_METHOD 0
    0x43,0x27, // POP_JUMP_IF_TRUE -25
    0x50, // LOAD_CONST_FALSE
    0xb4, // LOAD_FAST 4
    0x18,0x15, // STORE_ATTR 'state'
    0xb7, // LOAD_FAST 7
    0x43,0x50, // POP_JUMP_IF_TRUE 16
    0x12,0x57, // LOAD_GLOBAL 'isinstance'
    0xb6, // LOAD_FAST 6
    0xb2, // LOAD_FAST 2
    0x34,0x02, // CALL_FUNCTION 2
    0x43,0x48, // POP_JUMP_IF_TRUE 8
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0xb4, // LOAD_FAST 4
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb6, // LOAD_FAST 6
    0xb4, // LOAD_FAST 4
    0x18,0x21, // STORE_ATTR 'data'
    0x42,0x62, // JUMP 34
    0xb4, // LOAD_FAST 4
    0x13,0x15, // LOAD_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x5b, // POP_JUMP_IF_FALSE 27
    0xb5, // LOAD_FAST 5
    0xb4, // LOAD_FAST 4
    0x18,0x21, // STORE_ATTR 'data'
    0xb5, // LOAD_FAST 5
    0x12,0x4c, // LOAD_GLOBAL '_exc_context'
    0x10,0x0f, // LOAD_CONST_STRING 'exception'
    0x56, // STORE_SUBSCR
    0xb4, // LOAD_FAST 4
    0x12,0x4c, // LOAD_GLOBAL '_exc_context'
    0x10,0x10, // LOAD_CONST_STRING 'future'
    0x56, // STORE_SUBSCR
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x14,0x24, // LOAD_METHOD 'call_exception_handler'
    0x12,0x4c, // LOAD_GLOBAL '_exc_context'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x40, // JUMP 0
    0xb4, // LOAD_FAST 4
    0xb0, // LOAD_FAST 0
    0xde, // BINARY_OP 7 <is>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0xb6, // LOAD_FAST 6
    0x13,0x25, // LOAD_ATTR 'value'
    0x63, // RETURN_VALUE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xc6, // STORE_FAST 6
    0x28,0x06, // DELETE_FAST 6
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0x42,0xbb,0x7d, // JUMP -325
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_run_until_complete = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_run_until_complete,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 346,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 15,
        .n_exc_stack = 2,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 27,
        .line_info = fun_data_asyncio_core_run_until_complete + 5,
        .line_info_top = fun_data_asyncio_core_run_until_complete + 5,
        .opcodes = fun_data_asyncio_core_run_until_complete + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_run_until_complete fun_data_asyncio_core_run_until_complete[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_run
static const byte fun_data_asyncio_core_run[14] = {
    0x19,0x04, // prelude
    0x26,0x22, // names: run, coro
     // code info
    0x12,0x1b, // LOAD_GLOBAL 'run_until_complete'
    0x12,0x18, // LOAD_GLOBAL 'create_task'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_run = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_run,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 38,
        .line_info = fun_data_asyncio_core_run + 4,
        .line_info_top = fun_data_asyncio_core_run + 4,
        .opcodes = fun_data_asyncio_core_run + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_run fun_data_asyncio_core_run[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core__stopper
static const byte fun_data_asyncio_core__stopper[6] = {
    0x80,0x40,0x02, // prelude
    0x27, // names: _stopper
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core__stopper = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_core__stopper,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 6,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 39,
        .line_info = fun_data_asyncio_core__stopper + 4,
        .line_info_top = fun_data_asyncio_core__stopper + 4,
        .opcodes = fun_data_asyncio_core__stopper + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core__stopper fun_data_asyncio_core__stopper[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop
static const byte fun_data_asyncio_core_Loop[52] = {
    0x00,0x02, // prelude
    0x13, // names: Loop
     // code info
    0x11,0x4f, // LOAD_NAME '__name__'
    0x16,0x50, // STORE_NAME '__module__'
    0x10,0x13, // LOAD_CONST_STRING 'Loop'
    0x16,0x51, // STORE_NAME '__qualname__'
    0x51, // LOAD_CONST_NONE
    0x16,0x41, // STORE_NAME '_exc_handler'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x18, // STORE_NAME 'create_task'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x3d, // STORE_NAME 'run_forever'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x1b, // STORE_NAME 'run_until_complete'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x3e, // STORE_NAME 'stop'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x3f, // STORE_NAME 'close'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x40, // STORE_NAME 'set_exception_handler'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x42, // STORE_NAME 'get_exception_handler'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x43, // STORE_NAME 'default_exception_handler'
    0x32,0x08, // MAKE_FUNCTION 8
    0x16,0x24, // STORE_NAME 'call_exception_handler'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_create_task
static const byte fun_data_asyncio_core_Loop_create_task[10] = {
    0x11,0x04, // prelude
    0x18,0x22, // names: create_task, coro
     // code info
    0x12,0x18, // LOAD_GLOBAL 'create_task'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_create_task = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_create_task,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 24,
        .line_info = fun_data_asyncio_core_Loop_create_task + 4,
        .line_info_top = fun_data_asyncio_core_Loop_create_task + 4,
        .opcodes = fun_data_asyncio_core_Loop_create_task + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_create_task fun_data_asyncio_core_Loop_create_task[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_run_forever
static const byte fun_data_asyncio_core_Loop_run_forever[26] = {
    0x10,0x02, // prelude
    0x3d, // names: run_forever
     // code info
    0x12,0x09, // LOAD_GLOBAL 'Task'
    0x12,0x27, // LOAD_GLOBAL '_stopper'
    0x34,0x00, // CALL_FUNCTION 0
    0x12,0x5a, // LOAD_GLOBAL 'globals'
    0x34,0x00, // CALL_FUNCTION 0
    0x34,0x02, // CALL_FUNCTION 2
    0x17,0x4e, // STORE_GLOBAL '_stop_task'
    0x12,0x1b, // LOAD_GLOBAL 'run_until_complete'
    0x12,0x4e, // LOAD_GLOBAL '_stop_task'
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_run_forever = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_run_forever,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 61,
        .line_info = fun_data_asyncio_core_Loop_run_forever + 3,
        .line_info_top = fun_data_asyncio_core_Loop_run_forever + 3,
        .opcodes = fun_data_asyncio_core_Loop_run_forever + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_run_forever fun_data_asyncio_core_Loop_run_forever[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_run_until_complete
static const byte fun_data_asyncio_core_Loop_run_until_complete[14] = {
    0x19,0x04, // prelude
    0x1b,0x56, // names: run_until_complete, aw
     // code info
    0x12,0x1b, // LOAD_GLOBAL 'run_until_complete'
    0x12,0x17, // LOAD_GLOBAL '_promote_to_task'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_run_until_complete = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_run_until_complete,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 27,
        .line_info = fun_data_asyncio_core_Loop_run_until_complete + 4,
        .line_info_top = fun_data_asyncio_core_Loop_run_until_complete + 4,
        .opcodes = fun_data_asyncio_core_Loop_run_until_complete + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_run_until_complete fun_data_asyncio_core_Loop_run_until_complete[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_stop
static const byte fun_data_asyncio_core_Loop_stop[24] = {
    0x10,0x02, // prelude
    0x3e, // names: stop
     // code info
    0x12,0x4e, // LOAD_GLOBAL '_stop_task'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0x12,0x5b, // LOAD_GLOBAL '_task_queue'
    0x14,0x1a, // LOAD_METHOD 'push'
    0x12,0x4e, // LOAD_GLOBAL '_stop_task'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x17,0x4e, // STORE_GLOBAL '_stop_task'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_stop = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_stop,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 62,
        .line_info = fun_data_asyncio_core_Loop_stop + 3,
        .line_info_top = fun_data_asyncio_core_Loop_stop + 3,
        .opcodes = fun_data_asyncio_core_Loop_stop + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_stop fun_data_asyncio_core_Loop_stop[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_close
static const byte fun_data_asyncio_core_Loop_close[5] = {
    0x00,0x02, // prelude
    0x3f, // names: close
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_close = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_close,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 5,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 63,
        .line_info = fun_data_asyncio_core_Loop_close + 3,
        .line_info_top = fun_data_asyncio_core_Loop_close + 3,
        .opcodes = fun_data_asyncio_core_Loop_close + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_close fun_data_asyncio_core_Loop_close[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_set_exception_handler
static const byte fun_data_asyncio_core_Loop_set_exception_handler[11] = {
    0x11,0x04, // prelude
    0x40,0x68, // names: set_exception_handler, handler
     // code info
    0xb0, // LOAD_FAST 0
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x18,0x41, // STORE_ATTR '_exc_handler'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_set_exception_handler = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_set_exception_handler,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 11,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 64,
        .line_info = fun_data_asyncio_core_Loop_set_exception_handler + 4,
        .line_info_top = fun_data_asyncio_core_Loop_set_exception_handler + 4,
        .opcodes = fun_data_asyncio_core_Loop_set_exception_handler + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_set_exception_handler fun_data_asyncio_core_Loop_set_exception_handler[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_get_exception_handler
static const byte fun_data_asyncio_core_Loop_get_exception_handler[8] = {
    0x00,0x02, // prelude
    0x42, // names: get_exception_handler
     // code info
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x13,0x41, // LOAD_ATTR '_exc_handler'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_get_exception_handler = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_get_exception_handler,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 8,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 66,
        .line_info = fun_data_asyncio_core_Loop_get_exception_handler + 3,
        .line_info_top = fun_data_asyncio_core_Loop_get_exception_handler + 3,
        .opcodes = fun_data_asyncio_core_Loop_get_exception_handler + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_get_exception_handler fun_data_asyncio_core_Loop_get_exception_handler[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_default_exception_handler
static const byte fun_data_asyncio_core_Loop_default_exception_handler[64] = {
    0x42,0x06, // prelude
    0x43,0x69,0x6a, // names: default_exception_handler, loop, context
     // code info
    0x12,0x6b, // LOAD_GLOBAL 'print'
    0xb1, // LOAD_FAST 1
    0x10,0x0e, // LOAD_CONST_STRING 'message'
    0x55, // LOAD_SUBSCR
    0x10,0x44, // LOAD_CONST_STRING 'file'
    0x12,0x06, // LOAD_GLOBAL 'sys'
    0x13,0x45, // LOAD_ATTR 'stderr'
    0x34,0x82,0x01, // CALL_FUNCTION 257
    0x59, // POP_TOP
    0x12,0x6b, // LOAD_GLOBAL 'print'
    0x10,0x46, // LOAD_CONST_STRING 'future:'
    0xb1, // LOAD_FAST 1
    0x10,0x10, // LOAD_CONST_STRING 'future'
    0x55, // LOAD_SUBSCR
    0x10,0x47, // LOAD_CONST_STRING 'coro='
    0xb1, // LOAD_FAST 1
    0x10,0x10, // LOAD_CONST_STRING 'future'
    0x55, // LOAD_SUBSCR
    0x13,0x22, // LOAD_ATTR 'coro'
    0x10,0x44, // LOAD_CONST_STRING 'file'
    0x12,0x06, // LOAD_GLOBAL 'sys'
    0x13,0x45, // LOAD_ATTR 'stderr'
    0x34,0x82,0x04, // CALL_FUNCTION 260
    0x59, // POP_TOP
    0x12,0x06, // LOAD_GLOBAL 'sys'
    0x14,0x48, // LOAD_METHOD 'print_exception'
    0xb1, // LOAD_FAST 1
    0x10,0x0f, // LOAD_CONST_STRING 'exception'
    0x55, // LOAD_SUBSCR
    0x12,0x06, // LOAD_GLOBAL 'sys'
    0x13,0x45, // LOAD_ATTR 'stderr'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_default_exception_handler = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_default_exception_handler,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 64,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 67,
        .line_info = fun_data_asyncio_core_Loop_default_exception_handler + 5,
        .line_info_top = fun_data_asyncio_core_Loop_default_exception_handler + 5,
        .opcodes = fun_data_asyncio_core_Loop_default_exception_handler + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_default_exception_handler fun_data_asyncio_core_Loop_default_exception_handler[0]
#endif

// child of asyncio_core_Loop
// frozen bytecode for file asyncio/core.py, scope asyncio_core_Loop_call_exception_handler
static const byte fun_data_asyncio_core_Loop_call_exception_handler[22] = {
    0x19,0x04, // prelude
    0x24,0x6a, // names: call_exception_handler, context
     // code info
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x13,0x41, // LOAD_ATTR '_exc_handler'
    0x45,0x04, // JUMP_IF_TRUE_OR_POP 4
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x13,0x43, // LOAD_ATTR 'default_exception_handler'
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0xb0, // LOAD_FAST 0
    0x34,0x02, // CALL_FUNCTION 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop_call_exception_handler = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop_call_exception_handler,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 22,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 36,
        .line_info = fun_data_asyncio_core_Loop_call_exception_handler + 4,
        .line_info_top = fun_data_asyncio_core_Loop_call_exception_handler + 4,
        .opcodes = fun_data_asyncio_core_Loop_call_exception_handler + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_Loop_call_exception_handler fun_data_asyncio_core_Loop_call_exception_handler[0]
#endif

static const mp_raw_code_t *const children_asyncio_core_Loop[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_create_task,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_run_forever,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_run_until_complete,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_stop,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_close,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_set_exception_handler,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_get_exception_handler,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_default_exception_handler,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop_call_exception_handler,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_core_Loop = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_Loop,
    .children = (void *)&children_asyncio_core_Loop,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 52,
    .n_children = 9,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 19,
        .line_info = fun_data_asyncio_core_Loop + 3,
        .line_info_top = fun_data_asyncio_core_Loop + 3,
        .opcodes = fun_data_asyncio_core_Loop + 3,
    },
    #endif
    #endif
};

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_get_event_loop
static const byte fun_data_asyncio_core_get_event_loop[10] = {
    0x92,0x80,0x01,0x06, // prelude
    0x28,0x60,0x61, // names: get_event_loop, runq_len, waitq_len
     // code info
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_get_event_loop = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_get_event_loop,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 40,
        .line_info = fun_data_asyncio_core_get_event_loop + 7,
        .line_info_top = fun_data_asyncio_core_get_event_loop + 7,
        .opcodes = fun_data_asyncio_core_get_event_loop + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_get_event_loop fun_data_asyncio_core_get_event_loop[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_current_task
static const byte fun_data_asyncio_core_current_task[19] = {
    0x08,0x02, // prelude
    0x29, // names: current_task
     // code info
    0x12,0x4d, // LOAD_GLOBAL 'cur_task'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0x12,0x62, // LOAD_GLOBAL 'RuntimeError'
    0x23,0x02, // LOAD_CONST_OBJ 2
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x4d, // LOAD_GLOBAL 'cur_task'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_current_task = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_current_task,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 19,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 41,
        .line_info = fun_data_asyncio_core_current_task + 3,
        .line_info_top = fun_data_asyncio_core_current_task + 3,
        .opcodes = fun_data_asyncio_core_current_task + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_current_task fun_data_asyncio_core_current_task[0]
#endif

// child of asyncio_core__lt_module_gt_
// frozen bytecode for file asyncio/core.py, scope asyncio_core_new_event_loop
static const byte fun_data_asyncio_core_new_event_loop[18] = {
    0x00,0x02, // prelude
    0x2a, // names: new_event_loop
     // code info
    0x12,0x08, // LOAD_GLOBAL 'TaskQueue'
    0x34,0x00, // CALL_FUNCTION 0
    0x17,0x5b, // STORE_GLOBAL '_task_queue'
    0x12,0x12, // LOAD_GLOBAL 'IOQueue'
    0x34,0x00, // CALL_FUNCTION 0
    0x17,0x5e, // STORE_GLOBAL '_io_queue'
    0x12,0x13, // LOAD_GLOBAL 'Loop'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_core_new_event_loop = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core_new_event_loop,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 18,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 42,
        .line_info = fun_data_asyncio_core_new_event_loop + 3,
        .line_info_top = fun_data_asyncio_core_new_event_loop + 3,
        .opcodes = fun_data_asyncio_core_new_event_loop + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_core_new_event_loop fun_data_asyncio_core_new_event_loop[0]
#endif

static const mp_raw_code_t *const children_asyncio_core__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_core_CancelledError,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_TimeoutError,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_SingletonGenerator,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_sleep_ms,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_sleep,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_IOQueue,
    (const mp_raw_code_t *)&proto_fun_asyncio_core__promote_to_task,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_create_task,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_run_until_complete,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_run,
    (const mp_raw_code_t *)&proto_fun_asyncio_core__stopper,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_Loop,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_get_event_loop,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_current_task,
    (const mp_raw_code_t *)&proto_fun_asyncio_core_new_event_loop,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_core__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_core__lt_module_gt_,
    .children = (void *)&children_asyncio_core__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 218,
    .n_children = 15,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 1,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_asyncio_core__lt_module_gt_ + 3,
        .line_info_top = fun_data_asyncio_core__lt_module_gt_ + 3,
        .opcodes = fun_data_asyncio_core__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_asyncio_core[108] = {
    MP_QSTR_asyncio_slash_core_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_ticks_ms,
    MP_QSTR_ticks_diff,
    MP_QSTR_ticks_add,
    MP_QSTR_time,
    MP_QSTR_sys,
    MP_QSTR_select,
    MP_QSTR_TaskQueue,
    MP_QSTR_Task,
    MP_QSTR__asyncio,
    MP_QSTR_task,
    MP_QSTR_CancelledError,
    MP_QSTR_TimeoutError,
    MP_QSTR_message,
    MP_QSTR_exception,
    MP_QSTR_future,
    MP_QSTR_SingletonGenerator,
    MP_QSTR_IOQueue,
    MP_QSTR_Loop,
    MP_QSTR_sleep_ms,
    MP_QSTR_state,
    MP_QSTR_sleep,
    MP_QSTR__promote_to_task,
    MP_QSTR_create_task,
    MP_QSTR_send,
    MP_QSTR_push,
    MP_QSTR_run_until_complete,
    MP_QSTR_peek,
    MP_QSTR_ph_key,
    MP_QSTR_map,
    MP_QSTR_wait_io_event,
    MP_QSTR_pop,
    MP_QSTR_data,
    MP_QSTR_coro,
    MP_QSTR_throw,
    MP_QSTR_call_exception_handler,
    MP_QSTR_value,
    MP_QSTR_run,
    MP_QSTR__stopper,
    MP_QSTR_get_event_loop,
    MP_QSTR_current_task,
    MP_QSTR_new_event_loop,
    MP_QSTR___init__,
    MP_QSTR_exc,
    MP_QSTR___iter__,
    MP_QSTR___next__,
    MP_QSTR___traceback__,
    MP_QSTR_poll,
    MP_QSTR_poller,
    MP_QSTR__enqueue,
    MP_QSTR_register,
    MP_QSTR_POLLIN,
    MP_QSTR_POLLOUT,
    MP_QSTR_modify,
    MP_QSTR__dequeue,
    MP_QSTR_unregister,
    MP_QSTR_queue_read,
    MP_QSTR_queue_write,
    MP_QSTR_remove,
    MP_QSTR_ipoll,
    MP_QSTR_run_forever,
    MP_QSTR_stop,
    MP_QSTR_close,
    MP_QSTR_set_exception_handler,
    MP_QSTR__exc_handler,
    MP_QSTR_get_exception_handler,
    MP_QSTR_default_exception_handler,
    MP_QSTR_file,
    MP_QSTR_stderr,
    MP_QSTR_future_colon_,
    MP_QSTR_coro_equals_,
    MP_QSTR_print_exception,
    MP_QSTR_ticks,
    MP_QSTR_BaseException,
    MP_QSTR_Exception,
    MP_QSTR__exc_context,
    MP_QSTR_cur_task,
    MP_QSTR__stop_task,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_t,
    MP_QSTR_sgen,
    MP_QSTR_max,
    MP_QSTR_int,
    MP_QSTR_aw,
    MP_QSTR_isinstance,
    MP_QSTR_hasattr,
    MP_QSTR_TypeError,
    MP_QSTR_globals,
    MP_QSTR__task_queue,
    MP_QSTR_main_task,
    MP_QSTR_StopIteration,
    MP_QSTR__io_queue,
    MP_QSTR_callable,
    MP_QSTR_runq_len,
    MP_QSTR_waitq_len,
    MP_QSTR_RuntimeError,
    MP_QSTR_self,
    MP_QSTR_s,
    MP_QSTR_idx,
    MP_QSTR_id,
    MP_QSTR_dt,
    MP_QSTR_handler,
    MP_QSTR_loop,
    MP_QSTR_context,
    MP_QSTR_print,
};

// constants
static const mp_obj_str_t const_obj_asyncio_core_0 = {{&mp_type_str}, 64973, 31, (const byte*)"\x54\x61\x73\x6b\x20\x65\x78\x63\x65\x70\x74\x69\x6f\x6e\x20\x77\x61\x73\x6e\x27\x74\x20\x72\x65\x74\x72\x69\x65\x76\x65\x64"};

// constant table
static const mp_rom_obj_t const_obj_table_data_asyncio_core[3] = {
    MP_ROM_PTR(&const_obj_asyncio_core_0),
    MP_ROM_QSTR(MP_QSTR_coroutine_space_expected),
    MP_ROM_QSTR(MP_QSTR_no_space_running_space_event_space_loop),
};

static const mp_frozen_module_t frozen_module_asyncio_core = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_asyncio_core,
        .obj_table = (mp_obj_t *)&const_obj_table_data_asyncio_core,
    },
    .proto_fun = &proto_fun_asyncio_core__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module asyncio_event
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\asyncio\event.mpy
// - frozen file name: asyncio/event.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file asyncio/event.py, scope asyncio_event__lt_module_gt_
static const byte fun_data_asyncio_event__lt_module_gt_[59] = {
    0x2c,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x02, // LOAD_CONST_STRING 'core'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x03, // IMPORT_NAME ''
    0x1c,0x02, // IMPORT_FROM 'core'
    0x16,0x02, // STORE_NAME 'core'
    0x59, // POP_TOP
    0x54, // LOAD_BUILD_CLASS
    0x32,0x00, // MAKE_FUNCTION 0
    0x10,0x04, // LOAD_CONST_STRING 'Event'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x04, // STORE_NAME 'Event'
    0x48,0x15, // SETUP_EXCEPT 21
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x05, // IMPORT_NAME 'io'
    0x16,0x05, // STORE_NAME 'io'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x01, // MAKE_FUNCTION 1
    0x10,0x06, // LOAD_CONST_STRING 'ThreadSafeFlag'
    0x11,0x05, // LOAD_NAME 'io'
    0x13,0x07, // LOAD_ATTR 'IOBase'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x06, // STORE_NAME 'ThreadSafeFlag'
    0x4a,0x0a, // POP_EXCEPT_JUMP 10
    0x57, // DUP_TOP
    0x11,0x19, // LOAD_NAME 'ImportError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x59, // POP_TOP
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_event__lt_module_gt_
// frozen bytecode for file asyncio/event.py, scope asyncio_event_Event
static const byte fun_data_asyncio_event_Event[33] = {
    0x00,0x02, // prelude
    0x04, // names: Event
     // code info
    0x11,0x1a, // LOAD_NAME '__name__'
    0x16,0x1b, // STORE_NAME '__module__'
    0x10,0x04, // LOAD_CONST_STRING 'Event'
    0x16,0x1c, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x08, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x0c, // STORE_NAME 'is_set'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x0d, // STORE_NAME 'set'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x12, // STORE_NAME 'clear'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x13, // STORE_NAME 'wait'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_event_Event
// frozen bytecode for file asyncio/event.py, scope asyncio_event_Event___init__
static const byte fun_data_asyncio_event_Event___init__[19] = {
    0x11,0x04, // prelude
    0x08,0x1d, // names: __init__, self
     // code info
    0x50, // LOAD_CONST_FALSE
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x0a, // LOAD_METHOD 'TaskQueue'
    0x36,0x00, // CALL_METHOD 0
    0xb0, // LOAD_FAST 0
    0x18,0x0b, // STORE_ATTR 'waiting'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_Event___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_Event___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 19,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 8,
        .line_info = fun_data_asyncio_event_Event___init__ + 4,
        .line_info_top = fun_data_asyncio_event_Event___init__ + 4,
        .opcodes = fun_data_asyncio_event_Event___init__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_Event___init__ fun_data_asyncio_event_Event___init__[0]
#endif

// child of asyncio_event_Event
// frozen bytecode for file asyncio/event.py, scope asyncio_event_Event_is_set
static const byte fun_data_asyncio_event_Event_is_set[8] = {
    0x09,0x04, // prelude
    0x0c,0x1d, // names: is_set, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'state'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_Event_is_set = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_Event_is_set,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 8,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 12,
        .line_info = fun_data_asyncio_event_Event_is_set + 4,
        .line_info_top = fun_data_asyncio_event_Event_is_set + 4,
        .opcodes = fun_data_asyncio_event_Event_is_set + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_Event_is_set fun_data_asyncio_event_Event_is_set[0]
#endif

// child of asyncio_event_Event
// frozen bytecode for file asyncio/event.py, scope asyncio_event_Event_set
static const byte fun_data_asyncio_event_Event_set[37] = {
    0x21,0x04, // prelude
    0x0d,0x1d, // names: set, self
     // code info
    0x42,0x50, // JUMP 16
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x0e, // LOAD_ATTR '_task_queue'
    0x14,0x0f, // LOAD_METHOD 'push'
    0xb0, // LOAD_FAST 0
    0x13,0x0b, // LOAD_ATTR 'waiting'
    0x14,0x10, // LOAD_METHOD 'pop'
    0x36,0x00, // CALL_METHOD 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x0b, // LOAD_ATTR 'waiting'
    0x14,0x11, // LOAD_METHOD 'peek'
    0x36,0x00, // CALL_METHOD 0
    0x43,0x27, // POP_JUMP_IF_TRUE -25
    0x52, // LOAD_CONST_TRUE
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_Event_set = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_Event_set,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 37,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 13,
        .line_info = fun_data_asyncio_event_Event_set + 4,
        .line_info_top = fun_data_asyncio_event_Event_set + 4,
        .opcodes = fun_data_asyncio_event_Event_set + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_Event_set fun_data_asyncio_event_Event_set[0]
#endif

// child of asyncio_event_Event
// frozen bytecode for file asyncio/event.py, scope asyncio_event_Event_clear
static const byte fun_data_asyncio_event_Event_clear[10] = {
    0x11,0x04, // prelude
    0x12,0x1d, // names: clear, self
     // code info
    0x50, // LOAD_CONST_FALSE
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_Event_clear = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_Event_clear,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 18,
        .line_info = fun_data_asyncio_event_Event_clear + 4,
        .line_info_top = fun_data_asyncio_event_Event_clear + 4,
        .opcodes = fun_data_asyncio_event_Event_clear + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_Event_clear fun_data_asyncio_event_Event_clear[0]
#endif

// child of asyncio_event_Event
// frozen bytecode for file asyncio/event.py, scope asyncio_event_Event_wait
static const byte fun_data_asyncio_event_Event_wait[36] = {
    0x99,0x40,0x04, // prelude
    0x13,0x1d, // names: wait, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'state'
    0x43,0x58, // POP_JUMP_IF_TRUE 24
    0xb0, // LOAD_FAST 0
    0x13,0x0b, // LOAD_ATTR 'waiting'
    0x14,0x0f, // LOAD_METHOD 'push'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x14, // LOAD_ATTR 'cur_task'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x0b, // LOAD_ATTR 'waiting'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x14, // LOAD_ATTR 'cur_task'
    0x18,0x15, // STORE_ATTR 'data'
    0x51, // LOAD_CONST_NONE
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x52, // LOAD_CONST_TRUE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_Event_wait = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_event_Event_wait,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 36,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 19,
        .line_info = fun_data_asyncio_event_Event_wait + 5,
        .line_info_top = fun_data_asyncio_event_Event_wait + 5,
        .opcodes = fun_data_asyncio_event_Event_wait + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_Event_wait fun_data_asyncio_event_Event_wait[0]
#endif

static const mp_raw_code_t *const children_asyncio_event_Event[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_event_Event___init__,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_Event_is_set,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_Event_set,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_Event_clear,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_Event_wait,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_event_Event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_Event,
    .children = (void *)&children_asyncio_event_Event,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 33,
    .n_children = 5,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 4,
        .line_info = fun_data_asyncio_event_Event + 3,
        .line_info_top = fun_data_asyncio_event_Event + 3,
        .opcodes = fun_data_asyncio_event_Event + 3,
    },
    #endif
    #endif
};

// child of asyncio_event__lt_module_gt_
// frozen bytecode for file asyncio/event.py, scope asyncio_event_ThreadSafeFlag
static const byte fun_data_asyncio_event_ThreadSafeFlag[33] = {
    0x00,0x02, // prelude
    0x06, // names: ThreadSafeFlag
     // code info
    0x11,0x1a, // LOAD_NAME '__name__'
    0x16,0x1b, // STORE_NAME '__module__'
    0x10,0x06, // LOAD_CONST_STRING 'ThreadSafeFlag'
    0x16,0x1c, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x08, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x16, // STORE_NAME 'ioctl'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x0d, // STORE_NAME 'set'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x12, // STORE_NAME 'clear'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x13, // STORE_NAME 'wait'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_event_ThreadSafeFlag
// frozen bytecode for file asyncio/event.py, scope asyncio_event_ThreadSafeFlag___init__
static const byte fun_data_asyncio_event_ThreadSafeFlag___init__[10] = {
    0x11,0x04, // prelude
    0x08,0x1d, // names: __init__, self
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_ThreadSafeFlag___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_ThreadSafeFlag___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 8,
        .line_info = fun_data_asyncio_event_ThreadSafeFlag___init__ + 4,
        .line_info_top = fun_data_asyncio_event_ThreadSafeFlag___init__ + 4,
        .opcodes = fun_data_asyncio_event_ThreadSafeFlag___init__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_ThreadSafeFlag___init__ fun_data_asyncio_event_ThreadSafeFlag___init__[0]
#endif

// child of asyncio_event_ThreadSafeFlag
// frozen bytecode for file asyncio/event.py, scope asyncio_event_ThreadSafeFlag_ioctl
static const byte fun_data_asyncio_event_ThreadSafeFlag_ioctl[19] = {
    0x23,0x08, // prelude
    0x16,0x1d,0x1e,0x1f, // names: ioctl, self, req, flags
     // code info
    0xb1, // LOAD_FAST 1
    0x83, // LOAD_CONST_SMALL_INT 3
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'state'
    0xb2, // LOAD_FAST 2
    0xf4, // BINARY_OP 29 __mul__
    0x63, // RETURN_VALUE
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_ThreadSafeFlag_ioctl = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_ThreadSafeFlag_ioctl,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 19,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 22,
        .line_info = fun_data_asyncio_event_ThreadSafeFlag_ioctl + 6,
        .line_info_top = fun_data_asyncio_event_ThreadSafeFlag_ioctl + 6,
        .opcodes = fun_data_asyncio_event_ThreadSafeFlag_ioctl + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_ThreadSafeFlag_ioctl fun_data_asyncio_event_ThreadSafeFlag_ioctl[0]
#endif

// child of asyncio_event_ThreadSafeFlag
// frozen bytecode for file asyncio/event.py, scope asyncio_event_ThreadSafeFlag_set
static const byte fun_data_asyncio_event_ThreadSafeFlag_set[10] = {
    0x11,0x04, // prelude
    0x0d,0x1d, // names: set, self
     // code info
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_ThreadSafeFlag_set = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_ThreadSafeFlag_set,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 13,
        .line_info = fun_data_asyncio_event_ThreadSafeFlag_set + 4,
        .line_info_top = fun_data_asyncio_event_ThreadSafeFlag_set + 4,
        .opcodes = fun_data_asyncio_event_ThreadSafeFlag_set + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_ThreadSafeFlag_set fun_data_asyncio_event_ThreadSafeFlag_set[0]
#endif

// child of asyncio_event_ThreadSafeFlag
// frozen bytecode for file asyncio/event.py, scope asyncio_event_ThreadSafeFlag_clear
static const byte fun_data_asyncio_event_ThreadSafeFlag_clear[10] = {
    0x11,0x04, // prelude
    0x12,0x1d, // names: clear, self
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_ThreadSafeFlag_clear = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_ThreadSafeFlag_clear,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 18,
        .line_info = fun_data_asyncio_event_ThreadSafeFlag_clear + 4,
        .line_info_top = fun_data_asyncio_event_ThreadSafeFlag_clear + 4,
        .opcodes = fun_data_asyncio_event_ThreadSafeFlag_clear + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_ThreadSafeFlag_clear fun_data_asyncio_event_ThreadSafeFlag_clear[0]
#endif

// child of asyncio_event_ThreadSafeFlag
// frozen bytecode for file asyncio/event.py, scope asyncio_event_ThreadSafeFlag_wait
static const byte fun_data_asyncio_event_ThreadSafeFlag_wait[27] = {
    0x99,0x40,0x04, // prelude
    0x13,0x1d, // names: wait, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'state'
    0x43,0x4b, // POP_JUMP_IF_TRUE 11
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x17, // LOAD_ATTR '_io_queue'
    0x14,0x18, // LOAD_METHOD 'queue_read'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_event_ThreadSafeFlag_wait = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_event_ThreadSafeFlag_wait,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 27,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 19,
        .line_info = fun_data_asyncio_event_ThreadSafeFlag_wait + 5,
        .line_info_top = fun_data_asyncio_event_ThreadSafeFlag_wait + 5,
        .opcodes = fun_data_asyncio_event_ThreadSafeFlag_wait + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_event_ThreadSafeFlag_wait fun_data_asyncio_event_ThreadSafeFlag_wait[0]
#endif

static const mp_raw_code_t *const children_asyncio_event_ThreadSafeFlag[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_event_ThreadSafeFlag___init__,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_ThreadSafeFlag_ioctl,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_ThreadSafeFlag_set,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_ThreadSafeFlag_clear,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_ThreadSafeFlag_wait,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_event_ThreadSafeFlag = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event_ThreadSafeFlag,
    .children = (void *)&children_asyncio_event_ThreadSafeFlag,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 33,
    .n_children = 5,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 6,
        .line_info = fun_data_asyncio_event_ThreadSafeFlag + 3,
        .line_info_top = fun_data_asyncio_event_ThreadSafeFlag + 3,
        .opcodes = fun_data_asyncio_event_ThreadSafeFlag + 3,
    },
    #endif
    #endif
};

static const mp_raw_code_t *const children_asyncio_event__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_event_Event,
    (const mp_raw_code_t *)&proto_fun_asyncio_event_ThreadSafeFlag,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_event__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_event__lt_module_gt_,
    .children = (void *)&children_asyncio_event__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 59,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 1,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_asyncio_event__lt_module_gt_ + 3,
        .line_info_top = fun_data_asyncio_event__lt_module_gt_ + 3,
        .opcodes = fun_data_asyncio_event__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_asyncio_event[32] = {
    MP_QSTR_asyncio_slash_event_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_core,
    MP_QSTR_,
    MP_QSTR_Event,
    MP_QSTR_io,
    MP_QSTR_ThreadSafeFlag,
    MP_QSTR_IOBase,
    MP_QSTR___init__,
    MP_QSTR_state,
    MP_QSTR_TaskQueue,
    MP_QSTR_waiting,
    MP_QSTR_is_set,
    MP_QSTR_set,
    MP_QSTR__task_queue,
    MP_QSTR_push,
    MP_QSTR_pop,
    MP_QSTR_peek,
    MP_QSTR_clear,
    MP_QSTR_wait,
    MP_QSTR_cur_task,
    MP_QSTR_data,
    MP_QSTR_ioctl,
    MP_QSTR__io_queue,
    MP_QSTR_queue_read,
    MP_QSTR_ImportError,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_self,
    MP_QSTR_req,
    MP_QSTR_flags,
};

static const mp_frozen_module_t frozen_module_asyncio_event = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_asyncio_event,
        .obj_table = NULL,
    },
    .proto_fun = &proto_fun_asyncio_event__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module asyncio_funcs
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\asyncio\funcs.mpy
// - frozen file name: asyncio/funcs.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs__lt_module_gt_
static const byte fun_data_asyncio_funcs__lt_module_gt_[56] = {
    0x18,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x02, // LOAD_CONST_STRING 'core'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x03, // IMPORT_NAME ''
    0x1c,0x02, // IMPORT_FROM 'core'
    0x16,0x02, // STORE_NAME 'core'
    0x59, // POP_TOP
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x07, // STORE_NAME '_run'
    0x11,0x02, // LOAD_NAME 'core'
    0x13,0x04, // LOAD_ATTR 'sleep'
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x01, // MAKE_FUNCTION_DEFARGS 1
    0x16,0x0b, // STORE_NAME 'wait_for'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x12, // STORE_NAME 'wait_for_ms'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x03, // MAKE_FUNCTION 3
    0x10,0x05, // LOAD_CONST_STRING '_Remove'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x05, // STORE_NAME '_Remove'
    0x53, // LOAD_NULL
    0x2c,0x00, // BUILD_MAP 0
    0x50, // LOAD_CONST_FALSE
    0x10,0x06, // LOAD_CONST_STRING 'return_exceptions'
    0x62, // STORE_MAP
    0x33,0x04, // MAKE_FUNCTION_DEFARGS 4
    0x16,0x14, // STORE_NAME 'gather'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_funcs__lt_module_gt_
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs__run
static const byte fun_data_asyncio_funcs__run[66] = {
    0xd2,0x42,0x06, // prelude
    0x07,0x1b,0x1c, // names: _run, waiter, aw
     // code info
    0x48,0x09, // SETUP_EXCEPT 9
    0xb1, // LOAD_FAST 1
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0xc2, // STORE_FAST 2
    0x52, // LOAD_CONST_TRUE
    0xc3, // STORE_FAST 3
    0x4a,0x16, // POP_EXCEPT_JUMP 22
    0x57, // DUP_TOP
    0x12,0x1d, // LOAD_GLOBAL 'BaseException'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x4f, // POP_JUMP_IF_FALSE 15
    0xc4, // STORE_FAST 4
    0x49,0x05, // SETUP_FINALLY 5
    0x51, // LOAD_CONST_NONE
    0xc2, // STORE_FAST 2
    0xb4, // LOAD_FAST 4
    0xc3, // STORE_FAST 3
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xc4, // STORE_FAST 4
    0x28,0x04, // DELETE_FAST 4
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'data'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x52, // POP_JUMP_IF_FALSE 18
    0xb0, // LOAD_FAST 0
    0x14,0x09, // LOAD_METHOD 'cancel'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x0a, // LOAD_METHOD 'CancelledError'
    0xb3, // LOAD_FAST 3
    0xb2, // LOAD_FAST 2
    0x36,0x02, // CALL_METHOD 2
    0xb0, // LOAD_FAST 0
    0x18,0x08, // STORE_ATTR 'data'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_funcs__run = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_funcs__run,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 66,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 2,
        .scope_flags = 1,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 7,
        .line_info = fun_data_asyncio_funcs__run + 6,
        .line_info_top = fun_data_asyncio_funcs__run + 6,
        .opcodes = fun_data_asyncio_funcs__run + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_funcs__run fun_data_asyncio_funcs__run[0]
#endif

// child of asyncio_funcs__lt_module_gt_
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs_wait_for
static const byte fun_data_asyncio_funcs_wait_for[119] = {
    0xdb,0x43,0x08, // prelude
    0x0b,0x1c,0x1e,0x04, // names: wait_for, aw, timeout, sleep
     // code info
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x0c, // LOAD_METHOD '_promote_to_task'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0xc0, // STORE_FAST 0
    0xb1, // LOAD_FAST 1
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x45, // POP_JUMP_IF_FALSE 5
    0xb0, // LOAD_FAST 0
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x63, // RETURN_VALUE
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x0d, // LOAD_METHOD 'create_task'
    0x12,0x07, // LOAD_GLOBAL '_run'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x0e, // LOAD_ATTR 'cur_task'
    0xb0, // LOAD_FAST 0
    0x34,0x02, // CALL_FUNCTION 2
    0x36,0x01, // CALL_METHOD 1
    0xc3, // STORE_FAST 3
    0x48,0x0a, // SETUP_EXCEPT 10
    0xb2, // LOAD_FAST 2
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x4a,0x32, // POP_EXCEPT_JUMP 50
    0x57, // DUP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x0a, // LOAD_ATTR 'CancelledError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x69, // POP_JUMP_IF_FALSE 41
    0xc4, // STORE_FAST 4
    0x49,0x1f, // SETUP_FINALLY 31
    0xb4, // LOAD_FAST 4
    0x13,0x0f, // LOAD_ATTR 'value'
    0xc5, // STORE_FAST 5
    0xb5, // LOAD_FAST 5
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb3, // LOAD_FAST 3
    0x14,0x09, // LOAD_METHOD 'cancel'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb4, // LOAD_FAST 4
    0x65, // RAISE_OBJ
    0xb5, // LOAD_FAST 5
    0x52, // LOAD_CONST_TRUE
    0xde, // BINARY_OP 7 <is>
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0xb4, // LOAD_FAST 4
    0x13,0x10, // LOAD_ATTR 'args'
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0x63, // RETURN_VALUE
    0xb5, // LOAD_FAST 5
    0x65, // RAISE_OBJ
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xc4, // STORE_FAST 4
    0x28,0x04, // DELETE_FAST 4
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0xb3, // LOAD_FAST 3
    0x14,0x09, // LOAD_METHOD 'cancel'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb3, // LOAD_FAST 3
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x11, // LOAD_ATTR 'TimeoutError'
    0x65, // RAISE_OBJ
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_funcs_wait_for = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_funcs_wait_for,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 119,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 2,
        .scope_flags = 1,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 11,
        .line_info = fun_data_asyncio_funcs_wait_for + 7,
        .line_info_top = fun_data_asyncio_funcs_wait_for + 7,
        .opcodes = fun_data_asyncio_funcs_wait_for + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_funcs_wait_for fun_data_asyncio_funcs_wait_for[0]
#endif

// child of asyncio_funcs__lt_module_gt_
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs_wait_for_ms
static const byte fun_data_asyncio_funcs_wait_for_ms[16] = {
    0x2a,0x06, // prelude
    0x12,0x1c,0x1e, // names: wait_for_ms, aw, timeout
     // code info
    0x12,0x0b, // LOAD_GLOBAL 'wait_for'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x13, // LOAD_ATTR 'sleep_ms'
    0x34,0x03, // CALL_FUNCTION 3
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_funcs_wait_for_ms = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_funcs_wait_for_ms,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 16,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 18,
        .line_info = fun_data_asyncio_funcs_wait_for_ms + 5,
        .line_info_top = fun_data_asyncio_funcs_wait_for_ms + 5,
        .opcodes = fun_data_asyncio_funcs_wait_for_ms + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_funcs_wait_for_ms fun_data_asyncio_funcs_wait_for_ms[0]
#endif

// child of asyncio_funcs__lt_module_gt_
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs__Remove
static const byte fun_data_asyncio_funcs__Remove[21] = {
    0x08,0x02, // prelude
    0x05, // names: _Remove
     // code info
    0x11,0x1f, // LOAD_NAME '__name__'
    0x16,0x20, // STORE_NAME '__module__'
    0x10,0x05, // LOAD_CONST_STRING '_Remove'
    0x16,0x21, // STORE_NAME '__qualname__'
    0x11,0x22, // LOAD_NAME 'staticmethod'
    0x32,0x00, // MAKE_FUNCTION 0
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x16, // STORE_NAME 'remove'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_funcs__Remove
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs__Remove_remove
static const byte fun_data_asyncio_funcs__Remove_remove[6] = {
    0x09,0x04, // prelude
    0x16,0x28, // names: remove, t
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_funcs__Remove_remove = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_funcs__Remove_remove,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 6,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 22,
        .line_info = fun_data_asyncio_funcs__Remove_remove + 4,
        .line_info_top = fun_data_asyncio_funcs__Remove_remove + 4,
        .opcodes = fun_data_asyncio_funcs__Remove_remove + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_funcs__Remove_remove fun_data_asyncio_funcs__Remove_remove[0]
#endif

static const mp_raw_code_t *const children_asyncio_funcs__Remove[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs__Remove_remove,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_funcs__Remove = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_funcs__Remove,
    .children = (void *)&children_asyncio_funcs__Remove,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 21,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 5,
        .line_info = fun_data_asyncio_funcs__Remove + 3,
        .line_info_top = fun_data_asyncio_funcs__Remove + 3,
        .opcodes = fun_data_asyncio_funcs__Remove + 3,
    },
    #endif
    #endif
};

// child of asyncio_funcs__lt_module_gt_
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs_gather
static const byte fun_data_asyncio_funcs_gather[281] = {
    0xf0,0xca,0x80,0xc0,0x40,0x85,0x01, // prelude
    0x14,0x06, // names: gather, return_exceptions
    0x00,0x07,0x08, // code info
    0xb0, // LOAD_FAST 0
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0x20,0x00,0x03, // MAKE_CLOSURE 0
    0xc2, // STORE_FAST 2
    0x32,0x01, // MAKE_FUNCTION 1
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xc3, // STORE_FAST 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0x27,0x07, // STORE_DEREF 7
    0x12,0x23, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x42,0xc4,0x80, // JUMP 68
    0x57, // DUP_TOP
    0xc4, // STORE_FAST 4
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x15, // LOAD_ATTR 'state'
    0x52, // LOAD_CONST_TRUE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x18,0x15, // STORE_ATTR 'state'
    0x25,0x07, // LOAD_DEREF 7
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x27,0x07, // STORE_DEREF 7
    0x42,0x69, // JUMP 41
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x15, // LOAD_ATTR 'state'
    0x43,0x5b, // POP_JUMP_IF_TRUE 27
    0x12,0x24, // LOAD_GLOBAL 'isinstance'
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x08, // LOAD_ATTR 'data'
    0x12,0x25, // LOAD_GLOBAL 'StopIteration'
    0x34,0x02, // CALL_FUNCTION 2
    0x43,0x4c, // POP_JUMP_IF_TRUE 12
    0x25,0x00, // LOAD_DEREF 0
    0x43,0x48, // POP_JUMP_IF_TRUE 8
    0x12,0x23, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0xd1, // UNARY_OP 1 __neg__
    0x27,0x07, // STORE_DEREF 7
    0x42,0x47, // JUMP 7
    0x12,0x26, // LOAD_GLOBAL 'RuntimeError'
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x58, // DUP_TOP_TWO
    0x5a, // ROT_TWO
    0xd7, // BINARY_OP 0 __lt__
    0x43,0xb6,0x7f, // POP_JUMP_IF_TRUE -74
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x0e, // LOAD_ATTR 'cur_task'
    0x27,0x08, // STORE_DEREF 8
    0x50, // LOAD_CONST_FALSE
    0xc5, // STORE_FAST 5
    0x25,0x07, // LOAD_DEREF 7
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x66, // POP_JUMP_IF_FALSE 38
    0x12,0x05, // LOAD_GLOBAL '_Remove'
    0x25,0x08, // LOAD_DEREF 8
    0x18,0x08, // STORE_ATTR 'data'
    0x48,0x05, // SETUP_EXCEPT 5
    0x51, // LOAD_CONST_NONE
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x4a,0x19, // POP_EXCEPT_JUMP 25
    0x57, // DUP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x0a, // LOAD_ATTR 'CancelledError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x50, // POP_JUMP_IF_FALSE 16
    0xc6, // STORE_FAST 6
    0x49,0x06, // SETUP_FINALLY 6
    0x52, // LOAD_CONST_TRUE
    0xc5, // STORE_FAST 5
    0xb6, // LOAD_FAST 6
    0x27,0x07, // STORE_DEREF 7
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xc6, // STORE_FAST 6
    0x28,0x06, // DELETE_FAST 6
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0x12,0x23, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x42,0xda,0x80, // JUMP 90
    0x57, // DUP_TOP
    0xc4, // STORE_FAST 4
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x15, // LOAD_ATTR 'state'
    0xb2, // LOAD_FAST 2
    0xde, // BINARY_OP 7 <is>
    0x44,0x53, // POP_JUMP_IF_FALSE 19
    0x52, // LOAD_CONST_TRUE
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x18,0x15, // STORE_ATTR 'state'
    0xb5, // LOAD_FAST 5
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x14,0x09, // LOAD_METHOD 'cancel'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x42,0x7a, // JUMP 58
    0x12,0x24, // LOAD_GLOBAL 'isinstance'
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x08, // LOAD_ATTR 'data'
    0x12,0x25, // LOAD_GLOBAL 'StopIteration'
    0x34,0x02, // CALL_FUNCTION 2
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x08, // LOAD_ATTR 'data'
    0x13,0x0f, // LOAD_ATTR 'value'
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x56, // STORE_SUBSCR
    0x42,0x61, // JUMP 33
    0x25,0x00, // LOAD_DEREF 0
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x08, // LOAD_ATTR 'data'
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x56, // STORE_SUBSCR
    0x42,0x53, // JUMP 19
    0x12,0x24, // LOAD_GLOBAL 'isinstance'
    0x25,0x07, // LOAD_DEREF 7
    0x12,0x27, // LOAD_GLOBAL 'int'
    0x34,0x02, // CALL_FUNCTION 2
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0x13,0x08, // LOAD_ATTR 'data'
    0x27,0x07, // STORE_DEREF 7
    0x42,0x40, // JUMP 0
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x58, // DUP_TOP_TWO
    0x5a, // ROT_TWO
    0xd7, // BINARY_OP 0 __lt__
    0x43,0xa0,0x7f, // POP_JUMP_IF_TRUE -96
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x25,0x07, // LOAD_DEREF 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x25,0x07, // LOAD_DEREF 7
    0x65, // RAISE_OBJ
    0xb3, // LOAD_FAST 3
    0x63, // RETURN_VALUE
};
// child of asyncio_funcs_gather
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs_gather_done
static const byte fun_data_asyncio_funcs_gather_done[64] = {
    0xb9,0x04,0x0c, // prelude
    0x17,0x29,0x29,0x29,0x28,0x2a, // names: done, *, *, *, t, er
     // code info
    0x25,0x02, // LOAD_DEREF 2
    0x13,0x08, // LOAD_ATTR 'data'
    0x12,0x05, // LOAD_GLOBAL '_Remove'
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0x25,0x00, // LOAD_DEREF 0
    0x43,0x4e, // POP_JUMP_IF_TRUE 14
    0x12,0x24, // LOAD_GLOBAL 'isinstance'
    0xb4, // LOAD_FAST 4
    0x12,0x25, // LOAD_GLOBAL 'StopIteration'
    0x34,0x02, // CALL_FUNCTION 2
    0x43,0x45, // POP_JUMP_IF_TRUE 5
    0xb4, // LOAD_FAST 4
    0x27,0x01, // STORE_DEREF 1
    0x42,0x4c, // JUMP 12
    0x25,0x01, // LOAD_DEREF 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe6, // BINARY_OP 15 __isub__
    0x27,0x01, // STORE_DEREF 1
    0x25,0x01, // LOAD_DEREF 1
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_task_queue'
    0x14,0x19, // LOAD_METHOD 'push'
    0x25,0x02, // LOAD_DEREF 2
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_funcs_gather_done = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_funcs_gather_done,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 64,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 23,
        .line_info = fun_data_asyncio_funcs_gather_done + 9,
        .line_info_top = fun_data_asyncio_funcs_gather_done + 9,
        .opcodes = fun_data_asyncio_funcs_gather_done + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_funcs_gather_done fun_data_asyncio_funcs_gather_done[0]
#endif

// child of asyncio_funcs_gather
// frozen bytecode for file asyncio/funcs.py, scope asyncio_funcs_gather__lt_listcomp_gt_
static const byte fun_data_asyncio_funcs_gather__lt_listcomp_gt_[23] = {
    0x49,0x04, // prelude
    0x1a,0x29, // names: <listcomp>, *
     // code info
    0x2b,0x00, // BUILD_LIST 0
    0xb0, // LOAD_FAST 0
    0x5f, // GET_ITER_STACK
    0x4b,0x0c, // FOR_ITER 12
    0xc1, // STORE_FAST 1
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x0c, // LOAD_METHOD '_promote_to_task'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x2f,0x14, // STORE_COMP 20
    0x42,0x32, // JUMP -14
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_funcs_gather__lt_listcomp_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_funcs_gather__lt_listcomp_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 23,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 26,
        .line_info = fun_data_asyncio_funcs_gather__lt_listcomp_gt_ + 4,
        .line_info_top = fun_data_asyncio_funcs_gather__lt_listcomp_gt_ + 4,
        .opcodes = fun_data_asyncio_funcs_gather__lt_listcomp_gt_ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_funcs_gather__lt_listcomp_gt_ fun_data_asyncio_funcs_gather__lt_listcomp_gt_[0]
#endif

static const mp_raw_code_t *const children_asyncio_funcs_gather[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs_gather_done,
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs_gather__lt_listcomp_gt_,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_funcs_gather = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_funcs_gather,
    .children = (void *)&children_asyncio_funcs_gather,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 281,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 15,
        .n_exc_stack = 2,
        .scope_flags = 13,
        .n_pos_args = 0,
        .n_kwonly_args = 1,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 20,
        .line_info = fun_data_asyncio_funcs_gather + 9,
        .line_info_top = fun_data_asyncio_funcs_gather + 9,
        .opcodes = fun_data_asyncio_funcs_gather + 12,
    },
    #endif
    #endif
};

static const mp_raw_code_t *const children_asyncio_funcs__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs__run,
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs_wait_for,
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs_wait_for_ms,
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs__Remove,
    (const mp_raw_code_t *)&proto_fun_asyncio_funcs_gather,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_funcs__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_funcs__lt_module_gt_,
    .children = (void *)&children_asyncio_funcs__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 56,
    .n_children = 5,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_asyncio_funcs__lt_module_gt_ + 3,
        .line_info_top = fun_data_asyncio_funcs__lt_module_gt_ + 3,
        .opcodes = fun_data_asyncio_funcs__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_asyncio_funcs[43] = {
    MP_QSTR_asyncio_slash_funcs_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_core,
    MP_QSTR_,
    MP_QSTR_sleep,
    MP_QSTR__Remove,
    MP_QSTR_return_exceptions,
    MP_QSTR__run,
    MP_QSTR_data,
    MP_QSTR_cancel,
    MP_QSTR_CancelledError,
    MP_QSTR_wait_for,
    MP_QSTR__promote_to_task,
    MP_QSTR_create_task,
    MP_QSTR_cur_task,
    MP_QSTR_value,
    MP_QSTR_args,
    MP_QSTR_TimeoutError,
    MP_QSTR_wait_for_ms,
    MP_QSTR_sleep_ms,
    MP_QSTR_gather,
    MP_QSTR_state,
    MP_QSTR_remove,
    MP_QSTR_done,
    MP_QSTR__task_queue,
    MP_QSTR_push,
    MP_QSTR__lt_listcomp_gt_,
    MP_QSTR_waiter,
    MP_QSTR_aw,
    MP_QSTR_BaseException,
    MP_QSTR_timeout,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_staticmethod,
    MP_QSTR_len,
    MP_QSTR_isinstance,
    MP_QSTR_StopIteration,
    MP_QSTR_RuntimeError,
    MP_QSTR_int,
    MP_QSTR_t,
    MP_QSTR__star_,
    MP_QSTR_er,
};

// constants

// constant table
static const mp_rom_obj_t const_obj_table_data_asyncio_funcs[1] = {
    MP_ROM_QSTR(MP_QSTR_can_squot_t_space_gather),
};

static const mp_frozen_module_t frozen_module_asyncio_funcs = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_asyncio_funcs,
        .obj_table = (mp_obj_t *)&const_obj_table_data_asyncio_funcs,
    },
    .proto_fun = &proto_fun_asyncio_funcs__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module asyncio_lock
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\asyncio\lock.mpy
// - frozen file name: asyncio/lock.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file asyncio/lock.py, scope asyncio_lock__lt_module_gt_
static const byte fun_data_asyncio_lock__lt_module_gt_[26] = {
    0x10,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x02, // LOAD_CONST_STRING 'core'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x03, // IMPORT_NAME ''
    0x1c,0x02, // IMPORT_FROM 'core'
    0x16,0x02, // STORE_NAME 'core'
    0x59, // POP_TOP
    0x54, // LOAD_BUILD_CLASS
    0x32,0x00, // MAKE_FUNCTION 0
    0x10,0x04, // LOAD_CONST_STRING 'Lock'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x04, // STORE_NAME 'Lock'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_lock__lt_module_gt_
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock
static const byte fun_data_asyncio_lock_Lock[37] = {
    0x00,0x02, // prelude
    0x04, // names: Lock
     // code info
    0x11,0x15, // LOAD_NAME '__name__'
    0x16,0x16, // STORE_NAME '__module__'
    0x10,0x04, // LOAD_CONST_STRING 'Lock'
    0x16,0x17, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x05, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x09, // STORE_NAME 'locked'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x0a, // STORE_NAME 'release'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x0f, // STORE_NAME 'acquire'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x13, // STORE_NAME '__aenter__'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x14, // STORE_NAME '__aexit__'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_lock_Lock
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock___init__
static const byte fun_data_asyncio_lock_Lock___init__[19] = {
    0x11,0x04, // prelude
    0x05,0x18, // names: __init__, self
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x18,0x06, // STORE_ATTR 'state'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x07, // LOAD_METHOD 'TaskQueue'
    0x36,0x00, // CALL_METHOD 0
    0xb0, // LOAD_FAST 0
    0x18,0x08, // STORE_ATTR 'waiting'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_lock_Lock___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 19,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 5,
        .line_info = fun_data_asyncio_lock_Lock___init__ + 4,
        .line_info_top = fun_data_asyncio_lock_Lock___init__ + 4,
        .opcodes = fun_data_asyncio_lock_Lock___init__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_lock_Lock___init__ fun_data_asyncio_lock_Lock___init__[0]
#endif

// child of asyncio_lock_Lock
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock_locked
static const byte fun_data_asyncio_lock_Lock_locked[10] = {
    0x11,0x04, // prelude
    0x09,0x18, // names: locked, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'state'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd9, // BINARY_OP 2 __eq__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock_locked = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_lock_Lock_locked,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 9,
        .line_info = fun_data_asyncio_lock_Lock_locked + 4,
        .line_info_top = fun_data_asyncio_lock_Lock_locked + 4,
        .opcodes = fun_data_asyncio_lock_Lock_locked + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_lock_Lock_locked fun_data_asyncio_lock_Lock_locked[0]
#endif

// child of asyncio_lock_Lock
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock_release
static const byte fun_data_asyncio_lock_Lock_release[57] = {
    0x19,0x04, // prelude
    0x0a,0x18, // names: release, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'state'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0x12,0x19, // LOAD_GLOBAL 'RuntimeError'
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'waiting'
    0x14,0x0b, // LOAD_METHOD 'peek'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'waiting'
    0x14,0x0c, // LOAD_METHOD 'pop'
    0x36,0x00, // CALL_METHOD 0
    0xb0, // LOAD_FAST 0
    0x18,0x06, // STORE_ATTR 'state'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x0d, // LOAD_ATTR '_task_queue'
    0x14,0x0e, // LOAD_METHOD 'push'
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'state'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x44, // JUMP 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x18,0x06, // STORE_ATTR 'state'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock_release = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_lock_Lock_release,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 57,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 10,
        .line_info = fun_data_asyncio_lock_Lock_release + 4,
        .line_info_top = fun_data_asyncio_lock_Lock_release + 4,
        .opcodes = fun_data_asyncio_lock_Lock_release + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_lock_Lock_release fun_data_asyncio_lock_Lock_release[0]
#endif

// child of asyncio_lock_Lock
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock_acquire
static const byte fun_data_asyncio_lock_Lock_acquire[88] = {
    0xb9,0x42,0x04, // prelude
    0x0f,0x18, // names: acquire, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'state'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xdc, // BINARY_OP 5 __ne__
    0x44,0xc5,0x80, // POP_JUMP_IF_FALSE 69
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'waiting'
    0x14,0x0e, // LOAD_METHOD 'push'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x10, // LOAD_ATTR 'cur_task'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'waiting'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x10, // LOAD_ATTR 'cur_task'
    0x18,0x11, // STORE_ATTR 'data'
    0x48,0x05, // SETUP_EXCEPT 5
    0x51, // LOAD_CONST_NONE
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x4a,0x29, // POP_EXCEPT_JUMP 41
    0x57, // DUP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x12, // LOAD_ATTR 'CancelledError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x60, // POP_JUMP_IF_FALSE 32
    0xc1, // STORE_FAST 1
    0x49,0x16, // SETUP_FINALLY 22
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'state'
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x10, // LOAD_ATTR 'cur_task'
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb0, // LOAD_FAST 0
    0x18,0x06, // STORE_ATTR 'state'
    0xb0, // LOAD_FAST 0
    0x14,0x0a, // LOAD_METHOD 'release'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb1, // LOAD_FAST 1
    0x65, // RAISE_OBJ
    0x51, // LOAD_CONST_NONE
    0xc1, // STORE_FAST 1
    0x28,0x01, // DELETE_FAST 1
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb0, // LOAD_FAST 0
    0x18,0x06, // STORE_ATTR 'state'
    0x52, // LOAD_CONST_TRUE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock_acquire = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_lock_Lock_acquire,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 88,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 2,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 15,
        .line_info = fun_data_asyncio_lock_Lock_acquire + 5,
        .line_info_top = fun_data_asyncio_lock_Lock_acquire + 5,
        .opcodes = fun_data_asyncio_lock_Lock_acquire + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_lock_Lock_acquire fun_data_asyncio_lock_Lock_acquire[0]
#endif

// child of asyncio_lock_Lock
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock___aenter__
static const byte fun_data_asyncio_lock_Lock___aenter__[14] = {
    0x91,0x40,0x04, // prelude
    0x13,0x18, // names: __aenter__, self
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x0f, // LOAD_METHOD 'acquire'
    0x36,0x00, // CALL_METHOD 0
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock___aenter__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_lock_Lock___aenter__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 19,
        .line_info = fun_data_asyncio_lock_Lock___aenter__ + 5,
        .line_info_top = fun_data_asyncio_lock_Lock___aenter__ + 5,
        .opcodes = fun_data_asyncio_lock_Lock___aenter__ + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_lock_Lock___aenter__ fun_data_asyncio_lock_Lock___aenter__[0]
#endif

// child of asyncio_lock_Lock
// frozen bytecode for file asyncio/lock.py, scope asyncio_lock_Lock___aexit__
static const byte fun_data_asyncio_lock_Lock___aexit__[14] = {
    0xa8,0x44,0x0a, // prelude
    0x14,0x18,0x1a,0x1b,0x1c, // names: __aexit__, self, exc_type, exc, tb
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x0a, // LOAD_METHOD 'release'
    0x36,0x00, // CALL_METHOD 0
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock___aexit__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_lock_Lock___aexit__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 20,
        .line_info = fun_data_asyncio_lock_Lock___aexit__ + 8,
        .line_info_top = fun_data_asyncio_lock_Lock___aexit__ + 8,
        .opcodes = fun_data_asyncio_lock_Lock___aexit__ + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_lock_Lock___aexit__ fun_data_asyncio_lock_Lock___aexit__[0]
#endif

static const mp_raw_code_t *const children_asyncio_lock_Lock[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock___init__,
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock_locked,
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock_release,
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock_acquire,
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock___aenter__,
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock___aexit__,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_lock_Lock = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_lock_Lock,
    .children = (void *)&children_asyncio_lock_Lock,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 37,
    .n_children = 6,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 4,
        .line_info = fun_data_asyncio_lock_Lock + 3,
        .line_info_top = fun_data_asyncio_lock_Lock + 3,
        .opcodes = fun_data_asyncio_lock_Lock + 3,
    },
    #endif
    #endif
};

static const mp_raw_code_t *const children_asyncio_lock__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_lock_Lock,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_lock__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_lock__lt_module_gt_,
    .children = (void *)&children_asyncio_lock__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_asyncio_lock__lt_module_gt_ + 3,
        .line_info_top = fun_data_asyncio_lock__lt_module_gt_ + 3,
        .opcodes = fun_data_asyncio_lock__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_asyncio_lock[29] = {
    MP_QSTR_asyncio_slash_lock_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_core,
    MP_QSTR_,
    MP_QSTR_Lock,
    MP_QSTR___init__,
    MP_QSTR_state,
    MP_QSTR_TaskQueue,
    MP_QSTR_waiting,
    MP_QSTR_locked,
    MP_QSTR_release,
    MP_QSTR_peek,
    MP_QSTR_pop,
    MP_QSTR__task_queue,
    MP_QSTR_push,
    MP_QSTR_acquire,
    MP_QSTR_cur_task,
    MP_QSTR_data,
    MP_QSTR_CancelledError,
    MP_QSTR___aenter__,
    MP_QSTR___aexit__,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_self,
    MP_QSTR_RuntimeError,
    MP_QSTR_exc_type,
    MP_QSTR_exc,
    MP_QSTR_tb,
};

// constants

// constant table
static const mp_rom_obj_t const_obj_table_data_asyncio_lock[1] = {
    MP_ROM_QSTR(MP_QSTR_Lock_space_not_space_acquired),
};

static const mp_frozen_module_t frozen_module_asyncio_lock = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_asyncio_lock,
        .obj_table = (mp_obj_t *)&const_obj_table_data_asyncio_lock,
    },
    .proto_fun = &proto_fun_asyncio_lock__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module asyncio_stream
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\asyncio\stream.mpy
// - frozen file name: asyncio/stream.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file asyncio/stream.py, scope asyncio_stream__lt_module_gt_
static const byte fun_data_asyncio_stream__lt_module_gt_[90] = {
    0x10,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x02, // LOAD_CONST_STRING 'core'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x03, // IMPORT_NAME ''
    0x1c,0x02, // IMPORT_FROM 'core'
    0x16,0x02, // STORE_NAME 'core'
    0x59, // POP_TOP
    0x54, // LOAD_BUILD_CLASS
    0x32,0x00, // MAKE_FUNCTION 0
    0x10,0x04, // LOAD_CONST_STRING 'Stream'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x04, // STORE_NAME 'Stream'
    0x11,0x04, // LOAD_NAME 'Stream'
    0x16,0x3c, // STORE_NAME 'StreamReader'
    0x11,0x04, // LOAD_NAME 'Stream'
    0x16,0x3d, // STORE_NAME 'StreamWriter'
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x01, // MAKE_FUNCTION_DEFARGS 1
    0x16,0x0a, // STORE_NAME 'open_connection'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x02, // MAKE_FUNCTION 2
    0x10,0x05, // LOAD_CONST_STRING 'Server'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x05, // STORE_NAME 'Server'
    0x85, // LOAD_CONST_SMALL_INT 5
    0x51, // LOAD_CONST_NONE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x03, // MAKE_FUNCTION_DEFARGS 3
    0x16,0x1a, // STORE_NAME 'start_server'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x04, // MAKE_FUNCTION_DEFARGS 4
    0x16,0x26, // STORE_NAME 'stream_awrite'
    0x11,0x04, // LOAD_NAME 'Stream'
    0x13,0x06, // LOAD_ATTR 'wait_closed'
    0x11,0x04, // LOAD_NAME 'Stream'
    0x18,0x07, // STORE_ATTR 'aclose'
    0x11,0x26, // LOAD_NAME 'stream_awrite'
    0x11,0x04, // LOAD_NAME 'Stream'
    0x18,0x08, // STORE_ATTR 'awrite'
    0x11,0x26, // LOAD_NAME 'stream_awrite'
    0x11,0x04, // LOAD_NAME 'Stream'
    0x18,0x09, // STORE_ATTR 'awritestr'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_stream__lt_module_gt_
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream
static const byte fun_data_asyncio_stream_Stream[62] = {
    0x08,0x02, // prelude
    0x04, // names: Stream
     // code info
    0x11,0x3e, // LOAD_NAME '__name__'
    0x16,0x3f, // STORE_NAME '__module__'
    0x10,0x04, // LOAD_CONST_STRING 'Stream'
    0x16,0x40, // STORE_NAME '__qualname__'
    0x2c,0x00, // BUILD_MAP 0
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x00, // MAKE_FUNCTION_DEFARGS 0
    0x16,0x29, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x2d, // STORE_NAME 'get_extra_info'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x2e, // STORE_NAME 'close'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x06, // STORE_NAME 'wait_closed'
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x04, // MAKE_FUNCTION_DEFARGS 4
    0x16,0x2f, // STORE_NAME 'read'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x31, // STORE_NAME 'readinto'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x32, // STORE_NAME 'readexactly'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x33, // STORE_NAME 'readline'
    0x32,0x08, // MAKE_FUNCTION 8
    0x16,0x27, // STORE_NAME 'write'
    0x32,0x09, // MAKE_FUNCTION 9
    0x16,0x28, // STORE_NAME 'drain'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream___init__
static const byte fun_data_asyncio_stream_Stream___init__[22] = {
    0xa3,0x01,0x08, // prelude
    0x29,0x46,0x2a,0x2b, // names: __init__, self, s, e
     // code info
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x18,0x2a, // STORE_ATTR 's'
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x2b, // STORE_ATTR 'e'
    0x23,0x00, // LOAD_CONST_OBJ 0
    0xb0, // LOAD_FAST 0
    0x18,0x2c, // STORE_ATTR 'out_buf'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Stream___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 22,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 41,
        .line_info = fun_data_asyncio_stream_Stream___init__ + 7,
        .line_info_top = fun_data_asyncio_stream_Stream___init__ + 7,
        .opcodes = fun_data_asyncio_stream_Stream___init__ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream___init__ fun_data_asyncio_stream_Stream___init__[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_get_extra_info
static const byte fun_data_asyncio_stream_Stream_get_extra_info[11] = {
    0x1a,0x06, // prelude
    0x2d,0x46,0x4c, // names: get_extra_info, self, v
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x2b, // LOAD_ATTR 'e'
    0xb1, // LOAD_FAST 1
    0x55, // LOAD_SUBSCR
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_get_extra_info = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Stream_get_extra_info,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 11,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 45,
        .line_info = fun_data_asyncio_stream_Stream_get_extra_info + 5,
        .line_info_top = fun_data_asyncio_stream_Stream_get_extra_info + 5,
        .opcodes = fun_data_asyncio_stream_Stream_get_extra_info + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_get_extra_info fun_data_asyncio_stream_Stream_get_extra_info[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_close
static const byte fun_data_asyncio_stream_Stream_close[6] = {
    0x09,0x04, // prelude
    0x2e,0x46, // names: close, self
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_close = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Stream_close,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 6,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 46,
        .line_info = fun_data_asyncio_stream_Stream_close + 4,
        .line_info_top = fun_data_asyncio_stream_Stream_close + 4,
        .opcodes = fun_data_asyncio_stream_Stream_close + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_close fun_data_asyncio_stream_Stream_close[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_wait_closed
static const byte fun_data_asyncio_stream_Stream_wait_closed[15] = {
    0x91,0x40,0x04, // prelude
    0x06,0x46, // names: wait_closed, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x2e, // LOAD_METHOD 'close'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_wait_closed = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Stream_wait_closed,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 15,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 6,
        .line_info = fun_data_asyncio_stream_Stream_wait_closed + 5,
        .line_info_top = fun_data_asyncio_stream_Stream_wait_closed + 5,
        .opcodes = fun_data_asyncio_stream_Stream_wait_closed + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_wait_closed fun_data_asyncio_stream_Stream_wait_closed[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_read
static const byte fun_data_asyncio_stream_Stream_read[61] = {
    0xb2,0x41,0x06, // prelude
    0x2f,0x46,0x4d, // names: read, self, n
     // code info
    0x23,0x00, // LOAD_CONST_OBJ 0
    0xc2, // STORE_FAST 2
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x30, // LOAD_METHOD 'queue_read'
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x2f, // LOAD_METHOD 'read'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0xc3, // STORE_FAST 3
    0xb3, // LOAD_FAST 3
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x54, // POP_JUMP_IF_FALSE 20
    0xb1, // LOAD_FAST 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xdb, // BINARY_OP 4 __ge__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0xb3, // LOAD_FAST 3
    0x63, // RETURN_VALUE
    0x12,0x4b, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x43,0x42, // POP_JUMP_IF_TRUE 2
    0xb2, // LOAD_FAST 2
    0x63, // RETURN_VALUE
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xe5, // BINARY_OP 14 __iadd__
    0xc2, // STORE_FAST 2
    0x42,0x0e, // JUMP -50
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_read = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Stream_read,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 61,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 47,
        .line_info = fun_data_asyncio_stream_Stream_read + 6,
        .line_info_top = fun_data_asyncio_stream_Stream_read + 6,
        .opcodes = fun_data_asyncio_stream_Stream_read + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_read fun_data_asyncio_stream_Stream_read[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_readinto
static const byte fun_data_asyncio_stream_Stream_readinto[28] = {
    0xa2,0x40,0x06, // prelude
    0x31,0x46,0x47, // names: readinto, self, buf
     // code info
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x30, // LOAD_METHOD 'queue_read'
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x31, // LOAD_METHOD 'readinto'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_readinto = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Stream_readinto,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 28,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 49,
        .line_info = fun_data_asyncio_stream_Stream_readinto + 6,
        .line_info_top = fun_data_asyncio_stream_Stream_readinto + 6,
        .opcodes = fun_data_asyncio_stream_Stream_readinto + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_readinto fun_data_asyncio_stream_Stream_readinto[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_readexactly
static const byte fun_data_asyncio_stream_Stream_readexactly[66] = {
    0xb2,0x40,0x06, // prelude
    0x32,0x46,0x4d, // names: readexactly, self, n
     // code info
    0x23,0x00, // LOAD_CONST_OBJ 0
    0xc2, // STORE_FAST 2
    0x42,0x72, // JUMP 50
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x30, // LOAD_METHOD 'queue_read'
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x2f, // LOAD_METHOD 'read'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0xc3, // STORE_FAST 3
    0xb3, // LOAD_FAST 3
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0x12,0x4b, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x43,0x43, // POP_JUMP_IF_TRUE 3
    0x12,0x4e, // LOAD_GLOBAL 'EOFError'
    0x65, // RAISE_OBJ
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xe5, // BINARY_OP 14 __iadd__
    0xc2, // STORE_FAST 2
    0xb1, // LOAD_FAST 1
    0x12,0x4b, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0xe6, // BINARY_OP 15 __isub__
    0xc1, // STORE_FAST 1
    0xb1, // LOAD_FAST 1
    0x43,0x0b, // POP_JUMP_IF_TRUE -53
    0xb2, // LOAD_FAST 2
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_readexactly = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Stream_readexactly,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 66,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 50,
        .line_info = fun_data_asyncio_stream_Stream_readexactly + 6,
        .line_info_top = fun_data_asyncio_stream_Stream_readexactly + 6,
        .opcodes = fun_data_asyncio_stream_Stream_readexactly + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_readexactly fun_data_asyncio_stream_Stream_readexactly[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_readline
static const byte fun_data_asyncio_stream_Stream_readline[56] = {
    0xa9,0x40,0x04, // prelude
    0x33,0x46, // names: readline, self
     // code info
    0x23,0x00, // LOAD_CONST_OBJ 0
    0xc1, // STORE_FAST 1
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x30, // LOAD_METHOD 'queue_read'
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x33, // LOAD_METHOD 'readline'
    0x36,0x00, // CALL_METHOD 0
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x42,0x50, // JUMP 16
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xe5, // BINARY_OP 14 __iadd__
    0xc1, // STORE_FAST 1
    0xb2, // LOAD_FAST 2
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0xb1, // LOAD_FAST 1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x55, // LOAD_SUBSCR
    0x8a, // LOAD_CONST_SMALL_INT 10
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0xb1, // LOAD_FAST 1
    0x63, // RETURN_VALUE
    0x42,0x12, // JUMP -46
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_readline = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Stream_readline,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 56,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 51,
        .line_info = fun_data_asyncio_stream_Stream_readline + 5,
        .line_info_top = fun_data_asyncio_stream_Stream_readline + 5,
        .opcodes = fun_data_asyncio_stream_Stream_readline + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_readline fun_data_asyncio_stream_Stream_readline[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_write
static const byte fun_data_asyncio_stream_Stream_write[54] = {
    0x2a,0x06, // prelude
    0x27,0x46,0x47, // names: write, self, buf
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x2c, // LOAD_ATTR 'out_buf'
    0x43,0x61, // POP_JUMP_IF_TRUE 33
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x27, // LOAD_METHOD 'write'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x12,0x4b, // LOAD_GLOBAL 'len'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb2, // LOAD_FAST 2
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x51, // LOAD_CONST_NONE
    0x2e,0x02, // BUILD_SLICE 2
    0x55, // LOAD_SUBSCR
    0xc1, // STORE_FAST 1
    0xb0, // LOAD_FAST 0
    0x57, // DUP_TOP
    0x13,0x2c, // LOAD_ATTR 'out_buf'
    0xb1, // LOAD_FAST 1
    0xe5, // BINARY_OP 14 __iadd__
    0x5a, // ROT_TWO
    0x18,0x2c, // STORE_ATTR 'out_buf'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_write = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Stream_write,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 54,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 39,
        .line_info = fun_data_asyncio_stream_Stream_write + 5,
        .line_info_top = fun_data_asyncio_stream_Stream_write + 5,
        .opcodes = fun_data_asyncio_stream_Stream_write + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_write fun_data_asyncio_stream_Stream_write[0]
#endif

// child of asyncio_stream_Stream
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Stream_drain
static const byte fun_data_asyncio_stream_Stream_drain[86] = {
    0xc1,0x40,0x04, // prelude
    0x28,0x46, // names: drain, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x2c, // LOAD_ATTR 'out_buf'
    0x43,0x4b, // POP_JUMP_IF_TRUE 11
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x23, // LOAD_METHOD 'sleep_ms'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x01, // CALL_METHOD 1
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x63, // RETURN_VALUE
    0x12,0x4a, // LOAD_GLOBAL 'memoryview'
    0xb0, // LOAD_FAST 0
    0x13,0x2c, // LOAD_ATTR 'out_buf'
    0x34,0x01, // CALL_FUNCTION 1
    0xc1, // STORE_FAST 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xc2, // STORE_FAST 2
    0x42,0x65, // JUMP 37
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x19, // LOAD_METHOD 'queue_write'
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x2a, // LOAD_ATTR 's'
    0x14,0x27, // LOAD_METHOD 'write'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x51, // LOAD_CONST_NONE
    0x2e,0x02, // BUILD_SLICE 2
    0x55, // LOAD_SUBSCR
    0x36,0x01, // CALL_METHOD 1
    0xc3, // STORE_FAST 3
    0xb3, // LOAD_FAST 3
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xe5, // BINARY_OP 14 __iadd__
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x12,0x4b, // LOAD_GLOBAL 'len'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x12, // POP_JUMP_IF_TRUE -46
    0x23,0x00, // LOAD_CONST_OBJ 0
    0xb0, // LOAD_FAST 0
    0x18,0x2c, // STORE_ATTR 'out_buf'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream_drain = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Stream_drain,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 86,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 40,
        .line_info = fun_data_asyncio_stream_Stream_drain + 5,
        .line_info_top = fun_data_asyncio_stream_Stream_drain + 5,
        .opcodes = fun_data_asyncio_stream_Stream_drain + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Stream_drain fun_data_asyncio_stream_Stream_drain[0]
#endif

static const mp_raw_code_t *const children_asyncio_stream_Stream[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream___init__,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_get_extra_info,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_close,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_wait_closed,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_read,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_readinto,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_readexactly,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_readline,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_write,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream_drain,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Stream = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Stream,
    .children = (void *)&children_asyncio_stream_Stream,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 62,
    .n_children = 10,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 4,
        .line_info = fun_data_asyncio_stream_Stream + 3,
        .line_info_top = fun_data_asyncio_stream_Stream + 3,
        .opcodes = fun_data_asyncio_stream_Stream + 3,
    },
    #endif
    #endif
};

// child of asyncio_stream__lt_module_gt_
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_open_connection
static const byte fun_data_asyncio_stream_open_connection[171] = {
    0x88,0xd6,0x01,0x0a, // prelude
    0x0a,0x41,0x42,0x12,0x16, // names: open_connection, host, port, ssl, server_hostname
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x0b, // LOAD_CONST_STRING 'EINPROGRESS'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x0c, // IMPORT_NAME 'errno'
    0x1c,0x0b, // IMPORT_FROM 'EINPROGRESS'
    0xc4, // STORE_FAST 4
    0x59, // POP_TOP
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x0d, // IMPORT_NAME 'socket'
    0xc5, // STORE_FAST 5
    0xb5, // LOAD_FAST 5
    0x14,0x0e, // LOAD_METHOD 'getaddrinfo'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb5, // LOAD_FAST 5
    0x13,0x0f, // LOAD_ATTR 'SOCK_STREAM'
    0x36,0x04, // CALL_METHOD 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0xc6, // STORE_FAST 6
    0xb5, // LOAD_FAST 5
    0x14,0x0d, // LOAD_METHOD 'socket'
    0xb6, // LOAD_FAST 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0xb6, // LOAD_FAST 6
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0xb6, // LOAD_FAST 6
    0x82, // LOAD_CONST_SMALL_INT 2
    0x55, // LOAD_SUBSCR
    0x36,0x03, // CALL_METHOD 3
    0xc7, // STORE_FAST 7
    0xb7, // LOAD_FAST 7
    0x14,0x10, // LOAD_METHOD 'setblocking'
    0x50, // LOAD_CONST_FALSE
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x48,0x0b, // SETUP_EXCEPT 11
    0xb7, // LOAD_FAST 7
    0x14,0x11, // LOAD_METHOD 'connect'
    0xb6, // LOAD_FAST 6
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x55, // LOAD_SUBSCR
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x4a,0x1b, // POP_EXCEPT_JUMP 27
    0x57, // DUP_TOP
    0x12,0x43, // LOAD_GLOBAL 'OSError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x54, // POP_JUMP_IF_FALSE 20
    0xc8, // STORE_FAST 8
    0x49,0x0a, // SETUP_FINALLY 10
    0xb8, // LOAD_FAST 8
    0x13,0x0c, // LOAD_ATTR 'errno'
    0xb4, // LOAD_FAST 4
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0xb8, // LOAD_FAST 8
    0x65, // RAISE_OBJ
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xc8, // STORE_FAST 8
    0x28,0x08, // DELETE_FAST 8
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0xb2, // LOAD_FAST 2
    0x44,0x6d, // POP_JUMP_IF_FALSE 45
    0xb2, // LOAD_FAST 2
    0x52, // LOAD_CONST_TRUE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x12, // IMPORT_NAME 'ssl'
    0xc9, // STORE_FAST 9
    0xb9, // LOAD_FAST 9
    0x14,0x13, // LOAD_METHOD 'SSLContext'
    0xb9, // LOAD_FAST 9
    0x13,0x14, // LOAD_ATTR 'PROTOCOL_TLS_CLIENT'
    0x36,0x01, // CALL_METHOD 1
    0xc2, // STORE_FAST 2
    0xb3, // LOAD_FAST 3
    0x43,0x42, // POP_JUMP_IF_TRUE 2
    0xb0, // LOAD_FAST 0
    0xc3, // STORE_FAST 3
    0xb2, // LOAD_FAST 2
    0x14,0x15, // LOAD_METHOD 'wrap_socket'
    0xb7, // LOAD_FAST 7
    0x10,0x16, // LOAD_CONST_STRING 'server_hostname'
    0xb3, // LOAD_FAST 3
    0x10,0x17, // LOAD_CONST_STRING 'do_handshake_on_connect'
    0x50, // LOAD_CONST_FALSE
    0x36,0x84,0x01, // CALL_METHOD 513
    0xc7, // STORE_FAST 7
    0xb7, // LOAD_FAST 7
    0x14,0x10, // LOAD_METHOD 'setblocking'
    0x50, // LOAD_CONST_FALSE
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x12,0x04, // LOAD_GLOBAL 'Stream'
    0xb7, // LOAD_FAST 7
    0x34,0x01, // CALL_FUNCTION 1
    0xca, // STORE_FAST 10
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x19, // LOAD_METHOD 'queue_write'
    0xb7, // LOAD_FAST 7
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0xba, // LOAD_FAST 10
    0xba, // LOAD_FAST 10
    0x2a,0x02, // BUILD_TUPLE 2
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_open_connection = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_open_connection,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 171,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 18,
        .n_exc_stack = 2,
        .scope_flags = 1,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 10,
        .line_info = fun_data_asyncio_stream_open_connection + 9,
        .line_info_top = fun_data_asyncio_stream_open_connection + 9,
        .opcodes = fun_data_asyncio_stream_open_connection + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_open_connection fun_data_asyncio_stream_open_connection[0]
#endif

// child of asyncio_stream__lt_module_gt_
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Server
static const byte fun_data_asyncio_stream_Server[33] = {
    0x00,0x02, // prelude
    0x05, // names: Server
     // code info
    0x11,0x3e, // LOAD_NAME '__name__'
    0x16,0x3f, // STORE_NAME '__module__'
    0x10,0x05, // LOAD_CONST_STRING 'Server'
    0x16,0x40, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x34, // STORE_NAME '__aenter__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x35, // STORE_NAME '__aexit__'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x2e, // STORE_NAME 'close'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x06, // STORE_NAME 'wait_closed'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x21, // STORE_NAME '_serve'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of asyncio_stream_Server
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Server___aenter__
static const byte fun_data_asyncio_stream_Server___aenter__[7] = {
    0x89,0x40,0x04, // prelude
    0x34,0x46, // names: __aenter__, self
     // code info
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Server___aenter__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Server___aenter__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 7,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 52,
        .line_info = fun_data_asyncio_stream_Server___aenter__ + 5,
        .line_info_top = fun_data_asyncio_stream_Server___aenter__ + 5,
        .opcodes = fun_data_asyncio_stream_Server___aenter__ + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Server___aenter__ fun_data_asyncio_stream_Server___aenter__[0]
#endif

// child of asyncio_stream_Server
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Server___aexit__
static const byte fun_data_asyncio_stream_Server___aexit__[25] = {
    0xa8,0x44,0x0a, // prelude
    0x35,0x46,0x4f,0x50,0x51, // names: __aexit__, self, exc_type, exc, tb
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x2e, // LOAD_METHOD 'close'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x06, // LOAD_METHOD 'wait_closed'
    0x36,0x00, // CALL_METHOD 0
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Server___aexit__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Server___aexit__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 25,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 53,
        .line_info = fun_data_asyncio_stream_Server___aexit__ + 8,
        .line_info_top = fun_data_asyncio_stream_Server___aexit__ + 8,
        .opcodes = fun_data_asyncio_stream_Server___aexit__ + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Server___aexit__ fun_data_asyncio_stream_Server___aexit__[0]
#endif

// child of asyncio_stream_Server
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Server_close
static const byte fun_data_asyncio_stream_Server_close[18] = {
    0x11,0x04, // prelude
    0x2e,0x46, // names: close, self
     // code info
    0x52, // LOAD_CONST_TRUE
    0xb0, // LOAD_FAST 0
    0x18,0x36, // STORE_ATTR 'state'
    0xb0, // LOAD_FAST 0
    0x13,0x22, // LOAD_ATTR 'task'
    0x14,0x25, // LOAD_METHOD 'cancel'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Server_close = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Server_close,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 18,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 46,
        .line_info = fun_data_asyncio_stream_Server_close + 4,
        .line_info_top = fun_data_asyncio_stream_Server_close + 4,
        .opcodes = fun_data_asyncio_stream_Server_close + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Server_close fun_data_asyncio_stream_Server_close[0]
#endif

// child of asyncio_stream_Server
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Server_wait_closed
static const byte fun_data_asyncio_stream_Server_wait_closed[14] = {
    0x91,0x40,0x04, // prelude
    0x06,0x46, // names: wait_closed, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x22, // LOAD_ATTR 'task'
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Server_wait_closed = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Server_wait_closed,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 6,
        .line_info = fun_data_asyncio_stream_Server_wait_closed + 5,
        .line_info_top = fun_data_asyncio_stream_Server_wait_closed + 5,
        .opcodes = fun_data_asyncio_stream_Server_wait_closed + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Server_wait_closed fun_data_asyncio_stream_Server_wait_closed[0]
#endif

// child of asyncio_stream_Server
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_Server__serve
static const byte fun_data_asyncio_stream_Server__serve[174] = {
    0xf8,0x46,0x0a, // prelude
    0x21,0x46,0x2a,0x44,0x12, // names: _serve, self, s, cb, ssl
     // code info
    0x50, // LOAD_CONST_FALSE
    0xb0, // LOAD_FAST 0
    0x18,0x36, // STORE_ATTR 'state'
    0x48,0x0d, // SETUP_EXCEPT 13
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x18, // LOAD_ATTR '_io_queue'
    0x14,0x30, // LOAD_METHOD 'queue_read'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x4a,0x23, // POP_EXCEPT_JUMP 35
    0x57, // DUP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x24, // LOAD_ATTR 'CancelledError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x5a, // POP_JUMP_IF_FALSE 26
    0xc4, // STORE_FAST 4
    0x49,0x10, // SETUP_FINALLY 16
    0xb1, // LOAD_FAST 1
    0x14,0x2e, // LOAD_METHOD 'close'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x36, // LOAD_ATTR 'state'
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb4, // LOAD_FAST 4
    0x65, // RAISE_OBJ
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0xc4, // STORE_FAST 4
    0x28,0x04, // DELETE_FAST 4
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0x48,0x0b, // SETUP_EXCEPT 11
    0xb1, // LOAD_FAST 1
    0x14,0x37, // LOAD_METHOD 'accept'
    0x36,0x00, // CALL_METHOD 0
    0x30,0x02, // UNPACK_SEQUENCE 2
    0xc5, // STORE_FAST 5
    0xc6, // STORE_FAST 6
    0x4a,0x06, // POP_EXCEPT_JUMP 6
    0x59, // POP_TOP
    0x40,0xda,0x80,0x01, // UNWIND_JUMP 90
    0x5d, // END_FINALLY
    0xb3, // LOAD_FAST 3
    0x44,0x76, // POP_JUMP_IF_FALSE 54
    0x48,0x10, // SETUP_EXCEPT 16
    0xb3, // LOAD_FAST 3
    0x14,0x15, // LOAD_METHOD 'wrap_socket'
    0xb5, // LOAD_FAST 5
    0x10,0x38, // LOAD_CONST_STRING 'server_side'
    0x52, // LOAD_CONST_TRUE
    0x10,0x17, // LOAD_CONST_STRING 'do_handshake_on_connect'
    0x50, // LOAD_CONST_FALSE
    0x36,0x84,0x01, // CALL_METHOD 513
    0xc5, // STORE_FAST 5
    0x4a,0x24, // POP_EXCEPT_JUMP 36
    0x57, // DUP_TOP
    0x12,0x43, // LOAD_GLOBAL 'OSError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x5d, // POP_JUMP_IF_FALSE 29
    0xc7, // STORE_FAST 7
    0x49,0x13, // SETUP_FINALLY 19
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x39, // LOAD_ATTR 'sys'
    0x14,0x3a, // LOAD_METHOD 'print_exception'
    0xb7, // LOAD_FAST 7
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb5, // LOAD_FAST 5
    0x14,0x2e, // LOAD_METHOD 'close'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x40,0x68,0x02, // UNWIND_JUMP 40
    0x51, // LOAD_CONST_NONE
    0xc7, // STORE_FAST 7
    0x28,0x07, // DELETE_FAST 7
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0xb5, // LOAD_FAST 5
    0x14,0x10, // LOAD_METHOD 'setblocking'
    0x50, // LOAD_CONST_FALSE
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x12,0x04, // LOAD_GLOBAL 'Stream'
    0xb5, // LOAD_FAST 5
    0x2c,0x01, // BUILD_MAP 1
    0xb6, // LOAD_FAST 6
    0x10,0x3b, // LOAD_CONST_STRING 'peername'
    0x62, // STORE_MAP
    0x34,0x02, // CALL_FUNCTION 2
    0xc8, // STORE_FAST 8
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x20, // LOAD_METHOD 'create_task'
    0xb2, // LOAD_FAST 2
    0xb8, // LOAD_FAST 8
    0xb8, // LOAD_FAST 8
    0x34,0x02, // CALL_FUNCTION 2
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0xe0,0x7e, // JUMP -160
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Server__serve = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_Server__serve,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 174,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 16,
        .n_exc_stack = 2,
        .scope_flags = 1,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 33,
        .line_info = fun_data_asyncio_stream_Server__serve + 8,
        .line_info_top = fun_data_asyncio_stream_Server__serve + 8,
        .opcodes = fun_data_asyncio_stream_Server__serve + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_Server__serve fun_data_asyncio_stream_Server__serve[0]
#endif

static const mp_raw_code_t *const children_asyncio_stream_Server[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Server___aenter__,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Server___aexit__,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Server_close,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Server_wait_closed,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Server__serve,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_stream_Server = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream_Server,
    .children = (void *)&children_asyncio_stream_Server,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 33,
    .n_children = 5,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 5,
        .line_info = fun_data_asyncio_stream_Server + 3,
        .line_info_top = fun_data_asyncio_stream_Server + 3,
        .opcodes = fun_data_asyncio_stream_Server + 3,
    },
    #endif
    #endif
};

// child of asyncio_stream__lt_module_gt_
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_start_server
static const byte fun_data_asyncio_stream_start_server[138] = {
    0x81,0xd6,0x01,0x0c, // prelude
    0x1a,0x44,0x41,0x42,0x45,0x12, // names: start_server, cb, host, port, backlog, ssl
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x0d, // IMPORT_NAME 'socket'
    0xc5, // STORE_FAST 5
    0xb5, // LOAD_FAST 5
    0x14,0x0e, // LOAD_METHOD 'getaddrinfo'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x36,0x02, // CALL_METHOD 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0xc6, // STORE_FAST 6
    0xb5, // LOAD_FAST 5
    0x14,0x0d, // LOAD_METHOD 'socket'
    0xb6, // LOAD_FAST 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x36,0x01, // CALL_METHOD 1
    0xc7, // STORE_FAST 7
    0xb7, // LOAD_FAST 7
    0x14,0x10, // LOAD_METHOD 'setblocking'
    0x50, // LOAD_CONST_FALSE
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb7, // LOAD_FAST 7
    0x14,0x1b, // LOAD_METHOD 'setsockopt'
    0xb5, // LOAD_FAST 5
    0x13,0x1c, // LOAD_ATTR 'SOL_SOCKET'
    0xb5, // LOAD_FAST 5
    0x13,0x1d, // LOAD_ATTR 'SO_REUSEADDR'
    0x81, // LOAD_CONST_SMALL_INT 1
    0x36,0x03, // CALL_METHOD 3
    0x59, // POP_TOP
    0xb7, // LOAD_FAST 7
    0x14,0x1e, // LOAD_METHOD 'bind'
    0xb6, // LOAD_FAST 6
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x55, // LOAD_SUBSCR
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb7, // LOAD_FAST 7
    0x14,0x1f, // LOAD_METHOD 'listen'
    0xb3, // LOAD_FAST 3
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x12,0x05, // LOAD_GLOBAL 'Server'
    0x34,0x00, // CALL_FUNCTION 0
    0xc8, // STORE_FAST 8
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x20, // LOAD_METHOD 'create_task'
    0xb8, // LOAD_FAST 8
    0x14,0x21, // LOAD_METHOD '_serve'
    0xb7, // LOAD_FAST 7
    0xb0, // LOAD_FAST 0
    0xb4, // LOAD_FAST 4
    0x36,0x03, // CALL_METHOD 3
    0x36,0x01, // CALL_METHOD 1
    0xb8, // LOAD_FAST 8
    0x18,0x22, // STORE_ATTR 'task'
    0x48,0x0d, // SETUP_EXCEPT 13
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x14,0x23, // LOAD_METHOD 'sleep_ms'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x01, // CALL_METHOD 1
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x4a,0x1d, // POP_EXCEPT_JUMP 29
    0x57, // DUP_TOP
    0x12,0x02, // LOAD_GLOBAL 'core'
    0x13,0x24, // LOAD_ATTR 'CancelledError'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x54, // POP_JUMP_IF_FALSE 20
    0xc9, // STORE_FAST 9
    0x49,0x0a, // SETUP_FINALLY 10
    0xb8, // LOAD_FAST 8
    0x13,0x22, // LOAD_ATTR 'task'
    0x14,0x25, // LOAD_METHOD 'cancel'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb9, // LOAD_FAST 9
    0x65, // RAISE_OBJ
    0x51, // LOAD_CONST_NONE
    0xc9, // STORE_FAST 9
    0x28,0x09, // DELETE_FAST 9
    0x5d, // END_FINALLY
    0x4a,0x01, // POP_EXCEPT_JUMP 1
    0x5d, // END_FINALLY
    0xb8, // LOAD_FAST 8
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_start_server = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_start_server,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 138,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 2,
        .scope_flags = 1,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 26,
        .line_info = fun_data_asyncio_stream_start_server + 10,
        .line_info_top = fun_data_asyncio_stream_start_server + 10,
        .opcodes = fun_data_asyncio_stream_start_server + 10,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_start_server fun_data_asyncio_stream_start_server[0]
#endif

// child of asyncio_stream__lt_module_gt_
// frozen bytecode for file asyncio/stream.py, scope asyncio_stream_stream_awrite
static const byte fun_data_asyncio_stream_stream_awrite[63] = {
    0xb8,0xc4,0x01,0x0a, // prelude
    0x26,0x46,0x47,0x48,0x49, // names: stream_awrite, self, buf, off, sz
     // code info
    0xb2, // LOAD_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xdc, // BINARY_OP 5 __ne__
    0x43,0x45, // POP_JUMP_IF_TRUE 5
    0xb3, // LOAD_FAST 3
    0x7f, // LOAD_CONST_SMALL_INT -1
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x5a, // POP_JUMP_IF_FALSE 26
    0x12,0x4a, // LOAD_GLOBAL 'memoryview'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xc1, // STORE_FAST 1
    0xb3, // LOAD_FAST 3
    0x7f, // LOAD_CONST_SMALL_INT -1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0x12,0x4b, // LOAD_GLOBAL 'len'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xc3, // STORE_FAST 3
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xf2, // BINARY_OP 27 __add__
    0x2e,0x02, // BUILD_SLICE 2
    0x55, // LOAD_SUBSCR
    0xc1, // STORE_FAST 1
    0xb0, // LOAD_FAST 0
    0x14,0x27, // LOAD_METHOD 'write'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x28, // LOAD_METHOD 'drain'
    0x36,0x00, // CALL_METHOD 0
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_asyncio_stream_stream_awrite = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_asyncio_stream_stream_awrite,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 63,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 38,
        .line_info = fun_data_asyncio_stream_stream_awrite + 9,
        .line_info_top = fun_data_asyncio_stream_stream_awrite + 9,
        .opcodes = fun_data_asyncio_stream_stream_awrite + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_asyncio_stream_stream_awrite fun_data_asyncio_stream_stream_awrite[0]
#endif

static const mp_raw_code_t *const children_asyncio_stream__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Stream,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_open_connection,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_Server,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_start_server,
    (const mp_raw_code_t *)&proto_fun_asyncio_stream_stream_awrite,
};

static const mp_raw_code_truncated_t proto_fun_asyncio_stream__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_asyncio_stream__lt_module_gt_,
    .children = (void *)&children_asyncio_stream__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 90,
    .n_children = 5,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_asyncio_stream__lt_module_gt_ + 3,
        .line_info_top = fun_data_asyncio_stream__lt_module_gt_ + 3,
        .opcodes = fun_data_asyncio_stream__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_asyncio_stream[82] = {
    MP_QSTR_asyncio_slash_stream_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_core,
    MP_QSTR_,
    MP_QSTR_Stream,
    MP_QSTR_Server,
    MP_QSTR_wait_closed,
    MP_QSTR_aclose,
    MP_QSTR_awrite,
    MP_QSTR_awritestr,
    MP_QSTR_open_connection,
    MP_QSTR_EINPROGRESS,
    MP_QSTR_errno,
    MP_QSTR_socket,
    MP_QSTR_getaddrinfo,
    MP_QSTR_SOCK_STREAM,
    MP_QSTR_setblocking,
    MP_QSTR_connect,
    MP_QSTR_ssl,
    MP_QSTR_SSLContext,
    MP_QSTR_PROTOCOL_TLS_CLIENT,
    MP_QSTR_wrap_socket,
    MP_QSTR_server_hostname,
    MP_QSTR_do_handshake_on_connect,
    MP_QSTR__io_queue,
    MP_QSTR_queue_write,
    MP_QSTR_start_server,
    MP_QSTR_setsockopt,
    MP_QSTR_SOL_SOCKET,
    MP_QSTR_SO_REUSEADDR,
    MP_QSTR_bind,
    MP_QSTR_listen,
    MP_QSTR_create_task,
    MP_QSTR__serve,
    MP_QSTR_task,
    MP_QSTR_sleep_ms,
    MP_QSTR_CancelledError,
    MP_QSTR_cancel,
    MP_QSTR_stream_awrite,
    MP_QSTR_write,
    MP_QSTR_drain,
    MP_QSTR___init__,
    MP_QSTR_s,
    MP_QSTR_e,
    MP_QSTR_out_buf,
    MP_QSTR_get_extra_info,
    MP_QSTR_close,
    MP_QSTR_read,
    MP_QSTR_queue_read,
    MP_QSTR_readinto,
    MP_QSTR_readexactly,
    MP_QSTR_readline,
    MP_QSTR___aenter__,
    MP_QSTR___aexit__,
    MP_QSTR_state,
    MP_QSTR_accept,
    MP_QSTR_server_side,
    MP_QSTR_sys,
    MP_QSTR_print_exception,
    MP_QSTR_peername,
    MP_QSTR_StreamReader,
    MP_QSTR_StreamWriter,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_host,
    MP_QSTR_port,
    MP_QSTR_OSError,
    MP_QSTR_cb,
    MP_QSTR_backlog,
    MP_QSTR_self,
    MP_QSTR_buf,
    MP_QSTR_off,
    MP_QSTR_sz,
    MP_QSTR_memoryview,
    MP_QSTR_len,
    MP_QSTR_v,
    MP_QSTR_n,
    MP_QSTR_EOFError,
    MP_QSTR_exc_type,
    MP_QSTR_exc,
    MP_QSTR_tb,
};

// constants

// constant table
static const mp_rom_obj_t const_obj_table_data_asyncio_stream[1] = {
    MP_ROM_PTR(&mp_const_empty_bytes_obj),
};

static const mp_frozen_module_t frozen_module_asyncio_stream = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_asyncio_stream,
        .obj_table = (mp_obj_t *)&const_obj_table_data_asyncio_stream,
    },
    .proto_fun = &proto_fun_asyncio_stream__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module neopixel
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\neopixel.mpy
// - frozen file name: neopixel.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file neopixel.py, scope neopixel__lt_module_gt_
static const byte fun_data_neopixel__lt_module_gt_[26] = {
    0x10,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x02, // LOAD_CONST_STRING 'bitstream'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x03, // IMPORT_NAME 'machine'
    0x1c,0x02, // IMPORT_FROM 'bitstream'
    0x16,0x02, // STORE_NAME 'bitstream'
    0x59, // POP_TOP
    0x54, // LOAD_BUILD_CLASS
    0x32,0x00, // MAKE_FUNCTION 0
    0x10,0x04, // LOAD_CONST_STRING 'NeoPixel'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x04, // STORE_NAME 'NeoPixel'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of neopixel__lt_module_gt_
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel
static const byte fun_data_neopixel_NeoPixel[46] = {
    0x08,0x02, // prelude
    0x04, // names: NeoPixel
     // code info
    0x11,0x14, // LOAD_NAME '__name__'
    0x16,0x15, // STORE_NAME '__module__'
    0x10,0x04, // LOAD_CONST_STRING 'NeoPixel'
    0x16,0x16, // STORE_NAME '__qualname__'
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x16,0x0f, // STORE_NAME 'ORDER'
    0x83, // LOAD_CONST_SMALL_INT 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x00, // MAKE_FUNCTION_DEFARGS 0
    0x16,0x05, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x0d, // STORE_NAME '__len__'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x0e, // STORE_NAME '__setitem__'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x10, // STORE_NAME '__getitem__'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x11, // STORE_NAME 'fill'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x12, // STORE_NAME 'write'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of neopixel_NeoPixel
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel___init__
static const byte fun_data_neopixel_NeoPixel___init__[72] = {
    0xb9,0x84,0x01,0x0c, // prelude
    0x05,0x17,0x06,0x07,0x08,0x0c, // names: __init__, self, pin, n, bpp, timing
     // code info
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x18,0x06, // STORE_ATTR 'pin'
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x07, // STORE_ATTR 'n'
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x18,0x08, // STORE_ATTR 'bpp'
    0x12,0x18, // LOAD_GLOBAL 'bytearray'
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xf4, // BINARY_OP 29 __mul__
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x09, // STORE_ATTR 'buf'
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'pin'
    0x14,0x0a, // LOAD_METHOD 'init'
    0xb1, // LOAD_FAST 1
    0x13,0x0b, // LOAD_ATTR 'OUT'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x12,0x19, // LOAD_GLOBAL 'isinstance'
    0xb4, // LOAD_FAST 4
    0x12,0x1a, // LOAD_GLOBAL 'int'
    0x34,0x02, // CALL_FUNCTION 2
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0xb4, // LOAD_FAST 4
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x42,0x42, // JUMP 2
    0x23,0x02, // LOAD_CONST_OBJ 2
    0xb0, // LOAD_FAST 0
    0x18,0x0c, // STORE_ATTR 'timing'
    0x42,0x44, // JUMP 4
    0xb4, // LOAD_FAST 4
    0xb0, // LOAD_FAST 0
    0x18,0x0c, // STORE_ATTR 'timing'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 72,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 5,
        .line_info = fun_data_neopixel_NeoPixel___init__ + 10,
        .line_info_top = fun_data_neopixel_NeoPixel___init__ + 10,
        .opcodes = fun_data_neopixel_NeoPixel___init__ + 10,
    },
    #endif
    #endif
};
#else
#define proto_fun_neopixel_NeoPixel___init__ fun_data_neopixel_NeoPixel___init__[0]
#endif

// child of neopixel_NeoPixel
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel___len__
static const byte fun_data_neopixel_NeoPixel___len__[8] = {
    0x09,0x04, // prelude
    0x0d,0x17, // names: __len__, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x07, // LOAD_ATTR 'n'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel___len__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel___len__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 8,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 13,
        .line_info = fun_data_neopixel_NeoPixel___len__ + 4,
        .line_info_top = fun_data_neopixel_NeoPixel___len__ + 4,
        .opcodes = fun_data_neopixel_NeoPixel___len__ + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_neopixel_NeoPixel___len__ fun_data_neopixel_NeoPixel___len__[0]
#endif

// child of neopixel_NeoPixel
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel___setitem__
static const byte fun_data_neopixel_NeoPixel___setitem__[45] = {
    0x5b,0x08, // prelude
    0x0e,0x17,0x1b,0x1c, // names: __setitem__, self, i, v
     // code info
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'bpp'
    0xf4, // BINARY_OP 29 __mul__
    0xc3, // STORE_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'bpp'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x42,0x52, // JUMP 18
    0x57, // DUP_TOP
    0xc4, // STORE_FAST 4
    0xb2, // LOAD_FAST 2
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'buf'
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x0f, // LOAD_ATTR 'ORDER'
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0xf2, // BINARY_OP 27 __add__
    0x56, // STORE_SUBSCR
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x58, // DUP_TOP_TWO
    0x5a, // ROT_TWO
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x29, // POP_JUMP_IF_TRUE -23
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel___setitem__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel___setitem__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 45,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 14,
        .line_info = fun_data_neopixel_NeoPixel___setitem__ + 6,
        .line_info_top = fun_data_neopixel_NeoPixel___setitem__ + 6,
        .opcodes = fun_data_neopixel_NeoPixel___setitem__ + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_neopixel_NeoPixel___setitem__ fun_data_neopixel_NeoPixel___setitem__[0]
#endif

// child of neopixel_NeoPixel
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel___getitem__
static const byte fun_data_neopixel_NeoPixel___getitem__[37] = {
    0x32,0x86,0x01, // prelude
    0x10,0x17,0x1b, // names: __getitem__, self, i
    0x00,0x02, // code info
    0xb1, // LOAD_FAST 1
    0x25,0x00, // LOAD_DEREF 0
    0x13,0x08, // LOAD_ATTR 'bpp'
    0xf4, // BINARY_OP 29 __mul__
    0x27,0x02, // STORE_DEREF 2
    0x12,0x1d, // LOAD_GLOBAL 'tuple'
    0xb0, // LOAD_FAST 0
    0xb2, // LOAD_FAST 2
    0x20,0x00,0x02, // MAKE_CLOSURE 0
    0x12,0x1e, // LOAD_GLOBAL 'range'
    0x25,0x00, // LOAD_DEREF 0
    0x13,0x08, // LOAD_ATTR 'bpp'
    0x34,0x01, // CALL_FUNCTION 1
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
// child of neopixel_NeoPixel___getitem__
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel___getitem____lt_genexpr_gt_
static const byte fun_data_neopixel_NeoPixel___getitem____lt_genexpr_gt_[34] = {
    0xdb,0x40,0x08, // prelude
    0x13,0x20,0x20,0x20, // names: <genexpr>, *, *, *
     // code info
    0x53, // LOAD_NULL
    0xb2, // LOAD_FAST 2
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x13, // FOR_ITER 19
    0xc3, // STORE_FAST 3
    0x25,0x00, // LOAD_DEREF 0
    0x13,0x09, // LOAD_ATTR 'buf'
    0x25,0x01, // LOAD_DEREF 1
    0x25,0x00, // LOAD_DEREF 0
    0x13,0x0f, // LOAD_ATTR 'ORDER'
    0xb3, // LOAD_FAST 3
    0x55, // LOAD_SUBSCR
    0xf2, // BINARY_OP 27 __add__
    0x55, // LOAD_SUBSCR
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x2b, // JUMP -21
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel___getitem____lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_neopixel_NeoPixel___getitem____lt_genexpr_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 34,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 19,
        .line_info = fun_data_neopixel_NeoPixel___getitem____lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_neopixel_NeoPixel___getitem____lt_genexpr_gt_ + 7,
        .opcodes = fun_data_neopixel_NeoPixel___getitem____lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_neopixel_NeoPixel___getitem____lt_genexpr_gt_ fun_data_neopixel_NeoPixel___getitem____lt_genexpr_gt_[0]
#endif

static const mp_raw_code_t *const children_neopixel_NeoPixel___getitem__[] = {
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel___getitem____lt_genexpr_gt_,
};

static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel___getitem__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel___getitem__,
    .children = (void *)&children_neopixel_NeoPixel___getitem__,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 37,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 16,
        .line_info = fun_data_neopixel_NeoPixel___getitem__ + 6,
        .line_info_top = fun_data_neopixel_NeoPixel___getitem__ + 6,
        .opcodes = fun_data_neopixel_NeoPixel___getitem__ + 8,
    },
    #endif
    #endif
};

// child of neopixel_NeoPixel
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel_fill
static const byte fun_data_neopixel_NeoPixel_fill[61] = {
    0x7a,0x06, // prelude
    0x11,0x17,0x1c, // names: fill, self, v
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'buf'
    0xc2, // STORE_FAST 2
    0x12,0x1f, // LOAD_GLOBAL 'len'
    0xb2, // LOAD_FAST 2
    0x34,0x01, // CALL_FUNCTION 1
    0xc3, // STORE_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'bpp'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x42,0x5f, // JUMP 31
    0x57, // DUP_TOP
    0xc4, // STORE_FAST 4
    0xb1, // LOAD_FAST 1
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0xc5, // STORE_FAST 5
    0x12,0x1e, // LOAD_GLOBAL 'range'
    0xb0, // LOAD_FAST 0
    0x13,0x0f, // LOAD_ATTR 'ORDER'
    0xb4, // LOAD_FAST 4
    0x55, // LOAD_SUBSCR
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x13,0x08, // LOAD_ATTR 'bpp'
    0x34,0x03, // CALL_FUNCTION 3
    0x5f, // GET_ITER_STACK
    0x4b,0x07, // FOR_ITER 7
    0xc6, // STORE_FAST 6
    0xb5, // LOAD_FAST 5
    0xb2, // LOAD_FAST 2
    0xb6, // LOAD_FAST 6
    0x56, // STORE_SUBSCR
    0x42,0x37, // JUMP -9
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x58, // DUP_TOP_TWO
    0x5a, // ROT_TWO
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x1c, // POP_JUMP_IF_TRUE -36
    0x59, // POP_TOP
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel_fill = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel_fill,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 61,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 16,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 17,
        .line_info = fun_data_neopixel_NeoPixel_fill + 5,
        .line_info_top = fun_data_neopixel_NeoPixel_fill + 5,
        .opcodes = fun_data_neopixel_NeoPixel_fill + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_neopixel_NeoPixel_fill fun_data_neopixel_NeoPixel_fill[0]
#endif

// child of neopixel_NeoPixel
// frozen bytecode for file neopixel.py, scope neopixel_NeoPixel_write
static const byte fun_data_neopixel_NeoPixel_write[21] = {
    0x29,0x04, // prelude
    0x12,0x17, // names: write, self
     // code info
    0x12,0x02, // LOAD_GLOBAL 'bitstream'
    0xb0, // LOAD_FAST 0
    0x13,0x06, // LOAD_ATTR 'pin'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x13,0x0c, // LOAD_ATTR 'timing'
    0xb0, // LOAD_FAST 0
    0x13,0x09, // LOAD_ATTR 'buf'
    0x34,0x04, // CALL_FUNCTION 4
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel_write = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel_write,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 21,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 18,
        .line_info = fun_data_neopixel_NeoPixel_write + 4,
        .line_info_top = fun_data_neopixel_NeoPixel_write + 4,
        .opcodes = fun_data_neopixel_NeoPixel_write + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_neopixel_NeoPixel_write fun_data_neopixel_NeoPixel_write[0]
#endif

static const mp_raw_code_t *const children_neopixel_NeoPixel[] = {
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel___init__,
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel___len__,
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel___setitem__,
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel___getitem__,
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel_fill,
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel_write,
};

static const mp_raw_code_truncated_t proto_fun_neopixel_NeoPixel = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel_NeoPixel,
    .children = (void *)&children_neopixel_NeoPixel,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 46,
    .n_children = 6,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 4,
        .line_info = fun_data_neopixel_NeoPixel + 3,
        .line_info_top = fun_data_neopixel_NeoPixel + 3,
        .opcodes = fun_data_neopixel_NeoPixel + 3,
    },
    #endif
    #endif
};

static const mp_raw_code_t *const children_neopixel__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_neopixel_NeoPixel,
};

static const mp_raw_code_truncated_t proto_fun_neopixel__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_neopixel__lt_module_gt_,
    .children = (void *)&children_neopixel__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_neopixel__lt_module_gt_ + 3,
        .line_info_top = fun_data_neopixel__lt_module_gt_ + 3,
        .opcodes = fun_data_neopixel__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_neopixel[33] = {
    MP_QSTR_neopixel_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_bitstream,
    MP_QSTR_machine,
    MP_QSTR_NeoPixel,
    MP_QSTR___init__,
    MP_QSTR_pin,
    MP_QSTR_n,
    MP_QSTR_bpp,
    MP_QSTR_buf,
    MP_QSTR_init,
    MP_QSTR_OUT,
    MP_QSTR_timing,
    MP_QSTR___len__,
    MP_QSTR___setitem__,
    MP_QSTR_ORDER,
    MP_QSTR___getitem__,
    MP_QSTR_fill,
    MP_QSTR_write,
    MP_QSTR__lt_genexpr_gt_,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_self,
    MP_QSTR_bytearray,
    MP_QSTR_isinstance,
    MP_QSTR_int,
    MP_QSTR_i,
    MP_QSTR_v,
    MP_QSTR_tuple,
    MP_QSTR_range,
    MP_QSTR_len,
    MP_QSTR__star_,
};

// constants
static const mp_rom_obj_tuple_t const_obj_neopixel_0 = {{&mp_type_tuple}, 4, {
    MP_ROM_INT(1),
    MP_ROM_INT(0),
    MP_ROM_INT(2),
    MP_ROM_INT(3),
}};
static const mp_rom_obj_tuple_t const_obj_neopixel_1 = {{&mp_type_tuple}, 4, {
    MP_ROM_INT(400),
    MP_ROM_INT(850),
    MP_ROM_INT(800),
    MP_ROM_INT(450),
}};
static const mp_rom_obj_tuple_t const_obj_neopixel_2 = {{&mp_type_tuple}, 4, {
    MP_ROM_INT(800),
    MP_ROM_INT(1700),
    MP_ROM_INT(1600),
    MP_ROM_INT(900),
}};

// constant table
static const mp_rom_obj_t const_obj_table_data_neopixel[3] = {
    MP_ROM_PTR(&const_obj_neopixel_0),
    MP_ROM_PTR(&const_obj_neopixel_1),
    MP_ROM_PTR(&const_obj_neopixel_2),
};

static const mp_frozen_module_t frozen_module_neopixel = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_neopixel,
        .obj_table = (mp_obj_t *)&const_obj_table_data_neopixel,
    },
    .proto_fun = &proto_fun_neopixel__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// frozen module tdui
// - original source file: D:\projects\tiny_desk_2\tinydesk\build-builder-20261104\frozen-080\tdui.mpy
// - frozen file name: tdui.py
// - .mpy header: 4d:06:00:1f

// frozen bytecode for file tdui.py, scope tdui__lt_module_gt_
static const byte fun_data_tdui__lt_module_gt_[475] = {
    0x40,0x02, // prelude
    0x01, // names: <module>
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x02, // IMPORT_NAME '_tdui'
    0x16,0x02, // STORE_NAME '_tdui'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x03, // IMPORT_NAME 'os'
    0x16,0x03, // STORE_NAME 'os'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x04, // IMPORT_NAME 'time'
    0x16,0x04, // STORE_NAME 'time'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x05, // LOAD_CONST_STRING 'const'
    0x2a,0x01, // BUILD_TUPLE 1
    0x1b,0x06, // IMPORT_NAME 'micropython'
    0x1c,0x05, // IMPORT_FROM 'const'
    0x16,0x05, // STORE_NAME 'const'
    0x59, // POP_TOP
    0x81, // LOAD_CONST_SMALL_INT 1
    0x16,0x81,0x60, // STORE_NAME 'SUPPORTS_GRADIENT_FILLS'
    0x11,0x81,0x61, // LOAD_NAME 'bool'
    0x11,0x81,0x62, // LOAD_NAME 'getattr'
    0x11,0x02, // LOAD_NAME '_tdui'
    0x10,0x07, // LOAD_CONST_STRING 'SUPPORTS_RGB_PICTURES'
    0x50, // LOAD_CONST_FALSE
    0x34,0x03, // CALL_FUNCTION 3
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x07, // STORE_NAME 'SUPPORTS_RGB_PICTURES'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x1d, // STORE_NAME 'enable_rgb_pictures'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x1e, // STORE_NAME 'graphics_info'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x1f, // STORE_NAME 'scanout_info'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x20, // STORE_NAME 'scanout_timing'
    0x22,0x81,0x00, // LOAD_CONST_SMALL_INT 128
    0x52, // LOAD_CONST_TRUE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x04, // MAKE_FUNCTION_DEFARGS 4
    0x16,0x21, // STORE_NAME 'render_cache'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x22, // STORE_NAME 'render_cache_info'
    0x84, // LOAD_CONST_SMALL_INT 4
    0x16,0x81,0x63, // STORE_NAME 'MAX_WINDOWS'
    0x22,0x30, // LOAD_CONST_SMALL_INT 48
    0x16,0x81,0x64, // STORE_NAME 'MAX_WIDGETS'
    0x11,0x02, // LOAD_NAME '_tdui'
    0x13,0x08, // LOAD_ATTR 'MAX_CELLS'
    0x16,0x08, // STORE_NAME 'MAX_CELLS'
    0x90, // LOAD_CONST_SMALL_INT 16
    0x16,0x81,0x65, // STORE_NAME 'MAX_PAGES'
    0x11,0x02, // LOAD_NAME '_tdui'
    0x13,0x09, // LOAD_ATTR 'MAX_PIXELS'
    0x16,0x09, // STORE_NAME 'MAX_PIXELS'
    0x11,0x81,0x62, // LOAD_NAME 'getattr'
    0x11,0x02, // LOAD_NAME '_tdui'
    0x10,0x0a, // LOAD_CONST_STRING 'MAX_PICTURES'
    0x11,0x08, // LOAD_NAME 'MAX_CELLS'
    0x22,0x8f,0x50, // LOAD_CONST_SMALL_INT 2000
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x45, // POP_JUMP_IF_FALSE 5
    0x22,0x81,0x40, // LOAD_CONST_SMALL_INT 192
    0x42,0x42, // JUMP 2
    0x22,0x30, // LOAD_CONST_SMALL_INT 48
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x0a, // STORE_NAME 'MAX_PICTURES'
    0x11,0x81,0x62, // LOAD_NAME 'getattr'
    0x11,0x02, // LOAD_NAME '_tdui'
    0x10,0x0b, // LOAD_CONST_STRING 'MAX_VISIBLE_PICTURES'
    0x22,0x80,0x40, // LOAD_CONST_SMALL_INT 64
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x0b, // STORE_NAME 'MAX_VISIBLE_PICTURES'
    0x7e, // LOAD_CONST_SMALL_INT -2
    0x16,0x81,0x66, // STORE_NAME 'CLEAR'
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x16,0x81,0x67, // STORE_NAME '_KEYS'
    0x2c,0x00, // BUILD_MAP 0
    0x16,0x81,0x68, // STORE_NAME '_objs'
    0x2c,0x00, // BUILD_MAP 0
    0x16,0x81,0x69, // STORE_NAME '_results'
    0x2b,0x00, // BUILD_LIST 0
    0x16,0x81,0x6a, // STORE_NAME '_timers'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x17,0x81,0x6b, // STORE_GLOBAL '_next'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x17,0x81,0x6c, // STORE_GLOBAL '_windows'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x17,0x81,0x6d, // STORE_GLOBAL '_widgets'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x17,0x71, // STORE_GLOBAL '_cells'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x23, // STORE_NAME '_new_id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x2a,0x09, // BUILD_TUPLE 9
    0x53, // LOAD_NULL
    0x33,0x07, // MAKE_FUNCTION_DEFARGS 7
    0x16,0x24, // STORE_NAME '_send'
    0x32,0x08, // MAKE_FUNCTION 8
    0x16,0x26, // STORE_NAME '_key_name'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x09, // MAKE_FUNCTION 9
    0x10,0x0c, // LOAD_CONST_STRING 'Timer'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x0c, // STORE_NAME 'Timer'
    0x32,0x0a, // MAKE_FUNCTION 10
    0x16,0x2a, // STORE_NAME 'every'
    0x32,0x0b, // MAKE_FUNCTION 11
    0x16,0x2b, // STORE_NAME 'after'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x0c, // MAKE_FUNCTION 12
    0x10,0x0d, // LOAD_CONST_STRING 'Window'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x0d, // STORE_NAME 'Window'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x0d, // MAKE_FUNCTION 13
    0x10,0x0e, // LOAD_CONST_STRING '_Widget'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x0e, // STORE_NAME '_Widget'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x0e, // MAKE_FUNCTION 14
    0x10,0x0f, // LOAD_CONST_STRING 'Label'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x0f, // STORE_NAME 'Label'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x0f, // MAKE_FUNCTION 15
    0x10,0x10, // LOAD_CONST_STRING 'Button'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x10, // STORE_NAME 'Button'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x10, // MAKE_FUNCTION 16
    0x10,0x11, // LOAD_CONST_STRING 'Checkbox'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x11, // STORE_NAME 'Checkbox'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x11, // MAKE_FUNCTION 17
    0x10,0x12, // LOAD_CONST_STRING 'Textbox'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x12, // STORE_NAME 'Textbox'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x12, // MAKE_FUNCTION 18
    0x10,0x13, // LOAD_CONST_STRING 'Progress'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x13, // STORE_NAME 'Progress'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x13, // MAKE_FUNCTION 19
    0x10,0x14, // LOAD_CONST_STRING 'List'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x14, // STORE_NAME 'List'
    0x32,0x14, // MAKE_FUNCTION 20
    0x16,0x2d, // STORE_NAME 'rgb'
    0x32,0x15, // MAKE_FUNCTION 21
    0x16,0x2e, // STORE_NAME '_part'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x16, // MAKE_FUNCTION 22
    0x10,0x15, // LOAD_CONST_STRING '_Batch'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x15, // STORE_NAME '_Batch'
    0x11,0x15, // LOAD_NAME '_Batch'
    0x34,0x00, // CALL_FUNCTION 0
    0x16,0x81,0x6e, // STORE_NAME '_batch'
    0x32,0x17, // MAKE_FUNCTION 23
    0x16,0x2f, // STORE_NAME 'batch'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x18, // MAKE_FUNCTION 24
    0x10,0x16, // LOAD_CONST_STRING '_Pixels'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x16, // STORE_NAME '_Pixels'
    0x81, // LOAD_CONST_SMALL_INT 1
    0x50, // LOAD_CONST_FALSE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x19, // MAKE_FUNCTION_DEFARGS 25
    0x16,0x30, // STORE_NAME 'text_size'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x1a, // MAKE_FUNCTION 26
    0x10,0x17, // LOAD_CONST_STRING 'CanvasButton'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x17, // STORE_NAME 'CanvasButton'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x1b, // MAKE_FUNCTION 27
    0x10,0x18, // LOAD_CONST_STRING 'Image'
    0x11,0x16, // LOAD_NAME '_Pixels'
    0x34,0x03, // CALL_FUNCTION 3
    0x16,0x18, // STORE_NAME 'Image'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x1c, // MAKE_FUNCTION 28
    0x10,0x19, // LOAD_CONST_STRING 'Font'
    0x34,0x02, // CALL_FUNCTION 2
    0x16,0x19, // STORE_NAME 'Font'
    0x54, // LOAD_BUILD_CLASS
    0x32,0x1d, // MAKE_FUNCTION 29
    0x10,0x1a, // LOAD_CONST_STRING 'Canvas'
    0x11,0x0e, // LOAD_NAME '_Widget'
    0x11,0x16, // LOAD_NAME '_Pixels'
    0x34,0x04, // CALL_FUNCTION 4
    0x16,0x1a, // STORE_NAME 'Canvas'
    0x81, // LOAD_CONST_SMALL_INT 1
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x1e, // MAKE_FUNCTION_DEFARGS 30
    0x16,0x36, // STORE_NAME 'big_size'
    0x32,0x1f, // MAKE_FUNCTION 31
    0x16,0x37, // STORE_NAME '_run_timers'
    0x32,0x20, // MAKE_FUNCTION 32
    0x16,0x40, // STORE_NAME '_dispatch'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x21, // MAKE_FUNCTION_DEFARGS 33
    0x16,0x45, // STORE_NAME 'poll'
    0x32,0x22, // MAKE_FUNCTION 34
    0x16,0x47, // STORE_NAME 'run'
    0x32,0x23, // MAKE_FUNCTION 35
    0x16,0x48, // STORE_NAME 'run_async'
    0x32,0x24, // MAKE_FUNCTION 36
    0x16,0x4b, // STORE_NAME 'quit'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x25, // MAKE_FUNCTION_DEFARGS 37
    0x16,0x4e, // STORE_NAME '_request'
    0x10,0x1b, // LOAD_CONST_STRING 'OK'
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x26, // MAKE_FUNCTION_DEFARGS 38
    0x16,0x51, // STORE_NAME 'msgbox'
    0x32,0x27, // MAKE_FUNCTION 39
    0x16,0x53, // STORE_NAME 'confirm'
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x50, // LOAD_CONST_FALSE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x28, // MAKE_FUNCTION_DEFARGS 40
    0x16,0x55, // STORE_NAME 'inputbox'
    0x32,0x29, // MAKE_FUNCTION 41
    0x16,0x56, // STORE_NAME 'menu'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_enable_rgb_pictures
static const byte fun_data_tdui_enable_rgb_pictures[16] = {
    0x08,0x02, // prelude
    0x1d, // names: enable_rgb_pictures
     // code info
    0x12,0x07, // LOAD_GLOBAL 'SUPPORTS_RGB_PICTURES'
    0x43,0x42, // POP_JUMP_IF_TRUE 2
    0x50, // LOAD_CONST_FALSE
    0x63, // RETURN_VALUE
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x1d, // LOAD_METHOD 'enable_rgb_pictures'
    0x36,0x00, // CALL_METHOD 0
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_enable_rgb_pictures = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_enable_rgb_pictures,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 16,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 29,
        .line_info = fun_data_tdui_enable_rgb_pictures + 3,
        .line_info_top = fun_data_tdui_enable_rgb_pictures + 3,
        .opcodes = fun_data_tdui_enable_rgb_pictures + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_enable_rgb_pictures fun_data_tdui_enable_rgb_pictures[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_graphics_info
static const byte fun_data_tdui_graphics_info[10] = {
    0x08,0x02, // prelude
    0x1e, // names: graphics_info
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x1e, // LOAD_METHOD 'graphics_info'
    0x36,0x00, // CALL_METHOD 0
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_graphics_info = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_graphics_info,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 30,
        .line_info = fun_data_tdui_graphics_info + 3,
        .line_info_top = fun_data_tdui_graphics_info + 3,
        .opcodes = fun_data_tdui_graphics_info + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_graphics_info fun_data_tdui_graphics_info[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_scanout_info
static const byte fun_data_tdui_scanout_info[29] = {
    0x20,0x02, // prelude
    0x1f, // names: scanout_info
     // code info
    0x12,0x81,0x62, // LOAD_GLOBAL 'getattr'
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x10,0x1f, // LOAD_CONST_STRING 'scanout_info'
    0x51, // LOAD_CONST_NONE
    0x34,0x03, // CALL_FUNCTION 3
    0xc0, // STORE_FAST 0
    0xb0, // LOAD_FAST 0
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0xb0, // LOAD_FAST 0
    0x34,0x00, // CALL_FUNCTION 0
    0x63, // RETURN_VALUE
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf4, // BINARY_OP 29 __mul__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_scanout_info = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_scanout_info,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 29,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 31,
        .line_info = fun_data_tdui_scanout_info + 3,
        .line_info_top = fun_data_tdui_scanout_info + 3,
        .opcodes = fun_data_tdui_scanout_info + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_scanout_info fun_data_tdui_scanout_info[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_scanout_timing
static const byte fun_data_tdui_scanout_timing[29] = {
    0x20,0x02, // prelude
    0x20, // names: scanout_timing
     // code info
    0x12,0x81,0x62, // LOAD_GLOBAL 'getattr'
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x10,0x20, // LOAD_CONST_STRING 'scanout_timing'
    0x51, // LOAD_CONST_NONE
    0x34,0x03, // CALL_FUNCTION 3
    0xc0, // STORE_FAST 0
    0xb0, // LOAD_FAST 0
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0xb0, // LOAD_FAST 0
    0x34,0x00, // CALL_FUNCTION 0
    0x63, // RETURN_VALUE
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x84, // LOAD_CONST_SMALL_INT 4
    0xf4, // BINARY_OP 29 __mul__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_scanout_timing = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_scanout_timing,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 29,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 32,
        .line_info = fun_data_tdui_scanout_timing + 3,
        .line_info_top = fun_data_tdui_scanout_timing + 3,
        .opcodes = fun_data_tdui_scanout_timing + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_scanout_timing fun_data_tdui_scanout_timing[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_render_cache
static const byte fun_data_tdui_render_cache[34] = {
    0xb2,0x80,0x01,0x0a, // prelude
    0x21,0x81,0x6f,0x81,0x70, // names: render_cache, kb, warm
     // code info
    0x12,0x81,0x62, // LOAD_GLOBAL 'getattr'
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x10,0x21, // LOAD_CONST_STRING 'render_cache'
    0x51, // LOAD_CONST_NONE
    0x34,0x03, // CALL_FUNCTION 3
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0x34,0x02, // CALL_FUNCTION 2
    0x63, // RETURN_VALUE
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_render_cache = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_render_cache,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 34,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 33,
        .line_info = fun_data_tdui_render_cache + 9,
        .line_info_top = fun_data_tdui_render_cache + 9,
        .opcodes = fun_data_tdui_render_cache + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_render_cache fun_data_tdui_render_cache[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_render_cache_info
static const byte fun_data_tdui_render_cache_info[29] = {
    0x20,0x02, // prelude
    0x22, // names: render_cache_info
     // code info
    0x12,0x81,0x62, // LOAD_GLOBAL 'getattr'
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x10,0x22, // LOAD_CONST_STRING 'render_cache_info'
    0x51, // LOAD_CONST_NONE
    0x34,0x03, // CALL_FUNCTION 3
    0xc0, // STORE_FAST 0
    0xb0, // LOAD_FAST 0
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0xb0, // LOAD_FAST 0
    0x34,0x00, // CALL_FUNCTION 0
    0x63, // RETURN_VALUE
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x8c, // LOAD_CONST_SMALL_INT 12
    0xf4, // BINARY_OP 29 __mul__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_render_cache_info = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_render_cache_info,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 29,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 34,
        .line_info = fun_data_tdui_render_cache_info + 3,
        .line_info_top = fun_data_tdui_render_cache_info + 3,
        .opcodes = fun_data_tdui_render_cache_info + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_render_cache_info fun_data_tdui_render_cache_info[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__new_id
static const byte fun_data_tdui__new_id[44] = {
    0x08,0x02, // prelude
    0x23, // names: _new_id
     // code info
    0x12,0x81,0x6b, // LOAD_GLOBAL '_next'
    0x22,0x83,0xff,0x7f, // LOAD_CONST_SMALL_INT 65535
    0xf8, // BINARY_OP 33 __mod__
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf2, // BINARY_OP 27 __add__
    0x17,0x81,0x6b, // STORE_GLOBAL '_next'
    0x12,0x81,0x6b, // LOAD_GLOBAL '_next'
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xdd, // BINARY_OP 6 <in>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0x12,0x81,0x6b, // LOAD_GLOBAL '_next'
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xdd, // BINARY_OP 6 <in>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x44, // POP_JUMP_IF_FALSE 4
    0x12,0x81,0x6b, // LOAD_GLOBAL '_next'
    0x63, // RETURN_VALUE
    0x42,0x19, // JUMP -39
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__new_id = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__new_id,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 44,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 35,
        .line_info = fun_data_tdui__new_id + 3,
        .line_info_top = fun_data_tdui__new_id + 3,
        .opcodes = fun_data_tdui__new_id + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__new_id fun_data_tdui__new_id[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__send
static const byte fun_data_tdui__send[46] = {
    0xbb,0x91,0x84,0x80,0x01,0x28, // prelude
    0x24,0x81,0x71,0x5d,0x5a,0x81,0x72,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x81,0x77,0x81,0x78,0x50, // names: _send, op, id, win, flags, a, b, c, d, e, f, text
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0xb9, // LOAD_FAST 9
    0xba, // LOAD_FAST 10
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__send = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__send,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 46,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 24,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 11,
        .n_kwonly_args = 0,
        .n_def_pos_args = 9,
        .qstr_block_name_idx = 36,
        .line_info = fun_data_tdui__send + 26,
        .line_info_top = fun_data_tdui__send + 26,
        .opcodes = fun_data_tdui__send + 26,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__send fun_data_tdui__send[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__key_name
static const byte fun_data_tdui__key_name[87] = {
    0x2a,0x0a, // prelude
    0x26,0x81,0x79,0x81,0x7a, // names: _key_name, key, mods
     // code info
    0xb0, // LOAD_FAST 0
    0x22,0x80,0xc4,0x80,0x00, // LOAD_CONST_SMALL_INT 1114112
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x6a, // POP_JUMP_IF_FALSE 42
    0xb0, // LOAD_FAST 0
    0x22,0x80,0xc4,0x80,0x01, // LOAD_CONST_SMALL_INT 1114113
    0xf3, // BINARY_OP 28 __sub__
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0x12,0x81,0x67, // LOAD_GLOBAL '_KEYS'
    0x34,0x01, // CALL_FUNCTION 1
    0xd7, // BINARY_OP 0 __lt__
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0x12,0x81,0x67, // LOAD_GLOBAL '_KEYS'
    0xb0, // LOAD_FAST 0
    0x22,0x80,0xc4,0x80,0x01, // LOAD_CONST_SMALL_INT 1114113
    0xf3, // BINARY_OP 28 __sub__
    0x55, // LOAD_SUBSCR
    0xc2, // STORE_FAST 2
    0xb1, // LOAD_FAST 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xef, // BINARY_OP 24 __and__
    0x44,0x45, // POP_JUMP_IF_FALSE 5
    0x10,0x27, // LOAD_CONST_STRING 'shift+'
    0xb2, // LOAD_FAST 2
    0xf2, // BINARY_OP 27 __add__
    0xc2, // STORE_FAST 2
    0x42,0x47, // JUMP 7
    0x12,0x81,0x7c, // LOAD_GLOBAL 'chr'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0xc2, // STORE_FAST 2
    0xb1, // LOAD_FAST 1
    0x82, // LOAD_CONST_SMALL_INT 2
    0xef, // BINARY_OP 24 __and__
    0x44,0x45, // POP_JUMP_IF_FALSE 5
    0x10,0x28, // LOAD_CONST_STRING 'alt+'
    0xb2, // LOAD_FAST 2
    0xf2, // BINARY_OP 27 __add__
    0xc2, // STORE_FAST 2
    0xb1, // LOAD_FAST 1
    0x84, // LOAD_CONST_SMALL_INT 4
    0xef, // BINARY_OP 24 __and__
    0x44,0x45, // POP_JUMP_IF_FALSE 5
    0x10,0x29, // LOAD_CONST_STRING 'ctrl+'
    0xb2, // LOAD_FAST 2
    0xf2, // BINARY_OP 27 __add__
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__key_name = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__key_name,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 87,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 38,
        .line_info = fun_data_tdui__key_name + 7,
        .line_info_top = fun_data_tdui__key_name + 7,
        .opcodes = fun_data_tdui__key_name + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__key_name fun_data_tdui__key_name[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Timer
static const byte fun_data_tdui_Timer[28] = {
    0x08,0x02, // prelude
    0x0c, // names: Timer
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x0c, // LOAD_CONST_STRING 'Timer'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x51, // LOAD_CONST_NONE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x00, // MAKE_FUNCTION_DEFARGS 0
    0x16,0x59, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x5c, // STORE_NAME 'cancel'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_Timer
// frozen bytecode for file tdui.py, scope tdui_Timer___init__
static const byte fun_data_tdui_Timer___init__[53] = {
    0xc1,0x05,0x0e, // prelude
    0x59,0x82,0x10,0x3d,0x3f,0x3b,0x5a, // names: __init__, self, ms, fn, repeat, win
     // code info
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x18,0x3d, // STORE_ATTR 'ms'
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x3f, // STORE_ATTR 'fn'
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x18,0x3b, // STORE_ATTR 'repeat'
    0xb4, // LOAD_FAST 4
    0xb0, // LOAD_FAST 0
    0x18,0x5a, // STORE_ATTR 'win'
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x3c, // LOAD_METHOD 'ticks_add'
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x38, // LOAD_METHOD 'ticks_ms'
    0x36,0x00, // CALL_METHOD 0
    0xb1, // LOAD_FAST 1
    0x36,0x02, // CALL_METHOD 2
    0xb0, // LOAD_FAST 0
    0x18,0x3a, // STORE_ATTR 'due'
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x14,0x5b, // LOAD_METHOD 'append'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Timer___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Timer___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 53,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Timer___init__ + 10,
        .line_info_top = fun_data_tdui_Timer___init__ + 10,
        .opcodes = fun_data_tdui_Timer___init__ + 10,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Timer___init__ fun_data_tdui_Timer___init__[0]
#endif

// child of tdui_Timer
// frozen bytecode for file tdui.py, scope tdui_Timer_cancel
static const byte fun_data_tdui_Timer_cancel[23] = {
    0x19,0x06, // prelude
    0x5c,0x82,0x10, // names: cancel, self
     // code info
    0xb0, // LOAD_FAST 0
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0xdd, // BINARY_OP 6 <in>
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x14,0x3e, // LOAD_METHOD 'remove'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Timer_cancel = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Timer_cancel,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 23,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 92,
        .line_info = fun_data_tdui_Timer_cancel + 5,
        .line_info_top = fun_data_tdui_Timer_cancel + 5,
        .opcodes = fun_data_tdui_Timer_cancel + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Timer_cancel fun_data_tdui_Timer_cancel[0]
#endif

static const mp_raw_code_t *const children_tdui_Timer[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Timer___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Timer_cancel,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Timer = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Timer,
    .children = (void *)&children_tdui_Timer,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 28,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 12,
        .line_info = fun_data_tdui_Timer + 3,
        .line_info_top = fun_data_tdui_Timer + 3,
        .opcodes = fun_data_tdui_Timer + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_every
static const byte fun_data_tdui_every[13] = {
    0x2a,0x06, // prelude
    0x2a,0x3d,0x3f, // names: every, ms, fn
     // code info
    0x12,0x0c, // LOAD_GLOBAL 'Timer'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0x52, // LOAD_CONST_TRUE
    0x34,0x03, // CALL_FUNCTION 3
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_every = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_every,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 13,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 42,
        .line_info = fun_data_tdui_every + 5,
        .line_info_top = fun_data_tdui_every + 5,
        .opcodes = fun_data_tdui_every + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_every fun_data_tdui_every[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_after
static const byte fun_data_tdui_after[13] = {
    0x2a,0x06, // prelude
    0x2b,0x3d,0x3f, // names: after, ms, fn
     // code info
    0x12,0x0c, // LOAD_GLOBAL 'Timer'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0x50, // LOAD_CONST_FALSE
    0x34,0x03, // CALL_FUNCTION 3
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_after = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_after,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 13,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 43,
        .line_info = fun_data_tdui_after + 5,
        .line_info_top = fun_data_tdui_after + 5,
        .opcodes = fun_data_tdui_after + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_after fun_data_tdui_after[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Window
static const byte fun_data_tdui_Window[160] = {
    0x18,0x02, // prelude
    0x0d, // names: Window
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x0d, // LOAD_CONST_STRING 'Window'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x50, // LOAD_CONST_FALSE
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x00, // MAKE_FUNCTION_DEFARGS 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x01, // MAKE_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x66, // STORE_NAME 'title'
    0x11,0x66, // LOAD_NAME 'title'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x02, // MAKE_FUNCTION 2
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x66, // STORE_NAME 'title'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x67, // STORE_NAME 'focus'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x68, // STORE_NAME 'resize'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x69, // STORE_NAME 'move'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x6a, // STORE_NAME 'restore'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x6b, // STORE_NAME 'maximize'
    0x32,0x08, // MAKE_FUNCTION 8
    0x16,0x6c, // STORE_NAME 'minimize'
    0x32,0x09, // MAKE_FUNCTION 9
    0x16,0x6d, // STORE_NAME 'fullscreen'
    0x32,0x0a, // MAKE_FUNCTION 10
    0x16,0x4d, // STORE_NAME 'close'
    0x32,0x0b, // MAKE_FUNCTION 11
    0x16,0x6e, // STORE_NAME '_gone'
    0x32,0x0c, // MAKE_FUNCTION 12
    0x16,0x44, // STORE_NAME '_event'
    0x32,0x0d, // MAKE_FUNCTION 13
    0x16,0x2a, // STORE_NAME 'every'
    0x32,0x0e, // MAKE_FUNCTION 14
    0x16,0x2b, // STORE_NAME 'after'
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x80, // LOAD_CONST_SMALL_INT 0
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x0f, // MAKE_FUNCTION_DEFARGS 15
    0x16,0x72, // STORE_NAME 'label'
    0x51, // LOAD_CONST_NONE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x10, // MAKE_FUNCTION_DEFARGS 16
    0x16,0x73, // STORE_NAME 'button'
    0x50, // LOAD_CONST_FALSE
    0x51, // LOAD_CONST_NONE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x11, // MAKE_FUNCTION_DEFARGS 17
    0x16,0x74, // STORE_NAME 'checkbox'
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x50, // LOAD_CONST_FALSE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2a,0x04, // BUILD_TUPLE 4
    0x53, // LOAD_NULL
    0x33,0x12, // MAKE_FUNCTION_DEFARGS 18
    0x16,0x75, // STORE_NAME 'textbox'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x13, // MAKE_FUNCTION_DEFARGS 19
    0x16,0x76, // STORE_NAME 'progress'
    0x2a,0x00, // BUILD_TUPLE 0
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x14, // MAKE_FUNCTION_DEFARGS 20
    0x16,0x77, // STORE_NAME 'list'
    0x51, // LOAD_CONST_NONE
    0x81, // LOAD_CONST_SMALL_INT 1
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x15, // MAKE_FUNCTION_DEFARGS 21
    0x16,0x78, // STORE_NAME 'canvas'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window___init__
static const byte fun_data_tdui_Window___init__[190] = {
    0x8b,0x95,0x01,0x1c, // prelude
    0x59,0x82,0x10,0x66,0x81,0x25,0x81,0x26,0x81,0x23,0x81,0x24,0x82,0x11, // names: __init__, self, title, w, h, x, y, resizable
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x4f, // LOAD_METHOD 'open'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x12,0x81,0x6c, // LOAD_GLOBAL '_windows'
    0x84, // LOAD_CONST_SMALL_INT 4
    0xdb, // BINARY_OP 4 __ge__
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x23,0x03, // LOAD_CONST_OBJ 3
    0x84, // LOAD_CONST_SMALL_INT 4
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb2, // LOAD_FAST 2
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x06, // JUMP_IF_FALSE_OR_POP 6
    0x22,0x82,0x2c, // LOAD_CONST_SMALL_INT 300
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x44,0x51, // POP_JUMP_IF_FALSE 17
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb3, // LOAD_FAST 3
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x06, // JUMP_IF_FALSE_OR_POP 6
    0x22,0x81,0x16, // LOAD_CONST_SMALL_INT 150
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x43,0x48, // POP_JUMP_IF_TRUE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x04, // LOAD_CONST_OBJ 4
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x23, // LOAD_GLOBAL '_new_id'
    0x34,0x00, // CALL_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x18,0x5d, // STORE_ATTR 'id'
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x5e, // STORE_ATTR '_title'
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x5f, // STORE_ATTR 'width'
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x18,0x60, // STORE_ATTR 'height'
    0x52, // LOAD_CONST_TRUE
    0xb0, // LOAD_FAST 0
    0x18,0x61, // STORE_ATTR 'is_open'
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x62, // STORE_ATTR 'on_close'
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x63, // STORE_ATTR 'on_key'
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x64, // STORE_ATTR 'on_resize'
    0x2b,0x00, // BUILD_LIST 0
    0xb0, // LOAD_FAST 0
    0x18,0x65, // STORE_ATTR '_kids'
    0xb0, // LOAD_FAST 0
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x56, // STORE_SUBSCR
    0x12,0x81,0x6c, // LOAD_GLOBAL '_windows'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x17,0x81,0x6c, // STORE_GLOBAL '_windows'
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb6, // LOAD_FAST 6
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x42,0x41, // JUMP 1
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x42,0x41, // JUMP 1
    0xb5, // LOAD_FAST 5
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0xb0, // LOAD_FAST 0
    0x13,0x5e, // LOAD_ATTR '_title'
    0x34,0x82,0x08, // CALL_FUNCTION 264
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 190,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 18,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Window___init__ + 18,
        .line_info_top = fun_data_tdui_Window___init__ + 18,
        .opcodes = fun_data_tdui_Window___init__ + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window___init__ fun_data_tdui_Window___init__[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_title
static const byte fun_data_tdui_Window_title[9] = {
    0x09,0x06, // prelude
    0x66,0x82,0x10, // names: title, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5e, // LOAD_ATTR '_title'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_title = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_title,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 9,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 102,
        .line_info = fun_data_tdui_Window_title + 5,
        .line_info_top = fun_data_tdui_Window_title + 5,
        .opcodes = fun_data_tdui_Window_title + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_title fun_data_tdui_Window_title[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_title2
static const byte fun_data_tdui_Window_title2[38] = {
    0x32,0x0a, // prelude
    0x66,0x82,0x10,0x82,0x13, // names: title, self, t
     // code info
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x5e, // STORE_ATTR '_title'
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4f, // POP_JUMP_IF_FALSE 15
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x84, // LOAD_CONST_SMALL_INT 4
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0xb0, // LOAD_FAST 0
    0x13,0x5e, // LOAD_ATTR '_title'
    0x34,0x82,0x02, // CALL_FUNCTION 258
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_title2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_title2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 38,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 102,
        .line_info = fun_data_tdui_Window_title2 + 7,
        .line_info_top = fun_data_tdui_Window_title2 + 7,
        .opcodes = fun_data_tdui_Window_title2 + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_title2 fun_data_tdui_Window_title2[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_focus
static const byte fun_data_tdui_Window_focus[21] = {
    0x19,0x06, // prelude
    0x67,0x82,0x10, // names: focus, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x88, // LOAD_CONST_SMALL_INT 8
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x34,0x02, // CALL_FUNCTION 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_focus = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_focus,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 21,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 103,
        .line_info = fun_data_tdui_Window_focus + 5,
        .line_info_top = fun_data_tdui_Window_focus + 5,
        .opcodes = fun_data_tdui_Window_focus + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_focus fun_data_tdui_Window_focus[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_resize
static const byte fun_data_tdui_Window_resize[29] = {
    0x4b,0x0e, // prelude
    0x68,0x82,0x10,0x81,0x25,0x81,0x26, // names: resize, self, w, h
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4d, // POP_JUMP_IF_FALSE 13
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x90, // LOAD_CONST_SMALL_INT 16
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x34,0x06, // CALL_FUNCTION 6
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_resize = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_resize,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 29,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 104,
        .line_info = fun_data_tdui_Window_resize + 9,
        .line_info_top = fun_data_tdui_Window_resize + 9,
        .opcodes = fun_data_tdui_Window_resize + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_resize fun_data_tdui_Window_resize[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_move
static const byte fun_data_tdui_Window_move[29] = {
    0x4b,0x0e, // prelude
    0x69,0x82,0x10,0x81,0x23,0x81,0x24, // names: move, self, x, y
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4d, // POP_JUMP_IF_FALSE 13
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x91, // LOAD_CONST_SMALL_INT 17
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x34,0x06, // CALL_FUNCTION 6
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_move = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_move,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 29,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 105,
        .line_info = fun_data_tdui_Window_move + 9,
        .line_info_top = fun_data_tdui_Window_move + 9,
        .opcodes = fun_data_tdui_Window_move + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_move fun_data_tdui_Window_move[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_restore
static const byte fun_data_tdui_Window_restore[24] = {
    0x31,0x06, // prelude
    0x6a,0x82,0x10, // names: restore, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x92, // LOAD_CONST_SMALL_INT 18
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x34,0x05, // CALL_FUNCTION 5
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_restore = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_restore,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 106,
        .line_info = fun_data_tdui_Window_restore + 5,
        .line_info_top = fun_data_tdui_Window_restore + 5,
        .opcodes = fun_data_tdui_Window_restore + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_restore fun_data_tdui_Window_restore[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_maximize
static const byte fun_data_tdui_Window_maximize[24] = {
    0x31,0x06, // prelude
    0x6b,0x82,0x10, // names: maximize, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x92, // LOAD_CONST_SMALL_INT 18
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x81, // LOAD_CONST_SMALL_INT 1
    0x34,0x05, // CALL_FUNCTION 5
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_maximize = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_maximize,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 107,
        .line_info = fun_data_tdui_Window_maximize + 5,
        .line_info_top = fun_data_tdui_Window_maximize + 5,
        .opcodes = fun_data_tdui_Window_maximize + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_maximize fun_data_tdui_Window_maximize[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_minimize
static const byte fun_data_tdui_Window_minimize[24] = {
    0x31,0x06, // prelude
    0x6c,0x82,0x10, // names: minimize, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x92, // LOAD_CONST_SMALL_INT 18
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x82, // LOAD_CONST_SMALL_INT 2
    0x34,0x05, // CALL_FUNCTION 5
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_minimize = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_minimize,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 108,
        .line_info = fun_data_tdui_Window_minimize + 5,
        .line_info_top = fun_data_tdui_Window_minimize + 5,
        .opcodes = fun_data_tdui_Window_minimize + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_minimize fun_data_tdui_Window_minimize[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_fullscreen
static const byte fun_data_tdui_Window_fullscreen[24] = {
    0x31,0x06, // prelude
    0x6d,0x82,0x10, // names: fullscreen, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x92, // LOAD_CONST_SMALL_INT 18
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x83, // LOAD_CONST_SMALL_INT 3
    0x34,0x05, // CALL_FUNCTION 5
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_fullscreen = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_fullscreen,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 109,
        .line_info = fun_data_tdui_Window_fullscreen + 5,
        .line_info_top = fun_data_tdui_Window_fullscreen + 5,
        .opcodes = fun_data_tdui_Window_fullscreen + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_fullscreen fun_data_tdui_Window_fullscreen[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_close
static const byte fun_data_tdui_Window_close[27] = {
    0x19,0x06, // prelude
    0x4d,0x82,0x10, // names: close, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x4f, // POP_JUMP_IF_FALSE 15
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x34,0x02, // CALL_FUNCTION 2
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x6e, // LOAD_METHOD '_gone'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_close = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_close,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 27,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 77,
        .line_info = fun_data_tdui_Window_close + 5,
        .line_info_top = fun_data_tdui_Window_close + 5,
        .opcodes = fun_data_tdui_Window_close + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_close fun_data_tdui_Window_close[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window__gone
static const byte fun_data_tdui_Window__gone[105] = {
    0x51,0x06, // prelude
    0x6e,0x82,0x10, // names: _gone, self
     // code info
    0x50, // LOAD_CONST_FALSE
    0xb0, // LOAD_FAST 0
    0x18,0x61, // STORE_ATTR 'is_open'
    0x12,0x81,0x6c, // LOAD_GLOBAL '_windows'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe6, // BINARY_OP 15 __isub__
    0x17,0x81,0x6c, // STORE_GLOBAL '_windows'
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0x14,0x6f, // LOAD_METHOD 'pop'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x51, // LOAD_CONST_NONE
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x65, // LOAD_ATTR '_kids'
    0x5f, // GET_ITER_STACK
    0x4b,0x21, // FOR_ITER 33
    0xc1, // STORE_FAST 1
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0x14,0x6f, // LOAD_METHOD 'pop'
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0x51, // LOAD_CONST_NONE
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x12,0x81,0x6d, // LOAD_GLOBAL '_widgets'
    0xb1, // LOAD_FAST 1
    0x13,0x70, // LOAD_ATTR '_n'
    0xe6, // BINARY_OP 15 __isub__
    0x17,0x81,0x6d, // STORE_GLOBAL '_widgets'
    0x12,0x71, // LOAD_GLOBAL '_cells'
    0xb1, // LOAD_FAST 1
    0x13,0x71, // LOAD_ATTR '_cells'
    0xe6, // BINARY_OP 15 __isub__
    0x17,0x71, // STORE_GLOBAL '_cells'
    0x42,0x1d, // JUMP -35
    0x2b,0x00, // BUILD_LIST 0
    0xb0, // LOAD_FAST 0
    0x18,0x65, // STORE_ATTR '_kids'
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2e,0x02, // BUILD_SLICE 2
    0x55, // LOAD_SUBSCR
    0x5f, // GET_ITER_STACK
    0x4b,0x13, // FOR_ITER 19
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x13,0x5a, // LOAD_ATTR 'win'
    0xb0, // LOAD_FAST 0
    0xde, // BINARY_OP 7 <is>
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x14,0x3e, // LOAD_METHOD 'remove'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x2b, // JUMP -21
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window__gone = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window__gone,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 105,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 110,
        .line_info = fun_data_tdui_Window__gone + 5,
        .line_info_top = fun_data_tdui_Window__gone + 5,
        .opcodes = fun_data_tdui_Window__gone + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window__gone fun_data_tdui_Window__gone[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window__event
static const byte fun_data_tdui_Window__event[109] = {
    0xeb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0xb1, // LOAD_FAST 1
    0x83, // LOAD_CONST_SMALL_INT 3
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x55, // POP_JUMP_IF_FALSE 21
    0xb0, // LOAD_FAST 0
    0x14,0x6e, // LOAD_METHOD '_gone'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x62, // LOAD_ATTR 'on_close'
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0xb0, // LOAD_FAST 0
    0x14,0x62, // LOAD_METHOD 'on_close'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0xbf,0x80, // JUMP 63
    0xb1, // LOAD_FAST 1
    0x84, // LOAD_CONST_SMALL_INT 4
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x5d, // POP_JUMP_IF_FALSE 29
    0xb0, // LOAD_FAST 0
    0x13,0x63, // LOAD_ATTR 'on_key'
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0xb0, // LOAD_FAST 0
    0x14,0x63, // LOAD_METHOD 'on_key'
    0xb0, // LOAD_FAST 0
    0x12,0x26, // LOAD_GLOBAL '_key_name'
    0xb2, // LOAD_FAST 2
    0x22,0x83,0xff,0x7f, // LOAD_CONST_SMALL_INT 65535
    0xef, // BINARY_OP 24 __and__
    0xb3, // LOAD_FAST 3
    0x90, // LOAD_CONST_SMALL_INT 16
    0xf0, // BINARY_OP 25 __lshift__
    0xed, // BINARY_OP 22 __or__
    0xb4, // LOAD_FAST 4
    0x34,0x02, // CALL_FUNCTION 2
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x42,0x5d, // JUMP 29
    0xb1, // LOAD_FAST 1
    0x88, // LOAD_CONST_SMALL_INT 8
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x5f, // STORE_ATTR 'width'
    0xb3, // LOAD_FAST 3
    0xb0, // LOAD_FAST 0
    0x18,0x60, // STORE_ATTR 'height'
    0xb0, // LOAD_FAST 0
    0x13,0x64, // LOAD_ATTR 'on_resize'
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb0, // LOAD_FAST 0
    0x14,0x64, // LOAD_METHOD 'on_resize'
    0xb0, // LOAD_FAST 0
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x36,0x03, // CALL_METHOD 3
    0x59, // POP_TOP
    0x42,0x40, // JUMP 0
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 109,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 14,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Window__event + 18,
        .line_info_top = fun_data_tdui_Window__event + 18,
        .opcodes = fun_data_tdui_Window__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window__event fun_data_tdui_Window__event[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_every
static const byte fun_data_tdui_Window_every[16] = {
    0x3b,0x0a, // prelude
    0x2a,0x82,0x10,0x3d,0x3f, // names: every, self, ms, fn
     // code info
    0x12,0x0c, // LOAD_GLOBAL 'Timer'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x52, // LOAD_CONST_TRUE
    0xb0, // LOAD_FAST 0
    0x34,0x04, // CALL_FUNCTION 4
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_every = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_every,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 16,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 42,
        .line_info = fun_data_tdui_Window_every + 7,
        .line_info_top = fun_data_tdui_Window_every + 7,
        .opcodes = fun_data_tdui_Window_every + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_every fun_data_tdui_Window_every[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_after
static const byte fun_data_tdui_Window_after[16] = {
    0x3b,0x0a, // prelude
    0x2b,0x82,0x10,0x3d,0x3f, // names: after, self, ms, fn
     // code info
    0x12,0x0c, // LOAD_GLOBAL 'Timer'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x50, // LOAD_CONST_FALSE
    0xb0, // LOAD_FAST 0
    0x34,0x04, // CALL_FUNCTION 4
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_after = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_after,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 16,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 43,
        .line_info = fun_data_tdui_Window_after + 7,
        .line_info_top = fun_data_tdui_Window_after + 7,
        .opcodes = fun_data_tdui_Window_after + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_after fun_data_tdui_Window_after[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_label
static const byte fun_data_tdui_Window_label[23] = {
    0xd1,0x84,0x01,0x12, // prelude
    0x72,0x82,0x10,0x81,0x23,0x81,0x24,0x50,0x5f, // names: label, self, x, y, text, width
     // code info
    0x12,0x0f, // LOAD_GLOBAL 'Label'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x34,0x05, // CALL_FUNCTION 5
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_label = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_label,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 23,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 114,
        .line_info = fun_data_tdui_Window_label + 13,
        .line_info_top = fun_data_tdui_Window_label + 13,
        .opcodes = fun_data_tdui_Window_label + 13,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_label fun_data_tdui_Window_label[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_button
static const byte fun_data_tdui_Window_button[22] = {
    0xd1,0x05,0x12, // prelude
    0x73,0x82,0x10,0x81,0x23,0x81,0x24,0x50,0x7e, // names: button, self, x, y, text, on_click
     // code info
    0x12,0x10, // LOAD_GLOBAL 'Button'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x34,0x05, // CALL_FUNCTION 5
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_button = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_button,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 22,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 115,
        .line_info = fun_data_tdui_Window_button + 12,
        .line_info_top = fun_data_tdui_Window_button + 12,
        .opcodes = fun_data_tdui_Window_button + 12,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_button fun_data_tdui_Window_button[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_checkbox
static const byte fun_data_tdui_Window_checkbox[27] = {
    0xe2,0x84,0x01,0x18, // prelude
    0x74,0x82,0x10,0x81,0x23,0x81,0x24,0x50,0x81,0x01,0x81,0x00, // names: checkbox, self, x, y, text, checked, on_change
     // code info
    0x12,0x11, // LOAD_GLOBAL 'Checkbox'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x34,0x06, // CALL_FUNCTION 6
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_checkbox = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_checkbox,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 27,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 6,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 116,
        .line_info = fun_data_tdui_Window_checkbox + 16,
        .line_info_top = fun_data_tdui_Window_checkbox + 16,
        .opcodes = fun_data_tdui_Window_checkbox + 16,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_checkbox fun_data_tdui_Window_checkbox[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_textbox
static const byte fun_data_tdui_Window_textbox[33] = {
    0x80,0x90,0x84,0x01,0x1e, // prelude
    0x75,0x82,0x10,0x81,0x23,0x81,0x24,0x5f,0x50,0x82,0x0f,0x81,0x02,0x81,0x00, // names: textbox, self, x, y, width, text, secret, on_enter, on_change
     // code info
    0x12,0x12, // LOAD_GLOBAL 'Textbox'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0x34,0x08, // CALL_FUNCTION 8
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_textbox = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_textbox,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 33,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 4,
        .qstr_block_name_idx = 117,
        .line_info = fun_data_tdui_Window_textbox + 20,
        .line_info_top = fun_data_tdui_Window_textbox + 20,
        .opcodes = fun_data_tdui_Window_textbox + 20,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_textbox fun_data_tdui_Window_textbox[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_progress
static const byte fun_data_tdui_Window_progress[23] = {
    0xd1,0x05,0x14, // prelude
    0x76,0x82,0x10,0x81,0x23,0x81,0x24,0x5f,0x81,0x04, // names: progress, self, x, y, width, value
     // code info
    0x12,0x13, // LOAD_GLOBAL 'Progress'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x34,0x05, // CALL_FUNCTION 5
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_progress = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_progress,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 23,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 118,
        .line_info = fun_data_tdui_Window_progress + 13,
        .line_info_top = fun_data_tdui_Window_progress + 13,
        .opcodes = fun_data_tdui_Window_progress + 13,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_progress fun_data_tdui_Window_progress[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_list
static const byte fun_data_tdui_Window_list[34] = {
    0x80,0x91,0x05,0x22, // prelude
    0x77,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x0a,0x81,0x06,0x81,0x07, // names: list, self, x, y, w, h, items, on_select, on_activate
     // code info
    0x12,0x14, // LOAD_GLOBAL 'List'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0x34,0x08, // CALL_FUNCTION 8
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_list = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_list,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 34,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 119,
        .line_info = fun_data_tdui_Window_list + 21,
        .line_info_top = fun_data_tdui_Window_list + 21,
        .opcodes = fun_data_tdui_Window_list + 21,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_list fun_data_tdui_Window_list[0]
#endif

// child of tdui_Window
// frozen bytecode for file tdui.py, scope tdui_Window_canvas
static const byte fun_data_tdui_Window_canvas[31] = {
    0xf3,0x84,0x01,0x1e, // prelude
    0x78,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x51,0x81,0x50, // names: canvas, self, x, y, w, h, on_mouse, pages
     // code info
    0x12,0x1a, // LOAD_GLOBAL 'Canvas'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0x34,0x07, // CALL_FUNCTION 7
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Window_canvas = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window_canvas,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 31,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 15,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 120,
        .line_info = fun_data_tdui_Window_canvas + 19,
        .line_info_top = fun_data_tdui_Window_canvas + 19,
        .opcodes = fun_data_tdui_Window_canvas + 19,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Window_canvas fun_data_tdui_Window_canvas[0]
#endif

static const mp_raw_code_t *const children_tdui_Window[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Window___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_title,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_title2,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_focus,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_resize,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_move,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_restore,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_maximize,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_minimize,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_fullscreen,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_close,
    (const mp_raw_code_t *)&proto_fun_tdui_Window__gone,
    (const mp_raw_code_t *)&proto_fun_tdui_Window__event,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_every,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_after,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_label,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_button,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_checkbox,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_textbox,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_progress,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_list,
    (const mp_raw_code_t *)&proto_fun_tdui_Window_canvas,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Window = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Window,
    .children = (void *)&children_tdui_Window,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 160,
    .n_children = 22,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 13,
        .line_info = fun_data_tdui_Window + 3,
        .line_info_top = fun_data_tdui_Window + 3,
        .opcodes = fun_data_tdui_Window + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__Widget
static const byte fun_data_tdui__Widget[91] = {
    0x10,0x02, // prelude
    0x0e, // names: _Widget
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x0e, // LOAD_CONST_STRING '_Widget'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x81, // LOAD_CONST_SMALL_INT 1
    0x16,0x70, // STORE_NAME '_n'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x16,0x71, // STORE_NAME '_cells'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x59, // STORE_NAME '__init__'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x01, // MAKE_FUNCTION_DEFARGS 1
    0x16,0x7b, // STORE_NAME '_req'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x02, // MAKE_FUNCTION 2
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x50, // STORE_NAME 'text'
    0x11,0x50, // LOAD_NAME 'text'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x03, // MAKE_FUNCTION 3
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x50, // STORE_NAME 'text'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x04, // MAKE_FUNCTION 4
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x7c, // STORE_NAME 'visible'
    0x11,0x7c, // LOAD_NAME 'visible'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x05, // MAKE_FUNCTION 5
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x7c, // STORE_NAME 'visible'
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x06, // MAKE_FUNCTION_DEFARGS 6
    0x16,0x7d, // STORE_NAME 'color'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x67, // STORE_NAME 'focus'
    0x32,0x08, // MAKE_FUNCTION 8
    0x16,0x44, // STORE_NAME '_event'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget___init__
static const byte fun_data_tdui__Widget___init__[125] = {
    0xa1,0x90,0x04,0x22, // prelude
    0x59,0x82,0x10,0x5a,0x82,0x14,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x77,0x50, // names: __init__, self, win, kind, x, y, w, h, e, text
     // code info
    0xb1, // LOAD_FAST 1
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x43,0x48, // POP_JUMP_IF_TRUE 8
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x23,0x05, // LOAD_CONST_OBJ 5
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x81,0x6d, // LOAD_GLOBAL '_widgets'
    0xb0, // LOAD_FAST 0
    0x13,0x70, // LOAD_ATTR '_n'
    0xf2, // BINARY_OP 27 __add__
    0x22,0x30, // LOAD_CONST_SMALL_INT 48
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x23,0x06, // LOAD_CONST_OBJ 6
    0x22,0x30, // LOAD_CONST_SMALL_INT 48
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x81,0x6d, // LOAD_GLOBAL '_widgets'
    0xb0, // LOAD_FAST 0
    0x13,0x70, // LOAD_ATTR '_n'
    0xe5, // BINARY_OP 14 __iadd__
    0x17,0x81,0x6d, // STORE_GLOBAL '_widgets'
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x18,0x5a, // STORE_ATTR 'win'
    0x12,0x23, // LOAD_GLOBAL '_new_id'
    0x34,0x00, // CALL_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x18,0x5d, // STORE_ATTR 'id'
    0xb8, // LOAD_FAST 8
    0xb0, // LOAD_FAST 0
    0x18,0x79, // STORE_ATTR '_text'
    0x52, // LOAD_CONST_TRUE
    0xb0, // LOAD_FAST 0
    0x18,0x7a, // STORE_ATTR '_visible'
    0xb0, // LOAD_FAST 0
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x56, // STORE_SUBSCR
    0xb1, // LOAD_FAST 1
    0x13,0x65, // LOAD_ATTR '_kids'
    0x14,0x5b, // LOAD_METHOD 'append'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x83, // LOAD_CONST_SMALL_INT 3
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb8, // LOAD_FAST 8
    0x34,0x0b, // CALL_FUNCTION 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 125,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 21,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 9,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui__Widget___init__ + 21,
        .line_info_top = fun_data_tdui__Widget___init__ + 21,
        .opcodes = fun_data_tdui__Widget___init__ + 21,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget___init__ fun_data_tdui__Widget___init__[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget__req
static const byte fun_data_tdui__Widget__req[40] = {
    0xe9,0x85,0x01,0x14, // prelude
    0x7b,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x50, // names: _req, self, op, a, b, text
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x51, // POP_JUMP_IF_FALSE 17
    0x12,0x24, // LOAD_GLOBAL '_send'
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0xb4, // LOAD_FAST 4
    0x34,0x82,0x06, // CALL_FUNCTION 262
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget__req = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget__req,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 40,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 14,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 123,
        .line_info = fun_data_tdui__Widget__req + 14,
        .line_info_top = fun_data_tdui__Widget__req + 14,
        .opcodes = fun_data_tdui__Widget__req + 14,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget__req fun_data_tdui__Widget__req[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget_text
static const byte fun_data_tdui__Widget_text[9] = {
    0x09,0x06, // prelude
    0x50,0x82,0x10, // names: text, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget_text = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget_text,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 9,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 80,
        .line_info = fun_data_tdui__Widget_text + 5,
        .line_info_top = fun_data_tdui__Widget_text + 5,
        .opcodes = fun_data_tdui__Widget_text + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget_text fun_data_tdui__Widget_text[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget_text2
static const byte fun_data_tdui__Widget_text2[31] = {
    0x32,0x0a, // prelude
    0x50,0x82,0x10,0x82,0x13, // names: text, self, t
     // code info
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x79, // STORE_ATTR '_text'
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x84, // LOAD_CONST_SMALL_INT 4
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0x36,0x82,0x01, // CALL_METHOD 257
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget_text2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget_text2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 31,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 80,
        .line_info = fun_data_tdui__Widget_text2 + 7,
        .line_info_top = fun_data_tdui__Widget_text2 + 7,
        .opcodes = fun_data_tdui__Widget_text2 + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget_text2 fun_data_tdui__Widget_text2[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget_visible
static const byte fun_data_tdui__Widget_visible[9] = {
    0x09,0x06, // prelude
    0x7c,0x82,0x10, // names: visible, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x7a, // LOAD_ATTR '_visible'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget_visible = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget_visible,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 9,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 124,
        .line_info = fun_data_tdui__Widget_visible + 5,
        .line_info_top = fun_data_tdui__Widget_visible + 5,
        .opcodes = fun_data_tdui__Widget_visible + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget_visible fun_data_tdui__Widget_visible[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget_visible2
static const byte fun_data_tdui__Widget_visible2[32] = {
    0x2a,0x0a, // prelude
    0x7c,0x82,0x10,0x82,0x15, // names: visible, self, v
     // code info
    0x12,0x81,0x61, // LOAD_GLOBAL 'bool'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x7a, // STORE_ATTR '_visible'
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x86, // LOAD_CONST_SMALL_INT 6
    0xb1, // LOAD_FAST 1
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget_visible2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget_visible2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 32,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 124,
        .line_info = fun_data_tdui__Widget_visible2 + 7,
        .line_info_top = fun_data_tdui__Widget_visible2 + 7,
        .opcodes = fun_data_tdui__Widget_visible2 + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget_visible2 fun_data_tdui__Widget_visible2[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget_color
static const byte fun_data_tdui__Widget_color[22] = {
    0xbb,0x80,0x01,0x0e, // prelude
    0x7d,0x82,0x10,0x81,0x2c,0x82,0x16, // names: color, self, fg, bg
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x87, // LOAD_CONST_SMALL_INT 7
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x36,0x03, // CALL_METHOD 3
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget_color = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget_color,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 22,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 125,
        .line_info = fun_data_tdui__Widget_color + 11,
        .line_info_top = fun_data_tdui__Widget_color + 11,
        .opcodes = fun_data_tdui__Widget_color + 11,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget_color fun_data_tdui__Widget_color[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget_focus
static const byte fun_data_tdui__Widget_focus[14] = {
    0x19,0x06, // prelude
    0x67,0x82,0x10, // names: focus, self
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x88, // LOAD_CONST_SMALL_INT 8
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget_focus = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget_focus,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 103,
        .line_info = fun_data_tdui__Widget_focus + 5,
        .line_info_top = fun_data_tdui__Widget_focus + 5,
        .opcodes = fun_data_tdui__Widget_focus + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget_focus fun_data_tdui__Widget_focus[0]
#endif

// child of tdui__Widget
// frozen bytecode for file tdui.py, scope tdui__Widget__event
static const byte fun_data_tdui__Widget__event[20] = {
    0xbb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Widget__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 20,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui__Widget__event + 18,
        .line_info_top = fun_data_tdui__Widget__event + 18,
        .opcodes = fun_data_tdui__Widget__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Widget__event fun_data_tdui__Widget__event[0]
#endif

static const mp_raw_code_t *const children_tdui__Widget[] = {
    (const mp_raw_code_t *)&proto_fun_tdui__Widget___init__,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget__req,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget_text,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget_text2,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget_visible,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget_visible2,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget_color,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget_focus,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui__Widget = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Widget,
    .children = (void *)&children_tdui__Widget,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 91,
    .n_children = 9,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 14,
        .line_info = fun_data_tdui__Widget + 3,
        .line_info_top = fun_data_tdui__Widget + 3,
        .opcodes = fun_data_tdui__Widget + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Label
static const byte fun_data_tdui_Label[23] = {
    0x08,0x03, // prelude
    0x0f, // names: Label
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x0f, // LOAD_CONST_STRING 'Label'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0xb0, // LOAD_FAST 0
    0x20,0x00,0x01, // MAKE_CLOSURE 0
    0x16,0x59, // STORE_NAME '__init__'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_Label
// frozen bytecode for file tdui.py, scope tdui_Label___init__
static const byte fun_data_tdui_Label___init__[41] = {
    0x8b,0x14,0x18, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x50,0x5f, // names: __init__, *, self, win, x, y, text, width
     // code info
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb6, // LOAD_FAST 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb5, // LOAD_FAST 5
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Label___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Label___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 41,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 18,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Label___init__ + 15,
        .line_info_top = fun_data_tdui_Label___init__ + 15,
        .opcodes = fun_data_tdui_Label___init__ + 15,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Label___init__ fun_data_tdui_Label___init__[0]
#endif

static const mp_raw_code_t *const children_tdui_Label[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Label___init__,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Label = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Label,
    .children = (void *)&children_tdui_Label,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 23,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 15,
        .line_info = fun_data_tdui_Label + 3,
        .line_info_top = fun_data_tdui_Label + 3,
        .opcodes = fun_data_tdui_Label + 4,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Button
static const byte fun_data_tdui_Button[27] = {
    0x08,0x03, // prelude
    0x10, // names: Button
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x10, // LOAD_CONST_STRING 'Button'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0xb0, // LOAD_FAST 0
    0x20,0x00,0x01, // MAKE_CLOSURE 0
    0x16,0x59, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x44, // STORE_NAME '_event'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_Button
// frozen bytecode for file tdui.py, scope tdui_Button___init__
static const byte fun_data_tdui_Button___init__[45] = {
    0x8b,0x14,0x18, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x50,0x7e, // names: __init__, *, self, win, x, y, text, on_click
     // code info
    0xb6, // LOAD_FAST 6
    0xb1, // LOAD_FAST 1
    0x18,0x7e, // STORE_ATTR 'on_click'
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb5, // LOAD_FAST 5
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Button___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Button___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 45,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 18,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Button___init__ + 15,
        .line_info_top = fun_data_tdui_Button___init__ + 15,
        .opcodes = fun_data_tdui_Button___init__ + 15,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Button___init__ fun_data_tdui_Button___init__[0]
#endif

// child of tdui_Button
// frozen bytecode for file tdui.py, scope tdui_Button__event
static const byte fun_data_tdui_Button__event[37] = {
    0xcb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0xb1, // LOAD_FAST 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0xb0, // LOAD_FAST 0
    0x13,0x7e, // LOAD_ATTR 'on_click'
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0xb0, // LOAD_FAST 0
    0x14,0x7e, // LOAD_METHOD 'on_click'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Button__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Button__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 37,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Button__event + 18,
        .line_info_top = fun_data_tdui_Button__event + 18,
        .opcodes = fun_data_tdui_Button__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Button__event fun_data_tdui_Button__event[0]
#endif

static const mp_raw_code_t *const children_tdui_Button[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Button___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Button__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Button = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Button,
    .children = (void *)&children_tdui_Button,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 27,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 16,
        .line_info = fun_data_tdui_Button + 3,
        .line_info_top = fun_data_tdui_Button + 3,
        .opcodes = fun_data_tdui_Button + 4,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Checkbox
static const byte fun_data_tdui_Checkbox[49] = {
    0x10,0x03, // prelude
    0x11, // names: Checkbox
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x11, // LOAD_CONST_STRING 'Checkbox'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0xb0, // LOAD_FAST 0
    0x20,0x00,0x01, // MAKE_CLOSURE 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x01, // MAKE_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x01, // STORE_NAME 'checked'
    0x11,0x81,0x01, // LOAD_NAME 'checked'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x02, // MAKE_FUNCTION 2
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x01, // STORE_NAME 'checked'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x44, // STORE_NAME '_event'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_Checkbox
// frozen bytecode for file tdui.py, scope tdui_Checkbox___init__
static const byte fun_data_tdui_Checkbox___init__[65] = {
    0x90,0x90,0x04,0x1e, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x50,0x81,0x01,0x81,0x00, // names: __init__, *, self, win, x, y, text, checked, on_change
     // code info
    0x12,0x81,0x61, // LOAD_GLOBAL 'bool'
    0xb6, // LOAD_FAST 6
    0x34,0x01, // CALL_FUNCTION 1
    0xb1, // LOAD_FAST 1
    0x18,0x7f, // STORE_ATTR '_checked'
    0xb7, // LOAD_FAST 7
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x00, // STORE_ATTR 'on_change'
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x83, // LOAD_CONST_SMALL_INT 3
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb6, // LOAD_FAST 6
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb5, // LOAD_FAST 5
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Checkbox___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Checkbox___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 65,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 19,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Checkbox___init__ + 19,
        .line_info_top = fun_data_tdui_Checkbox___init__ + 19,
        .opcodes = fun_data_tdui_Checkbox___init__ + 19,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Checkbox___init__ fun_data_tdui_Checkbox___init__[0]
#endif

// child of tdui_Checkbox
// frozen bytecode for file tdui.py, scope tdui_Checkbox_checked
static const byte fun_data_tdui_Checkbox_checked[10] = {
    0x09,0x08, // prelude
    0x81,0x01,0x82,0x10, // names: checked, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x7f, // LOAD_ATTR '_checked'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Checkbox_checked = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Checkbox_checked,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 10,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 129,
        .line_info = fun_data_tdui_Checkbox_checked + 6,
        .line_info_top = fun_data_tdui_Checkbox_checked + 6,
        .opcodes = fun_data_tdui_Checkbox_checked + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Checkbox_checked fun_data_tdui_Checkbox_checked[0]
#endif

// child of tdui_Checkbox
// frozen bytecode for file tdui.py, scope tdui_Checkbox_checked2
static const byte fun_data_tdui_Checkbox_checked2[33] = {
    0x2a,0x0c, // prelude
    0x81,0x01,0x82,0x10,0x82,0x15, // names: checked, self, v
     // code info
    0x12,0x81,0x61, // LOAD_GLOBAL 'bool'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x7f, // STORE_ATTR '_checked'
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x85, // LOAD_CONST_SMALL_INT 5
    0xb1, // LOAD_FAST 1
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Checkbox_checked2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Checkbox_checked2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 33,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 129,
        .line_info = fun_data_tdui_Checkbox_checked2 + 8,
        .line_info_top = fun_data_tdui_Checkbox_checked2 + 8,
        .opcodes = fun_data_tdui_Checkbox_checked2 + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Checkbox_checked2 fun_data_tdui_Checkbox_checked2[0]
#endif

// child of tdui_Checkbox
// frozen bytecode for file tdui.py, scope tdui_Checkbox__event
static const byte fun_data_tdui_Checkbox__event[48] = {
    0xcb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0xb1, // LOAD_FAST 1
    0x82, // LOAD_CONST_SMALL_INT 2
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x57, // POP_JUMP_IF_FALSE 23
    0x12,0x81,0x61, // LOAD_GLOBAL 'bool'
    0xb2, // LOAD_FAST 2
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x7f, // STORE_ATTR '_checked'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x00, // LOAD_ATTR 'on_change'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x00, // LOAD_METHOD 'on_change'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Checkbox__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Checkbox__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 48,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Checkbox__event + 18,
        .line_info_top = fun_data_tdui_Checkbox__event + 18,
        .opcodes = fun_data_tdui_Checkbox__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Checkbox__event fun_data_tdui_Checkbox__event[0]
#endif

static const mp_raw_code_t *const children_tdui_Checkbox[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Checkbox___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Checkbox_checked,
    (const mp_raw_code_t *)&proto_fun_tdui_Checkbox_checked2,
    (const mp_raw_code_t *)&proto_fun_tdui_Checkbox__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Checkbox = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Checkbox,
    .children = (void *)&children_tdui_Checkbox,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 49,
    .n_children = 4,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 17,
        .line_info = fun_data_tdui_Checkbox + 3,
        .line_info_top = fun_data_tdui_Checkbox + 3,
        .opcodes = fun_data_tdui_Checkbox + 4,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Textbox
static const byte fun_data_tdui_Textbox[27] = {
    0x08,0x03, // prelude
    0x12, // names: Textbox
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x12, // LOAD_CONST_STRING 'Textbox'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0xb0, // LOAD_FAST 0
    0x20,0x00,0x01, // MAKE_CLOSURE 0
    0x16,0x59, // STORE_NAME '__init__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x44, // STORE_NAME '_event'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_Textbox
// frozen bytecode for file tdui.py, scope tdui_Textbox___init__
static const byte fun_data_tdui_Textbox___init__[64] = {
    0xa2,0x90,0x04,0x24, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x5f,0x50,0x82,0x0f,0x81,0x02,0x81,0x00, // names: __init__, *, self, win, x, y, width, text, secret, on_enter, on_change
     // code info
    0xb8, // LOAD_FAST 8
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x02, // STORE_ATTR 'on_enter'
    0xb9, // LOAD_FAST 9
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x00, // STORE_ATTR 'on_change'
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x84, // LOAD_CONST_SMALL_INT 4
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb6, // LOAD_FAST 6
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Textbox___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Textbox___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 64,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 21,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 10,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Textbox___init__ + 22,
        .line_info_top = fun_data_tdui_Textbox___init__ + 22,
        .opcodes = fun_data_tdui_Textbox___init__ + 22,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Textbox___init__ fun_data_tdui_Textbox___init__[0]
#endif

// child of tdui_Textbox
// frozen bytecode for file tdui.py, scope tdui_Textbox__event
static const byte fun_data_tdui_Textbox__event[70] = {
    0xcb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0xb1, // LOAD_FAST 1
    0x82, // LOAD_CONST_SMALL_INT 2
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0xb6, // LOAD_FAST 6
    0x45,0x02, // JUMP_IF_TRUE_OR_POP 2
    0x10,0x1c, // LOAD_CONST_STRING ''
    0xb0, // LOAD_FAST 0
    0x18,0x79, // STORE_ATTR '_text'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x00, // LOAD_ATTR 'on_change'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x00, // LOAD_METHOD 'on_change'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x55, // JUMP 21
    0xb1, // LOAD_FAST 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x50, // POP_JUMP_IF_FALSE 16
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x02, // LOAD_ATTR 'on_enter'
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x02, // LOAD_METHOD 'on_enter'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x40, // JUMP 0
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Textbox__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Textbox__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 70,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Textbox__event + 18,
        .line_info_top = fun_data_tdui_Textbox__event + 18,
        .opcodes = fun_data_tdui_Textbox__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Textbox__event fun_data_tdui_Textbox__event[0]
#endif

static const mp_raw_code_t *const children_tdui_Textbox[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Textbox___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Textbox__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Textbox = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Textbox,
    .children = (void *)&children_tdui_Textbox,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 27,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 18,
        .line_info = fun_data_tdui_Textbox + 3,
        .line_info_top = fun_data_tdui_Textbox + 3,
        .opcodes = fun_data_tdui_Textbox + 4,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Progress
static const byte fun_data_tdui_Progress[45] = {
    0x10,0x03, // prelude
    0x13, // names: Progress
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x13, // LOAD_CONST_STRING 'Progress'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0xb0, // LOAD_FAST 0
    0x20,0x00,0x01, // MAKE_CLOSURE 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x01, // MAKE_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x04, // STORE_NAME 'value'
    0x11,0x81,0x04, // LOAD_NAME 'value'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x02, // MAKE_FUNCTION 2
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x04, // STORE_NAME 'value'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_Progress
// frozen bytecode for file tdui.py, scope tdui_Progress___init__
static const byte fun_data_tdui_Progress___init__[43] = {
    0x83,0x14,0x1a, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x5f,0x81,0x04, // names: __init__, *, self, win, x, y, width, value
     // code info
    0xb6, // LOAD_FAST 6
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x03, // STORE_ATTR '_value'
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x85, // LOAD_CONST_SMALL_INT 5
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb6, // LOAD_FAST 6
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Progress___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Progress___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 43,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Progress___init__ + 16,
        .line_info_top = fun_data_tdui_Progress___init__ + 16,
        .opcodes = fun_data_tdui_Progress___init__ + 16,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Progress___init__ fun_data_tdui_Progress___init__[0]
#endif

// child of tdui_Progress
// frozen bytecode for file tdui.py, scope tdui_Progress_value
static const byte fun_data_tdui_Progress_value[11] = {
    0x09,0x08, // prelude
    0x81,0x04,0x82,0x10, // names: value, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x03, // LOAD_ATTR '_value'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Progress_value = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Progress_value,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 11,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 132,
        .line_info = fun_data_tdui_Progress_value + 6,
        .line_info_top = fun_data_tdui_Progress_value + 6,
        .opcodes = fun_data_tdui_Progress_value + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Progress_value fun_data_tdui_Progress_value[0]
#endif

// child of tdui_Progress
// frozen bytecode for file tdui.py, scope tdui_Progress_value2
static const byte fun_data_tdui_Progress_value2[45] = {
    0x3a,0x0c, // prelude
    0x81,0x04,0x82,0x10,0x82,0x15, // names: value, self, v
     // code info
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x09, // LOAD_GLOBAL 'min'
    0x22,0x80,0x64, // LOAD_CONST_SMALL_INT 100
    0x12,0x82,0x19, // LOAD_GLOBAL 'int'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x02, // CALL_FUNCTION 2
    0x34,0x02, // CALL_FUNCTION 2
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x03, // STORE_ATTR '_value'
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x85, // LOAD_CONST_SMALL_INT 5
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x03, // LOAD_ATTR '_value'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Progress_value2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Progress_value2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 45,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 132,
        .line_info = fun_data_tdui_Progress_value2 + 8,
        .line_info_top = fun_data_tdui_Progress_value2 + 8,
        .opcodes = fun_data_tdui_Progress_value2 + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Progress_value2 fun_data_tdui_Progress_value2[0]
#endif

static const mp_raw_code_t *const children_tdui_Progress[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Progress___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Progress_value,
    (const mp_raw_code_t *)&proto_fun_tdui_Progress_value2,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Progress = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Progress,
    .children = (void *)&children_tdui_Progress,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 45,
    .n_children = 3,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 19,
        .line_info = fun_data_tdui_Progress + 3,
        .line_info_top = fun_data_tdui_Progress + 3,
        .opcodes = fun_data_tdui_Progress + 4,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_List
static const byte fun_data_tdui_List[83] = {
    0x10,0x03, // prelude
    0x14, // names: List
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x14, // LOAD_CONST_STRING 'List'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x82, // LOAD_CONST_SMALL_INT 2
    0x16,0x70, // STORE_NAME '_n'
    0xb0, // LOAD_FAST 0
    0x20,0x00,0x01, // MAKE_CLOSURE 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x01, // MAKE_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x0a, // STORE_NAME 'items'
    0x11,0x81,0x0a, // LOAD_NAME 'items'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x02, // MAKE_FUNCTION 2
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x0a, // STORE_NAME 'items'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x5b, // STORE_NAME 'append'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x81,0x0b, // STORE_NAME '_add'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x05, // MAKE_FUNCTION 5
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x0d, // STORE_NAME 'selected'
    0x11,0x81,0x0d, // LOAD_NAME 'selected'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x06, // MAKE_FUNCTION 6
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x0d, // STORE_NAME 'selected'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x44, // STORE_NAME '_event'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List___init__
static const byte fun_data_tdui_List___init__[86] = {
    0x9a,0x90,0x04,0x28, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x0a,0x81,0x06,0x81,0x07, // names: __init__, *, self, win, x, y, w, h, items, on_select, on_activate
     // code info
    0xb5, // LOAD_FAST 5
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x10,0x81,0x05, // LOAD_CONST_STRING 'list size'
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0xb8, // LOAD_FAST 8
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x06, // STORE_ATTR 'on_select'
    0xb9, // LOAD_FAST 9
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x07, // STORE_ATTR 'on_activate'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x08, // STORE_ATTR '_selected'
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x86, // LOAD_CONST_SMALL_INT 6
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x2b,0x00, // BUILD_LIST 0
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x09, // STORE_ATTR '_items'
    0xb7, // LOAD_FAST 7
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x0a, // STORE_ATTR 'items'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 86,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 20,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 10,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_List___init__ + 24,
        .line_info_top = fun_data_tdui_List___init__ + 24,
        .opcodes = fun_data_tdui_List___init__ + 24,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List___init__ fun_data_tdui_List___init__[0]
#endif

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List_items
static const byte fun_data_tdui_List_items[11] = {
    0x09,0x08, // prelude
    0x81,0x0a,0x82,0x10, // names: items, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x09, // LOAD_ATTR '_items'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List_items = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List_items,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 11,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 138,
        .line_info = fun_data_tdui_List_items + 6,
        .line_info_top = fun_data_tdui_List_items + 6,
        .opcodes = fun_data_tdui_List_items + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List_items fun_data_tdui_List_items[0]
#endif

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List_items2
static const byte fun_data_tdui_List_items2[37] = {
    0x22,0x0c, // prelude
    0x81,0x0a,0x82,0x10,0x81,0x0a, // names: items, self, items
     // code info
    0x32,0x00, // MAKE_FUNCTION 0
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x09, // STORE_ATTR '_items'
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x89, // LOAD_CONST_SMALL_INT 9
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x0b, // LOAD_METHOD '_add'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x09, // LOAD_ATTR '_items'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_List_items2
// frozen bytecode for file tdui.py, scope tdui_List_items2__lt_listcomp_gt_
static const byte fun_data_tdui_List_items2__lt_listcomp_gt_[24] = {
    0x41,0x08, // prelude
    0x81,0x5f,0x82,0x17, // names: <listcomp>, *
     // code info
    0x2b,0x00, // BUILD_LIST 0
    0xb0, // LOAD_FAST 0
    0x5f, // GET_ITER_STACK
    0x4b,0x0b, // FOR_ITER 11
    0xc1, // STORE_FAST 1
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x2f,0x14, // STORE_COMP 20
    0x42,0x33, // JUMP -13
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List_items2__lt_listcomp_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List_items2__lt_listcomp_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 223,
        .line_info = fun_data_tdui_List_items2__lt_listcomp_gt_ + 6,
        .line_info_top = fun_data_tdui_List_items2__lt_listcomp_gt_ + 6,
        .opcodes = fun_data_tdui_List_items2__lt_listcomp_gt_ + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List_items2__lt_listcomp_gt_ fun_data_tdui_List_items2__lt_listcomp_gt_[0]
#endif

static const mp_raw_code_t *const children_tdui_List_items2[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_List_items2__lt_listcomp_gt_,
};

static const mp_raw_code_truncated_t proto_fun_tdui_List_items2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List_items2,
    .children = (void *)&children_tdui_List_items2,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 37,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 138,
        .line_info = fun_data_tdui_List_items2 + 8,
        .line_info_top = fun_data_tdui_List_items2 + 8,
        .opcodes = fun_data_tdui_List_items2 + 8,
    },
    #endif
    #endif
};

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List_append
static const byte fun_data_tdui_List_append[36] = {
    0x22,0x0a, // prelude
    0x5b,0x82,0x10,0x82,0x1a, // names: append, self, item
     // code info
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xc1, // STORE_FAST 1
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x09, // LOAD_ATTR '_items'
    0x14,0x5b, // LOAD_METHOD 'append'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x0b, // LOAD_METHOD '_add'
    0xb1, // LOAD_FAST 1
    0x2b,0x01, // BUILD_LIST 1
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List_append = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List_append,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 36,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 91,
        .line_info = fun_data_tdui_List_append + 7,
        .line_info_top = fun_data_tdui_List_append + 7,
        .opcodes = fun_data_tdui_List_append + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List_append fun_data_tdui_List_append[0]
#endif

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List__add
static const byte fun_data_tdui_List__add[111] = {
    0x7a,0x0c, // prelude
    0x81,0x0b,0x82,0x10,0x81,0x0a, // names: _add, self, items
     // code info
    0x2b,0x00, // BUILD_LIST 0
    0xc2, // STORE_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xc3, // STORE_FAST 3
    0xb1, // LOAD_FAST 1
    0x5f, // GET_ITER_STACK
    0x4b,0x48, // FOR_ITER 72
    0xc4, // STORE_FAST 4
    0xb4, // LOAD_FAST 4
    0x14,0x81,0x0c, // LOAD_METHOD 'replace'
    0x10,0x32, // LOAD_CONST_STRING '\n'
    0x10,0x33, // LOAD_CONST_STRING ' '
    0x36,0x02, // CALL_METHOD 2
    0xc4, // STORE_FAST 4
    0xb2, // LOAD_FAST 2
    0x44,0x64, // POP_JUMP_IF_FALSE 36
    0xb3, // LOAD_FAST 3
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb4, // LOAD_FAST 4
    0x34,0x01, // CALL_FUNCTION 1
    0xf2, // BINARY_OP 27 __add__
    0x22,0x87,0x04, // LOAD_CONST_SMALL_INT 900
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x8a, // LOAD_CONST_SMALL_INT 10
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0x10,0x32, // LOAD_CONST_STRING '\n'
    0x14,0x58, // LOAD_METHOD 'join'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x36,0x82,0x01, // CALL_METHOD 257
    0x59, // POP_TOP
    0x2b,0x00, // BUILD_LIST 0
    0xc2, // STORE_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xc3, // STORE_FAST 3
    0xb2, // LOAD_FAST 2
    0x14,0x5b, // LOAD_METHOD 'append'
    0xb4, // LOAD_FAST 4
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb3, // LOAD_FAST 3
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb4, // LOAD_FAST 4
    0x34,0x01, // CALL_FUNCTION 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf2, // BINARY_OP 27 __add__
    0xe5, // BINARY_OP 14 __iadd__
    0xc3, // STORE_FAST 3
    0x42,0xb6,0x7f, // JUMP -74
    0xb2, // LOAD_FAST 2
    0x44,0x51, // POP_JUMP_IF_FALSE 17
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x8a, // LOAD_CONST_SMALL_INT 10
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0x10,0x32, // LOAD_CONST_STRING '\n'
    0x14,0x58, // LOAD_METHOD 'join'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x36,0x82,0x01, // CALL_METHOD 257
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List__add = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List__add,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 111,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 16,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 139,
        .line_info = fun_data_tdui_List__add + 8,
        .line_info_top = fun_data_tdui_List__add + 8,
        .opcodes = fun_data_tdui_List__add + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List__add fun_data_tdui_List__add[0]
#endif

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List_selected
static const byte fun_data_tdui_List_selected[11] = {
    0x09,0x08, // prelude
    0x81,0x0d,0x82,0x10, // names: selected, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x08, // LOAD_ATTR '_selected'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List_selected = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List_selected,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 11,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 141,
        .line_info = fun_data_tdui_List_selected + 6,
        .line_info_top = fun_data_tdui_List_selected + 6,
        .opcodes = fun_data_tdui_List_selected + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List_selected fun_data_tdui_List_selected[0]
#endif

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List_selected2
static const byte fun_data_tdui_List_selected2[31] = {
    0x2a,0x0c, // prelude
    0x81,0x0d,0x82,0x10,0x82,0x1b, // names: selected, self, i
     // code info
    0x12,0x82,0x19, // LOAD_GLOBAL 'int'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x08, // STORE_ATTR '_selected'
    0xb0, // LOAD_FAST 0
    0x14,0x7b, // LOAD_METHOD '_req'
    0x85, // LOAD_CONST_SMALL_INT 5
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x08, // LOAD_ATTR '_selected'
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List_selected2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List_selected2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 31,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 141,
        .line_info = fun_data_tdui_List_selected2 + 8,
        .line_info_top = fun_data_tdui_List_selected2 + 8,
        .opcodes = fun_data_tdui_List_selected2 + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List_selected2 fun_data_tdui_List_selected2[0]
#endif

// child of tdui_List
// frozen bytecode for file tdui.py, scope tdui_List__event
static const byte fun_data_tdui_List__event[72] = {
    0xcb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0xb1, // LOAD_FAST 1
    0x82, // LOAD_CONST_SMALL_INT 2
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x55, // POP_JUMP_IF_FALSE 21
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x08, // STORE_ATTR '_selected'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x06, // LOAD_ATTR 'on_select'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x06, // LOAD_METHOD 'on_select'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x5a, // JUMP 26
    0xb1, // LOAD_FAST 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x55, // POP_JUMP_IF_FALSE 21
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x08, // STORE_ATTR '_selected'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x07, // LOAD_ATTR 'on_activate'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x07, // LOAD_METHOD 'on_activate'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x40, // JUMP 0
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_List__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 72,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 10,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_List__event + 18,
        .line_info_top = fun_data_tdui_List__event + 18,
        .opcodes = fun_data_tdui_List__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_List__event fun_data_tdui_List__event[0]
#endif

static const mp_raw_code_t *const children_tdui_List[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_List___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_List_items,
    (const mp_raw_code_t *)&proto_fun_tdui_List_items2,
    (const mp_raw_code_t *)&proto_fun_tdui_List_append,
    (const mp_raw_code_t *)&proto_fun_tdui_List__add,
    (const mp_raw_code_t *)&proto_fun_tdui_List_selected,
    (const mp_raw_code_t *)&proto_fun_tdui_List_selected2,
    (const mp_raw_code_t *)&proto_fun_tdui_List__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui_List = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_List,
    .children = (void *)&children_tdui_List,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 83,
    .n_children = 8,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 20,
        .line_info = fun_data_tdui_List + 3,
        .line_info_top = fun_data_tdui_List + 3,
        .opcodes = fun_data_tdui_List + 4,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_rgb
static const byte fun_data_tdui_rgb[19] = {
    0x3b,0x0e, // prelude
    0x2d,0x82,0x01,0x82,0x02,0x81,0x74, // names: rgb, r, g, b
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x2d, // LOAD_METHOD 'rgb'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x36,0x03, // CALL_METHOD 3
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_rgb = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_rgb,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 19,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 45,
        .line_info = fun_data_tdui_rgb + 9,
        .line_info_top = fun_data_tdui_rgb + 9,
        .opcodes = fun_data_tdui_rgb + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_rgb fun_data_tdui_rgb[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__part
static const byte fun_data_tdui__part[53] = {
    0x53,0x0e, // prelude
    0x2e,0x81,0x25,0x81,0x26,0x81,0x33, // names: _part, w, h, border
     // code info
    0x12,0x82,0x03, // LOAD_GLOBAL 'bytes'
    0xb0, // LOAD_FAST 0
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0xb0, // LOAD_FAST 0
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf1, // BINARY_OP 26 __rshift__
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0xb1, // LOAD_FAST 1
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0xb1, // LOAD_FAST 1
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf1, // BINARY_OP 26 __rshift__
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0xb2, // LOAD_FAST 2
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0xb2, // LOAD_FAST 2
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf1, // BINARY_OP 26 __rshift__
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0x2a,0x06, // BUILD_TUPLE 6
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__part = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__part,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 53,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 46,
        .line_info = fun_data_tdui__part + 9,
        .line_info_top = fun_data_tdui__part + 9,
        .opcodes = fun_data_tdui__part + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__part fun_data_tdui__part[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__Batch
static const byte fun_data_tdui__Batch[26] = {
    0x00,0x02, // prelude
    0x15, // names: _Batch
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x15, // LOAD_CONST_STRING '_Batch'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x81,0x0e, // STORE_NAME '__enter__'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x81,0x0f, // STORE_NAME '__exit__'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui__Batch
// frozen bytecode for file tdui.py, scope tdui__Batch___enter__
static const byte fun_data_tdui__Batch___enter__[17] = {
    0x21,0x08, // prelude
    0x81,0x0e,0x82,0x10, // names: __enter__, self
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x9f, // LOAD_CONST_SMALL_INT 31
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Batch___enter__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Batch___enter__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 17,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 142,
        .line_info = fun_data_tdui__Batch___enter__ + 6,
        .line_info_top = fun_data_tdui__Batch___enter__ + 6,
        .opcodes = fun_data_tdui__Batch___enter__ + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Batch___enter__ fun_data_tdui__Batch___enter__[0]
#endif

// child of tdui__Batch
// frozen bytecode for file tdui.py, scope tdui__Batch___exit__
static const byte fun_data_tdui__Batch___exit__[24] = {
    0xb8,0x04,0x14, // prelude
    0x81,0x0f,0x82,0x10,0x82,0x14,0x81,0x04,0x82,0x1c, // names: __exit__, self, kind, value, traceback
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0xa0, // LOAD_CONST_SMALL_INT 32
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x02, // CALL_METHOD 2
    0x59, // POP_TOP
    0x50, // LOAD_CONST_FALSE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Batch___exit__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Batch___exit__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 143,
        .line_info = fun_data_tdui__Batch___exit__ + 13,
        .line_info_top = fun_data_tdui__Batch___exit__ + 13,
        .opcodes = fun_data_tdui__Batch___exit__ + 13,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Batch___exit__ fun_data_tdui__Batch___exit__[0]
#endif

static const mp_raw_code_t *const children_tdui__Batch[] = {
    (const mp_raw_code_t *)&proto_fun_tdui__Batch___enter__,
    (const mp_raw_code_t *)&proto_fun_tdui__Batch___exit__,
};

static const mp_raw_code_truncated_t proto_fun_tdui__Batch = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Batch,
    .children = (void *)&children_tdui__Batch,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 21,
        .line_info = fun_data_tdui__Batch + 3,
        .line_info_top = fun_data_tdui__Batch + 3,
        .opcodes = fun_data_tdui__Batch + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_batch
static const byte fun_data_tdui_batch[7] = {
    0x00,0x02, // prelude
    0x2f, // names: batch
     // code info
    0x12,0x81,0x6e, // LOAD_GLOBAL '_batch'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_batch = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_batch,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 7,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 1,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 47,
        .line_info = fun_data_tdui_batch + 3,
        .line_info_top = fun_data_tdui_batch + 3,
        .opcodes = fun_data_tdui_batch + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_batch fun_data_tdui_batch[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__Pixels
static const byte fun_data_tdui__Pixels[119] = {
    0x28,0x02, // prelude
    0x16, // names: _Pixels
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x16, // LOAD_CONST_STRING '_Pixels'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x81,0x10, // STORE_NAME '_gradient_data'
    0x32,0x01, // MAKE_FUNCTION 1
    0x16,0x2f, // STORE_NAME 'batch'
    0x22,0x80,0x64, // LOAD_CONST_SMALL_INT 100
    0xa8, // LOAD_CONST_SMALL_INT 40
    0x22,0x81,0x6d, // LOAD_CONST_SMALL_INT 237
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x2a,0x06, // BUILD_TUPLE 6
    0x53, // LOAD_NULL
    0x33,0x02, // MAKE_FUNCTION_DEFARGS 2
    0x16,0x81,0x14, // STORE_NAME 'bar'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x50, // LOAD_CONST_FALSE
    0x51, // LOAD_CONST_NONE
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x03, // MAKE_FUNCTION_DEFARGS 3
    0x16,0x81,0x17, // STORE_NAME 'rect'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x81,0x18, // STORE_NAME 'line'
    0x50, // LOAD_CONST_FALSE
    0x51, // LOAD_CONST_NONE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x05, // MAKE_FUNCTION_DEFARGS 5
    0x16,0x81,0x19, // STORE_NAME 'circle'
    0x50, // LOAD_CONST_FALSE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x06, // MAKE_FUNCTION_DEFARGS 6
    0x16,0x81,0x1a, // STORE_NAME 'gradient'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x07, // MAKE_FUNCTION_DEFARGS 7
    0x16,0x81,0x1b, // STORE_NAME 'image'
    0x51, // LOAD_CONST_NONE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x08, // MAKE_FUNCTION_DEFARGS 8
    0x16,0x81,0x1c, // STORE_NAME 'nine'
    0x7e, // LOAD_CONST_SMALL_INT -2
    0x81, // LOAD_CONST_SMALL_INT 1
    0x50, // LOAD_CONST_FALSE
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x09, // MAKE_FUNCTION_DEFARGS 9
    0x16,0x81,0x1d, // STORE_NAME 'draw_text'
    0x50, // LOAD_CONST_FALSE
    0x52, // LOAD_CONST_TRUE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x0a, // MAKE_FUNCTION_DEFARGS 10
    0x16,0x81,0x1e, // STORE_NAME 'pixels'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels__gradient_data
static const byte fun_data_tdui__Pixels__gradient_data[98] = {
    0x52,0x0c, // prelude
    0x81,0x10,0x82,0x10,0x81,0x1a, // names: _gradient_data, self, gradient
     // code info
    0xb1, // LOAD_FAST 1
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x23,0x07, // LOAD_CONST_OBJ 7
    0x63, // RETURN_VALUE
    0x12,0x82,0x1d, // LOAD_GLOBAL 'tuple'
    0xb1, // LOAD_FAST 1
    0x10,0x81,0x11, // LOAD_CONST_STRING 'rgb1'
    0x55, // LOAD_SUBSCR
    0x34,0x01, // CALL_FUNCTION 1
    0x12,0x82,0x1d, // LOAD_GLOBAL 'tuple'
    0xb1, // LOAD_FAST 1
    0x10,0x81,0x12, // LOAD_CONST_STRING 'rgb2'
    0x55, // LOAD_SUBSCR
    0x34,0x01, // CALL_FUNCTION 1
    0xf2, // BINARY_OP 27 __add__
    0xc2, // STORE_FAST 2
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb2, // LOAD_FAST 2
    0x34,0x01, // CALL_FUNCTION 1
    0x86, // LOAD_CONST_SMALL_INT 6
    0xdc, // BINARY_OP 5 __ne__
    0x43,0x4d, // POP_JUMP_IF_TRUE 13
    0x12,0x82,0x1e, // LOAD_GLOBAL 'any'
    0x32,0x00, // MAKE_FUNCTION 0
    0xb2, // LOAD_FAST 2
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x08, // LOAD_CONST_OBJ 8
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x82,0x03, // LOAD_GLOBAL 'bytes'
    0xb2, // LOAD_FAST 2
    0x12,0x82,0x19, // LOAD_GLOBAL 'int'
    0x12,0x81,0x61, // LOAD_GLOBAL 'bool'
    0xb1, // LOAD_FAST 1
    0x14,0x43, // LOAD_METHOD 'get'
    0x10,0x81,0x13, // LOAD_CONST_STRING 'across'
    0x50, // LOAD_CONST_FALSE
    0x36,0x02, // CALL_METHOD 2
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x2a,0x01, // BUILD_TUPLE 1
    0xf2, // BINARY_OP 27 __add__
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
// child of tdui__Pixels__gradient_data
// frozen bytecode for file tdui.py, scope tdui__Pixels__gradient_data__lt_genexpr_gt_
static const byte fun_data_tdui__Pixels__gradient_data__lt_genexpr_gt_[36] = {
    0xc1,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x15, // FOR_ITER 21
    0xc1, // STORE_FAST 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x06, // JUMP_IF_FALSE_OR_POP 6
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0xd3, // UNARY_OP 3 <not>
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x29, // JUMP -23
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels__gradient_data__lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui__Pixels__gradient_data__lt_genexpr_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 36,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui__Pixels__gradient_data__lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_tdui__Pixels__gradient_data__lt_genexpr_gt_ + 7,
        .opcodes = fun_data_tdui__Pixels__gradient_data__lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels__gradient_data__lt_genexpr_gt_ fun_data_tdui__Pixels__gradient_data__lt_genexpr_gt_[0]
#endif

static const mp_raw_code_t *const children_tdui__Pixels__gradient_data[] = {
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels__gradient_data__lt_genexpr_gt_,
};

static const mp_raw_code_truncated_t proto_fun_tdui__Pixels__gradient_data = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels__gradient_data,
    .children = (void *)&children_tdui__Pixels__gradient_data,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 98,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 144,
        .line_info = fun_data_tdui__Pixels__gradient_data + 8,
        .line_info_top = fun_data_tdui__Pixels__gradient_data + 8,
        .opcodes = fun_data_tdui__Pixels__gradient_data + 8,
    },
    #endif
    #endif
};

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_batch
static const byte fun_data_tdui__Pixels_batch[9] = {
    0x09,0x06, // prelude
    0x2f,0x82,0x10, // names: batch, self
     // code info
    0x12,0x81,0x6e, // LOAD_GLOBAL '_batch'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_batch = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_batch,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 9,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 47,
        .line_info = fun_data_tdui__Pixels_batch + 5,
        .line_info_top = fun_data_tdui__Pixels_batch + 5,
        .opcodes = fun_data_tdui__Pixels_batch + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_batch fun_data_tdui__Pixels_batch[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_bar
static const byte fun_data_tdui__Pixels_bar[230] = {
    0xe8,0x94,0x85,0x01,0x32, // prelude
    0x81,0x14,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x04,0x82,0x1f,0x7d,0x82,0x20,0x82,0x21,0x81,0x2d,0x81,0x1a, // names: bar, self, x, y, w, h, value, maximum, color, track, edge, radius, gradient
     // code info
    0xb6, // LOAD_FAST 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0xda, // BINARY_OP 3 __le__
    0x43,0x78, // POP_JUMP_IF_TRUE 56
    0xba, // LOAD_FAST 10
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x73, // POP_JUMP_IF_TRUE 51
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb7, // LOAD_FAST 7
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x06, // JUMP_IF_FALSE_OR_POP 6
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x44,0x62, // POP_JUMP_IF_FALSE 34
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb8, // LOAD_FAST 8
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x06, // JUMP_IF_FALSE_OR_POP 6
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x44,0x51, // POP_JUMP_IF_FALSE 17
    0x7f, // LOAD_CONST_SMALL_INT -1
    0xb9, // LOAD_FAST 9
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x06, // JUMP_IF_FALSE_OR_POP 6
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x43,0x48, // POP_JUMP_IF_TRUE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x09, // LOAD_CONST_OBJ 9
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0xf8,0x80, // POP_JUMP_IF_FALSE 120
    0x12,0x82,0x09, // LOAD_GLOBAL 'min'
    0x22,0x87,0x68, // LOAD_CONST_SMALL_INT 1000
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x19, // LOAD_GLOBAL 'int'
    0xb5, // LOAD_FAST 5
    0x22,0x87,0x68, // LOAD_CONST_SMALL_INT 1000
    0xf4, // BINARY_OP 29 __mul__
    0xb6, // LOAD_FAST 6
    0xf6, // BINARY_OP 31 __floordiv__
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x02, // CALL_FUNCTION 2
    0x34,0x02, // CALL_FUNCTION 2
    0xcc, // STORE_FAST 12
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x16, // LOAD_ATTR '_bar_colors'
    0xcd, // STORE_FAST 13
    0xbd, // LOAD_FAST 13
    0x44,0x5b, // POP_JUMP_IF_FALSE 27
    0xbd, // LOAD_FAST 13
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0xb7, // LOAD_FAST 7
    0xdc, // BINARY_OP 5 __ne__
    0x43,0x54, // POP_JUMP_IF_TRUE 20
    0xbd, // LOAD_FAST 13
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0xb8, // LOAD_FAST 8
    0xdc, // BINARY_OP 5 __ne__
    0x43,0x4d, // POP_JUMP_IF_TRUE 13
    0xbd, // LOAD_FAST 13
    0x82, // LOAD_CONST_SMALL_INT 2
    0x55, // LOAD_SUBSCR
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb9, // LOAD_FAST 9
    0x34,0x02, // CALL_FUNCTION 2
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0x12,0x82,0x03, // LOAD_GLOBAL 'bytes'
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb9, // LOAD_FAST 9
    0x34,0x02, // CALL_FUNCTION 2
    0x2a,0x03, // BUILD_TUPLE 3
    0x34,0x01, // CALL_FUNCTION 1
    0x57, // DUP_TOP
    0xcd, // STORE_FAST 13
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x16, // STORE_ATTR '_bar_colors'
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0xa1, // LOAD_CONST_SMALL_INT 33
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb9, // LOAD_FAST 9
    0x80, // LOAD_CONST_SMALL_INT 0
    0xdb, // BINARY_OP 4 __ge__
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xbc, // LOAD_FAST 12
    0xba, // LOAD_FAST 10
    0xbd, // LOAD_FAST 13
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x10, // LOAD_METHOD '_gradient_data'
    0xbb, // LOAD_FAST 11
    0x36,0x01, // CALL_METHOD 1
    0xf2, // BINARY_OP 27 __add__
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_bar = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_bar,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 230,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 30,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 12,
        .n_kwonly_args = 0,
        .n_def_pos_args = 6,
        .qstr_block_name_idx = 148,
        .line_info = fun_data_tdui__Pixels_bar + 30,
        .line_info_top = fun_data_tdui__Pixels_bar + 30,
        .opcodes = fun_data_tdui__Pixels_bar + 30,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_bar fun_data_tdui__Pixels_bar[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_rect
static const byte fun_data_tdui__Pixels_rect[73] = {
    0xb9,0x91,0x05,0x26, // prelude
    0x81,0x17,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x7d,0x81,0x2d,0x81,0x3b,0x81,0x1a, // names: rect, self, x, y, w, h, color, radius, outline, gradient
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x68, // POP_JUMP_IF_FALSE 40
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x99, // LOAD_CONST_SMALL_INT 25
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb8, // LOAD_FAST 8
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x82, // LOAD_CONST_SMALL_INT 2
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xed, // BINARY_OP 22 __or__
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x10, // LOAD_METHOD '_gradient_data'
    0xb8, // LOAD_FAST 8
    0x36,0x01, // CALL_METHOD 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_rect = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_rect,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 73,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 24,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 9,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 151,
        .line_info = fun_data_tdui__Pixels_rect + 23,
        .line_info_top = fun_data_tdui__Pixels_rect + 23,
        .opcodes = fun_data_tdui__Pixels_rect + 23,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_rect fun_data_tdui__Pixels_rect[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_line
static const byte fun_data_tdui__Pixels_line[45] = {
    0x8a,0x14,0x1a, // prelude
    0x81,0x18,0x82,0x10,0x82,0x22,0x82,0x23,0x82,0x24,0x82,0x25,0x7d, // names: line, self, x0, y0, x1, y1, color
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x53, // POP_JUMP_IF_FALSE 19
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x9a, // LOAD_CONST_SMALL_INT 26
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x0a, // CALL_METHOD 10
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_line = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_line,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 45,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 18,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 6,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 152,
        .line_info = fun_data_tdui__Pixels_line + 16,
        .line_info_top = fun_data_tdui__Pixels_line + 16,
        .opcodes = fun_data_tdui__Pixels_line + 16,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_line fun_data_tdui__Pixels_line[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_circle
static const byte fun_data_tdui__Pixels_circle[69] = {
    0xab,0x94,0x01,0x1e, // prelude
    0x81,0x19,0x82,0x10,0x81,0x23,0x81,0x24,0x82,0x01,0x7d,0x81,0x3b,0x81,0x1a, // names: circle, self, x, y, r, color, outline, gradient
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x68, // POP_JUMP_IF_FALSE 40
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x9c, // LOAD_CONST_SMALL_INT 28
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb5, // LOAD_FAST 5
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb6, // LOAD_FAST 6
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x82, // LOAD_CONST_SMALL_INT 2
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xed, // BINARY_OP 22 __or__
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x10, // LOAD_METHOD '_gradient_data'
    0xb6, // LOAD_FAST 6
    0x36,0x01, // CALL_METHOD 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_circle = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_circle,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 69,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 22,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 153,
        .line_info = fun_data_tdui__Pixels_circle + 19,
        .line_info_top = fun_data_tdui__Pixels_circle + 19,
        .opcodes = fun_data_tdui__Pixels_circle + 19,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_circle fun_data_tdui__Pixels_circle[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_gradient
static const byte fun_data_tdui__Pixels_gradient[75] = {
    0xb8,0x91,0x04,0x24, // prelude
    0x81,0x1a,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x11,0x81,0x12,0x81,0x13, // names: gradient, self, x, y, w, h, rgb1, rgb2, across
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x6b, // POP_JUMP_IF_FALSE 43
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x9b, // LOAD_CONST_SMALL_INT 27
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x03, // LOAD_GLOBAL 'bytes'
    0x12,0x82,0x1d, // LOAD_GLOBAL 'tuple'
    0xb5, // LOAD_FAST 5
    0x34,0x01, // CALL_FUNCTION 1
    0x12,0x82,0x1d, // LOAD_GLOBAL 'tuple'
    0xb6, // LOAD_FAST 6
    0x34,0x01, // CALL_FUNCTION 1
    0xf2, // BINARY_OP 27 __add__
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_gradient = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_gradient,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 75,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 24,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 154,
        .line_info = fun_data_tdui__Pixels_gradient + 22,
        .line_info_top = fun_data_tdui__Pixels_gradient + 22,
        .opcodes = fun_data_tdui__Pixels_gradient + 22,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_gradient fun_data_tdui__Pixels_gradient[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_image
static const byte fun_data_tdui__Pixels_image[69] = {
    0xd3,0x95,0x01,0x20, // prelude
    0x81,0x1b,0x82,0x10,0x82,0x26,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x31, // names: image, self, img, x, y, w, h, src
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x67, // POP_JUMP_IF_FALSE 39
    0xb6, // LOAD_FAST 6
    0x45,0x02, // JUMP_IF_TRUE_OR_POP 2
    0x23,0x0a, // LOAD_CONST_OBJ 10
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xc7, // STORE_FAST 7
    0xc8, // STORE_FAST 8
    0xc9, // STORE_FAST 9
    0xca, // STORE_FAST 10
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x98, // LOAD_CONST_SMALL_INT 24
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0x12,0x2e, // LOAD_GLOBAL '_part'
    0xb9, // LOAD_FAST 9
    0xba, // LOAD_FAST 10
    0x80, // LOAD_CONST_SMALL_INT 0
    0x34,0x03, // CALL_FUNCTION 3
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_image = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_image,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 69,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 27,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 155,
        .line_info = fun_data_tdui__Pixels_image + 20,
        .line_info_top = fun_data_tdui__Pixels_image + 20,
        .opcodes = fun_data_tdui__Pixels_image + 20,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_image fun_data_tdui__Pixels_image[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_nine
static const byte fun_data_tdui__Pixels_nine[71] = {
    0xd8,0x91,0x04,0x24, // prelude
    0x81,0x1c,0x82,0x10,0x82,0x26,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x33,0x81,0x31, // names: nine, self, img, x, y, w, h, border, src
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x67, // POP_JUMP_IF_FALSE 39
    0xb7, // LOAD_FAST 7
    0x45,0x02, // JUMP_IF_TRUE_OR_POP 2
    0x23,0x0a, // LOAD_CONST_OBJ 10
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xc8, // STORE_FAST 8
    0xc9, // STORE_FAST 9
    0xca, // STORE_FAST 10
    0xcb, // STORE_FAST 11
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x98, // LOAD_CONST_SMALL_INT 24
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb8, // LOAD_FAST 8
    0xb9, // LOAD_FAST 9
    0x12,0x2e, // LOAD_GLOBAL '_part'
    0xba, // LOAD_FAST 10
    0xbb, // LOAD_FAST 11
    0xb6, // LOAD_FAST 6
    0x34,0x03, // CALL_FUNCTION 3
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_nine = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_nine,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 71,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 28,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 156,
        .line_info = fun_data_tdui__Pixels_nine + 22,
        .line_info_top = fun_data_tdui__Pixels_nine + 22,
        .opcodes = fun_data_tdui__Pixels_nine + 22,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_nine fun_data_tdui__Pixels_nine[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_draw_text
static const byte fun_data_tdui__Pixels_draw_text[61] = {
    0xa8,0x91,0x05,0x20, // prelude
    0x81,0x1d,0x82,0x10,0x81,0x23,0x81,0x24,0x50,0x7d,0x82,0x16,0x81,0x2e,0x82,0x04, // names: draw_text, self, x, y, text, color, bg, scale, small
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x5f, // POP_JUMP_IF_FALSE 31
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x9d, // LOAD_CONST_SMALL_INT 29
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_draw_text = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_draw_text,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 61,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 22,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 157,
        .line_info = fun_data_tdui__Pixels_draw_text + 20,
        .line_info_top = fun_data_tdui__Pixels_draw_text + 20,
        .opcodes = fun_data_tdui__Pixels_draw_text + 20,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_draw_text fun_data_tdui__Pixels_draw_text[0]
#endif

// child of tdui__Pixels
// frozen bytecode for file tdui.py, scope tdui__Pixels_pixels
static const byte fun_data_tdui__Pixels_pixels[60] = {
    0x80,0x90,0x05,0x22, // prelude
    0x81,0x1e,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x82,0x27,0x2d,0x82,0x28, // names: pixels, self, x, y, w, h, data, rgb, dither
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x5d, // POP_JUMP_IF_FALSE 29
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x81,0x1e, // LOAD_METHOD 'pixels'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x82, // LOAD_CONST_SMALL_INT 2
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x07, // CALL_METHOD 7
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__Pixels_pixels = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels_pixels,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 60,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 158,
        .line_info = fun_data_tdui__Pixels_pixels + 21,
        .line_info_top = fun_data_tdui__Pixels_pixels + 21,
        .opcodes = fun_data_tdui__Pixels_pixels + 21,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__Pixels_pixels fun_data_tdui__Pixels_pixels[0]
#endif

static const mp_raw_code_t *const children_tdui__Pixels[] = {
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels__gradient_data,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_batch,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_bar,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_rect,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_line,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_circle,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_gradient,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_image,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_nine,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_draw_text,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels_pixels,
};

static const mp_raw_code_truncated_t proto_fun_tdui__Pixels = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__Pixels,
    .children = (void *)&children_tdui__Pixels,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 119,
    .n_children = 11,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 22,
        .line_info = fun_data_tdui__Pixels + 3,
        .line_info_top = fun_data_tdui__Pixels + 3,
        .opcodes = fun_data_tdui__Pixels + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_text_size
static const byte fun_data_tdui_text_size[84] = {
    0xc3,0x80,0x01,0x0c, // prelude
    0x30,0x50,0x81,0x2e,0x82,0x04, // names: text_size, text, scale, small
     // code info
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x14,0x31, // LOAD_METHOD 'split'
    0x10,0x32, // LOAD_CONST_STRING '\n'
    0x36,0x01, // CALL_METHOD 1
    0xc3, // STORE_FAST 3
    0xb2, // LOAD_FAST 2
    0x44,0x5e, // POP_JUMP_IF_FALSE 30
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0x32,0x00, // MAKE_FUNCTION 0
    0xb3, // LOAD_FAST 3
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0xc4, // STORE_FAST 4
    0xb4, // LOAD_FAST 4
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0x86, // LOAD_CONST_SMALL_INT 6
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0xf4, // BINARY_OP 29 __mul__
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf3, // BINARY_OP 28 __sub__
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0x2a,0x02, // BUILD_TUPLE 2
    0x63, // RETURN_VALUE
    0x88, // LOAD_CONST_SMALL_INT 8
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0x32,0x01, // MAKE_FUNCTION 1
    0xb3, // LOAD_FAST 3
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0xf4, // BINARY_OP 29 __mul__
    0x90, // LOAD_CONST_SMALL_INT 16
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0xf4, // BINARY_OP 29 __mul__
    0x2a,0x02, // BUILD_TUPLE 2
    0x63, // RETURN_VALUE
};
// child of tdui_text_size
// frozen bytecode for file tdui.py, scope tdui_text_size__lt_genexpr_gt_
static const byte fun_data_tdui_text_size__lt_genexpr_gt_[46] = {
    0xc1,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x1f, // FOR_ITER 31
    0xc1, // STORE_FAST 1
    0xb1, // LOAD_FAST 1
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0x12,0x82,0x08, // LOAD_GLOBAL 'sum'
    0x32,0x00, // MAKE_FUNCTION 0
    0xb1, // LOAD_FAST 1
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xf2, // BINARY_OP 27 __add__
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf3, // BINARY_OP 28 __sub__
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x1f, // JUMP -33
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_text_size__lt_genexpr_gt_
// frozen bytecode for file tdui.py, scope tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_
static const byte fun_data_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_[31] = {
    0xb9,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x10, // FOR_ITER 16
    0xc1, // STORE_FAST 1
    0xb1, // LOAD_FAST 1
    0x10,0x81,0x5e, // LOAD_CONST_STRING '.:'
    0xdd, // BINARY_OP 6 <in>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x83, // LOAD_CONST_SMALL_INT 3
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x2e, // JUMP -18
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 31,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_ + 7,
        .opcodes = fun_data_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_ fun_data_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_[0]
#endif

static const mp_raw_code_t *const children_tdui_text_size__lt_genexpr_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_text_size__lt_genexpr_gt___lt_genexpr_gt_,
};

static const mp_raw_code_truncated_t proto_fun_tdui_text_size__lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_text_size__lt_genexpr_gt_,
    .children = (void *)&children_tdui_text_size__lt_genexpr_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 46,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui_text_size__lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_tdui_text_size__lt_genexpr_gt_ + 7,
        .opcodes = fun_data_tdui_text_size__lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};

// child of tdui_text_size
// frozen bytecode for file tdui.py, scope tdui_text_size__lt_genexpr_gt_2
static const byte fun_data_tdui_text_size__lt_genexpr_gt_2[26] = {
    0xb9,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x0b, // FOR_ITER 11
    0xc1, // STORE_FAST 1
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x33, // JUMP -13
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_text_size__lt_genexpr_gt_2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_text_size__lt_genexpr_gt_2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui_text_size__lt_genexpr_gt_2 + 7,
        .line_info_top = fun_data_tdui_text_size__lt_genexpr_gt_2 + 7,
        .opcodes = fun_data_tdui_text_size__lt_genexpr_gt_2 + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_text_size__lt_genexpr_gt_2 fun_data_tdui_text_size__lt_genexpr_gt_2[0]
#endif

static const mp_raw_code_t *const children_tdui_text_size[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_text_size__lt_genexpr_gt_,
    (const mp_raw_code_t *)&proto_fun_tdui_text_size__lt_genexpr_gt_2,
};

static const mp_raw_code_truncated_t proto_fun_tdui_text_size = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_text_size,
    .children = (void *)&children_tdui_text_size,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 84,
    .n_children = 2,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 48,
        .line_info = fun_data_tdui_text_size + 10,
        .line_info_top = fun_data_tdui_text_size + 10,
        .opcodes = fun_data_tdui_text_size + 10,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_CanvasButton
static const byte fun_data_tdui_CanvasButton[74] = {
    0x20,0x02, // prelude
    0x17, // names: CanvasButton
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x17, // LOAD_CONST_STRING 'CanvasButton'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2a,0x05, // BUILD_TUPLE 5
    0x53, // LOAD_NULL
    0x33,0x00, // MAKE_FUNCTION_DEFARGS 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x01, // MAKE_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x50, // STORE_NAME 'text'
    0x11,0x50, // LOAD_NAME 'text'
    0x13,0x2c, // LOAD_ATTR 'setter'
    0x32,0x02, // MAKE_FUNCTION 2
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x50, // STORE_NAME 'text'
    0x51, // LOAD_CONST_NONE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x03, // MAKE_FUNCTION_DEFARGS 3
    0x16,0x81,0x36, // STORE_NAME 'draw'
    0x50, // LOAD_CONST_FALSE
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0x33,0x04, // MAKE_FUNCTION_DEFARGS 4
    0x16,0x81,0x39, // STORE_NAME '_draw'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x3e, // STORE_NAME 'remove'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x81,0x42, // STORE_NAME '_hit'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton___init__
static const byte fun_data_tdui_CanvasButton___init__[215] = {
    0xdb,0x95,0x80,0x05,0x58, // prelude
    0x59,0x82,0x10,0x81,0x20,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x50,0x7e,0x7d,0x81,0x2a,0x81,0x2b,0x81,0x2c,0x81,0x2d,0x81,0x2e,0x81,0x30,0x81,0x31,0x81,0x32,0x81,0x33,0x81,0x27,0x81,0x28,0x81,0x29,0x81,0x2f,0x81,0x1a, // names: __init__, self, cv, x, y, w, h, text, on_click, color, down, rim, fg, radius, scale, sheet, src, src_down, border, on_press, on_release, on_cancel, font, gradient
     // code info
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x20, // STORE_ATTR 'cv'
    0xb1, // LOAD_FAST 1
    0x13,0x81,0x21, // LOAD_ATTR 'drawing'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x22, // STORE_ATTR 'page'
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x2a,0x04, // BUILD_TUPLE 4
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x23, // STORE_ATTR 'x'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x24, // STORE_ATTR 'y'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x25, // STORE_ATTR 'w'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x26, // STORE_ATTR 'h'
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb6, // LOAD_FAST 6
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x79, // STORE_ATTR '_text'
    0xb7, // LOAD_FAST 7
    0xb0, // LOAD_FAST 0
    0x18,0x7e, // STORE_ATTR 'on_click'
    0x24,0x12, // LOAD_FAST_N 18
    0x24,0x13, // LOAD_FAST_N 19
    0x24,0x14, // LOAD_FAST_N 20
    0x5b, // ROT_THREE
    0x5a, // ROT_TWO
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x27, // STORE_ATTR 'on_press'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x28, // STORE_ATTR 'on_release'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x29, // STORE_ATTR 'on_cancel'
    0xb8, // LOAD_FAST 8
    0xb9, // LOAD_FAST 9
    0xba, // LOAD_FAST 10
    0xbb, // LOAD_FAST 11
    0x2a,0x04, // BUILD_TUPLE 4
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xb0, // LOAD_FAST 0
    0x18,0x7d, // STORE_ATTR 'color'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x2a, // STORE_ATTR 'down'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x2b, // STORE_ATTR 'rim'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x2c, // STORE_ATTR 'fg'
    0x24,0x16, // LOAD_FAST_N 22
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x1a, // STORE_ATTR 'gradient'
    0xbc, // LOAD_FAST 12
    0xbd, // LOAD_FAST 13
    0x5a, // ROT_TWO
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x2d, // STORE_ATTR 'radius'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x2e, // STORE_ATTR 'scale'
    0x24,0x15, // LOAD_FAST_N 21
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x2f, // STORE_ATTR 'font'
    0xbe, // LOAD_FAST 14
    0xbf, // LOAD_FAST 15
    0x24,0x10, // LOAD_FAST_N 16
    0x24,0x11, // LOAD_FAST_N 17
    0x2a,0x04, // BUILD_TUPLE 4
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x30, // STORE_ATTR 'sheet'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x31, // STORE_ATTR 'src'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x32, // STORE_ATTR 'src_down'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x33, // STORE_ATTR 'border'
    0x52, // LOAD_CONST_TRUE
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x34, // STORE_ATTR 'enabled'
    0xb1, // LOAD_FAST 1
    0x13,0x81,0x35, // LOAD_ATTR '_buttons'
    0x14,0x5b, // LOAD_METHOD 'append'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x36, // LOAD_METHOD 'draw'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 215,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 28,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 23,
        .n_kwonly_args = 0,
        .n_def_pos_args = 5,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_CanvasButton___init__ + 49,
        .line_info_top = fun_data_tdui_CanvasButton___init__ + 49,
        .opcodes = fun_data_tdui_CanvasButton___init__ + 49,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton___init__ fun_data_tdui_CanvasButton___init__[0]
#endif

// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton_text
static const byte fun_data_tdui_CanvasButton_text[9] = {
    0x09,0x06, // prelude
    0x50,0x82,0x10, // names: text, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton_text = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton_text,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 9,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 80,
        .line_info = fun_data_tdui_CanvasButton_text + 5,
        .line_info_top = fun_data_tdui_CanvasButton_text + 5,
        .opcodes = fun_data_tdui_CanvasButton_text + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton_text fun_data_tdui_CanvasButton_text[0]
#endif

// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton_text2
static const byte fun_data_tdui_CanvasButton_text2[25] = {
    0x1a,0x0a, // prelude
    0x50,0x82,0x10,0x82,0x13, // names: text, self, t
     // code info
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb0, // LOAD_FAST 0
    0x18,0x79, // STORE_ATTR '_text'
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x36, // LOAD_METHOD 'draw'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton_text2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton_text2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 25,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 80,
        .line_info = fun_data_tdui_CanvasButton_text2 + 7,
        .line_info_top = fun_data_tdui_CanvasButton_text2 + 7,
        .opcodes = fun_data_tdui_CanvasButton_text2 + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton_text2 fun_data_tdui_CanvasButton_text2[0]
#endif

// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton_draw
static const byte fun_data_tdui_CanvasButton_draw[101] = {
    0xd2,0x03,0x0c, // prelude
    0x81,0x36,0x82,0x10,0x82,0x29, // names: draw, self, pressed
     // code info
    0xb1, // LOAD_FAST 1
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x13,0x81,0x37, // LOAD_ATTR '_held'
    0xb0, // LOAD_FAST 0
    0xde, // BINARY_OP 7 <is>
    0xc1, // STORE_FAST 1
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x14,0x2f, // LOAD_METHOD 'batch'
    0x36,0x00, // CALL_METHOD 0
    0x47,0x3f, // SETUP_WITH 63
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x13,0x81,0x21, // LOAD_ATTR 'drawing'
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x22, // LOAD_ATTR 'page'
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x14,0x81,0x38, // LOAD_METHOD 'draw_to'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x22, // LOAD_ATTR 'page'
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x49,0x09, // SETUP_FINALLY 9
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x39, // LOAD_METHOD '_draw'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x22, // LOAD_ATTR 'page'
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x14,0x81,0x38, // LOAD_METHOD 'draw_to'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x5d, // END_FINALLY
    0x51, // LOAD_CONST_NONE
    0x5c, // WITH_CLEANUP
    0x5d, // END_FINALLY
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton_draw = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton_draw,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 101,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 2,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 182,
        .line_info = fun_data_tdui_CanvasButton_draw + 9,
        .line_info_top = fun_data_tdui_CanvasButton_draw + 9,
        .opcodes = fun_data_tdui_CanvasButton_draw + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton_draw fun_data_tdui_CanvasButton_draw[0]
#endif

// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton__draw
static const byte fun_data_tdui_CanvasButton__draw[424] = {
    0xea,0x11,0x0c, // prelude
    0x81,0x39,0x82,0x10,0x82,0x29, // names: _draw, self, pressed
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0xc2, // STORE_FAST 2
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x23, // LOAD_ATTR 'x'
    0xf4, // BINARY_OP 29 __mul__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x24, // LOAD_ATTR 'y'
    0xf4, // BINARY_OP 29 __mul__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x25, // LOAD_ATTR 'w'
    0xf4, // BINARY_OP 29 __mul__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x26, // LOAD_ATTR 'h'
    0xf4, // BINARY_OP 29 __mul__
    0x2a,0x04, // BUILD_TUPLE 4
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xc3, // STORE_FAST 3
    0xc4, // STORE_FAST 4
    0xc5, // STORE_FAST 5
    0xc6, // STORE_FAST 6
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x30, // LOAD_ATTR 'sheet'
    0x44,0xd7,0x80, // POP_JUMP_IF_FALSE 87
    0xb1, // LOAD_FAST 1
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x32, // LOAD_ATTR 'src_down'
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x32, // LOAD_ATTR 'src_down'
    0x42,0x44, // JUMP 4
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x31, // LOAD_ATTR 'src'
    0xc7, // STORE_FAST 7
    0xb2, // LOAD_FAST 2
    0x14,0x81,0x3a, // LOAD_METHOD 'picture'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x30, // LOAD_ATTR 'sheet'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x23, // LOAD_ATTR 'x'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x24, // LOAD_ATTR 'y'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x25, // LOAD_ATTR 'w'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x26, // LOAD_ATTR 'h'
    0x10,0x81,0x31, // LOAD_CONST_STRING 'src'
    0xb7, // LOAD_FAST 7
    0x10,0x81,0x33, // LOAD_CONST_STRING 'border'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x33, // LOAD_ATTR 'border'
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0x10,0x81,0x2c, // LOAD_CONST_STRING 'fg'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2c, // LOAD_ATTR 'fg'
    0x10,0x81,0x2e, // LOAD_CONST_STRING 'scale'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2e, // LOAD_ATTR 'scale'
    0x10,0x81,0x2f, // LOAD_CONST_STRING 'font'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2f, // LOAD_ATTR 'font'
    0x36,0x8c,0x05, // CALL_METHOD 1541
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x1a, // LOAD_ATTR 'gradient'
    0xc8, // STORE_FAST 8
    0xb1, // LOAD_FAST 1
    0x44,0x65, // POP_JUMP_IF_FALSE 37
    0xb8, // LOAD_FAST 8
    0x44,0x62, // POP_JUMP_IF_FALSE 34
    0x2c,0x03, // BUILD_MAP 3
    0xb8, // LOAD_FAST 8
    0x10,0x81,0x12, // LOAD_CONST_STRING 'rgb2'
    0x55, // LOAD_SUBSCR
    0x10,0x81,0x11, // LOAD_CONST_STRING 'rgb1'
    0x62, // STORE_MAP
    0xb8, // LOAD_FAST 8
    0x10,0x81,0x11, // LOAD_CONST_STRING 'rgb1'
    0x55, // LOAD_SUBSCR
    0x10,0x81,0x12, // LOAD_CONST_STRING 'rgb2'
    0x62, // STORE_MAP
    0xb8, // LOAD_FAST 8
    0x14,0x43, // LOAD_METHOD 'get'
    0x10,0x81,0x13, // LOAD_CONST_STRING 'across'
    0x50, // LOAD_CONST_FALSE
    0x36,0x02, // CALL_METHOD 2
    0x10,0x81,0x13, // LOAD_CONST_STRING 'across'
    0x62, // STORE_MAP
    0xc8, // STORE_FAST 8
    0xb2, // LOAD_FAST 2
    0x14,0x81,0x17, // LOAD_METHOD 'rect'
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb1, // LOAD_FAST 1
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2a, // LOAD_ATTR 'down'
    0x42,0x43, // JUMP 3
    0xb0, // LOAD_FAST 0
    0x13,0x7d, // LOAD_ATTR 'color'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2d, // LOAD_ATTR 'radius'
    0x10,0x81,0x1a, // LOAD_CONST_STRING 'gradient'
    0xb8, // LOAD_FAST 8
    0x36,0x82,0x06, // CALL_METHOD 262
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2b, // LOAD_ATTR 'rim'
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0xb2, // LOAD_FAST 2
    0x14,0x81,0x17, // LOAD_METHOD 'rect'
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2b, // LOAD_ATTR 'rim'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2d, // LOAD_ATTR 'radius'
    0x10,0x81,0x3b, // LOAD_CONST_STRING 'outline'
    0x52, // LOAD_CONST_TRUE
    0x36,0x82,0x06, // CALL_METHOD 262
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2f, // LOAD_ATTR 'font'
    0x44,0x75, // POP_JUMP_IF_FALSE 53
    0xb2, // LOAD_FAST 2
    0x14,0x81,0x3c, // LOAD_METHOD 'font_text'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2f, // LOAD_ATTR 'font'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x23, // LOAD_ATTR 'x'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x24, // LOAD_ATTR 'y'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x25, // LOAD_ATTR 'w'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x26, // LOAD_ATTR 'h'
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0x10,0x81,0x2c, // LOAD_CONST_STRING 'fg'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2c, // LOAD_ATTR 'fg'
    0x10,0x81,0x3d, // LOAD_CONST_STRING 'align'
    0x10,0x81,0x3e, // LOAD_CONST_STRING 'center'
    0x10,0x81,0x3f, // LOAD_CONST_STRING 'valign'
    0x10,0x81,0x40, // LOAD_CONST_STRING 'middle'
    0x36,0x86,0x06, // CALL_METHOD 774
    0x59, // POP_TOP
    0x42,0xf1,0x80, // JUMP 113
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2e, // LOAD_ATTR 'scale'
    0x44,0x7d, // POP_JUMP_IF_FALSE 61
    0x12,0x30, // LOAD_GLOBAL 'text_size'
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2e, // LOAD_ATTR 'scale'
    0x34,0x02, // CALL_FUNCTION 2
    0x30,0x02, // UNPACK_SEQUENCE 2
    0xc9, // STORE_FAST 9
    0xca, // STORE_FAST 10
    0xb2, // LOAD_FAST 2
    0x14,0x81,0x1d, // LOAD_METHOD 'draw_text'
    0xb3, // LOAD_FAST 3
    0xb5, // LOAD_FAST 5
    0xb9, // LOAD_FAST 9
    0xf3, // BINARY_OP 28 __sub__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf6, // BINARY_OP 31 __floordiv__
    0xf2, // BINARY_OP 27 __add__
    0xb4, // LOAD_FAST 4
    0xb6, // LOAD_FAST 6
    0xba, // LOAD_FAST 10
    0xf3, // BINARY_OP 28 __sub__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf6, // BINARY_OP 31 __floordiv__
    0xf2, // BINARY_OP 27 __add__
    0xb1, // LOAD_FAST 1
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xf2, // BINARY_OP 27 __add__
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2c, // LOAD_ATTR 'fg'
    0x10,0x81,0x2e, // LOAD_CONST_STRING 'scale'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2e, // LOAD_ATTR 'scale'
    0x36,0x82,0x04, // CALL_METHOD 260
    0x59, // POP_TOP
    0x42,0x6e, // JUMP 46
    0xb2, // LOAD_FAST 2
    0x14,0x81,0x41, // LOAD_METHOD 'put'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x23, // LOAD_ATTR 'x'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x25, // LOAD_ATTR 'w'
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0x34,0x01, // CALL_FUNCTION 1
    0xf3, // BINARY_OP 28 __sub__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf6, // BINARY_OP 31 __floordiv__
    0xf2, // BINARY_OP 27 __add__
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x24, // LOAD_ATTR 'y'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x26, // LOAD_ATTR 'h'
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf6, // BINARY_OP 31 __floordiv__
    0xf2, // BINARY_OP 27 __add__
    0xb0, // LOAD_FAST 0
    0x13,0x79, // LOAD_ATTR '_text'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x2c, // LOAD_ATTR 'fg'
    0x7e, // LOAD_CONST_SMALL_INT -2
    0x36,0x05, // CALL_METHOD 5
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton__draw = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton__draw,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 424,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 30,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 185,
        .line_info = fun_data_tdui_CanvasButton__draw + 9,
        .line_info_top = fun_data_tdui_CanvasButton__draw + 9,
        .opcodes = fun_data_tdui_CanvasButton__draw + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton__draw fun_data_tdui_CanvasButton__draw[0]
#endif

// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton_remove
static const byte fun_data_tdui_CanvasButton_remove[64] = {
    0x19,0x06, // prelude
    0x3e,0x82,0x10, // names: remove, self
     // code info
    0xb0, // LOAD_FAST 0
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x13,0x81,0x35, // LOAD_ATTR '_buttons'
    0xdd, // BINARY_OP 6 <in>
    0x44,0x4d, // POP_JUMP_IF_FALSE 13
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x13,0x81,0x35, // LOAD_ATTR '_buttons'
    0x14,0x3e, // LOAD_METHOD 'remove'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x13,0x81,0x37, // LOAD_ATTR '_held'
    0xb0, // LOAD_FAST 0
    0xde, // BINARY_OP 7 <is>
    0x44,0x56, // POP_JUMP_IF_FALSE 22
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x18,0x81,0x37, // STORE_ATTR '_held'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x29, // LOAD_ATTR 'on_cancel'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x29, // LOAD_METHOD 'on_cancel'
    0xb0, // LOAD_FAST 0
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton_remove = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton_remove,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 64,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 62,
        .line_info = fun_data_tdui_CanvasButton_remove + 5,
        .line_info_top = fun_data_tdui_CanvasButton_remove + 5,
        .opcodes = fun_data_tdui_CanvasButton_remove + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton_remove fun_data_tdui_CanvasButton_remove[0]
#endif

// child of tdui_CanvasButton
// frozen bytecode for file tdui.py, scope tdui_CanvasButton__hit
static const byte fun_data_tdui_CanvasButton__hit[81] = {
    0x2b,0x10, // prelude
    0x81,0x42,0x82,0x10,0x81,0x23,0x81,0x24, // names: _hit, self, x, y
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x34, // LOAD_ATTR 'enabled'
    0x46,0x40, // JUMP_IF_FALSE_OR_POP 64
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x22, // LOAD_ATTR 'page'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x20, // LOAD_ATTR 'cv'
    0x13,0x81,0x43, // LOAD_ATTR 'showing'
    0xd9, // BINARY_OP 2 __eq__
    0x46,0x32, // JUMP_IF_FALSE_OR_POP 50
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x23, // LOAD_ATTR 'x'
    0xb1, // LOAD_FAST 1
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x0c, // JUMP_IF_FALSE_OR_POP 12
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x23, // LOAD_ATTR 'x'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x25, // LOAD_ATTR 'w'
    0xf2, // BINARY_OP 27 __add__
    0xd7, // BINARY_OP 0 __lt__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x46,0x18, // JUMP_IF_FALSE_OR_POP 24
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x24, // LOAD_ATTR 'y'
    0xb2, // LOAD_FAST 2
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x0c, // JUMP_IF_FALSE_OR_POP 12
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x24, // LOAD_ATTR 'y'
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x26, // LOAD_ATTR 'h'
    0xf2, // BINARY_OP 27 __add__
    0xd7, // BINARY_OP 0 __lt__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton__hit = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton__hit,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 81,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 194,
        .line_info = fun_data_tdui_CanvasButton__hit + 10,
        .line_info_top = fun_data_tdui_CanvasButton__hit + 10,
        .opcodes = fun_data_tdui_CanvasButton__hit + 10,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_CanvasButton__hit fun_data_tdui_CanvasButton__hit[0]
#endif

static const mp_raw_code_t *const children_tdui_CanvasButton[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton_text,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton_text2,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton_draw,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton__draw,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton_remove,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton__hit,
};

static const mp_raw_code_truncated_t proto_fun_tdui_CanvasButton = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_CanvasButton,
    .children = (void *)&children_tdui_CanvasButton,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 74,
    .n_children = 7,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 23,
        .line_info = fun_data_tdui_CanvasButton + 3,
        .line_info_top = fun_data_tdui_CanvasButton + 3,
        .opcodes = fun_data_tdui_CanvasButton + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Image
static const byte fun_data_tdui_Image[56] = {
    0x20,0x02, // prelude
    0x18, // names: Image
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x18, // LOAD_CONST_STRING 'Image'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x00, // MAKE_FUNCTION_DEFARGS 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x07, // LOAD_NAME 'staticmethod'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x52, // LOAD_CONST_TRUE
    0x2a,0x04, // BUILD_TUPLE 4
    0x53, // LOAD_NULL
    0x33,0x01, // MAKE_FUNCTION_DEFARGS 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x45, // STORE_NAME 'load'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x81,0x4a, // STORE_NAME 'free'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x81,0x15, // STORE_NAME '_alive'
    0x32,0x04, // MAKE_FUNCTION 4
    0x16,0x44, // STORE_NAME '_event'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_Image
// frozen bytecode for file tdui.py, scope tdui_Image___init__
static const byte fun_data_tdui_Image___init__[123] = {
    0xe5,0x84,0x01,0x14, // prelude
    0x59,0x82,0x10,0x81,0x25,0x81,0x26,0x7d,0x82,0x2a, // names: __init__, self, w, h, color, _file
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x4f, // LOAD_METHOD 'open'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x16, // STORE_ATTR '_bar_colors'
    0x12,0x23, // LOAD_GLOBAL '_new_id'
    0x34,0x00, // CALL_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x18,0x5d, // STORE_ATTR 'id'
    0xb0, // LOAD_FAST 0
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x56, // STORE_SUBSCR
    0x48,0x3b, // SETUP_EXCEPT 59
    0xb4, // LOAD_FAST 4
    0x44,0x5f, // POP_JUMP_IF_FALSE 31
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x81,0x44, // LOAD_METHOD 'image_load'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0xb4, // LOAD_FAST 4
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0xb4, // LOAD_FAST 4
    0x82, // LOAD_CONST_SMALL_INT 2
    0x55, // LOAD_SUBSCR
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x36,0x06, // CALL_METHOD 6
    0x30,0x02, // UNPACK_SEQUENCE 2
    0xb0, // LOAD_FAST 0
    0x18,0x5f, // STORE_ATTR 'width'
    0xb0, // LOAD_FAST 0
    0x18,0x60, // STORE_ATTR 'height'
    0x42,0x57, // JUMP 23
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x81,0x1b, // LOAD_METHOD 'image'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x36,0x04, // CALL_METHOD 4
    0x59, // POP_TOP
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x5a, // ROT_TWO
    0xb0, // LOAD_FAST 0
    0x18,0x5f, // STORE_ATTR 'width'
    0xb0, // LOAD_FAST 0
    0x18,0x60, // STORE_ATTR 'height'
    0x4a,0x13, // POP_EXCEPT_JUMP 19
    0x57, // DUP_TOP
    0x12,0x82,0x2b, // LOAD_GLOBAL 'Exception'
    0xdf, // BINARY_OP 8 <exception match>
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0x59, // POP_TOP
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x53, // LOAD_NULL
    0x5b, // ROT_THREE
    0x56, // STORE_SUBSCR
    0x64, // RAISE_LAST
    0x5d, // END_FINALLY
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Image___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Image___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 123,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 1,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Image___init__ + 14,
        .line_info_top = fun_data_tdui_Image___init__ + 14,
        .opcodes = fun_data_tdui_Image___init__ + 14,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Image___init__ fun_data_tdui_Image___init__[0]
#endif

// child of tdui_Image
// frozen bytecode for file tdui.py, scope tdui_Image_load
static const byte fun_data_tdui_Image_load[87] = {
    0xe1,0x84,0x80,0x01,0x18, // prelude
    0x81,0x45,0x82,0x2c,0x81,0x25,0x81,0x26,0x81,0x79,0x82,0x28, // names: load, path, w, h, key, dither
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x46, // LOAD_METHOD 'startswith'
    0x10,0x81,0x47, // LOAD_CONST_STRING '/'
    0x36,0x01, // CALL_METHOD 1
    0x43,0x56, // POP_JUMP_IF_TRUE 22
    0x12,0x03, // LOAD_GLOBAL 'os'
    0x14,0x81,0x48, // LOAD_METHOD 'getcwd'
    0x36,0x00, // CALL_METHOD 0
    0x14,0x81,0x49, // LOAD_METHOD 'rstrip'
    0x10,0x81,0x47, // LOAD_CONST_STRING '/'
    0x36,0x01, // CALL_METHOD 1
    0x10,0x81,0x47, // LOAD_CONST_STRING '/'
    0xf2, // BINARY_OP 27 __add__
    0xb0, // LOAD_FAST 0
    0xf2, // BINARY_OP 27 __add__
    0xc0, // STORE_FAST 0
    0xb3, // LOAD_FAST 3
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x42,0x4f, // JUMP 15
    0xb3, // LOAD_FAST 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x90, // LOAD_CONST_SMALL_INT 16
    0xf0, // BINARY_OP 25 __lshift__
    0xb3, // LOAD_FAST 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x55, // LOAD_SUBSCR
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf0, // BINARY_OP 25 __lshift__
    0xed, // BINARY_OP 22 __or__
    0xb3, // LOAD_FAST 3
    0x82, // LOAD_CONST_SMALL_INT 2
    0x55, // LOAD_SUBSCR
    0xed, // BINARY_OP 22 __or__
    0xc5, // STORE_FAST 5
    0x12,0x18, // LOAD_GLOBAL 'Image'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0xb5, // LOAD_FAST 5
    0xb4, // LOAD_FAST 4
    0x2a,0x03, // BUILD_TUPLE 3
    0x34,0x04, // CALL_FUNCTION 4
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Image_load = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Image_load,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 87,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 5,
        .n_kwonly_args = 0,
        .n_def_pos_args = 4,
        .qstr_block_name_idx = 197,
        .line_info = fun_data_tdui_Image_load + 17,
        .line_info_top = fun_data_tdui_Image_load + 17,
        .opcodes = fun_data_tdui_Image_load + 17,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Image_load fun_data_tdui_Image_load[0]
#endif

// child of tdui_Image
// frozen bytecode for file tdui.py, scope tdui_Image_free
static const byte fun_data_tdui_Image_free[34] = {
    0x19,0x08, // prelude
    0x81,0x4a,0x82,0x10, // names: free, self
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x15, // LOAD_METHOD '_alive'
    0x36,0x00, // CALL_METHOD 0
    0x44,0x52, // POP_JUMP_IF_FALSE 18
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x34,0x02, // CALL_FUNCTION 2
    0x59, // POP_TOP
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x53, // LOAD_NULL
    0x5b, // ROT_THREE
    0x56, // STORE_SUBSCR
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Image_free = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Image_free,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 34,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 202,
        .line_info = fun_data_tdui_Image_free + 6,
        .line_info_top = fun_data_tdui_Image_free + 6,
        .opcodes = fun_data_tdui_Image_free + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Image_free fun_data_tdui_Image_free[0]
#endif

// child of tdui_Image
// frozen bytecode for file tdui.py, scope tdui_Image__alive
static const byte fun_data_tdui_Image__alive[14] = {
    0x11,0x08, // prelude
    0x81,0x15,0x82,0x10, // names: _alive, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xdd, // BINARY_OP 6 <in>
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Image__alive = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Image__alive,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 14,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 149,
        .line_info = fun_data_tdui_Image__alive + 6,
        .line_info_top = fun_data_tdui_Image__alive + 6,
        .opcodes = fun_data_tdui_Image__alive + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Image__alive fun_data_tdui_Image__alive[0]
#endif

// child of tdui_Image
// frozen bytecode for file tdui.py, scope tdui_Image__event
static const byte fun_data_tdui_Image__event[20] = {
    0xbb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Image__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Image__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 20,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Image__event + 18,
        .line_info_top = fun_data_tdui_Image__event + 18,
        .opcodes = fun_data_tdui_Image__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Image__event fun_data_tdui_Image__event[0]
#endif

static const mp_raw_code_t *const children_tdui_Image[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Image___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Image_load,
    (const mp_raw_code_t *)&proto_fun_tdui_Image_free,
    (const mp_raw_code_t *)&proto_fun_tdui_Image__alive,
    (const mp_raw_code_t *)&proto_fun_tdui_Image__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Image = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Image,
    .children = (void *)&children_tdui_Image,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 56,
    .n_children = 5,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 5,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 24,
        .line_info = fun_data_tdui_Image + 3,
        .line_info_top = fun_data_tdui_Image + 3,
        .opcodes = fun_data_tdui_Image + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Font
static const byte fun_data_tdui_Font[39] = {
    0x08,0x02, // prelude
    0x19, // names: Font
     // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x19, // LOAD_CONST_STRING 'Font'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x32,0x00, // MAKE_FUNCTION 0
    0x16,0x59, // STORE_NAME '__init__'
    0x11,0x82,0x07, // LOAD_NAME 'staticmethod'
    0x32,0x01, // MAKE_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x45, // STORE_NAME 'load'
    0x32,0x02, // MAKE_FUNCTION 2
    0x16,0x81,0x4a, // STORE_NAME 'free'
    0x32,0x03, // MAKE_FUNCTION 3
    0x16,0x44, // STORE_NAME '_event'
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_Font
// frozen bytecode for file tdui.py, scope tdui_Font___init__
static const byte fun_data_tdui_Font___init__[96] = {
    0x3a,0x0a, // prelude
    0x59,0x82,0x10,0x82,0x2c, // names: __init__, self, path
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x4f, // LOAD_METHOD 'open'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x12,0x23, // LOAD_GLOBAL '_new_id'
    0x34,0x00, // CALL_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x18,0x5d, // STORE_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x14,0x81,0x46, // LOAD_METHOD 'startswith'
    0x10,0x81,0x47, // LOAD_CONST_STRING '/'
    0x36,0x01, // CALL_METHOD 1
    0x43,0x56, // POP_JUMP_IF_TRUE 22
    0x12,0x03, // LOAD_GLOBAL 'os'
    0x14,0x81,0x48, // LOAD_METHOD 'getcwd'
    0x36,0x00, // CALL_METHOD 0
    0x14,0x81,0x49, // LOAD_METHOD 'rstrip'
    0x10,0x81,0x47, // LOAD_CONST_STRING '/'
    0x36,0x01, // CALL_METHOD 1
    0x10,0x81,0x47, // LOAD_CONST_STRING '/'
    0xf2, // BINARY_OP 27 __add__
    0xb1, // LOAD_FAST 1
    0xf2, // BINARY_OP 27 __add__
    0xc1, // STORE_FAST 1
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x81,0x4b, // LOAD_METHOD 'font_load'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x36,0x02, // CALL_METHOD 2
    0x30,0x05, // UNPACK_SEQUENCE 5
    0xb0, // LOAD_FAST 0
    0x18,0x60, // STORE_ATTR 'height'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x4c, // STORE_ATTR 'ascent'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x4d, // STORE_ATTR 'glyphs'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x4e, // STORE_ATTR 'size'
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x4f, // STORE_ATTR 'bpp'
    0xb0, // LOAD_FAST 0
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x56, // STORE_SUBSCR
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Font___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Font___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 96,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Font___init__ + 7,
        .line_info_top = fun_data_tdui_Font___init__ + 7,
        .opcodes = fun_data_tdui_Font___init__ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Font___init__ fun_data_tdui_Font___init__[0]
#endif

// child of tdui_Font
// frozen bytecode for file tdui.py, scope tdui_Font_load
static const byte fun_data_tdui_Font_load[12] = {
    0x11,0x08, // prelude
    0x81,0x45,0x82,0x2c, // names: load, path
     // code info
    0x12,0x19, // LOAD_GLOBAL 'Font'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Font_load = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Font_load,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 12,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 197,
        .line_info = fun_data_tdui_Font_load + 6,
        .line_info_top = fun_data_tdui_Font_load + 6,
        .opcodes = fun_data_tdui_Font_load + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Font_load fun_data_tdui_Font_load[0]
#endif

// child of tdui_Font
// frozen bytecode for file tdui.py, scope tdui_Font_free
static const byte fun_data_tdui_Font_free[35] = {
    0x19,0x08, // prelude
    0x81,0x4a,0x82,0x10, // names: free, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xdd, // BINARY_OP 6 <in>
    0x44,0x52, // POP_JUMP_IF_FALSE 18
    0x12,0x24, // LOAD_GLOBAL '_send'
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x34,0x02, // CALL_FUNCTION 2
    0x59, // POP_TOP
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x53, // LOAD_NULL
    0x5b, // ROT_THREE
    0x56, // STORE_SUBSCR
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Font_free = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Font_free,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 35,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 202,
        .line_info = fun_data_tdui_Font_free + 6,
        .line_info_top = fun_data_tdui_Font_free + 6,
        .opcodes = fun_data_tdui_Font_free + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Font_free fun_data_tdui_Font_free[0]
#endif

// child of tdui_Font
// frozen bytecode for file tdui.py, scope tdui_Font__event
static const byte fun_data_tdui_Font__event[20] = {
    0xbb,0x04,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Font__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Font__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 20,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Font__event + 18,
        .line_info_top = fun_data_tdui_Font__event + 18,
        .opcodes = fun_data_tdui_Font__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Font__event fun_data_tdui_Font__event[0]
#endif

static const mp_raw_code_t *const children_tdui_Font[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Font___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Font_load,
    (const mp_raw_code_t *)&proto_fun_tdui_Font_free,
    (const mp_raw_code_t *)&proto_fun_tdui_Font__event,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Font = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Font,
    .children = (void *)&children_tdui_Font,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 39,
    .n_children = 4,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 25,
        .line_info = fun_data_tdui_Font + 3,
        .line_info_top = fun_data_tdui_Font + 3,
        .opcodes = fun_data_tdui_Font + 3,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_Canvas
static const byte fun_data_tdui_Canvas[196] = {
    0x80,0x10,0x03, // prelude
    0x1a, // names: Canvas
    0x00, // code info
    0x11,0x81,0x7d, // LOAD_NAME '__name__'
    0x16,0x81,0x7e, // STORE_NAME '__module__'
    0x10,0x1a, // LOAD_CONST_STRING 'Canvas'
    0x16,0x81,0x7f, // STORE_NAME '__qualname__'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x16,0x70, // STORE_NAME '_n'
    0x81, // LOAD_CONST_SMALL_INT 1
    0x2a,0x01, // BUILD_TUPLE 1
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x21,0x00,0x01, // MAKE_CLOSURE_DEFARGS 0
    0x16,0x59, // STORE_NAME '__init__'
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x01, // MAKE_FUNCTION_DEFARGS 1
    0x16,0x81,0x41, // STORE_NAME 'put'
    0x10,0x33, // LOAD_CONST_STRING ' '
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x2a,0x03, // BUILD_TUPLE 3
    0x53, // LOAD_NULL
    0x33,0x02, // MAKE_FUNCTION_DEFARGS 2
    0x16,0x81,0x52, // STORE_NAME 'fill'
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x2a,0x02, // BUILD_TUPLE 2
    0x53, // LOAD_NULL
    0x33,0x03, // MAKE_FUNCTION_DEFARGS 3
    0x16,0x81,0x53, // STORE_NAME 'clear'
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x81, // LOAD_CONST_SMALL_INT 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x2a,0x04, // BUILD_TUPLE 4
    0x53, // LOAD_NULL
    0x33,0x04, // MAKE_FUNCTION_DEFARGS 4
    0x16,0x81,0x54, // STORE_NAME 'big'
    0x32,0x05, // MAKE_FUNCTION 5
    0x16,0x81,0x55, // STORE_NAME '_page'
    0x32,0x06, // MAKE_FUNCTION 6
    0x16,0x81,0x38, // STORE_NAME 'draw_to'
    0x32,0x07, // MAKE_FUNCTION 7
    0x16,0x81,0x57, // STORE_NAME 'show'
    0x32,0x08, // MAKE_FUNCTION 8
    0x16,0x81,0x58, // STORE_NAME 'copy_page'
    0x32,0x09, // MAKE_FUNCTION 9
    0x16,0x68, // STORE_NAME 'resize'
    0x32,0x0a, // MAKE_FUNCTION 10
    0x16,0x69, // STORE_NAME 'move'
    0x32,0x0b, // MAKE_FUNCTION 11
    0x16,0x81,0x56, // STORE_NAME '_req2'
    0x32,0x0c, // MAKE_FUNCTION 12
    0x16,0x81,0x15, // STORE_NAME '_alive'
    0x51, // LOAD_CONST_NONE
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x22,0x81,0x67, // LOAD_CONST_SMALL_INT 231
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x2a,0x06, // BUILD_TUPLE 6
    0x53, // LOAD_NULL
    0x33,0x0d, // MAKE_FUNCTION_DEFARGS 13
    0x16,0x81,0x3a, // STORE_NAME 'picture'
    0x22,0x81,0x67, // LOAD_CONST_SMALL_INT 231
    0x7e, // LOAD_CONST_SMALL_INT -2
    0x10,0x34, // LOAD_CONST_STRING 'left'
    0x10,0x35, // LOAD_CONST_STRING 'top'
    0x50, // LOAD_CONST_FALSE
    0x2a,0x05, // BUILD_TUPLE 5
    0x53, // LOAD_NULL
    0x33,0x0e, // MAKE_FUNCTION_DEFARGS 14
    0x16,0x81,0x3c, // STORE_NAME 'font_text'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x0f, // MAKE_FUNCTION 15
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x5c, // STORE_NAME 'pwidth'
    0x11,0x82,0x00, // LOAD_NAME 'property'
    0x32,0x10, // MAKE_FUNCTION 16
    0x34,0x01, // CALL_FUNCTION 1
    0x16,0x81,0x5d, // STORE_NAME 'pheight'
    0x32,0x11, // MAKE_FUNCTION 17
    0x16,0x44, // STORE_NAME '_event'
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x22,0x81,0x67, // LOAD_CONST_SMALL_INT 231
    0x86, // LOAD_CONST_SMALL_INT 6
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x84, // LOAD_CONST_SMALL_INT 4
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2a,0x10, // BUILD_TUPLE 16
    0x53, // LOAD_NULL
    0x33,0x12, // MAKE_FUNCTION_DEFARGS 18
    0x16,0x73, // STORE_NAME 'button'
    0xb0, // LOAD_FAST 0
    0x63, // RETURN_VALUE
};
// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas___init__
static const byte fun_data_tdui_Canvas___init__[196] = {
    0x91,0x91,0x04,0x24, // prelude
    0x59,0x82,0x17,0x82,0x10,0x5a,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x51,0x81,0x50, // names: __init__, *, self, win, x, y, w, h, on_mouse, pages
     // code info
    0xb2, // LOAD_FAST 2
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x43,0x48, // POP_JUMP_IF_TRUE 8
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x23,0x05, // LOAD_CONST_OBJ 5
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0xb3, // LOAD_FAST 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x4f, // POP_JUMP_IF_TRUE 15
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x4a, // POP_JUMP_IF_TRUE 10
    0xb5, // LOAD_FAST 5
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x45, // POP_JUMP_IF_TRUE 5
    0xb6, // LOAD_FAST 6
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd7, // BINARY_OP 0 __lt__
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x0b, // LOAD_CONST_OBJ 11
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x81, // LOAD_CONST_SMALL_INT 1
    0xb8, // LOAD_FAST 8
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x04, // JUMP_IF_FALSE_OR_POP 4
    0x90, // LOAD_CONST_SMALL_INT 16
    0xda, // BINARY_OP 3 __le__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x43,0x4a, // POP_JUMP_IF_TRUE 10
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x0c, // LOAD_CONST_OBJ 12
    0x90, // LOAD_CONST_SMALL_INT 16
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x71, // LOAD_GLOBAL '_cells'
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xf4, // BINARY_OP 29 __mul__
    0xb8, // LOAD_FAST 8
    0xf4, // BINARY_OP 29 __mul__
    0xf2, // BINARY_OP 27 __add__
    0x12,0x08, // LOAD_GLOBAL 'MAX_CELLS'
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x23,0x0d, // LOAD_CONST_OBJ 13
    0x12,0x08, // LOAD_GLOBAL 'MAX_CELLS'
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x71, // LOAD_GLOBAL '_cells'
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xf4, // BINARY_OP 29 __mul__
    0xb8, // LOAD_FAST 8
    0xf4, // BINARY_OP 29 __mul__
    0xe5, // BINARY_OP 14 __iadd__
    0x17,0x71, // STORE_GLOBAL '_cells'
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xf4, // BINARY_OP 29 __mul__
    0xb8, // LOAD_FAST 8
    0xf4, // BINARY_OP 29 __mul__
    0xb1, // LOAD_FAST 1
    0x18,0x71, // STORE_ATTR '_cells'
    0xb5, // LOAD_FAST 5
    0xb1, // LOAD_FAST 1
    0x18,0x5f, // STORE_ATTR 'width'
    0xb6, // LOAD_FAST 6
    0xb1, // LOAD_FAST 1
    0x18,0x60, // STORE_ATTR 'height'
    0xb8, // LOAD_FAST 8
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x50, // STORE_ATTR 'pages'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x21, // STORE_ATTR 'drawing'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x43, // STORE_ATTR 'showing'
    0xb7, // LOAD_FAST 7
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x51, // STORE_ATTR 'on_mouse'
    0x2b,0x00, // BUILD_LIST 0
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x35, // STORE_ATTR '_buttons'
    0x51, // LOAD_CONST_NONE
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x16, // STORE_ATTR '_bar_colors'
    0x51, // LOAD_CONST_NONE
    0xb1, // LOAD_FAST 1
    0x18,0x81,0x37, // STORE_ATTR '_held'
    0x12,0x82,0x18, // LOAD_GLOBAL 'super'
    0x25,0x00, // LOAD_DEREF 0
    0xb1, // LOAD_FAST 1
    0x15,0x59, // LOAD_SUPER_METHOD '__init__'
    0xb2, // LOAD_FAST 2
    0x87, // LOAD_CONST_SMALL_INT 7
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb8, // LOAD_FAST 8
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas___init__ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas___init__,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 196,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 19,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 9,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 89,
        .line_info = fun_data_tdui_Canvas___init__ + 22,
        .line_info_top = fun_data_tdui_Canvas___init__ + 22,
        .opcodes = fun_data_tdui_Canvas___init__ + 22,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas___init__ fun_data_tdui_Canvas___init__[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_put
static const byte fun_data_tdui_Canvas_put[51] = {
    0x9a,0x94,0x01,0x1a, // prelude
    0x81,0x41,0x82,0x10,0x81,0x23,0x81,0x24,0x50,0x81,0x2c,0x82,0x16, // names: put, self, x, y, text, fg, bg
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x59, // POP_JUMP_IF_FALSE 25
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x8b, // LOAD_CONST_SMALL_INT 11
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_put = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_put,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 51,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 20,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 6,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 193,
        .line_info = fun_data_tdui_Canvas_put + 17,
        .line_info_top = fun_data_tdui_Canvas_put + 17,
        .opcodes = fun_data_tdui_Canvas_put + 17,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_put fun_data_tdui_Canvas_put[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_fill
static const byte fun_data_tdui_Canvas_fill[51] = {
    0xa0,0x91,0x05,0x24, // prelude
    0x81,0x52,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x82,0x2d,0x81,0x2c,0x82,0x16, // names: fill, self, x, y, w, h, ch, fg, bg
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x54, // POP_JUMP_IF_FALSE 20
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x8c, // LOAD_CONST_SMALL_INT 12
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0xb5, // LOAD_FAST 5
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_fill = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_fill,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 51,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 21,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 3,
        .qstr_block_name_idx = 210,
        .line_info = fun_data_tdui_Canvas_fill + 22,
        .line_info_top = fun_data_tdui_Canvas_fill + 22,
        .opcodes = fun_data_tdui_Canvas_fill + 22,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_fill fun_data_tdui_Canvas_fill[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_clear
static const byte fun_data_tdui_Canvas_clear[33] = {
    0xdb,0x80,0x01,0x10, // prelude
    0x81,0x53,0x82,0x10,0x81,0x2c,0x82,0x16, // names: clear, self, fg, bg
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x52, // LOAD_METHOD 'fill'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb0, // LOAD_FAST 0
    0x13,0x5f, // LOAD_ATTR 'width'
    0xb0, // LOAD_FAST 0
    0x13,0x60, // LOAD_ATTR 'height'
    0x10,0x33, // LOAD_CONST_STRING ' '
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x36,0x07, // CALL_METHOD 7
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_clear = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_clear,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 33,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 211,
        .line_info = fun_data_tdui_Canvas_clear + 12,
        .line_info_top = fun_data_tdui_Canvas_clear + 12,
        .opcodes = fun_data_tdui_Canvas_clear + 12,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_clear fun_data_tdui_Canvas_clear[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_big
static const byte fun_data_tdui_Canvas_big[55] = {
    0xa8,0x90,0x84,0x01,0x20, // prelude
    0x81,0x54,0x82,0x10,0x81,0x23,0x81,0x24,0x50,0x81,0x2c,0x82,0x16,0x81,0x2e,0x5f, // names: big, self, x, y, text, fg, bg, scale, width
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x59, // POP_JUMP_IF_FALSE 25
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x95, // LOAD_CONST_SMALL_INT 21
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb3, // LOAD_FAST 3
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_big = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_big,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 55,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 22,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 8,
        .n_kwonly_args = 0,
        .n_def_pos_args = 4,
        .qstr_block_name_idx = 212,
        .line_info = fun_data_tdui_Canvas_big + 21,
        .line_info_top = fun_data_tdui_Canvas_big + 21,
        .opcodes = fun_data_tdui_Canvas_big + 21,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_big fun_data_tdui_Canvas_big[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas__page
static const byte fun_data_tdui_Canvas__page[43] = {
    0x2a,0x0c, // prelude
    0x81,0x55,0x82,0x10,0x82,0x2e, // names: _page, self, n
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb1, // LOAD_FAST 1
    0x57, // DUP_TOP
    0x5b, // ROT_THREE
    0xda, // BINARY_OP 3 __le__
    0x46,0x07, // JUMP_IF_FALSE_OR_POP 7
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x50, // LOAD_ATTR 'pages'
    0xd7, // BINARY_OP 0 __lt__
    0x42,0x42, // JUMP 2
    0x5a, // ROT_TWO
    0x59, // POP_TOP
    0x43,0x4f, // POP_JUMP_IF_TRUE 15
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x0e, // LOAD_CONST_OBJ 14
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x50, // LOAD_ATTR 'pages'
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf3, // BINARY_OP 28 __sub__
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0xb1, // LOAD_FAST 1
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas__page = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas__page,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 43,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 213,
        .line_info = fun_data_tdui_Canvas__page + 8,
        .line_info_top = fun_data_tdui_Canvas__page + 8,
        .opcodes = fun_data_tdui_Canvas__page + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas__page fun_data_tdui_Canvas__page[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_draw_to
static const byte fun_data_tdui_Canvas_draw_to[36] = {
    0x5a,0x0c, // prelude
    0x81,0x38,0x82,0x10,0x81,0x22, // names: draw_to, self, page
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x55, // LOAD_METHOD '_page'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x21, // STORE_ATTR 'drawing'
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x56, // LOAD_METHOD '_req2'
    0x93, // LOAD_CONST_SMALL_INT 19
    0xb1, // LOAD_FAST 1
    0x7f, // LOAD_CONST_SMALL_INT -1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_draw_to = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_draw_to,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 36,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 184,
        .line_info = fun_data_tdui_Canvas_draw_to + 8,
        .line_info_top = fun_data_tdui_Canvas_draw_to + 8,
        .opcodes = fun_data_tdui_Canvas_draw_to + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_draw_to fun_data_tdui_Canvas_draw_to[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_show
static const byte fun_data_tdui_Canvas_show[36] = {
    0x5a,0x0c, // prelude
    0x81,0x57,0x82,0x10,0x81,0x22, // names: show, self, page
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x55, // LOAD_METHOD '_page'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x43, // STORE_ATTR 'showing'
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x56, // LOAD_METHOD '_req2'
    0x93, // LOAD_CONST_SMALL_INT 19
    0x7f, // LOAD_CONST_SMALL_INT -1
    0xb1, // LOAD_FAST 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_show = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_show,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 36,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 215,
        .line_info = fun_data_tdui_Canvas_show + 8,
        .line_info_top = fun_data_tdui_Canvas_show + 8,
        .opcodes = fun_data_tdui_Canvas_show + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_show fun_data_tdui_Canvas_show[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_copy_page
static const byte fun_data_tdui_Canvas_copy_page[39] = {
    0x63,0x10, // prelude
    0x81,0x58,0x82,0x10,0x81,0x31,0x82,0x2f, // names: copy_page, self, src, dst
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x56, // LOAD_METHOD '_req2'
    0x94, // LOAD_CONST_SMALL_INT 20
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x55, // LOAD_METHOD '_page'
    0xb1, // LOAD_FAST 1
    0x36,0x01, // CALL_METHOD 1
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x55, // LOAD_METHOD '_page'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_copy_page = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_copy_page,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 39,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 216,
        .line_info = fun_data_tdui_Canvas_copy_page + 10,
        .line_info_top = fun_data_tdui_Canvas_copy_page + 10,
        .opcodes = fun_data_tdui_Canvas_copy_page + 10,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_copy_page fun_data_tdui_Canvas_copy_page[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_resize
static const byte fun_data_tdui_Canvas_resize[111] = {
    0x63,0x0e, // prelude
    0x68,0x82,0x10,0x81,0x25,0x81,0x26, // names: resize, self, w, h
     // code info
    0xb1, // LOAD_FAST 1
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x45, // POP_JUMP_IF_TRUE 5
    0xb2, // LOAD_FAST 2
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd7, // BINARY_OP 0 __lt__
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x0b, // LOAD_CONST_OBJ 11
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x71, // LOAD_GLOBAL '_cells'
    0xb0, // LOAD_FAST 0
    0x13,0x71, // LOAD_ATTR '_cells'
    0xf3, // BINARY_OP 28 __sub__
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xf4, // BINARY_OP 29 __mul__
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x50, // LOAD_ATTR 'pages'
    0xf4, // BINARY_OP 29 __mul__
    0xf2, // BINARY_OP 27 __add__
    0x12,0x08, // LOAD_GLOBAL 'MAX_CELLS'
    0xd8, // BINARY_OP 1 __gt__
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x23,0x0d, // LOAD_CONST_OBJ 13
    0x12,0x08, // LOAD_GLOBAL 'MAX_CELLS'
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x71, // LOAD_GLOBAL '_cells'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xf4, // BINARY_OP 29 __mul__
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x50, // LOAD_ATTR 'pages'
    0xf4, // BINARY_OP 29 __mul__
    0xb0, // LOAD_FAST 0
    0x13,0x71, // LOAD_ATTR '_cells'
    0xf3, // BINARY_OP 28 __sub__
    0xe5, // BINARY_OP 14 __iadd__
    0x17,0x71, // STORE_GLOBAL '_cells'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xf4, // BINARY_OP 29 __mul__
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x50, // LOAD_ATTR 'pages'
    0xf4, // BINARY_OP 29 __mul__
    0xb0, // LOAD_FAST 0
    0x18,0x71, // STORE_ATTR '_cells'
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x18,0x5f, // STORE_ATTR 'width'
    0xb2, // LOAD_FAST 2
    0xb0, // LOAD_FAST 0
    0x18,0x60, // STORE_ATTR 'height'
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x56, // LOAD_METHOD '_req2'
    0x90, // LOAD_CONST_SMALL_INT 16
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_resize = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_resize,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 111,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 104,
        .line_info = fun_data_tdui_Canvas_resize + 9,
        .line_info_top = fun_data_tdui_Canvas_resize + 9,
        .opcodes = fun_data_tdui_Canvas_resize + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_resize fun_data_tdui_Canvas_resize[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_move
static const byte fun_data_tdui_Canvas_move[26] = {
    0x63,0x0e, // prelude
    0x69,0x82,0x10,0x81,0x23,0x81,0x24, // names: move, self, x, y
     // code info
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x56, // LOAD_METHOD '_req2'
    0x91, // LOAD_CONST_SMALL_INT 17
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x36,0x08, // CALL_METHOD 8
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_move = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_move,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 105,
        .line_info = fun_data_tdui_Canvas_move + 9,
        .line_info_top = fun_data_tdui_Canvas_move + 9,
        .opcodes = fun_data_tdui_Canvas_move + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_move fun_data_tdui_Canvas_move[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas__req2
static const byte fun_data_tdui_Canvas__req2[50] = {
    0xa1,0x90,0x04,0x26, // prelude
    0x81,0x56,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x81,0x77,0x81,0x78,0x50, // names: _req2, self, op, a, b, c, d, e, f, text
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x52, // POP_JUMP_IF_FALSE 18
    0x12,0x24, // LOAD_GLOBAL '_send'
    0xb1, // LOAD_FAST 1
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0x34,0x0b, // CALL_FUNCTION 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas__req2 = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas__req2,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 50,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 21,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 9,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 214,
        .line_info = fun_data_tdui_Canvas__req2 + 23,
        .line_info_top = fun_data_tdui_Canvas__req2 + 23,
        .opcodes = fun_data_tdui_Canvas__req2 + 23,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas__req2 fun_data_tdui_Canvas__req2[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas__alive
static const byte fun_data_tdui_Canvas__alive[12] = {
    0x09,0x08, // prelude
    0x81,0x15,0x82,0x10, // names: _alive, self
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas__alive = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas__alive,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 12,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 149,
        .line_info = fun_data_tdui_Canvas__alive + 6,
        .line_info_top = fun_data_tdui_Canvas__alive + 6,
        .opcodes = fun_data_tdui_Canvas__alive + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas__alive fun_data_tdui_Canvas__alive[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_picture
static const byte fun_data_tdui_Canvas_picture[149] = {
    0xf8,0x94,0x85,0x01,0x32, // prelude
    0x81,0x3a,0x82,0x10,0x82,0x26,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x81,0x31,0x81,0x33,0x50,0x81,0x2c,0x81,0x2e,0x81,0x2f, // names: picture, self, img, x, y, w, h, src, border, text, fg, scale, font
     // code info
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0xed,0x80, // POP_JUMP_IF_FALSE 109
    0xb6, // LOAD_FAST 6
    0x45,0x02, // JUMP_IF_TRUE_OR_POP 2
    0x23,0x0a, // LOAD_CONST_OBJ 10
    0x30,0x04, // UNPACK_SEQUENCE 4
    0xcc, // STORE_FAST 12
    0xcd, // STORE_FAST 13
    0xce, // STORE_FAST 14
    0xcf, // STORE_FAST 15
    0x12,0x82,0x03, // LOAD_GLOBAL 'bytes'
    0x32,0x00, // MAKE_FUNCTION 0
    0xbe, // LOAD_FAST 14
    0xbf, // LOAD_FAST 15
    0xb7, // LOAD_FAST 7
    0xb9, // LOAD_FAST 9
    0xba, // LOAD_FAST 10
    0x2a,0x05, // BUILD_TUPLE 5
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0x26,0x10, // STORE_FAST_N 16
    0xbb, // LOAD_FAST 11
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x58, // POP_JUMP_IF_FALSE 24
    0x24,0x10, // LOAD_FAST_N 16
    0x12,0x82,0x03, // LOAD_GLOBAL 'bytes'
    0xbb, // LOAD_FAST 11
    0x13,0x5d, // LOAD_ATTR 'id'
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0xbb, // LOAD_FAST 11
    0x13,0x5d, // LOAD_ATTR 'id'
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf1, // BINARY_OP 26 __rshift__
    0x2a,0x02, // BUILD_TUPLE 2
    0x34,0x01, // CALL_FUNCTION 1
    0xe5, // BINARY_OP 14 __iadd__
    0x26,0x10, // STORE_FAST_N 16
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0x9e, // LOAD_CONST_SMALL_INT 30
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb7, // LOAD_FAST 7
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xbb, // LOAD_FAST 11
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x82, // LOAD_CONST_SMALL_INT 2
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xed, // BINARY_OP 22 __or__
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xbc, // LOAD_FAST 12
    0xbd, // LOAD_FAST 13
    0x24,0x10, // LOAD_FAST_N 16
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb8, // LOAD_FAST 8
    0x34,0x01, // CALL_FUNCTION 1
    0x14,0x81,0x59, // LOAD_METHOD 'encode'
    0x36,0x00, // CALL_METHOD 0
    0xf2, // BINARY_OP 27 __add__
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
// child of tdui_Canvas_picture
// frozen bytecode for file tdui.py, scope tdui_Canvas_picture__lt_genexpr_gt_
static const byte fun_data_tdui_Canvas_picture__lt_genexpr_gt_[49] = {
    0xe1,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x22, // FOR_ITER 34
    0xc1, // STORE_FAST 1
    0x23,0x11, // LOAD_CONST_OBJ 17
    0x5f, // GET_ITER_STACK
    0x4b,0x1a, // FOR_ITER 26
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf8, // BINARY_OP 33 __mod__
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x47, // POP_JUMP_IF_FALSE 7
    0xb1, // LOAD_FAST 1
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0x42,0x47, // JUMP 7
    0xb1, // LOAD_FAST 1
    0x88, // LOAD_CONST_SMALL_INT 8
    0xf1, // BINARY_OP 26 __rshift__
    0x22,0x81,0x7f, // LOAD_CONST_SMALL_INT 255
    0xef, // BINARY_OP 24 __and__
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x24, // JUMP -28
    0x42,0x1c, // JUMP -36
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_picture__lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_Canvas_picture__lt_genexpr_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 49,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui_Canvas_picture__lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_tdui_Canvas_picture__lt_genexpr_gt_ + 7,
        .opcodes = fun_data_tdui_Canvas_picture__lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_picture__lt_genexpr_gt_ fun_data_tdui_Canvas_picture__lt_genexpr_gt_[0]
#endif

static const mp_raw_code_t *const children_tdui_Canvas_picture[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_picture__lt_genexpr_gt_,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_picture = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_picture,
    .children = (void *)&children_tdui_Canvas_picture,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 149,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 32,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 12,
        .n_kwonly_args = 0,
        .n_def_pos_args = 6,
        .qstr_block_name_idx = 186,
        .line_info = fun_data_tdui_Canvas_picture + 30,
        .line_info_top = fun_data_tdui_Canvas_picture + 30,
        .opcodes = fun_data_tdui_Canvas_picture + 30,
    },
    #endif
    #endif
};

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_font_text
static const byte fun_data_tdui_Canvas_font_text[174] = {
    0xd8,0x95,0x84,0x01,0x32, // prelude
    0x81,0x3c,0x82,0x10,0x81,0x2f,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x50,0x81,0x2c,0x82,0x16,0x81,0x3d,0x81,0x3f,0x82,0x30, // names: font_text, self, font, x, y, w, h, text, fg, bg, align, valign, wrap
     // code info
    0x12,0x82,0x0c, // LOAD_GLOBAL 'isinstance'
    0xb1, // LOAD_FAST 1
    0x12,0x19, // LOAD_GLOBAL 'Font'
    0x34,0x02, // CALL_FUNCTION 2
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0xdd, // BINARY_OP 6 <in>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x0f, // LOAD_CONST_OBJ 15
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x2c,0x03, // BUILD_MAP 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x34, // LOAD_CONST_STRING 'left'
    0x62, // STORE_MAP
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x81,0x3e, // LOAD_CONST_STRING 'center'
    0x62, // STORE_MAP
    0x82, // LOAD_CONST_SMALL_INT 2
    0x10,0x81,0x5a, // LOAD_CONST_STRING 'right'
    0x62, // STORE_MAP
    0xcc, // STORE_FAST 12
    0x2c,0x03, // BUILD_MAP 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0x10,0x35, // LOAD_CONST_STRING 'top'
    0x62, // STORE_MAP
    0x81, // LOAD_CONST_SMALL_INT 1
    0x10,0x81,0x40, // LOAD_CONST_STRING 'middle'
    0x62, // STORE_MAP
    0x82, // LOAD_CONST_SMALL_INT 2
    0x10,0x81,0x5b, // LOAD_CONST_STRING 'bottom'
    0x62, // STORE_MAP
    0xcd, // STORE_FAST 13
    0xb9, // LOAD_FAST 9
    0xbc, // LOAD_FAST 12
    0xdd, // BINARY_OP 6 <in>
    0xd3, // UNARY_OP 3 <not>
    0x43,0x50, // POP_JUMP_IF_TRUE 16
    0xba, // LOAD_FAST 10
    0xbd, // LOAD_FAST 13
    0xdd, // BINARY_OP 6 <in>
    0xd3, // UNARY_OP 3 <not>
    0x43,0x4a, // POP_JUMP_IF_TRUE 10
    0xb4, // LOAD_FAST 4
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x45, // POP_JUMP_IF_TRUE 5
    0xb5, // LOAD_FAST 5
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd7, // BINARY_OP 0 __lt__
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0x12,0x82,0x12, // LOAD_GLOBAL 'ValueError'
    0x23,0x10, // LOAD_CONST_OBJ 16
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0xb0, // LOAD_FAST 0
    0x13,0x5a, // LOAD_ATTR 'win'
    0x13,0x61, // LOAD_ATTR 'is_open'
    0x44,0x6b, // POP_JUMP_IF_FALSE 43
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x25, // LOAD_METHOD 'send'
    0xa3, // LOAD_CONST_SMALL_INT 35
    0xb0, // LOAD_FAST 0
    0x13,0x5d, // LOAD_ATTR 'id'
    0xb1, // LOAD_FAST 1
    0x13,0x5d, // LOAD_ATTR 'id'
    0xbc, // LOAD_FAST 12
    0xb9, // LOAD_FAST 9
    0x55, // LOAD_SUBSCR
    0xbd, // LOAD_FAST 13
    0xba, // LOAD_FAST 10
    0x55, // LOAD_SUBSCR
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf0, // BINARY_OP 25 __lshift__
    0xed, // BINARY_OP 22 __or__
    0xbb, // LOAD_FAST 11
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x90, // LOAD_CONST_SMALL_INT 16
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0xed, // BINARY_OP 22 __or__
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb6, // LOAD_FAST 6
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x0b, // CALL_METHOD 11
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_font_text = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_font_text,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 174,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 28,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 12,
        .n_kwonly_args = 0,
        .n_def_pos_args = 5,
        .qstr_block_name_idx = 188,
        .line_info = fun_data_tdui_Canvas_font_text + 30,
        .line_info_top = fun_data_tdui_Canvas_font_text + 30,
        .opcodes = fun_data_tdui_Canvas_font_text + 30,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_font_text fun_data_tdui_Canvas_font_text[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_pwidth
static const byte fun_data_tdui_Canvas_pwidth[12] = {
    0x11,0x08, // prelude
    0x81,0x5c,0x82,0x10, // names: pwidth, self
     // code info
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x5f, // LOAD_ATTR 'width'
    0xf4, // BINARY_OP 29 __mul__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_pwidth = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_pwidth,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 12,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 220,
        .line_info = fun_data_tdui_Canvas_pwidth + 6,
        .line_info_top = fun_data_tdui_Canvas_pwidth + 6,
        .opcodes = fun_data_tdui_Canvas_pwidth + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_pwidth fun_data_tdui_Canvas_pwidth[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_pheight
static const byte fun_data_tdui_Canvas_pheight[12] = {
    0x11,0x08, // prelude
    0x81,0x5d,0x82,0x10, // names: pheight, self
     // code info
    0x82, // LOAD_CONST_SMALL_INT 2
    0xb0, // LOAD_FAST 0
    0x13,0x60, // LOAD_ATTR 'height'
    0xf4, // BINARY_OP 29 __mul__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_pheight = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_pheight,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 12,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 221,
        .line_info = fun_data_tdui_Canvas_pheight + 6,
        .line_info_top = fun_data_tdui_Canvas_pheight + 6,
        .opcodes = fun_data_tdui_Canvas_pheight + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_pheight fun_data_tdui_Canvas_pheight[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas__event
static const byte fun_data_tdui_Canvas__event[214] = {
    0x83,0x14,0x1e, // prelude
    0x44,0x82,0x10,0x81,0x71,0x81,0x73,0x81,0x74,0x81,0x75,0x81,0x76,0x82,0x13, // names: _event, self, op, a, b, c, d, t
     // code info
    0xb1, // LOAD_FAST 1
    0x85, // LOAD_CONST_SMALL_INT 5
    0xdc, // BINARY_OP 5 __ne__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x37, // LOAD_ATTR '_held'
    0xc7, // STORE_FAST 7
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x7c, // POP_JUMP_IF_FALSE 60
    0xb5, // LOAD_FAST 5
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x77, // POP_JUMP_IF_FALSE 55
    0xb7, // LOAD_FAST 7
    0x43,0x74, // POP_JUMP_IF_TRUE 52
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x35, // LOAD_ATTR '_buttons'
    0x5f, // GET_ITER_STACK
    0x4b,0x2a, // FOR_ITER 42
    0xc8, // STORE_FAST 8
    0xb8, // LOAD_FAST 8
    0x14,0x81,0x42, // LOAD_METHOD '_hit'
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x36,0x02, // CALL_METHOD 2
    0x44,0x5d, // POP_JUMP_IF_FALSE 29
    0xb8, // LOAD_FAST 8
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x37, // STORE_ATTR '_held'
    0xb8, // LOAD_FAST 8
    0x14,0x81,0x36, // LOAD_METHOD 'draw'
    0x52, // LOAD_CONST_TRUE
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb8, // LOAD_FAST 8
    0x13,0x81,0x27, // LOAD_ATTR 'on_press'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb8, // LOAD_FAST 8
    0x14,0x81,0x27, // LOAD_METHOD 'on_press'
    0xb8, // LOAD_FAST 8
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0x42,0x14, // JUMP -44
    0x42,0xe3,0x80, // JUMP 99
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd9, // BINARY_OP 2 __eq__
    0x44,0xd8,0x80, // POP_JUMP_IF_FALSE 88
    0xb5, // LOAD_FAST 5
    0x81, // LOAD_CONST_SMALL_INT 1
    0xd9, // BINARY_OP 2 __eq__
    0x44,0xd2,0x80, // POP_JUMP_IF_FALSE 82
    0xb7, // LOAD_FAST 7
    0x44,0xce,0x80, // POP_JUMP_IF_FALSE 78
    0x51, // LOAD_CONST_NONE
    0xb0, // LOAD_FAST 0
    0x18,0x81,0x37, // STORE_ATTR '_held'
    0xb7, // LOAD_FAST 7
    0x14,0x81,0x36, // LOAD_METHOD 'draw'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0xb7, // LOAD_FAST 7
    0x13,0x81,0x28, // LOAD_ATTR 'on_release'
    0x44,0x48, // POP_JUMP_IF_FALSE 8
    0xb7, // LOAD_FAST 7
    0x14,0x81,0x28, // LOAD_METHOD 'on_release'
    0xb7, // LOAD_FAST 7
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb7, // LOAD_FAST 7
    0x14,0x81,0x42, // LOAD_METHOD '_hit'
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x36,0x02, // CALL_METHOD 2
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0xb7, // LOAD_FAST 7
    0x13,0x7e, // LOAD_ATTR 'on_click'
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb7, // LOAD_FAST 7
    0x14,0x7e, // LOAD_METHOD 'on_click'
    0xb7, // LOAD_FAST 7
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x5a, // JUMP 26
    0xb7, // LOAD_FAST 7
    0x14,0x81,0x42, // LOAD_METHOD '_hit'
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0x36,0x02, // CALL_METHOD 2
    0x43,0x50, // POP_JUMP_IF_TRUE 16
    0xb7, // LOAD_FAST 7
    0x13,0x81,0x29, // LOAD_ATTR 'on_cancel'
    0x44,0x4a, // POP_JUMP_IF_FALSE 10
    0xb7, // LOAD_FAST 7
    0x14,0x81,0x29, // LOAD_METHOD 'on_cancel'
    0xb7, // LOAD_FAST 7
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0x42,0x40, // JUMP 0
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb7, // LOAD_FAST 7
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb0, // LOAD_FAST 0
    0x13,0x81,0x51, // LOAD_ATTR 'on_mouse'
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0xb0, // LOAD_FAST 0
    0x14,0x81,0x51, // LOAD_METHOD 'on_mouse'
    0xb0, // LOAD_FAST 0
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0x36,0x05, // CALL_METHOD 5
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas__event = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas__event,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 214,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 7,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 68,
        .line_info = fun_data_tdui_Canvas__event + 18,
        .line_info_top = fun_data_tdui_Canvas__event + 18,
        .opcodes = fun_data_tdui_Canvas__event + 18,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas__event fun_data_tdui_Canvas__event[0]
#endif

// child of tdui_Canvas
// frozen bytecode for file tdui.py, scope tdui_Canvas_button
static const byte fun_data_tdui_Canvas_button[115] = {
    0xe2,0xa4,0x80,0x84,0x80,0x01,0x54, // prelude
    0x73,0x82,0x10,0x81,0x23,0x81,0x24,0x81,0x25,0x81,0x26,0x50,0x7e,0x7d,0x81,0x2a,0x81,0x2b,0x81,0x2c,0x81,0x2d,0x81,0x2e,0x81,0x30,0x81,0x31,0x81,0x32,0x81,0x33,0x81,0x27,0x81,0x28,0x81,0x29,0x81,0x2f,0x81,0x1a, // names: button, self, x, y, w, h, text, on_click, color, down, rim, fg, radius, scale, sheet, src, src_down, border, on_press, on_release, on_cancel, font, gradient
     // code info
    0xb7, // LOAD_FAST 7
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0x12,0x2d, // LOAD_GLOBAL 'rgb'
    0xa8, // LOAD_CONST_SMALL_INT 40
    0x22,0x80,0x6e, // LOAD_CONST_SMALL_INT 110
    0x22,0x81,0x52, // LOAD_CONST_SMALL_INT 210
    0x34,0x03, // CALL_FUNCTION 3
    0xc7, // STORE_FAST 7
    0xb8, // LOAD_FAST 8
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x44,0x4b, // POP_JUMP_IF_FALSE 11
    0x12,0x2d, // LOAD_GLOBAL 'rgb'
    0x94, // LOAD_CONST_SMALL_INT 20
    0x22,0x3c, // LOAD_CONST_SMALL_INT 60
    0x22,0x81,0x0c, // LOAD_CONST_SMALL_INT 140
    0x34,0x03, // CALL_FUNCTION 3
    0xc8, // STORE_FAST 8
    0x12,0x17, // LOAD_GLOBAL 'CanvasButton'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0xb8, // LOAD_FAST 8
    0xb9, // LOAD_FAST 9
    0xba, // LOAD_FAST 10
    0xbb, // LOAD_FAST 11
    0xbc, // LOAD_FAST 12
    0xbd, // LOAD_FAST 13
    0xbe, // LOAD_FAST 14
    0xbf, // LOAD_FAST 15
    0x24,0x10, // LOAD_FAST_N 16
    0x24,0x11, // LOAD_FAST_N 17
    0x24,0x12, // LOAD_FAST_N 18
    0x24,0x13, // LOAD_FAST_N 19
    0x24,0x14, // LOAD_FAST_N 20
    0x24,0x15, // LOAD_FAST_N 21
    0x34,0x16, // CALL_FUNCTION 22
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_Canvas_button = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas_button,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 115,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 45,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 22,
        .n_kwonly_args = 0,
        .n_def_pos_args = 16,
        .qstr_block_name_idx = 115,
        .line_info = fun_data_tdui_Canvas_button + 49,
        .line_info_top = fun_data_tdui_Canvas_button + 49,
        .opcodes = fun_data_tdui_Canvas_button + 49,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_Canvas_button fun_data_tdui_Canvas_button[0]
#endif

static const mp_raw_code_t *const children_tdui_Canvas[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas___init__,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_put,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_fill,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_clear,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_big,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas__page,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_draw_to,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_show,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_copy_page,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_resize,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_move,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas__req2,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas__alive,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_picture,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_font_text,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_pwidth,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_pheight,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas__event,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas_button,
};

static const mp_raw_code_truncated_t proto_fun_tdui_Canvas = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_Canvas,
    .children = (void *)&children_tdui_Canvas,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 196,
    .n_children = 19,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 26,
        .line_info = fun_data_tdui_Canvas + 4,
        .line_info_top = fun_data_tdui_Canvas + 4,
        .opcodes = fun_data_tdui_Canvas + 5,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_big_size
static const byte fun_data_tdui_big_size[51] = {
    0xb2,0x01,0x08, // prelude
    0x36,0x50,0x81,0x2e, // names: big_size, text, scale
     // code info
    0x12,0x81,0x7b, // LOAD_GLOBAL 'len'
    0xb0, // LOAD_FAST 0
    0x34,0x01, // CALL_FUNCTION 1
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x43,0x43, // POP_JUMP_IF_TRUE 3
    0x23,0x02, // LOAD_CONST_OBJ 2
    0x63, // RETURN_VALUE
    0x12,0x82,0x08, // LOAD_GLOBAL 'sum'
    0x32,0x00, // MAKE_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x34,0x01, // CALL_FUNCTION 1
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0xb2, // LOAD_FAST 2
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf3, // BINARY_OP 28 __sub__
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0xf2, // BINARY_OP 27 __add__
    0xc3, // STORE_FAST 3
    0xb3, // LOAD_FAST 3
    0x85, // LOAD_CONST_SMALL_INT 5
    0xb1, // LOAD_FAST 1
    0xf4, // BINARY_OP 29 __mul__
    0x81, // LOAD_CONST_SMALL_INT 1
    0xf2, // BINARY_OP 27 __add__
    0x82, // LOAD_CONST_SMALL_INT 2
    0xf6, // BINARY_OP 31 __floordiv__
    0x2a,0x02, // BUILD_TUPLE 2
    0x63, // RETURN_VALUE
};
// child of tdui_big_size
// frozen bytecode for file tdui.py, scope tdui_big_size__lt_genexpr_gt_
static const byte fun_data_tdui_big_size__lt_genexpr_gt_[31] = {
    0xb9,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x10, // FOR_ITER 16
    0xc1, // STORE_FAST 1
    0xb1, // LOAD_FAST 1
    0x10,0x81,0x5e, // LOAD_CONST_STRING '.:'
    0xdd, // BINARY_OP 6 <in>
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x83, // LOAD_CONST_SMALL_INT 3
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x2e, // JUMP -18
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_big_size__lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_big_size__lt_genexpr_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 31,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui_big_size__lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_tdui_big_size__lt_genexpr_gt_ + 7,
        .opcodes = fun_data_tdui_big_size__lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_big_size__lt_genexpr_gt_ fun_data_tdui_big_size__lt_genexpr_gt_[0]
#endif

static const mp_raw_code_t *const children_tdui_big_size[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_big_size__lt_genexpr_gt_,
};

static const mp_raw_code_truncated_t proto_fun_tdui_big_size = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_big_size,
    .children = (void *)&children_tdui_big_size,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 51,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 7,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 54,
        .line_info = fun_data_tdui_big_size + 7,
        .line_info_top = fun_data_tdui_big_size + 7,
        .opcodes = fun_data_tdui_big_size + 7,
    },
    #endif
    #endif
};

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__run_timers
static const byte fun_data_tdui__run_timers[115] = {
    0x61,0x04, // prelude
    0x37,0x34, // names: _run_timers, left
     // code info
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x38, // LOAD_METHOD 'ticks_ms'
    0x36,0x00, // CALL_METHOD 0
    0xc1, // STORE_FAST 1
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x51, // LOAD_CONST_NONE
    0x51, // LOAD_CONST_NONE
    0x2e,0x02, // BUILD_SLICE 2
    0x55, // LOAD_SUBSCR
    0x5f, // GET_ITER_STACK
    0x4b,0x34, // FOR_ITER 52
    0xc2, // STORE_FAST 2
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x39, // LOAD_METHOD 'ticks_diff'
    0xb2, // LOAD_FAST 2
    0x13,0x3a, // LOAD_ATTR 'due'
    0xb1, // LOAD_FAST 1
    0x36,0x02, // CALL_METHOD 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xda, // BINARY_OP 3 __le__
    0x44,0x63, // POP_JUMP_IF_FALSE 35
    0xb2, // LOAD_FAST 2
    0x13,0x3b, // LOAD_ATTR 'repeat'
    0x44,0x4f, // POP_JUMP_IF_FALSE 15
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x3c, // LOAD_METHOD 'ticks_add'
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x13,0x3d, // LOAD_ATTR 'ms'
    0x36,0x02, // CALL_METHOD 2
    0xb2, // LOAD_FAST 2
    0x18,0x3a, // STORE_ATTR 'due'
    0x42,0x49, // JUMP 9
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x14,0x3e, // LOAD_METHOD 'remove'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0x59, // POP_TOP
    0xb2, // LOAD_FAST 2
    0x14,0x3f, // LOAD_METHOD 'fn'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x42,0x0a, // JUMP -54
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x38, // LOAD_METHOD 'ticks_ms'
    0x36,0x00, // CALL_METHOD 0
    0xc1, // STORE_FAST 1
    0x12,0x81,0x6a, // LOAD_GLOBAL '_timers'
    0x5f, // GET_ITER_STACK
    0x4b,0x14, // FOR_ITER 20
    0xc2, // STORE_FAST 2
    0x12,0x82,0x09, // LOAD_GLOBAL 'min'
    0xb0, // LOAD_FAST 0
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x39, // LOAD_METHOD 'ticks_diff'
    0xb2, // LOAD_FAST 2
    0x13,0x3a, // LOAD_ATTR 'due'
    0xb1, // LOAD_FAST 1
    0x36,0x02, // CALL_METHOD 2
    0x34,0x02, // CALL_FUNCTION 2
    0xc0, // STORE_FAST 0
    0x42,0x2a, // JUMP -22
    0x12,0x82,0x06, // LOAD_GLOBAL 'max'
    0xb0, // LOAD_FAST 0
    0x80, // LOAD_CONST_SMALL_INT 0
    0x34,0x02, // CALL_FUNCTION 2
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__run_timers = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__run_timers,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 115,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 13,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 55,
        .line_info = fun_data_tdui__run_timers + 4,
        .line_info_top = fun_data_tdui__run_timers + 4,
        .opcodes = fun_data_tdui__run_timers + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__run_timers fun_data_tdui__run_timers[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__dispatch
static const byte fun_data_tdui__dispatch[87] = {
    0x81,0x10,0x06, // prelude
    0x40,0x82,0x0a, // names: _dispatch, ev
     // code info
    0xb0, // LOAD_FAST 0
    0x30,0x07, // UNPACK_SEQUENCE 7
    0xc1, // STORE_FAST 1
    0xc2, // STORE_FAST 2
    0xc3, // STORE_FAST 3
    0xc4, // STORE_FAST 4
    0xc5, // STORE_FAST 5
    0xc6, // STORE_FAST 6
    0xc7, // STORE_FAST 7
    0xb1, // LOAD_FAST 1
    0x86, // LOAD_CONST_SMALL_INT 6
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x52, // POP_JUMP_IF_FALSE 18
    0xb2, // LOAD_FAST 2
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xdd, // BINARY_OP 6 <in>
    0x44,0x49, // POP_JUMP_IF_FALSE 9
    0xb3, // LOAD_FAST 3
    0xb7, // LOAD_FAST 7
    0x2a,0x02, // BUILD_TUPLE 2
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xb2, // LOAD_FAST 2
    0x56, // STORE_SUBSCR
    0x42,0x6e, // JUMP 46
    0xb1, // LOAD_FAST 1
    0x87, // LOAD_CONST_SMALL_INT 7
    0xd9, // BINARY_OP 2 __eq__
    0x44,0x4e, // POP_JUMP_IF_FALSE 14
    0x12,0x82,0x0b, // LOAD_GLOBAL 'OSError'
    0x10,0x41, // LOAD_CONST_STRING 'tdui: '
    0xb7, // LOAD_FAST 7
    0x45,0x02, // JUMP_IF_TRUE_OR_POP 2
    0x10,0x42, // LOAD_CONST_STRING 'error'
    0xf2, // BINARY_OP 27 __add__
    0x34,0x01, // CALL_FUNCTION 1
    0x65, // RAISE_OBJ
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0x14,0x43, // LOAD_METHOD 'get'
    0xb2, // LOAD_FAST 2
    0x36,0x01, // CALL_METHOD 1
    0xc8, // STORE_FAST 8
    0xb8, // LOAD_FAST 8
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0xd3, // UNARY_OP 3 <not>
    0x44,0x4c, // POP_JUMP_IF_FALSE 12
    0xb8, // LOAD_FAST 8
    0x14,0x44, // LOAD_METHOD '_event'
    0xb1, // LOAD_FAST 1
    0xb3, // LOAD_FAST 3
    0xb4, // LOAD_FAST 4
    0xb5, // LOAD_FAST 5
    0xb6, // LOAD_FAST 6
    0xb7, // LOAD_FAST 7
    0x36,0x06, // CALL_METHOD 6
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__dispatch = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__dispatch,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 87,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 17,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 64,
        .line_info = fun_data_tdui__dispatch + 6,
        .line_info_top = fun_data_tdui__dispatch + 6,
        .opcodes = fun_data_tdui__dispatch + 6,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__dispatch fun_data_tdui__dispatch[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_poll
static const byte fun_data_tdui_poll[85] = {
    0xd1,0x01,0x04, // prelude
    0x45,0x3d, // names: poll, ms
     // code info
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x3c, // LOAD_METHOD 'ticks_add'
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x38, // LOAD_METHOD 'ticks_ms'
    0x36,0x00, // CALL_METHOD 0
    0xb0, // LOAD_FAST 0
    0x36,0x02, // CALL_METHOD 2
    0xc1, // STORE_FAST 1
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x46, // LOAD_METHOD 'recv'
    0x12,0x37, // LOAD_GLOBAL '_run_timers'
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x39, // LOAD_METHOD 'ticks_diff'
    0xb1, // LOAD_FAST 1
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x38, // LOAD_METHOD 'ticks_ms'
    0x36,0x00, // CALL_METHOD 0
    0x36,0x02, // CALL_METHOD 2
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x01, // CALL_METHOD 1
    0xc2, // STORE_FAST 2
    0x42,0x4e, // JUMP 14
    0x12,0x40, // LOAD_GLOBAL '_dispatch'
    0xb2, // LOAD_FAST 2
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x46, // LOAD_METHOD 'recv'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x36,0x01, // CALL_METHOD 1
    0xc2, // STORE_FAST 2
    0xb2, // LOAD_FAST 2
    0x43,0x2f, // POP_JUMP_IF_TRUE -17
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x39, // LOAD_METHOD 'ticks_diff'
    0xb1, // LOAD_FAST 1
    0x12,0x04, // LOAD_GLOBAL 'time'
    0x14,0x38, // LOAD_METHOD 'ticks_ms'
    0x36,0x00, // CALL_METHOD 0
    0x36,0x02, // CALL_METHOD 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0xda, // BINARY_OP 3 __le__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0x42,0x00, // JUMP -64
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_poll = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_poll,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 85,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 11,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 69,
        .line_info = fun_data_tdui_poll + 5,
        .line_info_top = fun_data_tdui_poll + 5,
        .opcodes = fun_data_tdui_poll + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_poll fun_data_tdui_poll[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_run
static const byte fun_data_tdui_run[22] = {
    0x08,0x02, // prelude
    0x47, // names: run
     // code info
    0x42,0x48, // JUMP 8
    0x12,0x45, // LOAD_GLOBAL 'poll'
    0x22,0x80,0x64, // LOAD_CONST_SMALL_INT 100
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x12,0x81,0x6c, // LOAD_GLOBAL '_windows'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd8, // BINARY_OP 1 __gt__
    0x43,0x31, // POP_JUMP_IF_TRUE -15
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_run = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_run,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 22,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 2,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 71,
        .line_info = fun_data_tdui_run + 3,
        .line_info_top = fun_data_tdui_run + 3,
        .opcodes = fun_data_tdui_run + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_run fun_data_tdui_run[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_run_async
static const byte fun_data_tdui_run_async[36] = {
    0x98,0x40,0x02, // prelude
    0x48, // names: run_async
     // code info
    0x80, // LOAD_CONST_SMALL_INT 0
    0x51, // LOAD_CONST_NONE
    0x1b,0x49, // IMPORT_NAME 'asyncio'
    0xc0, // STORE_FAST 0
    0x42,0x50, // JUMP 16
    0x12,0x45, // LOAD_GLOBAL 'poll'
    0x80, // LOAD_CONST_SMALL_INT 0
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0xb0, // LOAD_FAST 0
    0x14,0x4a, // LOAD_METHOD 'sleep_ms'
    0x94, // LOAD_CONST_SMALL_INT 20
    0x36,0x01, // CALL_METHOD 1
    0x5e, // GET_ITER
    0x51, // LOAD_CONST_NONE
    0x68, // YIELD_FROM
    0x59, // POP_TOP
    0x12,0x81,0x6c, // LOAD_GLOBAL '_windows'
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd8, // BINARY_OP 1 __gt__
    0x43,0x29, // POP_JUMP_IF_TRUE -23
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_run_async = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_run_async,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 36,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 4,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 72,
        .line_info = fun_data_tdui_run_async + 4,
        .line_info_top = fun_data_tdui_run_async + 4,
        .opcodes = fun_data_tdui_run_async + 4,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_run_async fun_data_tdui_run_async[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_quit
static const byte fun_data_tdui_quit[38] = {
    0x38,0x02, // prelude
    0x4b, // names: quit
     // code info
    0x12,0x77, // LOAD_GLOBAL 'list'
    0x12,0x81,0x68, // LOAD_GLOBAL '_objs'
    0x14,0x4c, // LOAD_METHOD 'values'
    0x36,0x00, // CALL_METHOD 0
    0x34,0x01, // CALL_FUNCTION 1
    0x5f, // GET_ITER_STACK
    0x4b,0x13, // FOR_ITER 19
    0xc0, // STORE_FAST 0
    0x12,0x82,0x0c, // LOAD_GLOBAL 'isinstance'
    0xb0, // LOAD_FAST 0
    0x12,0x0d, // LOAD_GLOBAL 'Window'
    0x34,0x02, // CALL_FUNCTION 2
    0x44,0x46, // POP_JUMP_IF_FALSE 6
    0xb0, // LOAD_FAST 0
    0x14,0x4d, // LOAD_METHOD 'close'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x42,0x2b, // JUMP -21
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_quit = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_quit,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 38,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 75,
        .line_info = fun_data_tdui_quit + 3,
        .line_info_top = fun_data_tdui_quit + 3,
        .opcodes = fun_data_tdui_quit + 3,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_quit fun_data_tdui_quit[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui__request
static const byte fun_data_tdui__request[75] = {
    0xef,0x01,0x0c, // prelude
    0x4e,0x81,0x71,0x50,0x81,0x72, // names: _request, op, text, flags
     // code info
    0x12,0x02, // LOAD_GLOBAL '_tdui'
    0x14,0x4f, // LOAD_METHOD 'open'
    0x36,0x00, // CALL_METHOD 0
    0x59, // POP_TOP
    0x12,0x23, // LOAD_GLOBAL '_new_id'
    0x34,0x00, // CALL_FUNCTION 0
    0xc3, // STORE_FAST 3
    0x51, // LOAD_CONST_NONE
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xb3, // LOAD_FAST 3
    0x56, // STORE_SUBSCR
    0x49,0x24, // SETUP_FINALLY 36
    0x12,0x24, // LOAD_GLOBAL '_send'
    0xb0, // LOAD_FAST 0
    0xb3, // LOAD_FAST 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0xb2, // LOAD_FAST 2
    0x10,0x50, // LOAD_CONST_STRING 'text'
    0xb1, // LOAD_FAST 1
    0x34,0x82,0x04, // CALL_FUNCTION 260
    0x59, // POP_TOP
    0x42,0x46, // JUMP 6
    0x12,0x45, // LOAD_GLOBAL 'poll'
    0x94, // LOAD_CONST_SMALL_INT 20
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xb3, // LOAD_FAST 3
    0x55, // LOAD_SUBSCR
    0x51, // LOAD_CONST_NONE
    0xde, // BINARY_OP 7 <is>
    0x43,0x31, // POP_JUMP_IF_TRUE -15
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xb3, // LOAD_FAST 3
    0x55, // LOAD_SUBSCR
    0x63, // RETURN_VALUE
    0x12,0x81,0x69, // LOAD_GLOBAL '_results'
    0xb3, // LOAD_FAST 3
    0x53, // LOAD_NULL
    0x5b, // ROT_THREE
    0x56, // STORE_SUBSCR
    0x5d, // END_FINALLY
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui__request = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__request,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 75,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 14,
        .n_exc_stack = 1,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 78,
        .line_info = fun_data_tdui__request + 9,
        .line_info_top = fun_data_tdui__request + 9,
        .opcodes = fun_data_tdui__request + 9,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui__request fun_data_tdui__request[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_msgbox
static const byte fun_data_tdui_msgbox[24] = {
    0xc3,0x01,0x0a, // prelude
    0x51,0x66,0x50,0x82,0x0d, // names: msgbox, title, text, buttons
     // code info
    0x12,0x4e, // LOAD_GLOBAL '_request'
    0x8d, // LOAD_CONST_SMALL_INT 13
    0x10,0x52, // LOAD_CONST_STRING '%s\x00%s\x00%s'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x2a,0x03, // BUILD_TUPLE 3
    0xf8, // BINARY_OP 33 __mod__
    0x34,0x02, // CALL_FUNCTION 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_msgbox = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_msgbox,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 24,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 3,
        .n_kwonly_args = 0,
        .n_def_pos_args = 1,
        .qstr_block_name_idx = 81,
        .line_info = fun_data_tdui_msgbox + 8,
        .line_info_top = fun_data_tdui_msgbox + 8,
        .opcodes = fun_data_tdui_msgbox + 8,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_msgbox fun_data_tdui_msgbox[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_confirm
static const byte fun_data_tdui_confirm[16] = {
    0x2a,0x06, // prelude
    0x53,0x66,0x50, // names: confirm, title, text
     // code info
    0x12,0x51, // LOAD_GLOBAL 'msgbox'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0x10,0x54, // LOAD_CONST_STRING 'Yes|No'
    0x34,0x03, // CALL_FUNCTION 3
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd9, // BINARY_OP 2 __eq__
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_confirm = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_confirm,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 16,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 6,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 2,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 83,
        .line_info = fun_data_tdui_confirm + 5,
        .line_info_top = fun_data_tdui_confirm + 5,
        .opcodes = fun_data_tdui_confirm + 5,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_confirm fun_data_tdui_confirm[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_inputbox
static const byte fun_data_tdui_inputbox[48] = {
    0xd8,0x84,0x01,0x0e, // prelude
    0x55,0x66,0x82,0x0e,0x50,0x82,0x0f, // names: inputbox, title, prompt, text, secret
     // code info
    0x12,0x4e, // LOAD_GLOBAL '_request'
    0x8e, // LOAD_CONST_SMALL_INT 14
    0x10,0x52, // LOAD_CONST_STRING '%s\x00%s\x00%s'
    0xb0, // LOAD_FAST 0
    0xb1, // LOAD_FAST 1
    0xb2, // LOAD_FAST 2
    0x2a,0x03, // BUILD_TUPLE 3
    0xf8, // BINARY_OP 33 __mod__
    0xb3, // LOAD_FAST 3
    0x44,0x43, // POP_JUMP_IF_FALSE 3
    0x81, // LOAD_CONST_SMALL_INT 1
    0x42,0x41, // JUMP 1
    0x80, // LOAD_CONST_SMALL_INT 0
    0x34,0x03, // CALL_FUNCTION 3
    0x30,0x02, // UNPACK_SEQUENCE 2
    0xc4, // STORE_FAST 4
    0xc5, // STORE_FAST 5
    0xb4, // LOAD_FAST 4
    0x80, // LOAD_CONST_SMALL_INT 0
    0xd7, // BINARY_OP 0 __lt__
    0x44,0x42, // POP_JUMP_IF_FALSE 2
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
    0xb5, // LOAD_FAST 5
    0x45,0x02, // JUMP_IF_TRUE_OR_POP 2
    0x10,0x1c, // LOAD_CONST_STRING ''
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_inputbox = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_inputbox,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 48,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 12,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 4,
        .n_kwonly_args = 0,
        .n_def_pos_args = 2,
        .qstr_block_name_idx = 85,
        .line_info = fun_data_tdui_inputbox + 11,
        .line_info_top = fun_data_tdui_inputbox + 11,
        .opcodes = fun_data_tdui_inputbox + 11,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_inputbox fun_data_tdui_inputbox[0]
#endif

// child of tdui__lt_module_gt_
// frozen bytecode for file tdui.py, scope tdui_menu
static const byte fun_data_tdui_menu[30] = {
    0x41,0x06, // prelude
    0x56,0x81,0x0a, // names: menu, items
     // code info
    0x12,0x4e, // LOAD_GLOBAL '_request'
    0x8f, // LOAD_CONST_SMALL_INT 15
    0x10,0x57, // LOAD_CONST_STRING '\x00'
    0x14,0x58, // LOAD_METHOD 'join'
    0x32,0x00, // MAKE_FUNCTION 0
    0xb0, // LOAD_FAST 0
    0x51, // LOAD_CONST_NONE
    0x94, // LOAD_CONST_SMALL_INT 20
    0x2e,0x02, // BUILD_SLICE 2
    0x55, // LOAD_SUBSCR
    0x5e, // GET_ITER
    0x34,0x01, // CALL_FUNCTION 1
    0x36,0x01, // CALL_METHOD 1
    0x34,0x02, // CALL_FUNCTION 2
    0x80, // LOAD_CONST_SMALL_INT 0
    0x55, // LOAD_SUBSCR
    0x63, // RETURN_VALUE
};
// child of tdui_menu
// frozen bytecode for file tdui.py, scope tdui_menu__lt_genexpr_gt_
static const byte fun_data_tdui_menu__lt_genexpr_gt_[26] = {
    0xb9,0x40,0x08, // prelude
    0x81,0x1f,0x82,0x17, // names: <genexpr>, *
     // code info
    0x53, // LOAD_NULL
    0xb0, // LOAD_FAST 0
    0x53, // LOAD_NULL
    0x53, // LOAD_NULL
    0x4b,0x0b, // FOR_ITER 11
    0xc1, // STORE_FAST 1
    0x12,0x82,0x05, // LOAD_GLOBAL 'str'
    0xb1, // LOAD_FAST 1
    0x34,0x01, // CALL_FUNCTION 1
    0x67, // YIELD_VALUE
    0x59, // POP_TOP
    0x42,0x33, // JUMP -13
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
#if MICROPY_PERSISTENT_CODE_SAVE
static const mp_raw_code_truncated_t proto_fun_tdui_menu__lt_genexpr_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 1,
    .fun_data = fun_data_tdui_menu__lt_genexpr_gt_,
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 26,
    .n_children = 0,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 8,
        .n_exc_stack = 0,
        .scope_flags = 1,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 159,
        .line_info = fun_data_tdui_menu__lt_genexpr_gt_ + 7,
        .line_info_top = fun_data_tdui_menu__lt_genexpr_gt_ + 7,
        .opcodes = fun_data_tdui_menu__lt_genexpr_gt_ + 7,
    },
    #endif
    #endif
};
#else
#define proto_fun_tdui_menu__lt_genexpr_gt_ fun_data_tdui_menu__lt_genexpr_gt_[0]
#endif

static const mp_raw_code_t *const children_tdui_menu[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_menu__lt_genexpr_gt_,
};

static const mp_raw_code_truncated_t proto_fun_tdui_menu = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui_menu,
    .children = (void *)&children_tdui_menu,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 30,
    .n_children = 1,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 1,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 86,
        .line_info = fun_data_tdui_menu + 5,
        .line_info_top = fun_data_tdui_menu + 5,
        .opcodes = fun_data_tdui_menu + 5,
    },
    #endif
    #endif
};

static const mp_raw_code_t *const children_tdui__lt_module_gt_[] = {
    (const mp_raw_code_t *)&proto_fun_tdui_enable_rgb_pictures,
    (const mp_raw_code_t *)&proto_fun_tdui_graphics_info,
    (const mp_raw_code_t *)&proto_fun_tdui_scanout_info,
    (const mp_raw_code_t *)&proto_fun_tdui_scanout_timing,
    (const mp_raw_code_t *)&proto_fun_tdui_render_cache,
    (const mp_raw_code_t *)&proto_fun_tdui_render_cache_info,
    (const mp_raw_code_t *)&proto_fun_tdui__new_id,
    (const mp_raw_code_t *)&proto_fun_tdui__send,
    (const mp_raw_code_t *)&proto_fun_tdui__key_name,
    (const mp_raw_code_t *)&proto_fun_tdui_Timer,
    (const mp_raw_code_t *)&proto_fun_tdui_every,
    (const mp_raw_code_t *)&proto_fun_tdui_after,
    (const mp_raw_code_t *)&proto_fun_tdui_Window,
    (const mp_raw_code_t *)&proto_fun_tdui__Widget,
    (const mp_raw_code_t *)&proto_fun_tdui_Label,
    (const mp_raw_code_t *)&proto_fun_tdui_Button,
    (const mp_raw_code_t *)&proto_fun_tdui_Checkbox,
    (const mp_raw_code_t *)&proto_fun_tdui_Textbox,
    (const mp_raw_code_t *)&proto_fun_tdui_Progress,
    (const mp_raw_code_t *)&proto_fun_tdui_List,
    (const mp_raw_code_t *)&proto_fun_tdui_rgb,
    (const mp_raw_code_t *)&proto_fun_tdui__part,
    (const mp_raw_code_t *)&proto_fun_tdui__Batch,
    (const mp_raw_code_t *)&proto_fun_tdui_batch,
    (const mp_raw_code_t *)&proto_fun_tdui__Pixels,
    (const mp_raw_code_t *)&proto_fun_tdui_text_size,
    (const mp_raw_code_t *)&proto_fun_tdui_CanvasButton,
    (const mp_raw_code_t *)&proto_fun_tdui_Image,
    (const mp_raw_code_t *)&proto_fun_tdui_Font,
    (const mp_raw_code_t *)&proto_fun_tdui_Canvas,
    (const mp_raw_code_t *)&proto_fun_tdui_big_size,
    (const mp_raw_code_t *)&proto_fun_tdui__run_timers,
    (const mp_raw_code_t *)&proto_fun_tdui__dispatch,
    (const mp_raw_code_t *)&proto_fun_tdui_poll,
    (const mp_raw_code_t *)&proto_fun_tdui_run,
    (const mp_raw_code_t *)&proto_fun_tdui_run_async,
    (const mp_raw_code_t *)&proto_fun_tdui_quit,
    (const mp_raw_code_t *)&proto_fun_tdui__request,
    (const mp_raw_code_t *)&proto_fun_tdui_msgbox,
    (const mp_raw_code_t *)&proto_fun_tdui_confirm,
    (const mp_raw_code_t *)&proto_fun_tdui_inputbox,
    (const mp_raw_code_t *)&proto_fun_tdui_menu,
};

static const mp_raw_code_truncated_t proto_fun_tdui__lt_module_gt_ = {
    .proto_fun_indicator[0] = MP_PROTO_FUN_INDICATOR_RAW_CODE_0,
    .proto_fun_indicator[1] = MP_PROTO_FUN_INDICATOR_RAW_CODE_1,
    .kind = MP_CODE_BYTECODE,
    .is_generator = 0,
    .fun_data = fun_data_tdui__lt_module_gt_,
    .children = (void *)&children_tdui__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .fun_data_len = 475,
    .n_children = 42,
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    .line_of_definition = 0,
    .prelude = {
        .n_state = 9,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_tdui__lt_module_gt_ + 3,
        .line_info_top = fun_data_tdui__lt_module_gt_ + 3,
        .opcodes = fun_data_tdui__lt_module_gt_ + 3,
    },
    #endif
    #endif
};

static const qstr_short_t const_qstr_table_data_tdui[305] = {
    MP_QSTR_tdui_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR__tdui,
    MP_QSTR_os,
    MP_QSTR_time,
    MP_QSTR_const,
    MP_QSTR_micropython,
    MP_QSTR_SUPPORTS_RGB_PICTURES,
    MP_QSTR_MAX_CELLS,
    MP_QSTR_MAX_PIXELS,
    MP_QSTR_MAX_PICTURES,
    MP_QSTR_MAX_VISIBLE_PICTURES,
    MP_QSTR_Timer,
    MP_QSTR_Window,
    MP_QSTR__Widget,
    MP_QSTR_Label,
    MP_QSTR_Button,
    MP_QSTR_Checkbox,
    MP_QSTR_Textbox,
    MP_QSTR_Progress,
    MP_QSTR_List,
    MP_QSTR__Batch,
    MP_QSTR__Pixels,
    MP_QSTR_CanvasButton,
    MP_QSTR_Image,
    MP_QSTR_Font,
    MP_QSTR_Canvas,
    MP_QSTR_OK,
    MP_QSTR_,
    MP_QSTR_enable_rgb_pictures,
    MP_QSTR_graphics_info,
    MP_QSTR_scanout_info,
    MP_QSTR_scanout_timing,
    MP_QSTR_render_cache,
    MP_QSTR_render_cache_info,
    MP_QSTR__new_id,
    MP_QSTR__send,
    MP_QSTR_send,
    MP_QSTR__key_name,
    MP_QSTR_shift_plus_,
    MP_QSTR_alt_plus_,
    MP_QSTR_ctrl_plus_,
    MP_QSTR_every,
    MP_QSTR_after,
    MP_QSTR_setter,
    MP_QSTR_rgb,
    MP_QSTR__part,
    MP_QSTR_batch,
    MP_QSTR_text_size,
    MP_QSTR_split,
    MP_QSTR__0x0a_,
    MP_QSTR__space_,
    MP_QSTR_left,
    MP_QSTR_top,
    MP_QSTR_big_size,
    MP_QSTR__run_timers,
    MP_QSTR_ticks_ms,
    MP_QSTR_ticks_diff,
    MP_QSTR_due,
    MP_QSTR_repeat,
    MP_QSTR_ticks_add,
    MP_QSTR_ms,
    MP_QSTR_remove,
    MP_QSTR_fn,
    MP_QSTR__dispatch,
    MP_QSTR_tdui_colon__space_,
    MP_QSTR_error,
    MP_QSTR_get,
    MP_QSTR__event,
    MP_QSTR_poll,
    MP_QSTR_recv,
    MP_QSTR_run,
    MP_QSTR_run_async,
    MP_QSTR_asyncio,
    MP_QSTR_sleep_ms,
    MP_QSTR_quit,
    MP_QSTR_values,
    MP_QSTR_close,
    MP_QSTR__request,
    MP_QSTR_open,
    MP_QSTR_text,
    MP_QSTR_msgbox,
    MP_QSTR__percent_s_0x00__percent_s_0x00__percent_s,
    MP_QSTR_confirm,
    MP_QSTR_Yes_pipe_No,
    MP_QSTR_inputbox,
    MP_QSTR_menu,
    MP_QSTR__0x00_,
    MP_QSTR_join,
    MP_QSTR___init__,
    MP_QSTR_win,
    MP_QSTR_append,
    MP_QSTR_cancel,
    MP_QSTR_id,
    MP_QSTR__title,
    MP_QSTR_width,
    MP_QSTR_height,
    MP_QSTR_is_open,
    MP_QSTR_on_close,
    MP_QSTR_on_key,
    MP_QSTR_on_resize,
    MP_QSTR__kids,
    MP_QSTR_title,
    MP_QSTR_focus,
    MP_QSTR_resize,
    MP_QSTR_move,
    MP_QSTR_restore,
    MP_QSTR_maximize,
    MP_QSTR_minimize,
    MP_QSTR_fullscreen,
    MP_QSTR__gone,
    MP_QSTR_pop,
    MP_QSTR__n,
    MP_QSTR__cells,
    MP_QSTR_label,
    MP_QSTR_button,
    MP_QSTR_checkbox,
    MP_QSTR_textbox,
    MP_QSTR_progress,
    MP_QSTR_list,
    MP_QSTR_canvas,
    MP_QSTR__text,
    MP_QSTR__visible,
    MP_QSTR__req,
    MP_QSTR_visible,
    MP_QSTR_color,
    MP_QSTR_on_click,
    MP_QSTR__checked,
    MP_QSTR_on_change,
    MP_QSTR_checked,
    MP_QSTR_on_enter,
    MP_QSTR__value,
    MP_QSTR_value,
    MP_QSTR_list_space_size,
    MP_QSTR_on_select,
    MP_QSTR_on_activate,
    MP_QSTR__selected,
    MP_QSTR__items,
    MP_QSTR_items,
    MP_QSTR__add,
    MP_QSTR_replace,
    MP_QSTR_selected,
    MP_QSTR___enter__,
    MP_QSTR___exit__,
    MP_QSTR__gradient_data,
    MP_QSTR_rgb1,
    MP_QSTR_rgb2,
    MP_QSTR_across,
    MP_QSTR_bar,
    MP_QSTR__alive,
    MP_QSTR__bar_colors,
    MP_QSTR_rect,
    MP_QSTR_line,
    MP_QSTR_circle,
    MP_QSTR_gradient,
    MP_QSTR_image,
    MP_QSTR_nine,
    MP_QSTR_draw_text,
    MP_QSTR_pixels,
    MP_QSTR__lt_genexpr_gt_,
    MP_QSTR_cv,
    MP_QSTR_drawing,
    MP_QSTR_page,
    MP_QSTR_x,
    MP_QSTR_y,
    MP_QSTR_w,
    MP_QSTR_h,
    MP_QSTR_on_press,
    MP_QSTR_on_release,
    MP_QSTR_on_cancel,
    MP_QSTR_down,
    MP_QSTR_rim,
    MP_QSTR_fg,
    MP_QSTR_radius,
    MP_QSTR_scale,
    MP_QSTR_font,
    MP_QSTR_sheet,
    MP_QSTR_src,
    MP_QSTR_src_down,
    MP_QSTR_border,
    MP_QSTR_enabled,
    MP_QSTR__buttons,
    MP_QSTR_draw,
    MP_QSTR__held,
    MP_QSTR_draw_to,
    MP_QSTR__draw,
    MP_QSTR_picture,
    MP_QSTR_outline,
    MP_QSTR_font_text,
    MP_QSTR_align,
    MP_QSTR_center,
    MP_QSTR_valign,
    MP_QSTR_middle,
    MP_QSTR_put,
    MP_QSTR__hit,
    MP_QSTR_showing,
    MP_QSTR_image_load,
    MP_QSTR_load,
    MP_QSTR_startswith,
    MP_QSTR__slash_,
    MP_QSTR_getcwd,
    MP_QSTR_rstrip,
    MP_QSTR_free,
    MP_QSTR_font_load,
    MP_QSTR_ascent,
    MP_QSTR_glyphs,
    MP_QSTR_size,
    MP_QSTR_bpp,
    MP_QSTR_pages,
    MP_QSTR_on_mouse,
    MP_QSTR_fill,
    MP_QSTR_clear,
    MP_QSTR_big,
    MP_QSTR__page,
    MP_QSTR__req2,
    MP_QSTR_show,
    MP_QSTR_copy_page,
    MP_QSTR_encode,
    MP_QSTR_right,
    MP_QSTR_bottom,
    MP_QSTR_pwidth,
    MP_QSTR_pheight,
    MP_QSTR__dot__colon_,
    MP_QSTR__lt_listcomp_gt_,
    MP_QSTR_SUPPORTS_GRADIENT_FILLS,
    MP_QSTR_bool,
    MP_QSTR_getattr,
    MP_QSTR_MAX_WINDOWS,
    MP_QSTR_MAX_WIDGETS,
    MP_QSTR_MAX_PAGES,
    MP_QSTR_CLEAR,
    MP_QSTR__KEYS,
    MP_QSTR__objs,
    MP_QSTR__results,
    MP_QSTR__timers,
    MP_QSTR__next,
    MP_QSTR__windows,
    MP_QSTR__widgets,
    MP_QSTR__batch,
    MP_QSTR_kb,
    MP_QSTR_warm,
    MP_QSTR_op,
    MP_QSTR_flags,
    MP_QSTR_a,
    MP_QSTR_b,
    MP_QSTR_c,
    MP_QSTR_d,
    MP_QSTR_e,
    MP_QSTR_f,
    MP_QSTR_key,
    MP_QSTR_mods,
    MP_QSTR_len,
    MP_QSTR_chr,
    MP_QSTR___name__,
    MP_QSTR___module__,
    MP_QSTR___qualname__,
    MP_QSTR_property,
    MP_QSTR_r,
    MP_QSTR_g,
    MP_QSTR_bytes,
    MP_QSTR_small,
    MP_QSTR_str,
    MP_QSTR_max,
    MP_QSTR_staticmethod,
    MP_QSTR_sum,
    MP_QSTR_min,
    MP_QSTR_ev,
    MP_QSTR_OSError,
    MP_QSTR_isinstance,
    MP_QSTR_buttons,
    MP_QSTR_prompt,
    MP_QSTR_secret,
    MP_QSTR_self,
    MP_QSTR_resizable,
    MP_QSTR_ValueError,
    MP_QSTR_t,
    MP_QSTR_kind,
    MP_QSTR_v,
    MP_QSTR_bg,
    MP_QSTR__star_,
    MP_QSTR_super,
    MP_QSTR_int,
    MP_QSTR_item,
    MP_QSTR_i,
    MP_QSTR_traceback,
    MP_QSTR_tuple,
    MP_QSTR_any,
    MP_QSTR_maximum,
    MP_QSTR_track,
    MP_QSTR_edge,
    MP_QSTR_x0,
    MP_QSTR_y0,
    MP_QSTR_x1,
    MP_QSTR_y1,
    MP_QSTR_img,
    MP_QSTR_data,
    MP_QSTR_dither,
    MP_QSTR_pressed,
    MP_QSTR__file,
    MP_QSTR_Exception,
    MP_QSTR_path,
    MP_QSTR_ch,
    MP_QSTR_n,
    MP_QSTR_dst,
    MP_QSTR_wrap,
};

// constants
static const mp_rom_obj_tuple_t const_obj_tdui_0 = {{&mp_type_tuple}, 26, {
    MP_ROM_QSTR(MP_QSTR_enter),
    MP_ROM_QSTR(MP_QSTR_tab),
    MP_ROM_QSTR(MP_QSTR_backspace),
    MP_ROM_QSTR(MP_QSTR_esc),
    MP_ROM_QSTR(MP_QSTR_up),
    MP_ROM_QSTR(MP_QSTR_down),
    MP_ROM_QSTR(MP_QSTR_right),
    MP_ROM_QSTR(MP_QSTR_left),
    MP_ROM_QSTR(MP_QSTR_home),
    MP_ROM_QSTR(MP_QSTR_end),
    MP_ROM_QSTR(MP_QSTR_insert),
    MP_ROM_QSTR(MP_QSTR_delete),
    MP_ROM_QSTR(MP_QSTR_pgup),
    MP_ROM_QSTR(MP_QSTR_pgdn),
    MP_ROM_QSTR(MP_QSTR_f1),
    MP_ROM_QSTR(MP_QSTR_f2),
    MP_ROM_QSTR(MP_QSTR_f3),
    MP_ROM_QSTR(MP_QSTR_f4),
    MP_ROM_QSTR(MP_QSTR_f5),
    MP_ROM_QSTR(MP_QSTR_f6),
    MP_ROM_QSTR(MP_QSTR_f7),
    MP_ROM_QSTR(MP_QSTR_f8),
    MP_ROM_QSTR(MP_QSTR_f9),
    MP_ROM_QSTR(MP_QSTR_f10),
    MP_ROM_QSTR(MP_QSTR_f11),
    MP_ROM_QSTR(MP_QSTR_f12),
}};
static const mp_rom_obj_tuple_t const_obj_tdui_1 = {{&mp_type_tuple}, 1, {
    MP_ROM_INT(0),
}};
static const mp_rom_obj_tuple_t const_obj_tdui_2 = {{&mp_type_tuple}, 2, {
    MP_ROM_INT(0),
    MP_ROM_INT(0),
}};
static const mp_obj_str_t const_obj_tdui_5 = {{&mp_type_str}, 14414, 26, (const byte*)"\x74\x64\x75\x69\x3a\x20\x74\x68\x65\x20\x77\x69\x6e\x64\x6f\x77\x20\x69\x73\x20\x63\x6c\x6f\x73\x65\x64"};
static const mp_rom_obj_tuple_t const_obj_tdui_10 = {{&mp_type_tuple}, 4, {
    MP_ROM_INT(0),
    MP_ROM_INT(0),
    MP_ROM_INT(0),
    MP_ROM_INT(0),
}};
static const mp_obj_str_t const_obj_tdui_13 = {{&mp_type_str}, 48006, 45, (const byte*)"\x74\x64\x75\x69\x3a\x20\x63\x61\x6e\x76\x61\x73\x65\x73\x20\x61\x72\x65\x20\x6c\x69\x6d\x69\x74\x65\x64\x20\x74\x6f\x20\x25\x64\x20\x63\x65\x6c\x6c\x73\x20\x69\x6e\x20\x61\x6c\x6c"};
static const mp_obj_str_t const_obj_tdui_15 = {{&mp_type_str}, 63608, 29, (const byte*)"\x66\x6f\x6e\x74\x5f\x74\x65\x78\x74\x20\x6e\x65\x65\x64\x73\x20\x61\x20\x6c\x6f\x61\x64\x65\x64\x20\x46\x6f\x6e\x74"};
static const mp_obj_str_t const_obj_tdui_16 = {{&mp_type_str}, 37431, 29, (const byte*)"\x66\x6f\x6e\x74\x5f\x74\x65\x78\x74\x20\x62\x6f\x75\x6e\x64\x73\x20\x6f\x72\x20\x61\x6c\x69\x67\x6e\x6d\x65\x6e\x74"};
static const mp_rom_obj_tuple_t const_obj_tdui_17 = {{&mp_type_tuple}, 2, {
    MP_ROM_INT(0),
    MP_ROM_INT(1),
}};

// constant table
static const mp_rom_obj_t const_obj_table_data_tdui[18] = {
    MP_ROM_PTR(&const_obj_tdui_0),
    MP_ROM_PTR(&const_obj_tdui_1),
    MP_ROM_PTR(&const_obj_tdui_2),
    MP_ROM_QSTR(MP_QSTR_tdui_colon__space_at_space_most_space__percent_d_space_windows),
    MP_ROM_QSTR(MP_QSTR_window_space_size),
    MP_ROM_PTR(&const_obj_tdui_5),
    MP_ROM_QSTR(MP_QSTR_tdui_colon__space_at_space_most_space__percent_d_space_widgets),
    MP_ROM_PTR(&mp_const_empty_bytes_obj),
    MP_ROM_QSTR(MP_QSTR_gradient_space_RGB_space_range),
    MP_ROM_QSTR(MP_QSTR_bar_space_range_space_or_space_colour),
    MP_ROM_PTR(&const_obj_tdui_10),
    MP_ROM_QSTR(MP_QSTR_canvas_space_size),
    MP_ROM_QSTR(MP_QSTR_canvas_space_pages_colon__space_1_space_to_space__percent_d),
    MP_ROM_PTR(&const_obj_tdui_13),
    MP_ROM_QSTR(MP_QSTR_canvas_space_page_colon__space_0_space_to_space__percent_d),
    MP_ROM_PTR(&const_obj_tdui_15),
    MP_ROM_PTR(&const_obj_tdui_16),
    MP_ROM_PTR(&const_obj_tdui_17),
};

static const mp_frozen_module_t frozen_module_tdui = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_tdui,
        .obj_table = (mp_obj_t *)&const_obj_table_data_tdui,
    },
    .proto_fun = &proto_fun_tdui__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// collection of all frozen modules

const char mp_frozen_names[] = {
    #ifdef MP_FROZEN_STR_NAMES
    MP_FROZEN_STR_NAMES
    #endif
    "asyncio/__init__.py\0"
    "asyncio/core.py\0"
    "asyncio/event.py\0"
    "asyncio/funcs.py\0"
    "asyncio/lock.py\0"
    "asyncio/stream.py\0"
    "neopixel.py\0"
    "tdui.py\0"
    "\0"
};

const mp_frozen_module_t *const mp_frozen_mpy_content[] = {
    &frozen_module_asyncio___init__,
    &frozen_module_asyncio_core,
    &frozen_module_asyncio_event,
    &frozen_module_asyncio_funcs,
    &frozen_module_asyncio_lock,
    &frozen_module_asyncio_stream,
    &frozen_module_neopixel,
    &frozen_module_tdui,
};

#ifdef MICROPY_FROZEN_LIST_ITEM
MICROPY_FROZEN_LIST_ITEM("asyncio", "asyncio/__init__.py")
MICROPY_FROZEN_LIST_ITEM("asyncio/core", "asyncio/core.py")
MICROPY_FROZEN_LIST_ITEM("asyncio/event", "asyncio/event.py")
MICROPY_FROZEN_LIST_ITEM("asyncio/funcs", "asyncio/funcs.py")
MICROPY_FROZEN_LIST_ITEM("asyncio/lock", "asyncio/lock.py")
MICROPY_FROZEN_LIST_ITEM("asyncio/stream", "asyncio/stream.py")
MICROPY_FROZEN_LIST_ITEM("neopixel", "neopixel.py")
MICROPY_FROZEN_LIST_ITEM("tdui", "tdui.py")
#endif

/*
byte sizes:
qstr content: 369 unique, 4053 bytes
bc content: 13201
const str content: 160
const int content: 0
const obj content: 80
const table qstr content: 0 entries, 0 bytes
const table ptr content: 34 entries, 136 bytes
raw code content: 249 * 4 = 3984
mp_frozen_mpy_names_content: 125
mp_frozen_mpy_content_size: 32
total: 21771
*/
