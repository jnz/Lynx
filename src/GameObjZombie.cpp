#include "GameObjZombie.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjZombie::CGameObjZombie(CWorld* world) : CGameObj(world)
{
    SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
    SetAnimation(0);
    m_think.AddFunc(new CThinkFuncZombie(GetWorld()->GetLeveltime() + 50, GetWorld(), this));
    currenttarget = -1;
}

CGameObjZombie::~CGameObjZombie(void)
{

}

void CGameObjZombie::DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer)
{
    CGameObj::DealDamage(damage, hitpoint, dir, dealer);

    if(GetHealth() > 0)
    {
        SetVel(dir*5.0f);
        SpawnParticleBlood(hitpoint, dir);
        if(CLynx::randfabs() < 0.50f)
        {
            PlaySound(GetOrigin(), 
                      CLynx::GetBaseDirSound() + CLynx::GetRandNumInStr("monsterhit%i.ogg", 3), 
                      180);
        }
        return;
    }

    if(GetFlags() & OBJ_FLAGS_ELASTIC) // zombie ist schon tot
        return;

    if(dealer)
    {
        SetRot(TurnTo(dealer->GetOrigin()));
    }
    SetVel(dir*18.0f + vec3_t(0,30.0f,0));
    SetAnimation(GetMesh()->FindAnimation("crdeath"));
    SetNextAnimation(-1);
    AddFlags(OBJ_FLAGS_ELASTIC); // zombie ist damit tot
    m_think.RemoveAll();
    m_think.AddFunc(new CThinkFuncRespawnZombie(
                    GetWorld()->GetLeveltime() + 9000,
                    GetWorld(),
                    this));
    if(CLynx::randfabs() < 0.8f) // 80% change of sound playing
    {
        PlaySound(hitpoint, CLynx::GetBaseDirSound() + "monsterdie.ogg", 250);
    }
}

void CGameObjZombie::FindVictim()
{
    CObj* victim;
    const std::vector<CObj*> objlist = 
        GetWorld()->GetNearObj(GetOrigin(), 50.0f, GetID(), GAME_OBJ_TYPE_PLAYER);
    if(objlist.size() > 0)
    {
        int randid = rand()%(objlist.size());
        victim = objlist[randid];
        currenttarget = victim->GetID();
        if(CLynx::randfabs() < 0.8f) // 80% change of sound playing
        {
            PlaySound(GetOrigin(), 
                      CLynx::GetBaseDirSound() + CLynx::GetRandNumInStr("monsterstartle%i.ogg", 3), 
                      250);
        }
    }
}

bool CThinkFuncRespawnZombie::DoThink(uint32_t leveltime)
{
    quaternion_t zombierot(vec3_t::yAxis, CLynx::randf()*lynxmath::PI);
    GetWorld()->DelObj(GetObj()->GetID());

    bspbin_spawn_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();

    CGameObjZombie* zombie = new CGameObjZombie(GetWorld());
    zombie->SetOrigin(point.point);
    zombie->SetRot(point.rot*zombierot);
    GetWorld()->AddObj(zombie);

    return true;
}

bool CThinkFuncZombie::DoThink(uint32_t leveltime)
{
    CGameObjZombie* zombie = (CGameObjZombie*)GetObj();
    if(zombie->GetHealth() < 1)
        return true;

    int currenttarget = zombie->currenttarget;
    CGameObj* target = (CGameObj*)GetWorld()->GetObj(currenttarget);
    if(target == NULL)
    {
        zombie->FindVictim();
        SetThinktime(leveltime + 400 + rand()%400);
        zombie->SetVel(vec3_t::origin);
        return false;
    }

    vec3_t dir = target->GetOrigin() - GetObj()->GetOrigin();

    SetThinktime(leveltime + 50+rand()%100);
    quaternion_t qTo = zombie->TurnTo(target->GetOrigin());
    quaternion_t qFrom = zombie->GetRot();
    
    zombie->SetRot(quaternion_t(qFrom, qTo, 0.5f));

    if(dir.AbsSquared() > 22.0f)
    {
        zombie->GetRot().GetVec3(&dir, NULL, NULL);
        zombie->SetVel(dir*-16.0f);
        zombie->SetAnimation(zombie->GetMesh()->FindAnimation("run"));
   }
    else
    {
        zombie->SetVel(vec3_t::origin);
        zombie->SetAnimation(0);
        zombie->SetAnimation(zombie->GetMesh()->FindAnimation("attack"));
    }

    return false;
}
