#if defined(__WIIU__) || defined(HX_CAFE)

#include "WiiUJoystick.h"
#include <ui/JoystickEvent.h>

namespace lime {

    void WiiUJoystick::Init() {
        JoystickEvent connectEvent;
        connectEvent.id = 0;
        connectEvent.type = JOYSTICK_CONNECT;
        JoystickEvent::Dispatch(&connectEvent);
    }

    void WiiUJoystick::Update(const VPADStatus* vpad) {
        for (int i = 0; i < 32; i++) {
            uint32_t mask = 1 << i;

            if (vpad->trigger & mask) {
                JoystickEvent event;
                event.id = 0;
                event.type = JOYSTICK_BUTTON_DOWN;
                event.index = i;
                JoystickEvent::Dispatch(&event);
            }

            if (vpad->release & mask) {
                JoystickEvent event;
                event.id = 0;
                event.type = JOYSTICK_BUTTON_UP;
                event.index = i;
                JoystickEvent::Dispatch(&event);
            }
        }

        float stickAxes[4] = {
            vpad->leftStick.x,
            -vpad->leftStick.y,
            vpad->rightStick.x,
            -vpad->rightStick.y
        };

        for (int i = 0; i < 4; i++) {
            JoystickEvent event;
            event.id = 0;
            event.type = JOYSTICK_AXIS_MOVE;
            event.index = i;
            event.x = stickAxes[i];
            JoystickEvent::Dispatch(&event);
        }

        float sensorAxes[6] = {
            vpad->accelorometer.acc.x,
            vpad->accelorometer.acc.y,
            vpad->accelorometer.acc.z,
            vpad->gyro.x,
            vpad->gyro.y,
            vpad->gyro.z
        };

        for (int s = 0; s < 6; s++) {
            JoystickEvent event;
            event.id = 0;
            event.type = JOYSTICK_AXIS_MOVE;
            event.index = 4 + s;
            event.x = sensorAxes[s];
            JoystickEvent::Dispatch(&event);
        }
    }

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)