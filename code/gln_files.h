#ifndef __GLN_FILES__
#define __GLN_FILES__

#pragma once

/*
* GLN_FILES: these definitions must not change in any glnomad extension or project
*/

#ifndef MAX_NPATH
#define MAX_NPATH 64
#endif

#ifndef MAX_GDR_PATH
#define MAX_GDR_PATH 64
#endif

// the minimum size in bytes a lump should be before compressing it
#define COMPRESSED_LUMP_SIZE (4*1024*1024)

#define COMPRESS_NONE 0
#define COMPRESS_ZLIB 1
#define COMPRESS_BZIP2 2

#define TEXTURE_FILE_EXT ".tex2d"
#define TILESET_FILE_EXT ".tile2d"
#define MAP_FILE_EXT ".map"
#define ANIMATION_FILE_EXT ".anim2d"
#define TEXTURE_FILE_EXT_RAW "tex2d"
#define TILESET_FILE_EXT_RAW "tile2d"
#define MAP_FILE_EXT_RAW "map"
#define ANIMATION_FILE_EXT_RAW "anim2d"
#define LEVEL_FILE_EXT ".bmf"
#define LEVEL_FILE_EXT_RAW "bmf"

typedef enum {
    DIR_NORTH = 0,
    DIR_NORTH_EAST,
    DIR_EAST,
    DIR_SOUTH_EAST,
    DIR_SOUTH,
    DIR_SOUTH_WEST,
    DIR_WEST,
    DIR_NORTH_WEST,
    
    NUMDIRS,

	DIR_NULL
} dirtype_t;

typedef struct {
    uint64_t fileofs;
    uint64_t length;
} lump_t;

#define TEX2D_IDENT (('D'<<16)+('2'<<8)+'T')
#define TEX2D_VERSION 1

typedef struct {
    uint64_t ident;
    uint64_t version;

    char name[MAX_GDR_PATH];
    uint32_t minfilter;
    uint32_t magfilter;
    uint32_t wrapS;
    uint32_t wrapT;
    uint32_t multisampling;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t format; // OpenGL internal format
    uint32_t compression; // zlib or bzip2
    uint64_t compressedSize;
    uint64_t fileSize; // og file size
} tex2d_t;

#define TILE2D_MAGIC 0xfda218891
#define TILE2D_VERSION 0

typedef struct {
    vec2_t uv[4];
    uint32_t index;
} tile2d_sprite_t;

typedef struct {
    uint32_t numTiles;
    uint32_t tileWidth;
    uint32_t tileHeight;
    uint32_t tileCountX;
    uint32_t tileCountY;
    uint32_t compression; // only for sprites
    uint64_t compressedSize; // only for sprites
    char texture[MAX_GDR_PATH]; // store a texture inside of a tileset
} tile2d_info_t;

typedef struct {
    uint64_t magic;
    uint64_t version;
    tile2d_info_t info;
    tile2d_sprite_t *sprites;
} tile2d_header_t;

#define ANIM2D_IDENT (('D'<<24)+('2'<<16)+('A'<<8)+'#')
#define ANIM2D_VERSION 1

typedef struct {
    uint32_t index;
    uint32_t numtics;
} anim2d_frame_t;

typedef struct {
    uint64_t ident;
    uint64_t version;
    uint64_t numFrames;
    anim2d_frame_t *frames;
} anim2d_header_t;

#define MAP_IDENT (('#'<<24)+('P'<<16)+('A'<<8)+'M')
#define MAP_VERSION 1

#define MAX_MAP_SPAWNS 1024
#define MAX_MAP_CHECKPOINTS 256
#define MAX_MAP_LIGHTS 256

#define MAX_MAP_WIDTH 1024
#define MAX_MAP_HEIGHT 1024
#define MAX_MAP_TILES (MAX_MAP_WIDTH*MAX_MAP_HEIGHT)
#define MAX_MAP_VERTICES (MAX_MAP_TILES*4)
#define MAX_MAP_INDICES (MAX_MAP_TILES*6)

#define LUMP_TILES 0
#define LUMP_CHECKPOINTS 1
#define LUMP_SPAWNS 2
#define LUMP_LIGHTS 3
#define LUMP_VERTICES 4
#define LUMP_INDICES 5
#define LUMP_SPRITES 6
#define NUMLUMPS 7

#define TILETYPE_CHECKPOINT       0x0001
#define TILETYPE_SPAWN            0x0002
#define TILETYPE_NORMAL           0x0004
#define TILETYPE_BITS             0x000f

#define SURFACEPARM_NODLIGHT      0x0010
#define SURFACEPARM_METAL         0x0020
#define SURFACEPARM_LIQUID        0x0040
#define SURFACEPARM_BITS          0x00f0

typedef enum {
    light_point = 0,
    light_spotlight,
    light_area
} lighttype_t;

typedef struct {
    vec4_t color;
    uvec3_t origin;
    float brightness;
    float range;
    float linear;
    float quadratic;
    float constant;
    lighttype_t type;
} maplight_t;

typedef struct {
    float texcoords[4][2];
    byte sides[DIR_NULL]; // for physics
    vec4_t color;
    uvec3_t pos;
    int32_t index; // tileset texture index, -1 if not bound
    uint32_t flags;
} maptile_t;

typedef struct {
    vec3_t xyz;
    vec2_t uv;
    vec4_t color;
} mapvert_t;

typedef struct {
    char name[MAX_GDR_PATH];
    uint32_t minfilter;
    uint32_t magfilter;
    uint32_t wrapS;
    uint32_t wrapT;
    uint32_t multisampling;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t format; // OpenGL internal format
    uint32_t compression; // zlib or bzip2
    uint64_t compressedSize;
} maptexture_t;

typedef struct {
    char name[MAX_GDR_PATH];
    uint32_t numTiles;
    uint32_t tileWidth;
    uint32_t tileHeight;
    uint32_t compression;
    uint64_t compressedSize;
} maptileset_t;

typedef struct {
    uvec3_t xyz;
} mapcheckpoint_t;

typedef struct {
    uvec3_t xyz;
    uint32_t entitytype;
    uint32_t entityid;
} mapspawn_t;

typedef struct {
	uint32_t ident;
	uint32_t version;
    uint32_t mapWidth;
    uint32_t mapHeight;
    lump_t lumps[NUMLUMPS];
} mapheader_t;

#define LEVEL_IDENT (('M'<<24)+('F'<<16)+('F'<<8)+'B')
#define LEVEL_VERSION 0

typedef struct {
    uint32_t ident;
    uint32_t version;
    mapheader_t map;
    tile2d_header_t tileset;
} bmf_t;

#endif