#include "gln.h"
#include "editor.h"
#include "project.h"

CProjectManager *g_pProjectManager;

CProjectManager::CProjectManager( void )
{
    char filePath[MAX_OSPATH*2+1];
    
    m_bLoaded = false;
    m_ProjectsDirectory =  g_pEditor->m_CurrentPath + g_pPrefsDlg->m_ProjectDataPath;
    
    for ( const auto& directoryEntry : std::filesystem::directory_iterator{ m_ProjectsDirectory } ) {
        std::string path = directoryEntry.path().string();

        // is it a project directory?
        if ( directoryEntry.is_directory() && !N_stricmp( COM_GetExtension( path.c_str() ), "proj" ) ) {
            AddToCache( path, true );
        }
    }

    m_bLoaded = true;
}

CProjectManager::~CProjectManager()
{
    Save();
}

static void MakeAssetDirectoryPath()
{

}

void CProjectManager::InitProjectConfig( const char *filepath ) const
{
    json config;
    const char *path;

    path = va( "%s%cConfig%cconfig.json", filepath, PATH_SEP, PATH_SEP );
    std::ofstream file( path );
    if ( !file.is_open() ) {
        Error( "[CProjectManager::New] failed to create project configuration file '%s'", path );
    }

    config["name"] = "untitled";
    config["assetdirectory"] = va( "%s%cAssets", filepath, PATH_SEP );

    file.width( 4 );
    file << config;
}

void CProjectManager::AddToCache( const std::string& path, bool loadJSON, bool buildPath )
{
    std::string filePath;
    json data;
    std::shared_ptr<Project> proj;

    Log_Printf( "[CProjectManager::AddToCache] adding project directory '%s' to project cache\n", path.c_str() );

    proj = std::make_shared<Project>();

    proj->m_FilePath = va( "%s%c", path.c_str(), PATH_SEP );
    m_ProjList[path] = proj;

    if ( loadJSON ) {
        const char *ospath = va( "%s%cConfig%cconfig.json", path.c_str(), PATH_SEP, PATH_SEP );

        std::ifstream file( ospath, std::ios::in );
        if ( !file.is_open() ) {
            Log_Printf( "[CProjectManager::AddToCache] failed to open project config file '%s', creating default...\n", ospath );
            InitProjectConfig( path.c_str() );

            proj->m_Name = "untitled";
            proj->m_AssetDirectory = va( "%s%cAssets", path.c_str(), PATH_SEP );
            proj->m_AssetPath = "Assets";

            return;
        }

        try {
            data = json::parse( file );
        } catch ( const json::exception& e ) {
            Log_Printf( "[CProjectManager::AddToCache] failed to load project config file '%s', nlohmann::json error ->\n    id: %i\n    what: %s\n",
                e.id, e.what() );
            return;
        }
        file.close();

        proj->m_Name = data["name"];
        proj->m_AssetDirectory = va( "%s%c%s", path.c_str(), PATH_SEP, data["assetdirectory"].get<std::string>().c_str() );
        proj->m_AssetPath = data["assetdirectory"];

        if ( data.contains( "data_lists" ) ) {
            const std::vector<json>& itemlist = data["data_lists"]["itemlist"];
            const std::vector<json>& moblist = data["data_lists"]["moblist"];
            const std::vector<json>& botlist = data["data_lists"]["botlist"];

            proj->m_EntityList[ ET_ITEM ].reserve( itemlist.size() );
            proj->m_EntityList[ ET_MOB ].reserve( moblist.size() );
            proj->m_EntityList[ ET_BOT ].reserve( botlist.size() );

            for ( const auto& it : itemlist ) {
                proj->m_EntityList[ ET_ITEM ].emplace_back( it.at( "name" ).get<std::string>().c_str(), it.at( "id" ) );
            }
            for ( const auto& it : moblist ) {
                proj->m_EntityList[ ET_MOB ].emplace_back( it.at( "name" ).get<std::string>().c_str(), it.at( "id" ) );
            }
            for ( const auto& it : botlist ) {
                proj->m_EntityList[ ET_BOT ].emplace_back( it.at( "name" ).get<std::string>().c_str(), it.at( "id" ) );
            }
        }
        if ( data.contains( "mobtypes" ) ) {
            proj->m_MobTypes = data["mobtypes"];
        }
    } else {
        proj->m_Name = "untitled";
        proj->m_AssetDirectory = va( "%s%cAssets", path.c_str(), PATH_SEP );
        proj->m_AssetPath = "Assets";
    }
    try {
        for ( const auto& mapIterator : std::filesystem::directory_iterator{ proj->m_AssetDirectory } ) {
            filePath = mapIterator.path().string();

            if ( mapIterator.is_regular_file() && ( !N_stricmp( COM_GetExtension( filePath.c_str() ), "map" )
                || !N_stricmp( COM_GetExtension( filePath.c_str() ), "bmf") ) )
            {
                Map_LoadFile( filePath.c_str() );
                if ( !m_bLoaded ) {
                    proj->m_MapList.emplace_back( std::addressof( g_MapCache.back() ) );
                }
            }
        }
    } catch ( const std::filesystem::filesystem_error& e ) {
        Log_FPrintf( SYS_WRN, "[CProjectManager::AddToCache] Filesystem exception occured, what: %s\n", e.what() );
    }
}

void CProjectManager::New( void )
{
    int count;
    const char *path;
    char filePath[MAX_OSPATH*2+1];

    for ( count = 0; FolderExists( ( path = va( "%s%sproject-%i.proj%c", g_pEditor->m_CurrentPath.c_str(), g_pPrefsDlg->m_ProjectDataPath.c_str(),
    count, PATH_SEP ) ) ); count++ )
        ;
    
    //
    // create directories
    //
    if ( !Q_mkdir( path ) ) {
        Error( "[CProjectManager::New] failed to create project directory '%s'", path );
    }
    if ( !Q_mkdir( va( "%s%cConfig", path, PATH_SEP ) ) ) {
        Error( "[CProjectManager::New] failed to create project directory '%s%cConfig'", path, PATH_SEP );
    }
    if ( !Q_mkdir( va( "%s%cAssets", path, PATH_SEP ) ) ) {
        Error( "[CProjectManager::New] failed to create project directory '%s%cAssets'", path, PATH_SEP );
    }
    if ( !Q_mkdir( va( "%s%cAssets%cshaders", path, PATH_SEP, PATH_SEP ) ) ) {
        Error( "[CProjectManager::New] failed to create project directory '%s%cAssets%cshaders'", path, PATH_SEP, PATH_SEP );
    }
    if ( !Q_mkdir( va( "%s%cAssets%cmaps", path, PATH_SEP, PATH_SEP ) ) ) {
        Error( "[CProjectManager::New] failed to create project directory '%s%cAssets%cshaders'", path, PATH_SEP, PATH_SEP );
    }
    AddToCache( path );
    m_CurrentProject = m_ProjList.find( path )->second;

    InitProjectConfig( path );
}

void CProjectManager::Save( void ) const
{
    const char *ospath;
    std::vector<json> itemlist;
    std::vector<json> moblist;
    std::vector<json> botlist;
    std::vector<json> mobtypes;
    uint32_t i;

    ospath = BuildOSPath( m_CurrentProject->m_FilePath.c_str(), "Config", "config.json" );

    Log_Printf( "[CProjectManager::Save] Archiving project '%s' data to '%s'...\n", m_CurrentProject->m_Name.c_str(), ospath );

    std::ofstream file( ospath, std::ios::out );
    json data;

    if ( !file.is_open() ) {
        Error( "[CProjectManager::Save] Failed to save project file '%s'", ospath );
    }

    data["name"] = m_CurrentProject->m_Name;
    data["assetdirectory"] = strrchr( m_CurrentProject->m_AssetDirectory.string().c_str(), PATH_SEP );

    for ( const auto& it : m_CurrentProject->m_MapList ) {
        data["maplist"].emplace_back( it->name );
    }

    itemlist.reserve( m_CurrentProject->m_EntityList[ ET_ITEM ].size() );
    for ( i = 0; i < m_CurrentProject->m_EntityList[ ET_ITEM ].size(); i++ ) {
        json& it = itemlist.emplace_back();

        it["id"] = m_CurrentProject->m_EntityList[ ET_ITEM ][i].m_Id;
        it["name"] = m_CurrentProject->m_EntityList[ ET_ITEM ][i].m_Name;
    }

    moblist.reserve( m_CurrentProject->m_EntityList[ ET_MOB ].size() );
    for ( i = 0; i < m_CurrentProject->m_EntityList[ ET_MOB ].size(); i++ ) {
        json& it = moblist.emplace_back();

        it["id"] = m_CurrentProject->m_EntityList[ ET_MOB ][i].m_Id;
        it["name"] = m_CurrentProject->m_EntityList[ ET_MOB ][i].m_Name;
    }

    botlist.reserve( m_CurrentProject->m_EntityList[ ET_BOT ].size() );
    for ( i = 0; i < m_CurrentProject->m_EntityList[ ET_BOT ].size(); i++ ) {
        json& it = botlist.emplace_back();

        it["id"] = m_CurrentProject->m_EntityList[ ET_BOT ][i].m_Id;
        it["name"] = m_CurrentProject->m_EntityList[ ET_BOT ][i].m_Name;
    }

    data["data_lists"]["itemlist"] = itemlist;
    data["data_lists"]["moblist"] = moblist;
    data["data_lists"]["botlist"] = botlist;

    for ( const auto& it : m_CurrentProject->m_MobTypes ) {
        data["mobtypes"][it.first] = it.second;
    }

    file.width( 4 );
    file << data;
}

void CProjectManager::SetCurrent( const std::string& name, bool buildPath )
{
    char filePath[MAX_OSPATH*2+1];
    const char *ospath;
    const char *ext = COM_GetExtension( name.c_str() );
    char *path = strdupa( name.c_str() );

    if ( N_stricmp( ext, "proj" ) && *ext != '\0' ) {
        COM_StripExtension( name.c_str(), path, name.size() );
    } else {
        ext = NULL;
    }

    // create a proper ospath is if isn't one
    if ( buildPath ) {
        if ( name.front() != PATH_SEP ) {
            snprintf( filePath, sizeof(filePath), "%s%s%s%s", g_pEditor->m_CurrentPath.c_str(), g_pPrefsDlg->m_ProjectDataPath.c_str(), path,
                ext ? "" : ".proj" );
        } else {
            snprintf( filePath, sizeof(filePath), "%s%s", path, ext ? "" : ".proj" );
        }
        ospath = filePath;
    } else {
        ospath = name.c_str();
    }

    std::unordered_map<std::string, std::shared_ptr<Project>>::iterator it = m_ProjList.find( ospath );

    if ( it == m_ProjList.end() || it == NULL ) {
        Log_Printf( "[CProjectManager::SetCurrent] Project '%s' doesn't exist, adding to cache...\n", ospath );

        if ( !FolderExists( ospath ) ) {
            New();
        } else {
            AddToCache( ospath, true );
        }

        it = m_ProjList.find( ospath );
    }

    Log_Printf( "[CProjectManager::SetCurrent] Loading project '%s'...\n", ospath );

    m_CurrentProject = it->second;

    if ( m_CurrentProject->m_MapList.size() ) {
        Map_LoadFile( m_CurrentProject->m_MapList.front()->name );
    }
}
