#ifndef LIME_WIIU_INPUT_H
#define LIME_WIIU_INPUT_H

#if defined(__WIIU__) || defined(HX_CAFE)

namespace lime {

    class WiiUInput {
    public:
        static void Init();
        static void Update();
        static void Rumble(double lowFrequency, double highFrequency, int durationMs);
    };

} // namespace lime

#endif // __WIIU__ || defined(HX_CAFE)
#endif // LIME_WIIU_INPUT_H