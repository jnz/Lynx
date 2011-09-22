#include "lynx.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include <vector>
#include "Menu.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

//-Menu Global Stuff----------------------------------------------------

// All coordinates are hardcoded for a virtual screen, the real
// coordinates are calculated according to the current physical
// screen resolution.

#define VIRTUAL_WIDTH  800
#define VIRTUAL_HEIGHT 600

enum
{
    MENU_MAIN = 0,
    MENU_HOST,
    MENU_JOIN,
    MENU_CREDITS,
    MENU_COUNT // make this the last item
};

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
    // Draw background
    DrawRect(m_menu_background.texture,
             0, 0,
             m_phy_width,
             m_phy_height);

    // Draw lynx logo
    DrawRectVirtual(m_menu_lynx, 80.0f, 80.0f);

    int i;
    const menu_t& menu = m_menus[m_cur_menu];

    // Draw static images
    const int images = menu.images.size();
    for(i = 0; i < images; i++)
    {
        const menu_item_t& item = menu.images[i];
        DrawRectVirtual(item.bitmap, item.x, item.y);
    }

    // Draw selectable (active) items
    const int items = menu.items.size();
    for(i = 0; i < items; i++)
    {
        const menu_item_t& item = menu.items[i];
        DrawRectVirtual(item.bitmap, item.x, item.y);
    }

    // Draw cursor
    const menu_item_t& active_item = menu.items[m_cursor];
    const float bx = active_item.x - m_menu_bullet.width - 2.0f;
    const float by = active_item.y + active_item.bitmap.height*0.5f - m_menu_bullet.height*0.5f;
    DrawRectVirtual(m_menu_bullet, bx, by);
}

bool CMenu::Init(const unsigned int physical_width, const unsigned int physical_height)
{
    int error = 0; // if this ends up larger than 0, some texture was not loaded

    m_phy_width = physical_width;
    m_phy_height = physical_height;

    m_menus.resize(MENU_COUNT);

    // --------resources -------------------
    menu_bitmap_t menu_return;

    menu_bitmap_t menu0_host;
    menu_bitmap_t menu0_join;
    menu_bitmap_t menu0_credits;
    menu_bitmap_t menu0_quit;

    menu_bitmap_t menu2_creditsStatic;

    // loading resources -------------------
    error += LoadBitmap("background.tga"      , &m_menu_background);
    error += LoadBitmap("lynx.tga"            , &m_menu_lynx);
    error += LoadBitmap("bullet.tga"          , &m_menu_bullet);

    error += LoadBitmap("return.tga"          , &menu_return);

    error += LoadBitmap("host.tga"            , &menu0_host);
    error += LoadBitmap("join.tga"            , &menu0_join);
    error += LoadBitmap("credits.tga"         , &menu0_credits);
    error += LoadBitmap("quit.tga"            , &menu0_quit);

    error += LoadBitmap("creditsStatic.tga"   , &menu2_creditsStatic);

    if(error > 0)
    {
        fprintf(stderr, "Failed to load menu resources.\n");
        return false;
    }

    float x = 275.0f;
    float y = 235.0f;

    // -------------------------------------
    // Main menu

    menu_t menu0; // main

    menu0.parent = -1; // no parent
    menu0.items.push_back(menu_item_t(
                menu0_host,
                MENU_BUTTON,
                MENU_FUNC_HOST,
                x, y)); y += menu0_host.height;
    menu0.items.push_back(menu_item_t(
                menu0_join,
                MENU_BUTTON,
                MENU_FUNC_JOIN,
                x, y)); y += menu0_join.height;
    menu0.items.push_back(menu_item_t(
                menu0_credits,
                MENU_BUTTON,
                MENU_FUNC_CREDITS,
                x, y)); y += menu0_credits.height;
    menu0.items.push_back(menu_item_t(
                menu0_quit,
                MENU_BUTTON,
                MENU_FUNC_QUIT,
                x, y));

    m_menus[MENU_MAIN] = menu0;

    // -------------------------------------
    // Host menu
    y = 235.0f;

    menu_t menu1_host; // host a game

    menu1_host.parent = MENU_MAIN; // parent is main menu
    menu1_host.items.push_back(menu_item_t(
                menu0_host,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu0_host.height;
    menu1_host.items.push_back(menu_item_t(
                menu0_credits,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu0_credits.height;
    menu1_host.items.push_back(menu_item_t(
                menu0_join,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu0_join.height;
    menu1_host.items.push_back(menu_item_t(
                menu0_quit,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y));

    m_menus[MENU_HOST] = menu1_host;

    // -------------------------------------
    // Join menu
    y = 235.0f;

    menu_t menu2_join; // join a game

    menu2_join.parent = MENU_MAIN; // parent is main menu
    menu2_join.items.push_back(menu_item_t(
                menu0_join,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu0_join.height;
    menu2_join.items.push_back(menu_item_t(
                menu0_host,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu0_host.height;
    menu2_join.items.push_back(menu_item_t(
                menu0_credits,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu0_credits.height;
    menu2_join.items.push_back(menu_item_t(
                menu0_quit,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y));

    m_menus[MENU_JOIN] = menu2_join;

    // -------------------------------------
    // Credits menu
    y = 235.0f;

    menu_t menu3_credits; // display some credits

    menu3_credits.parent = MENU_MAIN; // parent is main menu
    menu3_credits.images.push_back(menu_item_t(
                menu2_creditsStatic,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu2_creditsStatic.height;
    menu3_credits.items.push_back(menu_item_t(
                menu_return,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu_return.height;

    m_menus[MENU_CREDITS] = menu3_credits;

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
    if(!m_visible)
        return;

    const menu_t& menu = m_menus[m_cur_menu];
    const int items = (int)menu.items.size();

    m_cursor = ++m_cursor%items;
}

void CMenu::KeyUp()
{
    if(!m_visible)
        return;

    const menu_t& menu = m_menus[m_cur_menu];
    const int items = (int)menu.items.size();

    m_cursor--;
    if(m_cursor < 0)
        m_cursor = items-1;
}

void CMenu::KeyEnter(bool* should_we_quit)
{
    *should_we_quit = false;

    if(!m_visible)
        return;

    const menu_t& menu = m_menus[m_cur_menu];
    const menu_item_t& item = menu.items[m_cursor];
    switch(item.func)
    {
        case MENU_FUNC_MAIN: // goto main menu
            m_cur_menu = MENU_MAIN;
            m_cursor = 0;
            break;
        case MENU_FUNC_HOST: // host a game dialog
            m_cur_menu = MENU_HOST;
            m_cursor = 0;
            break;
        case MENU_FUNC_JOIN: // join a game dialog
            m_cur_menu = MENU_JOIN;
            m_cursor = 0;
            break;
        case MENU_FUNC_CREDITS: // credits screen
            m_cur_menu = MENU_CREDITS;
            m_cursor = 0;
            break;
        case MENU_FUNC_QUIT: // quit game
            *should_we_quit = true;
            break;
        default:
            assert(0);
            break;
    }
}

void CMenu::KeyEsc()
{
    if(!m_visible) // invisible, make it active
    {
        Toggle();
        return;
    }

    // go back one screen if not in main menu
    // or, if in main menu: hide menu
    const menu_t& menu = m_menus[m_cur_menu];

    if(menu.parent == -1)
    {
        Toggle();
        return;
    }

    m_cur_menu = menu.parent;
    m_cursor = 0;
}

