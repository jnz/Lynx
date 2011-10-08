#pragma once
#include <enet/enet.h>
#include <vector>
#include <string>
#include "ClientHUD.h"

#define MAX_CLIENT_NAME_LEN   32

class CClientInfo
{
public:
    // peer is the ENet struct,
    // hostname is a human readable address (e.g. "192.168.0.5")
    // connecttime is the time in ms, when the client connected (e.g. from SDL_GetTicks())
    CClientInfo(ENetPeer* peer, const std::string hostname, uint32_t connecttime)
    {
        m_id          = ++m_idpool;
        m_peer        = peer;
        m_obj         = 0;
        worldidACK    = 0;
        lat = lon     = 0.0f;
        name          = "unnamed";
        got_challenge = false;
        disconnected  = false;
        m_hostname    = hostname;
        m_connecttime = connecttime;
    }

    int GetID() const { return m_id; }
    ENetPeer* GetPeer() { return m_peer; }
    const std::string& GetHostname() { return m_hostname; }
    uint32_t GetConnecttime() { return m_connecttime; }

    // Client Data
    int         m_obj;
    uint32_t    worldidACK;    // Last ACK'd world from client (for delta-compr.)
    CClientHUD  hud;
    std::string name;          // human readable name
    float       lat, lon;      // mouse lat and lon
    bool        got_challenge; // do we have the challenge msg from this client
    bool        disconnected;  // is this client waiting for a disconnect?

    // Client Input
    std::vector<std::string> clcmdlist; // all user commands are stored here as
                                        // string. e.g. "+attack"
private:
    int m_id;
    ENetPeer* m_peer;
    std::string m_hostname;     // human readable hostname
    uint32_t m_connecttime;     // time of connect event
    static int m_idpool;
};
