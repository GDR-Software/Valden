#include "editor.h"
#include <glm/glm.hpp>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

mapData_t *mapData;
std::vector<mapData_t> g_MapCache;
static const char *unnamed_map = "unnamed.map";
static spriteCoord_t *s_pSpritePOD;
static maptile_t *s_pTilePOD;
static bool s_bLoadingMap;

typedef enum {
    CHUNK_CHECKPOINT,
    CHUNK_SPAWN,
    CHUNK_LIGHT,
    CHUNK_TILE,
    CHUNK_TILESET,
    CHUNK_TEXCOORDS,
    CHUNK_SECRET,
    
    CHUNK_INVALID
} chunkType_t;

static bool ParseChunk( const char **text, mapData_t *tmpData )
{
    const char *tok;
    chunkType_t type;

    type = CHUNK_INVALID;

    while ( 1 ) {
        tok = COM_ParseExt( text, qtrue );
        if ( !tok[0] ) {
            COM_ParseWarning( "no matching '}' found" );
            return false;
        }
        
        if ( tok[0] == '}' ) {
            switch ( type ) {
            case CHUNK_CHECKPOINT:
                tmpData->numCheckpoints++;
                break;
            case CHUNK_SPAWN:
                tmpData->numSpawns++;
                break;
            case CHUNK_LIGHT:
                tmpData->numLights++;
                break;
            case CHUNK_TILE:
                tmpData->numTiles++;
                break;
            case CHUNK_TEXCOORDS:
                tmpData->tileset.numTiles++;
                break;
            case CHUNK_SECRET:
                tmpData->numSecrets++;
                break;
            };
            break;
        }
        //
        // classname <name>
        //
        else if ( !N_stricmp( tok, "classname" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseWarning( "missing parameter for classname" );
                return false;
            }
            else if ( !N_stricmp( tok, "map_checkpoint" ) ) {
                type = CHUNK_CHECKPOINT;
            }
            else if ( !N_stricmp( tok, "map_spawn" ) ) {
                type = CHUNK_SPAWN;
            }
            else if ( !N_stricmp(tok, "map_light" ) ) {
                type = CHUNK_LIGHT;
            }
            else if ( !N_stricmp( tok, "texcoords" ) ) {
                type = CHUNK_TEXCOORDS;
            }
            else if ( !N_stricmp( tok, "map_tile" ) ) {
                type = CHUNK_TILE;
            }
            else if ( !N_stricmp( tok, "map_secret" ) ) {
                type = CHUNK_SECRET;
            }
            else if ( !N_stricmp( tok, "tilesetdata" ) ) {
                type = CHUNK_TILESET;
            }
            else {
                COM_ParseWarning( "unrecognized token for classname '%s'", tok );
                return false;
            }
        }
        //
        // entity <entitytype>
        //
        else if ( !N_stricmp( tok, "entity" ) ) {
            if ( type != CHUNK_SPAWN ) {
                COM_ParseError( "found parameter \"entity\" in chunk that isn't a spawn" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for spawn entity type" );
                return false;
            }
            tmpData->spawns[tmpData->numSpawns].entitytype = (uint32_t)atoi( tok );
        }
        /*
        //
        // tileCountX <count>
        //
        else if ( !N_stricmp( tok, "tileCountX" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset tileCountX" );
                return false;
            }
            tileset->tileCountX = (uint32_t)atoi(tok);
        }
        //
        // tileCountY <count>
        //
        else if (!N_stricmp(tok, "tileCountY")) {
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for tileset tileCountY");
                return false;
            }
            tileset->tileCountY = (uint32_t)atoi(tok);
        }
        */
        //
        // numTiles <number>
        //
        else if ( !N_stricmp( tok, "numTiles" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset numTiles" );
                return false;
            }
            tmpData->tileset.numTiles = (uint32_t)atoi( tok );
        }
        //
        // tileWidth <width>
        //
        else if ( !N_stricmp( tok, "tileWidth" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset tileWidth" );
                return false;
            }
            tmpData->tileset.tileWidth = (uint32_t)atoi( tok );
        }
        //
        // shader <name>
        //
        else if ( !N_stricmp( tok, "shader" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset shader" );
                return false;
            }
            N_strncpyz( tmpData->tileset.texture, tok, sizeof(tmpData->tileset.texture) );
        }
        //
        // diffuseMap <name>
        //
        else if ( !N_stricmp( tok, "diffuseMap" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset diffuseMap" );
                return false;
            }
            else if ( tok[0] == ' ' ) {
                continue;
            }
            tmpData->textures[Walnut::TB_DIFFUSEMAP] = new Walnut::Image( tok );
        }
        //
        // specularMap <name>
        //
        else if ( !N_stricmp( tok, "specularMap" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset specularMap" );
                return false;
            }
            else if ( tok[0] == ' ' ) {
                continue;
            }
            tmpData->textures[Walnut::TB_SPECULARMAP] = new Walnut::Image( tok );
        }
        //
        // normalMap <name>
        //
        else if ( !N_stricmp( tok, "normalMap" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset normalMap" );
                return false;
            }
            else if ( tok[0] == ' ' ) {
                continue;
            }
            tmpData->textures[Walnut::TB_NORMALMAP] = new Walnut::Image( tok );
        }
        //
        // lightMap <name>
        //
        else if ( !N_stricmp( tok, "lightMap" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset diffuseMap" );
                return false;
            }
            else if ( tok[0] == ' ' ) {
                continue;
            }
            tmpData->textures[Walnut::TB_LIGHTMAP] = new Walnut::Image( tok );
        }
        //
        // shadowMap <name>
        //
        else if ( !N_stricmp( tok, "shadowMap" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset shadowMap" );
                return false;
            }
            else if ( tok[0] == ' ' ) {
                continue;
            }
            tmpData->textures[Walnut::TB_SHADOWMAP] = new Walnut::Image( tok );
        }
        //
        // tileHeight <height>
        //
        else if ( !N_stricmp( tok, "tileHeight" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tileset tileHeight" );
                return false;
            }
            tmpData->tileset.tileHeight = (uint32_t)atoi( tok );
        }
        //
        // texIndex <index>
        //
        else if ( !N_stricmp( tok, "texIndex" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for map tile texIndex" );
                return false;
            }
            tmpData->tiles[tmpData->numTiles].index = (int32_t)atoi( tok );
        }
        //
        // id <entityid>
        //
        else if ( !N_stricmp( tok, "id" ) ) {
            if ( type != CHUNK_SPAWN ) {
                COM_ParseError( "found parameter \"id\" in chunk that isn't a spawn" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for spawn entity id" );
                return false;
            }
            tmpData->spawns[tmpData->numSpawns].entityid = atoi( tok );

            bool valid = false;
            for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[tmpData->spawns[tmpData->numSpawns].entitytype] ) {
                if ( it.m_Id == tmpData->spawns[tmpData->numSpawns].entityid ) {
                    valid = true;
                    break;
                }
            }
            if ( !valid ) {
                COM_ParseError( "invalid entity id found in map spawn: %u", tmpData->spawns[tmpData->numSpawns].entityid );
                return false;
            }
        }
        //
        // flags <flags>
        //
        else if ( !N_stricmp( tok, "flags" ) ) {
            if ( type != CHUNK_TILE ) {
                COM_ParseError( "found parameter \"flags\" in chunk that isn't a tile" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for tile flags" );
                return false;
            }
            tmpData->tiles[tmpData->numTiles].flags = (uint32_t)ParseHex( tok );
        }
        //
        // sides <sides...>
        //
        else if ( !N_stricmp( tok, "sides" ) ) {
            float sides[NUMDIRS];
            if ( !Parse1DMatrix( text, NUMDIRS, sides ) ) {
                COM_ParseError( "failed to parse sides for map tile" );
                return false;
            }
            tmpData->tiles[tmpData->numTiles].sides[DIR_NORTH]       = sides[DIR_NORTH];
            tmpData->tiles[tmpData->numTiles].sides[DIR_NORTH_EAST]  = sides[DIR_NORTH_EAST];
            tmpData->tiles[tmpData->numTiles].sides[DIR_EAST]        = sides[DIR_EAST];
            tmpData->tiles[tmpData->numTiles].sides[DIR_SOUTH_EAST]  = sides[DIR_SOUTH_EAST];
            tmpData->tiles[tmpData->numTiles].sides[DIR_SOUTH]       = sides[DIR_SOUTH];
            tmpData->tiles[tmpData->numTiles].sides[DIR_SOUTH_WEST]  = sides[DIR_SOUTH_WEST];
            tmpData->tiles[tmpData->numTiles].sides[DIR_WEST]        = sides[DIR_WEST];
            tmpData->tiles[tmpData->numTiles].sides[DIR_NORTH_WEST]  = sides[DIR_NORTH_WEST];
            tmpData->tiles[tmpData->numTiles].sides[DIR_NULL]        = sides[DIR_NULL];
        }
        //
        // trigger <checkpoint>
        //
        else if ( !N_stricmp( tok, "trigger" ) ) {
            if ( type != CHUNK_SECRET ) {
                COM_ParseError( "found parameter \"trigger\" in a chunk that isn't a secret" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for secret trigger" );
                return false;
            }
            tmpData->secrets[ tmpData->numSecrets ].trigger = atoi( tok );
        }
        //
        // pos <x y elevation>
        //
        else if ( !N_stricmp( tok, "pos" ) ) {
            uint32_t *xyz;
            if ( type == CHUNK_INVALID ) {
                COM_ParseError( "chunk type not specified before parameters" );
                return false;
            } else if ( type == CHUNK_CHECKPOINT ) {
                xyz = tmpData->checkpoints[tmpData->numCheckpoints].xyz;
            } else if ( type == CHUNK_SPAWN ) {
                xyz = tmpData->spawns[tmpData->numSpawns].xyz;
            } else if ( type == CHUNK_LIGHT ) {
                xyz = tmpData->lights[ tmpData->numLights ].origin;
            } else if ( type == CHUNK_TILE ) {
                xyz = tmpData->tiles[tmpData->numTiles].pos;
            }

            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for pos.x" );
                return false;
            }
            xyz[0] = clamp( atoi( tok ), 0, tmpData->width );

            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for pos.y" );
                return false;
            }
            xyz[1] = clamp( atoi( tok ), 0, tmpData->height );
            
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for pos.elevation" );
                return false;
            }
            xyz[2] = (uint32_t)atoi( tok );
        }
        //
        // angle <value>
        //
        else if ( !N_stricmp( tok, "angle" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"angle\" in a chunk that isn't a light" );
                return false;
            }

            tok = COM_ParseExt( text, qfalse );
            if (!tok[0]) {
                COM_ParseError( "missing parameter for angle" );
                return false;
            }
            tmpData->lights[tmpData->numLights].angle = atof( tok );
        }
        //
        // brightness <value>
        //
        else if ( !N_stricmp( tok, "brightness" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"brightness\" in a chunk that isn't a light" );
                return false;
            }

            tok = COM_ParseExt( text, qfalse );
            if (!tok[0]) {
                COM_ParseError( "missing parameter for brightness" );
                return false;
            }
            tmpData->lights[tmpData->numLights].brightness = atof( tok );
        }
        //
        // color <r g b a>
        //
        else if ( !N_stricmp( tok, "color" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"color\" in a chunk that isn't a light" );
                return false;
            }

            if ( !Parse1DMatrix( text, 4, tmpData->lights[tmpData->numLights].color ) ) {
                COM_ParseError( "failed to parse light color" );
                return false;
            }
        }
        //
        // type <light_type>
        //
        else if ( !N_stricmp( tok, "type" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"type\" in a chunk that isn't a light" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for light type" );
                return false;
            }
            tmpData->lights[tmpData->numLights].type = atoi( tok );
        }
        //
        // brightness <brightness>
        //
        else if ( !N_stricmp( tok, "brightness" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"brightness\" in a chunk that isn't a light" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if (!tok[0]) {
                COM_ParseError( "missing parameter for light brightness" );
                return false;
            }
            tmpData->lights[tmpData->numLights].brightness = atof( tok );
        }
        //
        // lightLinear <amount>
        //
        else if ( !N_stricmp( tok, "lightLinear" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"lightLinear\" in a chunk that isn't a light" );
                return false;
            }
            tok = COM_ParseExt( text, false );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for light linear" );
                return false;
            }
            tmpData->lights[ tmpData->numLights ].linear = atof( tok );
        }
        //
        // lightQuadratic <amount>
        //
        else if ( !N_stricmp( tok, "lightQuadratic" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"lightQuadratic\" in a chunk that isn't a light" );
                return false;
            }
            tok = COM_ParseExt( text, false );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for light quadratic" );
                return false;
            }
            tmpData->lights[ tmpData->numLights ].quadratic = atof( tok );
        }
        //
        // lightConstant <amount>
        //
        else if ( !N_stricmp( tok, "lightConstant" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"lightConstant\" in a chunk that isn't a light" );
                return false;
            }
            tok = COM_ParseExt( text, false );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for light constant" );
                return false;
            }
            tmpData->lights[ tmpData->numLights ].constant = atof( tok );
        }
        //
        // range <range>
        //
        else if ( !N_stricmp( tok, "range" ) ) {
            if ( type != CHUNK_LIGHT ) {
                COM_ParseError( "found parameter \"range\" in a chunk that isn't a light" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if (!tok[0]) {
                COM_ParseError( "missing parameter for light range" );
                return false;
            }
            tmpData->lights[tmpData->numLights].range = atof( tok );
        }
        //
        // bindCheckpoint <index>
        //
        else if ( !N_stricmp( tok, "bindCheckpoint" ) ) {
            if ( type != CHUNK_SPAWN ) {
                COM_ParseError( "found parameter \"bindCheckpoint\" in a chunk that isn't a spawn" );
                return false;
            }
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for spawn checkpoint binding" );
                return false;
            }
            tmpData->spawns[ tmpData->numSpawns ].checkpoint = (uint32_t)atol( tok );
        }
        else {
            COM_ParseWarning( "unrecognized token '%s'", tok );
            continue;
        }
    }
    return true;
}

static bool ParseMap(const char **text, const char *path, mapData_t *tmpData)
{
    const char *tok;

    COM_BeginParseSession(path);

    tok = COM_ParseExt(text, qtrue);
    if (tok[0] != '{') {
        COM_ParseWarning("expected '{', got '%s'", tok);
        return false;
    }

    tmpData->texcoords = s_pSpritePOD;
    tmpData->tiles = s_pTilePOD;

    while ( 1 ) {
        tok = COM_ParseComplex( text, qtrue );
        if ( !tok[0] ) {
            COM_ParseWarning("no concluding '}' in map file '%s'", path);
            return false;
        }
        // end of map file
        if (tok[0] == '}') {
            break;
        }
        // chunk definition
        else if ( tok[0] == '{' ) {
            if ( !ParseChunk( text, tmpData ) ) {
                return false;
            }
            continue;
        }
        //
        // General Map Info
        //
        else if ( !N_stricmp( tok, "map_name" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError("missing parameter for map name");
                return false;
            }
            N_strncpyz( tmpData->name, tok, sizeof(tmpData->name) );
        }
        else if ( !N_stricmp( tok, "width" ) ) {
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for map width");
                return false;
            }
            tmpData->width = (uint32_t)atoi( tok );
        }
        else if ( !N_stricmp( tok, "height" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for map height" );
                return false;
            }
            tmpData->height = (uint32_t)atoi( tok );
        }
        else if ( !N_stricmp( tok, "ambientIntensity" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for map ambient intensity" );
                return false;
            }
            tmpData->ambientIntensity = atof( tok );
        }
        else if ( !N_stricmp( tok, "ambientColor" ) ) {
            if ( !Parse1DMatrix( text, 3, tmpData->ambientColor ) ) {
                COM_ParseError( "failed to parse map ambient color" );
                return false;
            }
        }
        else if ( !N_stricmp( tok, "numTiles" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for map numTiles" );
                return false;
            }
        }
        else {
            COM_ParseWarning( "unrecognized token: '%s'", tok );
        }
    }
    return true;
}


void Map_Init( void ) {
}

void Map_LoadFile( const char *filename, bool fromCommandLine )
{
    union {
        void *v;
        char *b;
    } f;
    const char *ptr;
    const char **text, *tok;
    char path[MAX_OSPATH];
    mapData_t tmpData;

    for ( const auto& it : g_MapCache ) {
        if ( !N_stricmp( it.name, strrchr( filename, PATH_SEP ) + 1 ) ) {
            Sys_MessageBox( "Map load Failed", va( "Map file %s is already loaded", filename ), MB_OK | MB_ICONINFORMATION );
            return;
        }
    }

    snprintf( path, sizeof( path ) - 1, "%s%s%cmaps%c%s"
        , g_pProjectManager->GetProject()->m_FilePath.c_str(),
        g_pProjectManager->GetProject()->m_AssetPath.c_str(), PATH_SEP, PATH_SEP, filename );
    LoadFile( path, &f.v );

    if ( !f.v ) {
        Sys_MessageBox( "Map Load Failed", va( "Failed to open map file '%s'", path ), MB_OK | MB_ICONWARNING );
        return;
    }

    ptr = f.b;
    text = &ptr;

    memset( &tmpData, 0, sizeof(tmpData) );

    s_bLoadingMap = true;

    if ( !s_pTilePOD ) {
        s_pTilePOD = (maptile_t *)GetMemory( sizeof(maptile_t) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    }
    if ( !s_pSpritePOD ) {
        s_pSpritePOD = (spriteCoord_t *)GetMemory( sizeof(spriteCoord_t) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    }

    if ( ParseMap( text, filename, &tmpData ) ) {
        Map_Free();
        Map_New();

        Log_Printf( "Successfully loaded map '%s'\n", filename );

        //
        // copy everything
        //
        N_strncpyz( mapData->name, tmpData.name, sizeof(mapData->name) );
        mapData->width = tmpData.width;
        mapData->height = tmpData.height;
        VectorCopy( mapData->ambientColor, tmpData.ambientColor );
        mapData->ambientIntensity = tmpData.ambientIntensity;
        mapData->tiles = s_pTilePOD;
        mapData->texcoords = s_pSpritePOD;
        memcpy( mapData->checkpoints, tmpData.checkpoints, sizeof(*mapData->checkpoints) * tmpData.numCheckpoints );
        memcpy( mapData->spawns, tmpData.spawns, sizeof(*mapData->spawns) * tmpData.numSpawns );
        memcpy( mapData->lights, tmpData.lights, sizeof(*mapData->lights) * tmpData.numLights );
        memcpy( mapData->secrets, tmpData.secrets, sizeof( *mapData->secrets ) * tmpData.numSecrets );
        mapData->numCheckpoints = tmpData.numCheckpoints;
        mapData->numSpawns = tmpData.numSpawns;
        mapData->numLights = tmpData.numLights;
        mapData->numSecrets = tmpData.numSecrets;
        memcpy( &mapData->tileset, &tmpData.tileset, sizeof(mapData->tileset) );

        for ( uint32_t i = 0; i < Walnut::NUM_TEXTURE_BUNDLES; i++ ) {
            mapData->textures[Walnut::TB_DIFFUSEMAP + i] = tmpData.textures[Walnut::TB_DIFFUSEMAP + i];
        }

        Map_BuildTileset();

        if ( g_pProjectManager->IsLoaded()
            &&  std::find( g_pProjectManager->GetProject()->m_MapList.begin(),
                    g_pProjectManager->GetProject()->m_MapList.end(), mapData )
                == g_pProjectManager->GetProject()->m_MapList.end() )
        {
            g_pProjectManager->GetProject()->m_MapList.emplace_back( mapData );
        }
    } else {
        mapData = NULL;
    }

    s_bLoadingMap = true;

    FreeMemory( f.b );
}

static void Map_ArchiveLights( IDataStream *out )
{
    uint32_t i;
    char buf[1024];

    for ( i = 0; i < mapData->numLights; i++ ) {
        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"map_light\"\n"
            "\t\tpos %i %i %i\n"
            "\t\trange %f\n"
            "\t\tbrightness %f\n"
            "\t\tangle %f\n"
            "\t\tlightConstant %f\n"
            "\t\tlightLinear %f\n"
            "\t\tlightQuadratic %f\n"
            "\t\tcolor ( %f %f %f %f )\n"
            "\t\ttype %i\n"
            "\t}\n"
        , mapData->lights[i].origin[0], mapData->lights[i].origin[1], mapData->lights[i].origin[2],
        mapData->lights[i].range, mapData->lights[i].brightness,
        mapData->lights[i].angle, mapData->lights[i].constant,
        mapData->lights[i].linear, mapData->lights[i].quadratic,
        mapData->lights[i].color[0], mapData->lights[i].color[1],
        mapData->lights[i].color[2], mapData->lights[i].color[3],
        mapData->lights[i].type );
        
        out->Write( buf, strlen( buf ) );
    }
}

static void Map_ArchiveCheckpoints( IDataStream *out )
{
    uint32_t i;
    char buf[1024];

    for ( i = 0; i < mapData->numCheckpoints; i++ ) {
        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"map_checkpoint\"\n"
            "\t\tpos %i %i %i\n"
            "\t}\n"
        , mapData->checkpoints[i].xyz[0], mapData->checkpoints[i].xyz[1], mapData->checkpoints[i].xyz[2] );
        
        out->Write( buf, strlen( buf ) );
    }
}

static void Map_ArchiveSpawns( IDataStream *out )
{
    uint32_t i;
    char buf[1024];

    for ( i = 0; i < mapData->numSpawns; i++ ) {
        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"map_spawn\"\n"
            "\t\tpos %i %i %i\n"
            "\t\tid %i\n"
            "\t\tentity %i\n"
            "\t\tbindCheckpoint %u\n"
            "\t}\n"
        , mapData->spawns[i].xyz[0], mapData->spawns[i].xyz[1], mapData->spawns[i].xyz[2], mapData->spawns[i].entityid, mapData->spawns[i].entitytype,
        mapData->spawns[i].checkpoint );
        
        out->Write( buf, strlen( buf ) );
    }
}

static void Map_ArchiveTiles( IDataStream *out )
{
    uint32_t y, x;
    int i;
    char buf[1024];

    for ( i = 0; i < mapData->numTiles; i++ ) {
        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"map_tile\"\n"
            "\t\tpos %i %i %i\n"
            "\t\tflags %x\n"
            "\t\ttexIndex %i\n"
            "\t\tsides ( %i %i %i %i %i %i %i %i %i )\n"
            "\t}\n"
        , mapData->tiles[i].pos[0], mapData->tiles[i].pos[1], mapData->tiles[i].pos[2],
        mapData->tiles[i].flags,
        mapData->tiles[i].index,
        mapData->tiles[i].sides[DIR_NORTH],
        mapData->tiles[i].sides[DIR_NORTH_EAST],
        mapData->tiles[i].sides[DIR_EAST],
        mapData->tiles[i].sides[DIR_SOUTH_EAST],
        mapData->tiles[i].sides[DIR_SOUTH],
        mapData->tiles[i].sides[DIR_SOUTH_WEST],
        mapData->tiles[i].sides[DIR_WEST],
        mapData->tiles[i].sides[DIR_NORTH_WEST],
        mapData->tiles[i].sides[DIR_NULL] );

        out->Write( buf, strlen( buf ) );
    }
}

static void Map_ArchiveSecrets( IDataStream *out )
{
    int i;
    char buf[1024];

    for ( i = 0; i < mapData->numSecrets; i++ ) {
        snprintf( buf, sizeof( buf ) - 1,
            "\t{\n"
            "\t\tclassname \"map_secret\"\n"
            "\t\ttrigger %u\n"
            "\t}\n"
        , mapData->secrets[i].trigger );

        out->Write( buf, strlen( buf ) );
    }
}

void ProjectSaveEntityIds( void )
{
    std::ofstream out;
    char outfile[MAX_OSPATH];
    uint64_t i;
    nlohmann::json entity;
    nlohmann::json data;

    snprintf( outfile, sizeof( outfile ), "%s%s%cscripts%centitydata.json", g_pProjectManager->GetProject()->m_FilePath.c_str(), g_pProjectManager->GetProject()->m_AssetPath.c_str(),
        PATH_SEP, PATH_SEP );

    Log_Printf( "ProjectSaveEntityIds: archiving entitydata file '%s'...\n", outfile );

    out.open( outfile, std::ios::out );
    if ( !out.is_open() ) {
        Error( "ProjectSaveEntityIds: failed to create entitydata file '%s'", outfile );
    }

    for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_MOB ] ) {
        entity[ "Name" ] = it.m_Name;
        entity[ "Id" ] = it.m_Id;
        data[ "MobData" ].emplace_back( entity );
    }
    for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_ITEM ] ) {
        entity[ "Name" ] = it.m_Name;
        entity[ "Id" ] = it.m_Id;
        data[ "ItemData" ].emplace_back( entity );
    }
    for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[ ET_WEAPON ] ) {
        entity[ "Name" ] = it.m_Name;
        entity[ "Id" ] = it.m_Id;
        data[ "WeaponData" ].emplace_back( entity );
    }

    out.width( 4 );
    out << data;
}

void Map_Save( void )
{
    FileStream out;
    char outfile[MAX_OSPATH];

    snprintf( outfile, sizeof( outfile ), "%s%s%cmaps%c%s", g_pProjectManager->GetProject()->m_FilePath.c_str(), g_pProjectManager->GetProject()->m_AssetPath.c_str(),
        PATH_SEP, PATH_SEP, mapData->name );

    Log_Printf( "Map_Save: archiving map file '%s'...\n", outfile );

    if ( !out.Open( outfile, "w" ) ) {
        Error( "Map_Save: failed to create map file '%s'", outfile );
    }

    {
        char buf[2048];

        sprintf( buf,
            "{\n"
            "\tmap_name \"%s\"\n"
            "\twidth %i\n"
            "\theight %i\n"
            "\tnumTiles %i\n"
            "\tambientColor ( %0.3f %0.3f %0.3f )\n"
            "\tambientIntensity %f\n"
        , mapData->name, mapData->width, mapData->height, mapData->numTiles,
        mapData->ambientColor[0], mapData->ambientColor[1], mapData->ambientColor[2],
        mapData->ambientIntensity );

        out.Write( buf, strlen( buf ) );

        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"tilesetdata\"\n"
            "\t\tshader \"%s\"\n"
            "\t\tdiffuseMap \"%s\"\n"
            "\t\tspecularMap \"%s\"\n"
            "\t\tlightMap \"%s\"\n"
            "\t\tnormalMap \"%s\"\n"
            "\t\tshadowMap \"%s\"\n"
            "\t\ttileWidth %u\n"
            "\t\ttileHeight %u\n"
            "\t\tnumTiles %u\n"
            "\t}\n"
        , mapData->tileset.texture,
        mapData->textures[Walnut::TB_DIFFUSEMAP] ? mapData->textures[Walnut::TB_DIFFUSEMAP]->GetName().c_str() : " ",
        mapData->textures[Walnut::TB_SPECULARMAP] ? mapData->textures[Walnut::TB_SPECULARMAP]->GetName().c_str() : " ",
        mapData->textures[Walnut::TB_LIGHTMAP] ? mapData->textures[Walnut::TB_LIGHTMAP]->GetName().c_str() : " ",
        mapData->textures[Walnut::TB_NORMALMAP] ? mapData->textures[Walnut::TB_NORMALMAP]->GetName().c_str() : " ",
        mapData->textures[Walnut::TB_SHADOWMAP] ? mapData->textures[Walnut::TB_SHADOWMAP]->GetName().c_str() : " ",
        mapData->tileset.tileWidth, mapData->tileset.tileHeight, mapData->tileset.numTiles );

        out.Write( buf, strlen( buf ) );
    }

    for ( int y = 0; y < mapData->width; y++ ) {
        for ( int x = 0; x < mapData->height; x++ ) {
            mapData->tiles[ y * mapData->width + x ].pos[0] = x;
            mapData->tiles[ y * mapData->width + x ].pos[1] = y;
        }
    }

    g_pEditor->m_nOldMapHeight = mapData->height;
    g_pEditor->m_nOldMapWidth = mapData->width;

    Map_ArchiveCheckpoints( &out );
    Map_ArchiveSpawns( &out );
    Map_ArchiveTiles( &out );
    Map_ArchiveLights( &out );
    Map_ArchiveSecrets( &out );

    out.Write( "}\n", 2 );

    out.Close();

    if ( std::find( g_pProjectManager->GetProject()->m_MapList.begin(),
        g_pProjectManager->GetProject()->m_MapList.end(), mapData ) == g_pProjectManager->GetProject()->m_MapList.end() )
    {
        g_pProjectManager->GetProject()->m_MapList.emplace_back( mapData );
    }

    // update the title to signal saved
    Sys_SetWindowTitle( mapData->name );

    g_pMapInfoDlg->m_bMapModified = false;
    g_pMapInfoDlg->m_bMapNameUpdated = false;
}

void Map_New( void )
{
    uint32_t y, x;

    Map_Free();

    mapData = std::addressof( g_MapCache.emplace_back() );

    memset( mapData, 0, sizeof(*mapData) );

    mapData->width = 64;
    mapData->height = 64;

    if ( !s_pTilePOD ) {
        s_pTilePOD = (maptile_t *)GetMemory( sizeof( maptile_t ) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    }
    if ( !s_pSpritePOD ) {
        s_pSpritePOD = (spriteCoord_t *)GetMemory( sizeof( spriteCoord_t ) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    }
    mapData->tiles = s_pTilePOD;
    mapData->texcoords = s_pSpritePOD;

    for ( y = 0; y < MAX_MAP_HEIGHT; y++ ) {
        for ( x = 0; x < MAX_MAP_WIDTH; x++ ) {
            mapData->tiles[y * MAX_MAP_WIDTH + x].pos[0] = x;
            mapData->tiles[y * MAX_MAP_WIDTH + x].pos[1] = y;
        }
    }

    mapData->ambientColor[0] = 0.0f;
    mapData->ambientColor[1] = 0.0f;
    mapData->ambientColor[2] = 0.0f;

    g_pEditor->m_nOldMapHeight = mapData->height;
    g_pEditor->m_nOldMapWidth = mapData->width;

    strcpy( mapData->name, unnamed_map );

    if ( !g_pProjectManager->IsLoaded() ) {
        g_pProjectManager->GetProject()->m_MapList.emplace_back( mapData );
    }

    if ( g_pMapInfoDlg ) {
        g_pMapInfoDlg->SetCurrent( mapData );
    }

    if ( g_pApplication ) {
        Sys_SetWindowTitle( mapData->name );
    }
}

void Map_Free( void )
{
    extern bool g_ApplicationRunning;
    if ( !mapData || !g_ApplicationRunning ) {
        return;
    }
    memset( mapData->tiles, 0, sizeof(*mapData->tiles) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    memset( mapData->texcoords, 0, sizeof(*mapData->texcoords) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    mapData = NULL;
}

void Map_ImportFile( const char *filename )
{
    Map_LoadFile( filename );
}

static void Map_GenerateShader( void )
{
    char buf[1024];
    char tmp[MAX_NPATH];
    FileStream file;
    std::string path;

    path = g_pProjectManager->GetProject()->m_FilePath + g_pProjectManager->GetProject()->m_AssetPath + va( "%cscripts%c%s", PATH_SEP, PATH_SEP, mapData->name );

    if ( !file.Open( path.c_str(), "w" ) ) {
        Error( "Failed to create shader file '%s'", path.c_str() );
    }

    COM_StripExtension( mapData->textures[Walnut::TB_DIFFUSEMAP]->GetName().c_str(), tmp, sizeof(tmp) );

    snprintf( buf, sizeof(buf),
        "%s\n"
        "{\n"
        "\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
        "\trgbGen vertex\n"
    , tmp );

    file.Write( buf, strlen( buf ) );

    file.Close();
}

void Map_BuildTileset( void )
{
    uint32_t y, x;

    if ( !mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
        Sys_MessageBox( "Tileset Error", "You must provide at least a diffuse texture map to build a tileset", MB_OK );
        g_pMapInfoDlg->m_bTilesetModified = true;
        return;
    }
    if ( !mapData->tileset.tileWidth ) {
        Sys_MessageBox( "Tileset Error", "Invalid tile width", MB_OK );
        g_pMapInfoDlg->m_bTilesetModified = true;
        return;
    }
    if ( !mapData->tileset.tileHeight ) {
        Sys_MessageBox( "Tileset Error", "Invalid tile height", MB_OK );
        g_pMapInfoDlg->m_bTilesetModified = true;
        return;
    }

    mapData->textureWidth = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetWidth();
    mapData->textureHeight = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetHeight();
    mapData->tileset.tileCountX = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetWidth() / mapData->tileset.tileWidth;
    mapData->tileset.tileCountY = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetHeight() / mapData->tileset.tileHeight;

    auto genCoords = [&](const glm::vec2& sheetDims, const glm::vec2& spriteDims, const glm::vec2& coords, spriteCoord_t *texcoords) {
        const glm::vec2 min = { ( ( coords.x + 1 ) * spriteDims.x ) / sheetDims.x, ( ( coords.y + 1 ) * spriteDims.y ) / sheetDims.y };
        const glm::vec2 max = { ( coords.x * spriteDims.x ) / sheetDims.x, ( coords.y * spriteDims.y ) / sheetDims.y };

        (*texcoords)[0][0] = min.x;
        (*texcoords)[0][1] = max.y;

        (*texcoords)[1][0] = min.x;
        (*texcoords)[1][1] = min.y;

        (*texcoords)[2][0] = max.x;
        (*texcoords)[2][1] = min.y;
        
        (*texcoords)[3][0] = max.x;
        (*texcoords)[3][1] = max.y;
    };

    mapData->tileset.numTiles = mapData->tileset.tileCountX * mapData->tileset.tileCountY;
    
    for ( y = 0; y < mapData->tileset.tileCountY; y++ ) {
        for ( x = 0; x < mapData->tileset.tileCountX; x++ ) {
            genCoords( { mapData->textures[Walnut::TB_DIFFUSEMAP]->GetWidth(), mapData->textures[Walnut::TB_DIFFUSEMAP]->GetHeight() },
                    { mapData->tileset.tileWidth, mapData->tileset.tileHeight }, { x, y },
                    &mapData->texcoords[y * mapData->tileset.tileCountX + x] );
        }
    }
}

void Map_SaveSelected( const char *filename );

void Map_Import( IDataStream *in, const char *type );
void Map_Export( IDataStream *out, const char *type );








