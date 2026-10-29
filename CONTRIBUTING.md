# Contributing to TinyDesk

Bug reports and patches are welcome. Include the board, terminal and source
revision when reporting a problem.

1. Clone with `git clone --recursive` (the shell is the submodule
   `third_party/tdsh`, repository `tinydesk-shell`).
2. Build and test on the PC: `cmake -B build -G Ninja && cmake --build build && ctest --test-dir build`.
3. Build the firmware you touched: `idf.py build` in `ports/esp32c6`, `ports/esp32` or `ports/esp32-4mb`
   (ESP-IDF 5.3.1). Code in `ports/esp_idf` is shared by all three.
4. Describe what changed for users (and the API) in the pull request, so the documentation
   and the changelog can follow. Add yourself to [CONTRIBUTORS.md](CONTRIBUTORS.md).
5. Set up formatting once:

   ```bash
   pip install pre-commit
   pre-commit install
   ```

   After this, every commit formats the C files you changed with the right
   clang-format version automatically. To format manually instead:
   `pip install clang-format==16.0.6`, then `clang-format -i <files>`. If the
   format check fails on your pull request, don't worry: I can fix it before
   merging.
6. Keep your own board out of it: pins go in `ports/*/board.conf` (ignored by git),
   never in code. New hardware keys go, commented out, into both `board.example.conf` files.

Names, code style, commands, board keys and release files follow the
[standards](https://github.com/tinydesk-project/tinydesk-shell/blob/main/STANDARDS.md), kept in TinyDesk Shell
(`third_party/tdsh/STANDARDS.md`) for both repositories.

Shell changes are made and pushed in `tinydesk-shell` first; then the new
submodule commit is recorded here.

Security problems: report them privately via GitHub (Security → Report a vulnerability).
Contributions are released under the MIT licence.
