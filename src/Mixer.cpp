#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "lynx.h"
#include "Mixer.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define SOUND_MAX_DIST      (120.0f)         // max. 120 m distance to hear the sound

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
    bool success;

    for(iter=m_world->ObjBegin();iter!=m_world->ObjEnd();iter++)
    {
        obj = (*iter).second;

        if(obj->GetSound() && !obj->GetSoundState()->is_playing)
        {
            success = obj->GetSound()->Play(obj->GetSoundState());

            CObj* localplayer = m_world->GetLocalObj();
            if(success && localplayer)
            {
                const vec3_t diff = localplayer->GetOrigin() - obj->GetOrigin();
                const float dist = std::min(diff.Abs(), SOUND_MAX_DIST);
                // y = m*x + b
                // MAX = m*0 + b
                // MIN = m*MAXDIST + b
                // b = MAX_VOLUME
                // 0 = m*MAXDIST + MAX_VOLUME
                // m = -MAX_VOLUME/MAXDIST
                int volume = (int)(-dist*MIX_MAX_VOLUME/SOUND_MAX_DIST) + MIX_MAX_VOLUME;

                Mix_Volume(obj->GetSoundState()->cur_channel, volume);
            }
        }
    }

}

