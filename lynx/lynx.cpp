#include "lynx.h"
#include "SDL.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif


DWORD CLynx::GetTicks()
{
	return SDL_GetTicks();
}

void CLynx::GetMouseDelta(int* dx, int* dy)
{
	SDL_GetRelativeMouseState(dx, dy);
}

BYTE* CLynx::GetKeyState()
{
	return SDL_GetKeyState(NULL);
}

#pragma warning(disable: 4244)

float CLynx::AngleMod(float a)
{
	// from quake 2
	return (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
}

std::string CLynx::StripFileExtension(std::string path)
{
	size_t pos;

	pos = path.rfind('.');
	if(pos == std::string::npos)
		return path;

	return path.substr(0, pos);
}

std::string CLynx::ChangeFileExtension(std::string path, std::string newext)
{
	return StripFileExtension(path) + "." + newext;
}

std::string CLynx::GetDirectory(std::string path)
{
	size_t pos;

	pos = path.find_last_of("/\\");
	if(pos == std::string::npos)
		return path;

	return path.substr(0, pos+1);
}
