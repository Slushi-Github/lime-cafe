#if defined(__WIIU__) || defined(HX_CAFE)

#include "WiiUInput.h"

#include <string.h>
#include <math.h>

#include <coreinit/debug.h>
#include <vpad/input.h>
#include <vpadbase/base.h>
#include <whb/log.h>

#include <ui/GamepadEvent.h>
#include <ui/JoystickEvent.h>
#include <ui/TouchEvent.h>

namespace lime {

    bool WiiUInput::touchWasDown = false;

    void WiiUInput::Init() {
        VPADInit();

        GamepadEvent padConnect;
        padConnect.id = 0;
        padConnect.type = GAMEPAD_CONNECT;
        GamepadEvent::Dispatch(&padConnect);

        JoystickEvent joyConnect;
        joyConnect.id = 0;
        joyConnect.type = JOYSTICK_CONNECT;
        JoystickEvent::Dispatch(&joyConnect);

        OSReport("[WiiUInput] Custom backend for Wii U input initialized\n");
        WHBLogPrintf("[WiiUInput] Custom backend for Wii U input initialized");
    }

    void WiiUInput::Rumble(double lowFrequency, double highFrequency, int durationMs) {
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

    void WiiUInput::Update() {
        static VPADReadError lastLoggedError = (VPADReadError)999;

        VPADStatus vpad;
        VPADReadError vpadError;
        int32_t readCount = VPADRead(VPAD_CHAN_0, &vpad, 1, &vpadError);

        if (vpadError != VPAD_READ_SUCCESS) {
            if (vpadError != lastLoggedError) {
                WHBLogPrintf("[WiiUInput] VPADRead ERROR: %d (readCount: %d)", vpadError, readCount);
                lastLoggedError = vpadError;
            }
            return;
        }
        lastLoggedError = vpadError;


        for (int i = 0; i < 32; i++) {
            uint32_t mask = 1 << i;
            if (vpad.trigger & mask) {
                GamepadEvent padBtn;
                padBtn.id = 0;
                padBtn.type = GAMEPAD_BUTTON_DOWN;
                padBtn.button = i;
                GamepadEvent::Dispatch(&padBtn);

                JoystickEvent joyBtn;
                joyBtn.id = 0;
                joyBtn.type = JOYSTICK_BUTTON_DOWN;
                joyBtn.index = i;
                JoystickEvent::Dispatch(&joyBtn);
            }
            
            if (vpad.release & mask) {
                GamepadEvent padBtn;
                padBtn.id = 0;
                padBtn.type = GAMEPAD_BUTTON_UP;
                padBtn.button = i;
                GamepadEvent::Dispatch(&padBtn);

                JoystickEvent joyBtn;
                joyBtn.id = 0;
                joyBtn.type = JOYSTICK_BUTTON_UP;
                joyBtn.index = i;
                JoystickEvent::Dispatch(&joyBtn);
            }
        }

        float stickAxes[4] = {
            vpad.leftStick.x,
            -vpad.leftStick.y,
            vpad.rightStick.x,
            -vpad.rightStick.y
        };

        for (int axisIdx = 0; axisIdx < 4; axisIdx++) {
            GamepadEvent padAxis;
            padAxis.id = 0;
            padAxis.type = GAMEPAD_AXIS_MOVE;
            padAxis.axis = axisIdx;
            padAxis.axisValue = stickAxes[axisIdx];
            GamepadEvent::Dispatch(&padAxis);

            JoystickEvent joyAxis;
            joyAxis.id = 0;
            joyAxis.type = JOYSTICK_AXIS_MOVE;
            joyAxis.index = axisIdx;
            joyAxis.x = stickAxes[axisIdx];
            JoystickEvent::Dispatch(&joyAxis);
        }

        bool isTouched = (vpad.tpNormal.touched != 0);
        if (isTouched || touchWasDown) {
            VPADTouchData calibrated;
            VPADGetTPCalibratedPoint(VPAD_CHAN_0, &calibrated, &vpad.tpNormal);

            TouchEvent touchEvent;
            touchEvent.id = 0;
            touchEvent.device = 0;
            touchEvent.x = (float)calibrated.x / 1280.0f;
            touchEvent.y = (float)calibrated.y / 720.0f;
            touchEvent.dx = 0;
            touchEvent.dy = 0;
            touchEvent.pressure = isTouched ? 1.0f : 0.0f;

            if (isTouched && !touchWasDown) touchEvent.type = TOUCH_START;
            else if (!isTouched && touchWasDown) touchEvent.type = TOUCH_END;
            else touchEvent.type = TOUCH_MOVE;

            TouchEvent::Dispatch(&touchEvent);
        }
        touchWasDown = isTouched;
    }

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)