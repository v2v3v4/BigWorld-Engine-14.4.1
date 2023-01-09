#include "pch.hpp"

#include "file_handle_streamer.hpp"

BW_BEGIN_NAMESPACE

FileHandleStreamer::FileHandleStreamer(FILE* file)
: file_( file )
{

}


FileHandleStreamer::~FileHandleStreamer()
{
	if (file_ != NULL)
	{
		fclose( file_ );
	}
}


size_t FileHandleStreamer::read( size_t nBytes, void* buffer )
{
	MF_ASSERT( file_ );
	return fread( buffer, nBytes, 1, file_ ) * nBytes;
}


bool FileHandleStreamer::skip( int nBytes )
{
	MF_ASSERT( file_ );
	return fseek( file_, nBytes, SEEK_CUR ) == 0;
}


bool FileHandleStreamer::setOffset( size_t offset )
{
	MF_ASSERT( file_ );
	return fseek( file_, static_cast<long>(offset), SEEK_SET ) == 0;

}

size_t FileHandleStreamer::getOffset() const
{
	return ftell( file_ );
}


void* FileHandleStreamer::memoryMap( size_t offset, size_t length, bool writable )
{
	CRITICAL_MSG( "Unimplemented" );
	return NULL;
}


void FileHandleStreamer::memoryUnmap( void * p )
{
	CRITICAL_MSG( "Unimplemented" );
}



BW_END_NAMESPACE

// file_handle_streamer.cpp
