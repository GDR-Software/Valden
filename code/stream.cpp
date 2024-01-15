#include "gln.h"
#include "idatastream.h"
#include "stream.h"

IDataStream::IDataStream()
{ }

IDataStream::~IDataStream()
{ }

/////////////////////////////////////////////////////////////////////////////
// File construction/destruction

MemStream::MemStream( void )
{
	m_nGrowBytes = 1024;
	m_nPosition = 0;
	m_nBufferSize = 0;
	m_nFileSize = 0;
	m_pBuffer = NULL;
	m_bAutoDelete = true;
}

MemStream::MemStream( uint64_t nLen )
{
	m_nGrowBytes = 1024;
	m_nPosition = 0;
	m_nBufferSize = 0;
	m_nFileSize = 0;
	m_pBuffer = NULL;
	m_bAutoDelete = true;

	GrowFile( nLen );
}

FileStream::FileStream( void )
{
	m_hFile = NULL;
	m_bCloseOnDelete = true;
}

MemStream::~MemStream()
{
	if ( m_pBuffer ) {
		Close();
	}

	m_nGrowBytes = 0;
	m_nPosition = 0;
	m_nBufferSize = 0;
	m_nFileSize = 0;
}

FileStream::~FileStream()
{
	if ( m_hFile != NULL && m_bCloseOnDelete ) {
		Close();
	}
}

/////////////////////////////////////////////////////////////////////////////
// File operations

char* MemStream::ReadString( char* pBuf, uint64_t nMax )
{
	uint64_t nRead = 0;
	byte ch;

	if ( nMax <= 0 ) {
		return NULL;
	}
	if ( m_nPosition >= m_nFileSize ) {
		return NULL;
	}

	while ( ( --nMax ) )
	{
		if ( m_nPosition == m_nFileSize ) {
			break;
		}

		ch = m_pBuffer[m_nPosition];
		m_nPosition++;
		pBuf[nRead++] = ch;

		if ( ch == '\n' ) {
			break;
		}
	}

	pBuf[nRead] = '\0';
	return (char *)pBuf;
}

char* FileStream::ReadString( char* pBuf, uint64_t nMax )
{
	return fgets( pBuf, nMax, m_hFile );
}

uint64_t MemStream::Read( void* pBuf, uint64_t nCount )
{
	if ( nCount == 0 ) {
		return 0;
	}

	if ( m_nPosition > m_nFileSize ) {
		return 0;
	}

	uint64_t nRead;
	if ( m_nPosition + nCount > m_nFileSize ) {
		nRead = (uint64_t)( m_nFileSize - m_nPosition );
	}
	else{
		nRead = nCount;
	}

	memcpy( (byte*)pBuf, (byte*)m_pBuffer + m_nPosition, nRead );
	m_nPosition += nRead;

	return nRead;
}

uint64_t FileStream::Read( void* pBuf, uint64_t nCount )
{
	return fread( pBuf, 1, nCount, m_hFile );
}

int MemStream::GetChar( void )
{
	if ( m_nPosition > m_nFileSize ) {
		return 0;
	}

	byte* ret = (byte*)m_pBuffer + m_nPosition;
	m_nPosition++;

	return *ret;
}

int FileStream::GetChar( void )
{
	return fgetc( m_hFile );
}

uint64_t MemStream::Write( const void* pBuf, uint64_t nCount )
{
	if ( nCount == 0 ) {
		return 0;
	}

	if ( m_nPosition + nCount > m_nBufferSize ) {
		GrowFile( m_nPosition + nCount );
	}

	memcpy( (byte*)m_pBuffer + m_nPosition, (byte*)pBuf, nCount );

	m_nPosition += nCount;

	if ( m_nPosition > m_nFileSize ) {
		m_nFileSize = m_nPosition;
	}

	return nCount;
}

uint64_t FileStream::Write( const void* pBuf, uint64_t nCount )
{
	return fwrite( pBuf, 1, nCount, m_hFile );
}

int MemStream::PutChar( int c )
{
	if ( m_nPosition + 1 > m_nBufferSize ) {
		GrowFile( m_nPosition + 1 );
	}

	byte* bt = (byte*)m_pBuffer + m_nPosition;
	*bt = c;

	m_nPosition++;

	if ( m_nPosition > m_nFileSize ) {
		m_nFileSize = m_nPosition;
	}

	return 1;
}

/*!\todo SPoG suggestion: replace printf with operator >> using c++ iostream and strstream */
void FileStream::printf( const char* s, ... )
{
	va_list args;

	va_start( args, s );
	vfprintf( m_hFile, s, args );
	va_end( args );
}

/*!\todo SPoG suggestion: replace printf with operator >> using c++ iostream and strstream */
void MemStream::printf( const char* s, ... )
{
	va_list args;

	char buffer[4096];
	va_start( args, s );
	vsprintf( buffer, s, args );
	va_end( args );
	Write( buffer, strlen( buffer ) );
}

int FileStream::PutChar( int c )
{
	return fputc( c, m_hFile );
}

bool FileStream::Open( const char *filename, const char *mode )
{
	m_hFile = fopen( filename, mode );
	m_bCloseOnDelete = true;

	return m_hFile != NULL;
}

void MemStream::Close( void )
{
	m_nGrowBytes = 0;
	m_nPosition = 0;
	m_nBufferSize = 0;
	m_nFileSize = 0;
	if ( m_pBuffer && m_bAutoDelete ) {
		FreeMemory( m_pBuffer );
	}
	m_pBuffer = NULL;
}

void FileStream::Close( void )
{
	if ( m_hFile != NULL ) {
		fclose( m_hFile );
	}

	m_hFile = NULL;
	m_bCloseOnDelete = false;
}

uint64_t MemStream::Seek( uint64_t lOff, int nFrom )
{
	uint64_t lNewPos = m_nPosition;

	if ( nFrom == SEEK_SET ) {
		lNewPos = lOff;
	}
	else if ( nFrom == SEEK_CUR ) {
		lNewPos += lOff;
	}
	else if ( nFrom == SEEK_END ) {
		lNewPos = m_nFileSize + lOff;
	}
	else{
		return (uint64_t)0;
	}

	m_nPosition = lNewPos;

	return m_nPosition;
}

uint64_t FileStream::Seek( uint64_t lOff, int nFrom )
{
	fseek( m_hFile, lOff, nFrom );

	return ftell( m_hFile );
}

uint64_t MemStream::GetPosition( void ) const
{
	return m_nPosition;
}

uint64_t FileStream::GetPosition( void ) const
{
	return ftell( m_hFile );
}

void MemStream::GrowFile( uint64_t nNewLen )
{
	if ( nNewLen > m_nBufferSize ) {
		// grow the buffer
		uint64_t nNewBufferSize = m_nBufferSize;

		// determine new buffer size
		while ( nNewBufferSize < nNewLen )
			nNewBufferSize += m_nGrowBytes;

		// allocate new buffer
		byte* lpNew;
		if ( m_pBuffer == NULL ) {
			lpNew = static_cast<byte*>( GetMemory( nNewBufferSize ) );
		}
		else {
			lpNew = static_cast<byte*>( GetResizedMemory( m_pBuffer, nNewBufferSize ) );
		}

		m_pBuffer = lpNew;
		m_nBufferSize = nNewBufferSize;
	}
}

void MemStream::Flush(void)
{
	// Nothing to be done
}

void FileStream::Flush(void)
{
	if ( m_hFile == NULL ) {
		return;
	}

	fflush( m_hFile );
}

void MemStream::Abort(void)
{
	Close();
}

void FileStream::Abort(void)
{
	if ( m_hFile != NULL ) {
		// close but ignore errors
		if ( m_bCloseOnDelete ) {
			fclose( m_hFile );
		}
		m_hFile = NULL;
		m_bCloseOnDelete = false;
	}
}

void MemStream::SetLength( uint64_t nNewLen )
{
	if ( nNewLen > m_nBufferSize ) {
		GrowFile( nNewLen );
	}

	if ( nNewLen < m_nPosition ) {
		m_nPosition = nNewLen;
	}

	m_nFileSize = nNewLen;
}

void FileStream::SetLength( uint64_t nNewLen )
{
	fseek( m_hFile, nNewLen, SEEK_SET );
}

uint64_t MemStream::GetLength( void ) const
{
	return m_nFileSize;
}

uint64_t FileStream::GetLength( void ) const
{
	uint64_t nLen, nCur;

	// Seek is a non const operation
	nCur = ftell( m_hFile );
	fseek( m_hFile, 0, SEEK_END );
	nLen = ftell( m_hFile );
	fseek( m_hFile, nCur, SEEK_SET );

	return nLen;
}

FILE *FileStream::GetStream( void )
{
	return m_hFile;
}
