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
#include "Menu.h"
#include "GameZombie.h"

// <memory leak detection>
#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif
// </memory leak detection>

#define WINDOW_TITLE        "Lynx"
#define SCREEN_WIDTH        1024
#define SCREEN_HEIGHT       768
#define BPP                 32
#define FULLSCREEN          0
#define DEFAULT_LEVEL       "testlvl/level1.lbsp"
//#define DEFAULT_LEVEL       "sponza/sponza.lbsp"

int main(int argc, char** argv)
{
    char* serveraddress = (char*)"localhost";
    int svport = 9999;
    bool startserver = true;
    char* level = (char*)DEFAULT_LEVEL;

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
    if(argc > 3) // level
    {
        level = argv[3];
        fprintf(stderr, "Level: %s", level);
    }
    srand((unsigned int)time(NULL));

    { // the { is for the win32 visual studio dumpmemleak at the end
    int run;
    float dt;
    uint32_t time, oldtime;
    uint32_t fpstimer, fpscounter=0;
    SDL_Event event;

    // Game Modules
    CMenu menu; // menu

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
        if(!svgame.InitGame(level))
        {
            fprintf(stderr, "Failed to init game\n");
            assert(0);
            return -1;
        }
        fprintf(stderr, "Server running\n");
    }

    // Startup renderer and mixer, before connecting to the client
    // so we can give some feedback to the player while connecting
    if(!renderer.Init(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, FULLSCREEN))
    {
        assert(0);
        return -1;
    }

    // Init sound mixer
    mixer.Init(); // we don't care, if it won't init

    // init menu
    if(!menu.Init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        fprintf(stderr, "Failed to load menu\n");
        assert(0);
        return -1;
    }

    fprintf(stderr, "Connecting to %s:%i\n", serveraddress, svport);
    if(!client.Connect(serveraddress, svport))
    {
        fprintf(stderr, "Failed to connect to server\n");
        assert(0);
        return -1;
    }

    bool should_we_quit = false; // set by CMenu functions

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
                    menu.KeyEsc();
                    break;
                case SDLK_F10:
                    run = 0;
                    break;
                case SDLK_DOWN:
                    menu.KeyDown();
                    break;
                case SDLK_UP:
                    menu.KeyUp();
                    break;
                case SDLK_RETURN:
                    menu.KeyEnter(&should_we_quit);
                    if(should_we_quit)
                        run = 0;
                    break;
                default:
                    break;
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
        mixer.Update(dt, time);

        if(startserver)
        {
            worldsv.Update(dt, time);
            svgame.Update(dt, time);
            server.Update(dt, time);
        }
        if(client.IsInGame())
        {
            worldcl.Update(dt, time);
        }

        client.Update(dt, time);
        renderer.Update(dt, time);
        menu.Update(dt, time);
        renderer.SwapBuffer();

        // so my notebook fan is quiet :-)
        const float dtrest = 1.0f/60.0f - dt;
        if(dtrest > 0.0f)
            SDL_Delay((uint32_t)(dtrest * 1000.0f));

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

	//SDLK_UNKNOWN              = 0,
	//SDLK_FIRST                = 0,
	//SDLK_BACKSPACE            = 8,
	//SDLK_TAB                  = 9,
	//SDLK_CLEAR                = 12,
	//SDLK_RETURN               = 13,
	//SDLK_PAUSE                = 19,
	//SDLK_ESCAPE               = 27,
	//SDLK_SPACE                = 32,
	//SDLK_EXCLAIM              = 33,
	//SDLK_QUOTEDBL             = 34,
	//SDLK_HASH                 = 35,
	//SDLK_DOLLAR               = 36,
	//SDLK_AMPERSAND            = 38,
	//SDLK_QUOTE                = 39,
	//SDLK_LEFTPAREN            = 40,
	//SDLK_RIGHTPAREN           = 41,
	//SDLK_ASTERISK             = 42,
	//SDLK_PLUS                 = 43,
	//SDLK_COMMA                = 44,
	//SDLK_MINUS                = 45,
	//SDLK_PERIOD               = 46,
	//SDLK_SLASH                = 47,
	//SDLK_0                    = 48,
	//SDLK_1                    = 49,
	//SDLK_2                    = 50,
	//SDLK_3                    = 51,
	//SDLK_4                    = 52,
	//SDLK_5                    = 53,
	//SDLK_6                    = 54,
	//SDLK_7                    = 55,
	//SDLK_8                    = 56,
	//SDLK_9                    = 57,
	//SDLK_COLON                = 58,
	//SDLK_SEMICOLON            = 59,
	//SDLK_LESS                 = 60,
	//SDLK_EQUALS               = 61,
	//SDLK_GREATER              = 62,
	//SDLK_QUESTION             = 63,
	//SDLK_AT                   = 64,
	//SDLK_LEFTBRACKET          = 91,
	//SDLK_BACKSLASH            = 92,
	//SDLK_RIGHTBRACKET         = 93,
	//SDLK_CARET                = 94,
	//SDLK_UNDERSCORE           = 95,
	//SDLK_BACKQUOTE            = 96,
	//SDLK_a                    = 97,
	//SDLK_b                    = 98,
	//SDLK_c                    = 99,
	//SDLK_d                    = 100,
	//SDLK_e                    = 101,
	//SDLK_f                    = 102,
	//SDLK_g                    = 103,
	//SDLK_h                    = 104,
	//SDLK_i                    = 105,
	//SDLK_j                    = 106,
	//SDLK_k                    = 107,
	//SDLK_l                    = 108,
	//SDLK_m                    = 109,
	//SDLK_n                    = 110,
	//SDLK_o                    = 111,
	//SDLK_p                    = 112,
	//SDLK_q                    = 113,
	//SDLK_r                    = 114,
	//SDLK_s                    = 115,
	//SDLK_t                    = 116,
	//SDLK_u                    = 117,
	//SDLK_v                    = 118,
	//SDLK_w                    = 119,
	//SDLK_x                    = 120,
	//SDLK_y                    = 121,
	//SDLK_z                    = 122,
	//SDLK_DELETE               = 127,
	//SDLK_UP                   = 273,
	//SDLK_DOWN                 = 274,
	//SDLK_RIGHT                = 275,
	//SDLK_LEFT                 = 276,
	//SDLK_INSERT               = 277,
	//SDLK_HOME                 = 278,
	//SDLK_END                  = 279,
	//SDLK_PAGEUP               = 280,
	//SDLK_PAGEDOWN             = 281,
	//SDLK_F1                   = 282,
	//SDLK_F2                   = 283,
	//SDLK_F3                   = 284,
	//SDLK_F4                   = 285,
	//SDLK_F5                   = 286,
	//SDLK_F6                   = 287,
	//SDLK_F7                   = 288,
	//SDLK_F8                   = 289,
	//SDLK_F9                   = 290,
	//SDLK_F10                  = 291,
	//SDLK_F11                  = 292,
	//SDLK_F12                  = 293,
	//SDLK_RSHIFT               = 303,
	//SDLK_LSHIFT               = 304,
	//SDLK_RCTRL                = 305,
	//SDLK_LCTRL                = 306,
	//SDLK_RALT                 = 307,
	//SDLK_LALT                 = 308,
	//SDLK_RMETA                = 309,
	//SDLK_LMETA                = 310,
	//SDLK_LSUPER               = 311,		[>*< Left "Windows" key <]
	//SDLK_RSUPER               = 312,		[>*< Right "Windows" key <]
	//SDLK_MODE                 = 313,		[>*< "Alt Gr" key <]
