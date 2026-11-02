/*
 * td_py_hw_pins.c - which pins TinyDesk keeps for itself, for the machine
 * module (td_py_hw_esp.c). No ESP-IDF headers, so the PC build tests it
 * (tests/test_py_pins.c).
 */
#include "td_py_hw_pins.h"

#include <stdlib.h>
#include <string.h>

#include "tdsh_board.h"

/* Keys of the board configuration whose value is a pin. */
static bool pin_key(const char *key)
{
    static const char *const ends[] = {".tx", ".rx", ".de", ".cs", ".sclk", ".mosi", ".miso", ".int", ".rst"};
    size_t n = strlen(key);
    for (size_t i = 0; i < sizeof(ends) / sizeof(ends[0]); i++)
    {
        size_t m = strlen(ends[i]);
        if (n > m && strcmp(key + n - m, ends[i]) == 0)
            return true;
    }
    return false;
}

const char *td_hw_pin_owner(int pin, td_hw_chip_t chip)
{
    const char *key, *value;
    tdsh_board_origin_t origin;
    for (int i = 0; tdsh_board_at(i, &key, &value, &origin); i++)
    {
        char *end = NULL;
        long v = strtol(value, &end, 10);
        if (pin_key(key) && end && end != value && !*end && v == pin)
            return key;
    }
    if (chip == TD_HW_CHIP_ESP32 || chip == TD_HW_CHIP_ESP32S3)
    {   /* the desktop's UART0 on its default pins */
        int tx = chip == TD_HW_CHIP_ESP32 ? 1 : 43, rx = chip == TD_HW_CHIP_ESP32 ? 3 : 44;
        if ((pin == tx && !tdsh_board_get("console.tx")) || (pin == rx && !tdsh_board_get("console.rx")))
            return "the desktop's serial link";
    }
    else if (chip == TD_HW_CHIP_ESP32C6)
    {
        if (pin == 12 || pin == 13)
            return "USB, the desktop's link";
    }
    return NULL;
}

bool td_hw_pin_strapping(int pin, td_hw_chip_t chip)
{
    if (chip == TD_HW_CHIP_ESP32)
        return pin == 0 || pin == 2 || pin == 5 || pin == 12 || pin == 15;
    if (chip == TD_HW_CHIP_ESP32C6)
        return pin == 8 || pin == 9 || pin == 15;
    if (chip == TD_HW_CHIP_ESP32S3)
        return pin == 0 || pin == 3 || pin == 45 || pin == 46;
    return false;
}

bool td_hw_uart_kept(int id, td_hw_chip_t chip)
{
    static const char *const keys[] = {"console.uart", "rs485.1.uart", "rs485.2.uart"};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if (tdsh_board_get(keys[i]) && tdsh_board_int(keys[i], -1) == id)
            return true;
    return (chip == TD_HW_CHIP_ESP32 || chip == TD_HW_CHIP_ESP32S3) && id == 0 &&
           !tdsh_board_get("console.uart");   /* the console */
}
