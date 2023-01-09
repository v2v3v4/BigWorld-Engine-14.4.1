#ifndef FILE_STREAM_HPP
#define FILE_STREAM_HPP

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cstdmf/bw_list.hpp"
#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class provides a file stream that is compliant with the stream formats
 *  imposed by BinaryStreams.  Unfortunately, the similar class in cstdmf
 *  doesn't use the same format for packing string lengths and therefore
 *  couldn't be used where this exact stream format is required.
 *
 *  It provides automatic limiting of the number of
 *  simultaneously open file handles via an efficient most-recently-used caching
 *  system.
 *
 *  I/O errors encountered during disk operations should be checked for by
 *  calling good() or error() after any call to an I/O operation.
 */
class FileStream : public MemoryOStream
{
public:
	static const int INIT_READ_BUF_SIZE = 128;

	FileStream( const char * pPath, const char * pMode );
	virtual ~FileStream();

	bool good() const;

	/*
	 *	Interface: MemoryOStream->BinaryIStream
	 */
	bool error() const;
	virtual const void *retrieve( int nBytes );

	const char * strerror() const;

	long tell();
	int seek( long offset, int whence = SEEK_SET );
	long length();
	bool commit();
	int stat( struct stat * pStatInfo );

protected:
	void setMode( char mode );
	bool open();
	void close();
	void remove();

	BW::string path_;
	BW::string mode_;
	FILE *pFile_;
	char *pReadBuf_;
	int readBufSize_;
	BW::string errorMsg_;

	// This is either 0, 'r', or 'w'
	char lastAction_;

	// Whether or not the underlying FILE* is actually open or not
	bool isOpen_;

	// The file offset as at the last time close() was called
	long offset_;

	// Static management of maximum number of open FileStream handles
	typedef BW::list< FileStream * > FileStreams;
	static FileStreams s_openFiles_;
	static const unsigned MAX_OPEN_FILES = 20;

	// An iterator pointing to this FileStream's position in s_openFiles_
	FileStreams::iterator fileStreamsIter_;
};


#if defined( CODE_INLINE )
#include "file_stream.ipp"
#endif

BW_END_NAMESPACE

#endif
