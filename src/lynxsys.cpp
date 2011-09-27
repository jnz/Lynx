#include "lynx.h"
#include "lynxsys.h"
#include <SDL/SDL.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

uint32_t CLynxSys::GetTicks()
{
	static uint32_t base; // sdl is using uint32_t
	static bool initialized = false;

	if(!initialized)
	{
		base = SDL_GetTicks();
		initialized = true;
	}
    return (SDL_GetTicks() - base);
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

// if you only call GetKeyState with
// key = 0, you get a map of currently pressed keys
// if you set a value for key > 0, the map will be changed
// according to keydown and keyup
uint8_t* CLynxSys::GetKeyState(unsigned int key, bool keydown, bool keyup)
{
	static bool initialized = false;
	static uint8_t keymap[SDLK_LAST];
	if(!initialized)
    {
        memset(keymap, 0, sizeof(keymap));
        initialized = true;
    }

    if(key > 0)
    {
        assert(key < SDLK_LAST);
        assert((int)keydown + (int)keyup == 1); // xor: keydown OR keyup
        if(keydown)
            keymap[key]++;
        if(keyup)
            keymap[key] = 0;
    }

    return keymap;
}
