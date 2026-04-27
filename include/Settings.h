#pragma once
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator> 
#include <map>    
#include <cctype> 

// RapidJSON headers
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/FileWriteStream.h>
#include <rapidjson/FileReadStream.h>

#include "InputManagerAPI.h"

constexpr uint32_t GAMEPAD_OFFSET = 266;

inline const char* gamepadKeyNames[] = {
        "None",
        "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
        "Start / Options", "Back / Share / Select", "LS / L3 (Left Stick)", "RS / R3 (Right Stick)",
        "LB / L1 (Left Bumper)", "RB / R1 (Right Bumper)",
        "LT / L2 (Left Trigger)", "RT / R2 (Right Trigger)",
        "A / Cross", "B / Circle", "X / Square", "Y / Triangle"
};

inline const int gamepadKeyIDs[] = {
    0,
    RE::BSWin32GamepadDevice::Keys::kUp + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kDown + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kLeft + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kRight + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kStart + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kBack + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kLeftThumb + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kRightThumb + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kLeftShoulder + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kRightShoulder + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kLeftTrigger + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kRightTrigger + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kA + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kB + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kX + GAMEPAD_OFFSET,
    RE::BSWin32GamepadDevice::Keys::kY + GAMEPAD_OFFSET
};

namespace Settings {
    extern int MouseModeActionID;
    extern bool IsMouseModeActive;
    extern bool HasInitializedInput;
    // Configurações do modo Mouse
    extern int MouseJoystick; // 0 = Left Stick, 1 = Right Stick
    extern int DirectionJoystick; // 0 = Left Stick, 1 = Right Stick
    extern int ButtonScrollUp;
    extern int ButtonScrollDown;
    extern int ButtonRightClick;
    extern int ButtonLeftClick;
    extern int ButtonScrollClick;
    extern int ButtonConfirm;
    extern int ButtonCancel;

    void Load();
    void Save();
    void RegisterMenu();
    void InitializeInput();
}