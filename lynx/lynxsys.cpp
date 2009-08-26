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

BYTE* CLynxSys::GetKeyState()
{
	return SDL_GetKeyState(NULL);
}
