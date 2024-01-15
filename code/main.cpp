#include "editor.h"
#if defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )
	#include <pwd.h>
	#include <unistd.h>
	#ifdef __linux__
		#include <mntent.h>
	#endif
	#include <dirent.h>
	#include <pthread.h>
	#include <sys/wait.h>
	#include <signal.h>
	#include <fcntl.h>
#endif

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>

extern bool g_ApplicationRunning;
extern Walnut::Application *CreateApplication( int argc, char **argv );

std::string g_pidFile;

// =============================================================================
// Windows WinMain() wrapper for main()
// used in Release Builds to suppress the Console
#if defined(_WIN32)

#include <winbase.h>

int main( int argc, char **argv );

int CALLBACK WinMain(
  __in  HINSTANCE hInstance,
  __in  HINSTANCE hPrevInstance,
  __in  LPSTR lpCmdLine,
  __in  int nCmdShow
)
{
	return main( __argc, __argv );
}

#endif


// =============================================================================
// Loki stuff

#if defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )

/* The directory where the data files can be found (run directory) */
static char datapath[PATH_MAX];

#ifdef __linux__
/* Code to determine the mount point of a CD-ROM */
int loki_getmountpoint( const char *device, char *mntpt, int max_size ){
	char devpath[PATH_MAX], mntdevpath[PATH_MAX];
	FILE * mountfp;
	struct mntent *mntent;
	int mounted;

	/* Nothing to do with no device file */
	if ( device == NULL ) {
		*mntpt = '\0';
		return -1;
	}

	/* Get the fully qualified path of the CD-ROM device */
	if ( realpath( device, devpath ) == NULL ) {
		perror( "realpath() on your CD-ROM failed" );
		return( -1 );
	}

	/* Get the mount point */
	mounted = -1;
	memset( mntpt, 0, max_size );
	mountfp = setmntent( _PATH_MNTTAB, "r" );
	if ( mountfp != NULL ) {
		mounted = 0;
		while ( ( mntent = getmntent( mountfp ) ) != NULL )
		{
			char *tmp, mntdev[1024];

			strcpy( mntdev, mntent->mnt_fsname );
			if ( strcmp( mntent->mnt_type, "supermount" ) == 0 ) {
				tmp = strstr( mntent->mnt_opts, "dev=" );
				if ( tmp ) {
					strcpy( mntdev, tmp + strlen( "dev=" ) );
					tmp = strchr( mntdev, ',' );
					if ( tmp ) {
						*tmp = '\0';
					}
				}
			}
			if ( strncmp( mntdev, "/dev", 4 ) ||
				 realpath( mntdev, mntdevpath ) == NULL ) {
				continue;
			}
			if ( strcmp( mntdevpath, devpath ) == 0 ) {
				mounted = 1;
				assert( (int)strlen( mntent->mnt_dir ) < max_size );
				strncpy( mntpt, mntent->mnt_dir, max_size - 1 );
				mntpt[max_size - 1] = '\0';
				break;
			}
		}
		endmntent( mountfp );
	}
	return( mounted );
}
#endif

/*
    This function gets the directory containing the running program.
    argv0 - the 0'th argument to the program
 */
void loki_init_datapath( char *argv0 ){
	char temppath[PATH_MAX];
	const char *home;

	home = getenv( "HOME" );
	if ( home == NULL ) {
		home = ".";
	}

	strcpy( temppath, argv0 );

	/* Now canonicalize it to a full pathname for the data path */
	if ( realpath( temppath, datapath ) ) {
		/* There should always be '/' in the path, cut after so we end our directories with a slash */
		*( strrchr( datapath, '/' ) + 1 ) = '\0';
	}
}

#endif

// end of Loki stuff
// =============================================================================

std::string g_strPluginsDir;
std::string g_strModulesDir;
std::string g_strBitmapsDir;

#ifdef __unix__
static void signalCatcher( int signum )
{
    // I understand that most of the breaks here won't actually do anything...
    switch ( signum ) {
    case SIGSEGV:
        g_UnsafeExit = true;
        Error( "Segmentation Violation (SIGSEGV)" );
        break;
    case SIGABRT:
        g_UnsafeExit = true;
        Error( "Abnormal Program Termination (SIGABRT)" );
        break;
    case SIGBUS:
        g_UnsafeExit = true;
        Error( "Buss Error (SIGBUS)" );
        break;
    case SIGILL:
        g_UnsafeExit = true;
        Error( "Illegal Instruction (SIGILL)" );
        break;
    };
}
#endif

int valdenMain( int argc, char **argv )
{
#if defined( _WIN32 ) && defined( _MSC_VER )
	//increase the max open files to its maximum for the C run-time of MSVC
	_setmaxstdio( 2048 );
#endif

	putenv( (char *)"LC_NUMERIC=C" );

#if defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )
	// Give away unnecessary root privileges.
	// Important: must be done before calling gtk_init().
	char *loginname;
	struct passwd *pw;
	seteuid( getuid() );
	if ( geteuid() == 0 && ( loginname = getlogin() ) != NULL && ( pw = getpwnam( loginname ) ) != NULL ) {
		setuid( pw->pw_uid );
	}
#endif

	g_strPluginsDir = "plugins/";
	g_strModulesDir = "modules/";
	g_strBitmapsDir = "bitmaps/";

	std::string tempPath;

#ifdef __unix__
    signal( SIGSEGV, signalCatcher );
    signal( SIGABRT, signalCatcher );
    signal( SIGBUS, signalCatcher );
    signal( SIGILL, signalCatcher );
#endif

    g_pEditor = std::make_shared<CEditorLayer>();

#ifdef _WIN32
	// get path to editor
	char szBuffer[_MAX_PATH + 1];
	GetModuleFileName( NULL, szBuffer, _MAX_PATH );
	*( strrchr( szBuffer, '\\' ) + 1 ) = '\0';
    QE_ConvertDOSToUnixName( szBuffer, szBuffer );
	
	tempPath = szBuffer;
    g_pEditor->m_CurrentPath = tempPath;
    g_pPrefsDlg = new CPrefsDlg;
	g_pPrefsDlg->SetLabel( "Valden Preferences" );
#endif

#if defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )
	tempPath = getenv( "HOME" );

	tempPath += "/.valden/";
	Q_mkdir( tempPath.c_str(), 0775 );
    Q_mkdir( va( "%s%s", tempPath.c_str(), g_strPluginsDir.c_str() ), 0775 );
    Q_mkdir( va( "%s%s", tempPath.c_str(), g_strModulesDir.c_str() ), 0775 );
	Q_mkdir( va( "%s%s", tempPath.c_str(), g_strBitmapsDir.c_str() ), 0775 );
    Q_mkdir( va( "%s%s", tempPath.c_str(), "Data" ), 0775 );

    g_pEditor->m_CurrentPath = tempPath;
    g_pPrefsDlg = new CPrefsDlg;
	g_pPrefsDlg->SetLabel( "Valden Preferences" );
    Q_mkdir( va( "%s%s", tempPath.c_str(), g_pPrefsDlg->m_ProjectDataPath.c_str() ), 0775 );

	loki_init_datapath( argv[0] );
#endif

    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) < 0 ) {
		Log_Printf( "ERROR: failed to initialize SDL2 -- %s\n", SDL_GetError() );
		return -1;
	}

	g_pidFile = tempPath;
	g_pidFile += "valden.pid";

	FILE *pid;
	pid = fopen( g_pidFile.c_str(), "r" );
	if ( pid != NULL ) {
		fclose( pid );
		std::string msg;

		if ( remove( g_pidFile.c_str() ) == -1 ) {
			Sys_MessageBox( "Valden", va( "WARNING: Could not delete %s", g_pidFile.c_str() ), MB_OK | MB_ICONERROR );
		}

		// in debug, never prompt to clean registry, turn console logging auto after a failed start
#if !defined( _DEBUG ) || defined( NDEBUG )
		msg = "Found the file ";
		msg += g_pidFile;
		msg += ".\nThis indicates that Valden failed during the game selection startup last time it was run.\n"
			   "Choose YES to clean Valden's registry settings and shut down Valden.\n"
			   "WARNING: the global prefs will be lost if you choose YES.";

		if ( Sys_MessageBox( "Valden - Clean Registry?", msg.c_str(), MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
			// remove global prefs and shutdown
			remove( va( "%spreferences.json", g_pEditor->m_CurrentPath.c_str() ));
//			char buf[MAX_OSPATH];
//			sprintf( "" );

			// remove global prefs too
			g_pPrefsDlg->Reset();
			Sys_MessageBox( "Valden", "Cleaned registry settings, choose OK to close Valden.\nThe next time Valden runs it will use default settings.", MB_OK );
			_Exit( -1 );
		}
		msg = "Logging console output to ";
		msg += tempPath;
		msg += "valden.log\nRefer to the log if Valden fails to start again.";
		
		Sys_MessageBox( "Valden - Console Log", msg.c_str(), MB_OK );
#endif

		// force console logging on! (will go in prefs too)
		g_pPrefsDlg->m_bLogToFile = true;
		g_pPrefsDlg->m_bForceLog = true;
		Sys_LogFile();

		g_pPrefsDlg->Load();
	}
	else {
		// create one, will remove right after entering message loop
		pid = fopen( g_pidFile.c_str(), "w" );
		if ( pid ) {
			fclose( pid );
		}

		g_pPrefsDlg->Load();

#if !defined( _DEBUG ) || defined( NDEBUG ) // I can't be arsed about that prompt in debug mode
		// if console logging is on in the prefs, warn about performance hit
		if ( g_pPrefsDlg->m_bLogToFile ) {
			if ( Sys_MessageBox( "Valden", "Preferences indicate that console logging is on. This affects performance.\nTurn it off?" ,
				MB_YESNO | MB_ICONQUESTION ) == IDYES )
			{
				g_pPrefsDlg->m_bLogToFile = false;
				g_pPrefsDlg->Save();
			}
		}
#endif
		// toggle console logging if necessary
		Sys_LogFile();
	}

    g_pApplication = Walnut::CreateApplication( argc, argv );

	myargc = argc;
	myargv = argv;

	// if the first parameter is a .map, load that
	if ( argc > 1 && IsMap( argv[1] ) ) {
		Map_LoadFile( argv[1] );
	} else if ( g_pPrefsDlg->m_bLoadLastMap && g_pPrefsDlg->m_LastProject.length() > 0 ) {
		Map_LoadFile( g_pPrefsDlg->m_LastProject.c_str() );
	} else {
		Map_New();
	}

	// create a primary .pid for global init run
	pid = fopen( g_pidFile.c_str(), "w" );
	if ( pid ) {
		fclose( pid );
	}

    // remove pid file
    remove( g_pidFile.c_str() );

	g_ApplicationRunning = true;
	g_pApplication->Run();

	g_pPrefsDlg->m_bLogToFile = false;
	Sys_LogFile();

	if ( g_pApplication ) {
		// sdl2 and delete don't agree on basic shit
		g_pApplication = NULL;
	}

	return 0;
}


#if defined( _WIN32 ) && defined( _MSC_VER ) && !defined( _DEBUG )
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <strsafe.h> //StringCchPrintf

int GenerateDump( EXCEPTION_POINTERS* pExceptionPointers ) {
	BOOL bMiniDumpSuccessful;
    char szPath[MAX_PATH]; 
    char szFileName[MAX_PATH]; 
    char szAppName[] = "Valden";
    char* szVersion = RADIANT_VERSION;
    DWORD dwBufferSize = MAX_PATH;
    HANDLE hDumpFile;
    SYSTEMTIME stLocalTime;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    GetLocalTime( &stLocalTime );
    GetTempPath( dwBufferSize, szPath );

    StringCchPrintf( szFileName, MAX_PATH, "%s%s", szPath, szAppName );
    CreateDirectory( szFileName, NULL );

    StringCchPrintf( szFileName, MAX_PATH, "%s%s\\%s-%s-%04d%02d%02d-%02d%02d%02d.dmp", 
               szPath, szAppName, szAppName, szVersion, 
               stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, 
               stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond );
    hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE, 
                FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

    bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
                    hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main( int argc, char **argv )
{
#if defined( _WIN32 ) && defined( _MSC_VER ) && !defined( _DEBUG )
	__try {
		return valdenMain( argc, argv );
	} __except( GenerateDump( GetExceptionInformation() ) ) {

		char szPath[MAX_PATH]; 
		char szText[MAX_PATH]; 
		char szFileName[MAX_PATH]; 
		char szAppName[] = "Valden";
		SYSTEMTIME stLocalTime;
		DWORD dwBufferSize = MAX_PATH;

	    GetLocalTime( &stLocalTime );
	    GetTempPath( dwBufferSize, szPath );

	    StringCchPrintf( szFileName, MAX_PATH, "%s%s", szPath, szAppName );

		StringCchPrintf( szText, MAX_PATH, "Application crashed!\nCreated a dump file in: \n%s", szFileName );

		MessageBox( NULL, szText, NULL, MB_ICONERROR );
	}
#else
	return valdenMain( argc, argv );
#endif
}
