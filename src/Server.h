#pragma once

#include <enet/enet.h>
#include <map>
#include "World.h"
#include "ClientInfo.h"
#include "Subject.h"
#include "Events.h"
#include "Stream.h"

#define CLIENTITER          std::map<int, CClientInfo*>::iterator

class CServer : public CSubject<EventNewClientConnected>,
                public CSubject<EventClientDisconnected>
{
public:
    CServer(CWorld* world);
    ~CServer(void);

    bool            Create(int port); // Start server on port
    void            Shutdown(); // Stop server

    // Server business
    void            Update(const float dt, const uint32_t ticks);

    // Manage client information
    CClientInfo*    GetClient(int id);
    int             GetClientCount() const;
    CLIENTITER      GetClientBegin() { return m_clientlist.begin(); }
    CLIENTITER      GetClientEnd() { return m_clientlist.end(); }

protected:
    bool SendWorldToClient(CClientInfo* client);
    void OnReceive(CStream* stream, CClientInfo* client);
    void OnReceiveClientCtrl(CStream* stream, CClientInfo* client);
    void OnReceiveChallenge(CStream* stream, CClientInfo* client);

    // Delete old history buffer entries if no client no longer needs them, or
    // they are so old, that the client probably is disconnected or has a huge lag.
    void UpdateHistoryBuffer();
    // Client ACK of snapshot, used for delta compression
    void ClientHistoryACK(CClientInfo* client, uint32_t worldid);

private:
    ENetHost* m_server;
    std::map<int, CClientInfo*> m_clientlist;

    // World History Buffer. Used for Q3 like delta compression.
    std::map<uint32_t, world_state_t> m_history;

    uint32_t m_lastupdate;
    CWorld* m_world;

    // We make this stream a member variable so that the server can reuse it
    // every frame. Otherwise we would have to new/delete 64k every frame.
    CStream m_stream;
};
