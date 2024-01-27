#include "editor.h"

#include <glm/glm.hpp>

mapData_t *mapData;
std::unordered_map<std::string, mapData_t> g_MapCache;
static const char *unnamed_map = "unnamed.map";
static tile2d_sprite_t *s_pSpritePOD;
static maptile_t *s_pTilePOD;

typedef enum {
    CHUNK_CHECKPOINT,
    CHUNK_SPAWN,
    CHUNK_LIGHT,
    CHUNK_TILE,
    CHUNK_TILESET,
    CHUNK_TEXCOORDS,
    
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
                tmpData->numCheckpoints++;
                type = CHUNK_CHECKPOINT;
            }
            else if ( !N_stricmp( tok, "map_spawn" ) ) {
                tmpData->numSpawns++;
                type = CHUNK_SPAWN;
            }
            else if ( !N_stricmp(tok, "map_light" ) ) {
                tmpData->numLights++;
                type = CHUNK_LIGHT;
            }
            else if ( !N_stricmp( tok, "texcoords" ) ) {
                tmpData->tileset.numTiles++;
                type = CHUNK_TEXCOORDS;
            }
            else if ( !N_stricmp( tok, "map_tile" ) ) {
                tmpData->numTiles++;
                type = CHUNK_TILE;
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
            mapData->tileset.numTiles = (uint32_t)atoi( tok );
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
            mapData->tileset.tileWidth = (uint32_t)atoi( tok );
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
            mapData->shader = Walnut::R_FindShader( tok );
            if ( !mapData->shader ) {
                COM_ParseWarning( "invalid shader in tileset" );
                continue;
            }
            N_strncpyz( mapData->tileset.texture, tok, sizeof(mapData->tileset.texture) );
        }
        //
        // tileHeight <height>
        //
        else if (!N_stricmp(tok, "tileHeight")) {
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for tileset tileHeight");
                return false;
            }
            tmpData->tileset.tileHeight = (uint32_t)atoi(tok);
        }
        //
        // texIndex <index>
        //
        else if (!N_stricmp(tok, "texIndex")) {
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for map tile texIndex");
                return false;
            }
            tmpData->tiles[tmpData->numTiles].index = (int32_t)atoi( tok );
        }
        //
        // id <entityid>
        //
        else if (!N_stricmp(tok, "id")) {
            if (type != CHUNK_SPAWN) {
                COM_ParseError("found parameter \"id\" in chunk that isn't a spawn");
                return false;
            }
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for spawn entity id");
                return false;
            }
            tmpData->spawns[tmpData->numSpawns].entityid = atoi( tok );

            bool valid = false;
            for ( const auto& it : g_pProjectManager->GetProject()->m_EntityList[tmpData->spawns[tmpData->numSpawns].entitytype] ) {
                if ( it.m_Id == (uint32_t)tmpData->spawns[tmpData->numSpawns].entityid ) {
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
        else if (!N_stricmp(tok, "flags")) {
            if (type != CHUNK_TILE) {
                COM_ParseError("found parameter \"flags\" in chunk that isn't a tile");
                return false;
            }
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for tile flags");
                return false;
            }
            tmpData->tiles[tmpData->numTiles].flags = (uint32_t)ParseHex( tok );
        }
        //
        // sides <sides...>
        //
        else if ( !N_stricmp( tok, "sides" ) ) {
            int sides[5];
            if ( !Parse1DMatrix( text, 5, (float *)sides ) ) {
                COM_ParseError("failed to parse sides for map tile");
                return false;
            }
            tmpData->tiles[tmpData->numTiles].sides[0] = sides[0];
            tmpData->tiles[tmpData->numTiles].sides[1] = sides[1];
            tmpData->tiles[tmpData->numTiles].sides[2] = sides[2];
            tmpData->tiles[tmpData->numTiles].sides[3] = sides[3];
            tmpData->tiles[tmpData->numTiles].sides[4] = sides[4];
        }
        //
        // texcoords <texcoords...>
        //
        else if ( !N_stricmp( tok, "texcoords" ) ) {
            float coords[4 * 2];
            if (!Parse2DMatrix(text, 4, 2, coords)) {
                COM_ParseError("failed to parse texture coordinates for map tile");
                return false;
            }
            memcpy(tmpData->texcoords[tmpData->tileset.numTiles].uv, coords, sizeof(coords));
        }
        //
        // pos <x y elevation>
        //
        else if (!N_stricmp(tok, "pos")) {
            uint32_t *xyz;
            if (type == CHUNK_INVALID) {
                COM_ParseError("chunk type not specified before parameters");
                return false;
            } else if (type == CHUNK_CHECKPOINT) {
                xyz = tmpData->checkpoints[tmpData->numCheckpoints].xyz;
            } else if (type == CHUNK_SPAWN) {
                xyz = tmpData->spawns[tmpData->numSpawns].xyz;
            }

            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for pos.x");
                return false;
            }
            xyz[0] = clamp( atoi( tok ), 0, tmpData->width );

            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for pos.y");
                return false;
            }
            xyz[1] = clamp( atoi( tok ), 0, tmpData->height );
            
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for pos.elevation");
                return false;
            }
            xyz[2] = (uint32_t)atoi( tok );
        }
        //
        // brightness <value>
        //
        else if ( !N_stricmp( tok, "brightness" ) ) {
            if (type != CHUNK_LIGHT) {
                COM_ParseError("found parameter \"brightness\" in chunk that isn't a light");
                return false;
            }

            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for brightness");
                return false;
            }
            tmpData->lights[tmpData->numLights].brightness = atof( tok );
        }
        //
        // color <r g b a>
        //
        else if ( !N_stricmp( tok, "color" ) ) {
            if (type != CHUNK_LIGHT) {
                COM_ParseError("found parameter \"color\" in chunk that isn't a light");
                return false;
            }

            if ( !Parse1DMatrix( text, 3, tmpData->lights[tmpData->numLights].color ) ) {
                COM_ParseError("failed to parse light color");
                return false;
            }
        }
        //
        // range <range>
        //
        else if ( !N_stricmp( tok, "range" ) ) {
            tok = COM_ParseExt(text, qfalse);
            if (!tok[0]) {
                COM_ParseError("missing parameter for light range");
                return false;
            }
            tmpData->lights[tmpData->numLights].range = atof( tok );
        }
        //
        // index <texCoordIndex>
        //
        else if ( !N_stricmp( tok, "index" ) ) {
            tok = COM_ParseExt( text, qfalse );
            if ( !tok[0] ) {
                COM_ParseError( "missing parameter for texcoords \"index\"" );
                return false;
            }
            const int index = atoi( tok );
            for ( uint32_t i = 0; i < tmpData->numTiles; i++ ) {
                if ( tmpData->tiles[i].index == index ) {

                }
            }
        }
        //
        // origin <x y elevation>
        //
        else if (!N_stricmp(tok, "origin")) {
            if (type != CHUNK_LIGHT) {
                COM_ParseError("found parameter \"origin\" in chunk that isn't a light");
                return false;
            }

            uvec3_t origin;
            if (!Parse1DMatrix(text, 3, (float *)origin)) {
                COM_ParseError("failed to parse light origin");
                return false;
            }
            VectorCopy( tmpData->lights[tmpData->numLights].origin, origin );
        }
        else {
            COM_ParseWarning("unrecognized token '%s'", tok);
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
            tmpData->tiles = (maptile_t *)alloca( sizeof(maptile_t) * atoi( tok ) );
        }
        else {
            COM_ParseWarning("unrecognized token: '%s'", tok);
        }
    }
    return true;
}


void Map_Init( void )
{
    Map_New();
}

void Map_LoadFile( const char *filename )
{
    union {
        void *v;
        char *b;
    } f;
    const char *ptr;
    const char **text, *tok;
    mapData_t tmpData;

    LoadFile( filename, &f.v );

    if ( !f.v ) {
        Sys_MessageBox( "Map Load Failed", va( "Failed to open map file '%s'", filename ), MB_OK | MB_ICONWARNING );
        return;
    }

    ptr = f.b;
    text = &ptr;

    if ( ParseMap( text, filename, &tmpData ) ) {
        Map_Free();

        Log_Printf( "Successfully loaded map '%s'\n", filename );

        Map_BuildTileset();

        memcpy( mapData, &tmpData, sizeof(*mapData) );
        mapData->texcoords = (tile2d_sprite_t *)GetMemory( sizeof(tile2d_sprite_t) * mapData->tileset.numTiles );
        mapData->tiles = (maptile_t *)GetMemory( sizeof(maptile_t) * mapData->numTiles );
    }

    FreeMemory( f.b );
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
            "\t}\n"
        , mapData->spawns[i].xyz[0], mapData->spawns[i].xyz[1], mapData->spawns[i].xyz[2], mapData->spawns[i].entityid, mapData->spawns[i].entitytype );
        
        out->Write( buf, strlen( buf ) );
    }
}

static void Map_ArchiveTiles( IDataStream *out )
{
    uint32_t y, x;
    uint32_t i;
    char buf[1024];

    for ( i = 0; mapData->tileset.numTiles; i++ ) {
        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"texcoords\"\n"
            "\t\tindex %i\n"
            "\t\tcoords ( ( %f %f ) ( %f %f ) ( %f %f ) ( %f %f ) )\n"
            "\t}\n"
        , mapData->texcoords[i].index,
            mapData->texcoords[i].uv[0][0], mapData->texcoords[i].uv[0][1],
            mapData->texcoords[i].uv[1][0], mapData->texcoords[i].uv[1][1],
            mapData->texcoords[i].uv[2][0], mapData->texcoords[i].uv[2][1],
            mapData->texcoords[i].uv[3][0], mapData->texcoords[i].uv[3][1] );
        
        out->Write( buf, strlen( buf ) );
    }

    for ( y = 0; y < mapData->height; y++ ) {
        for ( x = 0; x < mapData->width; x++ ) {
            sprintf( buf,
                "\t{\n"
                "\t\tclassname \"map_tile\"\n"
                "\t\tpos %i %i %i\n"
                "\t\ttexIndex %i\n"
                "\t}\n"
            , y, x, 0, mapData->tiles[y * mapData->width + x].index );

            out->Write( buf, strlen( buf ) );
        }
    }
}

void Map_Save( void )
{
    FileStream out;
    std::string outfile;

    outfile = g_pProjectManager->GetProject()->m_FilePath + g_pProjectManager->GetProject()->m_AssetPath + va( "%cmaps%c%s", PATH_SEP, PATH_SEP, mapData->name );

    Log_Printf( "Map_Save: arching map file '%s'...\n", outfile.c_str() );

    if ( !out.Open( outfile.c_str(), "w" ) ) {
        Error( "Map_Save: failed to create map file '%s'", outfile.c_str() );
    }

    {
        char buf[1024];

        sprintf( buf,
            "{\n"
            "\tmap_name \"%s\"\n"
            "\twidth %i\n"
            "\theight %i\n"
            "\tnumTiles %i\n"
        , mapData->name, mapData->width, mapData->height, mapData->numTiles );

        out.Write( buf, strlen( buf ) );

        sprintf( buf,
            "\t{\n"
            "\t\tclassname \"tilesetdata\"\n"
            "\t\tshader \"%s\"\n"
            "\t\ttileWidth %i\n"
            "\t\ttileHeight %i\n"
            "\t}\n"
        , mapData->tileset.texture, mapData->tileset.tileWidth, mapData->tileset.tileHeight );

        out.Write( buf, strlen( buf ) );
    }

    g_pEditor->m_nOldMapHeight = mapData->height;
    g_pEditor->m_nOldMapWidth = mapData->width;

    Map_ArchiveCheckpoints( &out );
    Map_ArchiveSpawns( &out );

    out.Write( "}\n", 2 );

    out.Close();

    g_pProjectManager->GetProject()->m_MapList.emplace_back( mapData );

    // update the title to signal saved
    Sys_SetWindowTitle( mapData->name );

    g_pMapInfoDlg->m_bMapModified = false;
    g_pMapInfoDlg->m_bMapNameUpdated = false;
}

void Map_New( void )
{
    Map_Free();

    if ( g_MapCache.find( "untitled" ) != g_MapCache.end() ) {
        g_MapCache.erase( "untitled" );
    }

    g_MapCache.try_emplace( "untitled" );
    mapData = std::addressof( g_MapCache["untitled"] );

    memset( mapData, 0, sizeof(*mapData) );

    mapData->width = 64;
    mapData->height = 64;

    if ( !s_pTilePOD ) {
        s_pTilePOD = (maptile_t *)GetMemory( sizeof(maptile_t) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    }
    if ( !s_pSpritePOD ) {
        s_pSpritePOD = (tile2d_sprite_t *)GetMemory( sizeof(tile2d_sprite_t) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT );
    }
    mapData->tiles = s_pTilePOD;
    mapData->texcoords = s_pSpritePOD;

    for ( uint32_t y = 0; y < MAX_MAP_HEIGHT; y++ ) {
        for ( uint32_t x = 0; x < MAX_MAP_WIDTH; x++ ) {
            mapData->tiles[y * MAX_MAP_WIDTH + x].index = 0;
            mapData->tiles[y * MAX_MAP_WIDTH + x].pos[0] = x;
            mapData->tiles[y * MAX_MAP_WIDTH + x].pos[1] = y;

            mapData->texcoords[y * MAX_MAP_WIDTH + x].index = 0;
        }
    }

    mapData->ambientColor[0] = 0.0f;
    mapData->ambientColor[1] = 0.0f;
    mapData->ambientColor[2] = 0.0f;

    g_pEditor->m_nOldMapHeight = mapData->height;
    g_pEditor->m_nOldMapWidth = mapData->width;

    if ( g_pMapInfoDlg ) {
        g_pMapInfoDlg->SetCurrent( mapData );
    }

    strcpy( mapData->name, unnamed_map );

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
        "%s {\n"
        "\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
        "\trgbGen vertex\n"
    , tmp );

    file.Write( buf, strlen( buf ) );

    //
//    // diffuse stage
//    //
//    snprintf( buf, sizeof(buf),
//        "\t{\n"
//        "\t\tstage diffuseMap\n"
//        "\t\tspecularReflectance %f\n"
//        "\t\tgloss %f\n"
//        "\t\troughness %f\n"
//        "\t\tnormalScale %f %f\n"
//        "\t\tspecularScale %f %f %f %f\n"
//        "\t}\n");

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

    const uint32_t tileCountX = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetWidth() / mapData->tileset.tileWidth;
    const uint32_t tileCountY = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetHeight() / mapData->tileset.tileHeight;

    auto genCoords = [&](const glm::vec2& sheetDims, const glm::vec2& spriteDims, const glm::vec2& coords, vec2_t texcoords[4]) {
        const glm::vec2 min = { ( ( coords.x + 1 ) * spriteDims.x ) / sheetDims.x, ( ( coords.y + 1 ) * spriteDims.y ) / sheetDims.y };
        const glm::vec2 max = { ( coords.x * spriteDims.x ) / sheetDims.x, ( coords.y * spriteDims.y ) / sheetDims.y };

        texcoords[0][0] = min.x;
        texcoords[0][1] = max.y;

        texcoords[1][0] = min.x;
        texcoords[1][1] = min.y;

        texcoords[2][0] = max.x;
        texcoords[2][1] = min.y;
        
        texcoords[3][0] = max.x;
        texcoords[3][1] = max.y;
    };

    mapData->tileset.numTiles = tileCountX * tileCountY;
    
    for ( y = 0; y < tileCountY; y++ ) {
        for ( x = 0; x < tileCountX; x++ ) {
            mapData->texcoords[y * tileCountX + x].index = y * tileCountX + x;
            genCoords( { mapData->textures[Walnut::TB_DIFFUSEMAP]->GetWidth(), mapData->textures[Walnut::TB_DIFFUSEMAP]->GetHeight() },
                    { mapData->tileset.tileWidth, mapData->tileset.tileHeight }, { x, y }, mapData->texcoords[y * tileCountX + x].uv );
        }
    }
}

void Map_SaveSelected( const char *filename );

void Map_Import( IDataStream *in, const char *type );
void Map_Export( IDataStream *out, const char *type );







