/*
 * touch.h - the CrowPanel's capacitive touch screen (GT911) as the
 * desktop's mouse.
 */
#ifndef TOUCH_H
#define TOUCH_H

#include <stdbool.h>

/* Called with mouse reports (SGR sequences, as a terminal sends them). */
typedef void (*touch_input_fn)(const char *data, int len);

/* Start polling the controller; false when it does not answer. */
bool touch_init(touch_input_fn input);

#endif
