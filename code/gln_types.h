#ifndef __GLN_TYPES__
#define __GLN_TYPES__

#pragma once

typedef float vec_t;
typedef int32_t ivec_t;
typedef uint32_t uvec_t;

typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

typedef ivec_t ivec2_t[2];
typedef ivec_t ivec3_t[3];
typedef ivec_t ivec4_t[4];

typedef uvec_t uvec2_t[2];
typedef uvec_t uvec3_t[3];
typedef uvec_t uvec4_t[4];

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef unsigned char byte;
#ifdef __cplusplus
#define qtrue 1
#define qfalse 0
typedef unsigned int qboolean;
#else
typedef enum { qfalse = 0, qtrue = 1 } qboolean;
#endif
#endif


typedef struct texture_s {
    struct texture_s *next;
    char name[MAX_NPATH];
    int width, height;
} texture_t;

typedef struct {
    vec4_t color;
    vec3_t xyz;
    vec3_t worldPos;
    vec2_t uv;
} drawVert_t;

typedef struct {
    char name[MAX_NPATH];
} entity_t;

typedef float vec_t;

typedef struct {
    float texcoords[4][2];
    byte sides[5]; // for physics
    vec4_t color;
    uvec3_t pos;
    int32_t index; // tileset texture index, -1 if not bound
    uint32_t flags;

    int undoId;
    int redoId;
} tile_t;

typedef struct {
    uvec3_t xyz;
    uint32_t entitytype;
    uint32_t entityid;

    int undoId;
    int redoId;
} spawn_t;

#endif