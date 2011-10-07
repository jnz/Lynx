#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include <vector>
#include "BSPBIN.h"

// KDTree basic job is to load a polygoon soup from file and
// write a .lbsp file.
// CBSPLevel can then load this lbsp file in the game and render it.

class CKDTree;

struct spawn_point_t
{
    vec3_t origin;
    quaternion_t rot;
};

struct kd_tri_t // triangle for kd-tree
{
    int vertices[3];
    int texcoords[3]; // UV coordinates
    plane_t plane;    // triangle plane
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

    // lightmap is a .obj file with the same geometry, but texture coordinates are for the lightmap
    bool        Load(const std::string& file,
                     const std::string& lightmap);

    void        Unload();
    std::string GetFilename() const; // Current filepath

    class CKDNode // KDTree helper class: CKDNode is a node in the tree
    {
    public:

        CKDNode(CKDTree* tree, const std::vector<int>& trianglesIn, const int recursionDepth);
        ~CKDNode();

        union
        {
            struct
            {
                CKDNode* m_front;
                CKDNode* m_back;
            };
            CKDNode* child[2];
        };

        // Calculate bounding sphere for this node
        void               CalculateSphere(const CKDTree* tree, const std::vector<int>& trianglesIn);
        bool               IsLeaf() const { return !m_front && !m_back; }
        void               SplitTrianglesAlongAxis(const CKDTree* tree,
                                                   const std::vector<int>& trianglesIn,
                                                   const int kdAxis,
                                                   std::vector<int>& front,
                                                   std::vector<int>& back,
                                                   std::vector<int>& splitlist,
                                                   plane_t& splitplane) const;
        plane_t            FindSplittingPlane(const CKDTree* tree, const std::vector<int>& triangles, const int kdAxis) const;

        plane_t            m_plane;          // Split plane, not set for leafs
        float              m_sphere;         // Sphere radius
        vec3_t             m_sphere_origin;  // Sphere origin
        std::vector<int>   m_triangles;      // Only filled when node is a leaf
    };

    std::vector<vec3_t>    m_vertices;       // Vertices from obj file
    std::vector<vec3_t>    m_texcoords;      // vec2_t would be sufficient
    std::vector<vec3_t>    m_lightmapcoords; // vec2_t would be sufficient
    std::vector<kd_tri_t>  m_triangles;      // Triangles from obj file
    CKDNode*               m_root;           // Starting node
    int                    m_estimatedtexturecount; // how many textures are used in the obj file?

    bool                   WriteToBinary(const std::string filepath);

protected:
    bool                   LoadLightmapCoordinates(const std::string& lightmappath,
                                                   std::vector<vec3_t>& lightcoords);

private:
    int                    m_nodecount; // increased by every CKDNode constructor
    int                    m_leafcount; // increased by every CKNode that is a leaf node
    int                    m_depth;     // set by CKNode constructor
    std::string            m_filename;

    // Spawn Point
    std::vector<spawn_point_t> m_spawnpoints; // Spawnpoints from level

    // Test Triangle: is a triangle in front or back of the plane?
    polyplane_t TestTriangle(const int triindex, const plane_t& plane) const;
};

