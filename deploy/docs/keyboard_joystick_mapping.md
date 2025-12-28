# Keyboard to Joystick Mapping - Proper Implementation

## Problem

The current implementation in FSMState.h uses a "brute force" approach to set joystick axis values by writing to memory offsets 4-60. This is unsafe and non-portable.

## Question: 如何确定偏移量 (How to determine the offset?)

There are several proper ways to determine the offset of a structure field in C++:

### Method 1: Direct Field Access (Recommended)

If the field is public, access it directly:

```cpp
joy.lx.value = lx_val;  // Direct access
joy.ly.value = ly_val;
```

### Method 2: Using `offsetof` Macro

For determining the exact offset programmatically:

```cpp
#include <cstddef>

template<typename T>
static void setAxisValue(T& axis_obj, float val) {
    // Calculate the offset of the 'value' field
    size_t offset = offsetof(T, value);
    
    // Access the field through pointer arithmetic
    float* value_ptr = reinterpret_cast<float*>(
        reinterpret_cast<uint8_t*>(&axis_obj) + offset
    );
    *value_ptr = val;
}
```

**Test Results**: Using our mock KeyBase structure:
```
Structure size: 12 bytes
Field               Offset    Size
----------------------------------------
pressed             0         1
on_pressed          1         1
on_released         2         1
pressed_time        4         4
value               8         4
```

The `value` field is at **offset 8**, not somewhere between 4-60! The brute force approach writes to many wrong locations.

### Method 3: Runtime Structure Inspection (Debug/Development Only)

To discover the structure layout at runtime:

```cpp
#include <iostream>
#include <cstddef>

template<typename T>
void printStructureLayout() {
    T obj{};
    std::cout << "Structure size: " << sizeof(T) << " bytes\n";
    std::cout << "Offset of pressed: " << offsetof(T, pressed) << "\n";
    std::cout << "Offset of on_pressed: " << offsetof(T, on_pressed) << "\n";
    std::cout << "Offset of on_released: " << offsetof(T, on_released) << "\n";
    std::cout << "Offset of pressed_time: " << offsetof(T, pressed_time) << "\n";
    std::cout << "Offset of value: " << offsetof(T, value) << "\n";
}
```

## Recommended Solution

The proper implementation is now in `deploy/include/FSM/FSMState.h`:

```cpp
/**
 * @brief Properly set axis value using direct field access or offsetof
 */
template<typename T>
static void setAxisValue(T& axis_obj, float val) {
    // Method 1: Direct access (preferred if 'value' field is public)
    axis_obj.value = val;
    
    // Method 2: Using offsetof (if you need explicit offset calculation)
    // constexpr size_t value_offset = offsetof(T, value);
    // float* value_ptr = reinterpret_cast<float*>(
    //     reinterpret_cast<uint8_t*>(&axis_obj) + value_offset
    // );
    // *value_ptr = val;
}

static void mapKeyboardToJoystick()
{
    if(!keyboard || !lowstate) return;
    auto& joy = lowstate->joystick;
    
    // ... key tracking code ...
    
    // Calculate joystick values from WASD/arrow keys
    float lx_val = 0.0f;
    float ly_val = 0.0f;
    // ... calculation logic ...
    
    // Set axis values properly
    setAxisValue(joy.lx, lx_val);
    setAxisValue(joy.ly, ly_val);
    joy.lx.pressed = (lx_val != 0.0f);
    joy.ly.pressed = (ly_val != 0.0f);
    
    // ... button mapping ...
}
```

## Why the Brute Force Approach is Bad

The brute-force approach writes to offsets 4-60:
1. **Unsafe**: May corrupt adjacent memory (e.g., `pressed_time` at offset 4)
2. **Non-portable**: Structure layout can change between compilers/platforms
3. **Inefficient**: Writes multiple times unnecessarily (14+ writes instead of 1)
4. **Hard to maintain**: Breaks when structure changes
5. **Can cause crashes**: Our test showed stack smashing when writing beyond structure bounds

### Demonstration

```cpp
// BAD: Brute force (writes to offsets 4, 8, 12, ..., 60)
for (int offset = 4; offset <= 60; offset += 4) {
    float* target = reinterpret_cast<float*>(base_ptr + offset);
    *target = val;  // Corrupts pressed_time (offset 4) AND value (offset 8) AND beyond!
}

// GOOD: Direct access
axis.value = val;  // Only writes to offset 8
```

## Test Program

A test program is provided in `deploy/docs/test_offset_determination.cpp` that demonstrates:
- How to print structure layout using `offsetof`
- Why brute force is dangerous (shows memory corruption)
- Proper methods for field access

Compile and run:
```bash
cd deploy/docs
g++ -std=c++17 test_offset_determination.cpp -o test_offset
./test_offset
```

## Conclusion

Always use proper structure access methods:
1. **Direct field access** when possible: `axis.value = val`
2. **`offsetof` macro** when you need compile-time offset: `offsetof(T, value)`
3. **Never write to arbitrary memory offsets** - it's unsafe and can corrupt memory
