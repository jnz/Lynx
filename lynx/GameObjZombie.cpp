#include "GameObjZombie.h"
#include "ParticleSystemBlood.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjZombie::CGameObjZombie(CWorld* world) : CGameObj(world)
{
	SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
    //SetResource(CLynx::GetBaseDirModel() + "pknight/tris.md2");
	SetAnimation(0);
    m_think.AddFunc(new CThinkFuncZombie(GetWorld()->GetLeveltime() + 1000, GetWorld(), this));
    currenttarget = -1;
}

CGameObjZombie::~CGameObjZombie(void)
{
}

void CGameObjZombie::DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir)
{
    CGameObj::DealDamage(damage, hitpoint, dir);

    if(GetHealth() > 0)
    {
        SetVel(dir*5.0f);

        CGameObj* blood = new CGameObj(GetWorld());
        blood->SetRadius(0.0f);
        blood->SetOrigin(hitpoint);
        blood->SetParticleSystem("blood|" + CParticleSystemBlood::GetConfigString(dir));
        blood->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 1000, GetWorld(), blood));
        blood->AddFlags(OBJ_FLAGS_NOGRAVITY);
        GetWorld()->AddObj(blood);

        SetAnimation(GetMesh()->FindAnimation("pain"));
        SetNextAnimation(0);
        return;
    }

    if(GetFlags() & OBJ_FLAGS_ELASTIC) // zombie ist schon tot
        return;

    SetVel(dir*18.0f + vec3_t(0,20.0f,0));
    SetAnimation(GetMesh()->FindAnimation("crdeath"));
    SetNextAnimation(-1);
    AddFlags(OBJ_FLAGS_ELASTIC);
    m_think.RemoveAll();
    m_think.AddFunc(new CThinkFuncRespawnZombie(
                    GetWorld()->GetLeveltime() + 5000,
                    GetWorld(),
                    this));
}

void CGameObjZombie::FindVictim()
{
    const std::vector<CObj*> objlist = GetWorld()->GetNearObj(GetOrigin(), 15.0f, GetID());
    if(objlist.size() > 0)
    {
        int randid = rand()%(objlist.size());
        CObj* victim = objlist[randid];
        currenttarget = victim->GetID();
        fprintf(stderr, "Zombie found victim!\n");
    }
}

void CGameObjZombie::LookAtNearestPlayer()
{
}

bool CThinkFuncRespawnZombie::DoThink(DWORD leveltime)
{
    GetWorld()->DelObj(GetObj()->GetID());

    spawn_point_t point = GetWorld()->GetBSP()->GetRandomSpawnPoint();

    CGameObjZombie* zombie = new CGameObjZombie(GetWorld());
    zombie->SetOrigin(point.origin);
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
        GetObj()->SetVel(vec3_t::origin);
        return false;
    }

    vec3_t dir = target->GetOrigin() - GetObj()->GetOrigin();
    if(dir.AbsSquared() > 35.0f)
    {
        dir = dir.Normalized()*20.0f;
        GetObj()->SetVel(dir);
        SetThinktime(leveltime + 40);
    }
    else
    {
        GetObj()->SetVel(vec3_t::origin);
        SetThinktime(leveltime + 1200);
    }

    return false;
}