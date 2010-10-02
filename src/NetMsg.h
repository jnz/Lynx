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

//#define USE_RANGE_ENCODER

#define NET_VERSION             30

#define NET_MAGIC               0x5     // 101 (binary)

#define NET_MSG_INVALID         0
#define NET_MSG_SERIALIZE_WORLD 1
#define NET_MSG_CLIENT_CTRL     2

#define NET_MSG_MAX             3 // make this the largest number

/*
#pragma pack(push, 1)
#define NET_GET_BYTE(d) BYTE get_##d() { return d; }
#define NET_SET_BYTE(d) void  set_##d(BYTE d) { this->d = d; }
#define NET_GET_WORD(d) WORD get_##d() { return ntohs(d); }
#define NET_SET_WORD(d) void  set_##d(WORD d) { this->d = htons(d); }
#define NET_GET_DWORD(d) DWORD get_##d() { return ntohl(d); }
#define NET_SET_DWORD(d) void  set_##d(DWORD d) { this->d = htonl(d); }

#define NET_GETSET_BYTE(d) NET_GET_BYTE(d) NET_SET_BYTE(d)
#define NET_GETSET_WORD(d) NET_GET_WORD(d) NET_SET_WORD(d)
#define NET_GETSET_DWORD(d) NET_GET_DWORD(d) NET_SET_DWORD(d)

typedef struct net_s
{
public:
    NET_GETSET_BYTE(magic);
    NET_GETSET_BYTE(type);

protected:
    BYTE magic;
    BYTE type;
} net_t;

typedef struct net_connect_s : public net_t
{
public:
    NET_GETSET_DWORD(version)

private:
    DWORD version;
} net_connect_t;

typedef struct net_world_serialize_s : public net_t
{
public:

} net_world_serialize_t;

#pragma pack(pop)
*/
