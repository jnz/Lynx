#include <assert.h>
#include <string.h>
#include "ClientHUD.h"

CClientHUD::CClientHUD(void)
{
    m_model = NULL;
    score = 0;
    health = 0;
}

CClientHUD::~CClientHUD(void)
{

}

void CClientHUD::Serialize(const bool write, CStream* stream, CResourceManager* resman)
{
    if(write)
    {
        stream->WriteString(weapon);
        stream->WriteInt16(score);
        stream->WriteChar(health);
    }
    else
    {
        std::string newweapon;

        stream->ReadString(&newweapon);
        stream->ReadInt16(&score);
        stream->ReadChar(&health);

        if(newweapon != weapon)
        {
            weapon = newweapon;
            UpdateModel(resman);
        }
    }
}

void CClientHUD::UpdateModel(CResourceManager* resman)
{
    assert(resman);
    if(!resman)
        return;

    CModelMD5* resmodel = resman->GetModel(CLynx::GetBaseDirModel() + weapon);
    if(resmodel != m_model)
    {
        m_model = resmodel;
        m_model->SetAnimation(&m_model_state, ANIMATION_NONE);
        m_model->SetAnimation(&m_model_state, ANIMATION_IDLE);
    }
}

void CClientHUD::GetModel(CModelMD5** model, md5_state_t** state)
{
    if(model)
        *model = m_model;
    if(state)
        *state = &m_model_state;
}

