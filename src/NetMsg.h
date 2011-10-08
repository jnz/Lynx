#pragma once

#include "lynx.h"
#include "Stream.h"

class CNetMsg
{
public:
    static void WriteHeader(CStream* stream, int msgtype); // prepares stream
    static int  ReadHeader(CStream* stream); // returns msg type
};

#define NET_VERSION             32      // Protocol compatible
#define NET_MAGIC               0x5     // 101 (binary)

typedef enum
{
    NET_MSG_INVALID = 0,
    NET_MSG_SERIALIZE_WORLD,       // world snapshot
    NET_MSG_CLIENT_CTRL,           // client input data
    NET_MSG_CLIENT_CHALLENGE,      // first message from client after connect
    NET_MSG_CLIENT_CHALLENGE_OK,   // server accepts us

    NET_MSG_MAX                    // make this the last entry
} net_msg_t;
