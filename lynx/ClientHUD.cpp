#include <assert.h>
#include <string.h>
#include "ClientHUD.h"

CClientHUD::CClientHUD(void)
{
    animation = 0;
    m_model = NULL;
    memset(&m_model_state, 0, sizeof(m_model_state));
}

CClientHUD::~CClientHUD(void)
{

}

void CClientHUD::Serialize(const bool write, CStream* stream, CResourceManager* resman)
{
    if(write)
    {
        stream->WriteString(weapon);
        stream->WriteBYTE(animation);
    }
    else
    {
        std::string newweapon;
        BYTE newanimation;

        stream->ReadString(&newweapon);
        stream->ReadBYTE(&newanimation);

        if(newweapon != weapon || newanimation != animation)
        {
            weapon = newweapon;
            animation = newanimation;
            UpdateModel(resman);
        }
    }
}

void CClientHUD::UpdateModel(CResourceManager* resman)
{
    assert(resman);
    CModelMD2* resmodel = resman->GetModel(CLynx::GetBaseDirModel() + weapon);
    if(resmodel != m_model)
    {
        m_model = resmodel;
        m_model->SetAnimation(&m_model_state, animation);
        m_model->SetNextAnimation(&m_model_state, animation);
    }
}

void CClientHUD::GetModel(CModelMD2** model, md2_state_t** state)
{
    if(model)
        *model = m_model;
    if(state)
        *state = &m_model_state;
}
