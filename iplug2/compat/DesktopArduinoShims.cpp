#include <cstddef>

// Host-build definitions for Arduino heap symbols used by freeram() helpers.
extern "C" {
char* __brkval = nullptr;
char _heap_end = 0;
}
