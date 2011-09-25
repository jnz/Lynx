#include "GameObjRocket.h"
#include "ParticleSystemRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjRocket::CGameObjRocket(CWorld* world) : CGameObj(world)
{
    SetOwner(0); // someone should take later the ownership
    SetResource(CLynx::GetBaseDirModel() + "rocket/projectile.md5mesh");
    SetAnimation(ANIMATION_NONE);
    SetRadius(0.2f);

    // remove the rocket after 14 seconds no matter what:
    m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 14000, GetWorld(), this));
    AddFlags(OBJ_FLAGS_NOGRAVITY); // move in a straight line through the world
    // attach a nice smoke trail particle system to the rocket
    SetParticleSystem("rock|" + CParticleSystemRocket::GetConfigString(vec3_t(0.0f, 0.0f, 0.0f)));
}

CGameObjRocket::~CGameObjRocket(void)
{

}

void CGameObjRocket::OnHitWall(const vec3_t& location, const vec3_t& normal)
{
    std::vector<int> objtypes;
    objtypes.push_back(GAME_OBJ_TYPE_PLAYER);
    objtypes.push_back(GAME_OBJ_TYPE_ZOMBIE);

    const float splashradius = GetSplashDamageRadius();
    const std::vector<CObj*> splashobjlist =
        GetWorld()->GetNearObjByTypeList(location,
                                         splashradius,
                                         GetID(),
                                         objtypes);
    DealDamageToNearbyObjs(splashobjlist);

    DestroyRocket(location);
}

void CGameObjRocket::DestroyRocket(const vec3_t& location)
{
    SpawnParticleExplosion(location, 8.0f); // 8.0f = explosion size
    // the rocket particle system would disappear, if we delete
    // the rocket straight away (it's linked to the rocket).
    // remove us in 800 ms, give the particle system time to fade out
    // meanwhile the rocket is a ghost
    SetVel(vec3_t::origin); // no need to move anymore
    AddFlags(OBJ_FLAGS_GHOST); // this makes the rocket invisible, but not the particle system
    m_think.RemoveAll(); // remove the safety delete thinkfunc, which might interrupt our fadeout
    m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 800, GetWorld(), this));

    PlaySound(location,
              CLynx::GetBaseDirSound() + "rifle.ogg",
              800);
}

void CGameObjRocket::DealDamageToNearbyObjs(const std::vector<CObj*>& nearobjs)
{
    bool killed_me;
    float damage;
    const float base_damage = GetDamage();

    // for every hit object:
    for(std::vector<CObj*>::const_iterator hititer =
            nearobjs.begin();
            hititer != nearobjs.end();
            hititer++)
    {
        CGameObj* hitobj = (CGameObj*)(*hititer);
        assert(hitobj->GetType() > GAME_OBJ_TYPE_OBJ); // make sure we get valid types here
        // now we deal some damage to the object

        damage = base_damage; // FIXME implement splash damage
        hitobj->DealDamage(damage,
                           hitobj->GetOrigin(),
                           vec3_t::origin,
                           this,
                           killed_me);
    }
}

