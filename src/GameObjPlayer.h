#pragma once
#include "GameObj.h"
#include "ClientInfo.h"

class CGameObjPlayer :
    public CGameObj
{
public:
    CGameObjPlayer(CWorld* world);
    ~CGameObjPlayer(void);

    virtual int     GetType() { return GAME_OBJ_TYPE_PLAYER; }

    void            CmdFire(bool active, CClientInfo* client); // Is cmd active?

    virtual void    DealDamage(int damage,
                               const vec3_t& hitpoint,
                               const vec3_t& dir,
                               CGameObj* dealer,
                               bool& killed_me);

    void            SetLookDir(const quaternion_t& dir) { m_lookdir = dir; }
    quaternion_t    GetLookDir() { return m_lookdir; }

    void            Respawn();

protected:
    void            FireGun(CClientInfo* client);
    void            FireRocket(CClientInfo* client);

private:
    bool            m_prim_triggered; // if +fire active?
    uint32_t        m_prim_triggered_time;
    quaternion_t    m_lookdir;
};
