#pragma once
#include "GameObj.h"

class CGameObjZombie :
    public CGameObj
{
public:
    CGameObjZombie(CWorld* world);
    ~CGameObjZombie(void);

    void DealDamage(int damage, vec3_t dir);

    void LookAtNearestPlayer();
    void FindVictim();

    int currenttarget;
};

class CThinkFuncRespawnZombie : public CThinkFunc
{
public:
    CThinkFuncRespawnZombie(DWORD time, CWorld* world, CObj* obj) : 
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(DWORD leveltime);
};

class CThinkFuncZombie : public CThinkFunc
{
public:
    CThinkFuncZombie(DWORD time, CWorld* world, CObj* obj) : 
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(DWORD leveltime);
};