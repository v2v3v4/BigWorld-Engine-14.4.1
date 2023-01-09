#ifndef FILE_STREAMER_HPP
#define FILE_STREAMER_HPP

#include "cstdmf/smartpointer.hpp"

#include <sys/types.h>

BW_BEGIN_NAMESPACE

/**
 *	This class provides an abstract interface to a file streamer
 *	
 *	The interface provides the following methods:
 *	read:
 *	 nBytes - the number of bytes to read
 *	 buffer - the buffer to read into
 *   return the actual read size
 *	skip:
 *	 nBytes - the number of bytes to skip in the stream
 *   return - true if successful, false if not
 *	setOffset:
 *	 offset - new offset into the file
 *   return true if successful, false if not
 */	
class IFileStreamer : public SafeReferenceCount
{
public:
	virtual size_t read( size_t nBytes, void* buffer ) = 0;
	virtual bool skip( int nBytes ) = 0;
	virtual bool setOffset( size_t offset ) = 0;
	virtual size_t getOffset() const = 0;

	virtual void* memoryMap( size_t offset, size_t length, bool writable ) = 0;
	virtual void memoryUnmap( void * p ) = 0;
};

BW_END_NAMESPACE

#endif // FILE_STREAMER_HPP

