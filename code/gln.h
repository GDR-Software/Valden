#ifndef __GLN__
#define __GLN__

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include "imgui.h"
#include "imgui_internal.h"
#include "nlohmann/json.hpp"
#include "idatastream.h"
#include "gln_files.h"

#if defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
	#define FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER) || defined(_MSVC_VER)
	#define FORCEINLINE __forceinline
#else
	#define FORCEINLINE

	#warning "Unsupported compiler"
#endif

using json = nlohmann::json;

#define WINDOW_TITLE "SIR Editor"
#define PROJECT_FILE_NAME "proj"
#define MAX_NPATH 64

#ifdef _WIN32
    #define PATH_SEP '\\'
	#define DECL __cdecl
#else
    #define PATH_SEP '/'
	#define DECL
#endif

void Sys_DebugAssertion( const char *expr, const char *file, unsigned line, const char *func );
#define Assert(x) if ( !( x ) ) { Sys_DebugAssertion( #x, __FILE__, __LINE__, __func__ ); }

template<typename T>
FORCEINLINE void Swap( T& a, T& b ) {
	T& c = a;
	a = b;
	b = c;
}

#include "gln_types.h"

#define HEADER_MAGIC 0x5f3759df
#define BFF_IDENT (('B'<<24)+('F'<<16)+('F'<<8)+'I')
#define MAX_BFFPATH 256
#define BFF_VERSION_MAJOR 0
#define BFF_VERSION_MINOR 1
#define BFF_VERSION ((BFF_VERSION_MAJOR<<8)+BFF_VERSION_MINOR)

typedef struct
{
	char *chunkName;
	int64_t chunkNameLen;
	int64_t chunkSize;
	char *chunkBuffer;
} bff_chunk_t;

typedef struct
{
	int64_t ident;
	int64_t magic;
	int64_t numChunks;
	int64_t compression;
	int16_t version;
} bffheader_t;

typedef struct
{
	char bffGamename[MAX_BFFPATH];
	bffheader_t header;
	
	int64_t numChunks;
	bff_chunk_t* chunkList;
} bff_t;

#define VALDEN_VERSION "1.1.0"
#define VALDEN_ABOUTMSG "This is the official map/level editor for \"The Nomad\" game"

#define arraylen(x) (sizeof((x))/sizeof((*(x))))

template<typename A, typename B, typename C>
inline A clamp( const A& value, const B& min, const C& max )
{
    if (value > max) {
        return max;
    } else if (value < min) {
        return min;
    }
    return value;
}

extern bool g_UnsafeExit;
extern int parm_compression;
extern int myargc;
extern char **myargv;

#if defined( __cplusplus )
extern "C"
{
#endif

// NOTE: might want to switch to bits if needed
#define SYS_VRB 0 ///< verbose support (on/off)
#define SYS_STD 1 ///< standard print level - this is the default
#define SYS_WRN 2 ///< warnings
#define SYS_ERR 3 ///< error
#define SYS_NOCON 4 ///< no console, only print to the file (useful whenever Sys_Printf and output IS the problem)

/*!
   those are the real implementation
 */
void Sys_Printf_VA( const char *text, va_list args ); ///< matches PFN_SYN_PRINTF_VA prototype
void Sys_FPrintf_VA( int level, const char *text, va_list args );

/*!
   this is easy to call, wrappers around va_list version
 */
void Log_Printf( const char *text, ... );
void Log_FPrintf( int flag, const char *text, ... );

#if defined( __cplusplus )
};
#endif

const bool IsMap( const char *filename );
void Sys_LogFile( void );

void QE_ConvertDOSToUnixName( char *dst, const char *src );
char *BuildOSPath( const char *curPath, const char *gamepath, const char *npath );

void DoTextureListDlg( void );
void DoScriptsDlg( void );
void DoCommandListDlg( void );
void DoTextEditor( const char *filename, int cursorpos );

void ExitApp( void );
void GLN_CheckAutoSave( void );
void GLN_Init( void );
qboolean GLN_LoadProject( const char *projectfile );

void Sys_SetWindowTitle( const char *title );

void *GetMemory( uint64_t size );
void *GetResizedMemory( void *ptr, uint64_t size );
void FreeMemory( void *ptr );

qboolean ConfirmModified( void );

const char *va( const char *fmt, ... );
char *CopyString( const char *str );

double Sys_DoubleTime( void );
void Sys_LoadAsset( const char *title, float *progress, const std::function<void( void )>& loadFunc );

bff_t *bffOpenRead( const char *path );
FILE *bffOpenWrite( const char *path, bffheader_t *h );
void bffWriteChunk( const char *name, const void *data, int size, bffheader_t *h, FILE *fp );
void bffClose( bff_t *archive );

bool Parse3DMatrix( const char **buf_p, int z, int y, int x, float *m );
bool Parse2DMatrix( const char **buf_p, int y, int x, float *m );
bool Parse1DMatrix( const char **buf_p, int x, float *m );
int ParseHex( const char *text );

void COM_DefaultExtension( char *path, int maxSize, const char *extension );
qboolean COM_CompareExtension(const char *in, const char *ext);
void COM_StripExtension( const char *in, char *out, int destsize );
const char *COM_GetExtension( const char *name );
uint64_t Com_GenerateHashValue( const char *fname, const uint64_t size );

void Hunk_Init( void );

void *Hunk_Alloc( int size );
void *Hunk_AllocName( int size, const char *name );
void *Hunk_HighAllocName( int size, const char *name );

void Hunk_Check( void );
void *Hunk_TempAlloc( int size );

int Hunk_LowMark( void );
void Hunk_FreeToLowMark( int mark );

int Hunk_HighMark( void );
void Hunk_FreeToHighMark( int mark );

#define IMAGE_FILEDLG_FILTERS \
	".jpg,.jpeg,.png,.bmp,.tga,.webp," \
	"Jpeg Files (*.jpeg *.jpg){.jpeg,.jpg}," \
	"Image Files (*.jpg *.jpeg *.png *.bmp *.tga *.webp){.jpg,.jpeg,.png,.bmp,.tga,.webp}"

#define MAP_FILEDLG_FILTERS \
	".map," \
	"Map Files (*.map){.map}"

typedef enum {
	TK_GENEGIC = 0, // for single-char tokens
	TK_STRING,
	TK_QUOTED,
	TK_EQ,
	TK_NEQ,
	TK_GT,
	TK_GTE,
	TK_LT,
	TK_LTE,
	TK_MATCH,
	TK_OR,
	TK_AND,
	TK_SCOPE_OPEN,
	TK_SCOPE_CLOSE,
	TK_NEWLINE,
	TK_EOF,
} tokenType_t;

extern tokenType_t com_tokentype;

#define MAX_TOKEN_CHARS 1024

uint64_t COM_GetCurrentParseLine( void );
uintptr_t COM_Compress( char *data_p );
int Hex( char c );
void SkipRestOfLine( const char **data );
qboolean SkipBracedSection( const char **program, int depth );
const char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks );
const char *SkipWhitespace( const char *data, qboolean *hasNewLines );
char *COM_ParseComplex( const char **data_p, qboolean allowLineBreaks );
void COM_ParseError( const char *format, ... );
void COM_ParseWarning( const char *format, ... );
void COM_BeginParseSession( const char *name );

#define VectorClear( x ) ((x)[0]=(x)[1]=(x)[2]=0)
#define VectorCopy( dst, src ) ((dst)[0]=(src)[0],(dst)[1]=(src)[1],(dst)[2]=(src)[2])
#define VectorSet4( dst, r, g, b, a ) ((dst)[0]=(r),(dst)[1]=(g),(dst)[2]=(b),(dst)[3]=(a))

void *CopyMemory( const void *src, int nBytes );

void N_strncpyz( char *dest, const char *src, size_t count );
int N_stricmp( const char *s1, const char *s2 );
char *N_stradd(char *dst, const char *src);

int Sys_MessageBox( const char *title, const char *message, unsigned flags );
bool Q_mkdir( const char *name, int perms = 0750 );
bool FolderExists( const char *name );
uint64_t LoadFile( const char *filename, void **buffer );
bool FileExists( const char *filename );
uint64_t FileLength( FILE *fp );
void SafeRead( void *data, uint64_t size, IDataStream *stream );
void SafeRead( void *buffer, uint64_t size, FILE *fp );
void SafeWrite( const void *buffer, uint64_t size, FILE *fp );
void WriteFile( const char *filename, const void *buffer, uint64_t length );

int CheckParm( const char *name );
void Error( const char *fmt, ... );
void ParseCommandLine( char *lpCmdLine );

const char *COM_ParseExt( const char **text, qboolean allowLineBreaks );

typedef enum
{
	ET_ITEM,
	ET_WEAPON,
	ET_MOB,
	ET_BOT,
	ET_WALL, // a tile with pre-determined collision physics
	ET_PLAYR,
	
	NUMENTITYTYPES
} entitytype_t;

#ifdef VALDEN
#undef new
#undef delete

inline void *operator new( size_t sz ) {
    return GetMemory( sz );
}

inline void *operator new[]( size_t sz ) {
    return GetMemory( sz );
}

inline void operator delete( void *ptr ) {
    FreeMemory( ptr );
}

inline void operator delete[]( void *ptr, size_t ) {
    FreeMemory( ptr );
}
#endif

#define VectorCopy(dst,src) ((dst)[0]=(src)[0],(dst)[1]=(src)[1],(dst)[2]=(src)[2])

template<typename T>
using Reference = std::shared_ptr<T>;

template<typename T>
using Ref = std::shared_ptr<T>;

#endif