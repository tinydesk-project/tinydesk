# Getting started

Clone with `git clone --recursive https://github.com/tinydesk-project/tinydesk.git`.
If you already cloned it, run `git submodule update --init --recursive`.

## On a PC

Install a C11 compiler, CMake and Ninja. Use GCC/Clang on Linux or macOS;
use MinGW GCC on Windows. From the repository root:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/tinydesk
```

On Windows run `./build/tinydesk.exe` inside Windows Terminal.
Host files live in `tinydesk_fs/` beneath the working directory.

## On an ESP32

Use ESP-IDF 5.3.1. Choose `ports/esp32s3` for a 16 MB S3 with Octal PSRAM,
`ports/esp32c6` for an 8 MB C6, `ports/esp32` for a 16 MB classic ESP32 with PSRAM,
or `ports/esp32-4mb` for a 4 MB classic ESP32. Check the board's flash and PSRAM before flashing.

In the selected directory, copy `board.example.conf` to `board.conf` only
if you need to change the wiring. Review the commented keys for RS-485,
Ethernet and SD peripherals; leave unconnected peripherals disabled.

```sh
idf.py build
idf.py -p PORT flash
```

Replace `PORT` with the board's serial port. Back up an existing board before
replacing its firmware. Close serial monitors before flashing or opening
another terminal. Avoid erasing flash as a routine update step.

## Terminal setup and first use

Use UTF-8, ANSI/VT cursor control, xterm mouse reporting and at least 80×25
cells. PuTTY with UTF-8 and xterm mouse reporting is one option for serial;
the [web terminal](https://tinydesk-project.github.io/console/) (Chrome or Edge) is
another, with nothing to install.
Classic ESP32 Desktop uses 921600 baud, 8 data bits, no parity, 1 stop bit,
and no flow control. C6 uses built-in USB Serial/JTAG; its baud setting is
ignored. The standalone shell uses 115200 baud on classic ESP32.

Press a key after connecting. Open Terminal and run `passwd` as root;
the initial password is `TinyDesk`. Try Files, create a small file in Editor,
save it, and reopen it. Drag a window by its title bar.

Telnet is disabled by default. It takes over the shared desktop, requires
root, and is unencrypted. Enable it explicitly in Network only when needed.
SSH is a separate shell with SFTP. All remote authentication requires changing
the factory root password first. See [security notes](../SECURITY.md).

If the display is garbled, check UTF-8, terminal dimensions and baud rate.
If clicks do not work, check xterm mouse reporting. If the port is busy,
close the program already connected to it.
