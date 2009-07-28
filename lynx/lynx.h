#pragma once

#include <assert.h>
#include <string>		// Default String Type for Lynx

typedef unsigned long	DWORD;
typedef int				INT32;
typedef short			INT16;
typedef unsigned short	WORD;
typedef unsigned char	BYTE;

class CLynx
{
public:
	CLynx();
	static DWORD GetTicks();
	static void GetMouseDelta(int* dx, int* dy);
	static BYTE* GetKeyState();
	static float AngleMod(float a);

	static std::string GetBaseDir() { return "baselynx/"; }
	static std::string GetBaseDirLevel() { return "baselynx/level/"; }
	static std::string GetBaseDirModel() { return "baselynx/model/"; }

	static std::string StripFileExtension(std::string path);
	static std::string ChangeFileExtension(std::string path, std::string newext);
	static std::string GetDirectory(std::string path);
};

union intfloat_u
{
	unsigned int ui;
	int i;
	float f;
};

#pragma warning(disable: 4996)