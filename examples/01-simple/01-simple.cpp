#include "Particle.h"

#include "DebounceSwitchRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

DebounceSwitch debounce;

void setup() {
    waitFor(Serial.isConnected, 15000);

    debounce.setup();

    debounce.addSwitch(D3, DebounceSwitchStyle::PRESS_LOW_PULLUP, [](DebounceSwitchState *switchState, void *) {
        Log.info("pin=%d state=%s", switchState->getPin(), switchState->getPressStateName());
    });
}

void loop() {
}