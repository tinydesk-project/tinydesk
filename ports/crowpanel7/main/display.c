/*
 * display.c - the CrowPanel 7.0's LCD (800x480 RGB565 TFT on the ESP32-S3's
 * RGB interface, EK9716BD3 + EK73002ACGB) as a terminal for the desktop.
 *
 * The desktop writes VT sequences as it does to PuTTY. They go into a
 * td_vterm of 100x30 cells; a task paints the cells that changed (8x16
 * pixels each, font8x16.c) into the frame buffer in PSRAM, which the RGB
 * peripheral scans out on its own. Pins and timings are Elecrow's
 * (CrowPanel 7.0 wiki, LovyanGFX configuration).
 */
#include "display.h"

#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#ifdef TD_DISPLAY_TEST
#include "crowpanel_platform.h"
#else
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_private/esp_gpio_reserve.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tdsh_board.h"
#endif
#include "font8x16.h"
#include "lcd_fonts.h"
#include "tinydesk/td_vterm.h"

#define PIN_BACKLIGHT 2
/* A frame is 928 x 525 clocks. 24 MHz gives about 49 Hz and leaves PSRAM
 * bandwidth for a full-page render without starving the RGB bounce reader.
 * Board configuration key lcd.pclk_mhz changes it (10 to 40). */
#define PCLK_MHZ      24
#define BOUNCE_LINES  20            /* internal-RAM bounce buffers keep the picture steady during flash writes */
#define PAINT_MS      20
#define PAINT_PRIO    4             /* below the desktop's UI task (5) */
#define PAINT_STACK   4096

static const char *TAG = "display";

/* RGB565 data lines: B0-B4, G0-G5, R0-R4. */
static const int s_data_pins[16] = {15, 7, 6, 5, 4, 9, 46, 3, 8, 16, 1, 14, 21, 47, 48, 45};
#define PIN_DE    41
#define PIN_VSYNC 40
#define PIN_HSYNC 39
#define PIN_PCLK  0

static uint16_t *s_fb;              /* off-screen drawing target, in PSRAM */
static uint16_t *s_frames[2];
static unsigned s_draw_frame = 1;   /* RGB starts scanning buffer zero */
static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_frame_done;
static portMUX_TYPE s_swap_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_swap_waiting, s_present_pending;
static atomic_uint s_scanout[8];
static atomic_uint s_paint_timing[4];
static uint32_t s_prepared_at, s_previous_prepared;
static int64_t s_last_prepared_us;

void display_scanout_info(uint32_t out[8])
{for(int i=0;i<8;i++)out[i]=atomic_load(&s_scanout[i]);}
void display_scanout_snapshot(uint32_t out[12])
{display_scanout_info(out);for(int i=0;i<4;i++)out[8+i]=atomic_load(&s_paint_timing[i]);}
static td_vterm_t *s_vt;            /* in PSRAM */
static SemaphoreHandle_t s_lock;    /* s_vt between the UI task and the painter */
static display_reply_fn s_reply;
static uint16_t s_palette[256];
static uint8_t s_rgb[256][3];       /* the same colours in 8-bit RGB, for the shades */

/* The font (lcd.font): cell size, bytes a glyph row, glyphs. */
static int s_cw = 8, s_ch = 16, s_bpr = 1;
static int s_cols = DISPLAY_MAX_COLS, s_rows = DISPLAY_MAX_ROWS;
static const uint8_t *(*s_glyph)(uint16_t cp) = font8x16_glyph;

/* What the panel shows, to paint only what changed. */
static td_vcell_t s_shown[2][DISPLAY_MAX_ROWS][DISPLAY_MAX_COLS];
static td_vcell_t s_snap[DISPLAY_MAX_ROWS][DISPLAY_MAX_COLS];
static int s_cur_x[2] = {-1, -1}, s_cur_y[2] = {-1, -1};

/* ------------------------------------------------------------ colours */

static uint16_t rgb565(int r, int g, int b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void set_colour(int i, int r, int g, int b)
{
    s_rgb[i][0] = (uint8_t)r;
    s_rgb[i][1] = (uint8_t)g;
    s_rgb[i][2] = (uint8_t)b;
    s_palette[i] = rgb565(r, g, b);
}

/* The xterm palette: 16 colours, the 6x6x6 cube, 24 greys. */
static void make_palette(void)
{
    static const uint8_t base[16][3] = {
        {0, 0, 0},
        {205, 0, 0},
        {0, 205, 0},
        {205, 205, 0},
        {0, 0, 238},
        {205, 0, 205},
        {0, 205, 205},
        {229, 229, 229},
        {127, 127, 127},
        {255, 0, 0},
        {0, 255, 0},
        {255, 255, 0},
        {92, 92, 255},
        {255, 0, 255},
        {0, 255, 255},
        {255, 255, 255},
    };
    for (int i = 0; i < 16; i++)
        set_colour(i, base[i][0], base[i][1], base[i][2]);
    static const uint8_t level[6] = {0, 95, 135, 175, 215, 255};
    for (int i = 0; i < 216; i++)
        set_colour(16 + i, level[i / 36], level[(i / 6) % 6], level[i % 6]);
    for (int i = 0; i < 24; i++)
    {
        int v = 8 + i * 10;
        set_colour(232 + i, v, v, v);
    }
}

/* fg over bg at quarters (1: 25 %, 2: 50 %, 3: 75 %). */
static uint16_t blend(uint8_t fg, uint8_t bg, int quarters)
{
    int c[3];
    for (int k = 0; k < 3; k++)
        c[k] = (s_rgb[fg][k] * quarters + s_rgb[bg][k] * (4 - quarters)) / 4;
    return rgb565(c[0], c[1], c[2]);
}

/* ------------------------------------------------------------ pictures */

/* Sixel pictures (td_picture.h on the desktop's side) at the panel's own
 * resolution. A decoded picture goes into a slot in full colour, and the
 * cells it covers get tile marks: character TILE_BASE + its tile, the slot
 * in fg. The painter draws a marked cell from the picture; text written
 * there later replaces the mark, as a terminal's text replaces a picture.
 * Slots no cell refers to any more are freed. */
#define IMG_SLOTS 128
#define TILE_BASE 0xE000          /* Unicode's private use area */
#define TILE_MAX  6400
#define RGB_SLOT 255

typedef struct
{
    uint16_t *px;                 /* w x h RGB565, in PSRAM */
    int w, h, cols;               /* cols: cells across */
    uint32_t generation;
} slot_t;

static slot_t s_slots[IMG_SLOTS];
static uint32_t s_slot_generation, s_shown_generation[2][IMG_SLOTS];
static int s_next_slot;
static SemaphoreHandle_t s_img_lock;   /* the slots, between decoding and painting */
/* Native RGB pictures are already composited/opaque. Keep their final pixels
 * once at screen coordinates instead of retaining every overlapping panel,
 * label, background and old page in separate full-size allocations. */
static uint16_t *s_rgb_plane;
static uint32_t *s_rgb_generation,*s_rgb_shown[2],s_rgb_sequence;
static bool s_have_slots;

/* Caller holds s_img_lock and s_lock. */
static void release_unused_slots(void)
{
    if(!s_have_slots)return;
    bool used[IMG_SLOTS]={false};
    for(int r=0;r<s_rows;r++)for(int c=0;c<s_cols;c++){
        td_vcell_t v=s_vt->cells[r*TD_VT_MAX_COLS+c];
        if(v.ch>=TILE_BASE&&v.ch<TILE_BASE+TILE_MAX&&v.fg<IMG_SLOTS)used[v.fg]=true;
    }
    s_have_slots=false;
    for(int i=0;i<IMG_SLOTS;i++)if(s_slots[i].px){
        if(!used[i]){heap_caps_free(s_slots[i].px);s_slots[i].px=NULL;}
        else s_have_slots=true;
    }
}

static struct
{
    bool data;                    /* past the 'q' */
    int cx, cy;                   /* the cell the picture starts at */
    int x, y;                     /* the next sixel's pixel column, band top */
    int w, h;
    uint16_t *buf;
    uint16_t pal[256];
    int colour, repeat;
    char cmd;                     /* '"', '#', '!' while their numbers come */
    int num[5], nn;
} s_six;

static void six_command_done(void)
{
    if (s_six.cmd == '"' && s_six.nn >= 4 && !s_six.buf)
    {
        int w = s_six.num[2], h = s_six.num[3];
        if (w > 0 && h > 0 && w <= DISPLAY_W && h <= DISPLAY_H)
        {
            s_six.buf = heap_caps_calloc((size_t)w * (size_t)h, 2, MALLOC_CAP_SPIRAM);
            if (s_six.buf)
            {
                s_six.w = w;
                s_six.h = h;
            }
        }
    }
    else if (s_six.cmd == '#' && s_six.nn >= 1)
    {
        int c = s_six.num[0] & 255;
        if (s_six.nn >= 5 && s_six.num[1] == 2)   /* RGB in percent */
            s_six.pal[c] = rgb565(s_six.num[2] * 255 / 100, s_six.num[3] * 255 / 100, s_six.num[4] * 255 / 100);
        s_six.colour = c;
    }
    else if (s_six.cmd == '!' && s_six.nn >= 1)
    {
        s_six.repeat = s_six.num[0];
    }
    s_six.cmd = 0;
}

static void six_sixel(int bits)
{
    int n = s_six.repeat > 0 ? s_six.repeat : 1;
    s_six.repeat = 0;
    uint16_t c = s_six.pal[s_six.colour];
    for (int i = 0; i < n; i++, s_six.x++)
    {
        if (!s_six.buf || s_six.x >= s_six.w)
            continue;
        for (int k = 0; k < 6; k++)
            if ((bits >> k) & 1 && s_six.y + k < s_six.h)
                s_six.buf[(s_six.y + k) * s_six.w + s_six.x] = c;
    }
}

/* The finished picture into a slot, and its cells marked. Runs while
 * td_vterm_write() holds s_lock. */
static bool six_finish(void)
{
    if (!s_six.buf)
        return false;
    int cols = (s_six.w + s_cw - 1) / s_cw, rows = (s_six.h + s_ch - 1) / s_ch;
    if (cols * rows > TILE_MAX)
    {
        heap_caps_free(s_six.buf);
        s_six.buf = NULL;
        return false;
    }
    xSemaphoreTake(s_img_lock, portMAX_DELAY);
    /* A rotating allocator must not replace a slot still referenced by a
     * different field. Rapid value changes used to wrap onto static labels
     * and other readings, leaving their tiles showing unrelated pixels.
     * Tiles inside this incoming rectangle will be replaced together, so
     * their slots can be reused unless another tile outside still needs it. */
    bool protected[IMG_SLOTS] = {false};
    for (int r = 0; r < s_rows; r++)
        for (int c = 0; c < s_cols; c++)
        {
            if (r >= s_six.cy && r < s_six.cy + rows && c >= s_six.cx && c < s_six.cx + cols)
                continue;
            td_vcell_t v = s_vt->cells[r * TD_VT_MAX_COLS + c];
            if (v.ch >= TILE_BASE && v.ch < TILE_BASE + TILE_MAX && v.fg < IMG_SLOTS)
                protected[v.fg] = true;
        }
    int slot = -1;
    for (int i = 0; i < IMG_SLOTS; i++)
    {
        int candidate = (s_next_slot + i) % IMG_SLOTS;
        if (!protected[candidate])
        {
            slot = candidate;
            break;
        }
    }
    if (slot < 0)
    {
        xSemaphoreGive(s_img_lock);
        heap_caps_free(s_six.buf);
        s_six.buf = NULL;
        return false; /* preserve existing fields when all slots are live */
    }
    s_next_slot = (slot + 1) % IMG_SLOTS;
    heap_caps_free(s_slots[slot].px);
    s_slots[slot] = (slot_t){s_six.buf, s_six.w, s_six.h, cols, ++s_slot_generation};
    s_have_slots=true;
    s_six.buf = NULL;
    for (int r = 0; r < rows && s_six.cy + r < s_rows; r++)
        for (int c = 0; c < cols && s_six.cx + c < s_cols; c++)
            s_vt->cells[(s_six.cy + r) * TD_VT_MAX_COLS + s_six.cx + c] =
                (td_vcell_t){(uint16_t)(TILE_BASE + r * cols + c), (uint8_t)slot, 0};
    release_unused_slots();
    xSemaphoreGive(s_img_lock);
    s_vt->dirty = true;
    return true;
}

/* Local kiosk output avoids encoding pixels as terminal bytes only to
 * decode them back on the same CPU. Serial mirror mode keeps Sixel. */
static bool direct_picture(int x,int y,const uint8_t *indexed,const uint16_t *rgb,int w,int h,const uint16_t *runs,size_t words)
{
    if (!s_vt || !s_img_lock || x < 0 || y < 0 || w <= 0 || h <= 0 ||
        x * s_cw + w > s_cols*s_cw || y * s_ch + h > s_rows*s_ch ||
        ((w + s_cw - 1) / s_cw) * ((h + s_ch - 1) / s_ch) > TILE_MAX)
        return false;
    if(runs){
        size_t total=(size_t)w*h,expanded=0;
        if(!words||(words&1u)||words>total*2)return false;
        for(size_t i=0;i<words;i+=2){
            if(!runs[i]||runs[i]>total-expanded)return false;
            expanded+=runs[i];
        }
        if(expanded!=total)return false;
    }
    if(rgb||runs){
        xSemaphoreTake(s_lock,portMAX_DELAY);
        if(s_six.buf){xSemaphoreGive(s_lock);return false;}
        xSemaphoreTake(s_img_lock,portMAX_DELAY);
        if(!s_rgb_plane){
            size_t pixels=(size_t)DISPLAY_W*DISPLAY_H,cells=(size_t)s_cols*s_rows;
            s_rgb_plane=heap_caps_calloc(1,pixels*sizeof(uint16_t)+3*cells*sizeof(uint32_t),MALLOC_CAP_SPIRAM);
            if(s_rgb_plane){
                s_rgb_generation=(uint32_t *)(s_rgb_plane+pixels);
                s_rgb_shown[0]=s_rgb_generation+cells;s_rgb_shown[1]=s_rgb_shown[0]+cells;
            }
        }
        if(!s_rgb_plane){xSemaphoreGive(s_img_lock);xSemaphoreGive(s_lock);return false;}
        int px=x*s_cw,py=y*s_ch;
        int cols=(w+s_cw-1)/s_cw,rows=(h+s_ch-1)/s_ch;
        if(w%s_cw||h%s_ch)for(int row=0;row<rows*s_ch;row++)memset(s_rgb_plane+(size_t)(py+row)*DISPLAY_W+px,0,(size_t)cols*s_cw*sizeof(uint16_t));
        if(runs){
            size_t at=0;
            for(size_t i=0;i<words;i+=2){
                size_t left=runs[i];uint16_t color=runs[i+1];
                while(left){
                    size_t row=at/(size_t)w,col=at%(size_t)w,n=(size_t)w-col;
                    if(n>left)n=left;
                    uint16_t *dst=s_rgb_plane+((size_t)py+row)*DISPLAY_W+px+col;
                    if((color&255u)==(color>>8))memset(dst,color&255u,n*sizeof(*dst));
                    else for(size_t k=0;k<n;k++)dst[k]=color;
                    at+=n;left-=n;
                }
            }
        }else for(int row=0;row<h;row++)memcpy(s_rgb_plane+(size_t)(py+row)*DISPLAY_W+px,rgb+(size_t)row*w,(size_t)w*sizeof(*rgb));
        uint32_t generation=++s_rgb_sequence;
        for(int r=y;r<y+rows;r++)for(int c=x;c<x+cols;c++){
            int tile=r*s_cols+c;s_rgb_generation[tile]=generation;
            s_vt->cells[r*TD_VT_MAX_COLS+c]=(td_vcell_t){(uint16_t)(TILE_BASE+tile),RGB_SLOT,0};
        }
        release_unused_slots();s_vt->dirty=true;
        xSemaphoreGive(s_img_lock);xSemaphoreGive(s_lock);return true;
    }
    uint16_t *buf = heap_caps_malloc((size_t)w * h * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!buf)
        return false;
    for (int i = 0; i < w * h; i++)
        buf[i] = rgb?rgb[i]:s_palette[indexed[i]];
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_six.buf) /* Do not disturb an unfinished terminal picture. */
    {
        xSemaphoreGive(s_lock);
        heap_caps_free(buf);
        return false;
    }
    s_six.cx = x;
    s_six.cy = y;
    s_six.w = w;
    s_six.h = h;
    s_six.buf = buf;
    bool completed = six_finish();
    xSemaphoreGive(s_lock);
    return completed;
}

bool display_picture(void *user,int x,int y,const uint8_t *pixels,int w,int h)
{(void)user;return direct_picture(x,y,pixels,NULL,w,h,NULL,0);}

bool display_picture_rgb(void *user,int x,int y,const uint16_t *pixels,int w,int h)
{(void)user;return direct_picture(x,y,NULL,pixels,w,h,NULL,0);}

bool display_picture_runs(void *user,int x,int y,const uint16_t *runs,size_t words,int w,int h)
{(void)user;return runs&&direct_picture(x,y,NULL,NULL,w,h,runs,words);}

/* td_vterm_t.dcs: the bytes of a DCS string; -1 before, -2 after. */
static void six_byte(void *user, int b)
{
    (void)user;
    if (b == -1)
    {
        heap_caps_free(s_six.buf);
        memset(&s_six, 0, sizeof(s_six));
        memcpy(s_six.pal, s_palette, sizeof(s_six.pal));
        s_six.cx = s_vt->cx;
        s_six.cy = s_vt->cy;
        return;
    }
    if (b == -2)
    {
        six_command_done();
        if (s_six.data)
            six_finish();
        heap_caps_free(s_six.buf);
        s_six.buf = NULL;
        return;
    }
    if (!s_six.data)
    {
        s_six.data = b == 'q';    /* the parameters before it are not needed */
        return;
    }
    if (s_six.cmd)
    {
        if (b >= '0' && b <= '9')
        {
            if (s_six.nn == 0)
                s_six.nn = 1;
            if (s_six.nn <= 5 && s_six.num[s_six.nn - 1] < 100000)
                s_six.num[s_six.nn - 1] = s_six.num[s_six.nn - 1] * 10 + (b - '0');
            return;
        }
        if (b == ';')
        {
            if (s_six.nn == 0)
                s_six.nn = 1;
            if (s_six.nn < 5)
                s_six.num[s_six.nn++] = 0;
            return;
        }
        six_command_done();
    }
    if (b == '"' || b == '#' || b == '!')
    {
        s_six.cmd = (char)b;
        s_six.nn = 0;
        memset(s_six.num, 0, sizeof(s_six.num));
    }
    else if (b == '$')
        s_six.x = 0;
    else if (b == '-')
    {
        s_six.x = 0;
        s_six.y += 6;
    }
    else if (b >= '?' && b <= '~')
        six_sixel(b - '?');
}

/* A tile of a picture into the cell at col, row; false if it is gone. */
static bool paint_tile(int col, int row, td_vcell_t c)
{
    bool ok = false;
    if(c.fg==RGB_SLOT&&s_rgb_plane){
        int tile=c.ch-TILE_BASE;
        if(tile>=s_cols*s_rows)return false;
        int tx=tile%s_cols,ty=tile/s_cols;
        for(int y=0;y<s_ch;y++)memcpy(s_fb+(size_t)(row*s_ch+y)*DISPLAY_W+col*s_cw,s_rgb_plane+(size_t)(ty*s_ch+y)*DISPLAY_W+tx*s_cw,(size_t)s_cw*sizeof(uint16_t));
        return true;
    }
    if (c.fg >= IMG_SLOTS)
        return false;
    const slot_t *s = &s_slots[c.fg];
    if (s->px)
    {
        int tile = c.ch - TILE_BASE, tx = tile % s->cols, ty = tile / s->cols;
        uint16_t *p = s_fb + (size_t)row * s_ch * DISPLAY_W + (size_t)col * s_cw;
        for (int y = 0; y < s_ch; y++, p += DISPLAY_W)
        {
            int sy = ty * s_ch + y;
            for (int x = 0; x < s_cw; x++)
            {
                int sx = tx * s_cw + x;
                p[x] = sx < s->w && sy < s->h ? s->px[sy * s->w + sx] : 0;
            }
        }
        ok = true;
    }
    return ok;
}

/* ----------------------------------------------------------- painting */

static void paint_cell(int col, int row, td_vcell_t c, bool cursor)
{
    /* A terminal caret must never reinterpret an image tile as a font
     * glyph (private-use tile codes otherwise become a '?' on black). */
    if (c.ch >= TILE_BASE && c.ch < TILE_BASE + TILE_MAX && paint_tile(col, row, c))
        return;
    /* The shades (the desktop's background) as one mixed colour: as dots
     * of pixels they flicker on this TN panel, whose polarity inversion
     * beats against such patterns. */
    uint16_t *p = s_fb + (size_t)row * s_ch * DISPLAY_W + (size_t)col * s_cw;
    if (c.ch >= 0x2591 && c.ch <= 0x2593 && !cursor)
    {
        uint16_t mix = blend(c.fg, c.bg, c.ch - 0x2590);
        for (int y = 0; y < s_ch; y++, p += DISPLAY_W)
            for (int x = 0; x < s_cw; x++)
                p[x] = mix;
        return;
    }
    const uint8_t *g = s_glyph(c.ch);
    if (!g && c.ch > ' ')
        g = s_glyph('?');
    uint16_t fg = s_palette[c.fg], bg = s_palette[c.bg];
    for (int y = 0; y < s_ch; y++, p += DISPLAY_W)
    {
        const uint8_t *bits = g ? g + y * s_bpr : NULL;
        bool under = cursor && y >= s_ch - s_ch / 8;   /* an underline cursor, in reverse */
        for (int x = 0; x < s_cw; x++)
        {
            bool on = bits && (bits[x >> 3] & (0x80 >> (x & 7)));
            p[x] = on != under ? fg : bg;
        }
    }
}

static bool same(td_vcell_t a, td_vcell_t b)
{
    return a.ch == b.ch && a.fg == b.fg && a.bg == b.bg;
}

static bool native_cell(td_vcell_t cell)
{return s_rgb_plane&&cell.fg==RGB_SLOT&&cell.ch>=TILE_BASE&&cell.ch<TILE_BASE+s_cols*s_rows;}

static bool cell_pixels_changed(unsigned frame,int r,int c,td_vcell_t cell)
{
    if(native_cell(cell)&&s_rgb_shown[frame][r*s_cols+c]!=s_rgb_generation[cell.ch-TILE_BASE])return true;
    if(cell.ch>=TILE_BASE&&cell.ch<TILE_BASE+TILE_MAX&&cell.fg<IMG_SLOTS&&
       s_shown_generation[frame][cell.fg]!=s_slots[cell.fg].generation)return true;
    return !same(cell,s_shown[frame][r][c]);
}
static bool cell_changed(unsigned frame,int r,int c,td_vcell_t cell,int cx,int cy)
{return (c==cx&&r==cy)||(c==s_cur_x[frame]&&r==s_cur_y[frame])||cell_pixels_changed(frame,r,c,cell);}

/* Adjacent native cells are adjacent pixels in the shared composition
 * plane. Copy each horizontal row once instead of walking 20 tiny rows
 * per character tile; source-row boundaries still stop a run after VT
 * scrolling/moves. All writes remain in the off-screen framebuffer. */
static void paint_rgb_span(int col,int row,td_vcell_t cell,int cols)
{
    int tile=cell.ch-TILE_BASE,tx=tile%s_cols,ty=tile/s_cols;
    for(int y=0;y<s_ch;y++)
        memcpy(s_fb+(size_t)(row*s_ch+y)*DISPLAY_W+col*s_cw,
               s_rgb_plane+(size_t)(ty*s_ch+y)*DISPLAY_W+tx*s_cw,
               (size_t)cols*s_cw*sizeof(uint16_t));
}

/* Copy the cells under the lock, paint outside it. */
static void paint(void)
{
    static TickType_t sync_started;
    static bool waiting;
    /* Do not recycle the old front buffer until the RGB bounce reader
     * has switched to the submitted complete frame. A delayed callback
     * leaves both buffers intact; a later tick can finish the handoff. */
    if (s_present_pending)
    {
        if (xSemaphoreTake(s_frame_done, 0) != pdTRUE)
            return;
        s_present_pending = false;
        s_draw_frame ^= 1;
        s_fb = s_frames[s_draw_frame];
    }
    int cx = -1, cy = -1;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_vt->synchronized_output)
    {
        TickType_t now = xTaskGetTickCount();
        if (!waiting)
            sync_started = now;
        waiting = true;
        /* Recover if a sender disappears mid-frame. */
        if (now - sync_started < pdMS_TO_TICKS(1500))
        {
            xSemaphoreGive(s_lock);
            return;
        }
        s_vt->synchronized_output = false;
    }
    waiting = false;
    if (!s_vt->dirty)
    {
        xSemaphoreGive(s_lock);
        return;
    }
    s_vt->dirty = false;
    int64_t paint_began=esp_timer_get_time();
    /* Pin the image slots used by this snapshot until all its tiles have
     * been painted. Writers take s_lock then s_img_lock in the same order. */
    xSemaphoreTake(s_img_lock, portMAX_DELAY);
    for (int r = 0; r < s_rows; r++)
        memcpy(s_snap[r], &s_vt->cells[r * TD_VT_MAX_COLS], (size_t)s_cols * sizeof(td_vcell_t));
    if (s_vt->cursor_visible)
    {
        cx = s_vt->cx < s_cols ? s_vt->cx : s_cols - 1;
        cy = s_vt->cy;
    }
    xSemaphoreGive(s_lock);

    unsigned frame = s_draw_frame;
    unsigned front=frame^1u;
    bool visible_change=cx!=s_cur_x[front]||cy!=s_cur_y[front];
    for(int r=0;r<s_rows&&!visible_change;r++)for(int c=0;c<s_cols;c++)
        if(cell_pixels_changed(front,r,c,s_snap[r][c])){visible_change=true;break;}
    if(!visible_change){xSemaphoreGive(s_img_lock);return;}
    /* A retained backbuffer may already contain the requested scene.
     * Compare visibility against the front, then update against the back;
     * it still needs presenting even when no destination pixels changed. */
    bool changed = true;
    for (int r = 0; r < s_rows; r++)
        for (int c = 0; c < s_cols; c++)
        {
            td_vcell_t cell = s_snap[r][c];
            bool cur = (c == cx && r == cy);
            bool native=native_cell(cell);
            if (cell_changed(frame,r,c,cell,cx,cy))
            {
                if(native){
                    int cols=1,source_col=(cell.ch-TILE_BASE)%s_cols;
                    while(c+cols<s_cols&&source_col+cols<s_cols){
                        td_vcell_t next=s_snap[r][c+cols];
                        if(!native_cell(next)||next.ch!=cell.ch+cols||!cell_changed(frame,r,c+cols,next,cx,cy))break;
                        cols++;
                    }
                    paint_rgb_span(c,r,cell,cols);
                    for(int n=0;n<cols;n++){
                        td_vcell_t next=s_snap[r][c+n];
                        s_shown[frame][r][c+n]=next;
                        s_rgb_shown[frame][r*s_cols+c+n]=s_rgb_generation[next.ch-TILE_BASE];
                    }
                    changed=true;c+=cols-1;continue;
                }
                paint_cell(c, r, cell, cur);
                s_shown[frame][r][c] = cell;
                changed = true;
            }
            if(s_rgb_plane)s_rgb_shown[frame][r*s_cols+c]=native?s_rgb_generation[cell.ch-TILE_BASE]:0;
        }
    for (int i = 0; i < IMG_SLOTS; i++)
        s_shown_generation[frame][i] = s_slots[i].generation;
    s_cur_x[frame] = cx;
    s_cur_y[frame] = cy;
    xSemaphoreGive(s_img_lock);
    if (changed)
    {
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, DISPLAY_W, DISPLAY_H, s_fb);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "present frame: %s", esp_err_to_name(err));
            /* Retry the completed frame; never paint into a submitted one. */
            xSemaphoreTake(s_lock, portMAX_DELAY);
            s_vt->dirty = true;
            xSemaphoreGive(s_lock);
            memset(s_shown[frame], 0xFF, sizeof(s_shown[frame]));
            return;
        }
        int64_t submitted=esp_timer_get_time();uint32_t work_us=(uint32_t)(submitted-paint_began);
        atomic_store(&s_paint_timing[0],(uint32_t)(submitted/1000));
        atomic_store(&s_paint_timing[1],work_us);
        if(work_us>atomic_load(&s_paint_timing[2]))atomic_store(&s_paint_timing[2],work_us);
        atomic_fetch_add(&s_paint_timing[3],work_us);
        atomic_fetch_add(&s_scanout[4],1);
        /* Arm after draw_bitmap commits the requested buffer. A callback
         * racing before this point is ignored, so it cannot release the
         * previous buffer before the driver has stopped reading it. */
        portENTER_CRITICAL(&s_swap_lock);
        s_swap_waiting = true;
        portEXIT_CRITICAL(&s_swap_lock);
        s_present_pending = true;
    }
}

static void paint_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        paint();
        vTaskDelay(pdMS_TO_TICKS(PAINT_MS));
    }
}

/* ---------------------------------------------------------- terminal */

static void vt_reply(void *user, const char *data, int len)
{
    (void)user;
    if (s_reply)
        s_reply(data, len);
}

void display_write(const uint8_t *buf, int len)
{
    if (!s_vt || len <= 0)
        return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    td_vterm_write(s_vt, buf, len);
    xSemaphoreGive(s_lock);
}

/* -------------------------------------------------------------- panel */

static bool IRAM_ATTR frame_complete(esp_lcd_panel_handle_t panel,
                                    const esp_lcd_rgb_panel_event_data_t *event, void *user)
{
    (void)panel;
    (void)event;
    (void)user;
    int64_t now=esp_timer_get_time();
    if(s_last_prepared_us){
        uint32_t gap=(uint32_t)(now-s_last_prepared_us);
        if(gap>atomic_load(&s_scanout[3]))atomic_store(&s_scanout[3],gap);
    }
    s_last_prepared_us=now;
    s_prepared_at++;
    atomic_fetch_add(&s_scanout[1],1);
    portENTER_CRITICAL_ISR(&s_swap_lock);
    bool waiting = s_swap_waiting;
    s_swap_waiting = false;
    portEXIT_CRITICAL_ISR(&s_swap_lock);
    BaseType_t wake = pdFALSE;
    if (waiting)
        xSemaphoreGiveFromISR(s_frame_done, &wake);
    return wake == pdTRUE;
}

static bool IRAM_ATTR scanout_vsync(esp_lcd_panel_handle_t panel,
                                  const esp_lcd_rgb_panel_event_data_t *event,void *user)
{
    (void)panel;(void)event;(void)user;
    unsigned frames=atomic_fetch_add(&s_scanout[0],1);
    /* Skip startup phase alignment; completion should advance once between
     * successive VSYNCs. This callback precedes IDF's underrun recovery. */
    if(frames>2&&s_prepared_at==s_previous_prepared)atomic_fetch_add(&s_scanout[2],1);
    s_previous_prepared=s_prepared_at;
    return false;
}

static bool panel_init(void)
{
    int mhz = tdsh_board_int("lcd.pclk_mhz", PCLK_MHZ);
    if (mhz < 10 || mhz > 40)
        mhz = PCLK_MHZ;
    atomic_store(&s_scanout[5],(unsigned)mhz*1000000u);
    atomic_store(&s_scanout[6],xPortGetCoreID());
    atomic_store(&s_scanout[7],BOUNCE_LINES);
    esp_lcd_rgb_panel_config_t cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings =
            {
                .pclk_hz = (uint32_t)mhz * 1000000u,
                .h_res = DISPLAY_W,
                .v_res = DISPLAY_H,
                .hsync_pulse_width = 48,
                .hsync_back_porch = 40,
                .hsync_front_porch = 40,
                .vsync_pulse_width = 31,
                .vsync_back_porch = 13,
                .vsync_front_porch = 1,
                .flags.pclk_active_neg = 1,
            },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 2,
        .bounce_buffer_size_px = DISPLAY_W * BOUNCE_LINES,
        .psram_trans_align = 64,
        .hsync_gpio_num = PIN_HSYNC,
        .vsync_gpio_num = PIN_VSYNC,
        .de_gpio_num = PIN_DE,
        .pclk_gpio_num = PIN_PCLK,
        .disp_gpio_num = -1,
        .flags.fb_in_psram = 1,
    };
    memcpy(cfg.data_gpio_nums, s_data_pins, sizeof(s_data_pins));
    esp_lcd_panel_handle_t panel = NULL;
    s_frame_done = xSemaphoreCreateBinary();
    if (!s_frame_done)
        return false;
    esp_err_t err = esp_lcd_new_rgb_panel(&cfg, &panel);
    if (err == ESP_OK)
        err = esp_lcd_panel_reset(panel);
    if (err == ESP_OK)
        err = esp_lcd_rgb_panel_get_frame_buffer(panel, 2, (void **)&s_frames[0], (void **)&s_frames[1]);
    if (err == ESP_OK)
    {
        memset(s_frames[0], 0, (size_t)DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
        memset(s_frames[1], 0, (size_t)DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
        esp_lcd_rgb_panel_event_callbacks_t callbacks = {.on_bounce_frame_finish = frame_complete,.on_vsync=scanout_vsync};
        err = esp_lcd_rgb_panel_register_event_callbacks(panel, &callbacks, NULL);
    }
    if (err == ESP_OK)
        err = esp_lcd_panel_init(panel);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "RGB panel: %s", esp_err_to_name(err));
        return false;
    }
    s_panel = panel;
    s_fb = s_frames[s_draw_frame];

    /* The LCD's pins are not for scripts (machine.Pin refuses reserved pins). */
    uint64_t mask = BIT64(PIN_DE) | BIT64(PIN_VSYNC) | BIT64(PIN_HSYNC) | BIT64(PIN_PCLK) | BIT64(PIN_BACKLIGHT);
    for (int i = 0; i < 16; i++)
        mask |= BIT64(s_data_pins[i]);
    esp_gpio_reserve(mask);
    return true;
}

static void backlight_on(void)
{
    gpio_config_t io = {.pin_bit_mask = BIT64(PIN_BACKLIGHT), .mode = GPIO_MODE_OUTPUT};
    gpio_config(&io);
    gpio_set_level(PIN_BACKLIGHT, 1);
}

int display_cols(void)
{
    return s_cols;
}

int display_rows(void)
{
    return s_rows;
}

/* lcd.font: 16 (8x16, the default), 20, 24 or 32 pixels high. */
static void choose_font(void)
{
    switch (tdsh_board_int("lcd.font", 16))
    {
    case 20:
        s_cw = 10, s_ch = 20, s_glyph = font10x20_glyph;
        break;
    case 24:
        s_cw = 12, s_ch = 24, s_glyph = font12x24_glyph;
        break;
    case 32:
        s_cw = 16, s_ch = 32, s_glyph = font16x32_glyph;
        break;
    default:
        s_cw = 8, s_ch = 16, s_glyph = font8x16_glyph;
        break;
    }
    s_bpr = (s_cw + 7) / 8;
    s_cols = DISPLAY_W / s_cw;
    s_rows = DISPLAY_H / s_ch;
}

bool display_init(display_reply_fn reply)
{
    s_reply = reply;
    choose_font();
    make_palette();
    s_vt = heap_caps_calloc(1, sizeof(*s_vt), MALLOC_CAP_SPIRAM);
    s_lock = xSemaphoreCreateMutex();
    if (!s_vt || !s_lock || !panel_init())
        return false;
    td_vterm_init(s_vt, s_cols, s_rows);
    s_vt->reply = vt_reply;
    /* Pictures: Sixel at the panel's own pixels, a cell s_cw x s_ch. */
    s_img_lock = xSemaphoreCreateMutex();
    s_vt->dcs = six_byte;
    s_vt->dcs_user = NULL;
    s_vt->sixel = s_img_lock != NULL;
    s_vt->cell_w = s_cw;
    s_vt->cell_h = s_ch;

    /* Each buffer tracks its own last cells; updating small values does
     * not require copying the entire 750 KiB framebuffer on every swap. */
    memset(s_shown, 0xFF, sizeof(s_shown));
    s_vt->dirty = true;
    backlight_on();
#if defined(TD_CROWPANEL_RENDER_CORE) && TD_CROWPANEL_RENDER_CORE >= 0
    if (xTaskCreatePinnedToCore(paint_task, "lcd", PAINT_STACK, NULL, PAINT_PRIO, NULL, TD_CROWPANEL_RENDER_CORE) != pdPASS)
#else
    if (xTaskCreate(paint_task, "lcd", PAINT_STACK, NULL, PAINT_PRIO, NULL) != pdPASS)
#endif
        return false;
    ESP_LOGI(TAG, "LCD %dx%d double-buffered as %dx%d cells of %dx%d, pixel clock %d MHz", DISPLAY_W, DISPLAY_H, s_cols, s_rows, s_cw,
             s_ch, tdsh_board_int("lcd.pclk_mhz", PCLK_MHZ));
    return true;
}
