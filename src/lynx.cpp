#include "lynx.h"
#include <SDL/SDL.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#pragma warning(disable: 4244)

CConfig CLynx::cfg; // global config

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

std::string CLynx::GetFileExtension(std::string path)
{
    size_t pos;

    pos = path.rfind('.');
    if(pos == std::string::npos)
        return "";

    return path.substr(pos, std::string::npos);
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

std::string CLynx::GetFilename(std::string path)
{
    size_t pos;

    pos = path.find_last_of("/\\");
    if(pos == std::string::npos)
        return path;

    return path.substr(pos+1, std::string::npos);
}

std::string CLynx::FloatToString(float f, int precision)
{
   std::ostringstream o;
   o.precision(precision);
   if(!(o << f))
   {
       assert(0); // Unable to convert float to string?
       return "";
   }
   return o.str();
}

bool CLynx::IsFloat(float f)
{
   std::ostringstream o;
   return (o << f);
}

int CLynx::random(int min, int max)
{
    return min + (int)((double)rand() / (RAND_MAX / (max - min + 1) + 1));
}

std::string CLynx::GetRandNumInStr(const char* str, unsigned int maxnumber)
{
    char tmpstr[512];

    assert(strlen(str) < sizeof(tmpstr)-10);
    sprintf(tmpstr, str, CLynx::random(1, maxnumber));
    return std::string(tmpstr);
}

std::string CLynx::ReadCompleteFile(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if(!f)
        return "";

    fseek(f, 0, SEEK_END);
    unsigned int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buff = new char[fsize+5]; // 5 bytes, so you can sleep better
    if(!buff)
        return "";

    fread(buff, fsize, 1, f);
    buff[fsize] = NULL;
    std::string shader(buff);

    delete[] buff;
    fclose(f);

    return shader;
}

