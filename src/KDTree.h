#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include <vector>
#include "BSPBIN.h"

/*
    CKDTree verwaltet die Level Geometrie und wird
    für die Kollisionserkennung und Darstellung genutzt.
 */

class CKDTree;

struct spawn_point_t
{
    vec3_t origin;
    quaternion_t rot;
};

struct kd_tri_t // triangle for kd-tree
{
    int vertices[3];
    int normals[3];
    int texcoords[3]; // UV coordinates
    plane_t plane; // triangle plane
    std::string texturepath;

    vec3_t GetNormal(CKDTree* tree);
    void GeneratePlane(CKDTree* tree); // Calculate triangle normal
};

enum polyplane_t {  POLYPLANE_SPLIT = 0,
                    POLYPLANE_BACK = 1,
                    POLYPLANE_FRONT = 2,
                    POLYPLANE_COPLANAR = 3
                    }; // if you change the order, change TestPolygon too

class CKDTree
{
public:
    CKDTree(void);
    ~CKDTree(void);

    bool        Load(std::string file); // Lädt den Level aus Wavefront .obj Dateien. Texturen werden über den ResourceManager geladen
    void        Unload();
    std::string GetFilename() const; // Aktuell geladener Pfad zu Level

    class CKDNode // Hilfsklasse von KDTree, stellt einen Knoten im Baum dar
    {
    public:

        CKDNode(CKDTree* tree, const std::vector<int>& trianglesIn, const int kdAxis);
        ~CKDNode();

        union
        {
            struct
            {
                CKDNode* front;
                CKDNode* back;
            };
            CKDNode* child[2];
        };

        void                    CalculateSphere(const CKDTree* tree, const std::vector<int>& trianglesIn); // Bounding Sphere für diesen Knoten berechnen
        bool                    IsLeaf() const { return !front && !back; }
        plane_t                 FindSplittingPlane(const CKDTree* tree, const std::vector<int>& triangles, const int kdAxis);

        plane_t                 plane; // Split plane
        float                   sphere; // Sphere radius
        vec3_t                  sphere_origin; // Sphere origin
        std::vector<int>        triangles; // Only filled when node is a leaf
    };

    std::vector<vec3_t>         m_vertices; // Vertexvektor
    std::vector<vec3_t>         m_normals; // Normalenvektor
    std::vector<vec3_t>         m_texcoords; // FIXME vec2_t würde reichen
    std::vector<kd_tri_t>       m_triangles; // Vektor für Polygone
    CKDNode*                    m_root; // Starting node

    bool        WriteToBinary(const std::string filepath);

private:
    int         m_nodecount; // increased by every CKDNode constructor
    int         m_leafcount;
    bool        m_outofmem; // set to true by CKDNode constructor, if no memory for further child nodes is available
    std::string m_filename;

    // Spawn Point
    std::vector<spawn_point_t> m_spawnpoints; // Spawnpoints from level

    // Test Triangle: is a triangle in front or back of the plane?
    polyplane_t TestTriangle(const int triindex, const plane_t& plane);
};

