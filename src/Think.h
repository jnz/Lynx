#pragma once

#include "Obj.h"

/*
 * The idea of the think functions (thinkfunc) is that an
 * object will execute a piece of code at some point in the future.
 * To do this, you create a subclass of the CThinkFunc with
 * your own DoThink function (this is a virtual function) and use the
 * m_think object of the object to register your new think function.
 *
 * Example in GameObjZombie.cpp:
 *
 *  m_think.AddFunc(new CThinkFuncRespawnZombie(
 *                  GetWorld()->GetLeveltime() + 9000,
 *                  GetWorld(),
 *                  this));
 *  The custom think function object "CThinkFuncRespawnZombie" is added
 *  to the list of think functions and will be executed in 9000 ms.
 *
 * */

#define THINK_INTERVAL           50  // ms

class CThinkFunc
{
public:
    CThinkFunc(uint32_t time, CWorld* world, CObj* obj)
    {
        thinktime = time;
        m_world = world;
        m_obj = obj;
    }
    uint32_t GetThinktime() { return thinktime; }
    void SetThinktime(uint32_t newtime) { thinktime = newtime; }
    virtual bool DoThink(uint32_t leveltime) = 0; // bei rückgabe von true wird diese thinkfunc entfernt

protected:
    CWorld* GetWorld() { return m_world; }
    CObj* GetObj() { return m_obj; }
private:
    uint32_t thinktime;
    CWorld* m_world;
    CObj* m_obj;
};

class CThink
{
public:
    ~CThink();
    void AddFunc(CThinkFunc* func); // neue thinkfunc hinzufügen
    void RemoveAll(); // alle thinkfuncs löschen
    void DoThink(uint32_t leveltime); // alle thinkfuncs ausführen
private:
    std::list<CThinkFunc*> m_think;
};

