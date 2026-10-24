#if defined(__WIIU__) || defined(HX_CAFE)

#include "WiiUInput.h"
#include "WiiUGamepad.h"
#include "WiiUJoystick.h"
#include "WiiUTouch.h"

#include <coreinit/debug.h>
#include <vpad/input.h>

namespace lime {

    void WiiUInput::Init() {
        VPADInit();

        WiiUGamepad::Init();
        WiiUJoystick::Init();
        WiiUTouch::Init();

        OSReport("[WiiUInput] Custom backend for Wii U input initialized\n");
    }

    void WiiUInput::Rumble(double lowFrequency, double highFrequency, int durationMs) {
        WiiUGamepad::Rumble(lowFrequency, highFrequency, durationMs);
    }

    void WiiUInput::Update() {
        VPADStatus vpad;
        VPADReadError vpadError;
        VPADRead(VPAD_CHAN_0, &vpad, 1, &vpadError);

        if (vpadError == VPAD_READ_SUCCESS) {
            WiiUGamepad::Update(&vpad);

            WiiUJoystick::Update(&vpad);

            WiiUTouch::Update(&vpad);
        }
    }

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)