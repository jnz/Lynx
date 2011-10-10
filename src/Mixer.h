#pragma once

#include "WorldClient.h"

class CMixer
{
public:
    CMixer(CWorldClient* world);
    ~CMixer(void);

    bool Init();
    void Shutdown();

    void Update(const float dt, const uint32_t ticks);

protected:

private:
    CWorldClient* m_world;

    // Rule of three
    CMixer(const CMixer&);
    CMixer& operator=(const CMixer&);
};

