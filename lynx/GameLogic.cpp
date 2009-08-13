#include <assert.h>
#include "GameLogic.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameLogic::CGameLogic(CWorld* world)
{
	assert(m_world);
	m_world = world;
}

CGameLogic::~CGameLogic(void)
{
}

void CGameLogic::InitGame()
{
    // 1 Testobjekt erstellen
	CObj* obj;
	obj = new CObj(m_world);
	obj->SetOrigin(vec3_t(0.0f, 9.0f, -10.0f));
	obj->SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
	obj->SetAnimation("default");
	m_world->AddObj(obj);

    // Level laden
    m_world->LoadLevel(CLynx::GetBaseDirLevel() + "testlvl/level1.obj");
}

void CGameLogic::Notify(EventNewClientConnected e)
{
	CObj* obj;
	obj = new CObj(m_world);
	obj->SetOrigin(vec3_t(0.0f, 45.0f, 0));
    obj->SetResource(CLynx::GetBaseDirModel() + "pknight/tris.md2");
	obj->SetAnimation("default");
	m_world->AddObj(obj, true); // In diesem Frame, weil die Welt umgehend vom CServer serialized wird
	e.client->m_obj = obj->GetID(); // Mit Client verknüpfen
}

void CGameLogic::Notify(EventClientDisconnected e)
{
	m_world->DelObj(e.client->m_obj);
    e.client->m_obj = 0;
}

void CGameLogic::Update(const float dt, const DWORD ticks)
{
	CObj* obj;
	OBJITER iter;
	for(iter = m_world->ObjBegin();iter!=m_world->ObjEnd();iter++)
	{
		obj = (*iter).second;
		m_world->ObjMove(obj, dt);
        if(vec3_t(obj->GetVel().x, 0.0f, obj->GetVel().z).AbsSquared() > 
           100*lynxmath::EPSILON)
            obj->SetAnimation("run");
        else
            obj->SetAnimation("default");
	}

}