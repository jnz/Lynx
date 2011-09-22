#pragma once

#include "Menu.h"
#include "ResourceManager.h"
#include "Font.h"

// this struct is passed from the main control (main.cpp)
// to the menu, so that the menu can create a game, join
// a game a.s.o.
struct menu_engine_callback_t
{
    bool (*menu_func_host) (const char *levelname, const int svport, const bool join_as_client);
    bool (*menu_func_join) (const char *svaddr, const int port);
    void (*menu_func_quit) ();
};

// every menu item has a func (function) value from here
// depending on the func value (see KeyEnter), some
// action is taken
typedef enum
{
    MENU_FUNC_HOST=1,
    MENU_FUNC_JOIN,
    MENU_FUNC_CREDITS,
    MENU_FUNC_QUIT,
    MENU_FUNC_START_SERVER_AND_CLIENT,
    MENU_FUNC_MAIN
} menu_function_t;

// how should the menu item behave?
// if the menu_item is a button, you can only press enter on it.
// if it is a text field, you can enter something
typedef enum
{
    MENU_BUTTON,
    MENU_TEXT_FIELD
} menu_item_type_t;

#define MENU_MAX_TEXT_LEN     64

// OpenGL texture + dimension
struct menu_bitmap_t
{
    unsigned int texture;
    float width;
    float height;
};

// every menu has several items
struct menu_item_t
{
    // constructor for button creation
    menu_item_t(menu_bitmap_t _bitmap,
                menu_item_type_t _type,
                menu_function_t _func,
                float _x,
                float _y) :
                       bitmap(_bitmap),
                       type(_type),
                       func(_func),
                       x(_x),
                       y(_y) { }
    // constructor for textfield creation (extra parameters for text and text
    // offset)
    menu_item_t(menu_bitmap_t _bitmap,
                menu_item_type_t _type,
                menu_function_t _func,
                float _x,
                float _y,
                const std::string _text,
                float _textoffset_x,
                float _textoffset_y) :
                       bitmap(_bitmap),
                       type(_type),
                       func(_func),
                       x(_x),
                       y(_y),
                       text(_text),
                       textoffset_x(_textoffset_x),
                       textoffset_y(_textoffset_y) { }

    menu_bitmap_t bitmap;
    menu_item_type_t type;
    menu_function_t func;
    float x;
    float y;
    std::string text; // only used for text field items
    float textoffset_x; // only used for text field items:
    float textoffset_y; // draw inner text with this offset. 10 px or something is reasonable
};

// every menu has items (something the user can interact with)
// and images (static textures)
// CMenu has a vector of menu_t. This is the main
// data structure here.
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
                       const unsigned int physical_height,
                       menu_engine_callback_t callback);

    void          Unload();

    void          Toggle() { m_visible = !m_visible; }
    bool          IsVisible() const { return m_visible; }
    void          MakeVisible() { m_visible = true; }
    void          MakeInvisible() { m_visible = false; }
    void          Update(const float dt, const uint32_t ticks);
    void          DrawDefaultBackground();

    void          KeyDown();
    void          KeyUp();
    void          KeyEnter();
    void          KeyEsc();
    void          KeyAscii(unsigned char val, bool shift, bool ctrl);
    void          KeyBackspace();

protected:
    void          RenderGL() const;

    void          DrawMenu() const;
    void          DrawFontVirtual(const std::string text,
                                  const float x,
                                  const float y) const;
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

    CFont             m_font; // To draw some text

private:
    CResourceManager* m_resman;

    menu_engine_callback_t m_callback; // engine flow interaction (e.g. quit game, join game etc.)
    bool              m_visible;
    unsigned int      m_phy_width; // physical game window width in pixel
    unsigned int      m_phy_height; // physical game window height in pixel

    // constant resources
    menu_bitmap_t     m_menu_background_cube; // for default background
    menu_bitmap_t     m_menu_background;
    menu_bitmap_t     m_menu_lynx;
    menu_bitmap_t     m_menu_bullet;

    std::vector<menu_t> m_menus;

    // animation
    float             m_animation; // time for animations
};

