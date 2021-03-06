#include "lynx.h"
#include <SDL/SDL.h>
#include "GL/glew.h"
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include <stdio.h>
#include "../soil/src/SOIL.h"
#include "ResourceManager.h"
#include "World.h"
#include "ModelMD2.h"
#include "ModelMD5.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CResourceManager::CResourceManager(CWorld* world)
{
    // Don't call stuff from CWorld* world from here,
    // the world is not ready at this stage.
    m_world = world;
}

CResourceManager::~CResourceManager(void)
{
    Shutdown();
}

void CResourceManager::Shutdown()
{
    UnloadAllModels();
    UnloadAllTextures();
    UnloadAllSounds();
}

bool CResourceManager::IsServer() const
{
    if(m_world)
        return !m_world->IsClient();
    else
        return false;
}

void CResourceManager::Precache(const std::string filename, const resource_type_t type)
{
    // only print the path for the client
    if(!IsServer())
        fprintf(stderr, "Precache: %s\n", filename.c_str());

    switch(type)
    {
        case LYNX_RESOURCE_TYPE_TEXTURE:
            GetTexture(CLynx::GetBaseDirTexture() + filename, false);
            break;
        case LYNX_RESOURCE_TYPE_MD2:
        case LYNX_RESOURCE_TYPE_MD5:
            GetModel(CLynx::GetBaseDirModel() + filename);
            break;
        case LYNX_RESOURCE_TYPE_SOUND:
            GetSound(CLynx::GetBaseDirSound() + filename, true); // silent = true
            break;
        default:
            fprintf(stderr, "Unknown resource type in precache function\n");
            break;
    };
}

unsigned int CResourceManager::GetTexture(const std::string texname, const bool noerrormsg)
{
    if(IsServer())
        return 0;

    std::map<std::string, texture_t>::const_iterator iter;
    unsigned int texture; // opengl texture id

    iter = m_texmap.find(texname);
    if(iter == m_texmap.end())
    {
        texture_t t;
        t.path = texname;
        texture = LoadTexture(texname, &t.width, &t.height, noerrormsg);
        if(texture != 0)
        {
            t.id = texture;
            m_texmap[texname] = t;
        }
    }
    else
    {
        texture = ((*iter).second).id;
    }

    return texture;
}

bool CResourceManager::GetTextureDimension(const std::string texname,
                                           unsigned int* pwidth,
                                           unsigned int* pheight) const
{
    assert(pwidth && pheight);
    if(IsServer() || pwidth == NULL || pheight == NULL)
    {
        return 0;
    }

    std::map<std::string, texture_t>::const_iterator iter;

    iter = m_texmap.find(texname);
    if(iter == m_texmap.end())
    {
        *pwidth = 0;
        *pheight = 0;
        fprintf(stderr,
                "GetTextureDimension: Texture not loaded: %s\n",
                texname.c_str());
        return false;
    }
    else
    {
        *pwidth = ((*iter).second).width;
        *pheight = ((*iter).second).height;
        return true;
    }
}

void CResourceManager::UnloadAllTextures()
{
    std::map<std::string, texture_t>::iterator iter;

    for(iter=m_texmap.begin();iter!=m_texmap.end();iter++)
        glDeleteTextures(1, &(((*iter).second).id));
    m_texmap.clear();
}

CModel* CResourceManager::GetModel(std::string mdlname)
{
    std::map<std::string, CModel*>::iterator iter;
    CModel* model;

    if(IsServer())
        return NULL;

    iter = m_modelmap.find(mdlname);
    if(iter == m_modelmap.end())
    {
        if(mdlname.find(".md5") != std::string::npos)
        {
            model = new CModelMD5();
        }
        else if(mdlname.find(".md2") != std::string::npos)
        {
            model = new CModelMD2();
        }
        else
        {
            assert(0); // unknown file extension
            return NULL;
        }

        if(model->Load((char*)mdlname.c_str(), this, !IsServer()))
        {
            m_modelmap[mdlname] = model;
        }
        else
        {
            fprintf(stderr, "Failed to load model: %s\n", mdlname.c_str());
            delete model;
            model = NULL;
        }
    }
    else
    {
        model = (*iter).second;
    }

    return model;
}

void CResourceManager::UnloadAllModels()
{
    std::map<std::string, CModel*>::iterator iter;

    for(iter=m_modelmap.begin();iter!=m_modelmap.end();iter++)
        delete (*iter).second;
    m_modelmap.clear();
}

CSound* CResourceManager::GetSound(const std::string sndname, const bool silent)
{
    if(IsServer())
        return NULL;

    std::map<std::string, CSound*>::iterator iter;
    CSound* sound;

    iter = m_soundmap.find(sndname);
    if(iter == m_soundmap.end())
    {
        sound = new CSound();
        if(sound->Load((char*)sndname.c_str()))
        {
            if(!silent)
                fprintf(stderr, "Loaded sound: %s\n", sndname.c_str());
            m_soundmap[sndname] = sound;
        }
        else
        {
            fprintf(stderr, "Failed to load sound: %s\n", sndname.c_str());
            delete sound;
            sound = NULL;
        }
    }
    else
    {
        sound = (*iter).second;
    }

    return sound;
}

void CResourceManager::UnloadAllSounds()
{
    std::map<std::string, CSound*>::iterator iter;

    for(iter=m_soundmap.begin();iter!=m_soundmap.end();iter++)
        delete (*iter).second;
    m_soundmap.clear();
}

unsigned int CResourceManager::LoadTexture(const std::string path,
                                           unsigned int* pwidth,
                                           unsigned int* pheight,
                                           const bool noerrormsg)
{
    unsigned int width;
    unsigned int height;

    unsigned int tex = SOIL_load_OGL_texture(
        path.c_str(),
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
          SOIL_FLAG_MIPMAPS |
          SOIL_FLAG_INVERT_Y |
          SOIL_FLAG_NTSC_SAFE_RGB |
          SOIL_FLAG_COMPRESS_TO_DXT |
          SOIL_FLAG_TEXTURE_REPEATS,
        &width,
        &height
    );
    if(pwidth)
        *pwidth = width;
    if(pheight)
        *pheight = height;

    if(tex == 0 && !noerrormsg)
    {
        fprintf(stderr, "Failed to load texture: %s (%s)\n",
                path.c_str(),
                SOIL_last_result());
    }

    return tex;
}

#if 0
unsigned int CResourceManager::LoadTGA(std::string path)
{
    GLubyte* imageData;
    GLuint bpp;
    GLuint width;
    GLuint height;
    GLubyte TGAheader[12]={0,0,'x',0,0,0,0,0,0,0,0,0};
    GLubyte TGAcompare[12];
    GLubyte header[6];
    GLuint bytesPerPixel;
    GLuint imageSize;
    GLuint temp;
    GLuint type;
    GLuint rle;
    GLuint texture;

    FILE *f = fopen(path.c_str(), "rb");
    if(!f)
    {
        fprintf(stderr, "Could not open texture file: %s\n", path.c_str());
        //assert(0);
        return 0;
    }

    if(fread(TGAcompare, 1, sizeof(TGAcompare), f) != sizeof(TGAcompare))
    {
        fprintf(stderr, "Invalid TGA header: %s\n", path.c_str());
        fclose(f);
        return 0;
    }

    if(memcmp(TGAheader, TGAcompare, sizeof(TGAheader)))
    {
        if(TGAcompare[2] != 2 && TGAcompare[2] != 10)
        {
            fprintf(stderr, "Invalid TGA header: %s\n", path.c_str());
            fclose(f);
            return 0;
        }

        if(TGAcompare[2] == 10)
            rle = 1;
        else
            rle = 0;
    }
    else
    {
        fprintf(stderr, "Invalid TGA header: %s\n", path.c_str());
        fclose(f);
        return 0;
    }

    if(fread(header, 1, sizeof(header), f) != sizeof(header))
    {
        printf("Invalid header size\n");
        fclose(f);
        return 0;
    }

    width = header[1] * 256 + header[0];
    height = header[3] * 256 + header[2];

    if(width <=0 || height <=0 || (header[4] != 24 && header[4] != 32))
    {
        printf("Invalid TGA file\n");
        fclose(f);
        return 0;
    }

    bpp = header[4];
    bytesPerPixel = bpp / 8;
    imageSize = width * height * bytesPerPixel;

    imageData = new GLubyte[imageSize];
    if(!imageData)
    {
        fprintf(stderr, "Not enough memory to load texture: %i bytes req.\n", imageSize);
        fclose(f);
        return 0;
    }

    if(!rle)
    {
        if(fread(imageData, 1, imageSize, f) != imageSize)
        {
            delete[] imageData;
            fclose(f);
            return 0;
        }
    }
    else
    {
        // rle loader code
        int read;
        int count; // how many bytes are left in the packet
        int i=0, j;
        GLubyte pixel[4];

        while((unsigned int)i < imageSize)
        {
            read = fgetc(f);
            if(read == -1)
            {
                fclose(f);
                delete[] imageData;
                return 0;
            }

            count = (read & 0x7F) + 1;
            if(read & 0x80)
            {
                if(fread(pixel, 1, bytesPerPixel, f) != bytesPerPixel)
                {
                    fclose(f);
                    delete[] imageData;
                    return 0;
                }

                for(j=0;j<count;j++)
                {
                    memcpy(imageData + i, pixel, bytesPerPixel);
                    i += bytesPerPixel;
                }
            }
            else
            {
                if(fread(imageData + i, 1, count * bytesPerPixel, f) != count * bytesPerPixel)
                {
                    fclose(f);
                    delete[] imageData;
                    return 0;
                }

                i += count * bytesPerPixel;
            }
        }
    }

    fclose(f);

    for(int i=0;i<int(imageSize);i+=bytesPerPixel)
    {
        temp = imageData[i];
        imageData[i] = imageData[i + 2];
        imageData[i + 2] = temp;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if(bpp == 32)
        type = GL_RGBA;
    else if(bpp == 24)
        type = GL_RGB;

    //glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, imageData);
    if(gluBuild2DMipmaps(GL_TEXTURE_2D, type == GL_RGBA ? 4 : 3, width, height, type, GL_UNSIGNED_BYTE, imageData)!=0)
    {
        fprintf(stderr, "gluBuild2DMipmaps failed on %ix%i. glGetError=%i\n", width, height, glGetError());
        assert(0);
        texture = 0;
    }

    delete[] imageData;

    fprintf(stderr, "Loaded texture: %s\n", path.c_str());
    return texture;
}
#endif
