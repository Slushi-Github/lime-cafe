#if defined(__WIIU__) || defined(HX_CAFE)

#include "WiiUTouch.h"
#include <ui/TouchEvent.h>
#include <ui/MouseEvent.h>

namespace lime {

    bool WiiUTouch::touchWasDown = false;
    int WiiUTouch::lastPixelX = 0;
    int WiiUTouch::lastPixelY = 0;

    void WiiUTouch::Init() {
        touchWasDown = false;
        lastPixelX = 0;
        lastPixelY = 0;
    }

    void WiiUTouch::Update(const VPADStatus* vpad) {
        bool isTouched = (vpad->tpNormal.touched != 0);

        if (isTouched || touchWasDown) {
            VPADTouchData calibrated;
            VPADGetTPCalibratedPoint(VPAD_CHAN_0, &calibrated, &vpad->tpNormal);

            int pixelX = calibrated.x;
            int pixelY = calibrated.y;

            if (!isTouched && touchWasDown) {
                pixelX = lastPixelX;
                pixelY = lastPixelY;
            }

            TouchEvent touchEvent;
            touchEvent.id = 0;
            touchEvent.device = 0;

            touchEvent.x = (float)pixelX / 1280.0f;
            touchEvent.y = (float)pixelY / 720.0f;
            touchEvent.dx = (float)(pixelX - lastPixelX) / 1280.0f;
            touchEvent.dy = (float)(pixelY - lastPixelY) / 720.0f;
            touchEvent.pressure = isTouched ? 1.0f : 0.0f;

            if (isTouched && !touchWasDown) {
                touchEvent.type = TOUCH_START;
            } else if (!isTouched && touchWasDown) {
                touchEvent.type = TOUCH_END;
            } else {
                touchEvent.type = TOUCH_MOVE;
            }
            TouchEvent::Dispatch(&touchEvent);

            MouseEvent mouseEvent;
            mouseEvent.windowID = 0;
            mouseEvent.x = pixelX;
            mouseEvent.y = pixelY;
            mouseEvent.movementX = pixelX - lastPixelX;
            mouseEvent.movementY = pixelY - lastPixelY;
            mouseEvent.button = 0;

            if (isTouched && !touchWasDown) {
                mouseEvent.type = MOUSE_MOVE;
                MouseEvent::Dispatch(&mouseEvent);

                mouseEvent.type = MOUSE_DOWN;
                mouseEvent.clickCount = 1;
                MouseEvent::Dispatch(&mouseEvent);
            } 
            else if (!isTouched && touchWasDown) {
                mouseEvent.type = MOUSE_UP;
                mouseEvent.clickCount = 1;
                MouseEvent::Dispatch(&mouseEvent);
            } 
            else if (isTouched && touchWasDown) {
                mouseEvent.type = MOUSE_MOVE;
                MouseEvent::Dispatch(&mouseEvent);
            }

            lastPixelX = pixelX;
            lastPixelY = pixelY;
        }

        touchWasDown = isTouched;
    }

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)