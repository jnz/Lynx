#pragma once

#include "Obj.h"

class CThinkFunc
{
public:
    CThinkFunc(DWORD time, CWorld* world, CObj* obj)
    {
        thinktime = time;
        m_world = world;
        m_obj = obj;
    }
    DWORD GetThinktime() { return thinktime; }
    void SetThinktime(DWORD newtime) { thinktime = newtime; }
    virtual bool DoThink(DWORD leveltime) = 0; // bei rückgabe von true wird diese thinkfunc entfernt

protected:
    CWorld* GetWorld() { return m_world; }
    CObj* GetObj() { return m_obj; }
private:
    DWORD thinktime;
    CWorld* m_world;
    CObj* m_obj;
};

class CThink
{
public:
    ~CThink();
    void AddFunc(CThinkFunc* func); // neue thinkfunc hinzufügen
    void RemoveAll(); // alle thinkfuncs löschen
    void DoThink(DWORD leveltime); // alle thinkfuncs ausführen
private:
    std::list<CThinkFunc*> m_think;
};

