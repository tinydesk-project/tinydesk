/*
 * td_py_hw_pins.h - which pins and UARTs TinyDesk keeps for itself (the
 * board configuration, the desktop's link) and the strapping pins.
 */
#ifndef TD_PY_HW_PINS_H
#define TD_PY_HW_PINS_H

#include <stdbool.h>

typedef enum
{
    TD_HW_CHIP_OTHER,
    TD_HW_CHIP_ESP32,
    TD_HW_CHIP_ESP32C6
} td_hw_chip_t;

/* What of TinyDesk uses the pin (a board configuration key, or the link),
 * or NULL when it is free. */
const char *td_hw_pin_owner(int pin, td_hw_chip_t chip);

/* Pins the chip reads at reset to choose how it starts. */
bool td_hw_pin_strapping(int pin, td_hw_chip_t chip);

/* UARTs of the board configuration (and the classic ESP32's console). */
bool td_hw_uart_kept(int id, td_hw_chip_t chip);

#endif /* TD_PY_HW_PINS_H */
