#include <stdio.h>
#include <assert.h>
#include <SDL.h>
#include <time.h>
#include "Renderer.h"
#include "Server.h"
#include "Client.h"
#include "WorldClient.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		480
#define BPP					32
#define FULLSCREEN			0

int main(int argc, char** argv)
{
	srand((unsigned int)time(NULL));
	{ // for dumpmemleak
	int run;
	float dt;
	DWORD time, oldtime;
	DWORD fpstimer, fpscounter=0;
	SDL_Event event;
	
	// Game Modules
	CWorld worldsv; // Model
	CWorldClient worldcl; // Model
	CRenderer renderer(&worldcl); // View
	CServer server(&worldsv); // Controller
	CClient client(&worldcl); // Controller

	if(!renderer.Init(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, FULLSCREEN))
		return -1;

	if(!server.Create(9999))
	{
		fprintf(stderr, "Failed to create server\n");
		assert(0);
		return -1;
	}

	CObj* obj;
	obj = new CObj(&worldsv);
	obj->pos.origin = vec3_t(-45.0f, 8.0f, 0);
	obj->SetSpeed(6.0f);
	obj->pos.velocity = vec3_t(0.0f, 0, 0.0f);
	obj->pos.rot.y = 270;
	obj->SetResource(CLynx::GetBaseDirModel() + "mdl1/tris.md2");
	//obj->SetResource(CLynx::GetBaseDirModel() + "q2/tris2.md2");
	obj->SetAnimation("default");
	worldsv.AddObj(obj);

	worldsv.m_bsptree.Load(CLynx::GetBaseDirLevel() + "testlvl/level1.obj");

	if(!client.Connect("localhost", 9999))
	{
		fprintf(stderr, "Failed to connect to server\n");
		assert(0);
		return -1;
	}

	SDL_WM_SetCaption("lynx", NULL);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_GrabInput(SDL_GRAB_ON);
	run = 1;
	oldtime = fpstimer = CLynx::GetTicks();
	while(run)
	{
		time = CLynx::GetTicks();
		dt = 0.001f * (float)(time-oldtime);
		oldtime = time;
		fpscounter++;
		if(time - fpstimer > 1000.0f)
		{
			char title[128];
			sprintf(title, "lynx (FPS: %i) vis: %i/%i bsp-leafes: %i/%i", 
				fpscounter, renderer.stat_obj_visible, renderer.stat_obj_hidden, worldcl.m_bsptree.m_visited, worldcl.m_bsptree.GetLeafCount());
			SDL_WM_SetCaption(title, NULL);
			fpscounter = 0;
			fpstimer = time;
		}

		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					run = 0;
				}
				break;
			case SDL_QUIT:
				run = 0;
				break;
			default:
				break;
			};
		}

		worldsv.Update(dt);
		worldcl.Update(dt);
		server.Update(dt);
		client.Update(dt);
		renderer.Update(dt);
#ifdef _DEBUG
		SDL_Delay(10); // so my notebook fan is quiet :-)
#endif
	}
	}
	_CrtDumpMemoryLeaks();

	return 0;
}

