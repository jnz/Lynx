#include <assert.h>
#include "GameZombie.h"
#include "GameObjZombie.h"
#include "GameObjRocket.h"

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
    char* path; // path to resource
    resource_type_t type; // see ResourceManager.h
};

// list of resources to load
static const gamezombie_precache_t g_game_zombie_precache[] =
{
    {(char*)"rocket/rocketlauncher.md5mesh" , LYNX_RESOURCE_TYPE_MD5}   ,
    {(char*)"gun/gun.md5mesh"               , LYNX_RESOURCE_TYPE_MD5}   ,
    {(char*)"marine/marine.md5mesh"         , LYNX_RESOURCE_TYPE_MD5}   ,
    {(char*)"pinky/pinky.md5mesh"           , LYNX_RESOURCE_TYPE_MD5}   ,
    {(char*)"rocket/projectile.md5mesh"     , LYNX_RESOURCE_TYPE_MD5}   ,
    {(char*)"rifle.ogg"                     , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterhit1.ogg"               , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterhit2.ogg"               , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterhit3.ogg"               , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterdie.ogg"                , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterstartle1.ogg"           , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterstartle2.ogg"           , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterstartle3.ogg"           , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterattack1.ogg"            , LYNX_RESOURCE_TYPE_SOUND} ,
    {(char*)"monsterattack2.ogg"            , LYNX_RESOURCE_TYPE_SOUND} ,

    {(char*)"normal.jpg"                    , LYNX_RESOURCE_TYPE_TEXTURE}
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
    const int zombocount = 2; // this is zombo.count
    for(int i=0;i<zombocount;i++)
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
    player->SetClientID(this, e.client->GetID()); // let's do this early

    player->SetOrigin(spawn);
    player->SetResource(CLynx::GetBaseDirModel() + "marine/marine.md5mesh");
    player->SetRadius(2.0f);
    player->SetAnimation(ANIMATION_IDLE);
    player->SetEyePos(vec3_t(0,1.65f,0)); // FIXME player eye height should not be a magic number

    GetWorld()->AddObj(player, true); // set this argument to true, so that we are directly in the next serialize message
    e.client->m_obj = player->GetID(); // Link this object with the client

    player->InitHUD();
    player->ActivateRocket(); // give this man a gun
}

void CGameZombie::Notify(EventClientDisconnected e)
{
    CGameObjPlayer* player = (CGameObjPlayer*)GetWorld()->GetObj(e.client->m_obj);
    assert(player->GetType() == GAME_OBJ_TYPE_PLAYER);
    player->SetClientID(this, -1);
    GetWorld()->DelObj(e.client->m_obj);
    e.client->m_obj = 0;

    // in case of confusion: the player object is unlinked from
    // the network client info, because the object will
    // be destroyed at the end of the frame. but in the
    // meantime something could happen with the player object.
    // so we make sure no one gets an invalid CClientInfo* pointer.
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
        if(obj->GetOrigin().y < -500.0f)
        {
            fprintf(stderr, "GameZombie: obj in free fall (on ground: %s) obj id: %i res: %s\n",
                    obj->locGetIsOnGround() ? "true" : "false",
                    obj->GetID(), obj->GetResource().c_str());
            // Somehow the object is in free fall,
            // now we just select a random spawn point and place the object with
            // zero velocity there.
            bspbin_spawn_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();
            obj->Respawn(point.point, point.rot);
            return;
        }

        if(obj->GetType() == GAME_OBJ_TYPE_PLAYER)
        {
            // set player animation to run, if horizontal speed is > 0
            if((obj->GetVel().x*obj->GetVel().x + obj->GetVel().z*obj->GetVel().z) > 0.25f)
                obj->SetAnimation(ANIMATION_RUN);
            else
                obj->SetAnimation(ANIMATION_IDLE);

            client = ((CGameObjPlayer*)obj)->GetClient();
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
            // FIXME FIXME FIXME
            for(iter2 = GetWorld()->ObjBegin();iter2!=GetWorld()->ObjEnd();iter2++)
            {
                obj2 = (CGameObj*)(*iter2).second;
                if(obj2 == obj || obj2->GetType() != GAME_OBJ_TYPE_ZOMBIE)
                    continue;
                if(obj2->GetHealth() <= 0) // don't push dead bodies
                    continue;

                const float savey = obj2->GetVel().y;
                const float force = 75.0f; // not really a force, where f = ma
                vec3_t diff(obj2->GetOrigin() - obj->GetOrigin());
                float difflen = diff.AbsFast();
                if(difflen > (obj2->GetRadius()+obj->GetRadius()))
                    continue;
                difflen = lynxmath::SqrtFast(difflen); // sqrtfast is ok here
                if(difflen < 0.75f)
                    difflen = 0.75f;
                diff.y = 0.0f;
                diff = dt*diff*force*1/(difflen*difflen);
                diff += obj2->GetVel();
                if(diff.AbsSquared()>25.0f)
                    diff.SetLength(5.0f);
                diff.y = savey;
                obj2->SetVel(diff);
            }
        }
        else if(obj->GetType() == GAME_OBJ_TYPE_ROCKET && !(obj->GetFlags()&OBJ_FLAGS_GHOST)) // rockets are ghosts for a few seconds after the impact, to fade out the rocket trail. ignore them as ghost objects.
        {
            // FIXME: is this really something the game logic has to
            // take care of every frame? there should be an event callback
            // similar to the level geometry collision callback.

            // Check if the rocket is near a player or a monster.
            // if yes, explode and deal damage
            std::vector<int> objtypes;
            objtypes.push_back(GAME_OBJ_TYPE_PLAYER);
            objtypes.push_back(GAME_OBJ_TYPE_ZOMBIE);
            const std::vector<CObj*> nearobjlist =
                GetWorld()->GetNearObjByTypeList(obj->GetOrigin(),
                                                 obj->GetRadius(),
                                                 obj->GetID(),
                                                 objtypes);
            if(nearobjlist.size() > 0) // we have hit something
            {
                // as a rocket is dealing splash damage, we need to include
                // objects in a larger radius
                const float splashradius = ((CGameObjRocket*)obj)->GetSplashDamageRadius();
                const std::vector<CObj*> splashobjlist =
                    GetWorld()->GetNearObjByTypeList(obj->GetOrigin(),
                                                     splashradius,
                                                     obj->GetID(),
                                                     objtypes);
                assert(nearobjlist.size() > 0);
                ((CGameObjRocket*)obj)->DealDamageToNearbyObjs(splashobjlist);
                ((CGameObjRocket*)obj)->DestroyRocket(obj->GetOrigin());
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
        else if((*iter) == "w_rocket")
        {
            clientobj->ActivateRocket();
        }
        else if((*iter) == "w_gun")
        {
            clientobj->ActivateGun();
        }

    }

    clientobj->CmdFire(bFire);

    client->clcmdlist.clear();
}

