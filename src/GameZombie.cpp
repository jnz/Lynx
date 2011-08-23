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

bool CGameZombie::InitGame()
{
    // Level laden
    std::string levelpath = CLynx::GetBaseDirLevel() + "sponza/sponza.lbsp";
    bool success = GetWorld()->LoadLevel(levelpath);
    if(!success)
    {
        fprintf(stderr, "Game: Failed to load level: %s\n", levelpath.c_str());
        return false;
    }

    // n Testobjekte erstellen
    //for(int i=0;i<4;i++)
    //{
    //    bspbin_spawn_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();
    //    CGameObjZombie* zombie = new CGameObjZombie(GetWorld());

    //    zombie->SetOrigin(point.point);
    //    zombie->SetRot(point.rot);
    //    GetWorld()->AddObj(zombie);
    //}

    return true;
}

void CGameZombie::Notify(EventNewClientConnected e)
{
    CGameObjPlayer* player;
    vec3_t spawn = GetWorld()->GetBSP()->GetRandomSpawnPoint().point;
    player = new CGameObjPlayer(GetWorld());
    player->SetOrigin(spawn);
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

void CGameZombie::Update(const float dt, const uint32_t ticks)
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
                const quaternion_t qlat(vec3_t::xAxis, client->lat*lynxmath::DEGTORAD);
                const quaternion_t qlon(vec3_t::yAxis, client->lon*lynxmath::DEGTORAD);
                ((CGameObjPlayer*)obj)->SetLookDir(qlon*qlat);
            }
        }
        else if(obj->GetType() == GAME_OBJ_TYPE_ZOMBIE) // this if-condition branch applies a force to every other zombie.
        {   // FIXME: O(n^2) complexity - bad?
            OBJITER iter2;
            CGameObj* obj2;
            // FIXME use GameNearObj function?
            for(iter2 = GetWorld()->ObjBegin();iter2!=GetWorld()->ObjEnd();iter2++)
            {
                obj2 = (CGameObj*)(*iter2).second;
                if(obj2 == obj || obj2->GetType() != GAME_OBJ_TYPE_ZOMBIE)
                    continue;
                if(obj2->GetHealth() <= 0) // don't push dead bodies
                    continue;

                // the force will decrease by 1/dist^2
                const float force = 75.0f; // not a force in a strict physical sense
                vec3_t diff(obj2->GetOrigin() - obj->GetOrigin());
                float difflen = diff.AbsSquared();
                if(difflen > 25.0f)
                    continue;
                difflen = lynxmath::Sqrt(difflen);
                if(difflen < 0.15f)
                    difflen = 0.15f;
                diff = dt*diff*force*1/(difflen*difflen*difflen);
                diff += obj2->GetVel();
                obj2->SetVel(diff);
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

