#include "lynx.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include "ResourceManager.h"
#include "Font.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CFont::CFont(void)
{
    m_texture = 0;
}

CFont::~CFont(void)
{
    Unload();
}

void CFont::Unload()
{
    m_texture = 0;
}

void CFont::Init(const unsigned int textureid,
                 const unsigned int width_char,
                 const unsigned int height_char,
                 const unsigned int width_texture,
                 const unsigned int height_texture)
{
    glBindTexture(GL_TEXTURE_2D, textureid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_texture = textureid;
    m_width_char = width_char;
    m_height_char = height_char;
    m_width = width_texture;
    m_height = height_texture;
}

bool CFont::Init(const std::string texturepath,
                 const unsigned int width_char,
                 const unsigned int height_char,
                 CResourceManager* resman)
{
    const std::string fontpath = CLynx::GetBaseDirTexture() + texturepath;
    unsigned int fonttexwidth, fonttexheight;
    unsigned int fonttex = resman->GetTexture(fontpath, false);
    if(!fonttex)
    {
        assert(0);
        return false;
    }
    resman->GetTextureDimension(fontpath,
                                &fonttexwidth,
                                &fonttexheight);

    Init(fonttex, width_char, height_char, fonttexwidth, fonttexheight);
    return true;
}

void CFont::DrawGL(float x, float y, float z, const char* text) const
{
    const float width = (float)m_width_char; // in pixel
    const float height = (float)m_height_char; // in pixel
    const int chars_per_line = m_width / m_width_char;
    const float u = (float)(m_width_char)/(float)m_width; // from 0..1
    const float v = (float)(m_height_char)/(float)m_height; // from 0..1

    glColor4f(1,1,1,1);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glBegin(GL_QUADS);

    while(*text)
    {
        const uint8_t ascii = *text;

        const float top = (uint8_t((ascii - 32) / chars_per_line)) * v;
        const float left = (uint8_t((ascii - 32) % chars_per_line)) * u;
        const float eps = 0.5f/height;

        glTexCoord2d(left, 1.0f - top);
        glVertex3f(x, y, z);

        glTexCoord2d(left, 1.0f - top - v + eps);
        glVertex3f(x, y+height, z);

        glTexCoord2d(left + u, 1.0f - top - v + eps);
        glVertex3f(x+width, y+height, z);

        glTexCoord2d(left + u, 1.0f - top);
        glVertex3f(x+width, y, z);

        x += width;
        text++;
    }

    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
}

