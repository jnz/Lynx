#include <assert.h>
#include "GameZombie.h"
#include "GameObjZombie.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameZombie::CGameZombie(CWorld* world, CServer* server) : CGameLogic(world, server)
{

}

CGameZombie::~CGameZombie(void)
{
}

void CGameZombie::InitGame()
{
    // Level laden
    bool success = GetWorld()->LoadLevel(CLynx::GetBaseDirLevel() + "testlvl/level1.lbsp");
    assert(success);
    if(!success)
        return;

    // n Testobjekte erstellen
    for(int i=0;i<7;i++)
    {
        bspbin_spawn_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();
        CGameObjZombie* zombie = new CGameObjZombie(GetWorld());

        zombie->SetOrigin(point.point);
        zombie->SetRot(point.rot);
        GetWorld()->AddObj(zombie);
    }
}

void CGameZombie::Notify(EventNewClientConnected e)
{
	CGameObjPlayer* player;
	player = new CGameObjPlayer(GetWorld());
	player->SetOrigin(vec3_t(0.0f, 2.0f, 0));
    player->SetResource(CLynx::GetBaseDirModel() + "pknight/tris.md2");
	player->SetAnimation(0);
    player->SetEyePos(vec3_t(0,0.65f,0));
    player->SetClientID(e.client->GetID());

	GetWorld()->AddObj(player, true); // In diesem Frame, weil die Welt umgehend vom CServer serialized wird
    e.client->m_obj = player->GetID(); // Mit Client verknüpfen
    e.client->hud.weapon = "weapon/tris.md2";
    e.client->hud.animation = HUD_WEAPON_IDLE_ANIMATION;
}

void CGameZombie::Notify(EventClientDisconnected e)
{
    GetWorld()->DelObj(e.client->m_obj);
    e.client->m_obj = 0;
}

void CGameZombie::Update(const float dt, const DWORD ticks)
{
	CGameObj* obj;
	OBJITER iter;
    CClientInfo* client;

	for(iter = GetWorld()->ObjBegin();iter!=GetWorld()->ObjEnd();iter++)
	{
		obj = (CGameObj*)(*iter).second;

        obj->m_think.DoThink(GetWorld()->GetLeveltime());

		GetWorld()->ObjMove(obj, dt);
        if(obj->IsClient())
        {
            if(vec3_t(obj->GetVel().x, 0.0f, obj->GetVel().z).AbsSquared() > 
               1*lynxmath::EPSILON)
                obj->SetNextAnimation(obj->GetAnimationFromName("run"));
            else
                obj->SetNextAnimation(0);

            client = GetServer()->GetClient(obj->GetClientID());
            if(client) // client evtl. disconnected, clientobj wird im nächsten frame automatisch entfernt
            { 
                ProcessClientCmds((CGameObjPlayer*)obj, client);
                ClientMouse(obj, client->lat, client->lon);

                // Tatsächliche Client Blickrichtung Berechnen und merken
                quaternion_t qlat(vec3_t::xAxis, client->lat*lynxmath::DEGTORAD);
                quaternion_t qlon(vec3_t::yAxis, client->lon*lynxmath::DEGTORAD);
                ((CGameObjPlayer*)obj)->SetLookDir(qlon*qlat);
            }
        }
	}
}

void CGameZombie::ProcessClientCmds(CGameObjPlayer* clientobj, CClientInfo* client)
{
    bool bFire = false;

    std::vector<std::string>::iterator iter;
    for(iter = client->clcmdlist.begin();iter != client->clcmdlist.end();iter++)
    {
        if((*iter) == "+fire")
        {
            bFire = true;
        }
    }

    clientobj->CmdFire(bFire);

    client->clcmdlist.clear();
}
