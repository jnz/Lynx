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
#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       600
#define BPP                 32
#define FULLSCREEN          0
#define SV_PORT             9999
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

bool menu_func_host(const char *levelname, const int svport, const bool join_as_client);
bool menu_func_join(const char *svaddr, const int port);
void menu_func_quit();

bool initSDLMixer();
void shutdownSDLMixer();
bool initSDLvideo(int width, int height, int bpp, int fullscreen);

// Global Game Components
// These components are the core of the Lynx engine
// ----------------------------------------------------
// Menu
CMenu g_menu;

// Client
CWorldClient* g_worldcl = NULL; // Model
CRenderer* g_renderer   = NULL; // View
CMixer* g_mixer         = NULL; // View
CGameZombie* g_clgame   = NULL; // Controller
CClient* g_client       = NULL; // Controller

// Server
CWorld* g_worldsv       = NULL; // Model
CServer* g_server       = NULL; // Controller
CGameZombie* g_svgame   = NULL; // Controller

int g_run; // the app runs, as long as this is not zero
// ----------------------------------------------------

int main(int argc, char** argv)
{
    fprintf(stderr, "%s version %i.%i\n", LYNX_TITLE, LYNX_MAJOR, LYNX_MINOR);
    srand((unsigned int)time(NULL));

    { // the { is for the visual studio dumpmemleak (memory leak detection) at the end
    float dt;
    uint32_t time, oldtime;
    uint32_t fpstimer, fpscounter=0;
    SDL_Event event;

    // Game Modules
    // Startup SDL OpenGL window
    if(!initSDLvideo(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, FULLSCREEN))
    {
        assert(0);
        return -1;
    }
    SDL_WM_SetCaption(WINDOW_TITLE, NULL);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_GrabInput(SDL_GRAB_ON);

    // init menu
    menu_engine_callback_t callback;
    memset(&callback, 0, sizeof(callback));
    callback.menu_func_host = menu_func_host;
    callback.menu_func_join = menu_func_join;
    callback.menu_func_quit = menu_func_quit;
    if(!g_menu.Init(SCREEN_WIDTH, SCREEN_HEIGHT, callback))
    {
        fprintf(stderr, "Failed to load menu\n");
        assert(0);
        return -1;
    }
    // Draw the menu once, before we continue
    g_menu.DrawDefaultBackground();
    g_menu.Update(0.0f, 0);
    SDL_GL_SwapBuffers();

    // Init sound mixer
    initSDLMixer(); // we don't care, if it won't init. No sound for you.

    g_run = 1;
    oldtime = fpstimer = CLynxSys::GetTicks();
    while(g_run)
    {
        time = CLynxSys::GetTicks();
        dt = 0.001f * (float)(time-oldtime);
        oldtime = time;
        fpscounter++;
        if(time - fpstimer > 1000.0f)
        {
            char title[128];
            sprintf(title, "Lynx (FPS: %i)", (int)fpscounter);
            SDL_WM_SetCaption(title, NULL);
            fpscounter = 0;
            fpstimer = time;
        }

        // Update Server
        if(g_worldsv)
        {
            g_worldsv->Update(dt, time);
            g_svgame->Update(dt, time);
            g_server->Update(dt, time);
        }

        // Update Client
        if(g_worldcl)
        {
            g_mixer->Update(dt, time);
            if(g_client->IsInGame())
                g_worldcl->Update(dt, time);
            g_client->Update(dt, time);
            g_renderer->Update(dt, time);

            if(!g_client->IsRunning())
            {
                fprintf(stderr, "Disconnected\n");
                g_menu.MakeVisible(); // FIXME goto error screen
            }
        }
        else
        {
            // We have no gameplay background, let's clear the
            // menu screen
            g_menu.DrawDefaultBackground();
        }

        // Update Menu
        g_menu.Update(dt, time);

        // Draw GL buffer
        SDL_GL_SwapBuffers();

        // Limit to 60 fps
        // so my notebook fan is quiet :-)
        const float dtrest = 1.0f/60.0f - dt;
        if(dtrest > 0.0f)
            SDL_Delay((uint32_t)(dtrest * 1000.0f));

        // Handle system events
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym < 128)
                    {
                        // let KeyAscii figure out if it wants the key:
                        g_menu.KeyAscii(event.key.keysym.sym,
                                        event.key.keysym.mod & KMOD_SHIFT,
                                        event.key.keysym.mod & KMOD_CTRL);
                    }
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            g_menu.KeyEsc();
                            break;
                        case SDLK_F10:
                            g_run = 0;
                            break;
                        case SDLK_DOWN:
                            g_menu.KeyDown();
                            break;
                        case SDLK_UP:
                            g_menu.KeyUp();
                            break;
                        case SDLK_RETURN:
                            g_menu.KeyEnter();
                            break;
                        case SDLK_BACKSPACE:
                            g_menu.KeyBackspace();
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_QUIT:
                    g_run = 0;
                    break;
                default:
                    break;
            };
        }
    }

    shutdownSDLMixer();
    shutdown(&g_worldsv,
             &g_server,
             &g_svgame,
             &g_worldcl,
             &g_renderer,
             &g_mixer,
             &g_clgame,
             &g_client);
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

    //glClearColor(0.48f, 0.58f, 0.72f, 0.0f); // cornflower blue
    glClearColor(0.85f, 0.85f, 0.85f, 0.0f);
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

bool menu_func_host(const char *levelname, const int svport, const bool join_as_client)
{
    char* lvl = (char*)levelname;
    int port = svport;

    // swap the buffer, so that the menu can display
    // it's loading screen
    SDL_GL_SwapBuffers();

    // use default level and port
    if(lvl == NULL)
        lvl = (char*)DEFAULT_LEVEL;
    if(port == 0)
        port = SV_PORT;

    if(!restartserver(&g_worldsv, &g_server, &g_svgame, port, lvl))
        return false;

    if(join_as_client)
        return menu_func_join("localhost", port);
    else
        return true;
}

bool menu_func_join(const char *svaddr, const int port)
{

    if(!clconnect(&g_worldcl,
                  &g_renderer,
                  &g_mixer,
                  &g_clgame,
                  &g_client,
                  svaddr,
                  port))
    {
        fprintf(stderr, "Failed to connect\n");
        return false;
    }

    return true;
}

void menu_func_quit()
{
    g_run = 0;
}

