/* Drive the real CrowPanel terminal/cache/painter in host integration tests.
 * Only the RTOS/peripheral boundary is simulated; captures never invalidate
 * the desktop or force pictures to be resent. */
#define TD_DISPLAY_TEST
#include "../ports/crowpanel7/main/display.c"
#include "tinydesk/td_picture.h"
#include <stdio.h>

bool sim_lcd_init(void)
{
    test_board_font = 20;
    if (!display_init(NULL))
        return false;
    td_picture_set_sink(display_picture, NULL);
    td_picture_set_rgb_sink(display_picture_rgb, NULL);
    td_picture_set_runs_sink(display_picture_runs, NULL);
    return true;
}

void sim_lcd_step(void)
{
    test_ticks += 5;
    if (test_requested)
    {
        test_visible = test_requested;
        test_requested = NULL;
        test_frame_callback(s_panel, NULL, NULL);
    }
    paint();
}

bool sim_lcd_capture(const char *path)
{
    FILE *out = fopen(path, "wb");
    if (!out)
        return false;
    fprintf(out, "P6\n800 480\n255\n");
    for (int i = 0; i < 800 * 480; i++)
    {
        uint8_t rgb[3];
        td_rgb565_rgb(test_visible[i], rgb);
        fwrite(rgb, 1, 3, out);
    }
    fclose(out);
    return true;
}
