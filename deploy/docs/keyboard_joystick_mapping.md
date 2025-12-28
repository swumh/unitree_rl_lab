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

### Method 3: Using Type Traits and Reflection (C++17+)

For compile-time offset calculation:

```cpp
template<typename T>
constexpr size_t get_value_offset() {
    return offsetof(T, value);
}

template<typename T>
static void setAxisValue(T& axis_obj, float val) {
    constexpr size_t offset = get_value_offset<T>();
    float* value_ptr = reinterpret_cast<float*>(
        reinterpret_cast<uint8_t*>(&axis_obj) + offset
    );
    *value_ptr = val;
}
```

### Method 4: Runtime Structure Inspection (Debug/Development Only)

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

Based on the Unitree SDK structure, the proper implementation should be:

```cpp
static void mapKeyboardToJoystick()
{
    if(!keyboard || !lowstate) return;

    auto& joy = lowstate->joystick;
    std::string key = keyboard->key();
    
    // ... existing key tracking code ...
    
    // Calculate joystick values
    float lx_val = 0.0f;
    float ly_val = 0.0f;
    // ... calculation logic ...
    
    // Proper way to set axis values
    // Option 1: Direct access (if field is public)
    joy.lx.value = lx_val;
    joy.ly.value = ly_val;
    
    // Option 2: Using offsetof (if you need to be explicit)
    // constexpr size_t value_offset = offsetof(decltype(joy.lx), value);
    // *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(&joy.lx) + value_offset) = lx_val;
    // *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(&joy.ly) + value_offset) = ly_val;
    
    // Set the pressed flags
    joy.lx.pressed = (lx_val != 0.0f);
    joy.ly.pressed = (ly_val != 0.0f);
    
    // ... rest of the code ...
}
```

## Why the Brute Force Approach is Bad

The brute-force approach writes to offsets 4-60:
1. **Unsafe**: May corrupt adjacent memory
2. **Non-portable**: Structure layout can change between compilers/platforms
3. **Inefficient**: Writes multiple times unnecessarily
4. **Hard to maintain**: Breaks when structure changes

## Conclusion

Always use proper structure access methods:
1. Direct field access when possible
2. `offsetof` when you need compile-time offset
3. Never write to arbitrary memory offsets
