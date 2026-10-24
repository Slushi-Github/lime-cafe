#if defined(__WIIU__) || defined(HX_CAFE)

#include "WiiUGamepad.h"
#include <vpadbase/base.h>
#include <ui/GamepadEvent.h>
#include <string.h>

namespace lime {

    struct ButtonMapping {
        uint32_t vpadMask;
        int limeButtonId;
    };

    static const ButtonMapping gamepadMappings[] = {
        { VPAD_BUTTON_A,        0 },  // GAMEPAD_BUTTON_A
        { VPAD_BUTTON_B,        1 },  // GAMEPAD_BUTTON_B
        { VPAD_BUTTON_X,        2 },  // GAMEPAD_BUTTON_X
        { VPAD_BUTTON_Y,        3 },  // GAMEPAD_BUTTON_Y
        { VPAD_BUTTON_MINUS,    4 },  // GAMEPAD_BUTTON_BACK
        { VPAD_BUTTON_HOME,     5 },  // GAMEPAD_BUTTON_GUIDE
        { VPAD_BUTTON_PLUS,     6 },  // GAMEPAD_BUTTON_START
        { VPAD_BUTTON_STICK_L,  7 },  // GAMEPAD_BUTTON_LEFT_STICK (Click)
        { VPAD_BUTTON_STICK_R,  8 },  // GAMEPAD_BUTTON_RIGHT_STICK (Click)
        { VPAD_BUTTON_L,        9 },  // GAMEPAD_BUTTON_LEFT_SHOULDER
        { VPAD_BUTTON_R,        10 }, // GAMEPAD_BUTTON_RIGHT_SHOULDER
        { VPAD_BUTTON_UP,       11 }, // GAMEPAD_BUTTON_DPAD_UP
        { VPAD_BUTTON_DOWN,     12 }, // GAMEPAD_BUTTON_DPAD_DOWN
        { VPAD_BUTTON_LEFT,     13 }, // GAMEPAD_BUTTON_DPAD_LEFT
        { VPAD_BUTTON_RIGHT,    14 }, // GAMEPAD_BUTTON_DPAD_RIGHT
        { VPAD_BUTTON_ZL,       15 }, // Digital ZL
        { VPAD_BUTTON_ZR,       16 }, // Digital ZR
        { VPAD_BUTTON_TV,       17 }  // TV Button
    };
    static const int numGamepadMappings = sizeof(gamepadMappings) / sizeof(gamepadMappings[0]);

    void WiiUGamepad::Init() {
        GamepadEvent connectEvent;
        connectEvent.id = 0;
        connectEvent.type = GAMEPAD_CONNECT;
        GamepadEvent::Dispatch(&connectEvent);
    }

    void WiiUGamepad::Rumble(double lowFrequency, double highFrequency, int durationMs) {
        if (lowFrequency <= 0.0 && highFrequency <= 0.0) {
            VPADStopMotor(VPAD_CHAN_0);
            return;
        }

        uint8_t pattern[120];
        int length = durationMs / 8;
        if (length > 120) length = 120;
        if (length <= 0) length = 15;

        memset(pattern, 0xFF, length);
        VPADControlMotor(VPAD_CHAN_0, pattern, length);
    }

    void WiiUGamepad::Update(const VPADStatus* vpad) {
        for (int i = 0; i < numGamepadMappings; i++) {
            uint32_t mask = gamepadMappings[i].vpadMask;
            int btnId = gamepadMappings[i].limeButtonId;

            if (vpad->trigger & mask) {
                GamepadEvent event;
                event.id = 0;
                event.type = GAMEPAD_BUTTON_DOWN;
                event.button = btnId;
                GamepadEvent::Dispatch(&event);
            }

            if (vpad->release & mask) {
                GamepadEvent event;
                event.id = 0;
                event.type = GAMEPAD_BUTTON_UP;
                event.button = btnId;
                GamepadEvent::Dispatch(&event);
            }
        }

        float axes[6] = {
            vpad->leftStick.x,
            -vpad->leftStick.y,
            vpad->rightStick.x,
            -vpad->rightStick.y,
            (vpad->hold & VPAD_BUTTON_ZL) ? 1.0f : 0.0f,
            (vpad->hold & VPAD_BUTTON_ZR) ? 1.0f : 0.0f
        };

        for (int i = 0; i < 6; i++) {
            GamepadEvent event;
            event.id = 0;
            event.type = GAMEPAD_AXIS_MOVE;
            event.axis = i;
            event.axisValue = axes[i];
            GamepadEvent::Dispatch(&event);
        }
    }

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)