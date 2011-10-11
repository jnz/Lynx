#include <stdio.h>
#include <assert.h>
#include "lynxsys.h"
#include <time.h>
#include "Server.h"
#include "GameZombie.h"
#include <SDL/SDL.h>

// <memory leak detection>
#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif
// </memory leak detection>

#define DEFAULT_LEVEL       "bath/bath.lbsp"

int main(int argc, char** argv)
{
    int svport = 9999;
    char* level = (char*)DEFAULT_LEVEL;

    if(argc > 1) // port
    {
        svport = atoi(argv[1]);
    }
    if(argc > 2) // level
    {
        level = argv[2];
    }
    fprintf(stderr, "Level: %s\n", level);
    srand((unsigned int)time(NULL));

    { // for dumpmemleak
    int run;
    float dt;
    uint32_t time, oldtime;
    uint32_t fpstimer, fpscounter=0;

    // Game Modules
    CWorld worldsv; // Model
    CServer server(&worldsv); // Controller
    CGameZombie svgame(&worldsv, &server); // Controller

    ((CSubject<EventNewClientConnected>*)&server)->AddObserver(&svgame);
    ((CSubject<EventClientDisconnected>*)&server)->AddObserver(&svgame);

    fprintf(stderr, "Starting Server at port: %i\n", svport);
    if(!server.Create(svport))
    {
        fprintf(stderr, "Failed to create server on port: %i\n", svport);
        return -1;
    }
    svgame.InitGame(level);
    fprintf(stderr, "Server running\n");

    run = 1;
    oldtime = fpstimer = CLynxSys::GetTicks();
    while(run)
    {
        time = CLynxSys::GetTicks();
        dt = 0.001f * (float)(time-oldtime);
        oldtime = time;
        fpscounter++;
        if(time - fpstimer > 1000.0f)
        {
            fpscounter = 0;
            fpstimer = time;
        }

        // Update Game Classes
        svgame.Update(dt, time);
        worldsv.Update(dt, time);
        server.Update(dt, time);

        // Limit to 20 fps
        // so my notebook fan is quiet :-)
        float dtrest = 1.0f/20.0f - dt;
        if(dtrest > 0.0f)
            dtrest = 0.0f;
        SDL_Delay((uint32_t)(dtrest * 1000.0f));
    }
    }
#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}


