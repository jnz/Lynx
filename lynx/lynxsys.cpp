#include "lynx.h"
#include "lynxsys.h"
#include "SDL.h"
 
#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

DWORD CLynxSys::GetTicks()
{
    return SDL_GetTicks();
}

void CLynxSys::GetMouseDelta(int* dx, int* dy)
{
    SDL_GetRelativeMouseState(dx, dy);
}

bool CLynxSys::MouseLeftDown()
{
    Uint8 s = SDL_GetMouseState(NULL, NULL);
    return SDL_BUTTON(s) == SDL_BUTTON_LEFT;
}

bool CLynxSys::MouseRightDown()
{
    Uint8 s = SDL_GetMouseState(NULL, NULL);
    return SDL_BUTTON(s) == SDL_BUTTON_RIGHT;
}

bool CLynxSys::MouseMiddleDown()
{
    Uint8 s = SDL_GetMouseState(NULL, NULL);
    return SDL_BUTTON(s) == SDL_BUTTON_MIDDLE;
}

BYTE* CLynxSys::GetKeyState()
{
    return SDL_GetKeyState(NULL);
}
