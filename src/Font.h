#pragma once

// Class to draw fixed bitmap fonts / ASCII only
// Let's keep things super simple here
class CFont
{
public:
    CFont(void);
    ~CFont(void);

    // Load a font from a texture id
    void          Init(const unsigned int textureid,
                       const unsigned int width_char,
                       const unsigned int height_char,
                       const unsigned int width_texture,
                       const unsigned int height_texture);

    void          Unload();

    void          DrawGL(float x, float y, float z, const char* text) const;

    int           GetWidth() { return m_width_char; } // height in px of one character
    int           GetHeight() { return m_height_char; } // height in px of one character

protected:
    unsigned int  m_texture;
    unsigned int  m_width_char;
    unsigned int  m_height_char;
    unsigned int  m_width;
    unsigned int  m_height;
};

