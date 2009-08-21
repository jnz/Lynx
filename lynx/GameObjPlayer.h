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

private:
    bool m_prim_triggered; // if +fire active?
    DWORD m_prim_triggered_time;
};
