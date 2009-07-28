#pragma once

#include "WorldClient.h"

class CRenderer
{
public:
	CRenderer(CWorldClient* world);
	~CRenderer(void);

	bool Init(int width, int height, int bpp, int fullscreen);
	void Shutdown();

	void Update(const float dt);

	// Public Stats (FIXME)
	int stat_obj_visible;
	int stat_obj_hidden;

protected:
	void UpdatePerspective();

private:
	int m_width, m_height;
	CWorldClient* m_world;
};
