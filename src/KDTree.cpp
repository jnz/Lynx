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

#define KDTREE_EPSILON                 0.01f // Plane offset for point classification (front or back)
#define KDTREE_MAX_TRIANGLES_PER_LEAF  5     // This is no hard limit, leafs can have more triangles, but at this point we don't go further down the tree.
#define KDTREE_MAX_RECURSION_DEPTH     35    // Tree depth

CKDTree::CKDTree(void)
{
    m_root = NULL;
    m_nodecount = 0;
    m_leafcount = 0;
    m_depth = 0;
    m_estimatedtexturecount = 0;
}

CKDTree::~CKDTree(void)
{
    Unload();
}

#pragma warning(disable: 4244)
bool CKDTree::Load(std::string file) // reading a wavefront obj file
{
    bool success = false; // failed to read file
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

    f = fopen(file.c_str(), "rt");
    if(!f)
    {
        fprintf(stderr, "Failed to open file: %s\n", file.c_str());
        return false;
    }

    fprintf(stderr, "Reading file...\n");
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
        //else if(strcmp(tok, "vn")==0) // normal
        //{
            //for(i=0;i<3;i++)
                //sv[i] = strtok(NULL, DELIM);
            //if(!sv[0] || !sv[1] || !sv[2])
                //goto cleanup;
            //m_normals.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        //}
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
            m_estimatedtexturecount++;
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
                continue;

            triangle.texturepath = texpath;
            for(i=0;i<3;i++)
            {
                if(sscanf(sv[i], "%i/%i/%i", &vi, &ti, &ni) != 3)
                    goto cleanup;

                triangle.vertices[i] = (vi-1);
                triangle.texcoords[i] = (ti-1);
            }

            triangle.GeneratePlane(this);
            m_triangles.push_back(triangle);
        }
    }

    fprintf(stderr, "Finished reading, building tree from %i vertices in %i triangles\n",
                    (int)m_vertices.size(),
                    (int)m_triangles.size());

    if(m_spawnpoints.size() < 1)
    {
        fprintf(stderr, "Warning: No spawn points found. Adding default spawn point at: 0.0 0.0 0.0\n");
        spawn_point_t spawn;
        spawn.origin = vec3_t(0.0f, 0.0f, 0.0f);
        m_spawnpoints.push_back(spawn);
    }

    // Vertices u. faces are loaded
    m_nodecount = 0;
    m_leafcount = 0;

    // Calculating vertex normals
    // fprintf(stderr, "Calculating vertex normals... ");
    // CalculateVertexNormals();
    // fprintf(stderr, "finished.\n");

    fprintf(stderr, "Creating triangle indices\n");
    alltriangles.reserve(m_triangles.size());
    for(i = 0; i < (int)m_triangles.size(); i++)
        alltriangles.push_back(i);

    fprintf(stderr, "Starting binary space partitioning...\n");
    m_root = new CKDNode(this, alltriangles, 0); // we begin with the x-axis (=0)
    if(m_root == NULL)
    {
        fprintf(stderr, "KD-Tree: not enough memory for tree\n");
        goto cleanup;
    }

    fprintf(stderr, "KD-Tree tree generated: %i nodes from %i triangles. Leafs: %i. Depth: %i\n",
                    m_nodecount-m_leafcount, // leafs also count as nodes
                    (int)m_triangles.size(),
                    m_leafcount,
                    m_depth);
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
    m_estimatedtexturecount = 0;
    m_depth = 0;
    m_vertices.clear();
    // m_normals.clear();
    m_texcoords.clear();
    m_triangles.clear();

    m_nodecount = 0;
    m_leafcount = 0;
    m_depth = 0;
}

/*
 *void CKDTree::CalculateVertexNormals()
 *{
 *    const unsigned int vertexcount = m_vertices.size();
 *    const unsigned int trianglecount = m_triangles.size();
 *    std::vector<vec3_t> normals;
 *    vec3_t normal;
 *    unsigned int vindex;
 *    unsigned int tindex;
 *    unsigned int v0, v1, v2; // vertex indices
 *
 *    normals.resize(vertexcount);
 *
 *    for(tindex = 0; tindex < trianglecount; tindex++)
 *    {
 *        v0 = m_triangles[tindex].vertices[0]; // vertex index 0
 *        v1 = m_triangles[tindex].vertices[1]; // vertex index 1
 *        v2 = m_triangles[tindex].vertices[2]; // vertex index 2
 *        const vec3_t& p0 = m_vertices[v0];
 *        const vec3_t& p1 = m_vertices[v1];
 *        const vec3_t& p2 = m_vertices[v2];
 *
 *        normal = (p1-p0)^(p2-p0);
 *
 *        normals[v0] += normal;
 *        normals[v1] += normal;
 *        normals[v2] += normal;
 *    }
 *
 *    m_normals.resize(vertexcount);
 *    for(vindex = 0; vindex < vertexcount; vindex++)
 *    {
 *        m_normals[vindex] = normals[vindex].Normalized();
 *    }
 *}
 */

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
polyplane_t CKDTree::TestTriangle(const int triindex, const plane_t& plane) const
{
    /*
        Check where the triangle is with respect to the plane.
        The polyplane_t return value is created with
        some bit combination.
        ALLFRNT ALLBACK     RESULT
        0       0           Split Polygon
        0       1           Back  (1)
        1       0           Front (2)
        1       1           Coplanar
        ----------------------------------
        Biwise: (allfront | allback)
    */
    int allfront = 2;
    int allback = 1;
    const int vindex[3] =  // index to the 3 triangle vertices
    {
        m_triangles[triindex].vertices[0],
        m_triangles[triindex].vertices[1],
        m_triangles[triindex].vertices[2]
    };

    for(int i=0;i<3;i++)
    {
        switch(plane.Classify(m_vertices[vindex[i]], KDTREE_EPSILON))
        {
        case POINTPLANE_FRONT:
            allback = 0;
            break;
        case POINTPLANE_BACK:
            allfront = 0;
            break;
        }
    }

    return (polyplane_t)(allfront | allback);
}

// WriteToBinary Helper Functions

// Computation of the tangent according to Lengyel p. 186
// return vectors are not normalized
static void kdbin_calculate_tangent(const std::vector<bspbin_triangle_t>& triangles,
                                    const std::vector<bspbin_vertex_t>& vertices,
                                    const int triangleindex,
                                    vec3_t& facenormal,
                                    vec3_t& tangent,
                                    vec3_t& bitangent)
{
    const vec3_t& p0 = vertices[triangles[triangleindex].v[0]].v;
    const vec3_t& p1 = vertices[triangles[triangleindex].v[1]].v;
    const vec3_t& p2 = vertices[triangles[triangleindex].v[2]].v;
    const float s1 = vertices[triangles[triangleindex].v[1]].tu - vertices[triangles[triangleindex].v[0]].tu;
    const float s2 = vertices[triangles[triangleindex].v[2]].tu - vertices[triangles[triangleindex].v[0]].tu;
    const float t1 = vertices[triangles[triangleindex].v[1]].tv - vertices[triangles[triangleindex].v[0]].tv;
    const float t2 = vertices[triangles[triangleindex].v[2]].tv - vertices[triangles[triangleindex].v[0]].tv;
    const float det = s1*t2 - s2*t1;
    const vec3_t Q1(p1 - p0);
    const vec3_t Q2(p2 - p0);
    if(fabsf(det) <= lynxmath::EPSILON)
    {
        fprintf(stderr, "Warning: Unable to compute tangent + bitangent\n");
        tangent = vec3_t(1.0f, 0.0f, 0.0f);
        bitangent = vec3_t(0.0f, 1.0f, 0.0f);
        return;
    }
    const float idet = 1 / det;
    tangent = vec3_t(idet*(t2*Q1.x - t1*Q2.x),
                     idet*(t2*Q1.y - t1*Q2.y),
                     idet*(t2*Q1.z - t1*Q2.z));
    bitangent = vec3_t(idet*(-s2*Q1.x + s1*Q2.x),
                       idet*(-s2*Q1.y + s1*Q2.y),
                       idet*(-s2*Q1.z + s1*Q2.z));
    facenormal = (p1-p0)^(p2-p0);

    assert(tangent.Abs()>0.01f);
    assert(bitangent.Abs()>0.01f);
}

// Calculate the vertex normals and tangents.
// Lengyel's algorithm for tangents and bitangents is used here
static void kdbin_create_tangents(std::vector<bspbin_vertex_t>& vertices,
                                  const std::vector<bspbin_triangle_t>& triangles)
{
    const unsigned int vertexcount = vertices.size();
    const unsigned int trianglecount = triangles.size();
    std::vector<vec3_t> normals;
    std::vector<vec3_t> tangents;
    std::vector<vec3_t> bitangents;
    vec3_t normal, tangent, bitangent;
    unsigned int vindex;
    unsigned int tindex;
    unsigned int v0, v1, v2; // vertex indices

    normals.resize(vertexcount);
    tangents.resize(vertexcount);
    bitangents.resize(vertexcount);

    for(tindex = 0; tindex < trianglecount; tindex++)
    {
        kdbin_calculate_tangent(triangles,
                                vertices,
                                tindex,
                                normal,
                                tangent,
                                bitangent);

        v0 = triangles[tindex].v[0]; // vertex index 0
        v1 = triangles[tindex].v[1]; // vertex index 1
        v2 = triangles[tindex].v[2]; // vertex index 2

        normals[v0] += normal;
        normals[v1] += normal;
        normals[v2] += normal;
        tangents[v0] += tangent;
        tangents[v1] += tangent;
        tangents[v2] += tangent;
        bitangents[v0] += bitangent;
        bitangents[v1] += bitangent;
        bitangents[v2] += bitangent;
    }

    for(vindex = 0; vindex < vertexcount; vindex++)
    {
        vertices[vindex].n = normals[vindex].Normalized();
        const vec3_t& n = vertices[vindex].n;
        vec3_t t = tangents[vindex];

        // tangent
        // Gram-Schmidt orthogonalize
        vertices[vindex].t = (t - n * (n*t)).Normalized();

        if(!vertices[vindex].t.IsNormalized())
        {
            fprintf(stderr, "Failed to calculate vertex tangent in vertex %i\n", vindex);
            fprintf(stderr, "V%i v: %3.2f %3.2f %3.2f n: %3.2f %3.2f %3.2f t: %3.2f %3.2f %3.2f t before Gram-Schmidt: %3.2f %3.2f %3.2f b: %3.2f %3.2f %3.2f\n",
                    vindex,
                    vertices[vindex].v.x, vertices[vindex].v.y, vertices[vindex].v.z,
                    vertices[vindex].n.x, vertices[vindex].n.y, vertices[vindex].n.z,
                    vertices[vindex].t.x, vertices[vindex].t.y, vertices[vindex].t.z,
                    t.x, t.y, t.z,
                    bitangents[vindex].x, bitangents[vindex].y, bitangents[vindex].z
                   );
            // HACKHACKHACK we just just this as tangent vector.
            vertices[vindex].t = vec3_t(1.0f, 0.0f, 0.0f);
            t = vertices[vindex].t;
        }

        // the bitangent is only stored by its handedness in the w component (-1 or 1)
        // Calculate handedness
        vertices[vindex].w = (((n^t) * bitangents[vindex]) < 0.0f) ? -1.0f : 1.0f;
    }
}

static int kdbin_addtexture(std::vector<bspbin_texture_t>& textures, const char* texpath)
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

static int kdbin_pushleaf(const CKDTree& tree,
                   const CKDTree::CKDNode* leaf,
                   std::vector<bspbin_leaf_t>& leafs)
{
    bspbin_leaf_t thisleaf;
    // fill thisleaf with the right triangle indices:
    thisleaf.triangles.insert(thisleaf.triangles.begin(),
                              leaf->m_triangles.begin(),
                              leaf->m_triangles.end());

    leafs.push_back(thisleaf);

    return -(int)leafs.size(); // leaf indices are stored with a negative index
}

// returns true if the vertex position and uv coordinates are equal
static bool kdbin_compare_vertex_and_uv(const bspbin_vertex_t& vA,
                                 const bspbin_vertex_t& vB,
                                 const float epsilon)
{
    // return false;
    return vA.v.Equals(vB.v, epsilon) &&
           (fabsf(vA.tu-vB.tu) < epsilon) &&
           (fabsf(vA.tv-vB.tv) < epsilon);
}

static int kdbin_getnodes(const CKDTree& tree,
                   const CKDTree::CKDNode* node,
                   std::vector<bspbin_plane_t>& planes,
                   std::vector<bspbin_node_t>& nodes,
                   std::vector<bspbin_leaf_t>& leafs)
{
    if(node->IsLeaf())
    {
        return kdbin_pushleaf(tree, node, leafs);
    }

    // we only save the m_d component and the axis type in the file
    bspbin_plane_t bspplane;
    bspplane.d = node->m_plane.m_d;
    for(int i=0;i<3;i++)
    {
        if(fabs(node->m_plane.m_n.v[i]) > 0.0f)
        {
            bspplane.type = i;
            break;
        }
    }

    planes.push_back(bspplane);
    const int planeindex = planes.size()-1;

    bspbin_node_t thisnode;
    const int curpos = nodes.size();
    thisnode.plane = planeindex;
    thisnode.radius = node->m_sphere;
    thisnode.sphere_origin = node->m_sphere_origin;
    nodes.push_back(thisnode);

    assert(node->m_front && node->m_back);
    nodes[curpos].children[0] = kdbin_getnodes(tree,
                                               node->m_front,
                                               planes,
                                               nodes,
                                               leafs);
    nodes[curpos].children[1] = kdbin_getnodes(tree,
                                               node->m_back,
                                               planes,
                                               nodes,
                                               leafs);
    return curpos;
}

// Write leaf writes all leafs of the KDTree to the
// file. This function can not be replaced by the generic
// WriteLump function, because the bspbin_leaf_t structure
// has a dynamic length.
static void WriteLeafs(FILE* f, const std::vector<bspbin_leaf_t>& leafs)
{
    uint32_t i;
    uint32_t trianglecount;
    uint32_t triangleindex;
    std::vector<bspbin_leaf_t>::const_iterator iter;
    for(iter = leafs.begin();iter != leafs.end();iter++)
    {
        trianglecount = (*iter).triangles.size();
        fwrite(&trianglecount, sizeof(trianglecount), 1, f);
        for(i = 0; i < trianglecount; i++)
        {
            triangleindex = (*iter).triangles[i];
            fwrite(&triangleindex, sizeof(triangleindex), 1, f);
        }
    }
}

template<typename Lump>
static void WriteLump(FILE*f, const std::vector<Lump>& l)
{
    typename std::vector<Lump>::const_iterator iter;
    for(iter = l.begin();iter != l.end();iter++)
    {
        const Lump& tl = (*iter);
        fwrite(&tl, sizeof(tl), 1, f);
    }
}

/*
LBSP file layout

HEADER
bspbin_direntry_t dirplane; // offset and length in bytes
bspbin_direntry_t dirtextures; // offset and length in bytes
bspbin_direntry_t dirnodes; // offset and length in bytes
bspbin_direntry_t dirpoly; // offset and length in bytes
bspbin_direntry_t dirvertices; // offset and length in bytes
bspbin_direntry_t dirspawnpoints; // offset and length in bytes
bspbin_direntry_t dirleafs; // offset and count of leafs (!)
planes
textures
nodes
triangles
vertices
spawnpoints
leafs: for every leaf there is a trianglecount (uint32_t), followed
       by the triangle indices (uint32_t) of this leaf, then the next leaf
       follows with its trianglecount a.s.o.
*/
bool CKDTree::WriteToBinary(const std::string filepath)
{
    unsigned int i, j;
    int k;
    FILE* f = fopen(filepath.c_str(), "wb");
    assert(f);
    if(!f)
        return false;

    std::vector<bspbin_plane_t> planes;
    std::vector<bspbin_texture_t> textures;
    std::vector<bspbin_node_t> nodes;
    std::vector<bspbin_triangle_t> triangles;
    std::vector<bspbin_vertex_t> vertices;
    std::vector<bspbin_spawn_t> spawnpoints;
    std::vector<bspbin_leaf_t> leafs;

    planes.reserve(m_nodecount-m_leafcount);
    textures.reserve(m_estimatedtexturecount);
    nodes.reserve(m_nodecount-m_leafcount);
    triangles.reserve(m_triangles.size());
    vertices.reserve(triangles.size()*3);
    leafs.reserve(m_leafcount);

    // add triangles from tree
    fprintf(stderr, "Building vertices\n");
    for(i = 0; i < m_triangles.size(); i++) // for each triangle
    {
        bspbin_triangle_t thistriangle;

        // if the texture is not yet in "textures", add it and
        // return the index to texturenum
        thistriangle.tex = kdbin_addtexture(textures, m_triangles[i].texturepath.c_str());

        // add vertices for this triangle to the file
        for(j=0; j<3; j++) // for every vertex
        {
            bspbin_vertex_t thisvertex;
            thisvertex.v = m_vertices[m_triangles[i].vertices[j]];
            // thisvertex.n = m_normals[m_triangles[i].vertices[j]];
            thisvertex.tu = m_texcoords[m_triangles[i].texcoords[j]].x;
            thisvertex.tv = m_texcoords[m_triangles[i].texcoords[j]].y;

            for(k = vertices.size()-1; k >= 0; k--)
            {
                if(kdbin_compare_vertex_and_uv(vertices[k],
                                               thisvertex,
                                               lynxmath::EPSILON))
                {
                    break;
                }
            }
            if(k >= vertices.size()) // no matching vertex found
            {
                vertices.push_back(thisvertex);
                k = vertices.size()-1;
            }
            thistriangle.v[j] = k;
        }

        triangles.push_back(thistriangle);
        if(i%100==0)
            fprintf(stderr, ".");
    }
    fprintf(stderr, "\nVertices for file: %i\n", (int)vertices.size());
    if(vertices.size() > USHRT_MAX)
    {
        fprintf(stderr, "Information: file has more than %i vertices\n", USHRT_MAX);
    }

    // The tree is stored with pointers in memory.
    // Now we want to write this structure to the file.
    fprintf(stderr, "Building tree structure\n");
    kdbin_getnodes(*this,
                    m_root,
                    planes,
                    nodes,
                    leafs);
    // Get spawnpoints
    std::vector<spawn_point_t>::const_iterator iter;
    for(iter = m_spawnpoints.begin(); iter != m_spawnpoints.end(); iter++)
    {
        bspbin_spawn_t spawn;
        memset(&spawn, 0, sizeof(spawn));
        spawn.point = iter->origin;
        spawn.rot = iter->rot;
        spawnpoints.push_back(spawn);
    }

    // Create tangent vectors for each vertex
    fprintf(stderr, "Creating tangent vectors\n");
    kdbin_create_tangents(vertices, triangles);

    if(leafs.size() < 1)
    {
        fprintf(stderr, "KD-Tree error: leaf count < 1\n");
        return false;
    }

    if(nodes.size() == 0)
    {
        bspbin_node_t nullnode;
        nullnode.children[0] = -1;
        nullnode.children[1] = -1;
        nullnode.radius = MAX_TRACE_DIST-1.0f;
        nullnode.sphere_origin = vec3_t::origin;
        nullnode.plane = 0;
        nodes.push_back(nullnode);
        bspbin_plane_t nullplane;
        nullplane.type = 0;
        nullplane.d = 0;
        planes.push_back(nullplane);
    }

    // Now we have all the required information to write the file.

    bspbin_direntry_t dirplane;
    bspbin_direntry_t dirtextures;
    bspbin_direntry_t dirnodes;
    bspbin_direntry_t dirpoly;
    bspbin_direntry_t dirvertices;
    bspbin_direntry_t dirspawnpoints;
    bspbin_direntry_t dirleafs;

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
    dirpoly.offset = dirnodes.length + dirnodes.offset;
    dirpoly.length = triangles.size() * sizeof(bspbin_triangle_t);
    dirvertices.offset = dirpoly.length + dirpoly.offset;
    dirvertices.length = vertices.size() * sizeof(bspbin_vertex_t);
    dirspawnpoints.offset = dirvertices.length + dirvertices.offset;
    dirspawnpoints.length = spawnpoints.size() * sizeof(bspbin_spawn_t);

    dirleafs.offset = dirspawnpoints.length + dirspawnpoints.offset;
    dirleafs.length = leafs.size(); // this is not the length in bytes, but the leaf count

    // Header
    fwrite(&header, sizeof(header), 1, f);
    // Lump table
    fwrite(&dirplane, sizeof(dirplane), 1, f);
    fwrite(&dirtextures, sizeof(dirtextures), 1, f);
    fwrite(&dirnodes, sizeof(dirnodes), 1, f);
    fwrite(&dirpoly, sizeof(dirpoly), 1, f);
    fwrite(&dirvertices, sizeof(dirvertices), 1, f);
    fwrite(&dirspawnpoints, sizeof(dirspawnpoints), 1, f);
    fwrite(&dirleafs, sizeof(dirleafs), 1, f);

    // Lumps
    WriteLump(f, planes);
    WriteLump(f, textures);
    WriteLump(f, nodes);
    WriteLump(f, triangles);
    WriteLump(f, vertices);
    WriteLump(f, spawnpoints);
    WriteLeafs(f, leafs); // special function for leafs

    const uint32_t endmark = BSPBIN_MAGIC; // for the reader to check if the file is valid
    fwrite(&endmark, sizeof(endmark), 1, f);

    fclose(f);

    return true;
}

CKDTree::CKDNode::~CKDNode()
{
    if(m_front)
        delete m_front;
    if(m_back)
        delete m_back;
}


// kdAxis = current KD-Tree axis (x, y or z)
CKDTree::CKDNode::CKDNode(CKDTree* tree, const std::vector<int>& trianglesIn, const int recursionDepth)
{
    int trianglecount = (int)trianglesIn.size(); // number of input triangles
    //const int kdAxis = recursionDepth%3; // keep axis between 0 and 2 (x, y, or z)

    if(recursionDepth > tree->m_depth)
        tree->m_depth = recursionDepth;
    if(recursionDepth > KDTREE_MAX_RECURSION_DEPTH)
    {
        fprintf(stderr, "Unable to compile polygon soup. Recursion depth too deep: %i.", recursionDepth);
        assert(0);
        exit(0);
    }

    tree->m_nodecount++; // even if this ends up as a leaf, we count it as a node
    CalculateSphere(tree, trianglesIn); // calculate node bounding sphere

    std::vector<int> flX, flY, flZ; // front list
    std::vector<int> blX, blY, blZ; // back list
    std::vector<int> slX, slY, slZ; // split list
    std::vector<int>* frontlist[] = {&flX, &flY, &flZ}; // every triangle in front of the plane will be stored here
    std::vector<int>* backlist[] = {&blX, &blY, &blZ}; // every triangle behind the plane will be stored here
    std::vector<int>* splitlist[] = {&slX, &slY, &slZ}; // every triangle on the splitting plane
    plane_t plane[3];
    float ratioFrontBack[3]; // ratio between the frontlist size and the backlist size or vice versa (always <= 1.0f)

    int i;
    int best=recursionDepth%3; // best plane (x, y, z)
    // we try to find out which splitting plane gives us
    // a good balanced node (x, y or z)
    for(i=0;i<3;i++)
    {
        SplitTrianglesAlongAxis(tree,
                                trianglesIn,
                                i,
                                *frontlist[i],
                                *backlist[i],
                                *splitlist[i],
                                plane[i]);

        const unsigned int flsize = (*frontlist[i]).size();
        const unsigned int blsize = (*backlist[i]).size();
        if(flsize > blsize)
            ratioFrontBack[i] = (float)blsize / (float)flsize;
        else
            ratioFrontBack[i] = (float)flsize / (float)blsize;
    }

    float bestratio = 0.0f;
    for(i=0;i<3;i++)
    {
        if(ratioFrontBack[i] > bestratio)
        {
            best = i;
            bestratio = ratioFrontBack[i];
        }
    }

    m_plane = plane[best];

    // if there are not many triangles left, we create a leaf
    // or we can't do any further subdivision
    if(frontlist[best]->size() < 1 ||
       backlist[best]->size() < 1 ||
       trianglecount <= KDTREE_MAX_TRIANGLES_PER_LEAF)
    {
        fprintf(stderr, "KD-Tree: subspace with %i polygons formed\n", trianglecount);

        // save remaining triangles from trianglesIn in this leaf
        m_triangles.insert(m_triangles.begin(),
                           trianglesIn.begin(),
                           trianglesIn.end());

        m_front = NULL;
        m_back = NULL;
        tree->m_leafcount++;
        return;
    }

    // add the split triangles to both sides of the plane
    frontlist[best]->insert(frontlist[best]->end(),
                           splitlist[best]->begin(),
                           splitlist[best]->end());
    backlist[best]->insert(backlist[best]->end(),
                          splitlist[best]->begin(),
                          splitlist[best]->end());

    m_front = new CKDNode(tree, *frontlist[best], recursionDepth+1);
    m_back = new CKDNode(tree, *backlist[best], recursionDepth+1);
}

void CKDTree::CKDNode::SplitTrianglesAlongAxis(const CKDTree* tree,
    const std::vector<int>& trianglesIn,
    const int kdAxis,
    std::vector<int>& frontlist,
    std::vector<int>& backlist,
    std::vector<int>& splitlist,
    plane_t& splitplane) const
{
    int trianglecount = trianglesIn.size();
    // find a good (tm) splitting plane now
    // attention: don't write FindSpittingPlane
    splitplane = FindSplittingPlane(tree, trianglesIn, kdAxis);

    for(int i=0;i<trianglecount;i++) // for every triangle
    {
        switch(tree->TestTriangle(trianglesIn[i], splitplane))
        {
        case POLYPLANE_COPLANAR:
        case POLYPLANE_SPLIT:
            splitlist.push_back(trianglesIn[i]);
            break;
        case POLYPLANE_FRONT:
            frontlist.push_back(trianglesIn[i]);
            break;
        case POLYPLANE_BACK:
            backlist.push_back(trianglesIn[i]);
            break;
        }
    }
}

plane_t CKDTree::CKDNode::FindSplittingPlane(const CKDTree* tree, const std::vector<int>& trianglesIn, const int kdAxis) const
{
    static const vec3_t axis[3] = {vec3_t::xAxis,
                                   vec3_t::yAxis,
                                   vec3_t::zAxis};

    int i, j, k;
    int trianglecount = (int)trianglesIn.size();
    std::vector<float> coords(trianglecount*3); // every triangle has 3 vertices
    // we store every vertex component (x OR y OR z, depending on kdAxis)
    // in the coords array.
    // then the coords array gets sorted and we choose the median as splitting
    // plane.
    assert(kdAxis >= 0 && kdAxis <= 2);
    assert(trianglecount > 0);

    k = 0;
    for(i=0; i<trianglecount; i++) // for every triangleIn
    {
        for(j = 0; j<3; j++) // for every triangle vertex
        {
            const int vertexindex = tree->m_triangles[trianglesIn[i]].vertices[j];
            coords[k++] = tree->m_vertices[vertexindex].v[kdAxis];
        }
    }

    // sort the coordinates for a L1 median estimation
    // this makes us robust against vertex outliers
    std::sort(coords.begin(), coords.end());
    const float split = coords[coords.size()/2]; // median like
    const vec3_t p(axis[kdAxis]*split); // point
    const vec3_t n(axis[kdAxis]); // normal vector
    return plane_t(p, n); // create plane from point and normal vector
}

// Bounding sphere for this node
void CKDTree::CKDNode::CalculateSphere(const CKDTree* tree, const std::vector<int>& trianglesIn)
{
    vec3_t min(0.0f,0.0f,0.0f), max(0.0f,0.0f,0.0f);
    int i, j;
    const int tricount = (int)trianglesIn.size();
    int triindex; // triangle index
    int vertexindex; // vertex index

    for(i=0;i<tricount;i++) // for every triangle
    {
        triindex = trianglesIn[i];
        for(j=0;j<3;j++) // for every vertex in triangle
        {
            vertexindex = tree->m_triangles[triindex].vertices[j];
            const vec3_t* v = &tree->m_vertices[vertexindex];
            if(v->x < min.x)
                min.x = v->x;
            if(v->y < min.y)
                min.y = v->y;
            if(v->z < min.z)
                min.z = v->z;
            if(v->x > max.x)
                max.x = v->x;
            if(v->y > max.y)
                max.y = v->y;
            if(v->z > max.z)
                max.z = v->z;
        }
    }
    vec3_t maxmin = (max - min)*0.5f;
    m_sphere_origin = min + maxmin;
    m_sphere = maxmin.Abs();

    if(m_sphere > MAX_TRACE_DIST)
    {
        fprintf(stderr, "Level geometry too large. Scale down the level. Max. distance: %f\n", MAX_TRACE_DIST);
        assert(0);
        exit(0);
    }
}

vec3_t kd_tri_t::GetNormal(CKDTree* tree)
{
    if(plane.m_n.IsNull())
        GeneratePlane(tree);
    return plane.m_n;
}

void kd_tri_t::GeneratePlane(CKDTree* tree)
{
    plane.SetupPlane(tree->m_vertices[vertices[0]],
                     tree->m_vertices[vertices[1]],
                     tree->m_vertices[vertices[2]]);

}

