#pragma once

#include <string>
#include "Stream.h"
#include "ModelMD5.h"
#include "ResourceManager.h"

// Animation Index Hardcoded
#define HUD_WEAPON_FIRE_ANIMATION       1
#define HUD_WEAPON_IDLE_ANIMATION       2

class CClientHUD
{
public:
    CClientHUD(void);
    ~CClientHUD(void);

    std::string weapon;         // weapon model
    int16_t score;              // current points
    int8_t health;              // health

    /*
        Server side: calls Serialize(true, stream, NULL)
        Client side: calls Serialize(false, stream, resman), then the matching
                     CModelMD5 gets loaded, so the renderer can quickly access
                     the model by a GetModel() call.
     */

    void        Serialize(const bool write, CStream* stream, CResourceManager* resman);

    void        GetModel(CModelMD5** model, md5_state_t** state);
    void        UpdateModel(CResourceManager* resman); // Load CModelMD5 for "weapon" and set animation

protected:
    CModelMD5*  m_model;
    md5_state_t m_model_state;
};

