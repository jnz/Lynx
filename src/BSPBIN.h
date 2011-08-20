#pragma once

#include "lynx.h"
#include "math/plane.h"
#include "math/quaternion.h"

// #pragma pack(push, 1) // manual padding

#define BSPBIN_MAGIC                    0x12051982
#define BSPBIN_VERSION                  4
#define BSPBIN_MAX_TRIANGLES_PER_LEAF   64
#define BSPBIN_HEADER_LEN               (sizeof(bspbin_header_t) + 8*sizeof(bspbin_direntry_t))

struct bspbin_header_t
{
    int32_t magic;
    int32_t version;
};

struct bspbin_direntry_t
{
    int32_t offset;
    int32_t length;
};

struct bspbin_plane_t
{
    plane_t p;
};

struct bspbin_texture_t
{
    char name[64];
};

struct bspbin_node_t
{
    int32_t plane;
    int32_t children[2];
    float radius;
    vec3_t sphere_origin;
};

struct bspbin_leaf_t
{
    int32_t triangles[BSPBIN_MAX_TRIANGLES_PER_LEAF];
    int32_t trianglecount;
};

struct bspbin_triangle_t
{
    int32_t tex;
    int32_t v[3];
};

struct bspbin_vertex_t
{
    vec3_t v;
    vec3_t n;
    float tu, tv;
};

struct bspbin_spawn_t
{
    bspbin_spawn_t() : point(0,0,0), rot(0,0,0,1.0f) { }

    vec3_t point;
    quaternion_t rot;
};

// #pragma pack(pop)
