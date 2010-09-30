#pragma once
#include "GameObj.h"

class CGameObjZombie :
    public CGameObj
{
public:
    CGameObjZombie(CWorld* world);
    ~CGameObjZombie(void);

    virtual int     GetType() { return GAME_OBJ_TYPE_ZOMBIE; }

    void            DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer);

    void            FindVictim();
    int             currenttarget;
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
