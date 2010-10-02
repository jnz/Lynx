#pragma once

//#define USE_RANGE_ENCODER

#define MAX_SV_PACKETLEN        (24000)

#define MAX_SV_CL_POS_DIFF      (35.0f*35.0f)           // abstand im quadrat
#define SERVER_UPDATETIME       (100)                   // Nach wievielen ms wird ein Update vom Server geschickt
#define RENDER_DELAY            (2*SERVER_UPDATETIME)   // Render Delay in ms
#define MAX_CLIENT_HISTORY      (20*SERVER_UPDATETIME)

#define MAXCLIENTS              (8)
#define SERVER_MAX_WORLD_AGE    (5000)              // wie viele ms heben wir für den client eine welt auf
#define MAX_WORLD_BACKLOG       (10*MAXCLIENTS)     // wie viele welten werden gespeichert

#define OUTGOING_BANDWIDTH      (1024*15)   // bytes/sec
#define CLIENT_UPDATERATE       (100)       // msec

