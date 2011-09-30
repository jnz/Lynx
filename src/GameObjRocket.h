#pragma once
#include "GameObj.h"

class CGameObjRocket :
    public CGameObj
{
public:
    CGameObjRocket(CWorld* world);
    ~CGameObjRocket(void);

    virtual int      GetType() const { return GAME_OBJ_TYPE_ROCKET; }

    // Wallhit notification
    virtual void     OnHitWall(const vec3_t& location, const vec3_t& normal);

    // Rocket speed
    static float     GetRocketSpeed() { return 35.0f; } // m/s
    static float     GetSplashDamageRadius() { return 10.0f; } // in [m]
    static float     GetDamage() { return 80.0f; } // base value for splash damage

    // Deal damage to all objects in the nearobjs list
    void             DealDamageToNearbyObjs(const std::vector<CObj*>& nearobjs);

    // which player fired this rocket?
    void             SetOwner(int objid) { m_owner = objid; }
    int              GetOwner() { return m_owner; }

    void             DestroyRocket(const vec3_t& location);
protected:
    int m_owner;
};

