#ifndef __DEBOUNCESWITCHRK_H
#define __DEBOUNCESWITCHRK_H

#include "Particle.h"

#include <vector>


enum class DebounceSwitchStyle {
    PRESS_LOW,
    PRESS_HIGH,
    PRESS_LOW_PULLUP,
    PRESS_HIGH_PULLDOWN,
    TOGGLE,
    TOGGLE_PULLDOWN,
    TOGGLE_PULLUP,
};

enum class DebouncePressState {
    NOT_PRESSED,
    PRESS_START,
    PROGRESS,
    SHORT,
    LONG,
    VERY_LONG,
    WAIT_RELEASE,
    RELEASED,
    TAP_1,
    TAP_2,
    TAP_3,
    TAP_MANY,
    TOGGLE_START,
    TOGGLE_LOW,
    TOGGLE_HIGH
};

struct DebounceStateChange {
    bool signal;
    unsigned long ms;
};

class DebounceSwitchState {

public:
    DebounceSwitchState(pin_t pin, DebounceSwitchStyle style, std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context, std::function<bool(DebounceSwitchState *switchState, void *pollContext)> pollCallback, void *pollContext);
    virtual ~DebounceSwitchState();
    
    bool poll();
    void notify(bool signal);
    void run();

    bool isPressed() const;

    void setPressState(DebouncePressState pressState, bool callCallback);
    bool processSwitchState(DebounceStateChange *stateChange);
    DebouncePressState getPressState() const { return pressState; };
    const char *getPressStateName() const { return getPressStateName(pressState); };

    void checkDebounce();

    pin_t getPin() const { return pin; };

    static DebouncePressState signalToPressState(bool signal);

    static const char *getPressStateName(DebouncePressState pressState);

    static const size_t CHANGE_QUEUE_SIZE = 16;

protected:
    pin_t pin;
    DebounceSwitchStyle style;
    std::function<void(DebounceSwitchState *switchState, void *context)> callback;
    void *context;
    std::function<bool(DebounceSwitchState *switchState, void *context)> pollCallback;
    void *pollContext;

    DebouncePressState pressState = DebouncePressState::NOT_PRESSED;
    bool lastSignal = false;
    unsigned long pressMs = 0;
    unsigned long releaseMs = 0;
    int sequenceCount = 0;

    unsigned long debounceLastSameMs = 0;
    bool debouncedLastSignal = false;
};

class DebounceSwitch {
public:

public:
    DebounceSwitch();
    virtual ~DebounceSwitch();

    void setup();

    DebounceSwitchState *addSwitch(pin_t pin, DebounceSwitchStyle style, std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context = 0, std::function<bool(DebounceSwitchState *switchState, void *pollContext)> pollCallback = 0, void *pollContext = 0);

    DebounceSwitchState *addVirtualSwitch(DebounceSwitchStyle style, std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context, std::function<bool(DebounceSwitchState *switchState, void *pollContext)> pollCallback, void *pollContext) {
        return addSwitch(nextVirtualPin++, style, callback, context, pollCallback, pollContext);
    }


    DebounceSwitchState *addNotifySwitch(DebounceSwitchStyle style, std::function<void(DebounceSwitchState *switchState, void *context)> callback, void *context = 0);

    // TODO: withXXX() methods for setting debounceMs, interTapMs, etc.

    /**
     * @brief Adjust how often to poll the switches in milliseconds (default: 5)
     * 
     * @param ms Value to change to in milliseconds
     * 
     * You probably should not change this, because making it smaller
     * doesn't really improve performance, and making it longer can cause presses
     * to be missed. It cannot be larger than debounceMs.
     */
    DebounceSwitch &withCheckMs(unsigned long ms) { checkMs = ms; return *this; };

    unsigned long getCheckMs() const { return checkMs; };

    /**
     * @brief Set the debounce time in milliseconds (default: 20)
     * 
     * @param ms Value to change to in milliseconds. Reasonable values are 20 to 100.
     * 
     * If the switch is particularly bouncy you can make this larger, but increasing it
     * also adds to the latency for detecting button presses, so 20 is a good balance.
     */
    DebounceSwitch &withDebounceMs(unsigned long ms) { debounceMs = ms; return *this; };

    unsigned long getDebounceMs() const { return debounceMs; };


    /**
     * @brief Set the inter-tap time in milliseconds (default: 500)
     * 
     * @param ms Value to change to in milliseconds. Reasonable values are 250 to 2000.
     * 
     * When detecting multiple taps (TAP_1, TAP_2, TAP_3), there needs to be a timeout
     * from the last release before we know whether it was a single, double, or triple
     * tap. After the inter-tap timeout occurs, we definitively know that the last tap
     * has been made and any new tap will start over again at 1.
     * 
     * Making this longer makes it easier to double-tap, but it also delays the amount
     * of time until a TAP_1 is generated.
     * 
     * If you are not using double or triple tap, you can ignore this setting and only
     * respond to the SHORT state instead of TAP_1. SHORT is generated after each
     * release without consulting the inter-tap timeout.
     */
    DebounceSwitch &withInterTapMs(unsigned long ms) { interTapMs = ms; return *this; };


    unsigned long getInterTapMs() const { return interTapMs; };

    /**
     * @brief Set the long press duration in milliseconds (default: 3000, 3 seconds)
     * 
     * @param ms Value to change to in milliseconds. Reasonable values are 2000 to 6000.
     * 
     * If the button is held down longer than long press, but shorter than very long
     * press, then a LONG button state is generated. It also means a SHORT will not
     * be generated.
     */
    DebounceSwitch &withLongPressMs(unsigned long ms) { longPressMs = ms; return *this; };

    DebounceSwitch &withNoLongPress() { longPressMs = 0; return *this; };

    unsigned long getLongPressMs() const { return longPressMs; };

    /**
     * @brief Set the very long press duration in milliseconds (default: 10000, 10 seconds)
     * 
     * @param ms Value to change to in milliseconds. Reasonable values are 2000 to 15000. 
     * Also 0 is valid as very long press disabled; see withNoVeryLongPress()
     */
    DebounceSwitch &withVeryLongPressMs(unsigned long ms) { veryLongPressMs = ms; return *this; };

    /**
     * @brief Disables support for very long press
     * 
     * By disabling VERY_LONG the states: PRESS_START, LONG, and RELEASED will be sent to the
     * callback. PROGRESS and VERY_LONG will never be sent. This can simplify your code if
     * you only need two press states (SHORT and LONG).
     */
    DebounceSwitch &withNoVeryLongPress() { veryLongPressMs = 0; return *this; };

    unsigned long getVeryLongPressMs() const { return veryLongPressMs; };

    DebounceSwitch &withStackSize(size_t _stackSize) { stackSize = _stackSize; return *this; };

    static DebounceSwitch *getInstance() { return instance; };

    static const pin_t FIRST_VIRTUAL_PIN = 8192;

protected:
    void processSwitchState(DebounceSwitchState *state, bool signal);

    void threadFunction();
    static os_thread_return_t threadFunctionStatic(void* param);

    static bool gpioPoll(DebounceSwitchState *switchState, void *context);

    unsigned long checkMs = 5;
    unsigned long debounceMs = 20;
    unsigned long interTapMs = 500;
    unsigned long longPressMs = 3000;
    unsigned long veryLongPressMs = 10000;

    std::vector<DebounceSwitchState *> switchStates;
    size_t stackSize = 1024; // OS_THREAD_STACK_SIZE_DEFAULT == 3K
    unsigned long lastCheck = 0;
    pin_t nextVirtualPin = FIRST_VIRTUAL_PIN;

    static DebounceSwitch *instance;
};

#endif /* __DEBOUNCESWITCHRK_H */
