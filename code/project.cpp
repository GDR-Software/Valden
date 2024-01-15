#include "gln.h"
#include "editor.h"
#include "project.h"

CProjectManager *g_pProjectManager;

CProjectManager::CProjectManager( void )
{
    char filePath[MAX_OSPATH*2+1];

    m_ProjectsDirectory =  g_pEditor->m_CurrentPath + g_pPrefsDlg->m_ProjectDataPath;
    
    for ( const auto& directoryEntry : std::filesystem::directory_iterator{ m_ProjectsDirectory } ) {
        std::string path = directoryEntry.path().string();

        // is it a project directory?
        if ( directoryEntry.is_directory() && !N_stricmp( COM_GetExtension( path.c_str() ), "proj" ) ) {
            AddToCache( path, true );
        }
    }
}

CProjectManager::~CProjectManager()
{
    Save();
}

static void MakeAssetDirectoryPath()
{

}

void CProjectManager::AddToCache( const std::string& path, bool loadJSON, bool buildPath )
{
    std::string filePath;
    json data;
    std::shared_ptr<Project> proj;

    Log_Printf( "[CProjectManager::AddToCache] adding project directory '%s' to project cache\n", path.c_str() );

    proj = std::make_shared<Project>();

    proj->m_FilePath = va( "%s%c", path.c_str(), PATH_SEP );

    if ( loadJSON ) {
        const char *ospath = buildPath ? va( "%s%s%cConfig%cconfig.json", g_pEditor->m_CurrentPath.c_str(), path.c_str(), PATH_SEP, PATH_SEP )
            : va( "%s%cConfig%cconfig.json", path.c_str(), PATH_SEP, PATH_SEP );

        std::ifstream file( ospath, std::ios::in );
        if ( !file.is_open() ) {
            Log_Printf( "[CProjectManager::AddToCache] failed to open project config file '%s'!\n", ospath );
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
        proj->m_AssetDirectory = va( "%s%s%s.proj%c%s", g_pEditor->m_CurrentPath.c_str(), g_pPrefsDlg->m_ProjectDataPath.c_str(), proj->m_Name.c_str(), PATH_SEP,
            data["assetdirectory"].get<std::string>().c_str() );
        proj->m_AssetPath = data["assetdirectory"];
    } else {
        proj->m_Name = "untitled";
        proj->m_AssetDirectory = va( "%s%s%cAssets", buildPath ? g_pEditor->m_CurrentPath.c_str() : "", path.c_str(), PATH_SEP );
        proj->m_AssetPath = "Assets";
    }

    m_ProjList[ path ] = proj;

    try {
        for ( const auto& mapIterator : std::filesystem::directory_iterator{ proj->m_AssetDirectory } ) {
            filePath = mapIterator.path().string();

            if ( mapIterator.is_regular_file() && ( !N_stricmp( COM_GetExtension( filePath.c_str() ), "map" )
                || !N_stricmp( COM_GetExtension( filePath.c_str() ), "bmf") ) )
            {
                Map_LoadFile( filePath.c_str() );
                proj->m_MapList.emplace_back() = std::addressof( g_MapCache[filePath] );
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

    for ( count = 0; FolderExists( ( path = va( "%s%suntitled-%i.proj%c", g_pEditor->m_CurrentPath.c_str(), g_pPrefsDlg->m_ProjectDataPath.c_str(),
    count, PATH_SEP ) ) ); count++ )
        ;
    
    if ( !Q_mkdir( path ) ) {
        Error( "[CProjectManager::New] failed to create project directory '%s'", path );
    }

    AddToCache( path );

    m_CurrentProject = m_ProjList.find( path )->second;
}

void CProjectManager::Save( void ) const
{
    const char *ospath;

    ospath = BuildOSPath( m_CurrentProject->m_FilePath.c_str(), "Config", "config.json" );

    Log_Printf( "[CProjectManager::Save] Archiving project '%s' data to '%s'...\n", m_CurrentProject->m_Name.c_str(), ospath );

    std::ofstream file( ospath, std::ios::out );
    json data;

    if ( !file.is_open() ) {
        Error( "[CProjectManager::Save] Failed to save project file '%s'", ospath );
    }

    data["name"] = m_CurrentProject->m_Name;
    data["assetdirectory"] = m_CurrentProject->m_AssetPath;

    for ( const auto& it : m_CurrentProject->m_MapList ) {
        data["maplist"].emplace_back( it->name );
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

    if ( it == m_ProjList.end() ) {
        Log_Printf( "[CProjectManager::SetCurrent] Project '%s' doesn't exist, adding to cache...\n", ospath );

        AddToCache( ospath, true, buildPath );

        it = m_ProjList.find( ospath );
    }

    Log_Printf( "[CProjectManager::SetCurrent] Loading project '%s'...\n", ospath );
    Map_New();

    m_CurrentProject = it->second;

    if ( m_CurrentProject->m_MapList.size() ) {
        Map_LoadFile( m_CurrentProject->m_MapList.front()->name );
    }
}
