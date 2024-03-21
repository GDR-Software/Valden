#ifndef __GLN_TYPES__
#define __GLN_TYPES__

#pragma once


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