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
#include <cmath>

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
        
        registered_checks.emplace_back(
            std::make_pair(
                []()->bool{ return lowstate->isTimeout(); },
                FSMStringMap.right.at("Passive")
            )
        );
    }

    static void mapKeyboardToJoystick()
    {
        if(!keyboard || !lowstate) return;

        auto& joy = lowstate->joystick;

        static bool joystick_configured = false;
        if(!joystick_configured) {
            // 键盘输入是数字的，不需要平滑和死区
            joy.lx.smooth = 1.0f;     // 禁用平滑
            joy.ly.smooth = 1.0f;
            joy.rx.smooth = 1.0f;
            joy.ry.smooth = 1.0f;
            joy.LT.smooth = 1.0f;
            joy.RT.smooth = 1.0f;
            
            joy.lx.deadzone = 0.0f;   // 禁用死区
            joy.ly.deadzone = 0.0f;
            joy.rx.deadzone = 0.0f;
            joy.ry.deadzone = 0.0f;
            joy.LT.deadzone = 0.0f;
            joy.RT.deadzone = 0.0f;
            
            joystick_configured = true;
            spdlog::info("Joystick configured for keyboard input (smooth=1.0, deadzone=0.0)");
        }

        std::string key = keyboard->key();
        
        // =========================================================
        // 1. 按键状态管理
        // =========================================================
        static std::set<std::string> pressed_keys;
        static std::map<std::string, std::chrono::steady_clock::time_point> key_press_times;
        auto now = std::chrono::steady_clock::now();
        
        // 记录新按键
        if(!key.empty()) {
            std::string normalized_key = key;
            // 转换为小写
            if(normalized_key.length() == 1) {
                normalized_key[0] = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(normalized_key[0]))
                );
            }
            pressed_keys.insert(normalized_key);
            key_press_times[normalized_key] = now;
        } else {
            // 清除超时的按键 (300ms)
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
        
        // =========================================================
        // 2. 辅助函数：检查按键是否被按下
        // =========================================================
        auto isKeyPressed = [&](const std::string& k) -> bool {
            // 直接匹配
            if(pressed_keys.count(k)) return true;
            
            // WASD 映射到方向键
            if(k == "up" && (pressed_keys.count("w") || pressed_keys.count("up"))) 
                return true;
            if(k == "down" && (pressed_keys.count("s") || pressed_keys.count("down"))) 
                return true;
            if(k == "left" && (pressed_keys.count("a") || pressed_keys.count("left"))) 
                return true;
            if(k == "right" && (pressed_keys.count("d") || pressed_keys.count("right"))) 
                return true;
                
            return false;
        };

        // =========================================================
        // 3. 计算摇杆数值 (WASD / 方向键)
        // =========================================================
        bool up = isKeyPressed("up");
        bool down = isKeyPressed("down");
        bool left = isKeyPressed("left");
        bool right = isKeyPressed("right");

        float lx_val = 0.0f;
        float ly_val = 0.0f;
        const float MAX_VAL = 1.0f;

        if(up)    ly_val += MAX_VAL;
        if(down)  ly_val -= MAX_VAL;
        if(left)  lx_val -= MAX_VAL;
        if(right) lx_val += MAX_VAL;

        // 对角线归一化（保持速度一致）
        if(lx_val != 0.0f && ly_val != 0.0f) {
            float magnitude = std::sqrt(lx_val * lx_val + ly_val * ly_val);
            if(magnitude > 0.0f) {
                lx_val /= magnitude;
                ly_val /= magnitude;
            }
        }

        // =========================================================
        // 4. ✅ 使用正确的接口更新摇杆值
        // =========================================================
        // Axis::operator()(const float&) 会自动处理：
        // - 死区 (deadzone)
        // - 平滑 (smooth)
        // - 更新 pressed/on_pressed/on_released 状态
        // - 更新 pressed_time
        joy.lx(lx_val);
        joy.ly(ly_val);
        

        // =========================================================
        // 5. ✅ 映射功能键
        // =========================================================
        // Button::operator()(const int&) 会自动处理：
        // - 更新 pressed/on_pressed/on_released 状态
        // - 更新 click_cnt (双击检测)
        // - 更新 pressed_time
        
        // 主要按钮 (ABXY)
        joy.A(isKeyPressed("j") ? 1 : 0);
        joy.B(isKeyPressed("k") ? 1 : 0);
        joy.X(isKeyPressed("u") ? 1 : 0);
        joy.Y(isKeyPressed("i") ? 1 : 0);
        
        // 肩键 (LB/RB)
        joy.LB(isKeyPressed("q") ? 1 : 0);
        joy.RB(isKeyPressed("e") ? 1 : 0);
        
        // 扳机键 (LT/RT) - Axis 类型，传 float
        joy.LT(isKeyPressed("z") ? 1.0f : 0.0f);
        joy.RT(isKeyPressed("c") ? 1.0f : 0.0f);
        
        // 系统按钮
        joy.start(isKeyPressed(" ") || isKeyPressed("space") ? 1 : 0);
        joy.back(isKeyPressed("backspace") || isKeyPressed("escape") ? 1 : 0);
        
        // 摇杆按键 (LS/RS) - 可选
        joy.LS(isKeyPressed("v") ? 1 : 0);
        joy.RS(isKeyPressed("n") ? 1 : 0);
        
        
        // F1/F2 功能键
        joy.F1(isKeyPressed("f1") ? 1 : 0);
        joy.F2(isKeyPressed("f2") ? 1 : 0);

        // =========================================================
        // 6. Debug 日志
        // =========================================================
        // 摇杆移动日志
        // if (lx_val != 0.0f || ly_val != 0.0f) {
        //     static int log_cnt = 0;
        //     if (log_cnt++ % 20 == 0) {
        //         // 使用 operator()() 读取实际值
        //         spdlog::info("Joystick -> LX: {:.2f} (actual: {:.2f}), LY: {:.2f} (actual: {:.2f})", 
        //                     lx_val, joy.lx(), 
        //                     ly_val, joy.ly());
        //     }
        // }
        
        // 按钮事件日志
        // if (joy.A.on_pressed) {
        //     spdlog::info("Button A pressed! Click count: {}", joy.A.click_cnt);
        // }
        // if (joy.B.on_pressed) {
        //     spdlog::info("Button B pressed! Click count: {}", joy.B.click_cnt);
        // }
        // if (joy.start.on_pressed) {
        //     spdlog::info("Start button pressed!");
        // }
        
        // 长按检测示例
        if (joy.LT.pressed && joy.LT.pressed_time > 2.0f) {
            static bool long_press_logged = false;
            if (!long_press_logged) {
                spdlog::info("LT long press detected! Held for {:.2f}s", joy.LT.pressed_time);
                long_press_logged = true;
            }
        } else {
            static bool long_press_logged = false;
            long_press_logged = false;
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