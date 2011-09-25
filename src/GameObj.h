#pragma once

#include "Obj.h"
#include "Think.h"

/*
    CGameObj: Serverside game logic
    Attributes in this class are not transmitted to the client.
 */

#define GAME_OBJ_TYPE_NONE          0
#define GAME_OBJ_TYPE_OBJ           1
#define GAME_OBJ_TYPE_ZOMBIE        2
#define GAME_OBJ_TYPE_PLAYER        3
#define GAME_OBJ_TYPE_ROCKET        4

class CGameObj :
    public CObj
{
public:
    CGameObj(CWorld* world);
    virtual ~CGameObj(void);

    virtual int GetType() const { return GAME_OBJ_TYPE_OBJ; }

    void SetClientID(int clientid) { m_clientid = clientid; }
    int GetClientID() { return m_clientid; }
    bool IsClient() { return m_clientid != -1; }

    int GetHealth() { return m_health; }
    void SetHealth(int health) { m_health = health; }
    void AddHealth(int health) { m_health += health; }

    quaternion_t TurnTo(const vec3_t& location) const; // rotation along yAxis.

    virtual void DealDamage(int damage,
                            const vec3_t& hitpoint,
                            const vec3_t& dir,
                            CGameObj* dealer,
                            bool& killed_me);

    CThink m_think;

    void SpawnParticleBlood(const vec3_t& location, const vec3_t& dir, const float size);
    void SpawnParticleDust(const vec3_t& location, const vec3_t& dir);
    void SpawnParticleRocket(const vec3_t& location, const vec3_t& dir);
    void SpawnParticleExplosion(const vec3_t& location, const float size);

    int PlaySound(const vec3_t& location, const std::string& soundpath, uint32_t lifetime); // id of sound obj

private:
    int m_health;
    int m_clientid;
};

class CThinkFuncRemoveMe : public CThinkFunc
{
public:
    CThinkFuncRemoveMe(uint32_t time, CWorld* world, CObj* obj) :
      CThinkFunc(time, world, obj) {}
    virtual bool DoThink(uint32_t leveltime)
    {
        GetWorld()->DelObj(GetObj()->GetID());
        return true;
    }
};

