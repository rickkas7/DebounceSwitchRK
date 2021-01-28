#include "Particle.h"

#include "DebounceSwitchRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

DebounceSwitch debounce;

void setup() {
    waitFor(Serial.isConnected, 15000);

    debounce.setup();

    debounce.addSwitch(D2, DebounceSwitchStyle::TOGGLE_PULLUP, [](DebounceSwitchState *switchState, void *) {
        Log.info("state=%s", switchState->getPressStateName());
    });
}

void loop() {
}