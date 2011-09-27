#include "lynx.h"

class CLynxSys
{
public:
    static uint32_t GetTicks();
    static void GetMouseDelta(int* dx, int* dy);
    static bool MouseLeftDown();
    static bool MouseRightDown();
    static bool MouseMiddleDown();

    // if you only call GetKeyState with
    // key = 0, you get a map of currently pressed keys
    // if you set a value for key > 0, the map will be changed
    // according to keydown and keyup.
    // keystate[key] is 0 if not pressed.
    // keystate[key] is 1 if pressed once.
    // keystate[key] is > 1 if auto-key repeat is active.
    static uint8_t* GetKeyState(unsigned int key=0, bool keydown=false, bool keyup=false);
};
