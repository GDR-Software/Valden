#include "gln.h"
#include "editor.h"
#include "shader.h"

namespace Walnut {

#define MAX_RENDER_SHADERS 1024
static CShader *defaultShader;
CShader *shaders[MAX_RENDER_SHADERS];
static CShader *hashTable[MAX_RENDER_SHADERS];
static uint64_t numShaders;

static char *r_shaderText;
static const char *r_extensionOffset;
static uint64_t r_extendedShader;

static CShader shader;
static shaderStage_t stages[MAX_SHADER_STAGES];
static	texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define MAX_SHADERTEXT_HASH 2048
static const char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

static void ParseSort(const char **text)
{
	const char *tok;

	tok = COM_ParseExt(text, qfalse);
	if (tok[0] == 0) {
		Log_Printf("WARNING: missing sort in shader '%s'\n", shader.name);
		return;
	}

//	shader.sort = SS_BAD;

	if (!N_stricmp(tok, "opaque")) {
//		shader.sort = SS_OPAQUE;
	} else if (!N_stricmp(tok, "decal")) {
//		shader.sort = SS_DECAL;
	} else if (!N_stricmp(tok, "blend")) {
//		shader.sort = SS_BLEND;
	} else {
		Log_Printf("WARNING: invalid shaderSort name '%s' in shader '%s'\n", tok, shader.name);
	}
}

/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{	
	if ( !N_stricmp( funcname, "GT0" ) )
	{
		return GLS_ATEST_GT_0;
	}
	else if ( !N_stricmp( funcname, "LT128" ) )
	{
		return GLS_ATEST_LT_80;
	}
	else if ( !N_stricmp( funcname, "GE128" ) )
	{
		return GLS_ATEST_GE_80;
	}

	Log_Printf( "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}

/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if ( !N_stricmp( name, "GL_ONE" ) ) {
		return GLS_SRCBLEND_ONE;
	}
	else if ( !N_stricmp( name, "GL_ZERO" ) ) {
		return GLS_SRCBLEND_ZERO;
	}
	else if ( !N_stricmp( name, "GL_DST_COLOR" ) ) {
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if ( !N_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if ( !N_stricmp( name, "GL_SRC_ALPHA" ) ) {
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_DST_ALPHA" ) ) {
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
    }
	else if ( !N_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) ) {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	Log_Printf( "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if ( !N_stricmp( name, "GL_ONE" ) ) {
		return GLS_DSTBLEND_ONE;
	}
	else if ( !N_stricmp( name, "GL_ZERO" ) ) {
		return GLS_DSTBLEND_ZERO;
	}
	else if ( !N_stricmp( name, "GL_SRC_ALPHA" ) ) {
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_DST_ALPHA" ) ) {
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !N_stricmp( name, "GL_SRC_COLOR" ) ) {
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if ( !N_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	Log_Printf( "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if ( !N_stricmp( funcname, "sin" ) ) {
		return GF_SIN;
	}
	else if ( !N_stricmp( funcname, "square" ) ) {
		return GF_SQUARE;
	}
	else if ( !N_stricmp( funcname, "triangle" ) ) {
		return GF_TRIANGLE;
	}
	else if ( !N_stricmp( funcname, "sawtooth" ) ) {
		return GF_SAWTOOTH;
	}
	else if ( !N_stricmp( funcname, "inversesawtooth" ) ) {
		return GF_INVERSE_SAWTOOTH;
	}
	else if ( !N_stricmp( funcname, "noise" ) ) {
		return GF_NOISE;
	}

	Log_Printf( "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( const char **text, int count, float *v ) {
	const char	*tok;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	tok = COM_ParseExt( text, qfalse );
	if ( strcmp( tok, "(" ) ) {
		Log_Printf( "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		tok = COM_ParseExt( text, qfalse );
		if ( !tok[0] ) {
			Log_Printf( "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( tok );
	}

	tok = COM_ParseExt( text, qfalse );
	if ( strcmp( tok, ")" ) ) {
		Log_Printf( "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave )
{
	const char *tok;

	tok = COM_ParseExt( text, qfalse );
	if ( tok[0] == 0 )
	{
		Log_Printf( "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( tok );

	// BASE, AMP, PHASE, FREQ
	tok = COM_ParseExt( text, qfalse );
	if ( tok[0] == 0 )
	{
		Log_Printf( "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( tok );

	tok = COM_ParseExt( text, qfalse );
	if ( tok[0] == 0 )
	{
		Log_Printf( "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( tok );

	tok = COM_ParseExt( text, qfalse );
	if ( tok[0] == 0 )
	{
		Log_Printf( "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( tok );

	tok = COM_ParseExt( text, qfalse );
	if ( tok[0] == 0 )
	{
		Log_Printf( "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( tok );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( const char *_text, shaderStage_t *stage )
{
	const char *tok;
	const char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		Error( "ERROR: too many tcMod stages in shader '%s'", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	tok = COM_ParseExt( text, qfalse );

	//
	// turb
	//
	if ( !N_stricmp( tok, "turb" ) ) {
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( tok );
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( tok );
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( tok );
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( tok );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !N_stricmp( tok, "scale" ) ) {
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( tok );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !N_stricmp( tok, "scroll" ) ) {
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( tok );
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( tok );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !N_stricmp( tok, "stretch" ) ) {
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( tok );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !N_stricmp( tok, "transform" ) ) {
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( tok );

		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( tok );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !N_stricmp( tok, "rotate" ) ) {
		tok = COM_ParseExt( text, qfalse );
		if ( tok[0] == 0 ) {
			Log_Printf( "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( tok );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !N_stricmp( tok, "entityTranslate" ) ) {
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else {
		Log_Printf( "WARNING: unknown tcMod '%s' in shader '%s'\n", tok, shader.name );
	}
}


static qboolean ParseStage(shaderStage_t *stage, const char **text)
{
    const char *tok;
    uint32_t depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;
	stage->bundle[0].filter = (textureFilter_t)-1;

    while (1) {
        tok = COM_ParseExt(text, qtrue);
        if (!tok[0]) {
            Log_Printf("WARNING: no matching '}' found\n");
            return qfalse;
        }

        if (tok[0] == '}') {
            break;
        }
        //
        // map <name>
        //
        else if (!N_stricmp(tok, "map")) {
            tok = COM_ParseExt(text, qfalse);
			if (!tok[0]) {
				Log_Printf("WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!N_stricmp(tok, "$whiteimage")) {
				stage->bundle[0].image = NULL;
				continue;
			}
			else if (!N_stricmp(tok, "$lightmap")) {
				stage->bundle[0].isLightmap = qtrue;
				stage->bundle[0].image = NULL;
			}
			else {
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if ( !shader.noMipMaps ) {
					flags |= IMGFLAG_MIPMAP;
				}
				if ( !shader.noPicMip ) {
					flags |= IMGFLAG_PICMIP;
				}
				if ( shader.noLightScale ) {
					flags |= IMGFLAG_NOLIGHTSCALE;
				}
                
                stage->bundle[0].image = FindImage( tok, type, flags );

				if ( !stage->bundle[0].image ) {
					Log_Printf( "WARNING: FindImage could not find '%s' in shader '%s'\n", tok, shader.name );
					return qfalse;
				}
			}
        }
		//
		// clampmap <name>
		//
		else if ( !N_stricmp( tok, "clampmap") || ( !N_stricmp( tok, "screenMap" ) && r_extendedShader ) ) {
			imgFlags_t flags;

			/*
			if ( !N_stricmp( tok, "screenMap" ) ) {
				flags = IMGFLAG_NONE;
				if ( glContext.ARB_framebuffer_object ) {
					stage->bundle[0].isScreenMap = qtrue;
					shader.hasScreenMap = qtrue;
					tr.needScreenMap = qtrue;
				}
			} else
			*/
			{
				flags = IMGFLAG_CLAMPTOEDGE;
			}

			tok = COM_ParseExt( text, qfalse );
			if ( !tok[0] ) {
				Log_Printf( "WARNING: missing parameter for '%s' keyword in shader '%s'\n",
					/* stage->bundle[0].isScreenMap */ 0 ? "screenMap" : "clampMap", shader.name );
				return qfalse;
			}

			if ( !shader.noMipMaps ) {
				flags |= IMGFLAG_MIPMAP;
			}
			if ( !shader.noPicMip ) {
				flags |= IMGFLAG_PICMIP;
			}
			if ( shader.noLightScale ) {
				flags |= IMGFLAG_NOLIGHTSCALE;
			}

			stage->bundle[0].image = FindImage( tok, IMGTYPE_COLORALPHA, flags );
			if ( !stage->bundle[0].image ) {
				Log_Printf( "WARNING: FindImage could not find '%s' in shader '%s'\n", tok, shader.name );
				return qfalse;
			}
		}
        //
		// alphafunc <func>
		//
		else if ( !N_stricmp( tok, "alphaFunc" ) ) {
			tok = COM_ParseExt( text, qfalse );
			if ( !tok[0] ) {
				Log_Printf( "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( tok );
		}
		//
		// depthFunc <func>
		//
		else if ( !N_stricmp( tok, "depthfunc" ) ) {
			tok = COM_ParseExt( text, qfalse );

			if ( !tok[0] ) {
				Log_Printf( "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !N_stricmp( tok, "lequal" ) ) {
				depthFuncBits = 0;
			}
			else if ( !N_stricmp( tok, "equal" ) ) {
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else {
				Log_Printf( "WARNING: unknown depthfunc '%s' in shader '%s'\n", tok, shader.name );
				continue;
			}
		}
		//
		// specularScale <rgb> <gloss>
		// or specularScale <metallic> <smoothness> with r_pbr 1
		// or specularScale <r> <g> <b>
		// or specularScale <r> <g> <b> <gloss>
		//
		else if ( !N_stricmp( tok, "specularscale" ) ) {
			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				Log_Printf( "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[0] = atof( tok );

			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				Log_Printf( "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[1] = atof( tok );

			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				// two values, rgb then gloss
				stage->specularScale[3] = stage->specularScale[1];
				stage->specularScale[1] =
				stage->specularScale[2] = stage->specularScale[0];
				continue;
			}

			stage->specularScale[2] = atof( tok );

			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				// three values, rgb
				continue;
			}

			stage->specularScale[3] = atof( tok );

		}
		//
		// tcGen <function>
		//
		else if ( !N_stricmp( tok, "texgen" ) || !N_stricmp( tok, "tcGen" ) ) {
			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				Log_Printf( "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}
		
			if ( !N_stricmp( tok, "environment" ) ) {
				const char *t = *text;
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
				tok = COM_ParseExt( text, qfalse );
				if ( N_stricmp( tok, "firstPerson" ) == 0 ) {
					//stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED_FP;
				}
				else
				{
					*text = t; // rewind
				}
			}
			else if ( !N_stricmp( tok, "lightmap" ) ) {
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if ( !N_stricmp( tok, "texture" ) || !N_stricmp( tok, "base" ) ) {
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if ( !N_stricmp( tok, "vector" ) ) {
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else {
				Log_Printf( "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !N_stricmp( tok, "tcMod" ) ) {
			char buffer[1024] = "";

			while ( 1 ) {
				tok = COM_ParseExt( text, qfalse );
				if ( tok[0] == 0 ) {
					break;
				}
				strncat( buffer, tok,  sizeof(buffer) );
				strncat( buffer, " ",  sizeof(buffer) );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !N_stricmp( tok, "blendfunc" ) ) {
			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				Log_Printf( "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !N_stricmp( tok, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !N_stricmp( tok, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !N_stricmp( tok, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( tok );

				tok = COM_ParseExt( text, qfalse );
				if ( tok[0] == 0 ) {
					Log_Printf( "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( tok );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit ) {
				depthMaskBits = 0;
			}
		}
		//
		// texFilter <linear|nearest|bilinear|trilinear>
		//
		else if ( !N_stricmp( tok, "texFilter" ) ) {
			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 ) {
				stage->bundle[0].filter = NumTexFilters;
				Log_Printf( "WARNING: missing parameters for texFilter in shader '%s'\n", shader.name );
				continue;
			}
			if ( !N_stricmp( tok, "linear" ) ) {
				stage->bundle[0].filter = TexFilter_Linear;
			}
			else if ( !N_stricmp( tok, "nearest" ) ) {
				stage->bundle[0].filter = TexFilter_Nearest;
			}
			else if ( !N_stricmp( tok, "bilinear" ) ) {
				stage->bundle[0].filter = TexFilter_Bilinear;
			}
			else if ( !N_stricmp( tok, "trilinear" ) ) {
				stage->bundle[0].filter = TexFilter_Trilinear;
			}
			else {
				stage->bundle[0].filter = NumTexFilters;
				Log_Printf( "WARNING: unknown texFilter parameter '%s' in shader '%s'\n", tok, shader.name );
				continue;
			}
		}
		//
		// rgbGen
		//
		else if ( !N_stricmp( tok, "rgbGen" ) )
		{
			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 )
			{
				Log_Printf( "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}
			if ( !N_stricmp( tok, "const" ) )
			{
				vec3_t	color;

				VectorClear( color );

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if ( !N_stricmp( tok, "identity" ) )
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if ( !N_stricmp( tok, "identityLighting" ) )
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if ( !N_stricmp( tok, "entity" ) )
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if ( !N_stricmp( tok, "oneMinusEntity" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if ( !N_stricmp( tok, "vertex" ) )
			{
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !N_stricmp( tok, "exactVertex" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if ( !N_stricmp( tok, "vertexLit" ) )
			{
				stage->rgbGen = CGEN_VERTEX_LIT;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !N_stricmp( tok, "exactVertexLit" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX_LIT;
			}
			else if ( !N_stricmp( tok, "lightingDiffuse" ) )
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if ( !N_stricmp( tok, "oneMinusVertex" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				Log_Printf( "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", tok, shader.name );
				continue;
			}
		}
        //
		// alphaGen
		//
		else if ( !N_stricmp( tok, "alphaGen" ) )
		{
			tok = COM_ParseExt( text, qfalse );
			if ( tok[0] == 0 )
			{
				Log_Printf( "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}
			if ( !N_stricmp( tok, "const" ) )
			{
				tok = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * atof( tok );
				stage->alphaGen = AGEN_CONST;
			}
			else if ( !N_stricmp( tok, "identity" ) )
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if ( !N_stricmp( tok, "entity" ) )
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if ( !N_stricmp( tok, "oneMinusEntity" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if ( !N_stricmp( tok, "vertex" ) )
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if ( !N_stricmp( tok, "lightingSpecular" ) )
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if ( !N_stricmp( tok, "oneMinusVertex" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				Log_Printf( "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", tok, shader.name );
				continue;
			}
		}
        else {
            Log_Printf( "WARNING: unrecognized parameter in shader '%s' stage %i: '%s'\n", shader.name, (int)(stage - stages), tok);
        }
    }

    //
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE ||
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) )
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == AGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			|| stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
		                blendSrcBits | blendDstBits |
		                atestBits |
		                depthFuncBits;

    return qtrue;
}

static qboolean ParseShader( const char **text )
{
    const char *tok;
	int s;
    unsigned depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
    qboolean depthMaskExplicit;
	uintptr_t p1, p2;

	// gcc doesn't like it when we do *text + r_extensionOffset
	p1 = (uintptr_t)(*text);
	p2 = (uintptr_t)(r_extensionOffset);

	s = 0;
	r_extendedShader = p1 + p2;

	tok = COM_ParseExt(text, qtrue);
	if (tok[0] != '{') {
		Log_Printf("WARNING: expecting '{', found '%s' instead in shader '%s'\n", tok, shader.name);
		return qfalse;
	}

    while (1) {
        tok = COM_ParseComplex(text, qtrue);

        if (!tok[0]) {
            Log_Printf("WARNING: no concluding '}' in shader %s\n", shader.name);
            return qfalse;
        }

		// end of shader definition
        if (tok[0] == '}') {
            break;
        }
        // stage definition
        if (tok[0] == '{') {
            if (s >= MAX_SHADER_STAGES) {
                Log_Printf("WARNING: too many stages in shader %s (max is %i)\n", shader.name, MAX_SHADER_STAGES);
                return qfalse;
            }
            if (!ParseStage(&stages[s], text)) {
                return qfalse;
            }
            stages[s].active = qtrue;
            s++;
            
            continue;
        }
		// disable picmipping
		else if ( !N_stricmp( tok, "nopicmip" )) {
			shader.noPicMip = qtrue;
			continue;
		}
		// disable mipmapping
		else if ( !N_stricmp( tok, "nomipmaps")) {
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
        //
        // shaderSort <sort>
        //
        else if ( !N_stricmp( tok, "shaderSort") )
        {
            tok = COM_ParseExt( text, qfalse );
            if ( tok[0] == 0 ) {
                Log_Printf( "WARNING: missing parameter for shaderSort in shader '%s'\n", shader.name);
                continue;
            }
            ParseSort( text );
        }
		// polygonOffset
		else if (!N_stricmp(tok, "polygonOffset")) {
			shader.polygonOffset = qtrue;
			continue;
		}
        else {
            Log_Printf("WARNING: unrecognized parameter in shader '%s': '%s'\n", shader.name, tok);
            continue;
        }
    }

    return qtrue;
}

static CShader *GeneratePermanentShader( void )
{
    CShader *newShader;
    uint64_t size, hash;

    newShader = new CShader;

    *newShader = shader;

    shaders[numShaders] = newShader;

    numShaders++;

	for (uint32_t i = 0; i < MAX_SHADER_STAGES; i++) {
		if (!stages[i].active) {
			break;
		}

		newShader->stages[i] = (shaderStage_t *)GetMemory( sizeof(stages[i]) );
		*newShader->stages[i] = stages[i];

		for ( uint32_t b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			if ( size ) {
				newShader->stages[i]->bundle[b].texMods =  (texModInfo_t *)GetMemory( size );
				memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
			}
		}
	}

    hash = Com_GenerateHashValue(newShader->name.c_str(), MAX_RENDER_SHADERS);
    newShader->next = hashTable[hash];
    hashTable[hash] = newShader;
    
    return newShader;
}



/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static const char *FindShaderInShaderText( const char *shadername )
{
	const char *tok, *p;
	uint64_t i, hash;

	hash = Com_GenerateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if (shaderTextHashTable[hash]) {
		for (i = 0; shaderTextHashTable[hash][i]; i++) {
			p = shaderTextHashTable[hash][i];
			tok = COM_ParseExt(&p, qtrue);
			if (!N_stricmp(tok, shadername))
				return p;
		}
	}

	return NULL;
}

/*
* InitShader
*/
static void InitShader( const char *name )
{
	memset( &shader, 0, sizeof(shader) );
	shader.name = name;

	for ( uint32_t i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];

		// default normal/specular
		VectorSet4( stages[i].normalScale, 0.0f, 0.0f, 0.0f, 0.0f );
		stages[i].specularScale[0] =
		stages[i].specularScale[1] =
		stages[i].specularScale[2] = 1.0f;
		stages[i].specularScale[3] = 1.0f;
	}
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
CShader *R_FindShaderByName( const char *name )
{
	char		strippedName[MAX_GDR_PATH];
	uint64_t	hash;
	CShader	    *sh;

	if ( (name == NULL) || (name[0] == 0) ) {
		return defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = Com_GenerateHashValue(strippedName, MAX_RENDER_SHADERS);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (N_stricmp(sh->name.c_str(), strippedName) == 0) {
			// match found
			return sh;
		}
	}

	return defaultShader;
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as appropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as appropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as appropriate for
most world construction surfaces.

===============
*/
CShader *R_FindShader( const char *name ) {
	char		strippedName[MAX_GDR_PATH];
	uint64_t	hash;
	const char	*shaderText;
	Image 		*image;
	CShader	    *sh;

	if ( name[0] == '\0' ) {
		return defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = Com_GenerateHashValue(strippedName, MAX_RENDER_SHADERS);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (!N_stricmp(sh->name.c_str(), strippedName)) {
			// match found
			return sh;
		}
	}

	InitShader( strippedName );

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
        sh = GeneratePermanentShader();
		return sh;
	}

    sh->lightmapIndex = LIGHTMAP_2D;

	//
	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	//
	{
		imgFlags_t flags;

		flags = IMGFLAG_NONE;
		flags |= IMGFLAG_CLAMPTOEDGE;

		image = FindImage( name, IMGTYPE_COLORALPHA, flags );
		if ( !image ) {
		    Log_Printf( "Couldn't find image file for shader %s\n", name );
			shader.defaultShader = qtrue;
			return sh;
		}
	}

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image = NULL;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image = NULL;
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return GeneratePermanentShader();
}

#define	MAX_SHADER_FILES 16384

static int loadShaderBuffers( std::vector<std::string>& shaderFiles, const uint64_t numShaderFiles, char **buffers )
{
	std::string filename;
	char shaderName[MAX_NPATH];
	const char *p, *tok;
	uint64_t summand, sum = 0;
	uint64_t shaderLine;
	uint64_t i;
	const char *shaderStart;
	qboolean denyErrors;

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ ) {
		filename = shaderFiles[i];
		Log_Printf( "...loading '%s'\n", filename.c_str() );
		summand = LoadFile( filename.c_str(), (void **)&buffers[i] );

		if ( !buffers[i] || !summand ) {
			Log_FPrintf( SYS_WRN, "Couldn't load %s.\n", filename.c_str() );
			continue;
		}

		p = buffers[i];
		COM_BeginParseSession( filename.c_str() );
		
		shaderStart = NULL;
		denyErrors = qfalse;

		while ( 1 ) {
			tok = COM_ParseExt( &p, qtrue );
			
			if ( !*tok )
				break;

			N_strncpyz( shaderName, tok, sizeof(shaderName) );
			shaderLine = COM_GetCurrentParseLine();

			tok = COM_ParseExt( &p, qtrue );
			if ( tok[0] != '{' || tok[1] != '\0' ) {
				Log_Printf( "File %s: shader \"%s\" " \
					"on line %lu missing opening brace", filename.c_str(), shaderName, shaderLine );
				if ( tok[0] )
					Log_Printf( " (found \"%s\" on line %lu)\n", tok, COM_GetCurrentParseLine() );
				else
					Log_Printf( "\n" );

				if ( denyErrors || !p )
				{
					Log_Printf( "Ignoring entire file '%s' due to error.\n", filename.c_str() );
					FreeMemory( buffers[i] );
					buffers[i] = NULL;
					break;
				}

				SkipRestOfLine( &p );
				shaderStart = p;
				continue;
			}

			if ( !SkipBracedSection( &p, 1 ) ) {
				Log_Printf( "WARNING: Ignoring shader file %s. Shader \"%s\" " \
					"on line %lu missing closing brace.\n", filename.c_str(), shaderName, shaderLine );
				FreeMemory( buffers[i] );
				buffers[i] = NULL;
				break;
			}

			denyErrors = qtrue;
		}

		if ( buffers[ i ] ) {
			if ( shaderStart ) {
				summand -= (shaderStart - buffers[i]);
				if ( summand >= 0 ) {
					memmove( buffers[i], shaderStart, summand + 1 );
				}
			}
			//sum += summand;
			sum += COM_Compress( buffers[ i ] );
		}
	}

	return sum;
}


static char *hashMem = NULL;


static void LoadShaderFiles( std::vector<std::string>& shaderFiles, const std::filesystem::path& currentDir )
{
	for ( const auto& it : std::filesystem::directory_iterator{ currentDir } ) {
		std::string path = std::move( it.path().string() );

        if ( it.is_directory() ) {
			LoadShaderFiles( shaderFiles, it );
		}
		else if ( !N_stricmp( COM_GetExtension( path.c_str() ), "shader" ) ) {
            shaderFiles.emplace_back( path );
        }
	}
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
static void ScanAndLoadShaderFiles( void )
{
    std::vector<std::string> shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	uint64_t numShaderFiles;
	int64_t i;
	const char *tok;
	char *textEnd;
	const char *p, *oldp;
	uint64_t shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;

	memset(buffers, 0, sizeof(buffers));
	memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

    uint64_t sum = 0;

	LoadShaderFiles( shaderFiles, g_pProjectManager->GetProjectDirectory() );

	numShaderFiles = shaderFiles.size();

	Log_Printf( "Found %lu shader files.\n", numShaderFiles );

	if (!numShaderFiles) {
		Log_Printf( "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	sum = 0;
	sum += loadShaderBuffers( shaderFiles, numShaderFiles, buffers );

	// build single large buffer
	r_shaderText = (char *)GetMemory( sum + numShaderFiles*2 + 1 );
	memset( r_shaderText, 0, sum + numShaderFiles*2 );

	textEnd = r_shaderText;

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0 ; i-- ) {
		if ( buffers[ i ] ) {
			textEnd = N_stradd( textEnd, buffers[ i ] );
			textEnd = N_stradd( textEnd, "\n" );
			FreeMemory( buffers[ i ] );
		}
	}

	// if shader text >= r_extensionOffset then it is an extended shader
	// normal shaders will never encounter that
	r_extensionOffset = textEnd;

	COM_Compress( r_shaderText );
	memset( shaderTextHashTableSizes, 0, sizeof( shaderTextHashTableSizes ) );
	size = 0;

	p = r_shaderText;
	// look for shader names
	while ( 1 ) {
		tok = COM_ParseExt( &p, qtrue );
		if ( tok[0] == 0 ) {
			break;
		}
		hash = Com_GenerateHashValue(tok, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection(&p, 0);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char *)GetMemory( size * sizeof(char *) );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (const char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	p = r_shaderText;
	// look for shader names
	while ( 1 ) {
		oldp = p;
		tok = COM_ParseExt( &p, qtrue );
		if ( tok[0] == 0 ) {
			break;
		}

		hash = Com_GenerateHashValue(tok, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][--shaderTextHashTableSizes[hash]] = (char*)oldp;

		SkipBracedSection(&p, 0);
	}
}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void )
{
	numShaders = 0;

	// init the default shader
	InitShader( "<default>" );
	stages[0].bundle[0].image = NULL;
	stages[0].active = qtrue;
    stages[0].stateBits = GLS_DEFAULT;
	defaultShader = GeneratePermanentShader();
}

/*
==================
InitShaders
==================
*/
void InitShaders( void )
{
	return;
	Log_Printf( "Initializing Shaders\n" );

	for ( uint64_t i = 0; i > numShaders; i++ ) {
		if ( hashTable[i] ) {
			delete hashTable[i];
		}
	}

	if ( r_shaderText ) {
		FreeMemory( r_shaderText );
	}

	if ( hashMem ) {
		FreeMemory( hashMem );
	}

    memset( hashTable, 0, sizeof(hashTable) );

	CreateInternalShaders();

	ScanAndLoadShaderFiles();
}

CShader::CShader( void )
{
    memset( this, 0, sizeof(*this) );
}

CShader::~CShader()
{
    for ( uint32_t i = 0; i < MAX_SHADER_STAGES; i++ ) {
        if ( !stages[i] ) {
            break;
        }

        if ( stages[i]->bundle[0].texMods ) {
            FreeMemory( stages[i]->bundle[0].texMods );
        }

        FreeMemory( stages[i] );
    }
}

};
