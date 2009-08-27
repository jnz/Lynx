#pragma once

#include <assert.h>
#include <string>		// Default String Type for Lynx

typedef unsigned long	DWORD;
typedef int				INT32;
typedef short			INT16;
typedef unsigned short	WORD;
typedef unsigned char	BYTE;

#define SAFE_RELEASE(x) if(x) { delete x; x = NULL; }
#define SAFE_RELEASE_ARRAY(x) if(x) { delete[] x; x = NULL; }

class CLynx
{
public:
	static float AngleMod(float a);

	static std::string GetBaseDir() { return "baselynx/"; }
	static std::string GetBaseDirLevel() { return "baselynx/level/"; }
	static std::string GetBaseDirModel() { return "baselynx/model/"; }
	static std::string GetBaseDirFX() { return "baselynx/fx/"; }

	static std::string StripFileExtension(std::string path);
	static std::string ChangeFileExtension(std::string path, std::string newext);
	static std::string GetDirectory(std::string path);

    static std::string FloatToString(float f, int precision);

    static float randf() { return (rand()%20000)*0.0001f-1.0f; } // FIXME srand aufruf sicherstellen
    static float randfabs() { return (rand()%10000)*0.0001f; }
};

union intfloat_u
{
	unsigned int ui;
	int i;
	float f;
};

#pragma warning(disable: 4996)