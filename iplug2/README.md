iPlug2 skeleton for CookieBox

This folder contains a minimal skeleton to integrate the CookieBox core into an iPlug2 plugin.

Steps to use:
1. Clone iPlug2 and set `IPLUG2_DIR` to the root of the iPlug2 repository.
2. Open `CMakeLists.txt` below and adjust `IPLUG2_DIR` if necessary.
3. Use CMake to generate an Xcode project, then open in Xcode and build.

Notes:
- `CookieBoxPlugin.cpp/.h` now provide a concrete iPlug plugin class that initializes `CookieBoxPluginApp`, attaches `CookieBoxUI`, and ticks it from `OnIdle()`.
- `PluginApp::processAudio()` now renders through shared `SynthEngine` logic, including optional direct part outs on channels 2..7.
- See `README_IPLUG2_SETUP.md` for more detailed setup steps.

UI module added
- `CookieBoxUI` now provides an actual plugin editor surface with:
	- 2x16 display panel (backed by the `Display` abstraction)
	- 4 draggable knobs
	- 10 momentary buttons
	- keyboard mapping `Q W E R T Z U I O P` -> button indices `0..9`

Integration sketch (in your iPlug2 plugin editor code)
```cpp
CookieBoxPluginApp app;
app.initialize();

CookieBoxUI ui(app);
ui.Attach(pGraphics);

// In your plugin/editor idle callback:
ui.OnIdle();
```

Button/key wiring follows the same index semantics as `CookieBox.ino`:
- Buttons `0..9` are passed to `Mode::pushButtonPressed/Released()`.
- Knob values are converted to 0..1023 and use the same pot lock/unlock threshold logic as Teensy.
