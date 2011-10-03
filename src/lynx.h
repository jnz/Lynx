#pragma once

#include <assert.h>
#include <string>       // Default String Type for Lynx
#include <cstdlib>
#ifdef __linux
#include <cstdio>
#endif
#include <limits.h>
#include <stdint.h>		// Visual Studio < 2010 has no stdint.h, google stdint.h and place it somewhere to be found

#include "Config.h"     // Lynx config system

#define LYNX_TITLE      "Lynx"
#define LYNX_MAJOR      0
#define LYNX_MINOR      2

#define SAFE_RELEASE(x) if((x)) { delete (x); (x) = NULL; }
#define SAFE_RELEASE_ARRAY(x) if((x)) { delete[] (x); (x) = NULL; }

class CLynx
{
public:
    static float AngleMod(float a);

    static std::string GetBaseDir() { return "baselynx/"; }
    static std::string GetBaseDirLevel() { return "baselynx/level/"; }
    static std::string GetBaseDirModel() { return "baselynx/model/"; }
    static std::string GetBaseDirFX() { return "baselynx/fx/"; }
    static std::string GetBaseDirSound() { return "baselynx/sound/"; }
    static std::string GetBaseDirTexture() { return "baselynx/texture/"; }
    static std::string GetBaseDirMenu() { return "baselynx/menu/"; }

    static std::string StripFileExtension(std::string path);
    static std::string GetFileExtension(std::string path);
    static std::string ChangeFileExtension(std::string path, std::string newext);
    static std::string GetDirectory(std::string path);
    static std::string GetFilename(std::string path);

    static std::string FloatToString(float f, int precision);
    static bool        IsFloat(float f);

    static float randf() { return (rand()%20000)*0.0001f-1.0f; } // random -1.0f - 1.0f
    static float randfabs() { return (rand()%10000)*0.0001f; } // random 0.0f - 1.0f

    static int random(int min, int max);
    static std::string GetRandNumInStr(const char* str, unsigned int maxnumber); // replace %i in string with number from 1 - maxnumber

    static CConfig cfg; // global static config system

    static std::string ReadCompleteFile(const std::string& path); // read a complete file and return it as a string
};

union intfloat_u
{
    unsigned int ui;
    int i;
    float f;
};

#pragma warning(disable: 4996)

