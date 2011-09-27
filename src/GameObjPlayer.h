#pragma once
#include "GameObj.h"
#include "GameLogic.h"

typedef enum
{
    WEAPON_NONE = 0,
    WEAPON_ROCKET,
    WEAPON_GUN,
    WEAPON_COUNT // last item
} weapon_type_t;

class CWeapon
{
public:
    weapon_type_t    type;
    std::string      name; // human readable
    int              damage; // base damage
    std::string      resource; // model md5 path
    int              firespeed; // fire a shot every x ms
    float            maxdist; // effective range
    int              maxammo; // max amount of ammo
};

class CGameObjPlayer :
    public CGameObj
{
public:
    CGameObjPlayer(CWorld* world);
    ~CGameObjPlayer(void);

    virtual int     GetType() const { return GAME_OBJ_TYPE_PLAYER; }

    void            CmdFire(bool active); // Is cmd active?

    void            ActivateRocket();
    void            ActivateGun();

    virtual void    DealDamage(int damage,
                               const vec3_t& hitpoint,
                               const vec3_t& dir,
                               CGameObj* dealer,
                               bool& killed_me);

    void            SetLookDir(const quaternion_t& dir) { m_lookdir = dir; }
    quaternion_t    GetLookDir() { return m_lookdir; }

    void            Respawn();

    // Network related: access to client HUD
    int             GetClientID() { return m_clientid; }
    void            SetClientID(CGameLogic* logic, int clientid);
    bool            IsClient();
    CClientInfo*    GetClient();
    // GetClient: remember, it is possible, that this object is in the world,
    // but the client is long gone, so this method will return NULL.

    const CWeapon*  GetWeapon(); // get read only information for current weapon
    int             GetAmmoLeft() { return 10; }
    int             GetAmmoMax() { return GetWeapon()->maxammo; }

protected:
    void            FireGun();
    void            FireRocket();

private:
    bool            m_prim_triggered; // if +fire active?
    uint32_t        m_prim_triggered_time;
    quaternion_t    m_lookdir;

    int             m_clientid;
    CGameLogic*     m_gamelogic;

    // weapon
    static const int GetWeaponInfoByType(weapon_type_t weapon); // search weapon info in g_weapon_registry
    int             m_weapon_id; // slot in g_weapon_registry
};
