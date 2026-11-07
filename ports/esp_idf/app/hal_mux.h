/*
 * hal_mux.h - tinydesk HAL for the ESP-IDF ports (see hal_mux.c).
 */
#ifndef HAL_MUX_H
#define HAL_MUX_H

#include "tinydesk/td_hal.h"
#include "tinydesk/td_sysinfo.h"

  /* Set up the board's link and return the HAL (NULL on error). */
const td_hal_t *hal_mux_init(void);

  /* Settings -> Displays: where display 1 and 2 get their terminals. */
const td_display_ops_t *hal_mux_displays(void);

  /* A board with its own display (the CrowPanel's LCD) can turn it: its
 * orientation in degrees, or -1 (the default: a terminal decides); the
 * setter saves one for the next start. */
int board_display_rotation(void);
bool board_set_display_rotation(int degrees);

#endif
