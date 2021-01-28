#include "Particle.h"

#include "DebounceSwitchRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

DebounceSwitch debounce;

void interruptHandler();
DebounceSwitchState *notifyHandler;

void setup() {
    waitFor(Serial.isConnected, 15000);

    debounce.setup();

    pinMode(D2, INPUT_PULLUP);
    attachInterrupt(D2, interruptHandler, CHANGE);

    notifyHandler = debounce.addNotifySwitch(DebounceSwitchStyle::PRESS_LOW, [](DebounceSwitchState *switchState, void *) {
        Log.info("pin=%d state=%s", switchState->getPin(), switchState->getPressStateName());
    });

    // Notify of initial state
    notifyHandler->notify(pinReadFast(D2));

}

void loop() {
}

void interruptHandler() {
    // It's safe to call this from an ISR
    if (notifyHandler) {
        notifyHandler->notify(pinReadFast(D2));
    }
}
