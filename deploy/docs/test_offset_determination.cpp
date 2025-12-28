/**
 * @file test_offset_determination.cpp
 * @brief Test program to demonstrate proper offset determination for KeyBase structures
 * 
 * This program shows how to properly determine the offset of fields in C++ structures
 * instead of using brute-force memory writing.
 * 
 * To compile (requires unitree_sdk2):
 * g++ -std=c++17 -I/usr/local/include test_offset_determination.cpp -o test_offset -lunitree_sdk2
 */

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <iomanip>

// Uncomment when unitree_sdk2 is available
// #include <unitree/dds_wrapper/common/unitree_joystick.hpp>

/**
 * Mock structure for demonstration (replace with actual SDK types when available)
 */
struct MockKeyBase {
    bool pressed;
    bool on_pressed;
    bool on_released;
    float pressed_time;
    float value;  // The field we want to access
};

/**
 * Method 1: Direct field access (Recommended)
 */
template<typename T>
void setAxisValue_Direct(T& axis, float val) {
    axis.value = val;
}

/**
 * Method 2: Using offsetof macro (Explicit offset determination)
 */
template<typename T>
void setAxisValue_Offsetof(T& axis, float val) {
    constexpr size_t value_offset = offsetof(T, value);
    float* value_ptr = reinterpret_cast<float*>(
        reinterpret_cast<uint8_t*>(&axis) + value_offset
    );
    *value_ptr = val;
}

/**
 * Method 3: Runtime offset discovery (for debugging)
 */
template<typename T>
void printStructureLayout() {
    std::cout << "\n=== Structure Layout for " << typeid(T).name() << " ===" << std::endl;
    std::cout << "Total size: " << sizeof(T) << " bytes" << std::endl;
    
    // Print offsets of each field
    std::cout << std::left << std::setw(20) << "Field" << std::setw(10) << "Offset" << "Size" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    std::cout << std::setw(20) << "pressed" 
              << std::setw(10) << offsetof(T, pressed) 
              << sizeof(bool) << std::endl;
    
    std::cout << std::setw(20) << "on_pressed" 
              << std::setw(10) << offsetof(T, on_pressed) 
              << sizeof(bool) << std::endl;
    
    std::cout << std::setw(20) << "on_released" 
              << std::setw(10) << offsetof(T, on_released) 
              << sizeof(bool) << std::endl;
    
    std::cout << std::setw(20) << "pressed_time" 
              << std::setw(10) << offsetof(T, pressed_time) 
              << sizeof(float) << std::endl;
    
    std::cout << std::setw(20) << "value" 
              << std::setw(10) << offsetof(T, value) 
              << sizeof(float) << std::endl;
    
    std::cout << std::endl;
}

/**
 * Demonstrate why brute-force is bad
 */
void demonstrateBruteForceProblems() {
    std::cout << "\n=== Why Brute Force is Dangerous ===" << std::endl;
    
    MockKeyBase axis;
    axis.pressed = false;
    axis.on_pressed = false;
    axis.on_released = false;
    axis.pressed_time = 0.0f;
    axis.value = 0.0f;
    
    std::cout << "Before brute force:" << std::endl;
    std::cout << "  value: " << axis.value << std::endl;
    std::cout << "  pressed_time: " << axis.pressed_time << std::endl;
    
    // Brute force approach (BAD!)
    uint8_t* base_ptr = reinterpret_cast<uint8_t*>(&axis);
    for (int offset = 4; offset <= 60; offset += 4) {
        float* target = reinterpret_cast<float*>(base_ptr + offset);
        *target = 1.23f; // This will corrupt memory!
    }
    
    std::cout << "After brute force:" << std::endl;
    std::cout << "  value: " << axis.value << std::endl;
    std::cout << "  pressed_time: " << axis.pressed_time << " (CORRUPTED!)" << std::endl;
    std::cout << "\nNotice: pressed_time was also overwritten!" << std::endl;
}

int main() {
    std::cout << "=== Offset Determination Test ===" << std::endl;
    std::cout << "This demonstrates the proper way to determine field offsets" << std::endl;
    
    // Print structure layout
    printStructureLayout<MockKeyBase>();
    
    // Test Method 1: Direct access
    {
        MockKeyBase axis;
        setAxisValue_Direct(axis, 1.5f);
        std::cout << "Method 1 (Direct): axis.value = " << axis.value << std::endl;
    }
    
    // Test Method 2: Using offsetof
    {
        MockKeyBase axis;
        setAxisValue_Offsetof(axis, 2.5f);
        std::cout << "Method 2 (Offsetof): axis.value = " << axis.value << std::endl;
    }
    
    // Demonstrate brute force problems
    demonstrateBruteForceProblems();
    
    std::cout << "\n=== Conclusion ===" << std::endl;
    std::cout << "Always use one of these methods:" << std::endl;
    std::cout << "1. Direct field access: axis.value = val" << std::endl;
    std::cout << "2. offsetof macro: offsetof(T, value)" << std::endl;
    std::cout << "Never use brute-force memory writing!" << std::endl;
    
    return 0;
}
