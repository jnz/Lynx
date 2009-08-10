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
    /*
	CObj* obj;
	obj = new CObj(m_world);
	obj->SetOrigin(vec3_t(-45.0f, 8.0f, 0));
	obj->SetSpeed(6.0f);
	obj->SetVel(vec3_t(0.0f, 0.5f, 0.0f));
	obj->SetRot(vec3_t(0,270,0));
	obj->SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
	//obj->SetResource(CLynx::GetBaseDirModel() + "q2/tris2.md2");
	obj->SetAnimation("default");
	m_world->AddObj(obj);
    */
    //m_world->LoadLevel(CLynx::GetBaseDirLevel() + "testlvl/boxlvl.obj");
    m_world->LoadLevel(CLynx::GetBaseDirLevel() + "testlvl/polygon.obj");
}

void CGameLogic::Notify(EventNewClientConnected e)
{
	CObj* obj;
	obj = new CObj(m_world);
	obj->SetOrigin(vec3_t(45.0f, 8.0f, 0));
	obj->SetSpeed(6.0f);
	obj->SetVel(vec3_t(0.0f, 0, 0.0f));
	obj->SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
	obj->SetAnimation("default");
	m_world->AddObj(obj, true); // In diesem Frame, weil die Welt umgehend vom CServer serialized wird
	e.client->m_obj = obj->GetID();
}

void CGameLogic::Notify(EventClientDisconnected e)
{
	m_world->DelObj(e.client->m_obj);
}

void CGameLogic::Update(const float dt, const DWORD ticks)
{
	CObj* obj;
	OBJITER iter;
	for(iter = m_world->ObjBegin();iter!=m_world->ObjEnd();iter++)
	{
		obj = (*iter).second;
		//obj->GetVel().SetLength(obj->GetSpeed());
		//assert(obj->GetSpeed() > 0);
		m_world->ObjMove(obj, dt);
	}

}