#include "pch.hpp"

#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS

#include "cstdmf/watcher.hpp"
#include "cstdmf/watcher_path_request.hpp"
#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

/*
 * WATCHER PATH REQUEST BASE CLASS
 */
void WatcherPathRequest::notifyParent( int32 replies )
{
	if (pParent_ != NULL)
	{
		pParent_->notifyComplete( *this, replies );
	}
}



/*
 * WATCHER PATH REQUEST V2
 */


/**
 * Constructor
 */
WatcherPathRequestV2::WatcherPathRequestV2( const BW::string & path ) :
	WatcherPathRequest( path ),
	hasSeqNum_( false ),
	visitingDirectories_( false ),
	streamData_( NULL ),
	setStream_( NULL )
{ }


/**
 * Destructor
 */
WatcherPathRequestV2::~WatcherPathRequestV2()
{
	bw_safe_delete_array(streamData_);
	bw_safe_delete(setStream_);
}



// Overidden from WatcherPathRequest
void WatcherPathRequestV2::setResult( const BW::string & desc, 
		const Watcher::Mode & mode, const Watcher * watcher, const void *base )
{
	if (mode == Watcher::WT_DIRECTORY)
	{
		// Value should already been streamed on
		if (!visitingDirectories_)
		{
			visitingDirectories_ = true;
			originalRequestPath_ = requestPath_;
			((Watcher *)watcher)->visitChildren( base, NULL, *this );
			requestPath_ = originalRequestPath_;
			visitingDirectories_ = false;

			// Ready to fill in the packet now
			this->notifyParent();
		}
	}
	else if (mode != Watcher::WT_INVALID)
	{

		// TODO: this might be better performed with a bool dedicated to the job
		// TODO: how will this perform in async cases? might need a notification
		//       mechanism to allow async calls to notify failure... boo hoo

		// If we are a set request and have reached this point the operation
		// has succeeded and we have a full data stream, so now just push on
		// the status boolean at the end and then we are done.
		if (setStream_)
		{
			result_ << true;
		}

		// At this point we have type, mode, size, and data on the stream
		// (as that is filled before calling this function)

		// Ready to fill in the packet now
		// Tell our parent we have collected all our data
		if (!visitingDirectories_)
		{
			this->notifyParent();
		}
	}

	return;
}


// Overidden from WatcherPathRequest
bool WatcherPathRequestV2::setWatcherValue()
{
	if (!setStream_)
		return false;

	bool status = Watcher::rootWatcher().setFromStream( NULL,
				requestPath_.c_str(), *this );

	if (!status)
	{
		WARNING_MSG( "WatcherPathRequestV2::setWatcherValue: "
					"Failed to set '%s'\n",
				requestPath_.c_str() );

		if (setStream_)
		{
			setStream_->finish();
		}

		// NB: The sequence number should have already been pushed onto
		// the stream here, so we must check that we have only that
		// much data on the stream before attempting to fill it out with
		// a default result.
		if ((uint32)result_.size() <= sizeof(int32))
		{
			result_ << (uchar)WATCHER_TYPE_UNKNOWN;
			result_ << (uchar)Watcher::WT_READ_ONLY;
			result_.writeStringLength( 0 );
			result_ << status;
		}

		// We're pretty much done setting watcher value (failed)
		// Ready to fill in the packet now
		this->notifyParent();
	}

	return true;
}


// Overidden from WatcherPathRequest
void WatcherPathRequestV2::fetchWatcherValue()
{
	if (!Watcher::rootWatcher().getAsStream( NULL,
									requestPath_.c_str(), *this ))
	{
		WARNING_MSG( "WatcherPathRequestV2::fetchWatcherValue: "
				"Failed to get %s\n", requestPath_.c_str() );

		// If the get operation failed, we still need to notify the parent
		// so it can continue with its response.
		result_ << (uchar)WATCHER_TYPE_UNKNOWN;
		result_ << (uchar)Watcher::WT_READ_ONLY;
		result_.writeStringLength( 0 );

		// And notify the parent of our failure.
		this->notifyParent();
	}
}


// Overidden from WatcherPathRequest
bool WatcherPathRequestV2::addWatcherPath( const void *base, const char *path,
					 BW::string & label, Watcher & watcher )
{
	BW::string desc;

	if (originalRequestPath_.size())
		requestPath_ = originalRequestPath_ + "/" + label;
	else
		requestPath_ = label;

	// Push the directory entry onto the result stream
	// We are using a reference to the correct watcher now, so no need
	// to pass in the path to search for.
	bool status = watcher.getAsStream( base, NULL, *this );
	if (!status)
	{
		// If the get operation failed, we still need to notify the parent
		// so it can continue with its response.
		result_ << (uchar)WATCHER_TYPE_UNKNOWN;
		result_ << (uchar)Watcher::WT_READ_ONLY;
		result_.writeStringLength( 0 );
	}

	// Directory entries require an appended label
	result_ << label;

	// Always need to return true from the version 2 protocol
	// otherwise the stream won't be completed.
	return true;
}


/**
 * Process the raw stream data and extract any information we can use.
 *
 * This is used from the WatcherNub when passing a received packet into a
 * new PathRequest.
 *
 * @param size The size of the data to use.
 * @param data The array of data to place into the stream.
 *
 * @returns true on success, false on error.
 */
bool WatcherPathRequestV2::setPacketData( uint32 size, const char *data )
{
	if ((streamData_ != NULL) || (setStream_ != NULL))
		return false;

	streamData_ = new char[size];
	if (!streamData_)
		return false;

	memcpy( streamData_, data, size );

	setStream_ = new MemoryIStream( streamData_, (int32)size );
	if (!setStream_)
	{
		bw_safe_delete_array(streamData_);
		return false;
	}

	return true;
}


/**
 * Notify the Path Request of data to be used during a forwarding watcher
 * call. 
 *
 * This is used in a component when it receives the Mercury call to
 * callWatcher. It will construct the path request and place the data
 * sent via mercury into the path request which is then used to apply
 * to the watcher path on the current component.
 *
 * @param data The pre-streamed data to use when performing a watcher SET.
 *
 * @returns true on success, false on error.
 */
bool WatcherPathRequestV2::setPacketData( BinaryIStream & data )
{
	if ((streamData_ != NULL) || (setStream_ != NULL))
		return false;

	// This should only be used through a forwarding watcher interface.
	// As such we know that at this point we have a syncronous call stack
	// and we don't need to make a copy of the data in the BinaryIStream.

	int size = data.remainingLength();
	setStream_ = new MemoryIStream( data.retrieve( size ), size );
	if (!setStream_)
	{
		return false;
	}

	return true;
}


// Overidden from WatcherPathRequest
void WatcherPathRequestV2::addWatcherCount( int32 count )
{
	result_ << count;
}


// TODO: document me
void WatcherPathRequestV2::setSequenceNumber( uint32 seqNum )
{
	if (hasSeqNum_)
		return;

	hasSeqNum_ = true;
	result_ << seqNum;
}


// Overidden from WatcherPathRequest
const char *WatcherPathRequestV2::getData()
{
	return (const char *)result_.data();
}


// Overidden from WatcherPathRequest
int32 WatcherPathRequestV2::getDataSize()
{
	return result_.size();
}



/*
 * WATCHER PATH REQUEST V1
 */


/**
 * Constructor
 */
WatcherPathRequestV1::WatcherPathRequestV1( const BW::string & path ) :
	WatcherPathRequest( path ),
	containedReplies_( 1 ),
	useDescription_( false )
{ }


/**
 * Method for assigning a value to be used when setting a watcher path.
 */
void WatcherPathRequestV1::setValueData( const char *valueStr )
{
	setValue_ = valueStr;
}


// Overidden from WatcherPathRequest
bool WatcherPathRequestV1::setWatcherValue()
{
	if (!Watcher::rootWatcher().setFromString( NULL, requestPath_.c_str(),
			setValue_.c_str() ) )
	{
		WARNING_MSG( "WatcherPathRequestV1::setWatcherValue: "
							"Failed to set '%s'\n",
						requestPath_.c_str() );
		// error out + notify our parent we have nothing
		this->notifyParent( 0 );
		return false;
	}

	this->fetchWatcherValue();

	return true;
}


// Overidden from WatcherPathRequest
void WatcherPathRequestV1::fetchWatcherValue()
{
	BW::string desc;
	Watcher::Mode mode;
	BW::string result;

	if (!Watcher::rootWatcher().getAsString( NULL, requestPath_.c_str(), 
											desc, result, mode ))
	{
		this->notifyParent( 0 );
		return;
	}

	if (mode == Watcher::WT_DIRECTORY)
	{
		containedReplies_ = 0;
		WatcherVisitor *visitor = (WatcherVisitor *)this;

		// Handle a directory path request
		Watcher::rootWatcher().visitChildren( NULL, requestPath_.c_str(), 
				*visitor );

		// At this point we've collected results from directory's children 
		// watchers. containedReplies_ also contains the amount of children 
		// under this directory.
		this->notifyParent( containedReplies_ );
	}
	else if (mode != Watcher::WT_INVALID)
	{
		// Add the result onto the final result stream
		resultStream_.addBlob( requestPath_.c_str(), 
									static_cast<int>(requestPath_.size() + 1) );
		resultStream_.addBlob( result.c_str(),
			static_cast<int>(result.size() + 1) );
		if (useDescription_)
			resultStream_.addBlob( desc.c_str(), 
								static_cast<int>(desc.size() + 1) );

		// Tell our parent we have collected all our data
		this->notifyParent();
	}

	return;
}


// Overidden from WatcherPathRequest
const char *WatcherPathRequestV1::getData()
{
	return (const char *)resultStream_.data();
}


// Overidden from WatcherPathRequest
int32 WatcherPathRequestV1::getDataSize()
{
	return resultStream_.size();
}


// Overidden from WatcherVisitor
bool WatcherPathRequestV1::visit( Watcher::Mode /*mode*/,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & valueStr )
{
	BW::string path;

	if (requestPath_.size())
	{
		path = requestPath_ + "/" + label;
	}
	else
	{
		path = label;
	}
	resultStream_.addBlob( path.c_str(), static_cast<int>(path.size() + 1) );
	resultStream_.addBlob( valueStr.c_str(), 
									static_cast<int>(valueStr.size() + 1) );
	if (useDescription_)
		resultStream_.addBlob( desc.c_str(), 
									static_cast<int>(desc.size() + 1 ) );
	containedReplies_++;
	return true;
}

BW_END_NAMESPACE

#endif
