# Documentation for Keyboard to Joystick Mapping

This directory contains documentation and test programs for the keyboard-to-joystick mapping feature.

## Files

### keyboard_joystick_mapping.md
Comprehensive guide answering the question: "如何确定偏移量" (How to determine the offset?)

This document explains:
- Why brute-force memory writing is dangerous
- Proper methods to determine structure field offsets
- Recommended implementation using `offsetof` or direct field access
- Test results showing the actual offset of the `value` field (offset 8, not 4-60)

### test_offset_determination.cpp
Test program that demonstrates:
- How to use `offsetof` to find field offsets
- Why brute-force approach corrupts memory
- Proper vs improper field access methods

Compile and run:
```bash
cd deploy/docs
g++ -std=c++17 test_offset_determination.cpp -o test_offset
./test_offset
```

## Summary

The problem statement showed code using a brute-force approach that writes to memory offsets 4-60 to try to set the `value` field. This is:
- **Unsafe** - corrupts adjacent fields
- **Non-portable** - structure layout varies
- **Inefficient** - 14+ writes instead of 1

The proper solution uses either:
1. Direct field access: `axis.value = val`
2. `offsetof` macro: `offsetof(T, value)` returns 8 (not 4-60!)

The implementation is in `/deploy/include/FSM/FSMState.h` with:
- `setAxisValue()` template function using direct access
- `mapKeyboardToJoystick()` function mapping WASD/arrows to joystick axes
- Integration into the FSM lifecycle via `pre_run()`

## Keyboard Mapping

The implementation maps:
- **WASD/Arrow keys** → Left joystick (lx, ly)
- **J** → A button
- **K** → B button  
- **U** → X button
- **I** → Y button
- **Q** → LB button
- **E** → RB button
- **Z** → LT button
- **C** → RT button
- **Space** → Start button
