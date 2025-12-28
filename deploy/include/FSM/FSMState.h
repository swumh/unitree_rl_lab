#pragma once

#include "Types.h"
#include "param.h"
#include "FSM/BaseState.h"
#include "isaaclab/devices/keyboard/keyboard.h"
#include "unitree_joystick_dsl.hpp"
#include <chrono>
#include <set>
#include <map>
#include <cctype>
#include <cstring>
#include <cstddef>

class FSMState : public BaseState
{
public:
    FSMState(int state, std::string state_string) 
    : BaseState(state, state_string) 
    {
        spdlog::info("Initializing State_{} ...", state_string);

        auto transitions = param::config["FSM"][state_string]["transitions"];

        if(transitions)
        {
            auto transition_map = transitions.as<std::map<std::string, std::string>>();

            for(auto it = transition_map.begin(); it != transition_map.end(); ++it)
            {
                std::string target_fsm = it->first;
                if(!FSMStringMap.right.count(target_fsm))
                {
                    spdlog::warn("FSM State_'{}' not found in FSMStringMap!", target_fsm);
                    continue;
                }

                int fsm_id = FSMStringMap.right.at(target_fsm);

                std::string condition = it->second;
                unitree::common::dsl::Parser p(condition);
                auto ast = p.Parse();
                auto func = unitree::common::dsl::Compile(*ast);
                registered_checks.emplace_back(
                    std::make_pair(
                        [func]()->bool{ return func(FSMState::lowstate->joystick); },
                        fsm_id
                    )
                );
            }
        }

        // register for all states
        registered_checks.emplace_back(
            std::make_pair(
                []()->bool{ return lowstate->isTimeout(); },
                FSMStringMap.right.at("Passive")
            )
        );
    }

    /**
     * @brief Properly set axis value using offsetof to determine the field offset
     * 
     * This function demonstrates how to determine the offset of the 'value' field
     * in a structure using the offsetof macro, which is the proper and portable way
     * to access structure fields at specific offsets.
     * 
     * @tparam T The axis type (e.g., decltype(joystick.lx))
     * @param axis_obj Reference to the axis object
     * @param val The value to set
     */
    template<typename T>
    static void setAxisValue(T& axis_obj, float val) {
        // Method 1: Direct access (preferred if 'value' field is public)
        axis_obj.value = val;
        
        // Method 2: Using offsetof (if you need explicit offset calculation)
        // This is the proper way to determine the offset, not brute force memory writing
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
        std::string key = keyboard->key();
        
        // Track pressed keys with timeout
        static std::set<std::string> pressed_keys;
        static std::map<std::string, std::chrono::steady_clock::time_point> key_press_times;
        auto now = std::chrono::steady_clock::now();
        
        if(!key.empty()) {
            std::string normalized_key = key;
            if(normalized_key.length() == 1) {
                normalized_key[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(normalized_key[0])));
            }
            pressed_keys.insert(normalized_key);
            key_press_times[normalized_key] = now;
        } else {
            // Clear timed-out keys (300ms timeout)
            auto timeout = std::chrono::milliseconds(300);
            for(auto it = pressed_keys.begin(); it != pressed_keys.end();) {
                if(now - key_press_times[*it] > timeout) {
                    key_press_times.erase(*it);
                    it = pressed_keys.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        // Helper to check if a key is pressed
        auto isKeyPressed = [&](const std::string& k) -> bool {
            if(pressed_keys.count(k)) return true;
            // Support WASD and arrow keys for directional input
            if(k=="up" && (pressed_keys.count("w") || pressed_keys.count("up"))) return true;
            if(k=="down" && (pressed_keys.count("s") || pressed_keys.count("down"))) return true;
            if(k=="left" && (pressed_keys.count("a") || pressed_keys.count("left"))) return true;
            if(k=="right" && (pressed_keys.count("d") || pressed_keys.count("right"))) return true;
            return false;
        };

        // Calculate joystick axis values from WASD/arrow keys
        bool up = isKeyPressed("up");
        bool down = isKeyPressed("down");
        bool left = isKeyPressed("left");
        bool right = isKeyPressed("right");

        float lx_val = 0.0f;
        float ly_val = 0.0f;
        const float MAX_VAL = 1.0f;

        if(up) ly_val += MAX_VAL;
        if(down) ly_val -= MAX_VAL;
        if(left) lx_val -= MAX_VAL;
        if(right) lx_val += MAX_VAL;

        // Normalize diagonal movement
        if(lx_val != 0.0f && ly_val != 0.0f) {
            lx_val *= 0.707f; // 1/sqrt(2) for diagonal normalization
            ly_val *= 0.707f;
        }

        // Set axis values and pressed flags
        setAxisValue(joy.lx, lx_val);
        setAxisValue(joy.ly, ly_val);
        joy.lx.pressed = (lx_val != 0.0f);
        joy.ly.pressed = (ly_val != 0.0f);

        // Helper to update button states
        auto updateBtn = [&](auto& btn, bool p) {
            bool prev = btn.pressed;
            btn.pressed = p;
            btn.on_pressed = p && !prev;
            btn.on_released = !p && prev;
        };

        // Map keyboard buttons to joystick buttons
        updateBtn(joy.A, isKeyPressed("j"));
        updateBtn(joy.B, isKeyPressed("k"));
        updateBtn(joy.X, isKeyPressed("u"));
        updateBtn(joy.Y, isKeyPressed("i"));
        updateBtn(joy.LB, isKeyPressed("q"));
        updateBtn(joy.RB, isKeyPressed("e"));
        updateBtn(joy.LT, isKeyPressed("z"));
        updateBtn(joy.RT, isKeyPressed("c"));
        updateBtn(joy.start, isKeyPressed(" ") || isKeyPressed("space"));
        
        // Optional: Debug logging
        if (lx_val != 0.0f || ly_val != 0.0f) {
            static int log_cnt = 0;
            if (log_cnt++ % 20 == 0) {
                spdlog::debug("Keyboard->Joystick: LX={:.2f}, LY={:.2f}", lx_val, ly_val);
            }
        }
    }

    void pre_run()
    {
        lowstate->update();
        
        if(keyboard) {
            keyboard->update();
            mapKeyboardToJoystick();
        }
    }

    void post_run()
    {
        lowcmd->unlockAndPublish();
    }

    static std::unique_ptr<LowCmd_t> lowcmd;
    static std::shared_ptr<LowState_t> lowstate;
    static std::shared_ptr<Keyboard> keyboard;
};