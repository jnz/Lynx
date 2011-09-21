#pragma once

#include "Menu.h"
#include "ResourceManager.h"

struct menu_bitmap_t
{
    unsigned int texture;
    unsigned int width;
    unsigned int height;
};

class CMenu
{
public:
    CMenu(void);
    ~CMenu(void);

    bool          Init(const unsigned int physical_width,
                       const unsigned int physical_height);

    void          Unload();

    void          Toggle() { m_visible = !m_visible; }
    bool          IsVisible() const { return m_visible; }
    void          Update(const float dt, const uint32_t ticks);

    virtual void  RenderGL() const;

    void          KeyDown();
    void          KeyUp();
    void          KeyEnter();

protected:
    void          DrawMenu() const;
    void          DrawRectVirtual(const menu_bitmap_t& bitmap,
                                  const float x,
                                  const float y) const; // aligned on a virtual screen (800x600)
    void          DrawRect(const unsigned int texture,
                           const float x,
                           const float y,
                           const float width,
                           const float height) const;

    int           LoadBitmap(const std::string path, menu_bitmap_t* bitmap); // returns 0 if loading was successful

    CResourceManager* GetResMan() { return m_resman; }

    int               m_cur_menu; // current menu
    int               m_cursor; // current selected element
private:
    CResourceManager* m_resman;

    bool              m_visible;
    unsigned int      m_phy_width; // physical game window width in pixel
    unsigned int      m_phy_height; // physical game window height in pixel
};

