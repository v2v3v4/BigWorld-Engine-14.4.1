#ifndef FILE_HANDLE_STREAMER_HPP
#define FILE_HANDLE_STREAMER_HPP

#include "file_streamer.hpp"
#include "file_system.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements the IFileStreamer interface
 *	for a file handle
 */
class FileHandleStreamer : public IFileStreamer
{
public:
	FileHandleStreamer( FILE* file );
	virtual ~FileHandleStreamer();

	virtual size_t read( size_t nBytes, void* buffer );
	virtual bool skip( int nBytes );
	virtual bool setOffset( size_t offset );
	virtual size_t getOffset() const;

	virtual void* memoryMap( size_t offset, size_t length, bool writable );
	virtual void memoryUnmap( void * p );

private:
	FILE*	file_;
};

BW_END_NAMESPACE

#endif // FILE_HANDLE_STREAMER_HPP

