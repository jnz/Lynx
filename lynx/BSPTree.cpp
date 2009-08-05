#include "BSPTree.h"
#include <stdio.h>
#include <math.h>
#include "math/mathconst.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define	DIST_EPSILON			(0.003125f)
#define BSP_EPSILON				(0.003125f)

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
bool CBSPTree::Load(std::string file) // Ugly loader code :-(
{
	const char* DELIM = " \t\r\n";
	FILE* f;
	char line[1024];
	char* tok;
	char* sv[3];
	int i;
	int vi, ti, ni;
	bsp_poly_t polygon;
	int nonplanar = 0; // anzahl der nicht-ebenen polygone zählen

	f = fopen(file.c_str(), "rb");
	if(!f)
		return false;

	while(!feof(f))
	{
		fgets(line, sizeof(line), f);
		tok = strtok(line, DELIM);

		if(strcmp(tok, "v")==0)	// vertex
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
			if(!sv[0] || !sv[1] || !sv[2])
				continue;
			m_texcoords.push_back(vec3_t(atof(sv[0]), atof(sv[1]), atof(sv[2])));
		}
		else if(strcmp(tok, "f")==0) // face
		{
			for(i=0;i<3;i++)
				sv[i] = strtok(NULL, DELIM);
			if(!sv[0] || !sv[1] || !sv[2])
				continue;
			polygon.Clear();
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
}

std::string CBSPTree::GetFilename()
{
	return m_filename;
}

void CBSPTree::TraceRay(const vec3_t& start, const vec3_t& dir, float* f, CBSPNode* node)
{
	if(node->IsLeaf())
	{
		float cf;
		float minf = 99999.9f;
		int minindex = -1;
		int size = (int)node->polylist.size();
		for(int i=0;i<size;i++)
		{
			if(node->polylist[i].plane.m_n * dir > 0.0f)
				continue;

			if(node->polylist[i].GetIntersectionPoint(start, dir, &cf))
				if(cf < minf && 
					cf > lynxmath::EPSILON)
				{
					minf = cf;
					minindex = i;
				}
		}
		*f = minf;
		if(minindex != -1)
		{
			node->polylist[minindex].colormarker++;
			node->marker++;
		}
		return;
	}

	pointplane_t locstart = node->plane.Classify(start, BSP_EPSILON);
	pointplane_t locend = node->plane.Classify(start+dir, BSP_EPSILON);

	if(locstart >= POINT_ON_PLANE && locend >= POINT_ON_PLANE)
	{
		TraceRay(start, dir, f, node->front);
		return;
	}
	if(locstart < POINT_ON_PLANE && locend < POINT_ON_PLANE)
	{
		TraceRay(start, dir, f, node->back);
		return;
	}

	float f1=99999.9f, f2=99999.9f;
	if(node->front)
		TraceRay(start, dir, &f1, node->front);
	if(node->back)
		TraceRay(start, dir, &f2, node->back);

	if(f1 < f2)
		*f = f1;
	else
		*f = f2;
}

void CBSPTree::TraceBBox(const vec3_t& start, const vec3_t& end, 
							const vec3_t& min, const vec3_t& max,
							bsp_trace_t* trace)
{
	trace->start = start;
	trace->end = end;
	trace->startsolid = true;
	trace->allsolid = true;
	trace->f = 1.0f;
	trace->min = min;
	trace->max = max;

	trace->trace_extend.v[0] = -min.v[0] > max.v[0] ? -min.v[0] : max.v[0];
	trace->trace_extend.v[1] = -min.v[1] > max.v[1] ? -min.v[1] : max.v[1];
	trace->trace_extend.v[2] = -min.v[2] > max.v[2] ? -min.v[2] : max.v[2];

	RecursiveTraceBBox(trace, m_root, 0.0f, 1.0f, trace->start, trace->end);

	trace->endpoint = start + (end-start)*trace->f;
}

void CBSPTree::RecursiveTraceBBox(bsp_trace_t* trace, CBSPNode* node, 
									float f1, float f2,
									vec3_t start, vec3_t end)
{
	assert(f1 >= 0 && f2 >= f1);

	if(trace->f < f1)
		return;

	if(node->IsLeaf())
	{
		TraceToLeaf(trace, node, f1, f2, start, end);
		return;
	}

	float offset;
	float dist1, dist2;

	dist1 = node->plane.GetDistFromPlane(start);
	dist2 = node->plane.GetDistFromPlane(end);

	offset = fabsf(trace->trace_extend.v[0] * node->plane.m_n.v[0]) +
			 fabsf(trace->trace_extend.v[1] * node->plane.m_n.v[1]) +
			 fabsf(trace->trace_extend.v[2] * node->plane.m_n.v[2]);

	if(dist1 >= offset && dist2 >= offset)
	{
		if(node->front)
			RecursiveTraceBBox(trace, node->front, f1, f2, start, end);
		return;
	}
	if(dist1 < -offset && dist2 < -offset)
	{
		if(node->back)
			RecursiveTraceBBox(trace, node->back, f1, f2, start, end);
		return;
	}
//#if 0
	if(node->front)
		RecursiveTraceBBox(trace, node->front, f1, f2, start, end);
	if(node->back)
			RecursiveTraceBBox(trace, node->back, f1, f2, start, end);
	return;
//#endif
	// put the crosspoint DIST_EPSILON pixels on the near side
	float idist;
	int side;
	float frac, frac2;
	float midf;
	vec3_t mid;

	if(dist1 < dist2)
	{
		idist = 1.0f / (dist1 - dist2);
		side = 1;
		frac2 = (dist1 + offset + DIST_EPSILON)*idist;
		frac = (dist1 - offset + DIST_EPSILON)*idist;
	}
	else if(dist1 > dist2)
	{
		idist = 1.0f / (dist1 - dist2);
		side = 0;
		frac2 = (dist1 - offset - DIST_EPSILON)*idist;
		frac = (dist1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if(frac < 0)
		frac = 0;
	if(frac > 1)
		frac = 1;
		
	midf = f1 + (f2 - f1)*frac;
	mid = start + (end - start)*frac;

	if(node->child[side])
		RecursiveTraceBBox(trace, node->child[side], f1, midf, start, mid);

	// go past the node
	if(frac2 < 0)
		frac2 = 0;
	if(frac2 > 1)
		frac2 = 1;
		
	midf = f1 + (f2 - f1)*frac2;
	mid = start + (end - start)*frac2;

	side ^= 1;
	if(node->child[side])
		RecursiveTraceBBox(trace, node->child[side], midf, f2, mid, end);
}

void CBSPTree::TraceToLeaf(bsp_trace_t* trace, CBSPNode* node, 
									float f1, float f2,
									vec3_t start, vec3_t end)
{
	int size = (int)node->polylist.size();
	assert(f1 >= 0.0f && f2 >= f1);

	node->marker++;
	for(int i=0;i<size;i++)
		node->polylist[i].ClipBox(trace, f1, f2, start, end);
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
		ALLFRNT ALLBACK 	RESULT
		0		0			Split Polygon
		0		1			Back  (1)
		1		0			Front (2)
		1		1			Coplanar
	*/
	return (polyplane_t)(allfront | allback);
}

int CBSPTree::SearchSplitPolygon(std::vector<bsp_poly_t>& polygons)
{
	int i, j;
	int polycount = (int)polygons.size();
	int maxtest;
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
	if(polycount > 200)
		maxtest = 200;
	else
		maxtest = polycount;

	for(i=0;i<maxtest;i++)
	{
		if(polygons[i].splitmarker) // do not use as split plane again
		{
#ifdef _DEBUG
			splitpolycount++;
#endif
			continue;
		}

		front = back = split = coplanar = 0;
	
		curplane.SetupPlane(m_vertices[polygons[i].vertices[2]],
						 m_vertices[polygons[i].vertices[1]],
						 m_vertices[polygons[i].vertices[0]]);
		assert(curplane.m_n == m_normals[polygons[i].normals[0]]);

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

	assert(splitpolycount != maxtest);
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
	assert(polyin.IsPlanar(this));

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
		polylist = polygons;
		for(i=0;i<(int)polylist.size();i++)
			polylist[i].GeneratePlanes(tree);
		front = NULL;
		back = NULL;
		return;
	}
	plane.SetupPlane(	tree->m_vertices[polygons[split].vertices[2]],
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
			if(polyfront.Size() > 2)
				frontlist.push_back(polyfront);
			if(polyback.Size() > 2)
				backlist.push_back(polyback);
			polyfront.Clear();
			polyback.Clear();
			break;
		}
	}

	if(frontlist.size() > 0)
	{
		front =	new CBSPNode(tree, frontlist);
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
	if(!testplane.m_n.IsNormalized()) // funny vertices make the plane cry
		return false;
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
		//		assert(0);
#endif
				return false;
			}
	}
	return true;
}

bool bsp_poly_t::GetIntersectionPoint(const vec3_t& p, const vec3_t& v, float* f)
{
	vec3_t tmpintersect;
	const int size = (int)planes.size();

	if(!plane.GetIntersection(f, p, v))
		return false;

	tmpintersect = p + v * (*f);
	for(int i=0;i<size;i++)
	{
		switch(planes[i].Classify(tmpintersect))
		{
		case POINTPLANE_FRONT:
		case POINT_ON_PLANE:
			continue;
		default:
			return false;
		}
	}
	return true;
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

void CBSPTree::GetLeftRightScore(int* left, int* right)
{
	*left = 0;
	*right = 0;
	if(m_root)
		GetLeftRightScore(left, right, m_root);
}

void CBSPTree::GetLeftRightScore(int* left, int* right, CBSPNode* node)
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

	n = plane.m_n;
	for(int i=0;i<size;i++)
	{
		a = tree->m_vertices[vertices[i]];
		b = tree->m_vertices[vertices[(i+1)%size]];
		p.m_n = n ^ (b-a);
		p.m_n.Normalize();
		p.m_d = -a * p.m_n;
		planes.push_back(p);
	}
}

void bsp_poly_t::ClipBox(bsp_trace_t* trace, float f1, float f2, vec3_t start, vec3_t end)
{
	float cf, f, offset;
	vec3_t intersect;
	vec3_t ofs, v = end-start;
	plane_t p=plane;
	int i;

	if(p.m_n * v > 0.0f) // ignore backfaces
		return;
	p.GetIntersection(&cf, start, v);
	if(cf < 0.0f)
		return;
	intersect = start + v * cf;

	for(i=0;i<3;i++)
	{
		if(p.m_n.v[i] < 0)
			ofs.v[i] = trace->max.v[i];
		else
			ofs.v[i] = trace->min.v[i];
	}
	offset = ofs * p.m_n;
	p.m_d += offset;

	p.GetIntersection(&cf, start, v);

	if(cf >= 1.0f)
		return;
	
	trace->allsolid = false;
	if(cf < 0.0f)
		trace->startsolid = false;
	
	for(int j=0;j<(int)planes.size();j++)
	{
		for(i=0;i<3;i++)
		{
			if(planes[j].m_n.v[i] < 0)
				ofs.v[i] = trace->max.v[i];
			else
				ofs.v[i] = trace->min.v[i];
		}
		if(planes[j].GetDistFromPlane(intersect) < (ofs * planes[j].m_n))
			return;
	}

	if(cf < -lynxmath::EPSILON)
	{
		fprintf(stderr, "BSP ClipBox: cf < 0.0f (%.2f)\n", cf);
		return;
	}

	f = f1 + (f2-f1)*cf;
	if(f < trace->f)
	{
		trace->f = f;
		trace->p = plane;
		trace->offset = offset;
	}
}
