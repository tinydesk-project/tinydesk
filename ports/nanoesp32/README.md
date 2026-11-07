# Arduino Nano ESP32

The Nano ESP32 is an ESP32-S3 (NORA-W106) with 16 MB flash and 8 MB Octal
PSRAM, so the chip matches `ports/esp32s3`. What differs is the console: the
board has **no USB-UART bridge chip**. Its USB-C connector is wired to the
ESP32-S3's own USB Serial/JTAG peripheral (GPIO19/20), so this port serves the
desktop from `main/link_usj.c` the way `ports/esp32c6` does, not from a UART the
way `ports/esp32s3` does.

```
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash
```

No baud argument is needed or used: the link is USB, and line speed is ignored.

## Two things that cost time once already

**The partition table replaces the Arduino one, and data does not come back.**
The board ships with `app3M_spiffs9M_fact512k_16MB.csv`: two 3 MB apps and a
9 MB spiffs volume from `0x610000`. This project's `partitions.csv` (the
`ports/esp32s3` layout) puts its second app slot and `otadata` over that
volume, so flashing this firmware over an Arduino or PlatformIO sketch
erases it, and flashing the sketch back does **not** restore it, because the
partition table is not rebuilt from the sketch alone. Read the flash out
first if it holds anything that cannot be regenerated:

```
esptool --port /dev/ttyACM0 --before usb_reset read_flash 0 0x1000000 backup.bin
```

The underscore in `read_flash` is esptool 4.x's spelling, and this is the
version ESP-IDF 5.3.1 installs; the older hyphenated name is not accepted.
`--before usb_reset` is the same rule as below: a reset over this port goes
through the USB peripheral, not a UART's RTS line.

**A reset over USB-JTAG is not a UART reset.** `esptool --after hard_reset`
toggles RTS, which does nothing on this port, and `--before no_reset` leaves the
chip parked in the stub loader with no USB response at all. If `idf.py flash`
seems to have produced a silent board, reset it into the application with:

```
esptool --port /dev/ttyACM0 --before usb_reset --after watchdog_reset read-mac
```

or unplug and replug. Plain `idf.py flash` is correct and needs neither.

## Not built here

The Shell edition is not part of this port. On this board the USB-C connector
belongs to the desktop, so a standalone shell would have to come up on UART0
(D0/D1) through an external adapter, which is a different bring-up.

## Software Update

This port defines `TD_UPDATE_BOARD="nanoesp32"`: Software Update reads
`update-desktop-nanoesp32.json` on the official update site, never the
`esp32s3` one. Without it the port would inherit `esp32s3` as its board
name and Software Update would offer the UART build, replacing a working
firmware with one this board cannot display.
