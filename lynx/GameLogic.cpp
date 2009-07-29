#include "GameLogic.h"

CGameLogic::CGameLogic(CWorld* world)
{
	m_world = world;
}

CGameLogic::~CGameLogic(void)
{
}

void CGameLogic::Notify(EventNewClientConnected e)
{
	CObj* obj;
	obj = new CObj(m_world);
	obj->pos.origin = vec3_t(45.0f, 8.0f, 0);
	obj->SetSpeed(6.0f);
	obj->pos.velocity = vec3_t(0.0f, 0, 0.0f);
	obj->SetResource(CLynx::GetBaseDirModel() + "q2/tris2.md2");
	obj->SetAnimation("default");
	m_world->AddObj(obj, true); // In diesem Frame, weil die Welt umgehend vom CServer serialized wird
	e.client->m_obj = obj->GetID();
}