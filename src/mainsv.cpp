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

int main(int argc, char** argv)
{
    int svport = 9999;

    if(argc > 1) // port
    {
        svport = atoi(argv[1]);
    }
    srand((unsigned int)time(NULL));

    { // for dumpmemleak
    int run;
    float dt;
    DWORD time, oldtime;
    DWORD fpstimer, fpscounter=0;
    SDL_Event event;
    
    // Game Modules
    CWorld worldsv; // Model
    CServer server(&worldsv); // Controller
    CGameZombie svgame(&worldsv, &server); // Controller

    ((CSubject<EventNewClientConnected>*)&server)->AddObserver(&svgame);
    ((CSubject<EventClientDisconnected>*)&server)->AddObserver(&svgame);

    fprintf(stderr, "Starting Server at port: %i\n", svport);
    if(!server.Create(svport))
    {
        fprintf(stderr, "Failed to create server\n");
        assert(0);
        return -1;
    }
    svgame.InitGame();
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

        SDL_Delay(15);
    }
    }
#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}


