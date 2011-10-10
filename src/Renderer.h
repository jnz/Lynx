#pragma once

#include "WorldClient.h"
#include "Font.h"

class CRenderer
{
public:
    CRenderer(CWorldClient* world);
    ~CRenderer(void);

    bool Init(int width, int height, int bpp, int fullscreen);
    void Shutdown();

    void Update(const float dt, const uint32_t ticks);

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
    unsigned int m_lightmap; // tex2
    unsigned int m_uselightmap; // shader variable uniform int
    int          m_lightmapactive; // on/off switch for lightmapping
    int          m_shaderactive; // on/off switch for GLSL shader

    // Shadow
    bool         m_useShadows;
    unsigned int m_shadowMapUniform;    // variable in fragment shader to access shadowmap
    unsigned int m_fboId;				// framebuffer object to render light source view to
    unsigned int m_depthTextureId;	    // this texture is bound to the fbo
    bool         CreateShadowFBO();     // Create framebuffer object
    void         PrepareShadowMap(const vec3_t& lightpos,
                                  const quaternion_t& lightrot,
                                  CWorld* world, int localctrlid);
    // Crosshair
    unsigned int m_crosshair; // crosshair texture
    unsigned int m_crosshair_width;
    unsigned int m_crosshair_height;

    // Font
    CFont        m_font;

private:
    int m_width, m_height;
    CWorldClient* m_world;

    // Rule of three
    CRenderer(const CRenderer&);
    CRenderer& operator=(const CRenderer&);
};

