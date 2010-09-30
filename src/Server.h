#pragma once

#include <enet/enet.h>
#include <map>
#include "World.h"
#include "ClientInfo.h"
#include "Subject.h"
#include "Events.h"
#include "Stream.h"

#define CLIENTITER      std::map<int, CClientInfo*>::iterator

class CServer : public CSubject<EventNewClientConnected>,
                public CSubject<EventClientDisconnected>
{
public:
    CServer(CWorld* world);
    ~CServer(void);

    bool            Create(int port); // Start server on port
    void            Shutdown(); // Stop server

    void            Update(const float dt, const DWORD ticks);

    CClientInfo*    GetClient(int id);
    int             GetClientCount() const;
    CLIENTITER      GetClientBegin() { return m_clientlist.begin(); }
    CLIENTITER      GetClientEnd() { return m_clientlist.end(); }

protected:
    bool SendWorldToClient(CClientInfo* client);
    void OnReceive(CStream* stream, CClientInfo* client);

    void UpdateHistoryBuffer(); // Alte HistoryBuffer Einträge löschen, kein Client benötigt mehr so eine alte Welt, oder die Welt ist zu alt und Client bekommt ein komplettes Update.
    void ClientHistoryACK(CClientInfo* client, DWORD worldid); // Client bestätigt

private:
    ENetHost* m_server;
    std::map<int, CClientInfo*> m_clientlist;

    std::map<DWORD, world_state_t> m_history; // World History Buffer. Benötigt für Quake 3 Network Modell bzw. differentielle Updates an Clients

    DWORD m_lastupdate;
    CWorld* m_world;

    CStream m_stream; // damit buffer nicht jedesmal neu erstellt werden muss
};
