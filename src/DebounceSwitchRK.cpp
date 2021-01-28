#include "DebounceSwitchRK.h"

// [static] 
DebounceSwitch *DebounceSwitch::instance = 0;


DebounceSwitch::DebounceSwitch() {
    instance = this;
}
    
DebounceSwitch::~DebounceSwitch() {
	while(!switchStates.empty()) {
		DebounceSwitchState *state = switchStates.back();
		switchStates.pop_back();

		delete state;
	}

}

void DebounceSwitch::setup() {
    new Thread("debounce", threadFunctionStatic, this, OS_THREAD_PRIORITY_DEFAULT, stackSize);
}

void DebounceSwitch::threadFunction() {
    while(true) {
        if ((millis() - lastCheck) >= checkMs) {
            // Time to handle debounce
            for(auto it = switchStates.begin(); it != switchStates.end(); it++) {
                DebounceSwitchState *state = *it;
                state->checkDebounce();
            }
        }

        // Run state handlers
        for(auto it = switchStates.begin(); it != switchStates.end(); it++) {
            DebounceSwitchState *state = *it;
            state->run();
        }

        os_thread_yield();
    }
}

// [static] 
os_thread_return_t DebounceSwitch::threadFunctionStatic(void* param) {
    ((DebounceSwitch *)param)->threadFunction();
}

DebounceSwitchState *DebounceSwitch::addSwitch(pin_t pin, DebounceSwitchStyle style, std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context, std::function<bool(DebounceSwitchState *switchState, void *pollContext)> pollCallback, void *pollContext) {

    if (pollCallback == NULL) {
        pollCallback = gpioPoll;
        pollContext = 0;
    }
    if (pin < FIRST_VIRTUAL_PIN) {
        // Real GPIO
        switch(style) {
        case DebounceSwitchStyle::PRESS_LOW:
        case DebounceSwitchStyle::PRESS_HIGH:
        case DebounceSwitchStyle::TOGGLE:
            pinMode(pin, INPUT);
            break;

        case DebounceSwitchStyle::PRESS_LOW_PULLUP:
        case DebounceSwitchStyle::TOGGLE_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;

        case DebounceSwitchStyle::PRESS_HIGH_PULLDOWN:
        case DebounceSwitchStyle::TOGGLE_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN);
            break;
        }
    }

    DebounceSwitchState *state = new DebounceSwitchState(pin, style, callback, context, pollCallback, pollContext);
    if (!state) {
        return NULL;
    }

    switchStates.push_back(state);

    return state;
}

DebounceSwitchState *DebounceSwitch::addNotifySwitch(DebounceSwitchStyle style, std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context) {

    DebounceSwitchState *state = new DebounceSwitchState(nextVirtualPin++, style, callback, context, 0, 0);
    if (!state) {
        return NULL;
    }

    switch(style) {
    case DebounceSwitchStyle::PRESS_LOW:
    case DebounceSwitchStyle::PRESS_LOW_PULLUP:
        // Initial state should be high, not low
        state->notify(true);
        break;

    default:
        break;
    }

    switchStates.push_back(state);

    return state;
}



// [static]
bool DebounceSwitch::gpioPoll(DebounceSwitchState *switchState, void *context) {
    bool bResult = pinReadFast(switchState->getPin());

    return bResult;
}

DebounceSwitchState::DebounceSwitchState(pin_t pin, DebounceSwitchStyle style,
    std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context, 
    std::function<bool(DebounceSwitchState *switchState, void *pollContext)> pollCallback, void *pollContext) :
    pin(pin), style(style), callback(callback), context(context), 
    pollCallback(pollCallback), pollContext(pollContext)
{
    if (style == DebounceSwitchStyle::TOGGLE || 
        style == DebounceSwitchStyle::TOGGLE_PULLDOWN ||
        style == DebounceSwitchStyle::TOGGLE_PULLUP) {
        pressState = DebouncePressState::TOGGLE_START;
    }
}

DebounceSwitchState::~DebounceSwitchState() {
}

bool DebounceSwitchState::poll() {
    if (pollCallback != NULL) {
        return (pollCallback)(this, pollContext);
    } 
    else {
        return false;
    }
}

void DebounceSwitchState::notify(bool signal) {
    lastSignal = signal;
}

bool DebounceSwitchState::isPressed() const {
    if (style == DebounceSwitchStyle::PRESS_LOW || style == DebounceSwitchStyle::PRESS_LOW_PULLUP) {
        return !debouncedLastSignal;
    }
    else {
        return debouncedLastSignal;
    }
}


void DebounceSwitchState::setPressState(DebouncePressState _pressState, bool callCallback) {
    pressState = _pressState;
    if (callCallback && this->callback) {
        this->callback(this, context);
    }
}


void DebounceSwitchState::run() {
    switch(pressState) {
    case DebouncePressState::NOT_PRESSED:
        if (isPressed()) {
            // Pressed
            setPressState(DebouncePressState::PRESS_START, true);
            pressMs = millis();
            break;
        }
        break;

    case DebouncePressState::PRESS_START:
        if (DebounceSwitch::getInstance()->getLongPressMs() == 0) {
            // LONG press is disabled, which also implies very long
            // is disabled. Just generate SHORT and RELEASED.
            setPressState(DebouncePressState::SHORT, true);            
            setPressState(DebouncePressState::WAIT_RELEASE, false);            
            sequenceCount++;
            break;
        }
        if ((millis() - pressMs) >= DebounceSwitch::getInstance()->getLongPressMs()) {
            if (DebounceSwitch::getInstance()->getVeryLongPressMs() == 0) {
                // Very long press is not used, generate the LONG state and
                // wait. PROGRESS and VERY_LONG will not be generated.
                setPressState(DebouncePressState::LONG, true);            
                setPressState(DebouncePressState::WAIT_RELEASE, false);            
            }
            else {
                // Have been holding down the button long enough for a long or very long press
                setPressState(DebouncePressState::PROGRESS, true);
            }
            break;
        }
        if (!isPressed()) {
            // Released before a long press
            setPressState(DebouncePressState::SHORT, true);
            sequenceCount++;

            setPressState(DebouncePressState::RELEASED, true);      
            releaseMs = millis();      
            break;
        }
        break;

    case DebouncePressState::PROGRESS:
        if ((millis() - pressMs) >= DebounceSwitch::getInstance()->getVeryLongPressMs()) {
            setPressState(DebouncePressState::VERY_LONG, true);            
            break;
        }
        if (!isPressed()) {
            // Released. We now know that we have a long (not very long) press
            setPressState(DebouncePressState::LONG, true);            
            setPressState(DebouncePressState::RELEASED, true);            
            releaseMs = millis();      
            break;
        }
        break;

    case DebouncePressState::VERY_LONG:
    case DebouncePressState::WAIT_RELEASE:
        if (!isPressed()) {
            // Released. We already sent the VERY_LONG so don't do it here again.
            setPressState(DebouncePressState::RELEASED, true);
            releaseMs = millis();      
            break;
        }
        break;

    case DebouncePressState::RELEASED:
        if ((millis() - releaseMs) >= DebounceSwitch::getInstance()->getInterTapMs()) {
            // Send out the total number of taps
            switch(sequenceCount) {
            case 0:
                // This occurs with a single LONG or VERY_LONG press
                break;

            case 1:
                setPressState(DebouncePressState::TAP_1, true);
                break;

            case 2:
                setPressState(DebouncePressState::TAP_2, true);
                break;

            case 3:
                setPressState(DebouncePressState::TAP_3, true);
                break;

            default:
                setPressState(DebouncePressState::TAP_MANY, true);
                break;
            }

            setPressState(DebouncePressState::NOT_PRESSED, false);
            releaseMs = 0;
            sequenceCount = 0;
        }
        if (isPressed()) {
            // Pressed again before the getInterTapMs
            setPressState(DebouncePressState::PRESS_START, true);
            pressMs = millis();
            releaseMs = 0;
        }

        break;

    case DebouncePressState::TOGGLE_START:
        setPressState(signalToPressState(isPressed()), true);
        break;

    case DebouncePressState::TOGGLE_LOW:
    case DebouncePressState::TOGGLE_HIGH:
        if (signalToPressState(isPressed()) != pressState) {
            // Toggle state changed
            setPressState(signalToPressState(isPressed()), true);
        }
        break;


    }

}

void DebounceSwitchState::checkDebounce() {
    // Time to check switch state. We do this periodically for debouncing
    // purposes even when the actual value is fed by notification.
    if (pollCallback != NULL) {
        lastSignal = poll(); 
    }

    if (lastSignal == debouncedLastSignal) {
        debounceLastSameMs = millis();
    }
    else {
        // Signal state changed
        if ((millis() - debounceLastSameMs) >= DebounceSwitch::getInstance()->getDebounceMs()) {
            // Timer expired
            debouncedLastSignal = lastSignal;
        }
    }
}


// [static]
const char *DebounceSwitchState::getPressStateName(DebouncePressState pressState) {
    switch(pressState) {
    case DebouncePressState::NOT_PRESSED:
        return "NOT_PRESSED";

    case DebouncePressState::PRESS_START:
        return "PRESS_START";

    case DebouncePressState::PROGRESS:
        return "PROGRESS";

    case DebouncePressState::SHORT:
        return "SHORT";

    case DebouncePressState::LONG:
        return "LONG";

    case DebouncePressState::VERY_LONG:
        return "VERY_LONG";

    case DebouncePressState::WAIT_RELEASE:
        return "WAIT_RELEASE";

    case DebouncePressState::RELEASED:
        return "RELEASED";

    case DebouncePressState::TAP_1:
        return "TAP_1";

    case DebouncePressState::TAP_2:
        return "TAP_2";

    case DebouncePressState::TAP_3:
        return "TAP_3";

    case DebouncePressState::TAP_MANY:
        return "TAP_MANY";

    case DebouncePressState::TOGGLE_START:
        return "TOGGLE_START";

    case DebouncePressState::TOGGLE_LOW:
        return "TOGGLE_LOW";

    case DebouncePressState::TOGGLE_HIGH:
        return "TOGGLE_HIGH";

        // Unknown cases should cause a warning here
    }

    return "UNKNOWN";
}


// [static]
DebouncePressState DebounceSwitchState::signalToPressState(bool signal) {
    return signal ? DebouncePressState::TOGGLE_HIGH : DebouncePressState::TOGGLE_LOW;
}
