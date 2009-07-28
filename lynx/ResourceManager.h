#pragma once

#include <string>
#include <map>
class CResourceManager;
#include "ModelMD2.h"

class CResourceManager
{
public:
	CResourceManager(void);
	~CResourceManager(void);

	unsigned int GetTexture(std::string texname);
	void UnloadAllTextures();

	CModelMD2* GetModel(std::string mdlname);
	void UnloadAllModels();

private:
	unsigned int LoadTGA(std::string path);

	std::map<std::string, unsigned int> m_texmap;
	std::map<std::string, CModelMD2*> m_modelmap;
};
