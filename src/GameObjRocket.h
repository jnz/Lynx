#pragma once
#include "GameObj.h"

class CGameObjRocket :
    public CGameObj
{
public:
    CGameObjRocket(CWorld* world);
    ~CGameObjRocket(void);

    virtual int     GetType() { return GAME_OBJ_TYPE_ROCKET; }

    // Wallhit notification
    virtual void     OnHitWall(const vec3_t location, const vec3_t normal);

    // Rocket speed
    static float     GetRocketSpeed() { return 25.0f; }

    // which player fired this rocket?
    void             SetOwner(int objid) { m_owner = objid; }
    int              GetOwner() { return m_owner; }

protected:
    int m_owner;
};

class CThinkFuncRocket : public CThinkFunc
{
public:
    CThinkFuncRocket(uint32_t time, CWorld* world, CObj* obj) :
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(uint32_t leveltime);
};
