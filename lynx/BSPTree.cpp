#include "BSPTree.h"
#include <stdio.h>
#include <math.h>
#include "math/mathconst.h"
#include "BSPBIN.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MAX_POLY_PER_LEAF       0
#define BSP_EPSILON             (0.003125f)

CBSPTree::CBSPTree(void)
{
    m_root = NULL;
    m_leafcount = 0;
}

CBSPTree::~CBSPTree(void)
{
    Unload();
}

#pragma warning(disable: 4244)
bool CBSPTree::Load(std::string file)
{
    const char* DELIM = " \t\r\n";
    FILE* f;
    char line[1024];
    char* tok;
    char* sv[3];
    int i;
    int vi, ti, ni;
    int texture=0;
    bsp_poly_t polygon;
    int nonplanar = 0; // anzahl der nicht-ebenen polygone zählen
    std::string texpath;

    Unload();

    f = fopen(file.c_str(), "rb");
    if(!f)
        return false;

    while(!feof(f))
    {
        fgets(line, sizeof(line), f);
        tok = strtok(line, DELIM);

        if(strcmp(tok, "v")==0) // vertex
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1] || !sv[2])
                continue;
            m_vertices.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        }
        else if(strcmp(tok, "vn")==0) // normal
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1] || !sv[2])
                continue;
            m_normals.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        }
        else if(strcmp(tok, "vt")==0) // textures
        {
            for(i=0;i<3;i++)
                sv[i] = strtok(NULL, DELIM);
            if(!sv[0] || !sv[1])
                continue;
            if(!sv[2])
                sv[2] = "0.0";
            m_texcoords.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
        }
        else if(strcmp(tok, "usemtl")==0)
        {
            tok = strtok(NULL, DELIM);
            texpath = tok;
            //if(resman)
            //{
            //    texture = resman->GetTexture(dir + tok);
            //    if(texture == 0)
            //    {
            //        fprintf(stderr, "BSP: Texture not found: %s\n", tok);
            //        assert(0);
            //        return false;
            //    }
            //}
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
            polygon.Clear();
            polygon.texture = texture;
            polygon.texturepath = texpath;
            for(i=0;i<3;i++)
            {
                sscanf(sv[i], "%i/%i/%i", &vi, &ti, &ni);

                polygon.vertices.push_back(vi-1);
                polygon.normals.push_back(ni-1);
                polygon.texcoords.push_back(ti-1);
            }
            if(polygon.IsPlanar(this))
            {
                m_polylist.push_back(polygon);
            }
            else
            {
                /*
                fprintf(stderr, "BSP: Nonplanar triangle (%.1f/%.1f/%.1f)(%.1f/%.1f/%.1f)(%.1f/%.1f/%.1f)\n", 
                            m_vertices[polygon.vertices[0]].x, 
                            m_vertices[polygon.vertices[0]].y, 
                            m_vertices[polygon.vertices[0]].z, 
                            m_vertices[polygon.vertices[1]].x, 
                            m_vertices[polygon.vertices[1]].y, 
                            m_vertices[polygon.vertices[1]].z, 
                            m_vertices[polygon.vertices[2]].x, 
                            m_vertices[polygon.vertices[2]].y, 
                            m_vertices[polygon.vertices[2]].z);
                */
                nonplanar++;
            }
        }
    }
    fclose(f);

    fprintf(stderr, "BSP: Building tree from %i vertices in %i faces\n", 
                    m_vertices.size(), 
                    m_polylist.size());
    fprintf(stderr, "BSP: Ignoring %i non-planar polygons\n", nonplanar);

    // Vertices u. faces wurde geladen
    m_nodecount = 0;
    m_leafcount = 0;
    m_outofmem = false;
    m_root = new CBSPNode(this, m_polylist);
    if(m_outofmem || !m_root)
    {
        fprintf(stderr, "BSP: not enough memory for tree\n");
        assert(0);
        return false;
    }

    int left, right;
    GetLeftRightScore(&left, &right);
    fprintf(stderr, "BSP tree generated: %i nodes from %i polygons. Left: %i nodes. Right: %i nodes. Leafs: %i. Ratio: %.2f\n", 
                    m_nodecount,
                    (int)m_polylist.size(),
                    left,
                    right,
                    m_leafcount,
                    (float)left / (float)right);

    m_filename = file;

    return true;
}

void CBSPTree::Unload()
{
    if(m_root)
    {
        delete m_root;
        m_root = NULL;
    }
    m_filename = "";
    m_spawnpoints.clear();
}

static const spawn_point_t s_spawn_default;
spawn_point_t CBSPTree::GetRandomSpawnPoint() const
{
    if(m_spawnpoints.size() < 1)
        return s_spawn_default;
    return m_spawnpoints[rand()%(m_spawnpoints.size())];
}

std::string CBSPTree::GetFilename() const
{
    return m_filename;
}

void CBSPTree::TraceRay(const vec3_t& start, const vec3_t& dir, float* f) const
{
    TraceRay(start, dir, f, m_root);
}

void CBSPTree::TraceRay(const vec3_t& start, const vec3_t& dir, float* f, const CBSPNode* node) const
{
    if(node->IsLeaf())
    {
        float cf;
        float minf = MAX_TRACE_DIST;
        int minindex = -1;
        int size = (int)node->polylist.size();
        for(int i=0;i<size;i++)
        {
            if(node->polylist[i].plane.m_n * dir > 0.0f)
                continue;

            if(node->polylist[i].GetIntersectionPoint(start, dir, &cf, this))
                if(cf < minf)
                {
                    minf = cf;
                    minindex = i;
                }
        }
        *f = minf;
        return;
    }

    pointplane_t locstart = node->plane.Classify(start, BSP_EPSILON);
    pointplane_t locend = node->plane.Classify(start+dir, BSP_EPSILON);

    if(node->front && locstart > POINT_ON_PLANE && locend > POINT_ON_PLANE)
    {
        TraceRay(start, dir, f, node->front);
        return;
    }
    if(node->back && locstart < POINT_ON_PLANE && locend < POINT_ON_PLANE)
    {
        TraceRay(start, dir, f, node->back);
        return;
    }

    float f1=MAX_TRACE_DIST, f2=MAX_TRACE_DIST;
    if(node->front)
        TraceRay(start, dir, &f1, node->front);
    if(node->back)
        TraceRay(start, dir, &f2, node->back);

    if(f1 < f2)
        *f = f1;
    else
        *f = f2;
}

void CBSPTree::TraceSphere(bsp_sphere_trace_t* trace) const
{
    if(m_root == NULL)
    {
        trace->f = MAX_TRACE_DIST;
        return;
    }
    TraceSphere(trace, m_root);
}

void CBSPTree::TraceSphere(bsp_sphere_trace_t* trace, const CBSPNode* node) const
{
    if(node->IsLeaf())
    {
        const int size = (int)node->polylist.size();
        float cf;
        float minf = MAX_TRACE_DIST;
        int minindex = -1;
        plane_t hitplane;
        vec3_t hitpoint;
        vec3_t normal;
        for(int i=0;i<size;i++)
        {
            if(node->polylist[i].plane.m_n * trace->dir > BSP_EPSILON) // backface culling
                continue;

            // - Prüfen ob Polygonfläche getroffen wird
            // - Prüfen ob Polygon Edge getroffen wird
            // - Prüfen ob Polygon Vertex getroffen wird
            if(node->polylist[i].GetIntersectionPoint(trace->start, 
                                                      trace->dir, &cf, 
                                                      this, trace->radius))
            {
                if(cf < minf)
                {
                    minf = cf;
                    minindex = i;
                    hitplane = node->polylist[minindex].plane;
                }
            }
            else if(node->polylist[i].GetEdgeIntersection(trace->start,
                                                          trace->dir,
                                                          &cf, trace->radius, 
                                                          &normal, &hitpoint, this) || 
               node->polylist[i].GetVertexIntersection(trace->start,
                                                       trace->dir,
                                                       &cf, trace->radius, 
                                                       &normal, &hitpoint, this))
            {
                if(cf < minf)
                {
                    minf = cf;
                    minindex = i;
                    hitplane.SetupPlane(hitpoint, normal);
                }
            }
        }
        trace->f = minf;
        if(minindex != -1)
        {
            trace->p = hitplane;
        }
        return;
    }

    pointplane_t locstart;
    pointplane_t locend;

    // Prüfen, ob alles vor der Splitplane liegt
    plane_t tmpplane = node->plane;
    tmpplane.m_d -= trace->radius;
    locstart = tmpplane.Classify(trace->start, BSP_EPSILON);
    locend = tmpplane.Classify(trace->start + trace->dir, BSP_EPSILON);
    if(node->front && locstart > POINT_ON_PLANE && locend > POINT_ON_PLANE)
    {
        TraceSphere(trace, node->front);
        return;
    }

    // Prüfen, ob alles hinter der Splitplane liegt
    tmpplane = node->plane;
    tmpplane.m_d += trace->radius;
    locstart = tmpplane.Classify(trace->start, BSP_EPSILON);
    locend = tmpplane.Classify(trace->start + trace->dir, BSP_EPSILON);
    if(node->back && locstart < POINT_ON_PLANE && locend < POINT_ON_PLANE)
    {
        TraceSphere(trace, node->back);
        return;
    }

    bsp_sphere_trace_t trace1 = *trace;
    bsp_sphere_trace_t trace2 = *trace;
    if(node->front)
        TraceSphere(&trace1, node->front);
    if(node->back)
        TraceSphere(&trace2, node->back);

    if(trace1.f < trace2.f)
        *trace = trace1;
    else
        *trace = trace2;

}

void CBSPTree::ClearMarks(CBSPNode* node)
{
    if(node->IsLeaf())
    {
        node->marker = 0;
        for(int i=0;i<(int)node->polylist.size();i++)
            node->polylist[i].colormarker = 0;
        return;
    }
    if(node->front)
        ClearMarks(node->front);
    if(node->back)
        ClearMarks(node->back);
}

void CBSPTree::MarkLeaf(const vec3_t& pos, float radius, CBSPNode* node)
{
    if(node->IsLeaf())
    {
        for(int i=0;i<(int)node->polylist.size();i++)
        {
            if(fabsf(node->polylist[i].plane.GetDistFromPlane(pos)) < radius)
                node->polylist[i].colormarker++;
        }
        return;
    }

    if(node->front && node->plane.GetDistFromPlane(pos) > -radius)
        MarkLeaf(pos, radius, node->front);
    if(node->back && node->plane.GetDistFromPlane(pos) < radius)
        MarkLeaf(pos, radius, node->back);
}

CBSPTree::CBSPNode* CBSPTree::GetLeaf(const vec3_t& pos)
{
    assert(m_root);
    if(m_root)
        return GetLeaf(pos, m_root);
    else
        return NULL;
}

CBSPTree::CBSPNode* CBSPTree::GetLeaf(const vec3_t& pos, CBSPTree::CBSPNode* node)
{
    if(node->IsLeaf())
        return node;

    if(node->plane.Classify(pos) == POINTPLANE_BACK)
        return GetLeaf(pos, node->back);
    else
        return GetLeaf(pos, node->front);
}

/*
        Polygon/Plane relation:
        Split Polygon
        Back
        Front
        Coplanar    
*/
polyplane_t CBSPTree::TestPolygon(bsp_poly_t& poly, plane_t& plane)
{
    int size = poly.Size();
    int allfront = 2;
    int allback = 1;

    for(int i=0;i<size;i++)
    {
        switch(plane.Classify(m_vertices[poly.vertices[i]], BSP_EPSILON))
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

int CBSPTree::SearchSplitPolygon(std::vector<bsp_poly_t>& polygons)
{
    int i, j;
    int polycount = (int)polygons.size();
    int maxtest;
    int tests=0;
    plane_t curplane;
    vec3_t polynormal;
    int bestscore = INT_MAX;
    int bestpoly = -1;
    int score;
    int front, back, split, coplanar;
#ifdef _DEBUG
    int splitpolycount = 0;
#endif

    assert(polycount >= 1);
    if(polycount < 1)
        return -1;
    if(polycount == 1)
        return 0;
    if(polycount > 30)
        maxtest = 30;
    else
        maxtest = polycount;

    for(i=0;i<polycount;i++)
    {
        if(polygons[i].splitmarker) // do not use as split plane again
        {
#ifdef _DEBUG
            splitpolycount++;
#endif
            continue;
        }
        tests++;
        if(tests >= maxtest && bestpoly != -1)
            break;

        front = back = split = coplanar = 0;
    
        curplane.SetupPlane(m_vertices[polygons[i].vertices[2]],
                         m_vertices[polygons[i].vertices[1]],
                         m_vertices[polygons[i].vertices[0]]);
#ifdef _DEBUG
        bsp_poly_t curpoly = polygons[i];
        vec3_t curplane_m_n = curplane.m_n;
        vec3_t m_norm = m_normals[polygons[i].normals[0]];
        //assert(curplane_m_n.Equals(m_norm, 0.1f));
#endif

        for(j=0;j<polycount;j++)
        {
            if(j==i)
                continue;
            switch(TestPolygon(polygons[j], curplane))
            {
            case POLYPLANE_SPLIT:
                split++;
                break;
            case POLYPLANE_FRONT:
                front++;
                break;
            case POLYPLANE_BACK:
                back++;
                break;
            case POLYPLANE_COPLANAR:
                coplanar++;
                break;
#ifdef _DEBUG
            default:
                assert(0);
#endif
            }
        }

        score = 2*split + abs(front-back) + coplanar;
        if(score < bestscore)
        {
            bestpoly = i;
            bestscore = score;
        }
    }

    //assert(splitpolycount != maxtest);
    assert(bestpoly != -1);
    return bestpoly;
}

void CBSPTree::SplitPolygon(bsp_poly_t& polyin, plane_t& plane, 
                        bsp_poly_t& polyfront, bsp_poly_t& polyback)
{
    int polycount = (int)polyin.Size();
    int from, to; // polygon index
    pointplane_t fromloc, toloc;
    float f; // for plane intersection
    vec3_t fromvec, tovec, splitvec;
    vec3_t texfrom, texto; // calculating new tex coords
    int newindex; // for new vertices/texcoords

    assert(polycount > 2);
    //assert(polyin.IsPlanar(this));
#ifdef _DEBUG
    bool planar = polyin.IsPlanar(this);
    assert(planar);
#endif
    polyfront.texture = polyin.texture;
    polyback.texture = polyin.texture;

    from = polycount-1;
    fromloc = plane.Classify(m_vertices[polyin.vertices[from]], BSP_EPSILON);
    
    for(to=0;to<polycount;from=to++)
    {
        switch(fromloc)
        {
        case POINTPLANE_FRONT:
            polyfront.vertices.push_back(polyin.vertices[from]);
            polyfront.normals.push_back(polyin.normals[from]);
            polyfront.texcoords.push_back(polyin.texcoords[from]);
            break;
        case POINT_ON_PLANE:
            polyfront.vertices.push_back(polyin.vertices[from]);
            polyfront.normals.push_back(polyin.normals[from]);
            polyfront.texcoords.push_back(polyin.texcoords[from]);
        case POINTPLANE_BACK:
            polyback.vertices.push_back(polyin.vertices[from]);
            polyback.normals.push_back(polyin.normals[from]);
            polyback.texcoords.push_back(polyin.texcoords[from]);
            break;
        }
        tovec = m_vertices[polyin.vertices[to]];
        toloc = plane.Classify(tovec, BSP_EPSILON);
        if(toloc == POINTPLANE_BACK && fromloc == POINTPLANE_FRONT ||
            toloc == POINTPLANE_FRONT && fromloc == POINTPLANE_BACK)
        {
            fromvec = m_vertices[polyin.vertices[from]];
            tovec = tovec-fromvec;

            if(!plane.GetIntersection(&f, fromvec, tovec))
            {
                assert(0); // let me see this
                polyfront.Clear();
                polyback.Clear();
                return;
            }
            assert(fabsf(f) >= 0.0f && fabsf(f) <= (1.0f+lynxmath::EPSILON));

            // Generating new vertex at split point
            fromvec += tovec * f;
            m_vertices.push_back(fromvec);
            newindex = (int)(m_vertices.size()-1);
            polyfront.vertices.push_back(newindex);
            polyback.vertices.push_back(newindex);

            // Generating new texture coordinates for split point
            texfrom = m_texcoords[polyin.texcoords[from]];
            texto = m_texcoords[polyin.texcoords[to]];
            texfrom = texfrom + (texto-texfrom) * f;
            m_texcoords.push_back(texfrom);
            newindex = (int)(m_texcoords.size()-1);
            polyfront.texcoords.push_back(newindex);
            polyback.texcoords.push_back(newindex);

            // FIXME are the normals OK?
            polyfront.normals.push_back(polyin.normals[from]);
            polyback.normals.push_back(polyin.normals[to]);
        }

        fromloc = toloc;
    }

    if(polyback.Size() < 3)
        polyback.Clear();
    if(polyfront.Size() < 3)
        polyfront.Clear();
}

bool CBSPTree::IsConvexSet(std::vector<bsp_poly_t>& polygons)
{
    int size = (int)polygons.size();
    int i, j, k, ksize;
    plane_t polyplane;

    for(i=0;i<size;i++)
    {
        polyplane.SetupPlane(m_vertices[polygons[i].vertices[2]],
                             m_vertices[polygons[i].vertices[1]],
                             m_vertices[polygons[i].vertices[0]]);

        for(j=0;j<size;j++)
        {
            if(j==i)
                continue;

            ksize = polygons[j].Size();
            for(k=0;k<ksize;k++)
            {
                if(polyplane.Classify(m_vertices[polygons[j].vertices[k]], 
                    BSP_EPSILON)
                    == POINTPLANE_BACK)
                    return false;
            }
        }
    }
    return true;
}

// WriteToBinary Helper Functions

int bspbin_addtexture(std::vector<bspbin_texture_t>& textures, const char* texpath)
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

int bspbin_pushleaf(const CBSPTree& tree,
                    const CBSPTree::CBSPNode* leaf, 
                        std::vector<bspbin_texture_t>& textures,
                        std::vector<bspbin_leaf_t>& leafs,
                        std::vector<bspbin_poly_t>& poly,
                        std::vector<bspbin_vertex_t>& vertices,
                        std::vector<bspbin_vertexindex_t>& vertexindices)
{
    std::vector<bsp_poly_t>::const_iterator iter;
    std::vector<int>::const_iterator intiter;

    const int firstpoly = poly.size();
    int polycount = 0;

    for(iter = leaf->polylist.begin(); iter != leaf->polylist.end(); iter++)
    {
        const int firstvertex = vertexindices.size();
        unsigned int index;

        assert(iter->vertices.size() == iter->normals.size());
        assert(iter->normals.size() == iter->texcoords.size());

        // vertices von polygon hinzufügen
        for(index = 0; index < iter->vertices.size(); index++)
        {
            bspbin_vertexindex_t vindex;
            memset(&vindex, 0, sizeof(vindex));
            vindex.vertex = vertices.size();
            vertexindices.push_back(vindex);

            bspbin_vertex_t vertex;
            memset(&vertex, 0, sizeof(vertex));
            vertex.v = tree.m_vertices[iter->vertices[index]];
            vertex.n = tree.m_normals[iter->normals[index]];
            vertex.tu = tree.m_texcoords[iter->texcoords[index]].x;
            vertex.tv = tree.m_texcoords[iter->texcoords[index]].y;
            vertices.push_back(vertex);
        }

        // Textur hinzufügen
        int texturenum = bspbin_addtexture(textures, iter->texturepath.c_str());

        bspbin_poly_t thispoly;
        memset(&thispoly, 0, sizeof(thispoly));
        thispoly.tex = texturenum;
        thispoly.firstvertex = firstvertex;
        thispoly.vertexcount = index; // index == vertexcount
        poly.push_back(thispoly);
        polycount++;
    }

    bspbin_leaf_t thisleaf;
    memset(&thisleaf, 0, sizeof(thisleaf));
    thisleaf.firstpoly = firstpoly;
    thisleaf.polycount = polycount;
    leafs.push_back(thisleaf);

    return -(int)leafs.size(); // um 1 erhöht
}

int bspbin_getnodes(const CBSPTree& tree,
                    const CBSPTree::CBSPNode* node, 
                        std::vector<bspbin_plane_t>& planes,
                        std::vector<bspbin_texture_t>& textures,
                        std::vector<bspbin_node_t>& nodes,
                        std::vector<bspbin_leaf_t>& leafs,
                        std::vector<bspbin_poly_t>& poly,
                        std::vector<bspbin_vertex_t>& vertices,
                        std::vector<bspbin_vertexindex_t>& vertexindices)
{
    if(node->IsLeaf())
    {
        return bspbin_pushleaf(tree, node, textures, leafs, poly, vertices, vertexindices);
    }

    bspbin_plane_t bspplane;
    bspplane.p = node->plane;
    planes.push_back(bspplane);

    bspbin_node_t thisnode;
    const int curpos = nodes.size();
    thisnode.plane = planes.size()-1;
    thisnode.radius = node->sphere;
    thisnode.sphere_origin = node->sphere_origin;
    nodes.push_back(thisnode);

    assert(node->front && node->back);
    nodes[curpos].children[0] = bspbin_getnodes(tree,
                                                node->front, 
                                                planes, 
                                                textures,
                                                nodes,
                                                leafs,
                                                poly,
                                                vertices,
                                                vertexindices);
    nodes[curpos].children[1] = bspbin_getnodes(tree,
                                                node->back, 
                                                planes, 
                                                textures,
                                                nodes,
                                                leafs,
                                                poly,
                                                vertices,
                                                vertexindices);
    return curpos;
}

template<typename Lump>
void WriteLump(FILE*f, const std::vector<Lump>& l)
{
    std::vector<Lump>::const_iterator iter;
    for(iter = l.begin();iter != l.end();iter++)
    {
        Lump tl = (*iter);
        fwrite(&tl, sizeof(tl), 1, f);
    }
}

bool CBSPTree::WriteToBinary(const std::string filepath)
{
    FILE* f = fopen(filepath.c_str(), "wb");
    assert(f);
    if(!f)
        return false;

    std::vector<bspbin_plane_t> planes;
    std::vector<bspbin_texture_t> textures;
    std::vector<bspbin_node_t> nodes;
    std::vector<bspbin_leaf_t> leafs;
    std::vector<bspbin_poly_t> poly;
    std::vector<bspbin_vertex_t> vertices;
    std::vector<bspbin_vertexindex_t> vertexindices;
    std::vector<bspbin_spawn_t> spawnpoints;

    // Daten aus Baum holen
    bspbin_getnodes(*this,
                    m_root, 
                    planes, 
                    textures,
                    nodes,
                    leafs,
                    poly,
                    vertices,
                    vertexindices);
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
        fprintf(stderr, "BSP: Leaf count < 1\n");
        return false;
    }

    if(nodes.size() == 0)
    {
        bspbin_node_t nullnode;
        nullnode.children[0] = -1;
        nullnode.children[1] = -1;
        nullnode.radius = MAX_TRACE_DIST;
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
    bspbin_direntry_t dirvertexindices;
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
    dirpoly.length = poly.size() * sizeof(bspbin_poly_t);
    dirvertices.offset = dirpoly.length + dirpoly.offset;
    dirvertices.length = vertices.size() * sizeof(bspbin_vertex_t);
    dirvertexindices.offset = dirvertices.length + dirvertices.offset;
    dirvertexindices.length = vertexindices.size() * sizeof(bspbin_vertexindex_t);
    dirspawnpoints.offset = dirvertexindices.length + dirvertexindices.offset;
    dirspawnpoints.length = spawnpoints.size() * sizeof(bspbin_spawn_t);
    
    // Header
    fwrite(&header, sizeof(header), 1, f);
    // Lump Table
    fwrite(&dirplane, sizeof(dirplane), 1, f);
    fwrite(&dirtextures, sizeof(dirtextures), 1, f);
    fwrite(&dirnodes, sizeof(dirnodes), 1, f);
    fwrite(&dirleafs, sizeof(dirleafs), 1, f);
    fwrite(&dirpoly, sizeof(dirpoly), 1, f);
    fwrite(&dirvertices, sizeof(dirvertices), 1, f);
    fwrite(&dirvertexindices, sizeof(dirvertexindices), 1, f);
    fwrite(&dirspawnpoints, sizeof(dirspawnpoints), 1, f);

    // Lumps
    WriteLump(f, planes);
    WriteLump(f, textures);
    WriteLump(f, nodes);
    WriteLump(f, leafs);
    WriteLump(f, poly);
    WriteLump(f, vertices);
    WriteLump(f, vertexindices);
    WriteLump(f, spawnpoints);

    fclose(f);

    return true;
}

CBSPTree::CBSPNode::~CBSPNode()
{
    if(front)
        delete front;
    if(back)
        delete back;
}

CBSPTree::CBSPNode::CBSPNode(CBSPTree* tree, 
                 std::vector<bsp_poly_t>& polygons)
{
    std::vector<bsp_poly_t> frontlist;
    std::vector<bsp_poly_t> backlist;
    bsp_poly_t polyfront, polyback;
    int count = (int)polygons.size();
    int split;
    int i;
    assert(tree->m_nodecount < (int)tree->m_polylist.size()*5);

    tree->m_nodecount++;
    CalculateSphere(tree, polygons);

    if(polygons.size() < MAX_POLY_PER_LEAF)
    {
        fprintf(stderr, "BSP: subspace with %i polygons formed\n", polygons.size());
        polylist = polygons;
        for(i=0;i<(int)polylist.size();i++)
            polylist[i].GeneratePlanes(tree);
        front = NULL;
        back = NULL;
        tree->m_leafcount++;
        marker = tree->m_leafcount;
        return;
    }

    if(tree->IsConvexSet(polygons))
    {
        fprintf(stderr, "BSP: Convex subspace with %i polygons formed\n", polygons.size());
        polylist = polygons;
        for(i=0;i<(int)polylist.size();i++)
            polylist[i].GeneratePlanes(tree);
        front = NULL;
        back = NULL;
        tree->m_leafcount++;
        marker = tree->m_leafcount;
        return;
    }
    marker = 0;

    assert(polygons.size()>0);
    split = tree->SearchSplitPolygon(polygons);
    if(split < 0)
    {
        assert(0);
        fprintf(stderr, "BSP: Weird problem\n");
        // Im Endeffekt ist das hier dann auf ein neues Leaf. Sollte so nie vorkommen
        polylist = polygons;
        for(i=0;i<(int)polylist.size();i++)
            polylist[i].GeneratePlanes(tree);
        front = NULL;
        back = NULL;
        return;
    }
    plane.SetupPlane(tree->m_vertices[polygons[split].vertices[2]],
                     tree->m_vertices[polygons[split].vertices[1]],
                     tree->m_vertices[polygons[split].vertices[0]]);
    polygons[split].splitmarker = true;
    frontlist.push_back(polygons[split]);

    for(i=0;i<count;i++)
    {
        if(i == split)
            continue;
        switch(tree->TestPolygon(polygons[i], plane))
        {
        case POLYPLANE_COPLANAR:
            if(polygons[i].GetNormal(tree) * plane.m_n > 0.0f)
                frontlist.push_back(polygons[i]);
            else
                backlist.push_back(polygons[i]);
            break;
        case POLYPLANE_FRONT:
            frontlist.push_back(polygons[i]);
            break;
        case POLYPLANE_BACK:
            backlist.push_back(polygons[i]);
            break;
        case POLYPLANE_SPLIT:
            tree->SplitPolygon(polygons[i], plane, polyfront, polyback);

            // Wir akzeptieren nur Dreiecke als Eingangspolygone und wenn jetzt nach einem
            // Split ein Quad entsteht teilen wir das wieder in 
            if(polyfront.Size() > 3)
            {
                bsp_poly_t poly1 = polyfront;
                bsp_poly_t poly2 = polyfront;
                poly1.vertices.pop_back();
                poly1.texcoords.pop_back();
                poly1.normals.pop_back();
                poly2.vertices.erase(poly2.vertices.begin() + 1);
                poly2.texcoords.erase(poly2.texcoords.begin() + 1);
                poly2.normals.erase(poly2.normals.begin() + 1);
                if(poly1.IsPlanar(tree))
                    frontlist.push_back(poly1);
                if(poly2.IsPlanar(tree))
                    frontlist.push_back(poly2);
            }
            else if(polyfront.Size() > 2 && polyfront.IsPlanar(tree))
            {
                frontlist.push_back(polyfront);
            }
            if(polyback.Size() > 3) // FIXME copy and paste code
            {
                bsp_poly_t poly1 = polyback;
                bsp_poly_t poly2 = polyback;
                poly1.vertices.pop_back();
                poly1.texcoords.pop_back();
                poly1.normals.pop_back();
                poly2.vertices.erase(poly2.vertices.begin() + 1);
                poly2.texcoords.erase(poly2.texcoords.begin() + 1);
                poly2.normals.erase(poly2.normals.begin() + 1);
                if(poly1.IsPlanar(tree))
                    backlist.push_back(poly1);
                if(poly2.IsPlanar(tree))
                    backlist.push_back(poly2);
            }
            else if(polyback.Size() > 2 && polyback.IsPlanar(tree))
            {
                backlist.push_back(polyback);
            }

            polyfront.Clear();
            polyback.Clear();
            break;
        }
    }

    if(frontlist.size() > 0)
    {
        front = new CBSPNode(tree, frontlist);
        if(!front)
            tree->m_outofmem = true;
    }
    else
    {
        front = NULL;
    }
    if(backlist.size() > 0)
    {
        back = new CBSPNode(tree, backlist);
        if(!back)
            tree->m_outofmem = true;
    }
    else
    {
        back = NULL;
    }
}

void CBSPTree::CBSPNode::CalculateSphere(CBSPTree* tree, std::vector<bsp_poly_t>& polygons)
{
    vec3_t v, min(0,0,0), max(0,0,0);
    int i, j, jsize, isize = (int)polygons.size();

    for(i=0;i<isize;i++)
    {
        jsize = polygons[i].Size();
        for(j=0;j<jsize;j++)
        {
            v = tree->m_vertices[polygons[i].vertices[j]];
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

bool bsp_poly_t::IsPlanar(CBSPTree* tree)
{
    plane_t testplane;
    int j;
    testplane.SetupPlane(tree->m_vertices[vertices[2]],
                         tree->m_vertices[vertices[1]],
                         tree->m_vertices[vertices[0]]);
    if(!testplane.m_n.IsNormalized())
    {
//#ifdef _DEBUG
//        vec3_t v1 = tree->m_vertices[vertices[2]];
//        vec3_t v2 = tree->m_vertices[vertices[1]];
//      vec3_t v3 = tree->m_vertices[vertices[0]];
//        assert(0);
//#endif
        return false;
    }
    int polysize = Size();
    assert(polysize > 2);
    if(polysize > 3)
    {
        for(j=3;j<polysize;j++)
            if(testplane.Classify(tree->m_vertices[vertices[j]], 
                BSP_EPSILON) != 
                POINT_ON_PLANE)
            {
#ifdef _DEBUG
                vec3_t v = tree->m_vertices[vertices[j]];
                fprintf(stderr, "BSP: Nonplanar polygon. Dist: %f\n",
                    testplane.GetDistFromPlane(v));
                testplane.Classify(v, BSP_EPSILON);
                assert(0);
#endif
                return false;
            }
    }
    return true;
}

bool bsp_poly_t::GetIntersectionPoint(const vec3_t& start, const vec3_t& dir, float* f, const CBSPTree* tree, const float offset) const
{
    vec3_t tmpintersect;
    float cf;

    plane_t ptmp = plane;
    ptmp.m_d -= offset; // Plane shift
    const bool hit = ptmp.GetIntersection(&cf, start, dir);
    if(!hit || cf > 1.0f || cf < -BSP_EPSILON)
        return false;
    tmpintersect = start + dir*cf - plane.m_n*offset;
    *f = cf;

    // Berechnung über Barycentric coordinates (math for 3d game programming p. 144)
    assert(vertices.size() == 3);
    const vec3_t P0 = tree->m_vertices[vertices[0]];
    const vec3_t P1 = tree->m_vertices[vertices[1]];
    const vec3_t P2 = tree->m_vertices[vertices[2]];
    const vec3_t R = tmpintersect - P0;
    const vec3_t Q1 = P1 - P0;
    const vec3_t Q2 = P2 - P0;
    const float Q1Q2 = Q1*Q2;
    const float Q1_sqr = Q1.AbsSquared();
    const float Q2_sqr = Q2.AbsSquared();
    assert(fabsf(Q1_sqr*Q2_sqr - Q1Q2*Q1Q2) > lynxmath::EPSILON);
    const float invdet = 1/(Q1_sqr*Q2_sqr - Q1Q2*Q1Q2);
    const float RQ1 = R * Q1;
    const float RQ2 = R * Q2;
    
    const float w1 = invdet*(Q2_sqr*RQ1 - Q1Q2*RQ2);
    const float w2 = invdet*(-Q1Q2*RQ1  + Q1_sqr*RQ2);

    return w1 >= 0 && w2 >= 0 && (w1 + w2 <= 1);
}

bool bsp_poly_t::GetEdgeIntersection(const vec3_t& start, const vec3_t& dir,
                                     float* f, const float radius, vec3_t* normal,
                                     vec3_t* hitpoint, const CBSPTree* tree) const
{
    const int size = (int)vertices.size();
    float minf = MAX_TRACE_DIST;
    float cf;
    vec3_t a, b;
    int minindex = -1;
    for(int i=0;i<size;i++)
    {
        a = tree->m_vertices[vertices[i]];
        b = tree->m_vertices[vertices[(i+1)%size]];
        if(!vec3_t::RayCylinderIntersect(start, dir, a, b, radius, &cf))
            continue;
        if(cf < minf && cf >= -BSP_EPSILON)
        {
            minf = cf;
            minindex = i;
        }
    }
    if(minf <= 1.0f)
    {
        *hitpoint = start + minf*dir;
        a = tree->m_vertices[vertices[minindex]];
        b = tree->m_vertices[vertices[(minindex+1)%size]];
        *normal = ((a-*hitpoint)^(b-*hitpoint)) ^ (b-a);
        normal->Normalize();
        *f = minf;
        return true;
    }
    else
    {
        return false;
    }
}

bool bsp_poly_t::GetVertexIntersection(const vec3_t& start, const vec3_t& dir,
                                       float* f, const float radius, vec3_t* normal, 
                                       vec3_t* hitpoint, const CBSPTree* tree) const
{
    const int vertexcount = vertices.size();
    float minf = MAX_TRACE_DIST;
    float cf;
    int minindex = -1;
    for(int i=0;i<vertexcount;i++)
    {
        if(!vec3_t::RaySphereIntersect(start, dir,
                                       tree->m_vertices[vertices[i]], radius, &cf))
            continue;
        if(cf < minf && cf >= -BSP_EPSILON)
        {
            minindex = i;
            minf = cf;
        }
    }
    if(minf <= 1.0f)
    {
        *hitpoint = start + minf*dir;
        *normal = *hitpoint - tree->m_vertices[vertices[minindex]];
        normal->Normalize();
        *f = minf;
        return true;
    }
    else
        return false;
}

vec3_t bsp_poly_t::GetNormal(CBSPTree* tree) // not unit length
{
    assert(plane.m_n.IsNull()); // we've got the plane - use plane.m_n instead
    assert(tree->m_normals[normals[0]] == 
            tree->m_normals[normals[1]]);
    assert(tree->m_normals[normals[1]] == 
            tree->m_normals[normals[2]]);
    vec3_t np = tree->m_normals[normals[0]];
    return np;
}

void CBSPTree::GetLeftRightScore(int* left, int* right) const
{
    *left = 0;
    *right = 0;
    if(m_root)
        GetLeftRightScore(left, right, m_root);
}

void CBSPTree::GetLeftRightScore(int* left, int* right, CBSPNode* node) const
{
    if(node->front)
    {
        (*left)++;
        GetLeftRightScore(left, right, node->front);
    }
    if(node->back)
    {
        (*right)++;
        GetLeftRightScore(left, right, node->back);
    }
}
void bsp_poly_t::GeneratePlanes(CBSPTree* tree)
{
    int size = (int)vertices.size();
    vec3_t a, b, n;
    plane_t p;

    plane.SetupPlane(tree->m_vertices[vertices[2]],
                     tree->m_vertices[vertices[1]],
                     tree->m_vertices[vertices[0]]);

}
