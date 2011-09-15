#pragma once

#include <string>
#include <map>
class CResourceManager;
#include "ModelMD5.h"
#include "Sound.h"

class CWorld;

struct texture_t
{
    unsigned int id; // OpenGL texture id
    unsigned int width; // width in pixel
    unsigned int height; // height in pixel
    std::string path; // file path
};

class CResourceManager
{
public:
    CResourceManager(CWorld* world);
    ~CResourceManager(void);

    unsigned int GetTexture(const std::string texname, const bool noerrormsg=false);
    void UnloadAllTextures();

    bool GetTextureDimension(const std::string texname,
                             unsigned int* pwidth,
                             unsigned int* pheight) const;

    CModelMD5* GetModel(std::string mdlname);
    void UnloadAllModels();

    CSound* GetSound(std::string sndname);
    void UnloadAllSounds();

    bool IsServer() const;

    static animation_t GetAnimationFromString(std::string animation_name);
    static std::string GetStringFromAnimation(animation_t animation);

private:
    unsigned int LoadTexture(const std::string path,
                             unsigned int* pwidth,
                             unsigned int* pheight,
                             const bool noerrormsg=false);

    std::map<std::string, texture_t> m_texmap;
    std::map<std::string, CModelMD5*> m_modelmap;
    std::map<std::string, CSound*> m_soundmap;

    CWorld* m_world;
};
