# Keyboard Input

## Hardware
- Legacy PS/2 controller; scancodes read from port 0x60.
- We use Set 1 scancodes and handle make (press) and break (release) codes.

## Driver behavior
- IRQ1 is unmasked; handler reads scancode from 0x60 on each interrupt.
- Shift state is tracked (left/right) and applied to ASCII conversion.
- Releases (break codes) are ignored except to update modifier state.

## Mapping
- `keymap_unshift` and `keymap_shift` map common keys to ASCII.
- Backspace (`\b`) and Enter (`\n`) are supported; Tab is `\t`.
- Extend tables if you need more keys (arrows, function keys require E0 handling).
