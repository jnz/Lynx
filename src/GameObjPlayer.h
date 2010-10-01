#pragma once
#include "GameObj.h"

class CGameObjPlayer :
    public CGameObj
{
public:
    CGameObjPlayer(CWorld* world);
    ~CGameObjPlayer(void);

    virtual int GetType() { return GAME_OBJ_TYPE_PLAYER; }

    void CmdFire(bool active); // Is cmd active?
    void OnCmdFire(); // Called if +fire is activated

    virtual void DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer);

    void SetLookDir(const quaternion_t& dir) { m_lookdir = dir; }
    quaternion_t GetLookDir() { return m_lookdir; }

private:
    bool m_prim_triggered; // if +fire active?
    uint32_t m_prim_triggered_time;
    quaternion_t m_lookdir;
};
