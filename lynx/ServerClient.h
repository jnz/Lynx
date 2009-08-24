#pragma once

#define MAX_SV_CL_POS_DIFF      (15.0f*15.0f)           // abstand im quadrat
#define SERVER_UPDATETIME		(50)                    // Nach wievielen ms wird ein Update vom Server geschickt
#define RENDER_DELAY            (2*SERVER_UPDATETIME)   // Render Delay in ms
#define MAX_CLIENT_HISTORY      (20*SERVER_UPDATETIME)

#define MAXCLIENTS				(8)
#define SERVER_MAX_WORLD_AGE	(5000)				// wie viele ms heben wir für den client eine welt auf
#define MAX_WORLD_BACKLOG		(10*MAXCLIENTS)		// wie viele welten werden gespeichert
