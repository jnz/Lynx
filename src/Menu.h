#pragma once

#include "Menu.h"
#include "ResourceManager.h"

typedef enum
{
    MENU_FUNC_HOST=1,
    MENU_FUNC_JOIN,
    MENU_FUNC_CREDITS,
    MENU_FUNC_QUIT,
    MENU_FUNC_MAIN
} menu_function_t;

typedef enum
{
    MENU_BUTTON,
    MENU_TEXT_FIELD
} menu_item_type_t;

struct menu_bitmap_t
{
    unsigned int texture;
    unsigned int width;
    unsigned int height;
};

struct menu_item_t
{
    // constructor for easy creation
    menu_item_t(menu_bitmap_t _bitmap,
                menu_item_type_t _type,
                menu_function_t _func,
                float _x,
                float _y) :
                       bitmap(_bitmap),
                       type(_type),
                       func(_func),
                       x(_x),
                       y(_y) {}

    menu_bitmap_t bitmap;
    menu_item_type_t type;
    menu_function_t func;
    float x;
    float y;
};

struct menu_t
{
    std::vector<menu_item_t> items;  // selectable items
    std::vector<menu_item_t> images; // static images
    int parent; // parent menu index, or -1 if main menu
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
    void          KeyEnter(bool* should_we_quit);
    void          KeyEsc();

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

    // constant resources
    menu_bitmap_t     m_menu_background;
    menu_bitmap_t     m_menu_lynx;
    menu_bitmap_t     m_menu_bullet;

    std::vector<menu_t> m_menus;
};

