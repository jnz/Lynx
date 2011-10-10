#pragma once

#include <string>

struct sound_state_t
{
    sound_state_t() { init(); }
    void init()
    {
        cur_channel = -1;
        is_playing = 0;
        soundpath = "";
    }

    int cur_channel;
    int is_playing;
    std::string soundpath;
};

class CSound
{
public:
    CSound();
    ~CSound(void);

    bool Load(const std::string& path);
    void Unload();

    bool Play() const;
    bool Play(sound_state_t* state) const;

private:
    void* m_chunk; // not type safe here. should point to something that  is readable by CMixer

    // Rule of three
    CSound(const CSound&);
    CSound& operator=(const CSound&);
};

