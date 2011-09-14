#pragma once

#include <string>
#include <map>
class CResourceManager;
#include "ModelMD5.h"
#include "Sound.h"

class CWorld;

class CResourceManager
{
public:
    CResourceManager(CWorld* world);
    ~CResourceManager(void);

    unsigned int GetTexture(std::string texname, bool noerrormsg=false);
    void UnloadAllTextures();

    CModelMD5* GetModel(std::string mdlname);
    void UnloadAllModels();

    CSound* GetSound(std::string sndname);
    void UnloadAllSounds();

    bool IsServer();

    static animation_t GetAnimationFromString(std::string animation_name);
    static std::string GetStringFromAnimation(animation_t animation);

private:
    unsigned int LoadTexture(std::string path, bool noerrormsg=false);

    std::map<std::string, unsigned int> m_texmap;
    std::map<std::string, CModelMD5*> m_modelmap;
    std::map<std::string, CSound*> m_soundmap;

    CWorld* m_world;
};
