Teensy adapter skeleton

Files:
- `TeensyDisplay.h/.cpp` — implements `Display` using `LiquidCrystal`.
- `TeensySystem.h/.cpp` — minimal timing/log wrapper using Arduino `millis()`/`delay()`/`Serial`.

Usage (in your `CookieBox.ino`):

```cpp
#include "teensy/TeensyDisplay.h"

// existing LCD pins are already used to create `LiquidCrystal lcd(...);`
TeensyDisplay display(&lcd);

// pass pointer to core systems or inject where needed
// e.g. core code that needs a Display can accept `Display*` in setup()
```

Keep the sketch layout unchanged; place these files inside the sketch folder so Arduino IDE compiles them automatically.
