#pragma once
#include "gameobj.h"

class CGameObjPlayer :
    public CGameObj
{
public:
    CGameObjPlayer(CWorld* world);
    ~CGameObjPlayer(void);


    void CmdFire(bool active); // Is cmd active?
    void OnCmdFire(); // Called if +fire is activated

    virtual void DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir);

private:
    bool m_prim_triggered; // if +fire active?
    DWORD m_prim_triggered_time;
};
