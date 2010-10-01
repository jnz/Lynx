#include "lynx.h"

class CLynxSys
{
public:
    static uint32_t GetTicks();
    static void GetMouseDelta(int* dx, int* dy);
    static uint8_t* GetKeyState();
    static bool MouseLeftDown();
    static bool MouseRightDown();
    static bool MouseMiddleDown();
};
