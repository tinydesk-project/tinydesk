/*
 * displays.c - Settings -> Displays: where the two displays are, which
 * terminal each one is (td_display_ops_t, from the port), and each one's
 * resolution and orientation, like a PC's display settings.
 *
 * The picture at the top shows display 1 and 2 as they are arranged
 * (td_screens_layout_t): drag display 2 to any side of display 1 and
 * along it, or select it and use the arrow keys (Shift+arrows move it
 * along the edge). Clicking a display selects it for the settings below.
 * Changes apply at once and are kept by the port (save_layout).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"
#include "tinydesk/td.h"

#define AREA_Y 1   /* the picture's first client row */
#define AREA_H 9

static td_window_t *s_win;
static td_widget_t *s_mode, *s_source, *s_source_label, *s_status, *s_res, *s_rot, *s_rot_note, *s_main, *s_msg;
static td_widget_t *s_pick[2];
static int s_sel;   /* the selected display */

/* Dragging display 2 in the picture. */
static struct
{
    bool on;
    int grab_x, grab_y;    /* where in its box it was taken, in picture cells */
    int x, y;              /* its box's corner now, in picture cells */
} s_drag;

/* Identify: a number on each display for a few seconds. */
static td_window_t *s_ident[2];
static uint32_t s_ident_until;

static const td_display_ops_t *ops(void)
{
    return td_sysinfo()->displays;
}

static bool allowed(void)
{
    if (td_session_has_right(TD_RIGHT_SYSTEM))
        return true;
    td_widget_set_text(s_msg, "Changing displays needs the System settings right.");
    return false;
}

static void apply(const td_screens_layout_t *l)
{
    td_screens_set_layout(l);
    if (ops() && ops()->save_layout)
        ops()->save_layout(l);
}

/* ------------------------------------------------------------ geometry */

/* Each display's size for the picture: the area it shows, else what its
 * terminal reported or was given, else a usual terminal's. */
static void display_size(int i, int *cols, int *rows)
{
    td_screens_layout_t l;
    td_screens_layout(&l);
    td_display_info_t info;
    *cols = 80;
    *rows = 24;
    if (td_screens_display(i, &info) && info.term_cols > 0)
    {
        *cols = info.term_cols;
        *rows = info.term_rows;
    }
    if (l.cols[i] > 0 && l.cols[i] < *cols)
        *cols = l.cols[i];
    if (l.rows[i] > 0 && l.rows[i] < *rows)
        *rows = l.rows[i];
}

/* The two displays' rectangles in cells, display 1's corner at 0, 0. */
static void arrangement(const td_screens_layout_t *l, td_rect_t r[2])
{
    int c1, r1, c2, r2;
    display_size(0, &c1, &r1);
    display_size(1, &c2, &r2);
    bool beside = l->place == TD_PLACE_RIGHT || l->place == TD_PLACE_LEFT;
    int off = l->offset;
    int lo = beside ? -(r2 - 1) : -(c2 - 1), hi = beside ? r1 - 1 : c1 - 1;
    off = off < lo ? lo : off > hi ? hi
                                   : off;
    r[0] = td_rect(0, 0, c1, r1);
    switch (l->place)
    {
    case TD_PLACE_LEFT:
        r[1] = td_rect(-c2, off, c2, r2);
        break;
    case TD_PLACE_BELOW:
        r[1] = td_rect(off, r1, c2, r2);
        break;
    case TD_PLACE_ABOVE:
        r[1] = td_rect(off, -r2, c2, r2);
        break;
    default:
        r[1] = td_rect(c1, off, c2, r2);
        break;
    }
}

/* The picture: the arrangement scaled into the area, centred. Cells are
 * about twice as tall as wide, so one cell of the picture stands for the
 * same number of cells both ways. */
typedef struct
{
    int x0, y0, minx, miny;
    int num, den;          /* picture cells = display cells * num / den */
} view_t;

static view_t view_of(const td_rect_t r[2], int aw, int ah)
{
    int minx = r[0].x < r[1].x ? r[0].x : r[1].x, miny = r[0].y < r[1].y ? r[0].y : r[1].y;
    int maxx = r[0].x + r[0].w > r[1].x + r[1].w ? r[0].x + r[0].w : r[1].x + r[1].w;
    int maxy = r[0].y + r[0].h > r[1].y + r[1].h ? r[0].y + r[0].h : r[1].y + r[1].h;
    int w = maxx - minx, h = maxy - miny;
    view_t v = {0, 0, minx, miny, aw, w};
    if (ah * w < aw * h)   /* the height decides */
    {
        v.num = ah;
        v.den = h;
    }
    v.x0 = (aw - w * v.num / v.den) / 2;
    v.y0 = (ah - h * v.num / v.den) / 2;
    return v;
}

static td_rect_t to_view(const view_t *v, td_rect_t r)
{
    int x = v->x0 + (r.x - v->minx) * v->num / v->den, y = v->y0 + (r.y - v->miny) * v->num / v->den;
    int w = r.w * v->num / v->den, h = r.h * v->num / v->den;
    return td_rect(x, y, w < 4 ? 4 : w, h < 3 ? 3 : h);
}

/* ------------------------------------------------------------- picture */

static void draw_box(td_rect_t b, const char *name, bool selected, bool main, bool attached)
{
    const td_theme_t *t = td_theme();
    uint8_t fg = selected ? t->select_fg : attached ? t->win_fg
                                                    : t->dim;
    uint8_t bg = selected ? t->select_bg : t->input_bg;
    td_fill(b, ' ', fg, bg);
    td_box(b, selected ? TD_BOX_DOUBLE : TD_BOX_SINGLE, fg, bg);
    char label[24];
    snprintf(label, sizeof(label), "%s%s", name, main ? " main" : "");
    int len = (int)strlen(label);
    if (len > b.w - 2)
        len = b.w - 2;
    td_textn(b.x + (b.w - len) / 2, b.y + b.h / 2, label, len, fg, bg, TD_BOLD);
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)h;
    const td_theme_t *t = td_theme();
    td_text(1, 0, "Drag display 2 beside display 1, or select it and use the arrow keys.", t->dim, t->win_bg, 0);
    int aw = w - 2;
    td_fill(td_rect(1, AREA_Y, aw, AREA_H), ' ', t->win_fg, t->desktop_bg);
    td_screens_layout_t l;
    td_screens_layout(&l);
    td_rect_t r[2];
    arrangement(&l, r);
    view_t v = view_of(r, aw - 2, AREA_H - 2);
    td_rect_t b1 = to_view(&v, r[0]), b2 = to_view(&v, r[1]);
    b1.x += 2, b1.y += AREA_Y + 1, b2.x += 2, b2.y += AREA_Y + 1;
    if (s_drag.on)
    {
        b2.x = s_drag.x;
        b2.y = s_drag.y;
    }
    bool ext = l.mode == TD_SCREENS_EXTEND;
    if (!ext)
        b2 = b1;   /* duplicated: one picture */
    draw_box(b1, ext ? "1" : "1 | 2", ext && s_sel == 0, ext && l.main == 0, true);
    if (ext)
        draw_box(b2, "2", s_sel == 1, l.main == 1, td_screen2_attached());
    td_text(1, 21, "Move a window to the other display: Alt+Shift+Left / Right,", t->dim, t->win_bg, 0);
    td_text(1, 22, "Win+Shift+Left / Right, or Other display in its title bar menu.", t->dim, t->win_bg, 0);
}

/* Where display 2 goes when its box is dropped with its corner at x, y
 * (picture cells): beside the side of display 1 it is nearest to, at the
 * offset its position gives (snapped to the corner when close). */
static void drop(int x, int y)
{
    td_screens_layout_t l;
    td_screens_layout(&l);
    td_rect_t r[2];
    arrangement(&l, r);
    int aw = td_win_client(s_win).w - 2;
    view_t v = view_of(r, aw - 2, AREA_H - 2);
    td_rect_t b1 = to_view(&v, r[0]);
    b1.x += 2, b1.y += AREA_Y + 1;
    /* Back to display cells. */
    int dx = (x - b1.x) * v.den / v.num, dy = (y - b1.y) * v.den / v.num;
    int c1 = r[0].w, r1 = r[0].h, c2 = r[1].w, r2 = r[1].h;
    int cx = dx + c2 / 2 - c1 / 2, cy = dy + r2 / 2 - r1 / 2;   /* centre to centre */
    /* Compare in screen proportions (a cell is about twice as tall). */
    bool beside = abs(cx) * (r1 + r2) >= abs(cy) * (c1 + c2);
    if (beside)
    {
        l.place = cx >= 0 ? TD_PLACE_RIGHT : TD_PLACE_LEFT;
        l.offset = (int16_t)(abs(dy) * 10 < r1 ? 0 : dy);
    }
    else
    {
        l.place = cy >= 0 ? TD_PLACE_BELOW : TD_PLACE_ABOVE;
        l.offset = (int16_t)(abs(dx) * 10 < c1 ? 0 : dx);
    }
    apply(&l);
}

static void update(void);

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    td_screens_layout_t l;
    td_screens_layout(&l);
    bool ext = l.mode == TD_SCREENS_EXTEND;
    if (ev->type == TD_EV_MOUSE && ev->button == TD_BUTTON_LEFT)
    {
        td_rect_t r[2];
        arrangement(&l, r);
        int aw = td_win_client(win).w - 2;
        view_t v = view_of(r, aw - 2, AREA_H - 2);
        td_rect_t b1 = to_view(&v, r[0]), b2 = to_view(&v, r[1]);
        b1.x += 2, b1.y += AREA_Y + 1, b2.x += 2, b2.y += AREA_Y + 1;
        if (ev->action == TD_MOUSE_PRESS)
        {
            if (ext && td_rect_contains(b2, ev->x, ev->y))
            {
                s_sel = 1;
                if (allowed())
                {
                    s_drag.on = true;
                    s_drag.grab_x = ev->x - b2.x;
                    s_drag.grab_y = ev->y - b2.y;
                    s_drag.x = b2.x;
                    s_drag.y = b2.y;
                }
                update();
                return true;
            }
            if (td_rect_contains(b1, ev->x, ev->y))
            {
                s_sel = 0;
                update();
                return true;
            }
        }
        else if (s_drag.on && (ev->action == TD_MOUSE_DRAG || ev->action == TD_MOUSE_MOVE))
        {
            s_drag.x = ev->x - s_drag.grab_x;
            s_drag.y = ev->y - s_drag.grab_y;
            td_win_invalidate(win);
            return true;
        }
        else if (s_drag.on && ev->action == TD_MOUSE_RELEASE)
        {
            s_drag.on = false;
            drop(ev->x - s_drag.grab_x, ev->y - s_drag.grab_y);
            update();
            return true;
        }
    }
    if (ev->type == TD_EV_KEY && s_sel == 1 && ext &&
        (ev->key == TD_KEY_LEFT || ev->key == TD_KEY_RIGHT || ev->key == TD_KEY_UP || ev->key == TD_KEY_DOWN) &&
        !(ev->mods & (TD_MOD_ALT | TD_MOD_CTRL | TD_MOD_SUPER)))
    {
        if (!allowed())
            return true;
        bool beside = l.place == TD_PLACE_RIGHT || l.place == TD_PLACE_LEFT;
        bool along = beside ? (ev->key == TD_KEY_UP || ev->key == TD_KEY_DOWN) : (ev->key == TD_KEY_LEFT || ev->key == TD_KEY_RIGHT);
        if ((ev->mods & TD_MOD_SHIFT) && along)
            l.offset += ev->key == TD_KEY_DOWN || ev->key == TD_KEY_RIGHT ? 1 : -1;   /* along the edge */
        else if (!(ev->mods & TD_MOD_SHIFT))
        {
            l.place = ev->key == TD_KEY_RIGHT  ? TD_PLACE_RIGHT
                      : ev->key == TD_KEY_LEFT ? TD_PLACE_LEFT
                      : ev->key == TD_KEY_UP   ? TD_PLACE_ABOVE
                                               : TD_PLACE_BELOW;
            l.offset = 0;
        }
        apply(&l);
        update();
        return true;
    }
    return false;
}

/* ------------------------------------------------------------ settings */

static const int s_presets[][2] = {{80, 24}, {80, 25}, {100, 30}, {120, 40}, {132, 43}, {160, 50}, {200, 60}, {240, 67}};
#define PRESETS ((int)(sizeof(s_presets) / sizeof(s_presets[0])))

static void update(void)
{
    if (!td_win_is_open(s_win))
        return;
    td_screens_layout_t l;
    td_screens_layout(&l);
    bool ext = l.mode == TD_SCREENS_EXTEND;
    td_widget_set_text(s_mode, ext ? "Extend these displays" : "Duplicate these displays");
    for (int i = 0; i < 2; i++)
        td_widget_printf(s_pick[i], s_sel == i ? "* Display %d" : "  Display %d", i + 1);

    const td_display_ops_t *o = ops();
    if (o && o->source)
    {
        int src = o->source(s_sel);
        td_widget_set_text(s_source, src == TD_DISPLAY_OFF      ? "Off"
                                     : src == TD_DISPLAY_TELNET ? "Telnet"
                                     : src == TD_DISPLAY_SERIAL ? "Serial 2"
                                                                : "This board");
        td_widget_set_text(s_source_label, o->source_label ? o->source_label(src) : "");
        char st[64] = "";
        if (o->status)
            o->status(s_sel, st, sizeof(st));
        td_widget_printf(s_status, "Now: %s", st);
    }
    else
    {
        td_widget_set_text(s_source, s_sel == 0 ? "This terminal" : "Attached");
        td_widget_set_text(s_source_label, s_sel == 0 ? "the program's own terminal" : "td_screen2_attach()");
        td_widget_printf(s_status, "Now: %s", s_sel == 0 || td_screen2_attached() ? "connected" : "off");
    }

    td_display_info_t info;
    bool known = td_screens_display(s_sel, &info) && info.term_cols > 0;
    if (l.cols[s_sel] > 0)
        td_widget_printf(s_res, "%d x %d", l.cols[s_sel], l.rows[s_sel]);
    else if (known)
        td_widget_printf(s_res, "Auto (%d x %d)", info.term_cols, info.term_rows);
    else
        td_widget_set_text(s_res, "Auto (its terminal's size)");

    int rot = o && o->rotation ? o->rotation(s_sel) : -1;
    td_widget_set_visible(s_rot, rot >= 0);
    if (rot >= 0)
    {
        td_widget_printf(s_rot, "%-19s %3d\xC2\xB0", rot == 0 ? "Landscape" : rot == 90 ? "Portrait"
                                                                          : rot == 180  ? "Landscape (flipped)"
                                                                                        : "Portrait (flipped)",
                         rot);
        td_widget_set_text(s_rot_note, "a change applies at the next start");
    }
    else
        td_widget_set_text(s_rot_note, "set by its terminal: turn or resize that window");
    td_checkbox_set(s_main, ext && l.main == s_sel);
    td_widget_set_visible(s_main, ext);
    td_win_invalidate(s_win);
}

static void on_pick(td_widget_t *w, void *user)
{
    (void)w;
    s_sel = (int)(intptr_t)user;
    update();
}

static void on_mode(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (!allowed())
        return;
    td_screens_layout_t l;
    td_screens_layout(&l);
    l.mode = l.mode == TD_SCREENS_EXTEND ? TD_SCREENS_DUPLICATE : TD_SCREENS_EXTEND;
    apply(&l);
    update();
}

/* The next source display s can use. */
static void on_source(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    const td_display_ops_t *o = ops();
    if (!o || !o->source || !o->set_source || !allowed())
        return;
    int src = o->source(s_sel);
    for (int k = 1; k <= TD_DISPLAY_SOURCES; k++)
    {
        int next = (src + k) % TD_DISPLAY_SOURCES;
        if (!o->source_available || o->source_available(s_sel, next))
        {
            o->set_source(s_sel, next);
            break;
        }
    }
    if (o->source(s_sel) == TD_DISPLAY_TELNET)
        td_widget_set_text(s_msg, "Telnet: log in as root on port 23 (Network: Telnet desktop on).");
    else
        td_widget_set_text(s_msg, "");
    update();
}

/* Auto, then the usual sizes up to what its terminal has. */
static void on_res(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (!allowed())
        return;
    td_screens_layout_t l;
    td_screens_layout(&l);
    td_display_info_t info;
    int maxc = 1000, maxr = 1000;
    if (td_screens_display(s_sel, &info) && info.term_cols > 0)
    {
        maxc = info.term_cols;
        maxr = info.term_rows;
    }
    int at = -1;
    for (int i = 0; i < PRESETS; i++)
        if (l.cols[s_sel] == s_presets[i][0] && l.rows[s_sel] == s_presets[i][1])
            at = i;
    int next = -1;
    for (int i = at + 1; i < PRESETS; i++)
        if (s_presets[i][0] < maxc || s_presets[i][1] < maxr)
            if (s_presets[i][0] <= maxc && s_presets[i][1] <= maxr)
            {
                next = i;
                break;
            }
    l.cols[s_sel] = (int16_t)(next < 0 ? 0 : s_presets[next][0]);
    l.rows[s_sel] = (int16_t)(next < 0 ? 0 : s_presets[next][1]);
    apply(&l);
    update();
}

static void on_rot(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    const td_display_ops_t *o = ops();
    if (!o || !o->rotation || !o->set_rotation || !allowed())
        return;
    int rot = o->rotation(s_sel);
    if (rot < 0)
        return;
    if (o->set_rotation(s_sel, (rot + 90) % 360))
        td_widget_set_text(s_msg, "Saved: restart the board to turn the display.");
    update();
}

static void on_main(td_widget_t *w, void *user)
{
    (void)user;
    if (!allowed())
    {
        update();
        return;
    }
    td_screens_layout_t l;
    td_screens_layout(&l);
    if (td_checkbox_get(w))
        l.main = (uint8_t)s_sel;
    else
        l.main = (uint8_t)(s_sel == 0 ? 1 : 0);
    apply(&l);
    update();
}

static void ident_draw(td_window_t *win, int w, int h)
{
    const td_theme_t *t = td_theme();
    td_fill(td_rect(0, 0, w, h), ' ', t->title_fg, t->title_bg);
    char n[4];
    snprintf(n, sizeof(n), "%d", win == s_ident[0] ? 1 : 2);
    td_text(w / 2, h / 2, n, t->title_fg, t->title_bg, TD_BOLD);
}

static void ident_close(void)
{
    for (int i = 0; i < 2; i++)
    {
        if (td_win_is_open(s_ident[i]))
            td_win_close(s_ident[i]);
        s_ident[i] = NULL;
    }
}

static void on_identify(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    ident_close();
    td_rect_t m[2];
    int n = td_wm_monitors(m, 2, NULL);
    for (int i = 0; i < n; i++)
    {
        td_window_desc_t d = {.title = i ? "Display 2" : "Display 1", .rect = td_rect(0, 0, 14, 5), .on_draw = ident_draw};
        s_ident[i] = td_win_create(&d);
        if (s_ident[i])
            td_win_move(s_ident[i], m[i].x + 2, m[i].y + 1);
    }
    td_win_focus(s_win);
    s_ident_until = td_millis() + 3000;
}

static void on_tick(td_window_t *win)
{
    (void)win;
    if (s_ident_until && (int32_t)(td_millis() - s_ident_until) >= 0)
    {
        s_ident_until = 0;
        ident_close();
    }
    update();   /* a terminal connected or left, or reported a new size */
}

static void on_close_btn(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_win_close(s_win);
}

static void on_close(td_window_t *win)
{
    (void)win;
    ident_close();
    s_win = NULL;
    s_drag.on = false;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    td_window_desc_t d = {
        .title = "Displays",
        .rect = td_rect(-1, -1, 74, 27),
        .flags = TD_WIN_DEFAULT,
        .min_w = 70,
        .min_h = 27,
        .on_draw = on_draw,
        .on_event = on_event,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 1000,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;
    s_sel = 0;
    td_button(s_win, 1, 11, "Identify", on_identify, NULL);
    /* Buttons keep the width of their first caption: make room. */
    s_mode = td_button(s_win, 14, 11, "Duplicate these displays", on_mode, NULL);
    s_pick[0] = td_button(s_win, 1, 12, "* Display 1", on_pick, (void *)(intptr_t)0);
    s_pick[1] = td_button(s_win, 17, 12, "* Display 2", on_pick, (void *)(intptr_t)1);
    td_label(s_win, 1, 13, 0, "Shown on");
    s_source = td_button(s_win, 14, 13, "This terminal ", on_source, NULL);
    s_source_label = td_label(s_win, 33, 13, 39, "");
    s_status = td_label(s_win, 14, 14, 58, "");
    td_label(s_win, 1, 15, 0, "Resolution");
    s_res = td_button(s_win, 14, 15, "Auto (its terminal's size)", on_res, NULL);
    td_label(s_win, 1, 16, 0, "Orientation");
    s_rot = td_button(s_win, 14, 16, "Landscape (flipped) 180\xC2\xB0", on_rot, NULL);
    s_rot_note = td_label(s_win, 14, 17, 58, "");
    s_main = td_checkbox(s_win, 1, 19, "Make this my main display (taskbar, new windows)", false, on_main, NULL);
    s_msg = td_label(s_win, 1, -2, 70, "");
    td_button(s_win, 1, -1, "Close", on_close_btn, NULL);
    update();
}

static const td_app_t s_app = {"Displays", launch, "▌▌"};   /* two screens: U+258C U+258C */

void td_displays_register(void)
{
    td_app_register(&s_app);
}
