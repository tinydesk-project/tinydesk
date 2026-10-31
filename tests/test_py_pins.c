/*
 * test_py_pins.c - the pins and UARTs the machine module leaves to
 * TinyDesk (ports/common/python/hw/td_py_hw_pins.c): the board
 * configuration's, the desktop's link, and the strapping pins.
 */
#include <string.h>

#include "td_py_hw_pins.h"
#include "td_test.h"
#include "tdsh_board.h"

static bool owner_is(int pin, td_hw_chip_t chip, const char *expect)
{
    const char *got = td_hw_pin_owner(pin, chip);
    if (expect ? got && strcmp(got, expect) == 0 : got == NULL)
        return true;
    printf("  GPIO %d: \"%s\", expected \"%s\"\n", pin, got ? got : "(free)", expect ? expect : "(free)");
    return false;
}

int main(void)
{
    CHECK_EQ(tdsh_board_load("rs485.1.tx = 16\n"
                             "rs485.1.rx = 17\n"
                             "rs485.1.de = -1\n"
                             "rs485.1.uart = 1\n"
                             "eth.chip = w6100\n"
                             "eth.cs = 5\n"
                             "eth.spi_mhz = 20\n"
                             "sd.automount = yes\n",
                             NULL),
             0);

    /* Pins of the board configuration; other numbers in it are not pins. */
    CHECK(owner_is(16, TD_HW_CHIP_ESP32, "rs485.1.tx"));
    CHECK(owner_is(17, TD_HW_CHIP_ESP32C6, "rs485.1.rx"));
    CHECK(owner_is(5, TD_HW_CHIP_ESP32, "eth.cs"));
    CHECK(owner_is(20, TD_HW_CHIP_ESP32, NULL));   /* eth.spi_mhz */
    CHECK(owner_is(25, TD_HW_CHIP_ESP32, NULL));

    /* The desktop's link: UART0 on the classic ESP32 and the ESP32-S3, USB on the C6. */
    CHECK(owner_is(1, TD_HW_CHIP_ESP32, "the desktop's serial link"));
    CHECK(owner_is(3, TD_HW_CHIP_ESP32, "the desktop's serial link"));
    CHECK(owner_is(1, TD_HW_CHIP_ESP32C6, NULL));
    CHECK(owner_is(12, TD_HW_CHIP_ESP32C6, "USB, the desktop's link"));
    CHECK(owner_is(13, TD_HW_CHIP_ESP32C6, "USB, the desktop's link"));
    CHECK(owner_is(12, TD_HW_CHIP_ESP32, NULL));
    CHECK(owner_is(43, TD_HW_CHIP_ESP32S3, "the desktop's serial link"));
    CHECK(owner_is(44, TD_HW_CHIP_ESP32S3, "the desktop's serial link"));
    CHECK(owner_is(1, TD_HW_CHIP_ESP32S3, NULL));

    /* UARTs: RS-485's, and UART0 as the classic ESP32's console. */
    CHECK(td_hw_uart_kept(1, TD_HW_CHIP_ESP32C6));
    CHECK(!td_hw_uart_kept(2, TD_HW_CHIP_ESP32));
    CHECK(td_hw_uart_kept(0, TD_HW_CHIP_ESP32));
    CHECK(!td_hw_uart_kept(0, TD_HW_CHIP_ESP32C6));
    CHECK(td_hw_uart_kept(0, TD_HW_CHIP_ESP32S3));

    /* A console on other pins frees UART0's usual ones. */
    CHECK_EQ(tdsh_board_load("console.uart = 2\nconsole.tx = 22\nconsole.rx = 21\n", NULL), 0);
    CHECK(owner_is(1, TD_HW_CHIP_ESP32, NULL));
    CHECK(owner_is(43, TD_HW_CHIP_ESP32S3, NULL));
    CHECK(owner_is(22, TD_HW_CHIP_ESP32, "console.tx"));
    CHECK(td_hw_uart_kept(2, TD_HW_CHIP_ESP32));
    CHECK(!td_hw_uart_kept(0, TD_HW_CHIP_ESP32));
    CHECK(!td_hw_uart_kept(0, TD_HW_CHIP_ESP32S3));

    CHECK(td_hw_pin_strapping(0, TD_HW_CHIP_ESP32));
    CHECK(td_hw_pin_strapping(12, TD_HW_CHIP_ESP32));
    CHECK(!td_hw_pin_strapping(25, TD_HW_CHIP_ESP32));
    CHECK(td_hw_pin_strapping(9, TD_HW_CHIP_ESP32C6));
    CHECK(!td_hw_pin_strapping(0, TD_HW_CHIP_ESP32C6));
    CHECK(td_hw_pin_strapping(46, TD_HW_CHIP_ESP32S3));
    CHECK(!td_hw_pin_strapping(2, TD_HW_CHIP_ESP32S3));
    return TD_TEST_RESULT();
}
