#include "GameObjZombie.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjZombie::CGameObjZombie(CWorld* world) : CGameObj(world)
{
	SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
	SetAnimation(0);
    m_think.AddFunc(new CThinkFuncZombie(GetWorld()->GetLeveltime() + 1000, GetWorld(), this));
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
                    GetWorld()->GetLeveltime() + 5000,
                    GetWorld(),
                    this));
}

void CGameObjZombie::FindVictim()
{
    const std::vector<CObj*> objlist = GetWorld()->GetNearObj(GetOrigin(), 50.0f, GetID());
    if(objlist.size() > 0)
    {
        int randid = rand()%(objlist.size());
        CObj* victim = objlist[randid];
        currenttarget = victim->GetID();
        fprintf(stderr, "Zombie found victim!\n");
    }
}

bool CThinkFuncRespawnZombie::DoThink(DWORD leveltime)
{
    GetWorld()->DelObj(GetObj()->GetID());

    bspbin_spawn_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();

    CGameObjZombie* zombie = new CGameObjZombie(GetWorld());
    zombie->SetOrigin(point.point);
    zombie->SetRot(point.rot);
    GetWorld()->AddObj(zombie);

    return true;
}

bool CThinkFuncZombie::DoThink(DWORD leveltime)
{
    CGameObjZombie* zombie = (CGameObjZombie*)GetObj();
    if(zombie->GetHealth() < 1)
        return true;

    int currenttarget = zombie->currenttarget;
    CGameObj* target = (CGameObj*)GetWorld()->GetObj(currenttarget);
    if(target == NULL)
    {
        zombie->FindVictim();
        SetThinktime(leveltime + 500);
        zombie->SetVel(vec3_t::origin);
        return false;
    }

    vec3_t dir = target->GetOrigin() - GetObj()->GetOrigin();

    SetThinktime(leveltime + 100);
    quaternion_t qTo = zombie->TurnTo(target->GetOrigin());
    quaternion_t qFrom = zombie->GetRot();
    
    zombie->SetRot(quaternion_t(qFrom, qTo, 0.4f));

    if(dir.AbsSquared() > 35.0f)
    {
        zombie->GetRot().GetVec3(&dir, NULL, NULL);
        zombie->SetVel(dir*-16.0f);
        zombie->SetAnimation(zombie->GetMesh()->FindAnimation("run"));
   }
    else
    {
        zombie->SetVel(vec3_t::origin);
        zombie->SetAnimation(0);
    }

    return false;
}