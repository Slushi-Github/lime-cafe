#ifndef LIME_WIIU_TOUCH_H
#define LIME_WIIU_TOUCH_H

#if defined(__WIIU__) || defined(HX_CAFE)

#include <vpad/input.h>

namespace lime {

    class WiiUTouch {
    public:
        static void Init();
        static void Update(const VPADStatus* vpad);

    private:
        static bool touchWasDown;
        static int lastPixelX;
        static int lastPixelY;
    };

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)
#endif // LIME_WIIU_TOUCH_H