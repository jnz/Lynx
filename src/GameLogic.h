#pragma once

#include "Subject.h"
#include "Events.h"
#include "World.h"
#include "Server.h"

class CGameLogic : public CObserver<EventNewClientConnected>,
                   public CObserver<EventClientDisconnected>
{
public:
    CGameLogic(CWorld* world, CServer* server); // Wenn CClient diese Klasse nutzt, server auf NULL setzen
    virtual ~CGameLogic(void);

    virtual bool InitGame();
    virtual void Update(const float dt, const uint32_t ticks);

    virtual void ClientMove(CObj* clientobj, const std::vector<std::string>& clcmdlist); // Wird von CClient mitgenutzt
    virtual void ClientMouse(CObj* clientobj, float lat, float lon);

protected:
    virtual void Notify(EventNewClientConnected);
    virtual void Notify(EventClientDisconnected);

    CWorld* GetWorld() { return m_world; }
    CServer* GetServer() { return m_server; }

private:
    CWorld* m_world;
    CServer* m_server;
};
