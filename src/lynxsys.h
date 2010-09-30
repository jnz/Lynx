#include "lynx.h"

class CLynxSys
{
public:
    static DWORD GetTicks();
    static void GetMouseDelta(int* dx, int* dy);
    static BYTE* GetKeyState();
    static bool MouseLeftDown();
    static bool MouseRightDown();
    static bool MouseMiddleDown();
};
