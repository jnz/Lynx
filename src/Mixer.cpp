#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "lynx.h"
#include "Mixer.h"
#include <math.h> //atan2

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

void CMixer::Shutdown()
{
    //Mix_HaltChannel(-1); // FIXME gehört das hier hin?
    Mix_CloseAudio();

#ifndef __linux
    while(Mix_Init(0))
        Mix_Quit();
#endif
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
				// Distance to sound source
                const vec3_t diff = localplayer->GetOrigin() - obj->GetOrigin();
                const float dist = std::min(diff.Abs(), SOUND_MAX_DIST);
                int volume = (int)(dist*255/SOUND_MAX_DIST);
				if(volume > 255)
					volume = 255;

				uint16_t angle; // 0-360 deg. for Mix_SetPosition
				vec3_t playerlook; // player is looking in this direction
				float fAlpha; // riwi to sound source
				float fBeta; // riwi look dir
				m_world->GetLocalController()->GetDir(&playerlook, NULL, NULL);

				fAlpha = atan2(diff.x, -diff.z);
				fBeta = atan2(playerlook.x, -playerlook.z);
				angle = (uint16_t)((fAlpha - fBeta)*180/lynxmath::PI);
				Mix_SetPosition(obj->GetSoundState()->cur_channel,
							    angle, (uint8_t)volume);
            }
        }
    }
}

