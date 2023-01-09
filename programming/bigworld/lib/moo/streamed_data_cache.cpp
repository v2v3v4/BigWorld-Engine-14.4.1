
#include "pch.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/binaryfile.hpp"
#include "cstdmf/slot_tracker.hpp"
#include "streamed_data_cache.hpp"
#include "resmgr/binary_block.hpp"

#include <io.h>
#include <process.h>

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

/** 
 *	The SEPARATE_READ_THREAD token specifies whether streamed animations
 *	should use a separate thread for file read operations.
 *
 *	The previous version of this class used window's overlapped (asynchronous)
 *  file access, but has been replaced with the BWResource file system POSIX
 *  file methods.
 */

namespace Moo
{
MEMTRACKER_DECLARE( StreamedDataCache_load, "StreamedDataCache_load",  0 );

const int StreamedDataCache::CACHE_VERSION = 6;


#ifdef SEPARATE_READ_THREAD
// -----------------------------------------------------------------------------
// Section: ReadThread
// -----------------------------------------------------------------------------



#define READREQMAX 32
static HANDLE s_readThread = INVALID_HANDLE_VALUE;

/**
 * This structure holds a read request for a file.
 */
struct ReadReq
{
	FILE*							file;
	BinaryPtr						pData;
	StreamedDataCache::TrackerPtr	pTracker;
};

static ReadReq readReqs[READREQMAX];
uint readReqIn = 0;
uint readReqOut = 0;
SimpleMutex readReqMtx;
SimpleSemaphore	readReqListIn;
SimpleSemaphore	readReqListOut;

void addReadReq( const ReadReq & rr )
{
	BW_GUARD;
	readReqListIn.pull();

	readReqs[ (readReqIn++) & (READREQMAX-1) ] = rr;

	readReqListOut.push();
}


void __cdecl read_start( void * arg )
{
	BW_GUARD;
#if ENABLE_PROFILER && !defined(MF_SERVER)
	// Threads add themselves to the profiling system by default.
	g_profiler.addThisThread("Moo::read_start");
#endif
	for (uint i = 0; i < READREQMAX; i++) readReqListIn.push();

	while (1)
	{
		readReqListOut.pull();

		ReadReq & rr = readReqs[ (readReqOut++) & (READREQMAX-1) ];
		readReqMtx.grab();
		rr.pTracker->finished_ = 0;
		readReqMtx.give();

		clearerr( rr.file );
		MF_VERIFY( fseek( rr.file, rr.pTracker->offset_, SEEK_SET ) == 0 );
		size_t numRead = fread( rr.pData->cdata(), 1, rr.pData->len(), rr.file );

		if ( !ferror( rr.file ) )
		{
			readReqMtx.grab();
				// As far as this thread is concerned we are now finished with 
				// the tracker and block of data. However we must mark as 
				// finished before dereferencing.
				rr.pTracker->finished_ = 1;
				rr.pTracker = NULL;
				rr.pData	= NULL;
			readReqMtx.give();
		}
		else
		{
			ERROR_MSG( "StreamedDataCache::load: "
				"Read error: (off 0x%08X siz %d data 0x%p)\n",
				rr.pTracker->offset_, rr.pData->len(), rr.pData->data() );

			readReqMtx.grab();
				// As far as this thread is concerned we are now finished with 
				// the tracker and block of data. However we must mark as 
				// finished before dereferencing.
				rr.pTracker->finished_ = -1;
				rr.pTracker = NULL;
				rr.pData	= NULL;
			readReqMtx.give();
		}

		readReqListIn.push();
	}
}


#endif // SEPARATE_READ_THREAD

// -----------------------------------------------------------------------------
// Section: StreamedDataCache
// -----------------------------------------------------------------------------

bool StreamedDataCache::fileAccessDone( TrackerPtr tracker )
{
#ifdef SEPARATE_READ_THREAD
	readReqMtx.grab();
	bool result = tracker->finished_ != 0;
	readReqMtx.give();
	return result;
#else
	tracker->finished_ = 1;
	return true;
#endif // !SEPARATE_READ_THREAD
}

/**
 *	Constructor
 */
StreamedDataCache::StreamedDataCache( const BW::string & resourceID,
		bool createIfAbsent ) :
	lastEntry_( NULL ),
	deleteOnClose_( false )
{
	BW_GUARD;	
#ifdef SEPARATE_READ_THREAD
	if (s_readThread == INVALID_HANDLE_VALUE)
		s_readThread = HANDLE( _beginthread( read_start, 0, NULL ) );
#endif

	fileName_ = resourceID;

	// first try r/w for an existing
	file_ = BWResource::instance().fileSystem()->posixFileOpen( fileName_, "r+b" );
	if ( createIfAbsent )
	{
	// if it didn't work, try to create the file and open it
		if (!file_)
			file_ = BWResource::instance().fileSystem()->posixFileOpen( fileName_, "w+b" );
	}
	// if it didn't work still, then try read-only
	if (!file_)
		file_ = BWResource::instance().fileSystem()->posixFileOpen( fileName_, "rb" );

	// ok, we've opened the file.
	// now read in the directory
	if (file_)
		this->loadSelf();
}

/**
 *	Destructor
 */
StreamedDataCache::~StreamedDataCache()
{
	BW_GUARD;
	if (file_)
	{
		fclose( file_ );
	}

	if (deleteOnClose_)
	{
		BWResource::instance().fileSystem()->eraseFileOrDirectory( fileName_ );
	}

	while (!directory_.empty())
	{
		directory_.erase( directory_.begin() );
	}
}


/**
 *	Find the data for the given entry name.
 *
 *	If either minVersion or minModified is nonzero, then the version
 *	or modification time must be at least the value passed in.
 */
const StreamedDataCache::EntryInfo * StreamedDataCache::findEntryData(
	const BW::string & name, uint32 minVersion, uint64 minModified )
{
	BW_GUARD;
	EntryInfos::iterator found = directory_.find( name );
	if (found != directory_.end())
	{
		if (minVersion == 0 || minVersion <= found->second.version_)
		{
			if (minModified == 0 || minModified <= found->second.modifiedTime_)
			{
				return &found->second;
			}
		}
	}

	return NULL;
}

/**
 *	Add an entry under the given name.
 *	Returns the offset at which to begin writing.
 */
uint32 StreamedDataCache::addEntryData( const BW::string & name,
	uint32 version, uint64 modified )
{
	BW_GUARD;
	uint32 dataEnd = (lastEntry_ != NULL) ? lastEntry_->fileOffset_ +
		((lastEntry_->preloadSize_+lastEntry_->streamSize_ + 3) & ~3L) : 0;

	EntryInfo * ei = const_cast<EntryInfo*>(this->findEntryData( name, 0, 0 ));
	if (ei == NULL)
	{
		// it's really easy if there's not an entry already there
		EntryInfo entryInfo = { 0, 0, dataEnd, version, modified };
		lastEntry_ = &directory_.insert(
			std::make_pair( name, entryInfo ) ).first->second;
	}
	else
	{
		INFO_MSG( "StreamedDataCache::addEntryData: "
			"Replacing existing entry %s in cache.\n", name.c_str() );

		// first move up all the data
		const uint32 SHUFFLE_BUFFER_SIZE = 256*1024;
		char * dataBuffer = new char[SHUFFLE_BUFFER_SIZE];

		uint32 oldLen = ((ei->preloadSize_+ei->streamSize_ + 3) & ~3L);
		uint32 dataOffset = ei->fileOffset_ + oldLen;
		while (dataOffset < dataEnd)
		{
			uint32 thisLen = std::min( dataEnd - dataOffset, SHUFFLE_BUFFER_SIZE );
			this->preload( dataOffset, thisLen, dataBuffer );
			this->immsave( dataOffset - oldLen, thisLen, dataBuffer );
			dataOffset += thisLen;
		}
		dataEnd -= oldLen;

		bw_safe_delete_array(dataBuffer);

		// now fix up all the entry infos after us
		for (EntryInfos::iterator it = directory_.begin();
			it != directory_.end();
			it++)
		{
			EntryInfo * aei = &it->second;
			if (aei->fileOffset_ > ei->fileOffset_) aei->fileOffset_ -= oldLen;
		}

		// and finally fix up our entry info
		ei->preloadSize_ = 0;
		ei->streamSize_ = 0;
		ei->fileOffset_ = dataEnd;
		ei->version_ = version;
		ei->modifiedTime_ = modified;

		lastEntry_ = ei;	// we are now the last entry
	}

	return dataEnd;
}

/**
 *	Mark the end of the data for a newly added entry.
 *	The argument is the size of the entry data written.
 */
void StreamedDataCache::endEntryData( uint32 psize, uint32 ssize )
{
	BW_GUARD;
	lastEntry_->preloadSize_ = psize;
	lastEntry_->streamSize_ = ssize;
}


/**
 *	Load the given data spec immediately
 */
bool StreamedDataCache::preload( uint32 offset, uint32 size, void * data )
{
	BW_GUARD;
	MF_VERIFY( fseek( file_, offset, SEEK_SET ) == 0 );
	size_t numRead = fread( data, 1, size, file_ );

	return numRead == size;
}


/**
 *	Save the given data spec immediately
 */
bool StreamedDataCache::immsave( uint32 offset, uint32 size, const void * data )
{
	BW_GUARD;
	TrackerPtr tracker = this->save( offset, size, data );

	while ( !fileAccessDone( tracker ) )
		Sleep( 5 );

	bool result = ( tracker->finished_ == 1 );
	tracker = NULL;

	return result;
}


/**
 *	Start loading the given data spec
 */
StreamedDataCache::TrackerPtr StreamedDataCache::load(	uint32		offset, 
														BinaryPtr	pData )
{
	BW_GUARD_MEMTRACKER( StreamedDataCache_load );

	TrackerPtr tracker = new Tracker( offset );

	DWORD numRead = 0;
	BOOL ok;
#ifdef SEPARATE_READ_THREAD
	ReadReq rr = { file_, pData, tracker };
	addReadReq( rr );
	ok = true;
#else	
	clearerr( file_ );
	MF_VERIFY( fseek( file_, offset, SEEK_SET ) == 0 );
	numRead = fread( pData->cdata(), 1, pData->len(), file_ );
	ok = !ferror( file_ );
#endif	//!SEPARATE_READ_THREAD

	if ( !ok )
	{
		clearerr( file_ );
		ERROR_MSG( "StreamedDataCache::load: "
			"Read error (off 0x%08X siz %d data 0x%p)\n",
			offset, pData->len(), pData->cdata() );

		tracker = NULL;
	}

	return tracker;
}

/**
 *	Start saving the given data spec
 */
StreamedDataCache::TrackerPtr StreamedDataCache::save(
	uint32 offset, uint32 size, const void * data )
{
	BW_GUARD;
	size_t numSaved = 0;

	clearerr( file_ );
	MF_VERIFY( fseek( file_, offset, SEEK_SET ) == 0 );
	numSaved = fwrite( data, 1, size, file_ );
	if (ferror( file_ ))
	{
		clearerr( file_ );
		ERROR_MSG( "StreamedDataCache::save: "
			"Write error\n" );

		return NULL;
	}

	return new Tracker( offset, 1 );
}

namespace
{
	/*
	 *	This class helps with loading the entry info from the .anca files
	 */
	class EntryInfoLoader
	{
	public:
		// Constructor
		EntryInfoLoader( StreamedDataCache* pCache, uint32 fileSize ) :
		  valid_( false ),
		  fileSize_( fileSize ),
		  readOffset_( 0 ),
		  fileDataOffset_( 0 )
		{

			BW_GUARD;
			if (fileSize_ > sizeof(uint32))
			{
				// Check if there is enough room in the file for the directory
				if (pCache->preload(fileSize - sizeof(uint32), sizeof(uint32), &directorySize_) &&
					directorySize_ < (fileSize - sizeof(uint32)))
				{
					// load up the directory
					directoryStart_ = fileSize - sizeof(uint32) - directorySize_;
					pData_ = new char[directorySize_];
					if (pCache->preload(directoryStart_, directorySize_, pData_))
					{
						valid_ = true;
					}
				}
			}
		}

		~EntryInfoLoader()
		{
			// Delete our data
			bw_safe_delete_array(pData_);
		}

		/* 
		 *	This method loads up the entryinfo for the next entry in the file
		 *	@return true if an entry was successfully loaded, false if not
		 */
		bool nextEntry()
		{	

			BW_GUARD;
			// Return false if we are at the end of the file
			// Or if the file is invalid
			if (!valid_ || readOffset_ >= directorySize_)
				return false;

			// The entry data length has its high bit set
			// so need a msk to mask it out
			const uint32 ENTRY_DATA_MASK = ~(1<<31);

			uint32 entryDataLen = 0;
			uint32 entryPreloadLen = 0;
			uint32 entryVersion = 0;
			uint64 entryModified = 0;

			// Load all the data used to fill in the entry info
			if (load( entryDataLen ) &&
				load( entryPreloadLen ) &&
				load( entryVersion) &&
				load( entryModified ) &&
				loadString( entryName_ ) &&
				(entryDataLen & ENTRY_DATA_MASK) >= entryPreloadLen)
			{
				// If the loading is successful, fill in the structure
				entryDataLen &= ENTRY_DATA_MASK;
				info_.streamSize_ = entryDataLen - entryPreloadLen;
				info_.preloadSize_ = entryPreloadLen;
				info_.fileOffset_ = fileDataOffset_;
				info_.modifiedTime_ = entryModified;
				info_.version_ = entryVersion;
				fileDataOffset_ += (entryDataLen + 3) & (~3L);
				valid_ = fileDataOffset_ <= directoryStart_;
			}
			else
			{
				valid_ = false;
			}

			return valid_;
		}

		/*
		 *	This template method loads a piece of data from the file
		 *	@param data return value for the data to load
		 *	@return true if the operation was a success
		 */
		template <typename KeyType>
		bool load( KeyType& data )
		{
			BW_GUARD;
			bool ret = false;

			// Check if the load goes beyond the end of the dirctory
			const uint32 KEY_SIZE = sizeof(KeyType);
			if (readOffset_ + KEY_SIZE <= directorySize_)
			{
				// Get the data
				data = *(KeyType*)(pData_ + readOffset_);
				readOffset_ += KEY_SIZE;
				ret = true;
			}
			else
			{
				ERROR_MSG( "EntryInfoLoader::load - Failed to read %d bytes\n",
					KEY_SIZE );
			}
			return ret;
		}

		/* This method loads a string from the file
		 *	@param data the return string
		 *	@return true if the operation was a success
		 */
		bool loadString( BW::string& data )
		{
			BW_GUARD;
			bool ret = false;
			// First get the length of the string
			uint32 len = 0;
			if (load(len))
			{
				// The data is multiple of 4 bytes
				uint32 dataLen = ((len + 3) & ~3L);
				// Check if the load goes beyond the end of the dirctory
				if (readOffset_ + dataLen <= directorySize_)
				{
					data.assign( pData_ + readOffset_, len );
					readOffset_ += dataLen;
					ret = true;
				}
			}

			if (!ret)
			{
				ERROR_MSG( "EntryInfoLoader::load - Failed to read string\n" );
			}
			return ret;
		}

		const StreamedDataCache::EntryInfo& info() const { return info_; }
		const BW::string& name() const { return entryName_; }
		bool valid() const { return valid_; }

		uint32 dataSize() const { return directoryStart_; }
	private:
		StreamedDataCache::EntryInfo info_;
		BW::string entryName_;
		bool valid_;
		uint32 fileSize_;
		uint32 directorySize_;
		uint32 directoryStart_;
		uint32 readOffset_;
		uint32 fileDataOffset_;
		char* pData_;
	};
}


/**
 *	Read in our directory
 */
void StreamedDataCache::loadSelf()
{
	BW_GUARD;
	// (note that this is in exactly the same format as a PrimitiveFile)
	long oldPos = ftell( file_ );
	MF_VERIFY( fseek( file_, 0, SEEK_END ) == 0 );
	long len = ftell( file_ );
	MF_VERIFY( fseek( file_, oldPos, SEEK_SET ) == 0 );

	if (len == 0)
	{
		// new empty file
		lastEntry_ = NULL;
		return;
	}

	// Create the entry info loader, this helps us load and validate
	// The entry info stored at the end of the anca file
	EntryInfoLoader loader(this, len);

	// Load up eacn entry in the anca file
	while (loader.nextEntry())
	{
		EntryInfos::iterator it = directory_.insert(
			std::make_pair( loader.name(), loader.info() ) ).first;
		lastEntry_ = &it->second;
	}

	// If the entry data is not valid, either the data 
	if 	(!loader.valid() || 
		!validateEntryData(loader.dataSize()))
	{
		directory_.clear();
		lastEntry_ = NULL;
	}
}

/**
 *	Write out our directory
 */
void StreamedDataCache::saveSelf()
{
	BW_GUARD;
	// figure out the end of our data
	uint32 dataEnd = (lastEntry_ != NULL) ? lastEntry_->fileOffset_ +
		((lastEntry_->preloadSize_+lastEntry_->streamSize_ + 3) & ~3L) : 0;

	// sort the directory by increasing order of offset
	BW::map< uint32, EntryInfos::iterator > revEntries;
	for (EntryInfos::iterator dirIt = directory_.begin();
		dirIt != directory_.end();
		dirIt++)
	{
		revEntries.insert( std::make_pair( dirIt->second.fileOffset_, dirIt ) );
	}

	// copy everything into a string
	BW::string	dirFile;
	BW::map< uint32, EntryInfos::iterator >::iterator it, nit;
	for (it = revEntries.begin();
		it != revEntries.end();
		it = nit)
	{
		nit = it;
		nit++;
		EntryInfos::const_iterator eit = it->second;
		const EntryInfo & ei = eit->second;
		uint32 entryDataLen = ei.streamSize_ + ei.preloadSize_;
		entryDataLen |= 1 << 31;
		dirFile.append( (char*)&entryDataLen, sizeof(entryDataLen) );
		dirFile.append( (char*)&ei.preloadSize_, sizeof(ei.preloadSize_) );
		dirFile.append( (char*)&ei.version_, sizeof(ei.version_) );
		dirFile.append( (char*)&ei.modifiedTime_, sizeof(ei.modifiedTime_) );
		uint32 entryNameLen = static_cast<uint32>(eit->first.length());
		dirFile.append( (char*)&entryNameLen, sizeof(entryNameLen) );
		dirFile.append( eit->first );
		uint32 zero = 0;	// VC7 dislikes unary minus on unsigned types
		dirFile.append( (char*)&zero, (0-entryNameLen) & (sizeof(zero)-1) );
	}

	uint32 dirLen = static_cast<uint32>(dirFile.length());
	dirFile.append( (char*)&dirLen, sizeof(dirLen) );

	// now write it out to the file
	this->immsave( dataEnd, static_cast<uint32>(dirFile.length()), dirFile.c_str() );
	fflush( file_ );

	// Make sure we are truncating the file to the size of the data + directory
	// Seek to the right position
	MF_VERIFY( fseek( file_, static_cast<long>(dataEnd + dirFile.length()), SEEK_SET ) == 0 );

	// Truncate the file, we use win32 functions as there is no posix equivalent
	HANDLE hFile = (HANDLE)_get_osfhandle( _fileno( file_ ) );
	MF_ASSERT( hFile != INVALID_HANDLE_VALUE );
	MF_VERIFY( SetEndOfFile( hFile ) != 0 );

}

bool StreamedDataCache::good()
{
	return !!file_;
}

namespace
{
	/*
	 *	This class helps with validating the actual data
	 *	owned by an animation in a .anca file
	 */
	class ValidateSingleEntryData
	{
	public:
		/*
		 *	Constructor
		 */
		ValidateSingleEntryData( StreamedDataCache* pCache, 
			const StreamedDataCache::EntryInfo& info, 
			uint32 headerStart ) :
		valid_(false),
		pData_(NULL),
		dataSize_(0),
		offset_(0)

		{
			BW_GUARD;
			// A big number that signals that there is something wrong in the animation
			const uint32 MAX_ANIM_CHANNELS = 10000;

			// Load up the preloadable data
			pData_ = new char[info.preloadSize_];
			if (pCache->preload( info.fileOffset_, info.preloadSize_, pData_ ))
			{
				dataSize_ = info.preloadSize_;

				// Check the first 4 entries in the data, the total time
				// the identifiers and the number of channel binders
				// If these seem to be valid, the animation should
				// be too
				float totalTime = 0.f;
				BW::string identifier;
				BW::string internalIdentifier;
				uint32 ncb = 0;
				if (read(totalTime) &&
					readString(identifier) &&
					readString(internalIdentifier) &&
					read(ncb))
				{
					if (ncb < MAX_ANIM_CHANNELS &&
						!_isnan(totalTime) &&
						totalTime > 0.f)
					{
						valid_ = true;
					}
				}
			}
		}

		/*
		 *	Destructor
		 */
		~ValidateSingleEntryData()
		{
			BW_GUARD;
			bw_safe_delete_array(pData_);
		}

		/*
		 *	This method helps by reading in a value from
		 *	the preloaded data, the index into the data
		 *	is advanced by the size of the size of the value
		 */
		template <typename KEY_TYPE>
		bool read(KEY_TYPE& value)
		{
			BW_GUARD;
			bool ret = false;
			const uint32 KEY_SIZE = sizeof(KEY_TYPE);
			if (offset_ + KEY_SIZE <= dataSize_)
			{
				value = *(KEY_TYPE*)(pData_ + offset_);
				offset_ += KEY_SIZE;
				ret = true;
			}
			return ret;
		}

		/*
		 *	This method helps by reading in a string from
		 *	the preloaded data, the index into the data
		 *	is advanced by the size of the size of the string
		 */
		bool readString(BW::string& value)
		{
			BW_GUARD;
			bool ret = false;

			// Get the string size
			uint32 stringSize = 0;
			if (read(stringSize))
			{
				// Read in the rest of the string if it does not
				// overshoot the data
				if (offset_ + stringSize <= dataSize_)
				{
					value.assign( pData_ + offset_, stringSize );
					offset_ += stringSize;
					ret = true;
				}
			}

			return ret;
		}
		bool valid() const { return valid_; }
	private:
		bool							valid_;
		char*							pData_;
		uint32							dataSize_;
		uint32							offset_;

	};

};

/**
 *	This method validates the entry data in the anca file by opening up each 
 *	individiual animation and checking if the header is ok.
 *	It also makes sure all the space is used up from the start of the file
 *	to the start of the header as this can be another sign of corruption.
 *	@param headerStart the location in the anca file for the header
 *	@return true if there was no error, false if an error occured
 */
bool StreamedDataCache::validateEntryData( uint32 headerStart )
{
	BW_GUARD;
	// maximum allowed name for the animation in the anca file
	const uint32 MAX_ANIM_NAME_LENGTH = 1024;

	// forward declaration of variable that tells us if the
	// whole file is filled
	bool hitEnd = false;

	// Iterate over the entries in the .anca file
	EntryInfos::const_iterator it = directory_.begin();
	while (it != directory_.end())
	{
		// Calculate the end of this entry's data
		const EntryInfo& ei = it->second;
		uint32 dataEnd = ((ei.fileOffset_ + ei.preloadSize_ + ei.streamSize_ + 3) & ~3L);

		// If the data goes past the header report an error
		if ( dataEnd > headerStart)
		{
			ERROR_MSG( "StreamedDataCache::validateEntryData: "
				"invalid entry %s, overshooting end of data in file %s by "
				"%d bytes\n", 
				it->first.c_str(), fileName_.c_str(), dataEnd - headerStart );
			return false;
		}
		// If the data goes up to the header we assume that we are utilising
		// all the data in the .anca file
		else if (headerStart == dataEnd)
		{
			hitEnd = true;
		}

		// Validate the start of the animation record in the .anca file
		ValidateSingleEntryData validationData( this, ei, headerStart );
		if (!validationData.valid())
		{
			ERROR_MSG( "StreamedDataCache::validateEntryData: "
				"invalid entry %s in file %s, corrupted header\n",
				it->first.c_str(), fileName_.c_str() );
			return false;
		}
		++it;
	}

	// If we didn't hit the header with all our data, report an error
	if (headerStart != 0 && !hitEnd)
	{
		ERROR_MSG( "StreamedDataCache::validateEntryData - data in %s "
			"not taking up all available space in the file, there is a gap "
			"between the last entry and the file index\n", fileName_.c_str() );
		return false;
	}

	return true;
}

}	// namespace Moo

BW_END_NAMESPACE

// streamed_data_cache.cpp
