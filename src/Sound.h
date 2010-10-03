#pragma once

#include <string>

struct sound_state_t
{
    sound_state_t() { init(); }
    void init()
    {
        cur_channel = -1;
        is_playing = 0;
    }

    int cur_channel;
    int is_playing;
};

class CSound
{
public:
    CSound();
    ~CSound(void);

    bool Load(const std::string& path);
    void Unload();

    void Play() const;
    void Play(sound_state_t* state) const;

private:
    void* m_chunk; // not type safe here. should point to something that  is readable by CMixer
};

