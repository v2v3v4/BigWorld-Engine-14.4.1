#include "pch.hpp"

#include "win_file_streamer.hpp"


BW_BEGIN_NAMESPACE

WinFileStreamer::WinFileStreamer( HANDLE hFile ) : hFile_( hFile )
{
	MF_ASSERT( hFile_ != NULL );
}


WinFileStreamer::~WinFileStreamer()
{
	MF_ASSERT( memoryMappings_.empty() ); // Someone hasn't unmapped something
	CloseHandle( hFile_ );
	hFile_ = NULL;
}


size_t WinFileStreamer::read( size_t nBytes, void* buffer )
{
	MF_ASSERT( hFile_ != NULL );
	DWORD actuallyRead = 0;
	if (!::ReadFile( hFile_, buffer, 
		static_cast<DWORD>( nBytes ), &actuallyRead, NULL ))
	{
		return 0;
	}
	else
	{
		return static_cast<size_t>( actuallyRead );
	}
}


bool WinFileStreamer::skip( int nBytes )
{
	MF_ASSERT( hFile_ != NULL );
	return ::SetFilePointer( hFile_, nBytes,
		NULL, FILE_CURRENT ) != INVALID_SET_FILE_POINTER;
}


bool WinFileStreamer::setOffset( size_t offset )
{
	MF_ASSERT( hFile_ != NULL );
	return ::SetFilePointer( hFile_, static_cast<LONG>( offset ), NULL,
		FILE_BEGIN ) != INVALID_SET_FILE_POINTER;
}


size_t WinFileStreamer::getOffset() const
{
	MF_ASSERT( hFile_ != NULL );
	return static_cast<size_t>( ::SetFilePointer( hFile_, 0,
		NULL, FILE_CURRENT ) );
}


void* WinFileStreamer::memoryMap( size_t userOffset, size_t userLength, bool writable )
{
	MF_ASSERT( hFile_ != NULL );
	SimpleMutexHolder smh( memoryMappingsLock_ );
	
	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );
	DWORD allocGranularity = (size_t)systemInfo.dwAllocationGranularity;

	size_t fileSize = (size_t)::GetFileSize( hFile_, NULL );

	if (userLength == 0)
	{
		if (userOffset >= fileSize )
		{
			CloseHandle( hFile_ );
			return NULL;
		}

		userLength = fileSize - userOffset;
	}

	// Offsets must be aligned on correct boundary. Move offset backwards and 
	// length forwards as much as necessary.
	size_t mappedOffset = static_cast<size_t>(
		int(userOffset / allocGranularity) * allocGranularity );
	size_t mappedLength = userLength + (userOffset - mappedOffset);

	HANDLE hMapping = ::CreateFileMapping( hFile_, NULL,
		writable ? PAGE_WRITECOPY : PAGE_READONLY, 0L, 
		(DWORD)(mappedOffset+mappedLength), NULL );
	if (!hMapping)
	{
		return NULL;
	}

	void* mappedPtr = ::MapViewOfFile( hMapping,
		writable ? FILE_MAP_COPY : FILE_MAP_READ, 0,
		(DWORD)mappedOffset, (DWORD)mappedLength);
	if (!mappedPtr)
	{
		CloseHandle( hMapping );
		return NULL;
	}

	void* userPtr =
		(void*)((unsigned char*)mappedPtr + (userOffset - mappedOffset));

	MemoryMapping mmf = 
	{
		writable,
		hMapping,
		fileSize,
		mappedOffset,
		userOffset,
		mappedLength,
		userLength,
		mappedPtr,
		userPtr
	};
	memoryMappings_.push_back( mmf );

	return userPtr;
}


void WinFileStreamer::memoryUnmap( void * ptr )
{
	MF_ASSERT( hFile_ != NULL );
	SimpleMutexHolder smh( memoryMappingsLock_ );
	MemoryMappings::iterator it =
		std::find_if( memoryMappings_.begin(),
		memoryMappings_.end(),
		MemoryMapping::CompareFunc(ptr) );

	// Someone is trying to unmap a pointer not mapped by us.
	MF_ASSERT( it != memoryMappings_.end() );
	if (it == memoryMappings_.end())
	{
		return;
	}

	const MemoryMapping& mmf = *it;

	::UnmapViewOfFile( mmf.mappedPtr_ );
	::CloseHandle( mmf.hFileMapping_ );

	std::swap( *it, memoryMappings_.back() );
	memoryMappings_.pop_back();
}

BW_END_NAMESPACE
