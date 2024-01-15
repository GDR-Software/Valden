#ifndef __SHADER__
#define __SHADER__

#pragma once

#include "Walnut/Image.h"

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader()
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

namespace Walnut {

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define GLS_SRCBLEND_BITS						0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define GLS_DSTBLEND_BITS						0x000000f0

#define GLS_BLEND_BITS							(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000
#define GLS_DEPTHFUNC_GREATER                   0x00040000
#define GLS_DEPTHFUNC_BITS                      0x00060000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define GLS_ATEST_BITS							0x70000000

#define GLS_DEFAULT                             (GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)


#define MAX_SHADER_STAGES 8

typedef enum {
    SS_BAD,             // throw error

    SS_OPAQUE,          // opaque stuff
    SS_DECAL,           // scorch marks, blood splats, etc.
    SS_SEE_THROUGH,     // ladders, grates, grills that may have small blended edges

    SS_BLEND,           // the standard
} shaderSort_t;

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;


typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST,
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_EXACT_VERTEX_LIT,	// like CGEN_EXACT_VERTEX but takes a light direction from the lightgrid
	CGEN_VERTEX_LIT,		// like CGEN_VERTEX but takes a light direction from the lightgrid
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_FOG,				// standard fog
	CGEN_CONST				// fixed color
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_ENVIRONMENT_MAPPED_FP, // with correct first-person mapping
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	float base;
	float amplitude;
	float phase;
	float frequency;

	genFunc_t	func;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE,
	TMOD_OFFSET,
	TMOD_SCALE_OFFSET,
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
} deformStage_t;

typedef struct {
	texMod_t		type;

	union {

		// used for TMOD_TURBULENT and TMOD_STRETCH
		waveForm_t		wave;

		// used for TMOD_TRANSFORM
		struct {
			float		matrix[2][2];	// s' = s * m[0][0] + t * m[1][0] + trans[0]
			float		translate[2];	// t' = s * m[0][1] + t * m[0][1] + trans[1]
		};

		// used for TMOD_SCALE, TMOD_OFFSET, TMOD_SCALE_OFFSET
		struct {
			float		scale[2];		// s' = s * scale[0] + offset[0]
			float		offset[2];		// t' = t * scale[1] + offset[1]
		};

		// used for TMOD_SCROLL
		float			scroll[2];		// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time
		// used for TMOD_ROTATE
		// + = clockwise
		// - = counterclockwise
		float			rotateSpeed;

	};

} texModInfo_t;


//#define MAX_IMAGE_ANIMATIONS		24
//#define MAX_IMAGE_ANIMATIONS_VQ3	8

#define LIGHTMAP_INDEX_NONE			0
#define LIGHTMAP_INDEX_SHADER		1
#define LIGHTMAP_INDEX_OFFSET		2

typedef struct {
	Image 			*image;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	uint32_t		numTexMods;
	texModInfo_t	*texMods;

    qboolean isLightmap;

    textureFilter_t filter;
} textureBundle_t;

#define TESS_ST0	1<<0
#define TESS_ST1	1<<1
#define TESS_ENV0	1<<2
#define TESS_ENV1	1<<3

enum
{
	TB_COLORMAP    = 0,
	TB_DIFFUSEMAP  = 0,
	TB_LIGHTMAP    = 1,
	TB_LEVELSMAP   = 1,
	TB_SHADOWMAP3  = 1,
	TB_NORMALMAP   = 2,
	TB_DELUXEMAP   = 3,
	TB_SHADOWMAP2  = 3,
	TB_SPECULARMAP = 4,
	TB_SHADOWMAP   = 5,
	TB_CUBEMAP     = 6,
	TB_SHADOWMAP4  = 6,

    NUM_TEXTURE_BUNDLES
};

typedef struct {
    qboolean active;

    textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

    waveForm_t rgbWave;
    colorGen_t rgbGen;

    waveForm_t alphaWave;
    alphaGen_t alphaGen;

    uint32_t stateBits;         // GLS_xxxx mask

    byte constantColor[4];      // for CGEN_CONST and AGEN_CONST

    vec4_t normalScale;
    vec4_t specularScale;
} shaderStage_t;

class CShader
{
public:
    CShader( void );
    ~CShader();
    
    void Iterate( void );
public:
    std::string name;
    
    shaderStage_t *stages[MAX_SHADER_STAGES];

    qboolean explicitlyDefined;     // found in a .shader file

    int32_t lightmapIndex;

    uint32_t surfaceFlags;          // if explicitly defined this will have SURFPARM_* flags

    uint32_t vertexAttribs;         // not all shaders will need all data to be gathered
    
    qboolean noLightScale;
    qboolean polygonOffset;		    // set for decals and other items that must be offset 
	qboolean noMipMaps;			    // for console fonts, 2D elements, etc.
	qboolean noPicMip;			    // for images that must always be full resolution

    int32_t lightingStage;

    qboolean defaultShader;		    // we want to return index 0 if the shader failed to
								    // load for some reason, but R_FindShader should
								    // still keep a name allocated for it, so if
								    // something calls RE_RegisterShader again with
								    // the same name, we don't try looking for it again
    
    CShader *next;
};

void InitShaders( void );
CShader *R_FindShader( const char *name );

};

#endif