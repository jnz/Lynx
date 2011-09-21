#include "lynx.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include "Menu.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

//-Menu Global Stuff----------------------------------------------------

#define VIRTUAL_WIDTH  800
#define VIRTUAL_HEIGHT 600

typedef enum
{
    MENU_FUNC_QUIT,
    MENU_FUNC_JOIN,
    MENU_FUNC_HOST,
    MENU_FUNC_CREDITS
} menu_function_t;

typedef enum
{
    MENU_BUTTON,
    MENU_TEXT_FIELD
} menu_item_type_t;

struct menu_item_t
{
    char* bitmappath;
    menu_bitmap_t* bitmap;
    menu_item_type_t type;
    menu_function_t func;
};

struct menu_t
{
    //menu_item_t items[];
    int itemcount;
};

enum
{
    MENU_MAIN,
    MENU_CONNECT,
    MENU_CREDITS,
};

menu_bitmap_t g_menu_background;
menu_bitmap_t g_menu_lynx;

menu_bitmap_t g_menu_bullet;

menu_bitmap_t g_menu0_host;
menu_bitmap_t g_menu0_join;
menu_bitmap_t g_menu0_credits;
menu_bitmap_t g_menu0_quit;

//menu_item_t g_main_menu_items[] =
//{
    //{"path", NULL, MENU_BUTTON, MENU_FUNC_HOST},
    //{"path", NULL, MENU_BUTTON, MENU_FUNC_JOIN}
//};

//menu_t g_main_menu[] =
//{
    //g_main_menu_items,
    //4
//};

//menu_t g_menus[] =
//{
    //g_main_menu
//};

#define MENU_BUTTONS_OFFSET_X    275.0f
#define MENU_BUTTONS_OFFSET_Y    235.0f
#define MENU_BUTTONS_SPACE       1.0f

//----------------------------------------------------------------------

CMenu::CMenu(void)
{
    m_resman = new CResourceManager(NULL);

    m_visible = true;
    m_phy_width = 0;
    m_phy_height = 0;
    m_cur_menu = MENU_MAIN;
    m_cursor = 0;
}

CMenu::~CMenu(void)
{
    Unload();

    SAFE_RELEASE(m_resman);
}

void CMenu::Update(const float dt, const uint32_t ticks)
{
    if(!IsVisible())
        return;

    RenderGL();
}

void CMenu::DrawMenu() const
{
    DrawRect(g_menu_background.texture,
             0, 0,
             m_phy_width,
             m_phy_height);

    DrawRectVirtual(g_menu_lynx, 80.0f, 80.0f);

    if(m_cur_menu == MENU_MAIN)
    {
        float y = MENU_BUTTONS_OFFSET_Y;

        DrawRectVirtual(g_menu0_host,    MENU_BUTTONS_OFFSET_X, y); y+= g_menu0_host.height + MENU_BUTTONS_SPACE;
        DrawRectVirtual(g_menu0_join,    MENU_BUTTONS_OFFSET_X, y); y+= g_menu0_join.height + MENU_BUTTONS_SPACE;
        DrawRectVirtual(g_menu0_credits, MENU_BUTTONS_OFFSET_X, y); y+= g_menu0_credits.height + MENU_BUTTONS_SPACE;
        DrawRectVirtual(g_menu0_quit,    MENU_BUTTONS_OFFSET_X, y); y+= g_menu0_quit.height + MENU_BUTTONS_SPACE;

        DrawRectVirtual(g_menu_bullet,
                        MENU_BUTTONS_OFFSET_X - g_menu_bullet.width,
                        MENU_BUTTONS_OFFSET_Y + g_menu0_host.height*m_cursor );
    }
}

bool CMenu::Init(const unsigned int physical_width, const unsigned int physical_height)
{
    int error = 0; // if this ends up larger than 0, some texture was not loaded

    // loading resources -------------------
    error += LoadBitmap("background.tga" , &g_menu_background);
    error += LoadBitmap("lynx.tga"       , &g_menu_lynx);
    error += LoadBitmap("bullet.tga"     , &g_menu_bullet);
    error += LoadBitmap("host.tga"       , &g_menu0_host);
    error += LoadBitmap("join.tga"       , &g_menu0_join);
    error += LoadBitmap("credits.tga"    , &g_menu0_credits);
    error += LoadBitmap("quit.tga"       , &g_menu0_quit);

    if(error > 0)
    {
        fprintf(stderr, "Failed to load menu resources.\n");
        return false;
    }
    // -------------------------------------

    m_phy_width = physical_width;
    m_phy_height = physical_height;

    return true;
}

void CMenu::Unload()
{

}

void CMenu::RenderGL() const
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glUseProgram(0); // no shader for HUD
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
	glOrtho(0, m_phy_width, m_phy_height, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1,1,1,1);

    DrawMenu();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

void CMenu::DrawRect(const unsigned int texture,
                     const float x,
                     const float y,
                     const float width,
                     const float height) const
{
    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
        glTexCoord2d(0,1);
        glVertex3f(x, y, 0.0f);
        glTexCoord2d(0,0);
        glVertex3f(x, y+height, 0.0f);
        glTexCoord2d(1,0);
        glVertex3f(x+width, y+height, 0.0f);
        glTexCoord2d(1,1);
        glVertex3f(x+width, y,0.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void CMenu::DrawRectVirtual(const menu_bitmap_t& bitmap,
                            const float x,
                            const float y) const
{
    DrawRect(bitmap.texture, m_phy_width*x/VIRTUAL_WIDTH,
                             m_phy_height*y/VIRTUAL_HEIGHT,
                             bitmap.width,
                             bitmap.height);
}

int CMenu::LoadBitmap(const std::string path, menu_bitmap_t* bitmap)
{
    const std::string bd = CLynx::GetBaseDirMenu(); // base dir for menu textures

    // load resources -------------------
    bitmap->texture = GetResMan()->GetTexture(bd + path);
    if(bitmap->texture)
    {
        if(!GetResMan()->GetTextureDimension(bd + path,
                                             &bitmap->width,
                                             &bitmap->height))
        {
            assert(0);
            return 1; // error
        }
        else
        {
            return 0; // it worked
        }
    }
    else
    {
        assert(0);
        return 1; // error
    }
}

void CMenu::KeyDown()
{
    int items = 4;

    m_cursor = ++m_cursor%items;
}

void CMenu::KeyUp()
{
    int items = 4;

    m_cursor--;
    if(m_cursor < 0)
        m_cursor = items-1;
}

void CMenu::KeyEnter()
{

}
