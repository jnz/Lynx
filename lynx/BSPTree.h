#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include "math/plane.h"
#include <vector>

class CBSPTree;

struct bsp_trace_t
{
	vec3_t	start; // start point
	vec3_t	end; // end point
	bool	allsolid; // if true, plane is not valid
	bool	startsolid; // start point in solid area
	float	f; // 1.0f == no hit
	plane_t	p; // impact plane
	vec3_t	endpoint; // endpoint = start + (end-start)*f
	float	offset;	// AABB offset from plane

	// private
	vec3_t	trace_extend;
	vec3_t	min;
	vec3_t	max;
};

struct bsp_poly_t
{
	bsp_poly_t() { splitmarker = false; colormarker = 0; }
	std::vector<int> vertices;
	std::vector<int> normals;
	std::vector<int> texcoords;
	plane_t	plane;
	std::vector<plane_t> planes; // Perpendicular Planes
	bool splitmarker; // true if this poly was used to split the space
	int colormarker;

	int Size() // vertex count
	{
		assert(vertices.size() == normals.size());
		return (int)vertices.size();
	}
	void Clear()
	{
		vertices.clear();
		normals.clear();
		texcoords.clear();
	}
	bool GetIntersectionPoint(const vec3_t& p, const vec3_t& v, float* f);
	vec3_t GetNormal(CBSPTree* tree); // not unit length
	bool IsPlanar(CBSPTree* tree);
	void GeneratePlanes(CBSPTree* tree); // Called when a polygon has reached a leaf - makes collision detection easier
	void ClipBox(bsp_trace_t* trace, float f1, float f2, vec3_t start, vec3_t end);
};

enum polyplane_t {	POLYPLANE_SPLIT = 0,
					POLYPLANE_BACK = 1, 
					POLYPLANE_FRONT = 2, 
					POLYPLANE_COPLANAR = 3
					}; // if you change the order, change TestPolygon too

class CBSPTree
{
public:
	CBSPTree(void);
	~CBSPTree(void);

	bool		Load(std::string file);
	void		Unload();
	std::string GetFilename();
	void		GetLeftRightScore(int* left, int* right);

	class CBSPNode
	{
	public:
		~CBSPNode();
		CBSPNode(CBSPTree* tree, 
				 std::vector<bsp_poly_t>& polygons);

		union
		{
			struct
			{
				CBSPNode* front;
				CBSPNode* back;
			};
			CBSPNode* child[2];
		};

		void					CalculateSphere(CBSPTree* tree, std::vector<bsp_poly_t>& polygons);
		bool					IsLeaf() { return !front && !back; }

		plane_t					plane;
		float					sphere; // sphere radius
		vec3_t					sphere_origin; // sphere origin
		std::vector<bsp_poly_t> polylist; // FIXME now every node has an empty polylist
		int						marker;
	};

	std::vector<vec3_t>		m_vertices;
	std::vector<vec3_t>		m_normals;
	std::vector<vec3_t>		m_texcoords; // FIXME use vec2_t
	std::vector<bsp_poly_t> m_polylist;
	CBSPNode*				m_root;

	void		TraceRay(const vec3_t& start, const vec3_t& dir, float* f, CBSPNode* node);
	void		TraceBBox(const vec3_t& start, const vec3_t& end, 
							const vec3_t& min, const vec3_t& max,
							bsp_trace_t* trace);
	void		ClearMarks(CBSPNode* node);
	void		MarkLeaf(const vec3_t& pos, float radius, CBSPNode* node);
	CBSPNode*	GetLeaf(const vec3_t& pos);
	// Temp. Stat Variable (FIXME)
	int			GetLeafCount() { return m_leafcount; }
	int			m_visited;

private:
	int			m_nodecount; // increased by every CBSPNode constructor
	int			m_leafcount;
	bool		m_outofmem; // set to true by CBSPNode constructor, if no memory for further child nodes is available
	std::string m_filename;

	// Helper functions for the CBSPNode constructor to create the BSP tree
	polyplane_t TestPolygon(bsp_poly_t& poly, plane_t& plane);
	int			SearchSplitPolygon(std::vector<bsp_poly_t>& polygons);
	void		SplitPolygon(bsp_poly_t& polyin, plane_t& plane, 
							 bsp_poly_t& polyfront, bsp_poly_t& polyback);
	bool		IsConvexSet(std::vector<bsp_poly_t>& polygons);

	void		GetLeftRightScore(int* left, int* right, CBSPNode* node);
	CBSPNode*	GetLeaf(const vec3_t& pos, CBSPNode* node);
	void		RecursiveTraceBBox(bsp_trace_t* trace, CBSPNode* node, 
									float f1, float f2,
									vec3_t start, vec3_t end);
	void		TraceToLeaf(bsp_trace_t* trace, CBSPNode* node, 
									float f1, float f2,
									vec3_t start, vec3_t end);
};
