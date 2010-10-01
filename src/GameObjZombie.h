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
    CThinkFuncRespawnZombie(uint32_t time, CWorld* world, CObj* obj) : 
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(uint32_t leveltime);
};

class CThinkFuncZombie : public CThinkFunc
{
public:
    CThinkFuncZombie(uint32_t time, CWorld* world, CObj* obj) : 
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(uint32_t leveltime);
};
