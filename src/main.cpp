#include <stdio.h>
#include <assert.h>
#include <SDL/SDL.h>
#include "lynxsys.h"
#include <time.h>
#include "Renderer.h"
#include "Mixer.h"
#include "Server.h"
#include "Client.h"
#include "WorldClient.h"
#include "GameZombie.h"

// <memory leak detection>
#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif
// </memory leak detection>

#define WINDOW_TITLE        "Lynx"
#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       600
#define BPP                 32
#define FULLSCREEN          0

int main(int argc, char** argv)
{
    char* serveraddress = (char*)"localhost";
    int svport = 9999;
    bool startserver = true;

    fprintf(stderr, "%s version %i.%i\n", LYNX_TITLE, LYNX_MAJOR, LYNX_MINOR);
    if(argc > 1) // connect to this server, disable local server
    {
        serveraddress = argv[1];
        startserver = false;
    }
    if(argc > 2) // port
    {
        svport = atoi(argv[2]);
    }
    srand((unsigned int)time(NULL));

    { // for dumpmemleak
    int run;
    float dt;
    uint32_t time, oldtime;
    uint32_t fpstimer, fpscounter=0;
    SDL_Event event;

    // Game Modules
    CWorldClient worldcl; // Model
    CRenderer renderer(&worldcl); // View
    CMixer mixer(&worldcl); // View
    CGameZombie clgame(&worldcl, NULL); // Controller
    CClient client(&worldcl, &clgame); // Controller

    CWorld worldsv; // Model
    CServer server(&worldsv); // Controller
    CGameZombie svgame(&worldsv, &server); // Controller

    if(startserver)
    {
        ((CSubject<EventNewClientConnected>*)&server)->AddObserver(&svgame);
        ((CSubject<EventClientDisconnected>*)&server)->AddObserver(&svgame);

        fprintf(stderr, "Starting Server at port: %i\n", svport);
        if(!server.Create(svport))
        {
            fprintf(stderr, "Failed to create server\n");
            assert(0);
            return -1;
        }
        if(!svgame.InitGame())
        {
            fprintf(stderr, "Failed to init game\n");
            assert(0);
            return -1;
        }
        fprintf(stderr, "Server running\n");
    }

    fprintf(stderr, "Connecting to %s:%i\n", serveraddress, svport);
    if(!client.Connect(serveraddress, svport))
    {
        fprintf(stderr, "Failed to connect to server\n");
        assert(0);
        return -1;
    }

    mixer.Init(); // we don't care, if it won't init

    if(!renderer.Init(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, FULLSCREEN))
        return -1;

    SDL_WM_SetCaption(WINDOW_TITLE, NULL);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_GrabInput(SDL_GRAB_ON);
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
            char title[128];
            sprintf(title, "lynx (FPS: %i) vis: %i/%i",
                (int)fpscounter, renderer.stat_obj_visible, renderer.stat_obj_hidden);
            SDL_WM_SetCaption(title, NULL);
            fpscounter = 0;
            fpstimer = time;
        }

        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    run = 0;
                }
                break;
            case SDL_QUIT:
                run = 0;
                break;
            default:
                break;
            };
        }

        // Update Game Classes
        if(startserver)
        {
            worldsv.Update(dt, time);
            svgame.Update(dt, time);
            server.Update(dt, time);
        }
        if(client.IsConnected())
        {
            worldcl.Update(dt, time);
        }
        client.Update(dt, time);
        renderer.Update(dt, time);
        mixer.Update(dt, time);

        // so my notebook fan is quiet :-)
        const float dtrest = 1.0f/60.0f - dt;
        if(dtrest > 0.0f)
            SDL_Delay(dtrest * 1000);

        if(!client.IsRunning())
        {
            fprintf(stderr, "Disconnected\n");
            run = 0;
        }
    }
    }
#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}

