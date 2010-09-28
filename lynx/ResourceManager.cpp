#include "lynx.h"
#include <SDL/SDL.h>
#include "GL/glew.h"
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include <stdio.h>
#include "ResourceManager.h"
#include "World.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CResourceManager::CResourceManager(CWorld* world)
{
    // Keine Aufrufe in CWorld* world hinein (ist hier noch nicht fertig mit dem Konstruktor)
    assert(world);
    m_world = world;
}

CResourceManager::~CResourceManager(void)
{
    UnloadAllModels();
    UnloadAllTextures();
}

bool CResourceManager::IsServer()
{
    return !m_world->IsClient();
}

unsigned int CResourceManager::GetTexture(std::string texname)
{
    if(IsServer())
    {
        assert(0);
        return 0;
    }

    std::map<std::string, unsigned int>::iterator iter;
    unsigned int texture;

    iter = m_texmap.find(texname);
    if(iter == m_texmap.end())
    {
        texture = LoadTGA(texname);
        if(texture != 0)
            m_texmap[texname] = texture;
    }
    else
    {
        texture = (*iter).second;
    }

    return texture;
}

void CResourceManager::UnloadAllTextures()
{
    std::map<std::string, unsigned int>::iterator iter;
    
    for(iter=m_texmap.begin();iter!=m_texmap.end();iter++)
        glDeleteTextures(1, &((*iter).second));
    m_texmap.clear();
}

CModelMD2* CResourceManager::GetModel(std::string mdlname)
{
    std::map<std::string, CModelMD2*>::iterator iter;
    CModelMD2* model;

    iter = m_modelmap.find(mdlname);
    if(iter == m_modelmap.end())
    {
        model = new CModelMD2();
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
    std::map<std::string, CModelMD2*>::iterator iter;
    
    for(iter=m_modelmap.begin();iter!=m_modelmap.end();iter++)
        delete (*iter).second;
    m_modelmap.clear();
}

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

    return texture;
}
