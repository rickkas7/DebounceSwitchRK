#include "Particle.h"

#include "DebounceSwitchRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

DebounceSwitch debounce;

void setup() {
    waitFor(Serial.isConnected, 15000);

    debounce.setup();

    // It's usually easier to use DebounceSwitchStyle::TOGGLE_PULLUP, but you can
    // do it separately if you prefer.
    pinMode(D2, INPUT_PULLUP);
    pinMode(D3, INPUT_PULLUP);

    debounce.addSwitch(D2, DebounceSwitchStyle::TOGGLE, [](DebounceSwitchState *switchState, void *) {
        Log.info("toggle state=%s", switchState->getPressStateName());
    });
    debounce.addSwitch(D3, DebounceSwitchStyle::PRESS_LOW, [](DebounceSwitchState *switchState, void *) {
        Log.info("button state=%s", switchState->getPressStateName());
    });

}

void loop() {
}