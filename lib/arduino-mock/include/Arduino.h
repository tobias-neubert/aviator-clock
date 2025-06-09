#pragma once
#include <map>
#include <MockSerial.h>


// Arduino pin‐state macros (you already had these)
#define HIGH 0x1
#define LOW  0x0

// Arduino pin‐mode macros
#define INPUT        0x0
#define OUTPUT       0x1
#define INPUT_PULLUP 0x2
#define INPUT_PULLDOWN 0x8

// your fakePinValues() & pin functions…
inline std::map<int,int>& fakePinValues() {
    static std::map<int,int> _m;
    return _m;
}

inline unsigned long millis() {
    return 0; // Just return a constant value
}

inline void pinMode(int pin, int mode) {
    // no-op or record mode if you like:
    // fakePinModes()[pin] = mode;
}

inline void digitalWrite(int pin, int value) {
    fakePinValues()[pin] = value;
}

inline int digitalRead(int pin) {
    if (fakePinValues().count(pin)) {
        return fakePinValues()[pin];
    }
    return LOW;
}

inline void delay(uint32_t /*ms*/) {
    // no-op in tests (or record ms if you want to assert on timing)
}
