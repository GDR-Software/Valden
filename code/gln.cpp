#include "gln.h"
#include "editor.h"
#include "gui.h"
#include "events.h"
#include <SDL2/SDL.h>

#ifdef __unix__
	#include <unistd.h>
	#include <fcntl.h>
	#include <backtrace.h>
	#include <cxxabi.h>
#endif

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"

#include <bzlib.h>
#include <zlib.h>

bool g_UnsafeExit = false;
char **myargv;
int myargc;
int parm_compression;

#if !defined(NDEBUG) && !defined(_WIN32)
static struct backtrace_state *bt_state = NULL;

static void bt_error_callback( void *data, const char *msg, int errnum )
{
    Log_Printf( "libbacktrace ERROR: %d - %s\n", errnum, msg );
}

static void bt_syminfo_callback( void *data, uintptr_t pc, const char *symname,
								 uintptr_t symval, uintptr_t symsize )
{
	if (symname != NULL) {
		int status;
		// [glnomad] 10/6/2023: fixed buffer instead of malloc'd buffer, risky however
		char name[2048];
		size_t length = sizeof(name);
		abi::__cxa_demangle( symname, name, &length, &status );
		if ( name[0] ) {
			symname = name;
		}
		Log_Printf( "  %zu %s\n", pc, symname );
	} else {
        Log_Printf( "  %zu (unknown symbol)\n", pc );
	}
}

static int bt_pcinfo_callback( void *data, uintptr_t pc, const char *filename, int lineno, const char *function )
{
	if ( data != NULL ) {
		int* hadInfo = (int *)data;
		*hadInfo = ( function != NULL );
	}

	if ( function != NULL) {
		int status;
		// [glnomad] 10/6/2023: fixed buffer instead of malloc'd buffer, risky however
		char name[2048];
		size_t length = sizeof(name);
		abi::__cxa_demangle( function, name, &length, &status );
		if ( name[0] ) {
			function = name;
		}

		const char *fileNameSrc = strstr( filename, "/code/" );
		if ( fileNameSrc != NULL ) {
			filename = fileNameSrc + 1; // I want "code/bla/blub.cpp:42"
		}
        Log_Printf( "  %zu %s:%d %s\n", pc, filename, lineno, function );
	}

	return 0;
}

static void bt_error_dummy( void *data, const char *msg, int errnum )
{
	//CrashPrintf("ERROR-DUMMY: %d - %s\n", errnum, msg);
}

static int bt_simple_callback( void *data, uintptr_t pc )
{
	int pcInfoWorked = 0;
	// if this fails, the executable doesn't have debug info, that's ok (=> use bt_error_dummy())
	backtrace_pcinfo( bt_state, pc, bt_pcinfo_callback, bt_error_dummy, &pcInfoWorked );
	if ( !pcInfoWorked ) { // no debug info? use normal symbols instead
		// yes, it would be easier to call backtrace_syminfo() in bt_pcinfo_callback() if function == NULL,
		// but some libbacktrace versions (e.g. in Ubuntu 18.04's g++-7) don't call bt_pcinfo_callback
		// at all if no debug info was available - which is also the reason backtrace_full() can't be used..
		backtrace_syminfo( bt_state, pc, bt_syminfo_callback, bt_error_callback, NULL );
	}

	return 0;
}


static void do_backtrace( void )
{
    // can't use idStr here and thus can't use Sys_GetPath(PATH_EXE) => added Posix_GetExePath()
	const char* exePath = "Valden";
	bt_state = backtrace_create_state( exePath[0] ? exePath : NULL, 0, bt_error_callback, NULL );

    if ( bt_state != NULL ) {
		int skip = 2; // skip this function in backtrace
		backtrace_simple( bt_state, skip, bt_simple_callback, bt_error_callback, NULL );
	} else {
        Log_Printf( "*** !ERROR! ***: (No backtrace because libbacktrace state is NULL)" );
	}
}
#endif

#define MAX_VA_BUFFER 8192

const char *va( const char *fmt, ... )
{
	va_list argptr;
	static char string[8][MAX_VA_BUFFER];
    static int index = 0;
	char *buf;

	buf = string[ index % 8 ];
    index++;

	va_start( argptr, fmt );
	vsnprintf( buf, MAX_VA_BUFFER, fmt, argptr );
	va_end( argptr );

	return buf;
}

/*
void GLN_Init( void )
{
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) < 0 ) {
        Error( "GLN_Init: SDL2 failed to initialize -- %s", SDL_GetError() );
    }

    s_pWindow = SDL_CreateWindow( WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_OPENGL );
    if ( !s_pWindow ) {
        Error( "GLN_Init: failed to initialize SDL2 window -- %s", SDL_GetError() );
    }

    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    s_pGLContext = SDL_GL_CreateContext( s_pWindow );
    if ( !s_pGLContext ) {
        Error( "GLN_Init: failed to initialize SDL2 OpenGL Context -- %s", SDL_GetError() );
    }

    SDL_GL_MakeCurrent( s_pWindow, s_pGLContext );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL( s_pWindow, s_pGLContext );
    ImGui_ImplOpenGL3_Init();
}

void GLN_LoadGLProcs( void )
{
#define NGL(  )


#undef NGL
}

void GLN_ShutdownGL( void )
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    if ( s_pWindow ) {
        SDL_DestroyWindow( s_pWindow );
        s_pWindow = NULL;
    }
    if ( s_pGLContext ) {
        SDL_GL_DeleteContext( s_pGLContext );
        s_pGLContext = NULL;
    }

    SDL_Quit();
}
*/

char *CopyString( const char *str )
{
	char *out;
	uint64_t length;

	length = strlen( str ) + 1;
	out = (char *)GetMemory( length );

	N_strncpyz( out, str, length );

	return out;
}

void SafeRead( void *data, uint64_t size, IDataStream *stream )
{
	if ( !stream->Read( data, size ) ) {
		Error( "SafeRead: failed to read %i bytes from file", size );
	}
}

bff_t *bffOpenRead( const char *path )
{
	FileStream file;
	bff_t *bff;

	Log_Printf( "Loading BFF archive file '%s' in read mode...\n", path );

	if ( !file.Open( path, "rb" ) ) {
		Sys_MessageBox( "BFF Tool", va( "Failed to open BFF archive file '%s' in read mode", path ), MB_OK );
		return NULL;
	}

	if ( file.GetLength() < sizeof(bff->header) ) {
		Sys_MessageBox( "BFF Tool", va( "BFF archive file '%s' isn't large enough to contain a header", path ), MB_OK );
		return NULL;
	}

	bff = (bff_t *)GetMemory( sizeof(*bff) );

	SafeRead( &bff->header, sizeof(bff->header), &file );

	bff->chunkList = (bff_chunk_t *)GetMemory( sizeof(*bff->chunkList) * bff->header.numChunks );
	bff->numChunks = bff->header.numChunks;
	memcpy( &bff->header, &bff->header, sizeof(bff->header) );

	SafeRead( bff->bffGamename, sizeof(bff->bffGamename), &file );

	for ( int64_t i = 0; i < bff->numChunks; i++ ) {
		SafeRead( &bff->chunkList[i].chunkNameLen, sizeof(bff->chunkList[i].chunkNameLen), &file );
		if ( !bff->chunkList[i].chunkNameLen ) {
			bffClose( bff );

			Sys_MessageBox( "BFF Tool", va( "Failed to load BFF archive file '%s', chunk name length invalid", path ), MB_OK );
			
			return NULL;
		}

		bff->chunkList[i].chunkName = (char *)GetMemory( bff->chunkList[i].chunkNameLen );
		SafeRead( bff->chunkList[i].chunkName, bff->chunkList[i].chunkNameLen, &file );

		SafeRead( &bff->chunkList[i].chunkSize, sizeof(bff->chunkList[i].chunkSize), &file );
		if ( !bff->chunkList[i].chunkSize ) {
			bffClose( bff );

			Sys_MessageBox( "BFF Tool", va( "Failed to load BFF archive file '%s', chunk length invalid", path ), MB_OK );

			return NULL;
		}
		bff->chunkList[i].chunkBuffer = (char *)GetMemory( bff->chunkList[i].chunkSize );
		SafeRead( bff->chunkList[i].chunkBuffer, bff->chunkList[i].chunkSize, &file );
	}

	file.Close();

	Log_Printf( "Done.\n" );

	return bff;
}

FILE *bffOpenWrite( const char *path, bffheader_t *h )
{
	FILE *fp;

	fp = fopen( path, "wb" );
	if ( !fp ) {
		Sys_MessageBox( "BFF Tool", va( "Failed to create BFF archive file '%s'", path ), MB_OK );
		return NULL;
	}

	memset( h, 0, sizeof(*h) );
	h->magic = HEADER_MAGIC;
	h->numChunks = 0;
	h->compression = parm_compression;
	h->ident = BFF_IDENT;
	h->version = BFF_VERSION;
	
	// will be overwritten
	SafeWrite( h, sizeof(*h), fp );

	return fp;
}

void bffWriteChunk( const char *name, const void *data, int size, bffheader_t *h, FILE *fp )
{
	int64_t nameLength, bufSize;

	Log_Printf( "Writing chunk %s (%i bytes)...\n", name, size );

	nameLength = strlen( name ) + 1;
	bufSize = size;

	SafeWrite( &nameLength, sizeof(nameLength), fp );
	SafeWrite( name, nameLength, fp );
	SafeWrite( &bufSize, sizeof(bufSize), fp );
	SafeWrite( data, size, fp );

	h->numChunks++;
}

void bffClose( bff_t *archive )
{
	for ( int64_t i = 0; i < archive->numChunks; i++ ) {
		if ( archive->chunkList[i].chunkName ) {
			free( archive->chunkList[i].chunkName );
			archive->chunkList[i].chunkName = NULL;
		}
		if ( archive->chunkList[i].chunkBuffer ) {
			free( archive->chunkList[i].chunkBuffer );
			archive->chunkList[i].chunkBuffer = NULL;
		}
	}
	free( archive );
}

double Sys_DoubleTime( void ) {
	return (double)clock() / 1000.0f;
}

void Sys_LoadAsset( const char *title, float *progress, const std::function<void( void )>& loadFunc )
{
	int result;
	std::mutex loadLock;
	ImGuiIO& io = ImGui::GetIO();

	ImGui::OpenPopup( "Loading Asset" );
	std::thread loadThread( loadFunc );
	while ( *progress < 100.0f ) {

		events.EventLoop();

		for ( auto& layer : g_pApplication->m_LayerStack ) {
			layer->OnUpdate( g_pApplication->m_TimeStep );
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		{
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
			// because it would be confusing to have two docking targets within each others.
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
			if (g_pApplication->m_MenubarCallback)
				window_flags |= ImGuiWindowFlags_MenuBar;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
			// and handle the pass-thru hole, so we ask Begin() to not render a background.
			if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				window_flags |= ImGuiWindowFlags_NoBackground;

			// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
			// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
			// all active windows docked into it will lose their parent and become undocked.
			// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
			// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace Demo", nullptr, window_flags);
			ImGui::PopStyleVar();

			ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
				ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}

			if ( g_pApplication->m_MenubarCallback ) {
				if ( ImGui::BeginMenuBar() ) {
					g_pApplication->m_MenubarCallback();
					ImGui::EndMenuBar();
				}
			}

			for ( auto& layer : g_pApplication->m_LayerStack ) {
				layer->OnUIRender();
			}
			
			ImGui::End();
		}

		loadLock.lock();
		if ( ImGui::BeginPopupModal( "Loading Asset" ) ) {
			ImGui::Text( "Loading %s [%.03f / 100.0]...", title, *progress );
			ImGui::ProgressBar( *progress );

			if ( *progress >= 100.0f ) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		loadLock.unlock();

		// Rendering
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = ( main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f );
		if (!main_is_minimized) {
			ImGui_ImplOpenGL3_RenderDrawData( main_draw_data );
		}

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Present Main Platform Window
		if (!main_is_minimized) {
//			ImGui_ImplOpenGL3_RenderDrawData( main_draw_data );
		}

		float time = g_pApplication->GetTime();
		g_pApplication->m_FrameTime = time - g_pApplication->m_LastFrameTime;
		g_pApplication->m_TimeStep = glm::min<float>( g_pApplication->m_FrameTime, 0.0333f );
		g_pApplication->m_LastFrameTime = time;
		SDL_GL_SwapWindow( g_pApplication->m_WindowHandle );
	}

	loadThread.join();
}

int N_stricmp( const char *s1, const char *s2 ) 
{
	unsigned char c1, c2;

	if (s1 == NULL)  {
		if (s2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (s2 == NULL)
		return 1;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 != c2) {
			if ( c1 <= 'Z' && c1 >= 'A' )
				c1 += ('a' - 'A');

			if ( c2 <= 'Z' && c2 >= 'A' )
				c2 += ('a' - 'A');

			if ( c1 != c2 ) 
				return c1 < c2 ? -1 : 1;
		}
	} while ( c1 != '\0' );

	return 0;
}

void N_strncpyz (char *dest, const char *src, size_t count)
{
	if (!dest)
		Error( "N_strncpyz: NULL dest");
	if (!src)
		Error( "N_strncpyz: NULL src");
	if (count < 1)
		Error( "N_strncpyz: bad count");
	
#if 0 // this ain't quake3
	// do not fill whole remaining buffer with zeros
	// this is obvious behavior change but actually it may affect only buggy QVMs
	// which passes overlapping or short buffers to cvar reading routines
	// what is rather good than bad because it will no longer cause overwrites, maybe
	while ( --count > 0 && (*dest++ = *src++) != '\0' );
	*dest = '\0';
#else
	strncpy( dest, src, count-1 );
	dest[ count-1 ] = '\0';
#endif
}

int CheckParm( const char *name )
{
	int i;

	for ( i = 1; i < myargc; i++ ) {
		if ( !N_stricmp( name, myargv[i] ) ) {
			return i;
		}
	}

	return -1;
}

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}


/*
============
COM_GetExtension
============
*/
const char *COM_GetExtension( const char *name )
{
	const char *dot = strrchr(name, '.'), *slash;
	if (dot && ((slash = strrchr(name, '/')) == NULL || slash < dot))
		return dot + 1;
	else
		return "";
}


/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '.'), *slash;

	if (dot && ((slash = strrchr(in, '/')) == NULL || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		N_strncpyz(out, in, destsize);
}


/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
qboolean COM_CompareExtension(const char *in, const char *ext)
{
	int inlen, extlen;
	
	inlen = strlen(in);
	extlen = strlen(ext);
	
	if(extlen <= inlen)
	{
		in += inlen - extlen;
		
		if(!N_stricmp(in, ext))
			return qtrue;
	}
	
	return qfalse;
}


/*
==================
COM_DefaultExtension

if path doesn't have an extension, then append
 the specified one (which should include the .)
==================
*/
void COM_DefaultExtension( char *path, int maxSize, const char *extension )
{
	const char *dot = strrchr(path, '.'), *slash;
	if (dot && ((slash = strrchr(path, '/')) == NULL || slash < dot))
		return;
	else
		strncat(path, extension, maxSize);
}


char *BuildOSPath( const char *curPath, const char *gamepath, const char *npath )
{
	static char ospath[MAX_OSPATH*2+1];
	char temp[MAX_OSPATH];

	if ( npath ) {
		snprintf( temp, sizeof(temp), "%c%s%c%s", PATH_SEP, gamepath, PATH_SEP, npath );
    } else {
		snprintf( temp, sizeof(temp), "%c%s%c", PATH_SEP, gamepath, PATH_SEP );
    }

	snprintf( ospath, sizeof(ospath), "%s%s", curPath, temp );
	return ospath;
}

static inline const char *bzip2_strerror( int err )
{
	switch ( err ) {
	case BZ_DATA_ERROR: return "(BZ_DATA_ERROR) buffer provided to bzip2 was corrupted";
	case BZ_MEM_ERROR: return "(BZ_MEM_ERROR) memory allocation request made by bzip2 failed";
	case BZ_DATA_ERROR_MAGIC: return "(BZ_DATA_ERROR_MAGIC) buffer was not compressed with bzip2, it did not contain \"BZA\"";
	case BZ_IO_ERROR: return va("(BZ_IO_ERROR) failure to read or write, file I/O error");
	case BZ_UNEXPECTED_EOF: return "(BZ_UNEXPECTED_EOF) unexpected end of data stream";
	case BZ_OUTBUFF_FULL: return "(BZ_OUTBUFF_FULL) buffer overflow";
	case BZ_SEQUENCE_ERROR: return "(BZ_SEQUENCE_ERROR) bad function call error, please report this bug";
	case BZ_OK:
		break;
	};
	return "No Error... How?";
}

static inline bool CheckBZIP2( int errcode, uint64_t buflen, const char *action )
{
	switch ( errcode ) {
	case BZ_OK:
	case BZ_RUN_OK:
	case BZ_FLUSH_OK:
	case BZ_FINISH_OK:
	case BZ_STREAM_END:
		return true; // all good
	case BZ_CONFIG_ERROR:
	case BZ_DATA_ERROR:
	case BZ_DATA_ERROR_MAGIC:
	case BZ_IO_ERROR:
	case BZ_MEM_ERROR:
	case BZ_PARAM_ERROR:
	case BZ_SEQUENCE_ERROR:
	case BZ_OUTBUFF_FULL:
	case BZ_UNEXPECTED_EOF:
		Sys_MessageBox( va( "BZip2 %s Failure", action ), va( "Failure on %s of %lu bytes. BZIP2 error reason:\n\t%s", action, buflen, bzip2_strerror( errcode ) ),
			MB_OK | MB_ICONERROR );
		break;
	};
	return false;
}

static char *Compress_BZIP2( void *buf, uint64_t buflen, uint64_t *outlen )
{
	char *out, *newbuf;
	unsigned int len;
	int ret;

	Log_Printf( "Compressing %lu bytes with bzip2...\n", buflen );

	len = buflen;
	out = (char *)GetMemory( buflen );

	ret = BZ2_bzBuffToBuffCompress( out, &len, (char *)buf, buflen, 9, 4, 50 );
	if ( !CheckBZIP2( ret, buflen, "Compression" ) ) {
		return (char *)buf;
	}

	Log_Printf( "Successful compression of %lu to %u bytes with bzip2.\n", buflen, len );
	newbuf = (char *)GetMemory( len );
	memcpy( newbuf, out, len );
	FreeMemory( out );
	*outlen = len;

	return newbuf;
}

static inline const char *zlib_strerror( int err )
{
	switch ( err ) {
	case Z_DATA_ERROR: return "(Z_DATA_ERROR) buffer provided to zlib was corrupted";
	case Z_BUF_ERROR: return "(Z_BUF_ERROR) buffer overflow";
	case Z_STREAM_ERROR: return "(Z_STREAM_ERROR) bad params passed to zlib, please report this bug";
	case Z_MEM_ERROR: return "(Z_MEM_ERROR) memory allocation request made by zlib failed";
	case Z_OK:
		break;
	};
	return "No Error... How?";
}

static void *zalloc( voidpf opaque, uInt items, uInt size )
{
	(void)opaque;
	(void)items;
	return GetMemory( size );
}

static void zfreeMemory( voidpf opaque, voidpf address )
{
	(void)opaque;
	FreeMemory( (void *)address );
}

static char *Compress_ZLIB( void *buf, uint64_t buflen, uint64_t *outlen )
{
	char *out, *newbuf;
	const uint64_t expectedLen = buflen / 2;
	int ret;

	out = (char *)GetMemory( buflen );

#if 0
	stream.zalloc = zalloc;
	stream.zfree = zfree;
	stream.opaque = Z_NULL;
	stream.next_in = (Bytef *)buf;
	stream.avail_in = buflen;
	stream.next_out = (Bytef *)out;
	stream.avail_out = buflen;

	ret = deflateInit2( &stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15, 9, );
	if (ret != Z_OK) {
		Error( "Failed to compress buffer of %lu bytes (inflateInit2)", buflen );
		return NULL;
	}
	inflate

	do {
		ret = inflate( &stream, Z_SYNC_FLUSH );

		switch (ret) {
		case Z_NEED_DICT:
		case Z_STREAM_ERROR:
			ret = Z_DATA_ERROR;
			break;
		case Z_DATA_ERRO:
		case Z_MEM_ERROR:
			inflateEnd( &stream );
			Error( "Failed to compress buffer of %lu bytes (inflate)", buflen );
			return NULL;
		};
		
		if (ret != Z_STREAM_END) {
			newbuf = (char *)GetMemory( buflen * 2 );
			memcpy( newbuf, out, buflen * 2 );
			FreeMemory( out );
			out = newbuf;

			stream.next_out = (Bytef *)( out + buflen );
			stream.avail_out = buflen;
			buflen *= 2;
		}
	} while ( ret != Z_STREAM_END );
#endif
	Log_Printf( "Compressing %lu bytes with zlib...\n", buflen );

	ret = compress2( (Bytef *)out, (uLongf *)outlen, (const Bytef *)buf, buflen, Z_BEST_COMPRESSION );
	if ( ret != Z_OK ) {
		Sys_MessageBox( "ZLib Compression Failure", va( "Failure on compression of %lu bytes. ZLIB error reason:\n\t%s", buflen, zError( ret ) ),
			MB_OK );
	}
	
	Log_Printf( "Successful compression of %lu to %lu bytes with zlib.\n", buflen, *outlen );
	newbuf = (char *)GetMemory( *outlen );
	memcpy( newbuf, out, *outlen );
	FreeMemory( out );

	return newbuf;
}

char *Compress( void *buf, uint64_t buflen, uint64_t *outlen, int compression )
{
	switch ( compression ) {
	case COMPRESS_BZIP2:
		return Compress_BZIP2( buf, buflen, outlen );
	case COMPRESS_ZLIB:
		return Compress_ZLIB( buf, buflen, outlen );
	default:
		break;
	};
	return (char *)buf;
}

static char *Decompress_BZIP2( void *buf, uint64_t buflen, uint64_t *outlen )
{
	char *out, *newbuf;
	unsigned int len;
	int ret;

	Log_Printf( "Decompressing %lu bytes with bzip2...\n", buflen );

	len = buflen * 2;
	out = (char *)GetMemory( buflen * 2 );

	ret = BZ2_bzBuffToBuffDecompress( out, &len, (char *)buf, buflen, 0, 4 );
	if ( !CheckBZIP2( ret, buflen, "Decompression" ) ) {
		return (char *)buf;
	}

	Log_Printf( "Successful decompression of %lu to %u bytes with bzip2.\n", buflen, len );
	newbuf = (char *)GetMemory( len );
	memcpy( newbuf, out, len );
	FreeMemory( out );
	*outlen = len;

	return newbuf;
}

static char *Decompress_ZLIB( void *buf, uint64_t buflen, uint64_t *outlen )
{
	char *out, *newbuf;
	int ret;

	Log_Printf( "Decompressing %lu bytes with zlib...\n", buflen );
	out = (char *)GetMemory( buflen * 2 );
	*outlen = buflen * 2;
	
	ret = uncompress( (Bytef *)out, (uLongf *)outlen, (const Bytef *)buf, buflen );
	if ( ret != Z_OK ) {
		Sys_MessageBox( "ZLib Decompression Failure", va( "Failure on decompression of %lu bytes. ZLIB error reason:\n\t:%s", buflen * 2, zError( ret ) ),
			MB_OK | MB_ICONERROR );
		return (char *)buf;
	}
	
	Log_Printf( "Successful decompression of %lu bytes to %lu bytes with zlib.\n", buflen, *outlen );

	newbuf = (char *)GetMemory( *outlen );
	memcpy( newbuf, out, *outlen );
	FreeMemory( out );

	return newbuf;
}

char *Decompress( void *buf, uint64_t buflen, uint64_t *outlen, int compression )
{
	switch ( compression ) {
	case COMPRESS_BZIP2:
		return Decompress_BZIP2( buf, buflen, outlen );
	case COMPRESS_ZLIB:
		return Decompress_ZLIB( buf, buflen, outlen );
	default:
		break;
	};
	return (char *)buf;
}


void Sys_SetWindowTitle( const char *title )
{
	SDL_SetWindowTitle( g_pApplication->GetWindowHandle(), title );
}

bool FileExists( const char *filename )
{
    FILE *fp;

    fp = fopen( filename, "rb" );
    if ( !fp ) {
        return false;
    }

    fclose( fp );
    return true;
}

const bool IsMap( const char *filename ) {
	return strlen( filename ) >= 4 && strcmp( filename + strlen( filename ) - 4, ".map" ) == 0;
}

//
// Sys_MessageBox: adapted slightly from the source engine
//
int Sys_MessageBox( const char *title, const char *message, unsigned flags )
{
	int buttonid;
	int buttonType, iconType;
	int numButtons;
	SDL_MessageBoxButtonData *buttonData;
	SDL_MessageBoxData boxData;

	memset( &boxData, 0, sizeof(boxData) );
	buttonid = 0;

	buttonType = flags & MB_BUTTONBITS;
	iconType = flags & MB_ICONBITS;

	switch ( buttonType ) {
	case MB_OK: {
		static SDL_MessageBoxButtonData data[] = {
	        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK,     "OK"     }
	    };
		numButtons = 1;
		buttonData = data;
		break; }
	case MB_OKCANCEL: {
		static SDL_MessageBoxButtonData data[] = {
	        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK,     "OK"     },
	        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "CANCEL" }
	    };
		numButtons = 2;
		buttonData = data;
		break; }
	case MB_RETRYCANCEL: {
		static SDL_MessageBoxButtonData data[] = {
	        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDRETRY,  "RETRY"  },
	        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "CANCEL" }
	    };
		numButtons = 2;
		buttonData = data;
		break; }
	case MB_ABORTRETRYIGNORE: {
		static SDL_MessageBoxButtonData data[] = {
	        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDABORT,  "ABORT"  },
	        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDRETRY,  "RETRY"  },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDIGNORE, "IGNORE" }
	    };
		numButtons = 3;
		buttonData = data;
		break; }
	case MB_YESNO: {
		static SDL_MessageBoxButtonData data[] = {
	        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES,	 "YES"    },
	        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDNO,     "NO"     }
	    };
		numButtons = 2;
		buttonData = data;
		break; }
	case MB_YESNOCANCEL: {
		static SDL_MessageBoxButtonData data[] = {
	        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES,    "YES"    },
			{ 0,                                       IDNO,     "NO"     },
	        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "CANCEL" }
	    };
		numButtons = 3;
		buttonData = data;
		break; }
	};

	boxData.window = NULL;
	boxData.title = title;
	boxData.message = message;
	boxData.numbuttons = numButtons;
	boxData.buttons = buttonData;

	switch ( iconType ) {
	case MB_ICONERROR:
		boxData.flags = SDL_MESSAGEBOX_ERROR;
		break;
	case MB_ICONWARNING:
		boxData.flags = SDL_MESSAGEBOX_WARNING;
		break;
	case MB_ICONINFORMATION:
		boxData.flags = SDL_MESSAGEBOX_INFORMATION;
		break;
	case MB_ICONQUESTION:
		break;
	};

	Log_Printf( "%s: %s\n", title, message );
	SDL_ShowMessageBox( &boxData, &buttonid );
	return buttonid;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

bool FolderExists( const char *name )
{
#ifdef _WIN32
    struct _stat fdata;
#else
	struct stat fdata;
#endif

#ifdef _WIN32
    if ( _stat( name, &fdata ) == 0 )
#else
	if ( stat( name, &fdata ) == 0 )
#endif
	{
        return true;
    }

    return false;
}

bool Q_mkdir( const char *name, int perms )
{
#ifdef _WIN32
	if ( _mkdir( path ) == 0 ) {
		return true;
	} else {
		if ( errno == EEXIST ) {
			return true;
		} else {
			return false;
		}
	}
#else
    if ( mkdir( name, perms ) == 0 ) {
        return true;
    } else {
        return errno == EEXIST;
    }
#endif
}

uint64_t FileLength( FILE *fp )
{
	uint64_t pos, end;

    pos = ftello64( fp );
    fseek( fp, 0L, SEEK_END );
    end = ftello64( fp );
    fseek( fp, pos, SEEK_SET );

    return end;
}

void SafeRead( void *buffer, uint64_t size, FILE *fp )
{
    if ( !fread( buffer, 1, size, fp ) ) {
        Error( "Failed to read %i bytes from file", size );
    }
}

void SafeWrite( const void *buffer, uint64_t size, FILE *fp )
{
    if ( !fwrite( buffer, 1, size, fp ) ) {
        Error( "Failed to write %i bytes to file", size );
    }
}

uint64_t LoadFile( const char *filename, void **buffer )
{
    uint64_t length;
    void *buf;
    FILE *fp;

    fp = fopen( filename, "rb" );
    if ( !fp ) {
        Log_Printf( "failed to load file '%s' in read-only mode.\n", filename );
		*buffer = NULL;
        return -1;
    }

    fseek( fp, 0L, SEEK_END );
    length = ftello64( fp );
    fseek( fp, 0L, SEEK_SET );

    buf = GetMemory( length );
    *buffer = buf;

    SafeRead( buf, length, fp );

    fclose( fp );

    return length;
}

void WriteFile( const char *filename, const void *buffer, uint64_t length )
{
    FILE *fp;

    fp = fopen( filename, "wb" );
    if ( !fp ) {
        Error( "failed to create file '%s' in write-only mode.", filename );
    }

    SafeWrite( buffer, length, fp );

    fclose( fp );
}

void *GetMemory( uint64_t size )
{
    void *buf;

    buf = malloc( size );
    if ( !buf ) {
        Error( "GetMemory: memory request for %i bytes failed!", size );
    }

    memset( buf, 0, size );

    return buf;
}

#ifdef _WIN32
#include <malloc.h>
#else
#include <malloc.h>
#endif

void *GetResizedMemory( void *ptr, uint64_t size )
{
    uint64_t oldsize;
    void *p;

    p = GetMemory( size );

    if ( ptr ) {
#ifdef _WIN32
        oldsize = _msize( ptr );
#else
        oldsize = malloc_usable_size( ptr );
#endif
        memcpy( p, ptr, size <= oldsize ? size : oldsize );

        free( ptr );
    }

    return p;
}

void FreeMemory( void *buf )
{
    if ( buf ) {
        free( buf );
    }
}


char *N_stradd(char *dst, const char *src)
{
	char c;
	while ( (c = *src++) != '\0' )
		*dst++ = c;
	*dst = '\0';
	return dst;
}


/*
Com_GenerateHashValue: used in renderer and filesystem
*/
// ASCII lowcase conversion table with '\\' turned to '/' and '.' to '\0'
static const byte hash_locase[ 256 ] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x00,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
	0x78,0x79,0x7a,0x5b,0x2f,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
	0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
	0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
	0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
	0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
	0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
	0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
	0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
	0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
	0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

uint64_t Com_GenerateHashValue( const char *fname, const uint64_t size )
{
	const byte *s;
	uint64_t hash;
	int c;

	s = (byte *)fname;
	hash = 0;
	
	while ( (c = hash_locase[(byte)*s++]) != '\0' ) {
		hash = hash * 101 + c;
	}
	
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);

	return hash;
}

// =================================================================================
// logging

#define BUFFER_SIZE 4096

static int s_hLogFile;

extern "C" void Sys_FPrintf_VA( int level, const char *text, va_list args )
{
	char buf[BUFFER_SIZE];

	buf[0] = 0;
	vsnprintf( buf, BUFFER_SIZE, text, args );
	buf[BUFFER_SIZE - 1] = 0;
	const unsigned int length = strlen( buf );

	if ( s_hLogFile ) {
#ifdef _WIN32
		_write( s_hLogFile, buf, length );
		_commit( s_hLogFile );
#elif defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )
		write( s_hLogFile, buf, length );
#endif
	}

	if ( level != SYS_NOCON ) {
		CMapRenderer::Print( "%s", buf );
	}

	fwrite( buf, 1, length, stdout );
}

// NOTE: this is the handler sent to synapse
// must match PFN_SYN_PRINTF_VA
extern "C" void Sys_Printf_VA( const char *text, va_list args ){
	Sys_FPrintf_VA( SYS_STD, text, args );
}

extern "C" void Log_Printf( const char *text, ... ) {
	va_list args;

	va_start( args, text );
	Sys_FPrintf_VA( SYS_STD, text, args );
	va_end( args );
}

extern "C" void Log_FPrintf( int level, const char *text, ... ){
	va_list args;

	va_start( args, text );
	Sys_FPrintf_VA( level, text, args );
	va_end( args );
}


void Error( const char *fmt, ... )
{
    va_list argptr;
    char msg[8192];

	if ( !s_hLogFile && g_pPrefsDlg ) {
		Sys_LogFile();
	}

    va_start( argptr, fmt );
    vsnprintf( msg, sizeof(msg) - 1, fmt, argptr );
    va_end( argptr );

    fprintf( stderr, "\n***************\nERROR: %s\n***************\n", msg );
    if ( s_hLogFile ) {
        Log_FPrintf( SYS_ERR, "\n***************\nERROR: %s\n***************\n", msg );
    }

#ifndef _WIN32
	do_backtrace();
#endif

	Sys_MessageBox( "Valden -- Error", msg, MB_OK | MB_ICONERROR );

	if ( g_pPrefsDlg ) {
		g_pPrefsDlg->m_bLogToFile = false;
		Sys_LogFile();
	}

	if ( g_pApplication && !g_UnsafeExit ) {
	    g_pApplication->Close();
		delete g_pApplication;
	}


	//
	// if we're exiting from a segfault or something like that, we don't want to run the atexit functions
	// in the case that those could just trigger another seggy and that would be VERY bad
	//
	if ( g_UnsafeExit ) {
		_Exit( EXIT_FAILURE );
	} else {
		exit( EXIT_FAILURE );
	}
}

// called whenever we need to open/close/check the console log file
void Sys_LogFile( void )
{
	if ( g_pPrefsDlg->m_bLogToFile && !s_hLogFile ) {
		// settings say we should be logging and we don't have a log file .. so create it
		// open a file to log the console (if user prefs say so)
		// the file handle is s_hLogFile
		// the log file is erased
		std::string name;
		name = g_pEditor->m_CurrentPath;
		name += "valden.log";
#if defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )
		s_hLogFile = open( name.c_str(), O_TRUNC | O_CREAT | O_WRONLY, S_IREAD | S_IWRITE );
#endif
#ifdef _WIN32
		s_hLogFile = _open( name.c_str(), _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE );
#endif
		if ( s_hLogFile ) {
			Log_Printf( "Started logging to %s\n", name.c_str() );
			time_t localtime;
			time( &localtime );
			Log_Printf( "Today is: %s", ctime( &localtime ) );
			Log_Printf( "This is valden '" VALDEN_VERSION "' compiled " __DATE__ "\n" );
			Log_Printf( VALDEN_ABOUTMSG "\n" );
		}
		else {
			Sys_MessageBox( "Console Logging", "Failed to create log file, check write permissions in Valden directory.", MB_OK );
		}
	}
	else if ( !g_pPrefsDlg->m_bLogToFile && s_hLogFile ) {
		// settings say we should not be logging but still we have an active logfile .. close it
		time_t localtime;
		time( &localtime );
		Log_Printf( "Closing log file at %s\n", ctime( &localtime ) );
	#ifdef _WIN32
		_close( s_hLogFile );
	#endif
	#if defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __APPLE__ )
		close( s_hLogFile );
	#endif
		s_hLogFile = 0;
	}
}

/*
* GLN_CheckAutoSave: if m_nAutoSaveTime (in minutes) have passed since last edit, and the map hasn't been saved yet, save it
*/
void GLN_CheckAutoSave( void )
{
	static time_t s_start = 0;
    time_t now;
    time( &now );

    if ( g_pMapInfoDlg->m_bMapModified || !s_start ) {
        s_start = now;
        return;
    }

    if ( ( now - s_start ) > ( 60 * g_pPrefsDlg->m_nAutoSaveTime ) ) {
        if ( g_pPrefsDlg->m_bAutoSave ) {
            Log_Printf( "Autosaving..." );
            Map_Save();
			Log_Printf( "Autosaving...Saved" );
			g_pMapInfoDlg->m_bMapModified = false;
        }
        else {
			Log_Printf( "Autosave skipped...\n" );
        }
        s_start = now;
    }
}

void QE_ConvertDOSToUnixName( char *dst, const char *src )
{
	while ( *src )
	{
		if ( *src == '\\' )
			*dst = '/';
		else
			*dst = *src;
		dst++; src++;
	}
	*dst = 0;
}

void *CopyMemory( const void *src, int nBytes ) {
	return memcpy( GetMemory( nBytes ), src, nBytes );
}

/*
===============================================================

Parsing

===============================================================
*/

static	char	com_token[MAX_TOKEN_CHARS];
static	char	com_parsename[MAX_TOKEN_CHARS];
static	uint64_t com_lines;
static  uint64_t com_tokenline;

// for complex parser
tokenType_t		com_tokentype;

void COM_BeginParseSession( const char *name )
{
	com_lines = 1;
	com_tokenline = 0;
	snprintf(com_parsename, sizeof(com_parsename), "%s", name);
}


uint64_t COM_GetCurrentParseLine( void )
{
	if ( com_tokenline )
	{
		return com_tokenline;
	}

	return com_lines;
}


const char *COM_Parse( const char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

void COM_ParseError( const char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf (string, format, argptr);
	va_end( argptr );

	Log_Printf( "ERROR: %s, line %lu: %s\n", com_parsename, COM_GetCurrentParseLine(), string );
}

void COM_ParseWarning( const char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf (string, format, argptr);
	va_end( argptr );

	Log_Printf( "WARNING: %s, line %lu: %s\n", com_parsename, COM_GetCurrentParseLine(), string );
}

/*
==================
COM_MatchToken
==================
*/
bool COM_MatchToken( const char **buf_p, const char *match ) {
	const char *token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) ) {
	    COM_ParseError( "MatchToken: %s != %s", token, match );
		return false;
	}
	return true;
}


/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
const char *SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	int c;

	while( (c = *data) <= ' ') {
		if( !c ) {
			return NULL;
		}
		if( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}


int ParseHex(const char *text)
{
    int value;
    int c;

    value = 0;
    while ((c = *text++) != 0) {
        if (c >= '0' && c <= '9') {
            value = value * 16 + c - '0';
            continue;
        }
        if (c >= 'a' && c <= 'f') {
            value = value * 16 + 10 + c - 'a';
            continue;
        }
        if (c >= 'A' && c <= 'F') {
            value = value * 16 + 10 + c - 'A';
            continue;
        }
    }

    return value;
}

bool Parse1DMatrix( const char **buf_p, int x, float *m ) {
	const char	*token;
	int		i;

	if (!COM_MatchToken( buf_p, "(" )) {
		return false;
	}

	for (i = 0 ; i < x; i++) {
		token = COM_Parse( buf_p );
		m[i] = atof( token );
	}

	if (!COM_MatchToken( buf_p, ")" )) {
		return false;
	}
	return true;
}

bool Parse2DMatrix( const char **buf_p, int y, int x, float *m ) {
	int		i;

	if (!COM_MatchToken( buf_p, "(" )) {
		return false;
	}

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix (buf_p, x, m + i * x);
	}

	if (!COM_MatchToken( buf_p, ")" )) {
		return false;
	}
	return true;
}

bool Parse3DMatrix( const char **buf_p, int z, int y, int x, float *m ) {
	int		i;

	if (!COM_MatchToken( buf_p, "(" )) {
		return false;
	}

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	}

	if (!COM_MatchToken( buf_p, ")" )) {
		return false;
	}

	return true;
}

uintptr_t COM_Compress( char *data_p ) {
	const char *in;
	char *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	while ((c = *in) != '\0') {
		// skip double slash comments
		if ( c == '/' && in[1] == '/' ) {
			while (*in && *in != '\n') {
				in++;
			}
		// skip /* */ comments
		} else if ( c == '/' && in[1] == '*' ) {
			while ( *in && ( *in != '*' || in[1] != '/' ) ) 
				in++;
			if ( *in ) 
				in += 2;
			// record when we hit a newline
		} else if ( c == '\n' || c == '\r' ) {
			newline = qtrue;
			in++;
			// record when we hit whitespace
		} else if ( c == ' ' || c == '\t') {
			whitespace = qtrue;
			in++;
			// an actual token
		} else {
			// if we have a pending newline, emit it (and it counts as whitespace)
			if (newline) {
				*out++ = '\n';
				newline = qfalse;
				whitespace = qfalse;
			} else if (whitespace) {
				*out++ = ' ';
				whitespace = qfalse;
			}
			// copy quoted strings unmolested
			if (c == '"') {
				*out++ = c;
				in++;
				while (1) {
					c = *in;
					if (c && c != '"') {
						*out++ = c;
						in++;
					} else {
						break;
					}
				}
				if (c == '"') {
					*out++ = c;
					in++;
				}
			} else {
				*out++ = c;
				in++;
			}
		}
	}

	*out = '\0';

	return (uintptr_t)(out - data_p);
}

const char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks )
{
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = '\0';
	com_tokenline = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data ) {
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks ) {
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[1] == '*' ) {
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) ) {
				if ( *data == '\n' ) {
					com_lines++;
				}
				data++;
			}
			if ( *data ) {
				data += 2;
			}
		}
		else {
			break;
		}
	}

	// token starts on this line
	com_tokenline = com_lines;

	// handle quoted strings
	if ( c == '"' )
	{
		data++;
		while ( 1 )
		{
			c = *data;
			if ( c == '"' || c == '\0' )
			{
				if ( c == '"' )
					data++;
				com_token[ len ] = '\0';
				*data_p = data;
				return com_token;
			}
			data++;
			if ( c == '\n' )
			{
				com_lines++;
			}
			if ( len < arraylen( com_token )-1 )
			{
				com_token[ len ] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < arraylen( com_token )-1 )
		{
			com_token[ len ] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > ' ' );

	com_token[ len ] = '\0';

	*data_p = data;
	return com_token;
}
	

/*
==============
COM_ParseComplex
==============
*/
char *COM_ParseComplex( const char **data_p, qboolean allowLineBreaks )
{
	static const byte is_separator[ 256 ] =
	{
	// \0 . . . . . . .\b\t\n . .\r . .
		1,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,
	//  . . . . . . . . . . . . . . . .
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	//    ! " # $ % & ' ( ) * + , - . /
		1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0, // excl. '-' '.' '/'
	//  0 1 2 3 4 5 6 7 8 9 : ; < = > ?
		0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
	//  @ A B C D E F G H I J K L M N O
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	//  P Q R S T U V W X Y Z [ \ ] ^ _
		0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0, // excl. '\\' '_'
	//  ` a b c d e f g h i j k l m n o
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	//  p q r s t u v w x y z { | } ~ 
		0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1
	};

	int c, len, shift;
	const byte *str;

	str = (byte*)*data_p;
	len = 0; 
	shift = 0; // token line shift relative to com_lines
	com_tokentype = TK_GENEGIC;
	
__reswitch:
	switch ( *str )
	{
	case '\0':
		com_tokentype = TK_EOF;
		break;

	// whitespace
	case ' ':
	case '\t':
		str++;
		while ( (c = *str) == ' ' || c == '\t' )
			str++;
		goto __reswitch;

	// newlines
	case '\n':
	case '\r':
	com_lines++;
		if ( *str == '\r' && str[1] == '\n' )
			str += 2; // CR+LF
		else
			str++;
		if ( !allowLineBreaks ) {
			com_tokentype = TK_NEWLINE;
			break;
		}
		goto __reswitch;

	// comments, single slash
	case '/':
		// until end of line
		if ( str[1] == '/' ) {
			str += 2;
			while ( (c = *str) != '\0' && c != '\n' && c != '\r' )
				str++;
			goto __reswitch;
		}

		// comment
		if ( str[1] == '*' ) {
			str += 2;
			while ( (c = *str) != '\0' && ( c != '*' || str[1] != '/' ) ) {
				if ( c == '\n' || c == '\r' ) {
					com_lines++;
					if ( c == '\r' && str[1] == '\n' ) // CR+LF?
						str++;
				}
				str++;
			}
			if ( c != '\0' && str[1] != '\0' ) {
				str += 2;
			} else {
				// FIXME: unterminated comment?
			}
			goto __reswitch;
		}

		// single slash
		com_token[ len++ ] = *str++;
		break;
	
	// quoted string?
	case '"':
		str++; // skip leading '"'
		//com_tokenline = com_lines;
		while ( (c = *str) != '\0' && c != '"' ) {
			if ( c == '\n' || c == '\r' ) {
				com_lines++; // FIXME: unterminated quoted string?
				shift++;
			}
			if ( len < MAX_TOKEN_CHARS-1 ) // overflow check
				com_token[ len++ ] = c;
			str++;
		}
		if ( c != '\0' ) {
			str++; // skip ending '"'
		} else {
			// FIXME: unterminated quoted string?
		}
		com_tokentype = TK_QUOTED;
		break;

	// single tokens:
	case '+': case '`':
	/*case '*':*/ case '~':
	case '{': case '}':
	case '[': case ']':
	case '?': case ',':
	case ':': case ';':
	case '%': case '^':
		com_token[ len++ ] = *str++;
		break;

	case '*':
		com_token[ len++ ] = *str++;
		com_tokentype = TK_MATCH;
		break;

	case '(':
		com_token[ len++ ] = *str++;
		com_tokentype = TK_SCOPE_OPEN;
		break;

	case ')':
		com_token[ len++ ] = *str++;
		com_tokentype = TK_SCOPE_CLOSE;
		break;

	// !, !=
	case '!':
		com_token[ len++ ] = *str++;
		if ( *str == '=' ) {
			com_token[ len++ ] = *str++;
			com_tokentype = TK_NEQ;
		}
		break;

	// =, ==
	case '=':
		com_token[ len++ ] = *str++;
		if ( *str == '=' ) {
			com_token[ len++ ] = *str++;
			com_tokentype = TK_EQ;
		}
		break;

	// >, >=
	case '>':
		com_token[ len++ ] = *str++;
		if ( *str == '=' ) {
			com_token[ len++ ] = *str++;
			com_tokentype = TK_GTE;
		} else {
			com_tokentype = TK_GT;
		}
		break;

	//  <, <=
	case '<':
		com_token[ len++ ] = *str++;
		if ( *str == '=' ) {
			com_token[ len++ ] = *str++;
			com_tokentype = TK_LTE;
		} else {
			com_tokentype = TK_LT;
		}
		break;

	// |, ||
	case '|':
		com_token[ len++ ] = *str++;
		if ( *str == '|' ) {
			com_token[ len++ ] = *str++;
			com_tokentype = TK_OR;
		}
		break;

	// &, &&
	case '&':
		com_token[ len++ ] = *str++;
		if ( *str == '&' ) {
			com_token[ len++ ] = *str++;
			com_tokentype = TK_AND;
		}
		break;

	// rest of the charset
	default:
		com_token[ len++ ] = *str++;
		while ( !is_separator[ (c = *str) ] ) {
			if ( len < MAX_TOKEN_CHARS-1 )
				com_token[ len++ ] = c;
			str++;
		}
		com_tokentype = TK_STRING;
		break;

	} // switch ( *str )

	com_tokenline = com_lines - shift;
	com_token[ len ] = '\0';
	*data_p = ( char * )str;
	return com_token;
}


/*
=================
SkipBracedSection

The next token should be an open brace or set depth to 1 if already parsed it.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
qboolean SkipBracedSection( const char **program, int depth ) {
	const char			*token;

	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );

	return (qboolean)( depth == 0 );
}


/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine( const char **data ) {
	const char *p;
	int		c;

	p = *data;

	if ( !*p )
		return;

	while ( (c = *p) != '\0' ) {
		p++;
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}

int Hex( char c )
{
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}
	else
	if ( c >= 'A' && c <= 'F' ) {
		return 10 + c - 'A';
	}
	else
	if ( c >= 'a' && c <= 'f' ) {
		return 10 + c - 'a';
	}

	return -1;
}

//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[8];
} hunk_t;

byte	*hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

qboolean	hunk_tempactive;
int		hunk_tempmark;

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check (void)
{
	hunk_t	*h;
	
	for (h = (hunk_t *)hunk_base ; (byte *)h != hunk_base + hunk_low_used ; )
	{
		if (h->sentinal != HUNK_SENTINAL)
			Error ("Hunk_Check: trahsed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Error ("Hunk_Check: bad size");
		h = (hunk_t *)((byte *)h+h->size);
	}
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void Hunk_Print (qboolean all)
{
	hunk_t	*h, *next, *endlow, *starthigh, *endhigh;
	int		count, sum;
	int		totalblocks;
	char	name[9];

	name[8] = 0;
	count = 0;
	sum = 0;
	totalblocks = 0;
	
	h = (hunk_t *)hunk_base;
	endlow = (hunk_t *)(hunk_base + hunk_low_used);
	starthigh = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	endhigh = (hunk_t *)(hunk_base + hunk_size);

	Log_Printf ("          :%8i total hunk size\n", hunk_size);
	Log_Printf ("-------------------------\n");

	while (1)
	{
	//
	// skip to the high hunk if done with low hunk
	//
		if ( h == endlow )
		{
			Log_Printf ("-------------------------\n");
			Log_Printf ("          :%8i REMAINING\n", hunk_size - hunk_low_used - hunk_high_used);
			Log_Printf ("-------------------------\n");
			h = starthigh;
		}
		
	//
	// if totally done, break
	//
		if ( h == endhigh )
			break;

	//
	// run consistancy checks
	//
		if (h->sentinal != HUNK_SENTINAL)
			Error ("Hunk_Check: trahsed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Error ("Hunk_Check: bad size");
			
		next = (hunk_t *)((byte *)h+h->size);
		count++;
		totalblocks++;
		sum += h->size;

	//
	// print the single block
	//
		memcpy (name, h->name, 8);
		if (all)
			Log_Printf ("%8p :%8i %8s\n",h, h->size, name);
			
	//
	// print the total
	//
		if (next == endlow || next == endhigh || 
		strncmp (h->name, next->name, 8) )
		{
			if (!all)
				Log_Printf ("          :%8i %8s (TOTAL)\n",sum, name);
			count = 0;
			sum = 0;
		}

		h = next;
	}

	Log_Printf ("-------------------------\n");
	Log_Printf ("%8i total blocks\n", totalblocks);
	
}

/*
===================
Hunk_AllocName
===================
*/
void *Hunk_AllocName( int size, const char *name )
{
	hunk_t	*h;
	
#if !defined(NDEBUG) || defined(_DEBUG)
	Hunk_Check ();
#endif

	if ( size < 0 ) {
		Error( "Hunk_Alloc: bad size: %i", size );
    }
	size = sizeof(hunk_t) + ((size+15)&~15);
	
	if ( hunk_size - hunk_low_used - hunk_high_used < size ) {
		Error( "Hunk_Alloc: failed on %i bytes", size );
    }
	
	h = (hunk_t *)( hunk_base + hunk_low_used );
	hunk_low_used += size;

	memset( h, 0, size );
	
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	strncpy( h->name, name, 8 );
	
	return (void *)( h + 1 );
}

/*
===================
Hunk_Alloc
===================
*/
void *Hunk_Alloc( int size )
{
	return Hunk_AllocName( size, "unknown" );
}

int	Hunk_LowMark( void )
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark( int mark )
{
	if ( mark < 0 || mark > hunk_low_used ) {
		Error( "Hunk_FreeToLowMark: bad mark %i", mark );
    }
	memset( hunk_base + mark, 0, hunk_low_used - mark );
	hunk_low_used = mark;
}

int	Hunk_HighMark( void )
{
	if ( hunk_tempactive ) {
		hunk_tempactive = qfalse;
		Hunk_FreeToHighMark( hunk_tempmark );
	}

	return hunk_high_used;
}

void Hunk_FreeToHighMark( int mark )
{
	if ( hunk_tempactive ) {
		hunk_tempactive = qfalse;
		Hunk_FreeToHighMark( hunk_tempmark );
	}
	if ( mark < 0 || mark > hunk_high_used ) {
		Error( "Hunk_FreeToHighMark: bad mark %i", mark );
    }
	memset( hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark );
	hunk_high_used = mark;
}


/*
===================
Hunk_HighAllocName
===================
*/
void *Hunk_HighAllocName( int size, const char *name )
{
	hunk_t	*h;

	if ( size < 0 ) {
	    Error( "Hunk_HighAllocName: bad size: %i", size );
    }

	if ( hunk_tempactive ) {
		Hunk_FreeToHighMark( hunk_tempmark );
		hunk_tempactive = qfalse;
	}

#if !defined(NDEBUG) || defined(_DEBUG)
	Hunk_Check();
#endif

	size = sizeof(hunk_t) + ((size+15)&~15);

	if ( hunk_size - hunk_low_used - hunk_high_used < size) {
		Log_Printf( "Hunk_HighAlloc: failed on %i bytes\n", size );
		return NULL;
	}

	hunk_high_used += size;

	h = (hunk_t *)( hunk_base + hunk_size - hunk_high_used );

	memset( h, 0, size );
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	strncpy( h->name, name, 8 );

	return (void *)( h + 1 );
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void *Hunk_TempAlloc( int size )
{
	void	*buf;

	size = (size+15)&~15;
	
	if ( hunk_tempactive ) {
		Hunk_FreeToHighMark( hunk_tempmark );
		hunk_tempactive = qfalse;
	}
	
	hunk_tempmark = Hunk_HighMark ();

	buf = Hunk_HighAllocName( size, "temp" );

	hunk_tempactive = qtrue;

	return buf;
}

