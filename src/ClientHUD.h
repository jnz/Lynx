#pragma once

#include <string>
#include "Stream.h"
#include "ModelMD2.h"
#include "ResourceManager.h"

// Animation Index Hardcoded
#define HUD_WEAPON_FIRE_ANIMATION       1
#define HUD_WEAPON_IDLE_ANIMATION       2

class CClientHUD
{
public:
    CClientHUD(void);
    ~CClientHUD(void);

    std::string weapon;     // weapon model
    BYTE        animation;     // animation index

    /*
        Serverseite: ruft Serialize(true, stream, NULL) auf
        Clientseite: ruft Serialize(false, stream, resman) auf, dann wird intern
                     das passende CModelMD2* m_model geladen, damit der Renderer
                     schnell über GetModel() auf das model zugreifen kann.
     */

    void        Serialize(const bool write, CStream* stream, CResourceManager* resman);

    void        GetModel(CModelMD2** model, md2_state_t** state);
    void        UpdateModel(CResourceManager* resman); // Load CModelMD2 for "weapon" and set animation

protected:
    CModelMD2* m_model;
    md2_state_t m_model_state;
};
