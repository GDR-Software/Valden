/*

file.h: slightly modified version from gtkradiant
*/

#ifndef __FILE_MEM_STREAM__
#define __FILE_MEM_STREAM__

#pragma once

class MemStream : public IDataStream
{
public:
    MemStream(void);
    MemStream(uint64_t nLen);
    virtual ~MemStream();

    uint32_t mRefCount;

    virtual void IncRef(void) override { mRefCount++; }
    virtual void DecRef(void) override
    {
        mRefCount--;
        if ( !mRefCount )
            delete this; // FIXME...?
    }
protected:
    uint64_t m_nGrowBytes;
    uint64_t m_nPosition;
    uint64_t m_nBufferSize;
    uint64_t m_nFileSize;
    byte* m_pBuffer;
    bool m_bAutoDelete;
    void GrowFile( uint64_t nNewLen );
public:
    virtual uint64_t GetPosition( void ) const override;
    virtual uint64_t Seek( uint64_t lOff, int nFrom ) override;
    virtual void SetLength( uint64_t nNewLen ) override;
    virtual uint64_t GetLength( void ) const override;

    const byte* GetBuffer( void ) const
    { return m_pBuffer; }
    byte* GetBuffer( void )
    { return m_pBuffer; }

    virtual char* ReadString( char* pBuf, uint64_t nMax ) override;
    virtual uint64_t Read( void* pBuf, uint64_t nCount ) override;
    virtual uint64_t Write( const void* pBuf, uint64_t nCount ) override;
    virtual int GetChar( void ) override;
    virtual int PutChar( int c ) override;

    virtual void printf( const char*, ... ) override; ///< \todo implement on MemStream

    virtual void Abort( void ) override;
    virtual void Flush( void ) override;
    virtual void Close( void ) override;
    bool Open( const char *filename, const char *mode );
};

class FileStream : public IDataStream
{
public:
    FileStream(void);
    virtual ~FileStream();

    uint32_t mRefCount;
    virtual void IncRef(void) override { mRefCount++; }
    virtual void DecRef(void) override
    {
    	mRefCount--;
        if ( !mRefCount )
    		delete this; // FIXME...?
    }

protected:
// DiscFile specific:
    FILE* m_hFile;
    bool m_bCloseOnDelete;

public:
    FILE *GetStream(void);
    virtual uint64_t GetPosition( void ) const override;
    virtual uint64_t Seek( uint64_t lOff, int nFrom ) override;
    virtual void SetLength( uint64_t nNewLen ) override;
    virtual uint64_t GetLength( void ) const override;

    virtual char* ReadString( char* pBuf, uint64_t nMax ) override;
    virtual uint64_t Read( void* pBuf, uint64_t nCount ) override;
    virtual uint64_t Write( const void* pBuf, uint64_t nCount ) override;
    virtual int GetChar( void ) override;
    virtual int PutChar( int c ) override;

    virtual void printf( const char*, ... ) override; ///< completely matches the usual printf behaviour

    virtual void Abort( void ) override;
    virtual void Flush( void ) override;
    virtual void Close( void ) override;
    bool Open( const char *filename, const char *mode );
};

#endif