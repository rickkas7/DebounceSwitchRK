# DebounceSwitchRK

Library for switch debouncing for Particle devices


## Library features

- Debounces both momentary (push button) and toggle switches.
- For push buttons, handles long press, very long press, and multi-tap (double-tap, triple-tap, etc.).
- Thread-based, so button changes are not lost even if loop() is blocked or delayed.
- Handles buttons connected to 3V3 or GND (active high or active low) with internal or external pull.
- Handles buttons connected to a GPIO pin.
- Also handles buttons connected to expanders, such as MCP23008 I2C GPIO expanders.
- Works with both polling (default) and notify-on-change (handy for I2C GPIO with interrupts).
- Many configurable parameters.


## General design



