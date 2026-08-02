Quick iPlug2 + Xcode setup notes

1. Clone iPlug2 (https://github.com/iPlug2/iPlug2) next to this repo or anywhere convenient.
2. Set `IPLUG2_DIR` when running CMake or edit `iplug2/CMakeLists.txt` to point to your iPlug2 checkout.
3. Generate an Xcode project:

```bash
mkdir build
cd build
cmake -G Xcode -DIPLUG2_DIR=/path/to/iPlug2 ..
```

4. Open the generated `.xcodeproj` in Xcode, select a scheme, and build.

Notes:
- You still need to wire the concrete iPlug2 adapter to `core/Display.h` and other interfaces.
- iOS builds require additional provisioning and signing; follow iPlug2 docs for iOS targets.
