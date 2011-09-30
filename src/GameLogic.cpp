#include <assert.h>
#include "GameLogic.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameLogic::CGameLogic(CWorld* world, CServer* server)
{
    m_world = world;
    m_server = server;
}

CGameLogic::~CGameLogic(void)
{
}

bool CGameLogic::InitGame(const char* level)
{
    assert(0);
    return true;
}

void CGameLogic::Notify(EventNewClientConnected e)
{
    assert(0);
}

void CGameLogic::Notify(EventClientDisconnected e)
{
    assert(0);
}

void CGameLogic::Update(const float dt, const uint32_t ticks)
{
    assert(0);
}

void CGameLogic::ClientMove(CObj* clientobj, const std::vector<std::string>& clcmdlist)
{
    vec3_t velocity, dir, side;
    vec3_t newdir(0,0,0);
    vec3_t jump(0,0,0);
    quaternion_t rot;

    velocity = clientobj->GetVel();
    rot = clientobj->GetRot();
    rot.GetVec3(&dir, NULL, &side);
    dir = -dir;

    for(size_t i=0;i<clcmdlist.size();i++)
    {
        if(clcmdlist[i] == "+mf") // move forward
        {
            newdir += dir;
        }
        else if(clcmdlist[i] == "+mb") // move backward
        {
            newdir -= dir;
        }
        else if(clcmdlist[i] == "+ml") // move left
        {
            newdir -= side;
        }
        else if(clcmdlist[i] == "+mr") // move right
        {
            newdir += side;
        }
        else if(clcmdlist[i] == "+jmp") // jump
        {
            jump = vec3_t(0,1.0f,0);
        }
    }

    const float speedfactor = clientobj->locGetIsOnGround() ? 1.0f : 0.3f;
    const float clientspeed = 20.0f;
    const float jumpspeed = 100.0f;

    newdir.y = 0;
    if(newdir.AbsSquared() > 0.1f)
        newdir.SetLength(clientspeed); // Speed
    newdir.y += velocity.y; // keep original y velocity
    if(clientobj->locGetIsOnGround())
        newdir += jump * jumpspeed;
    clientobj->SetVel(newdir);
}

void CGameLogic::ClientMouse(CObj* clientobj, float lat, float lon)
{
    // Welt sichtbare Rotation nur entlang der y-Achse anhand von lat und lon berechnen
    //quaternion_t qlat(vec3_t::xAxis, lat*lynxmath::DEGTORAD);
    quaternion_t qlon(vec3_t::yAxis, lon*lynxmath::DEGTORAD);
    clientobj->SetRot(qlon/**qlat*/);
}

