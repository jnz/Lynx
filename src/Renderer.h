#pragma once

#include "WorldClient.h"

class CRenderer
{
public:
    CRenderer(CWorldClient* world);
    ~CRenderer(void);

    bool Init(int width, int height, int bpp, int fullscreen);
    void Shutdown();

    void Update(const float dt, const uint32_t ticks);

    // Public Stats (FIXME)
    int stat_obj_visible;
    int stat_obj_hidden;

protected:
    void UpdatePerspective();
    void DrawScene(const CFrustum& frustum, CWorld* world, int localctrlid);

    // Shader
    bool InitShader();
    unsigned int m_vshader;
    unsigned int m_fshader;
    unsigned int m_program;

private:
    int m_width, m_height;
    CWorldClient* m_world;
};
