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
    void DrawScene(const CFrustum& frustum, 
                   CWorld* world, 
                   int localctrlid,
                   bool generateShadowMap);

    // Shader
    bool InitShader();
    unsigned int m_vshader;
    unsigned int m_fshader;
    unsigned int m_program;
    unsigned int m_tex; // tex0
    unsigned int m_normalMap; // tex1

    // Shadow
    bool         m_useShadows;
    unsigned int m_shadowMapUniform;    // variable in fragment shader to access shadowmap
    unsigned int m_fboId;				// framebuffer object to render light source view to
    unsigned int m_depthTextureId;	    // this texture is bound to the fbo
    bool         CreateShadowFBO();     // Create framebuffer object
    void         PrepareShadowMap(const vec3_t& lightpos, 
                                  const quaternion_t& lightrot,
                                  CWorld* world, int localctrlid);

private:
    int m_width, m_height;
    CWorldClient* m_world;
};

