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

// Precache functions
struct gamezombie_precache_t
{
    std::string name; // descriptive name
    std::string path; // path to resource
    resource_type_t type; // see ResourceManager.h
};

static const gamezombie_precache_t g_game_zombie_precache[] =
{
    {"rocket viewmodel", "rocket/rocketlauncher.md5mesh", LYNX_RESOURCE_TYPE_MD5},
    {"player model", "marine/marine.md5mesh", LYNX_RESOURCE_TYPE_MD5},
    {"zombie model", "pinky/pinky.md5mesh", LYNX_RESOURCE_TYPE_MD5},
    {"rifle sound", "rifle.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster hit sound 1", "monsterhit1.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster hit sound 2", "monsterhit2.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster hit sound 3", "monsterhit3.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster death sound", "monsterdie.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster startle sound", "monsterstartle1.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster startle sound", "monsterstartle2.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster startle sound", "monsterstartle3.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster attack sound 1", "monsterattack1.ogg", LYNX_RESOURCE_TYPE_SOUND},
    {"monster attack sound 2", "monsterattack2.ogg", LYNX_RESOURCE_TYPE_SOUND},

    {"normal texture", "normal.jpg", LYNX_RESOURCE_TYPE_TEXTURE}
};
static const int g_game_zombie_precache_count = sizeof(g_game_zombie_precache) / sizeof(g_game_zombie_precache[0]);

void CGameZombie::Precache(CResourceManager* resman)
{
    for(int i=0;i<g_game_zombie_precache_count;i++)
    {
        resman->Precache(g_game_zombie_precache[i].path,
                         g_game_zombie_precache[i].type);
    }
}

bool CGameZombie::InitGame(const char* level)
{
    // Loading level
    std::string leveluser(level);
    std::string levelpath;
    if(leveluser.length() < 1)
    {
        fprintf(stderr, "GameZombie init failed, level argument is null");
        return false;
    }
    levelpath = CLynx::GetBaseDirLevel() + leveluser;
    bool success = GetWorld()->LoadLevel(levelpath);
    if(!success)
    {
        fprintf(stderr, "Game: Failed to load level: %s\n", levelpath.c_str());
        return false;
    }

    // Spawn some zombies
    for(int i=0;i<1;i++)
    {
        bspbin_spawn_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();
        CGameObjZombie* zombie = new CGameObjZombie(GetWorld());

        zombie->SetOrigin(point.point);
        zombie->SetRot(point.rot);
        GetWorld()->AddObj(zombie);
    }

    return true;
}

// New client connected
void CGameZombie::Notify(EventNewClientConnected e)
{
    CGameObjPlayer* player;
    vec3_t spawn = GetWorld()->GetBSP()->GetRandomSpawnPoint().point;
    player = new CGameObjPlayer(GetWorld());
    player->SetOrigin(spawn);
    player->SetResource(CLynx::GetBaseDirModel() + "marine/marine.md5mesh");
    player->SetRadius(2.0f);
    player->SetAnimation(0);
    player->SetEyePos(vec3_t(0,1.65f,0));
    player->SetClientID(e.client->GetID());

    GetWorld()->AddObj(player, true); // set this argument to true, so that we are directly in the next serialize message
    e.client->m_obj = player->GetID(); // Link this object with the client
    e.client->hud.weapon = "rocket/rocketlauncher.md5mesh";
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

        // FIXME do this only think at fixed intervals, not every frame
        obj->m_think.DoThink(GetWorld()->GetLeveltime());

        GetWorld()->ObjMove(obj, dt);
        if(obj->IsClient())
        {
            if((obj->GetVel().x*obj->GetVel().x + obj->GetVel().z*obj->GetVel().z) > 0.25f)
                obj->SetAnimation(ANIMATION_RUN);
            else
                obj->SetAnimation(ANIMATION_IDLE);

            client = GetServer()->GetClient(obj->GetClientID());
            if(client) // safety check: maybe the object is associated with an invalid client id
            {
                ProcessClientCmds((CGameObjPlayer*)obj, client);
                ClientMouse(obj, client->lat, client->lon);

                // Calculate real view direction of the player.
                // This is not the same as the player obj rotation quaternion,
                // as the player can look up and down
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
            // FIXME this code sucks
            for(iter2 = GetWorld()->ObjBegin();iter2!=GetWorld()->ObjEnd();iter2++)
            {
                obj2 = (CGameObj*)(*iter2).second;
                if(obj2 == obj || obj2->GetType() != GAME_OBJ_TYPE_ZOMBIE)
                    continue;
                if(obj2->GetHealth() <= 0) // don't push dead bodies
                    continue;

                const float force = 75.0f; // not really a force, where f = ma
                vec3_t diff(obj2->GetOrigin() - obj->GetOrigin());
                float difflen = diff.AbsFast();
                if(difflen > 5.0f)
                    continue;
                difflen = lynxmath::SqrtFast(difflen); // sqrtfast is ok here
                if(difflen < 0.75f)
                    difflen = 0.75f;
                diff = dt*diff*force*1/(difflen*difflen);
                diff.y = 0.0f;
                diff += obj2->GetVel();
                if(diff.AbsSquared()>25.0f)
                    diff.SetLength(5.0f);
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

    clientobj->CmdFire(bFire, client);

    client->clcmdlist.clear();
}

