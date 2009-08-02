#include "NetMsg.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

void CNetMsg::WriteHeader(CStream *stream, int msgtype)
{
	stream->WriteBYTE(NET_MAGIC);
	stream->WriteBYTE(msgtype);
}

int CNetMsg::ReadHeader(CStream* stream)
{
	BYTE magic;
	BYTE type;

	stream->ReadBYTE(&magic);
	stream->ReadBYTE(&type);

    assert(magic == NET_MAGIC && type > NET_MSG_INVALID && type < NET_MSG_MAX);
	if(magic == NET_MAGIC && type > NET_MSG_INVALID && type < NET_MSG_MAX)
		return type;
	else
		return NET_MSG_INVALID;
}