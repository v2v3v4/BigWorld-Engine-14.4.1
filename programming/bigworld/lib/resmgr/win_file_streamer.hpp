#ifndef WIN_FILE_STREAMER_HPP
#define WIN_FILE_STREAMER_HPP

#include "file_streamer.hpp"
#include "file_system.hpp"

#include "cstdmf/bw_vector.hpp"

#include <Windows.h>

BW_BEGIN_NAMESPACE

/**
 *	This class implements the IFileStreamer interface
 *	for the Windows file system.
 */
class WinFileStreamer : public IFileStreamer
{
public:
	WinFileStreamer( HANDLE hFile );
	virtual ~WinFileStreamer();

	virtual size_t read( size_t nBytes, void* buffer );
	virtual bool skip( int nBytes );
	virtual bool setOffset( size_t offset );
	virtual size_t getOffset() const;

	virtual void* memoryMap( size_t offset, size_t length, bool writable );
	virtual void memoryUnmap( void * p );

private:
	HANDLE hFile_;

	struct MemoryMapping
	{
		bool writable_;
		HANDLE hFileMapping_;
		size_t fileSize_;
		size_t mappedOffset_;
		size_t userOffset_;
		size_t mappedLength_;
		size_t userLength_;
		void* mappedPtr_;
		void* userPtr_;

		struct CompareFunc : public std::unary_function<MemoryMapping, void*>
		{
			CompareFunc( void* ptr ) : ptr_(ptr) { }
			bool operator()(const MemoryMapping& other)
			{
				return ptr_ == other.userPtr_;
			}

			void* ptr_;
		};
	};

	typedef BW::vector<MemoryMapping> MemoryMappings;
	MemoryMappings memoryMappings_;
	SimpleMutex memoryMappingsLock_;
};

BW_END_NAMESPACE

#endif // FILE_HANDLE_STREAMER_HPP

