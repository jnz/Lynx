#pragma once

#include "Obj.h"
#include "Think.h"

/*
    CGameObj: Serverseitige/Spielelogicseitige Erweiterung
    der CObj Klasse. Attribute werden nicht über das Netzwerk übertragen
 */

class CGameObj :
    public CObj
{
public:
    CGameObj(CWorld* world) : CObj(world)
    {
        m_health = 100;
        m_clientid = -1;
    }
    virtual ~CGameObj(void);

    void SetClientID(int clientid) { m_clientid = clientid; }
    int GetClientID() { return m_clientid; }
    bool IsClient() { return m_clientid != -1; }

    int GetHealth() { return m_health; }
    void SetHealth(int health) { m_health = health; }
    void AddHealth(int health) { m_health += health; }

    virtual void DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir) { AddHealth(-damage); };

    CThink m_think;

private:
    int m_health;
    int m_clientid;
};

class CThinkFuncRemoveMe : public CThinkFunc
{
public:
    CThinkFuncRemoveMe(DWORD time, CWorld* world, CObj* obj) : 
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(DWORD leveltime)
    {
        GetWorld()->DelObj(GetObj()->GetID());
        return true;
    }
};