#ifndef LIME_WIIU_JOYSTICK_H
#define LIME_WIIU_JOYSTICK_H

#if defined(__WIIU__) || defined(HX_CAFE)

#include <vpad/input.h>

namespace lime {

    class WiiUJoystick {
    public:
        static void Init();
        static void Update(const VPADStatus* vpad);
    };

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)
#endif // LIME_WIIU_JOYSTICK_H