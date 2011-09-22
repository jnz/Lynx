#include <stdio.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
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

bool restartserver(CWorld** worldsv,
                   CServer** server,
                   CGameZombie** game,
                   const int port,
                   const char* level);

bool clconnect(CWorldClient** worldcl,
               CRenderer** renderer,
               CMixer** mixer,
               CGameZombie** clgame,
               CClient** client,
               const char* serveraddress,
               const int serverport);

void shutdown(CWorld** worldsv,
              CServer** server,
              CGameZombie** game,
              CWorldClient** worldcl,
              CRenderer** renderer,
              CMixer** mixer,
              CGameZombie** clgame,
              CClient** client);

bool initSDLMixer();
void shutdownSDLMixer();
bool initSDLvideo(int width, int height, int bpp, int fullscreen);

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

    { // the { is for the visual studio dumpmemleak (memory leak detection) at the end
    int run;
    float dt;
    uint32_t time, oldtime;
    uint32_t fpstimer, fpscounter=0;
    SDL_Event event;

    // Game Modules
    CMenu menu; // lynx menu

    CWorldClient* worldcl = NULL; // Model
    CRenderer* renderer   = NULL; // View
    CMixer* mixer         = NULL; // View
    CGameZombie* clgame   = NULL; // Controller
    CClient* client       = NULL; // Controller

    CWorld* worldsv       = NULL; // Model
    CServer* server       = NULL; // Controller
    CGameZombie* svgame   = NULL; // Controller

    if(startserver)
    {
        restartserver(&worldsv, &server, &svgame, svport, level);
    }

    // Startup SDL OpenGL window
    if(!initSDLvideo(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, FULLSCREEN))
    {
        assert(0);
        return -1;
    }

    // Init sound mixer
    initSDLMixer(); // we don't care, if it won't init. No sound for you.

    // init menu
    if(!menu.Init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        fprintf(stderr, "Failed to load menu\n");
        assert(0);
        return -1;
    }

    if(!clconnect(&worldcl,
                  &renderer,
                  &mixer,
                  &clgame,
                  &client,
                  serveraddress,
                  svport))
    {
        fprintf(stderr, "Failed to connect\n");
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
            sprintf(title, "lynx (FPS: %i)", (int)fpscounter);
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

        // Update Client
        if(worldcl)
        {
            mixer->Update(dt, time);
            if(client->IsInGame())
            {
                worldcl->Update(dt, time);
            }
            client->Update(dt, time);
            renderer->Update(dt, time);

            if(!client->IsRunning())
            {
                fprintf(stderr, "Disconnected\n");
                if(!menu.IsVisible())
                    menu.Toggle();
            }
        }

        // Update Server
        if(worldsv)
        {
            worldsv->Update(dt, time);
            svgame->Update(dt, time);
            server->Update(dt, time);
        }

        // Update Menu
        menu.Update(dt, time);

        // Draw GL buffer
        SDL_GL_SwapBuffers();

        // Limit to 60 fps
        // so my notebook fan is quiet :-)
        const float dtrest = 1.0f/60.0f - dt;
        if(dtrest > 0.0f)
            SDL_Delay((uint32_t)(dtrest * 1000.0f));
    }

    shutdownSDLMixer();
    shutdown(&worldsv,
             &server,
             &svgame,
             &worldcl,
             &renderer,
             &mixer,
             &clgame,
             &client);
    }
#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}

void shutdown(CWorld** worldsv,
              CServer** server,
              CGameZombie** game,
              CWorldClient** worldcl,
              CRenderer** renderer,
              CMixer** mixer,
              CGameZombie** clgame,
              CClient** client)
{
    if(*client)
        delete *client;
    if(*clgame)
        delete *clgame;
    if(*renderer)
        delete *renderer;
    if(*mixer)
        delete *mixer;
    if(*worldcl)
        delete *worldcl;
    if(*game)
        delete *game;
    if(*server)
        delete *server;
    if(*worldsv)
        delete *worldsv;
}

bool restartserver(CWorld** worldsv,
                   CServer** server,
                   CGameZombie** game,
                   const int port,
                   const char* level)
{
    if(*game)
        delete *game;
    if(*server)
        delete *server;
    if(*worldsv)
        delete *worldsv;

    *worldsv = new CWorld;
    *server = new CServer(*worldsv);
    *game = new CGameZombie(*worldsv, *server);

    ((CSubject<EventNewClientConnected>*)*server)->AddObserver(*game);
    ((CSubject<EventClientDisconnected>*)*server)->AddObserver(*game);

    fprintf(stderr, "Starting Server at port: %i\n", port);
    if(!(*server)->Create(port))
    {
        fprintf(stderr, "Failed to create server\n");
        return false;
    }
    if(!(*game)->InitGame(level))
    {
        fprintf(stderr, "Failed to init game\n");
        return false;
    }
    fprintf(stderr, "Server running\n");
    return true;
}

bool clconnect(CWorldClient** worldcl,
               CRenderer** renderer,
               CMixer** mixer,
               CGameZombie** clgame,
               CClient** client,
               const char* serveraddress,
               const int serverport)
{
    if(*client)
        delete *client;
    if(*clgame)
        delete *clgame;
    if(*renderer)
        delete *renderer;
    if(*mixer)
        delete *mixer;
    if(*worldcl)
        delete *worldcl;

    *worldcl  = new CWorldClient;
    *renderer = new CRenderer(*worldcl);
    *mixer    = new CMixer(*worldcl);
    *clgame   = new CGameZombie(*worldcl, NULL);
    *client   = new CClient(*worldcl, *clgame);

    (*mixer)->Init();
    (*renderer)->Init(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, FULLSCREEN);

    fprintf(stderr, "Connecting to %s:%i\n", serveraddress, serverport);
    if(!(*client)->Connect(serveraddress, serverport))
    {
        fprintf(stderr, "Failed to connect to server\n");
        return false;
    }

    return true;
}

bool initSDLvideo(int width, int height, int bpp, int fullscreen)
{
    int status;
    SDL_Surface* screen;

    fprintf(stderr, "Initialising OpenGL subsystem...\n");
    status = SDL_InitSubSystem(SDL_INIT_VIDEO);
    assert(status == 0);
    if(status)
    {
        fprintf(stderr, "Failed to init SDL OpenGL subsystem!\n");
        return false;
    }

    screen = SDL_SetVideoMode(width, height, bpp,
                SDL_HWSURFACE |
                SDL_ANYFORMAT |
                SDL_DOUBLEBUF |
                SDL_OPENGL |
                (fullscreen ? SDL_FULLSCREEN : 0));
    assert(screen);
    if(!screen)
    {
        fprintf(stderr, "Failed to set screen resolution mode: %i x %i", width, height);
        return false;
    }

    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
        fprintf(stderr, "GLEW init error: %s\n", glewGetErrorString(err));
        return false;
    }

    if(glewIsSupported("GL_VERSION_2_0"))
    {
        fprintf(stderr, "OpenGL 2.0 support found\n");
    }
    else
    {
        fprintf(stderr, "Fatal error: No OpenGL 2.0 support\n");
        return false;
    }

    glClearColor(0.48f, 0.58f, 0.72f, 0.0f); // cornflower blue
    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    return true;
}

bool initSDLMixer()
{
    int status;

    fprintf(stderr, "Sound mixer init...\n");
    status = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if(status != 0)
    {
        fprintf(stderr, "SDL_mixer: Failed to init audio subsystem\n");
        return false;
    }

#ifndef __linux
    int flags = MIX_INIT_OGG;
    status = Mix_Init(flags);
    if((status&flags) != flags)
    {
        fprintf(stderr, "SDL_mixer: Failed to init SDL_mixer (%s)\n", Mix_GetError());
        return false;
    }
#endif

    // open 44.1KHz, signed 16bit, system byte order,
    //      stereo audio, using 1024 byte chunks
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
    {
        fprintf(stderr, "Mix_OpenAudio error: %s\n", Mix_GetError());
        exit(2);
    }

    return true;
}

void shutdownSDLMixer()
{
    Mix_CloseAudio();

#ifndef __linux
    while(Mix_Init(0))
        Mix_Quit();
#endif
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
