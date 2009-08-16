#include <assert.h>
#include "GameZombie.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

class CObjInfo : public CObjOpaque
{
public:
    CObjInfo()
    {
        clientid = -1;
        health = 100;
    };
    virtual ~CObjInfo() {}

    int health;
    int clientid;
};

CGameZombie::CGameZombie(CWorld* world, CServer* server) : CGameLogic(world, server)
{

}

CGameZombie::~CGameZombie(void)
{
}

void CGameZombie::InitGame()
{
    // 1 Testobjekt erstellen
	CObj* obj;
	obj = new CObj(m_world);
	obj->SetOrigin(vec3_t(30.0f, 9.0f, -10.0f));
	obj->SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
	obj->SetAnimation(0);
    obj->SetOpaque(new CObjInfo());
	m_world->AddObj(obj);

    // Level laden
    m_world->LoadLevel(CLynx::GetBaseDirLevel() + "testlvl/level1.obj");
}

void CGameZombie::Notify(EventNewClientConnected e)
{
	CObj* obj;
	obj = new CObj(m_world);
	obj->SetOrigin(vec3_t(0.0f, 45.0f, 0));
    obj->SetResource(CLynx::GetBaseDirModel() + "pknight/tris.md2");
	obj->SetAnimation(0);
    obj->SetOpaque(new CObjInfo());
    ((CObjInfo*)obj->GetOpaque())->clientid = e.client->GetID();
	m_world->AddObj(obj, true); // In diesem Frame, weil die Welt umgehend vom CServer serialized wird
	e.client->m_obj = obj->GetID(); // Mit Client verknüpfen
}

void CGameZombie::Notify(EventClientDisconnected e)
{
	m_world->DelObj(e.client->m_obj);
    e.client->m_obj = 0;
}

void CGameZombie::Update(const float dt, const DWORD ticks)
{
	CObj* obj;
	OBJITER iter;
    int clientid;

	for(iter = m_world->ObjBegin();iter!=m_world->ObjEnd();iter++)
	{
		obj = (*iter).second;
		m_world->ObjMove(obj, dt);
        if(vec3_t(obj->GetVel().x, 0.0f, obj->GetVel().z).AbsSquared() > 
           100*lynxmath::EPSILON)
            obj->SetNextAnimation(obj->GetAnimationFromName("run"));
        else
            obj->SetNextAnimation(0);

        clientid = ((CObjInfo*)obj->GetOpaque())->clientid;
        if(clientid >= 0)
            ProcessClientCmds(obj, clientid);
	}
}

void CGameZombie::ProcessClientCmds(CObj* obj, int clientid)
{
    CClientInfo* client = m_server->GetClient(clientid);
    assert(client);
    if(!client)
        return;

    std::vector<std::string>::iterator iter;
    for(iter = client->clcmdlist.begin();iter != client->clcmdlist.end();iter++)
    {
        if((*iter) == "+fire")
        {
            m_world->GetBSP()->TraceRay(
        }
    }
}