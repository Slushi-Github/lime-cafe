#ifndef LIME_WIIU_GAMEPAD_H
#define LIME_WIIU_GAMEPAD_H

#if defined(__WIIU__) || defined(HX_CAFE)

#include <vpad/input.h>

namespace lime {

    class WiiUGamepad {
    public:
        static void Init();
        static void Update(const VPADStatus* vpad);
        static void Rumble(double lowFrequency, double highFrequency, int durationMs);
    };

}

#endif
#endif