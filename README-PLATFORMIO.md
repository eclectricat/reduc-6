PlatformIO build

1) Install PlatformIO (VSCode extension or CLI)

2) From project root run:

```bash
# build for Teensy 4.1 (adjust env/board in platformio.ini to match your board)
platformio run -e teensy41
```

3) To upload (requires Teensy in bootloader/auto):

```bash
platformio run -e teensy41 -t upload
```

Notes:
- `src_dir = .` in `platformio.ini` lets PlatformIO compile the existing `.ino` and folders in project root.
- If you prefer a conventional layout, move `CookieBox.ino` into `src/main.ino` and remove `src_dir` from `platformio.ini`.
- If the build needs extra include paths, add `build_flags = -I core -I teensy` under the env section.
