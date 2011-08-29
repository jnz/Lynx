#pragma once

#include "lynx.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include <vector>

#define BSPBIN_MAGIC                    0x12051982
#define BSPBIN_VERSION                  6
#define BSPBIN_HEADER_LEN               (sizeof(bspbin_header_t) + 7*sizeof(bspbin_direntry_t))

//#pragma pack(push, 1) // manual padding

struct bspbin_header_t
{
    uint32_t magic;
    uint32_t version;
};

struct bspbin_direntry_t
{
    uint32_t offset;
    uint32_t length;
};

struct bspbin_plane_t
{
    //plane_t p;
    uint8_t type; // 0 = x, 1 = y, 2 = z
    float d;
};

struct bspbin_texture_t
{
    char name[64];
};

struct bspbin_node_t
{
    uint32_t plane;
    int32_t children[2]; // don't change this to uint32_t, the negative sign is essential!
    float radius;
    vec3_t sphere_origin;
};

struct bspbin_triangle_t
{
    uint32_t tex;
    uint32_t v[3]; // integer index to bspbin_vertex_t array
};

struct bspbin_vertex_t
{
    vec3_t v;
    vec3_t n;
    float tu, tv;
    vec3_t t; // tangent
    float w; // value to compute bitangent vector: Bitangent = w * (Normal x Tangent)
};

struct bspbin_spawn_t
{
    bspbin_spawn_t() : point(0,0,0), rot(0,0,0,1.0f) { }

    vec3_t point;
    quaternion_t rot;
};

//#pragma pack(pop)

struct bspbin_leaf_t
{
    // Dynamic array:
    std::vector<uint32_t> triangles; // if you change this type, change the read/write functions too
};
// new in version 5: there is no fixed array of triangle indices
// triangle is a placeholder variable, and the triangles will
// be written after the trianglecount variable

