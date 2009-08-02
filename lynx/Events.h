#pragma once

#include "ClientInfo.h"

struct EventNewClientConnected
{
	CClientInfo* client;
};

struct EventClientDisconnected
{
	CClientInfo* client;
};
