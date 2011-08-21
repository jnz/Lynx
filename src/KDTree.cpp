#include "KDTree.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <algorithm> // for std::sort
#include "math/mathconst.h"
#include "BSPBIN.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define KDTREE_EPSILON                 0.01f
#define KDTREE_MAX_TRIANGLES_PER_LEAF  (BSPBIN_MAX_TRIANGLES_PER_LEAF*3/4)

CKDTree::CKDTree(void)
{
    m_root = NULL;
    m_leafcount = 0;
}

CKDTree::~CKDTree(void)
{
    Unload();
}

#pragma warning(disable: 4244)
bool CKDTree::Load(std::string file)
{
    bool success = false; // failed to read file, if false
    const char* DELIM = " \t\r\n";
    FILE* f;
    char line[2048];
    char* tok;
    char* sv[3];
    int i;
    int vi, ti, ni; // vertex, texture and normal index
    kd_tri_t triangle;
    std::string texpath;
    std::vector<int> alltriangles; // indices of all triangles, used for the init of the root node

    Unload();

    f = fopen(file.c_str(), "rb");
    if(!f)
    {
        fprintf(stderr, "Failed to open file: %s\n", file.c_str());
        return false;
    }

    while(!feof(f))
    {
        fgets(line, sizeof(line), f);
        tok = strtok(line, DELIM);

        if(strcmp(tok, "v")==0) // vertex
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1] || !sv[2])
                goto cleanup;
            m_vertices.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        }
        else if(strcmp(tok, "vn")==0) // normal
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1] || !sv[2])
                goto cleanup;
            m_normals.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        }
        else if(strcmp(tok, "vt")==0) // textures
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1])
                goto cleanup;
            if(!sv[2])
                sv[2] = (char*)"0.0";
            m_texcoords.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        }
        else if(strcmp(tok, "usemtl")==0)
        {
            tok = strtok(NULL, DELIM);
            if(strlen(tok) < 1)
                goto cleanup;
            texpath = tok;
        }
        else if(strcmp(tok, "spawn")==0)
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1] || !sv[2])
                continue;
            spawn_point_t spawn;
            spawn.origin = vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2]));
            m_spawnpoints.push_back(spawn);
        }
        else if(strcmp(tok, "f")==0) // face
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1] || !sv[2])
                goto cleanup;

            triangle.texturepath = texpath;
            for(i=0;i<3;i++)
            {
                if(sscanf(sv[i], "%i/%i/%i", &vi, &ti, &ni) != 3)
                    goto cleanup;

                triangle.vertices[i] = (vi-1);
                triangle.normals[i] = (ni-1);
                triangle.texcoords[i] = (ti-1);
            }

            triangle.GeneratePlane(this);
            m_triangles.push_back(triangle);
        }
    }

    fprintf(stderr, "KD-Tree: Building tree from %i vertices in %i triangles\n",
                    (int)m_vertices.size(),
                    (int)m_triangles.size());

    // Vertices u. faces are loaded
    m_nodecount = 0;
    m_leafcount = 0;
    m_outofmem = false;

    for(i = 0; i < (int)m_triangles.size(); i++)
        alltriangles.push_back(i);

    m_root = new CKDNode(this, alltriangles, 0); // we begin with the x-axis (=0)
    if(m_outofmem || !m_root)
    {
        fprintf(stderr, "KD-Tree: not enough memory for tree\n");
        assert(0);
        return false;
    }

    fprintf(stderr, "KD-Tree tree generated: %i nodes from %i triangles. Leafs: %i.\n",
                    m_nodecount,
                    (int)m_triangles.size(),
                    m_leafcount);
    m_filename = file;
    success = true;

cleanup:
    if(success == false)
        fprintf(stderr, "File parse error!\n");
    fclose(f);
    return success;
}

void CKDTree::Unload()
{
    if(m_root)
    {
        delete m_root;
        m_root = NULL;
    }
    m_filename = "";
    m_spawnpoints.clear();
}

std::string CKDTree::GetFilename() const
{
    return m_filename;
}

/*
        Polygon/Plane relation:
        Split Polygon
        Back
        Front
        Coplanar
*/
polyplane_t CKDTree::TestTriangle(const int triindex, const plane_t& plane)
{
    const int size = 3; // triangle size is always 3
    int allfront = 2;
    int allback = 1;

    for(int i=0;i<size;i++)
    {
        switch(plane.Classify(m_vertices[m_triangles[triindex].vertices[i]], 0.0f))
        {
        case POINTPLANE_FRONT:
            allback = 0;
            break;
        case POINTPLANE_BACK:
            allfront = 0;
            break;
        }
    }
    /*
        ALLFRNT ALLBACK     RESULT
        0       0           Split Polygon
        0       1           Back  (1)
        1       0           Front (2)
        1       1           Coplanar
    */
    return (polyplane_t)(allfront | allback);
}

// WriteToBinary Helper Functions

int kdbin_addtexture(std::vector<bspbin_texture_t>& textures, const char* texpath)
{
    int i=0;
    std::vector<bspbin_texture_t>::const_iterator iter;
    for(iter = textures.begin(); iter != textures.end(); iter++)
    {
        if(strcmp(iter->name, texpath) == 0)
        {
            return i;
        }
        i++;
    }
    bspbin_texture_t thistexture;
    memset(&thistexture, 0, sizeof(thistexture));
    strcpy(thistexture.name, texpath);
    textures.push_back(thistexture);
    return textures.size()-1;
}

int kdbin_pushleaf(const CKDTree& tree,
                   const CKDTree::CKDNode* leaf,
                   std::vector<bspbin_leaf_t>& leafs)
{
    std::vector<int>::const_iterator iter;
    int trianglecount = 0;

    bspbin_leaf_t thisleaf;
    memset(&thisleaf, 0, sizeof(thisleaf));

    // fill thisleaf with the right triangle indices:
    for(iter = leaf->triangles.begin(); iter != leaf->triangles.end(); iter++)
    {
        // Save triangle index in leaf
        // *iter is the triangle index tree.m_triangles
        // and m_triangles layout is also used in the file
        thisleaf.triangles[trianglecount] = *iter;
        trianglecount++;
    }

    thisleaf.trianglecount = trianglecount;
    leafs.push_back(thisleaf);

    return -(int)leafs.size(); // leaf indices are stored with a negative index
}

int kdbin_getnodes(const CKDTree& tree,
                   const CKDTree::CKDNode* node,
                   std::vector<bspbin_plane_t>& planes,
                   std::vector<bspbin_node_t>& nodes,
                   std::vector<bspbin_leaf_t>& leafs)
{
    if(node->IsLeaf())
    {
        return kdbin_pushleaf(tree, node, leafs);
    }

    bspbin_plane_t bspplane;
    bspplane.p = node->plane;
    planes.push_back(bspplane);
    const int planeindex = planes.size()-1;

    bspbin_node_t thisnode;
    const int curpos = nodes.size();
    thisnode.plane = planeindex;
    thisnode.radius = node->sphere;
    thisnode.sphere_origin = node->sphere_origin;
    nodes.push_back(thisnode);

    assert(node->front && node->back);
    nodes[curpos].children[0] = kdbin_getnodes(tree,
                                               node->front,
                                               planes,
                                               nodes,
                                               leafs);
    nodes[curpos].children[1] = kdbin_getnodes(tree,
                                               node->back,
                                               planes,
                                               nodes,
                                               leafs);
    return curpos;
}

template<typename Lump>
void WriteLump(FILE*f, const std::vector<Lump>& l)
{
    typename std::vector<Lump>::const_iterator iter;
    for(iter = l.begin();iter != l.end();iter++)
    {
        Lump tl = (*iter);
        fwrite(&tl, sizeof(tl), 1, f);
    }
}

bool CKDTree::WriteToBinary(const std::string filepath)
{
    int i, j;
    FILE* f = fopen(filepath.c_str(), "wb");
    assert(f);
    if(!f)
        return false;

    std::vector<bspbin_plane_t> planes;
    std::vector<bspbin_texture_t> textures;
    std::vector<bspbin_node_t> nodes;
    std::vector<bspbin_leaf_t> leafs;
    std::vector<bspbin_triangle_t> triangles;
    std::vector<bspbin_vertex_t> vertices;
    std::vector<bspbin_spawn_t> spawnpoints;

    // add triangles from tree
    for(i = 0; i < (int)m_triangles.size(); i++) // for each triangle
    {
        bspbin_triangle_t thistriangle;
        memset(&thistriangle, 0, sizeof(thistriangle));

        // if the texture is not yet in "textures", add it and
        // return the index to texturenum
        thistriangle.tex = kdbin_addtexture(textures, m_triangles[i].texturepath.c_str());

        // add vertices for this triangle to the file
        for(j=0; j<3; j++) // for every vertex
        {
            bspbin_vertex_t thisvertex;
            memset(&thisvertex, 0, sizeof(thisvertex));

            thisvertex.v = m_vertices[m_triangles[i].vertices[j]];
            thisvertex.n = m_normals[m_triangles[i].normals[j]];
            thisvertex.tu = m_texcoords[m_triangles[i].texcoords[j]].x;
            thisvertex.tv = m_texcoords[m_triangles[i].texcoords[j]].y;

            // assign this vertex to the current triangle
            thistriangle.v[j] = vertices.size();

            // store the vertex in the file
            vertices.push_back(thisvertex);
        }

        triangles.push_back(thistriangle);
    }

    // Daten aus Baum holen
    kdbin_getnodes(*this,
                    m_root,
                    planes,
                    nodes,
                    leafs);
    // Spawnpoints holen
    std::vector<spawn_point_t>::const_iterator iter;
    for(iter = m_spawnpoints.begin(); iter != m_spawnpoints.end(); iter++)
    {
        bspbin_spawn_t spawn;
        memset(&spawn, 0, sizeof(spawn));
        spawn.point = iter->origin;
        spawn.rot = iter->rot;
        spawnpoints.push_back(spawn);
    }

    if(leafs.size() < 1)
    {
        fprintf(stderr, "KD-Tree: Leaf count < 1\n");
        return false;
    }

    if(nodes.size() == 0)
    {
        bspbin_node_t nullnode;
        nullnode.children[0] = -1;
        nullnode.children[1] = -1;
        nullnode.radius = 99999.999f; // FIXME: this needs a define or something
        nullnode.sphere_origin = vec3_t::origin;
        nullnode.plane = 0;
        nodes.push_back(nullnode);
        bspbin_plane_t nullplane;
        nullplane.p = plane_t(vec3_t::origin, vec3_t::yAxis);
        planes.push_back(nullplane);
    }

    bspbin_direntry_t dirplane;
    bspbin_direntry_t dirtextures;
    bspbin_direntry_t dirnodes;
    bspbin_direntry_t dirleafs;
    bspbin_direntry_t dirpoly;
    bspbin_direntry_t dirvertices;
    bspbin_direntry_t dirspawnpoints;

    bspbin_header_t header;
    header.magic = BSPBIN_MAGIC;
    header.version = BSPBIN_VERSION;

    unsigned int headeroffset = BSPBIN_HEADER_LEN;

    dirplane.offset = headeroffset;
    dirplane.length = planes.size() * sizeof(bspbin_plane_t);
    dirtextures.offset = dirplane.length + dirplane.offset;
    dirtextures.length = textures.size() * sizeof(bspbin_texture_t);
    dirnodes.offset = dirtextures.length + dirtextures.offset;
    dirnodes.length = nodes.size() * sizeof(bspbin_node_t);
    dirleafs.offset = dirnodes.length + dirnodes.offset;
    dirleafs.length = leafs.size() * sizeof(bspbin_leaf_t);
    dirpoly.offset = dirleafs.length + dirleafs.offset;
    dirpoly.length = triangles.size() * sizeof(bspbin_triangle_t);
    dirvertices.offset = dirpoly.length + dirpoly.offset;
    dirvertices.length = vertices.size() * sizeof(bspbin_vertex_t);
    dirspawnpoints.offset = dirvertices.length + dirvertices.offset;
    dirspawnpoints.length = spawnpoints.size() * sizeof(bspbin_spawn_t);

    // Header
    fwrite(&header, sizeof(header), 1, f);
    // Lump table
    fwrite(&dirplane, sizeof(dirplane), 1, f);
    fwrite(&dirtextures, sizeof(dirtextures), 1, f);
    fwrite(&dirnodes, sizeof(dirnodes), 1, f);
    fwrite(&dirleafs, sizeof(dirleafs), 1, f);
    fwrite(&dirpoly, sizeof(dirpoly), 1, f);
    fwrite(&dirvertices, sizeof(dirvertices), 1, f);
    fwrite(&dirspawnpoints, sizeof(dirspawnpoints), 1, f);

    // Lumps
    WriteLump(f, planes);
    WriteLump(f, textures);
    WriteLump(f, nodes);
    WriteLump(f, leafs);
    WriteLump(f, triangles);
    WriteLump(f, vertices);
    WriteLump(f, spawnpoints);

    fclose(f);

    return true;
}

CKDTree::CKDNode::~CKDNode()
{
    if(front)
        delete front;
    if(back)
        delete back;
}

// kdAxis = current KD-Tree axis (x, y or z)
CKDTree::CKDNode::CKDNode(CKDTree* tree, const std::vector<int>& trianglesIn, const int kdAxis)
{
    int count = (int)trianglesIn.size();
    int i;

    tree->m_nodecount++;
    CalculateSphere(tree, trianglesIn);

    // if there are not many triangles left, we create a leaf
    if(count < KDTREE_MAX_TRIANGLES_PER_LEAF)
    {
        fprintf(stderr, "KD-Tree: subspace with %i polygons formed\n", count);

        // save remaining triangles from trianglesIn in this leaf
        // FIXME: what is the right STLish way to do this?
        triangles.clear();
        for(i=0; i<count; i++)
            triangles.push_back(trianglesIn[i]);

        front = NULL;
        back = NULL;
        tree->m_leafcount++;
        return;
    }

    // we still have a lot of triangles,
    // so we find a good (tm) splitting plane now
    // attention: don't write FindSpittingPlane
    plane = FindSplittingPlane(tree, trianglesIn, kdAxis);

    std::vector<int> frontlist;
    std::vector<int> backlist;

    for(i=0;i<count;i++) // for every triangle
    {
        switch(tree->TestTriangle(trianglesIn[i], plane))
        {
        case POLYPLANE_COPLANAR:
        case POLYPLANE_SPLIT:
            frontlist.push_back(trianglesIn[i]);
            backlist.push_back(trianglesIn[i]);
            break;
        case POLYPLANE_FRONT:
            frontlist.push_back(trianglesIn[i]);
            break;
        case POLYPLANE_BACK:
            backlist.push_back(trianglesIn[i]);
            break;
        }
    }

    const int newkdAxis = (kdAxis+1)%3; // keep kdAxis between 0 and 2 (x, y, or z)

    if(frontlist.size() > 0)
    {
        front = new CKDNode(tree, frontlist, newkdAxis);
        if(!front)
            tree->m_outofmem = true;
    }
    else
    {
        front = NULL;
    }

    if(backlist.size() > 0)
    {
        back = new CKDNode(tree, backlist, newkdAxis);
        if(!back)
            tree->m_outofmem = true;
    }
    else
    {
        back = NULL;
    }
}

plane_t CKDTree::CKDNode::FindSplittingPlane(const CKDTree* tree, const std::vector<int>& trianglesIn, const int kdAxis)
{
    static const vec3_t axis[3] = {vec3_t::xAxis,
                                   vec3_t::yAxis,
                                   vec3_t::zAxis};

    int i, j, k;
    int count = (int)trianglesIn.size(); // triangle count
    std::vector<float> coords(count*3);
    // we store every vertex component (x OR y OR z, depending on kdAxis)
    // in the coords array.
    // then the coords array gets sorted and we choose the median as splitting
    // plane.

    k = 0;
    for(i=0; i<count; i++) // for every triangleIn
    {
        for(j = 0; j<3; j++) // for every triangle vertex
        {
            const int vertexindex = tree->m_triangles[trianglesIn[i]].vertices[j];
            coords[k++] = tree->m_vertices[vertexindex].v[kdAxis];
        }
    }

    // sort the coordinates for a L1 median estimation
    // this makes us robust against triangle outliers
    std::sort(coords.begin(), coords.end());
    const float split = coords[coords.size()/2]; // median like
    const vec3_t p(axis[kdAxis]*split); // point
    const vec3_t n(axis[kdAxis]); // normal vector
    return plane_t(p, n); // create plane from point and normal vector
}

void CKDTree::CKDNode::CalculateSphere(const CKDTree* tree, const std::vector<int>& trianglesIn) // Bounding Sphere für diesen Knoten berechnen
{
    vec3_t v, min(0,0,0), max(0,0,0);
    int i, j;
    const int trisize = 3;
    const int isize = (int)trianglesIn.size();
    int triindex; // triangle index
    int vertexindex; // vertex index

    for(i=0;i<isize;i++) // for every triangle
    {
        triindex = trianglesIn[i];
        for(j=0;j<3;j++) // for every vertex
        {
            vertexindex = tree->m_triangles[triindex].vertices[j];
            v = tree->m_vertices[vertexindex];
            if(v.x < min.x)
                min.x = v.x;
            if(v.y < min.y)
                min.y = v.y;
            if(v.z < min.z)
                min.z = v.z;
            if(v.x > max.x)
                max.x = v.x;
            if(v.y > max.y)
                max.y = v.y;
            if(v.z > max.z)
                max.z = v.z;
        }
    }
    vec3_t maxmin = (max - min)*0.5f;
    sphere_origin = min + maxmin;
    sphere = maxmin.Abs();
}

vec3_t kd_tri_t::GetNormal(CKDTree* tree)
{
    if(plane.m_n.IsNull())
        GeneratePlane(tree);
    return plane.m_n;
}

void kd_tri_t::GeneratePlane(CKDTree* tree)
{
    plane.SetupPlane(tree->m_vertices[vertices[2]],
                     tree->m_vertices[vertices[1]],
                     tree->m_vertices[vertices[0]]);

}

