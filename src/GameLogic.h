#pragma once

#include "Subject.h"
#include "Events.h"
#include "World.h"
#include "Server.h"

class CGameLogic : public CObserver<EventNewClientConnected>,
                   public CObserver<EventClientDisconnected>
{
public:
    CGameLogic(CWorld* world, CServer* server); // If a client is using this class, set server to NULL
    virtual ~CGameLogic(void);

    virtual bool InitGame(const char* level);
    virtual void Update(const float dt, const uint32_t ticks);

    virtual void ClientMove(CObj* clientobj, const std::vector<std::string>& clcmdlist); // Client is using the same movement code
    virtual void ClientMouse(CObj* clientobj, float lat, float lon);

    virtual void Precache(CResourceManager* resman) {} // gets called by InitGame to preload resources

    CServer* GetServer() { return m_server; }

protected:
    virtual void Notify(EventNewClientConnected);
    virtual void Notify(EventClientDisconnected);

    CWorld* GetWorld() { return m_world; }

private:
    CWorld* m_world;
    CServer* m_server;
};
