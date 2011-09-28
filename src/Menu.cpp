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

#define VIRTUAL_WIDTH  800.0f
#define VIRTUAL_HEIGHT 600.0f
#define TEXT_FIELD_MAX_CHARS_TO_DISPLAY    27

enum
{
    MENU_MAIN = 0,
    MENU_HOST,
    MENU_JOIN,
    MENU_CREDITS,
    MENU_LOADING,
    MENU_ERROR_SCREEN,
    MENU_COUNT // make this the last item
};

// the character input system sucks, this whole character
// input is something we shouldn't have to deal with.
// SDL should provide us with something sane.
int g_keyshift[256]; // characters when shift is pressed

//----------------------------------------------------------------------
static void RenderCube();
//----------------------------------------------------------------------

CMenu::CMenu(void)
{
    m_resman = new CResourceManager(NULL);

    m_visible = true;
    m_phy_width = 0;
    m_phy_height = 0;
    m_cur_menu = MENU_MAIN;
    m_cursor = 0;
    m_animation = 0.0f;

    unsigned int i;
    for(i=0;i<sizeof(g_keyshift)/sizeof(g_keyshift[0]);i++)
        g_keyshift[i] = i;
    for (i=0;i<sizeof(g_keyshift)/sizeof(g_keyshift[0]);i++)
        g_keyshift[i] = i;
    for (i='a';i<='z';i++)
        g_keyshift[i] = i - 'a' + 'A';
    g_keyshift[(int)'1'] = '!';
    g_keyshift[(int)'2'] = '@';
    g_keyshift[(int)'3'] = '#';
    g_keyshift[(int)'4'] = '$';
    g_keyshift[(int)'5'] = '%';
    g_keyshift[(int)'6'] = '^';
    g_keyshift[(int)'7'] = '&';
    g_keyshift[(int)'8'] = '*';
    g_keyshift[(int)'9'] = '(';
    g_keyshift[(int)'0'] = ')';
    g_keyshift[(int)'-'] = '_';
    g_keyshift[(int)'='] = '+';
    g_keyshift[(int)','] = '<';
    g_keyshift[(int)'.'] = '>';
    g_keyshift[(int)'/'] = '?';
    g_keyshift[(int)';'] = ':';
    g_keyshift[(int)'\''] = '"';
    g_keyshift[(int)'['] = '{';
    g_keyshift[(int)']'] = '}';
    g_keyshift[(int)'`'] = '~';
    g_keyshift[(int)'\\'] = '|';
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

    m_animation += dt * 25.0f; // magic menu animation speed in deg per sec.
    RenderGL();
}

void CMenu::DrawMenu() const
{
    // Draw background
    DrawRect(m_menu_background.texture,
             0, 0,
             (float)m_phy_width,
             (float)m_phy_height);

    // Draw lynx logo
    DrawRectVirtual(m_menu_lynx, 10.0f, 10.0f);

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
        if(item.type == MENU_TEXT_FIELD)
        {
            DrawFontVirtual(item.text,
                            item.x + item.textoffset_x,
                            item.y + item.textoffset_y);
        }
    }

    // Draw cursor next to active item
    if(menu.items.size() > 0)
    {
        const menu_item_t& active_item = menu.items[m_cursor];
        const float bx = active_item.x - m_menu_bullet.width - 2.0f; // 2.0: some space between the bullet and the items
        const float by = active_item.y + active_item.bitmap.height*0.5f - m_menu_bullet.height*0.5f;
        DrawRectVirtual(m_menu_bullet, bx, by);
    }
}

bool CMenu::Init(const unsigned int physical_width, const unsigned int physical_height, menu_engine_callback_t callback)
{
    int error = 0; // if this ends up larger than 0, some texture was not loaded
    m_animation = 0.0f;
    m_callback = callback;
    m_phy_width = physical_width;
    m_phy_height = physical_height;
    m_menus.resize(MENU_COUNT);

    // Load a font
    m_font.Init("font.png", 16, 24, GetResMan());

    // --------resources -------------------
    menu_bitmap_t menu_return;
    menu_bitmap_t menu_loading;

    menu_bitmap_t menu0_host;
    menu_bitmap_t menu0_join;
    menu_bitmap_t menu0_credits;
    menu_bitmap_t menu0_quit;

    menu_bitmap_t menu1_start;

    menu_bitmap_t menu2_serveraddr;
    menu_bitmap_t menu2_connect;

    menu_bitmap_t menu3_creditsStatic;

    // loading resources -------------------
    error += LoadBitmap("cube.jpg"            , &m_menu_background_cube);
    error += LoadBitmap("background.tga"      , &m_menu_background);
    error += LoadBitmap("lynx.tga"            , &m_menu_lynx);
    error += LoadBitmap("bullet.tga"          , &m_menu_bullet);

    error += LoadBitmap("return.tga"          , &menu_return);
    error += LoadBitmap("loading.tga"         , &menu_loading);

    error += LoadBitmap("host.tga"            , &menu0_host);
    error += LoadBitmap("join.tga"            , &menu0_join);
    error += LoadBitmap("credits.tga"         , &menu0_credits);
    error += LoadBitmap("quit.tga"            , &menu0_quit);

    error += LoadBitmap("starthost.tga"       , &menu1_start);

    error += LoadBitmap("serveraddr.tga"      , &menu2_serveraddr);
    error += LoadBitmap("connect.tga"         , &menu2_connect);

    error += LoadBitmap("creditsStatic.tga"   , &menu3_creditsStatic);

    if(error > 0)
    {
        fprintf(stderr, "Failed to load menu resources.\n");
        return false;
    }

    const float basey = 200.0f;
    const float basex = 225.0f; // read as "base x", ok?
    float x = basex;
    float y = basey;
    // Add some space between the items:
    const float scaleheight = 1.01f; // 1 % space between items

    // -------------------------------------
    // Main menu

    menu_t menu0; // main

    menu0.parent = -1; // no parent
    menu0.items.push_back(menu_item_t(
                menu0_host,
                MENU_BUTTON,
                MENU_FUNC_HOST,
                x, y)); y += menu0_host.height * scaleheight;
    menu0.items.push_back(menu_item_t(
                menu0_join,
                MENU_BUTTON,
                MENU_FUNC_JOIN,
                x, y)); y += menu0_join.height * scaleheight;
    menu0.items.push_back(menu_item_t(
                menu0_credits,
                MENU_BUTTON,
                MENU_FUNC_CREDITS,
                x, y)); y += menu0_credits.height * scaleheight;
    menu0.items.push_back(menu_item_t(
                menu0_quit,
                MENU_BUTTON,
                MENU_FUNC_QUIT,
                x, y));

    m_menus[MENU_MAIN] = menu0;

    // -------------------------------------
    // Host menu
    y = basey;

    menu_t menu1_host; // host a game

    menu1_host.parent = MENU_MAIN; // parent is main menu
    menu1_host.items.push_back(menu_item_t(
                menu1_start,
                MENU_BUTTON,
                MENU_FUNC_START_SERVER_AND_CLIENT,
                x, y)); y += menu0_host.height * scaleheight;
    menu1_host.items.push_back(menu_item_t(
                menu_return,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y));

    m_menus[MENU_HOST] = menu1_host;

    // -------------------------------------
    // Join menu
    y = basey;

    menu_t menu2_join; // join a game

    menu2_join.parent = MENU_MAIN; // parent is main menu
    menu2_join.items.push_back(menu_item_t(
                menu2_serveraddr,
                MENU_TEXT_FIELD,
                MENU_FUNC_MAIN,
                x, y,
                (CLynx::cfg.GetVarAsStr("default_server", "localhost", true)), // default text for text field: network address
                60.0f,  // offset for text x
                (menu2_serveraddr.height - m_font.GetHeight())/2, // vertical center
                "connect ip address" // text field name
                ));
                y += menu2_serveraddr.height * scaleheight;
    menu2_join.items.push_back(menu_item_t(
                menu2_serveraddr,
                MENU_TEXT_FIELD,
                MENU_FUNC_MAIN,
                x, y,
                (CLynx::cfg.GetVarAsStr("port", "9999", true)), // default text for text field: port
                60.0f,  // offset for text x
                (menu2_serveraddr.height - m_font.GetHeight())/2, // vertical center
                "connect ip port" // text field name
                ));
                y += menu2_serveraddr.height * scaleheight;
    menu2_join.items.push_back(menu_item_t(
                menu2_connect,
                MENU_BUTTON,
                MENU_FUNC_CONNECT,
                x, y)); y += menu2_connect.height * scaleheight;
    menu2_join.items.push_back(menu_item_t(
                menu_return,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y));

    m_menus[MENU_JOIN] = menu2_join;

    // -------------------------------------
    // Credits menu
    y = basey;

    menu_t menu3_credits; // display some credits

    menu3_credits.parent = MENU_MAIN; // parent is main menu
    menu3_credits.images.push_back(menu_item_t(
                menu3_creditsStatic,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); y += menu3_creditsStatic.height * scaleheight;
    menu3_credits.items.push_back(menu_item_t(
                menu_return,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y)); //y += menu_return.height * scaleheight;

    m_menus[MENU_CREDITS] = menu3_credits;

    // -------------------------------------
    // Loading menu
    x = (VIRTUAL_WIDTH - menu_loading.width)/2;
    y = (VIRTUAL_HEIGHT - menu_loading.height)/2;

    menu_t menu4_loading; // loading menu

    menu4_loading.parent = MENU_MAIN; // parent is main menu
    menu4_loading.images.push_back(menu_item_t(
                menu_loading,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y));

    m_menus[MENU_LOADING] = menu4_loading;

    // -------------------------------------
    // Error screen
    x = (VIRTUAL_WIDTH - menu2_serveraddr.width)/2;
    y = basey;

    menu_t menu5_error; // loading menu

    // To display any text on the error menu screen
    // we use a text field that is set to readonly.
    // Before we display the error screen to the user
    // we set the textfield text.
    menu5_error.parent = MENU_MAIN; // parent is main menu
    menu5_error.items.push_back(menu_item_t(
                menu2_serveraddr,
                MENU_TEXT_FIELD,
                MENU_FUNC_MAIN,
                x, y,
                "Error", // default text for text field: port
                60.0f,  // offset for text x
                (menu2_serveraddr.height - m_font.GetHeight())/2, // vertical center
                "errormsg" // text field name
                ));
    menu5_error.items[menu5_error.items.size()-1].readonly = true;
                y += menu2_serveraddr.height * scaleheight;
    x = basex;
    menu5_error.items.push_back(menu_item_t(
                menu_return,
                MENU_BUTTON,
                MENU_FUNC_MAIN,
                x, y));

    m_menus[MENU_ERROR_SCREEN] = menu5_error;

    return true;
}

void CMenu::Unload()
{

}

void CMenu::DrawDefaultBackground()
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(0); // no shader for menu

    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(120.0f,
                   (float)m_phy_width/(float)m_phy_height,
                   0.1f, // near
                   20.0f); // far
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4f(1,1,1,1);
    glBindTexture(GL_TEXTURE_2D, m_menu_background_cube.texture);
    glTranslatef(0.8f, 0.0f, -1.8f);
    glRotatef(m_animation, 0.0f, 1.0f, 0.0f);
    while(m_animation > 360.0f)
        m_animation -= 360.0f;
    RenderCube();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

void CMenu::RenderGL() const
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glUseProgram(0); // no shader for menu
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

void CMenu::DrawFontVirtual(std::string text, float x, float y) const
{
    if(text.length() > TEXT_FIELD_MAX_CHARS_TO_DISPLAY)
    {
        const std::string indicator("...");
        text = indicator + text.substr(text.length()-
                                       TEXT_FIELD_MAX_CHARS_TO_DISPLAY+
                                       indicator.length(),
                                       text.length());
    }
    const float vx = x + (m_phy_width - VIRTUAL_WIDTH)/2;
    const float vy = y + (m_phy_height - VIRTUAL_HEIGHT)/2;
    m_font.DrawGL(vx, vy, 0.0f, text.c_str());
}

void CMenu::DrawRectVirtual(const menu_bitmap_t& bitmap,
                            const float x,
                            const float y) const
{
    const float vx = x + (m_phy_width - VIRTUAL_WIDTH)/2;
    const float vy = y + (m_phy_height - VIRTUAL_HEIGHT)/2;
    DrawRect(bitmap.texture, vx, vy, bitmap.width, bitmap.height);
}

void CMenu::SetTextFieldValue(const std::string fieldid, const std::string text)
{
    int i, j;
    const int menus = (int)m_menus.size();
    for(i=0;i<menus;i++)
    {
        menu_t& menu = m_menus[i];

        const int items = menu.items.size();
        for(j = 0; j < items; j++)
        {
            menu_item_t& item = menu.items[j];
            if(item.type == MENU_TEXT_FIELD &&
               item.fieldid == fieldid)
            {
                item.text = text;
                return;
            }
        }
    }
    assert(0); // there is no field with fieldid. shame on the developer!
}

std::string CMenu::GetTextFieldValue(const std::string fieldid)
{
    int i, j;
    const int menus = (int)m_menus.size();
    for(i=0;i<menus;i++)
    {
        const menu_t& menu = m_menus[i];

        const int items = menu.items.size();
        for(j = 0; j < items; j++)
        {
            const menu_item_t& item = menu.items[j];
            if(item.type == MENU_TEXT_FIELD)
            {
                if(item.fieldid == fieldid)
                    return item.text;
            }
        }
    }

    return std::string("");
}

void CMenu::DisplayMain()
{
    m_cur_menu = MENU_MAIN;
    m_cursor = 0;
}

void CMenu::DisplayLoadingScreen()
{
    m_cur_menu = MENU_LOADING;
    m_cursor = 0;
}

void CMenu::DisplayError(const std::string errormsg)
{
    m_cur_menu = MENU_ERROR_SCREEN;
    // select last item:
    const menu_t& menu = m_menus[m_cur_menu];
    m_cursor = menu.items.size()-1;

    // "errormsg" is the id of the error text field
    // and std::string errormsg is what we want
    // to display
    SetTextFieldValue("errormsg", errormsg);
}

int CMenu::LoadBitmap(const std::string path, menu_bitmap_t* bitmap)
{
    const std::string bd = CLynx::GetBaseDirMenu(); // base dir for menu textures

    // load resources -------------------
    bitmap->texture = GetResMan()->GetTexture(bd + path);
    if(bitmap->texture)
    {
        unsigned int tempwidth;
        unsigned int tempheight;
        if(!GetResMan()->GetTextureDimension(bd + path,
                                             &tempwidth,
                                             &tempheight))
        {
            assert(0);
            return 1; // error
        }
        else
        {
            bitmap->width = (float)tempwidth;
            bitmap->height = (float)tempheight;
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

    m_cursor++;
    m_cursor = m_cursor%items;
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

void CMenu::KeyEnter()
{
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
            m_callback.menu_func_quit();
            break;
        case MENU_FUNC_START_SERVER_AND_CLIENT:
            m_cur_menu = MENU_LOADING;
            m_cursor = 0;
            DrawDefaultBackground();
            RenderGL();
            m_callback.menu_func_host(NULL, 0, true); // also join as client
            m_cur_menu = MENU_MAIN;
            MakeInvisible();
            break;
        case MENU_FUNC_CONNECT:
            m_cur_menu = MENU_LOADING;
            m_cursor = 0;
            DrawDefaultBackground();
            RenderGL();

            {
                std::string ipaddress = GetTextFieldValue("connect ip address");
                std::string ipport = GetTextFieldValue("connect ip port");
                m_callback.menu_func_join(ipaddress.c_str(), atoi(ipport.c_str()));
            }

            m_cur_menu = MENU_MAIN;
            MakeInvisible();
            break;
        default:
            assert(0);
            break;
    }
}

void CMenu::KeyAscii(unsigned char val, bool shift, bool ctrl)
{
    if(!m_visible)
        return;
    // clip valid input range
    if(val < 32 || val > 127)
        return;

    menu_t& menu = m_menus[m_cur_menu];
    menu_item_t& item = menu.items[m_cursor];

    // only use this for text fields
    if(item.type != MENU_TEXT_FIELD)
        return;
    // check if text is read only
    if(item.readonly)
        return;
    // stop, if the user likes too much typing
    if(item.text.length() > MENU_MAX_TEXT_LEN)
        return;
    if(shift) // our little toupper() function
        val = g_keyshift[val];

    char ascii[] = {(char)val, 0};
    item.text += std::string(ascii);
}

void CMenu::KeyBackspace()
{
    if(!m_visible)
        return;

    menu_t& menu = m_menus[m_cur_menu];
    menu_item_t& item = menu.items[m_cursor];

    // only use this for text fields
    if(item.type != MENU_TEXT_FIELD)
        return;
    // check for read only
    if(item.readonly)
        return;

    int textlen = item.text.length();
    if(textlen > 0)
        item.text = item.text.substr(0, textlen-1);
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

static void RenderCube() // this is handy sometimes
{
    glBegin(GL_QUADS);
        // Front Face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // Top Left Of The Texture and Quad
        // Back Face
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);  // Bottom Left Of The Texture and Quad
        // Top Face
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);  // Top Right Of The Texture and Quad
        // Bottom Face
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        // Right face
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        // Left Face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);  // Top Left Of The Texture and Quad
    glEnd();
}
