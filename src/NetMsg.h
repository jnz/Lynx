#pragma once

#include "lynx.h"
#include "Stream.h"

class CNetMsg
{
public:
    static int MaxHeaderLen() { return 8; }
    static void WriteHeader(CStream* stream, int msgtype); // prepares stream

    static int ReadHeader(CStream* stream); // returns msg type
};

#define NET_VERSION             32      // Protocol compatible

#define NET_MAGIC               0x5     // 101 (binary)

#define NET_MSG_INVALID         0
#define NET_MSG_SERIALIZE_WORLD 1
#define NET_MSG_CLIENT_CTRL     2

#define NET_MSG_MAX             3 // make this the largest number
