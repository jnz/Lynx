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
    return true;
}

void CMixer::Shutdown()
{
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

