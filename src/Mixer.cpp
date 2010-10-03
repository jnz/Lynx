#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "lynx.h"
#include "Mixer.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CMixer::CMixer(CWorldClient* world)
{
    m_world = world;
}

CMixer::~CMixer(void)
{
    Shutdown();
}

bool CMixer::Init()
{
    int status;
    int flags = MIX_INIT_OGG;

    fprintf(stderr, "SDL_mixer Init... ");
    status = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if(status != 0)
    {
        fprintf(stderr, "SDL: Failed to init audio subsystem\n");
        return false;
    }

    status = Mix_Init(flags);
    if(status&flags != flags)
    {
        fprintf(stderr, "SDL_mixer: Failed to init SDL_mixer (%s)\n", Mix_GetError());
        return false;
    }

    // open 44.1KHz, signed 16bit, system byte order,
    //      stereo audio, using 1024 byte chunks
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
    {
        fprintf(stderr, "Mix_OpenAudio error: %s\n", Mix_GetError());
        exit(2);
    }

    return true;
}

void CMixer::Shutdown()
{
    //Mix_HaltChannel(-1); // FIXME gehört das hier hin?
    Mix_CloseAudio();

    while(Mix_Init(0))
        Mix_Quit();
}

void CMixer::Update(const float dt, const uint32_t ticks)
{
    OBJITER iter;
    CObj* obj;

    for(iter=m_world->ObjBegin();iter!=m_world->ObjEnd();iter++)
    {
        obj = (*iter).second;

        if(obj->GetSound() && !obj->GetSoundState()->is_playing)
        {
            obj->GetSound()->Play(obj->GetSoundState());
        }
    }

}

