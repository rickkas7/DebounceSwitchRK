#include "Particle.h"

#include "DebounceSwitchRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

DebounceSwitch debounce;

const pin_t TEST_PIN = D3;

void setup() {
    waitFor(Serial.isConnected, 15000);

    debounce.withNoVeryLongPress().setup();

    debounce.addSwitch(TEST_PIN, DebounceSwitchStyle::PRESS_LOW_PULLUP, [](DebounceSwitchState *switchState, void *) {
        Log.info("pin=%d state=%s", switchState->getPin(), switchState->getPressStateName());
    });
}

void loop() {
}