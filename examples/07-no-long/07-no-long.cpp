#include "Particle.h"

#include "DebounceSwitchRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

void setup() {
    // Comment this out to wait for USB serial connections to see more debug logs
    // waitFor(Serial.isConnected, 15000);

    DebounceSwitch::getInstance()->withNoLongPress().setup();

    DebounceSwitch::getInstance()->addSwitch(D3, DebounceSwitchStyle::PRESS_LOW_PULLUP, [](DebounceSwitchState *switchState, void *) {
        Log.info("pin=%d state=%s", switchState->getPin(), switchState->getPressStateName());
        if (switchState->getPressState() == DebouncePressState::TAP) {
            Log.info("%d taps", switchState->getTapCount());
        }
    });
}

void loop() {
}