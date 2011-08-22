#include "lynx.h"
#include "Sound.h"
#include "Mixer.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CSound::CSound()
{
    m_chunk = NULL;
}

CSound::~CSound(void)
{
    Unload();
}

bool CSound::Load(const std::string& path)
{
    Mix_Chunk* chunk;
    chunk = Mix_LoadWAV(path.c_str());
    m_chunk = chunk;

    return (m_chunk != NULL);
}

void CSound::Unload()
{
    Mix_FreeChunk((Mix_Chunk*)m_chunk);
    m_chunk = NULL;
}

bool CSound::Play() const
{
    int result = Mix_PlayChannel(-1, (Mix_Chunk*)m_chunk, 0);
    // FIXME only for debug builds?
    if(result == -1)
        fprintf(stderr, "Failed to play sound\n");
    return (result != -1);
}

bool CSound::Play(sound_state_t* state) const
{
    assert(state);
    if(!state)
        return false;
    state->cur_channel = Mix_PlayChannel(-1, (Mix_Chunk*)m_chunk, 0);
    state->is_playing = (state->cur_channel != -1);
    // FIXME only for debug builds?
    if(state->is_playing == -1)
        fprintf(stderr, "Failed to play sound\n");

    return (state->is_playing != -1);
}

