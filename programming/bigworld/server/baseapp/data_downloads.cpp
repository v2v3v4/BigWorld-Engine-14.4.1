#include "script/first_include.hpp"

#include "data_downloads.hpp"

#include "loading_thread.hpp"

#include "connection/client_interface.hpp"

#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE


// Helper classes used by streamStringToClient() and streamFileToClient()

namespace
{
const unsigned int MAX_CONCURRENT_DOWNLOADS = 0xfffe;
}

// -----------------------------------------------------------------------------
// Section: FailedDataDownload
// -----------------------------------------------------------------------------

/**
 *	A DataDownload that was onloaded from another BaseApp. We just report
 *	failure the first time we try to send.
 */
class FailedDataDownload : public DataDownload
{
public:
	FailedDataDownload( uint16 id, DataDownloads &dls );

	~FailedDataDownload()	{}

private:
	// These should never be called
	void read( BinaryOStream &os, int nBytes );
	int available() const { return 0; }
	bool done() const { return false; }
};


/**
 *	Constructor. Immediately marks ourself failed, assuming we
 *	succeeded in being created at all.
 */
FailedDataDownload::FailedDataDownload( uint16 id, DataDownloads &dls ) :
	DataDownload( NULL, id, dls )
{
	MF_ASSERT( good_ );
	good_ = false;
}


/**
 *	Streams some data out of this buffer into the provided output stream.
 */
void FailedDataDownload::read( BinaryOStream &os, int nBytes )
{
	MF_ASSERT( !"FailedDataDownload tried to stream" );
}





// -----------------------------------------------------------------------------
// Section: DataDownloads
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
DataDownloads::DataDownloads() :
	freeID_( 0 )
{
}


/**
 *	Destructor.
 */
DataDownloads::~DataDownloads()
{
	for (Container::iterator it = dls_.begin(); it != dls_.end(); ++it)
	{
		delete *it;
	}
}


/**
 *	This method attempts to add some of the downloads to a bundle.
 *
 *	@param bundle	The bundle to stream to.
 *	@param remaining	The maximum amount to try to add. This value is reduced
 *				by the amount that is actually sent.
 *	@param finishedDownloads This is told of any downloads that finish during
 *				this call.
 *
 *	@return The number of bytes downloaded.
 */
int DataDownloads::addToBundle( Mercury::Bundle & bundle, int & remaining,
	IFinishedDownloads & finishedDownloads )
{
	int totalBytesDownloaded = 0;

	Container::iterator it = dls_.begin();

	// Iterate over pending downloads and stream em on until we fill the packet
	while ((it != dls_.end()) && (remaining > 0))
	{
		DataDownload &dl = **it;

		// If the download is in a bad state, drop it. No need for an error
		// message as this would have been output from the loading thread.
		if (!dl.good())
		{
			finishedDownloads.onFinished( dl.id(), false );
			it = this->erase( it );
			continue;
		}

		totalBytesDownloaded += dl.addToBundle( bundle, remaining );

		// If this download is done now, remove it from the queue
		if (dl.done())
		{
			dl.onDone();

			finishedDownloads.onFinished( dl.id(), true );
			it = this->erase( it );
			continue;
		}

		++it;
	}

	return totalBytesDownloaded;
}


/**
 *	Queue a download into this collection. Auto-assign a new ID to the download
 *	if it is passed with an ID of -1.
 */
bool DataDownloads::push_back( DataDownload *pDL )
{
	if (dls_.size() == MAX_CONCURRENT_DOWNLOADS)
	{
		ERROR_MSG( "DataDownloads::push_back: "
			"Limit of concurrent downloads reached (%d)\n",
			MAX_CONCURRENT_DOWNLOADS );

		return false;
	}

	// Use the freeID if necessary
	if (pDL->id() == uint16( -1 ))
	{
		pDL->id( freeID_ );
	}

	dls_.push_back( pDL );
	usedIDs_.insert( pDL->id() );

	// If we've just used the freeID_ we need to pick another one
	if (pDL->id() == freeID_)
	{
		for (uint i=1; i < MAX_CONCURRENT_DOWNLOADS; i++)
		{
			freeID_ = uint16( int( pDL->id() ) + i );
			if (usedIDs_.count( freeID_ ) == 0)
				break;

			MF_ASSERT( i != MAX_CONCURRENT_DOWNLOADS - 1 );
		}
	}

	MF_ASSERT( dls_.size() == usedIDs_.size() );

	return true;
}


/**
 *	Remove a download from this collection.
 */
DataDownloads::Container::iterator
	DataDownloads::erase( Container::iterator it )
{
	// Reclaim the ID
	usedIDs_.erase( (*it)->id() );
	if (freeID_ > (*it)->id())
	{
		freeID_ = (*it)->id();
	}

	// Clean up the download itself
	delete *it;

	// Drop it from the list
	it = dls_.erase( it );

	MF_ASSERT( dls_.size() == usedIDs_.size() );

	return it;
}


/**
 *	Returns true if the given id is already present in the list.
 */
bool DataDownloads::contains( uint16 id ) const
{
	if (id == uint16( -1 ))
		return false;

	return usedIDs_.count( id ) == 1;
}


/**
 *	Returns true if there are no active downloads
 */
bool DataDownloads::empty() const
{
	MF_ASSERT( dls_.empty() == usedIDs_.empty() );
	return usedIDs_.empty();
}


/**
 *	Aborts all current downloads and reverts us to a pristine state
 */
void DataDownloads::abortDownloads( IFinishedDownloads & finishedDownloads )
{
	if (dls_.empty())
	{
		MF_ASSERT( usedIDs_.empty() );
		MF_ASSERT( freeID_ == 0 );
		return;
	}

	// Clear out our downloads, and reset us back to freshly-instantiated
	for (Container::iterator it = dls_.begin(); it != dls_.end(); ++it)
	{
		finishedDownloads.onFinished( (*it)->id(), false );
		bw_safe_delete( *it );
	}

	dls_.clear();
	usedIDs_.clear();
	freeID_ = 0;
}


/**
 *	Add a download from the supplied factory to our list of active
 *	downloads, and start streaming at the next opportunity.
 */
PyObject * DataDownloads::streamToClient( DataDownloadFactory & factory,
	PyObjectPtr pDesc, int id )
{
	DataDownload * pDL = factory.create( pDesc, id, *this );

	if (!pDL)
	{
		return NULL;
	}

	if (!pDL->good())
	{
		PyErr_Format( PyExc_ValueError,
			"Supplied ID (%d) already in use",
			id );

		delete pDL;
		return NULL;
	}

	return Script::getData( pDL->id() );
}


// -----------------------------------------------------------------------------
// Section: Streaming methods for DataDownloads
// -----------------------------------------------------------------------------

/**
 *	Implement output streaming for DataDownloads instances.
 */
BinaryOStream & operator<<( BinaryOStream & b,
	const DataDownloads & rDownloads )
{
	b << rDownloads.usedIDs_.size();

	for (DataDownloads::IDSet::const_iterator iIDs =
			rDownloads.usedIDs_.begin();
		iIDs != rDownloads.usedIDs_.end();
		++iIDs)
	{
		b << *iIDs;
	}

	return b;
}


/**
 *	Implement input streaming for DataDownloads instances.
 *	All streamed-in downloads are considered to have failed.
 */
BinaryIStream & operator>>( BinaryIStream & b, DataDownloads & rDownloads )
{
	MF_ASSERT( rDownloads.usedIDs_.empty() );
	MF_ASSERT( rDownloads.dls_.empty() );
	MF_ASSERT( rDownloads.freeID_ == 0 );

	DataDownloads::IDSet::size_type count;

	b >> count;

	for (unsigned int i = 0; i < count; ++i)
	{
		uint16 usedID;
		b >> usedID;
		// DataDownload self-registers (and is owned by)
		// the supplied DataDownloads reference
		new FailedDataDownload( usedID, rDownloads );
	}

	return b;
}


// -----------------------------------------------------------------------------
// Section: DataDownloadFactory
// -----------------------------------------------------------------------------

/**
 *	Create a DataDownload instance.
 */
DataDownload * DataDownloadFactory::create( PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls )
{
	if (!this->checkDescription( pDesc ))
	{
		return NULL;
	}

	return this->onCreate( pDesc, id, dls );
}


/**
 *	Helper function for stream*ToClient() methods below, mostly common arg
 *	checking stuff.
 */
bool DataDownloadFactory::checkDescription( PyObjectPtr &rpDesc )
{
	if (rpDesc == NULL)
	{
		rpDesc = PyObjectPtr( PyString_FromString( "" ),
			PyObjectPtr::STEAL_REFERENCE );
	}

	if (!rpDesc || !PyString_Check( rpDesc.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Description must be a string" );
		return false;
	}

	// We don't support descriptions longer than 254 bytes
	if (PyString_Size( rpDesc.get() ) >= 255)
	{
		PyErr_SetString( PyExc_ValueError,
			"Description must be 254 chars or less" );
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: DataDownload
// -----------------------------------------------------------------------------

/* static */ int DataDownload::packetOverhead()
{
	return ClientInterface::resourceFragment.headerSize() +
		sizeof( ClientInterface::ResourceFragmentArgs );
}

/**
 *	Construct a new DataDownload, and insert it into the downloads list.
 */
DataDownload::DataDownload( PyObjectPtr pDesc, uint16 id,
	DataDownloads &dls ) :
	id_( id ),
	seq_( 0 ),
	pDesc_( pDesc ),
	bytesSent_( 0 ),
	packetsSent_( 0 )
{
	good_ = !dls.contains( id );

	if (good_)
	{
		dls.push_back( this );
	}
}


/**
 *	Add as much data as we can from this download to the given bundle.
 */
int DataDownload::addToBundle( Mercury::Bundle & bundle, int & remaining )
{
	// TODO: Passing remaining as a reference is inelegant.

	// Try to send the ResourceHeader message if it hasn't been sent yet
	if (pDesc_ != NULL)
	{
		// Make sure there's enough room. Magic +1 is for string length
		// prefix on the stream
		const uint32 size = ClientInterface::resourceHeader.headerSize() +
			sizeof( uint16 ) + 1 + PyString_Size( pDesc_.get() );

		bundle.startMessage( ClientInterface::resourceHeader );
		bundle << id_;
		bundle.appendString( PyString_AsString( pDesc_.get() ),
			PyString_Size( pDesc_.get() ) );

		// Throw away the description to avoid sending this header again
		pDesc_ = NULL;

		start_ = timestamp();

		remaining -= size;
		if (remaining <= 0)
		{
			return 0;
		}
	}

	// Skip if there's nothing to send at the moment
	if (this->available() == 0)
	{
		return 0;
	}

	// Header size for each resourceFragment message not including payload
	const uint32 headerSize = DataDownload::packetOverhead();

	// If there's no more room on this packet, bail
	remaining -= headerSize;
	if (remaining <= 0)
	{
		return 0;
	}

	// OK we're sending some data
	int payloadSize = std::min( remaining, this->available() );
	remaining -= payloadSize;

	bundle.startMessage( ClientInterface::resourceFragment );
	bundle << BW_HTONS( id_ ) << seq_++;

	// Stream on the payload onto a MemoryOStream as the
	// done flag needs to be added to the header / args.
	// NOTE: This can not be changed to reference the previous
	// pointer due to inconsistencies with UDP and TCP bundles/packets.
	MemoryOStream os;
	this->read( os, payloadSize );

	bundle << uint8(this->done());
	bundle.transfer( os, os.remainingLength() );

	bytesSent_ += payloadSize;
	packetsSent_ += bundle.numDataUnits();

	return payloadSize;
}


/**
 *	Callback when we have completed sending our data.
 */
void DataDownload::onDone()
{
	float secs = float( timestamp() - start_ ) / stampsPerSecond();

	DEBUG_MSG( "DataDownload::onDone(): Download %u complete, %d bytes, "
			"%d packets, %.1f seconds, %.1f kB/s\n",
		id_, bytesSent_, packetsSent_, secs,
		bytesSent_ / 1000.0 / secs );
}


// -----------------------------------------------------------------------------
// Section: StringDataDownload
// -----------------------------------------------------------------------------

/**
 *	A PyString being sent with streamStringToClient().
 */
class StringDataDownload : public DataDownload
{
public:
	StringDataDownload( PyObjectPtr pData, PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls );

	virtual ~StringDataDownload(){}

protected:
	virtual void read( BinaryOStream &os, int nBytes );
	virtual int available() const { return stream_.remainingLength(); }
	virtual bool done() const { return this->available() == 0; }

	PyObjectPtr pData_;
	MemoryIStream stream_;
};


/**
 *	Constructor.
 */
StringDataDownload::StringDataDownload(
	PyObjectPtr pData, PyObjectPtr pDesc, uint16 id, DataDownloads &dls ) :
	DataDownload( pDesc, id, dls ),
	pData_( pData ),
	stream_( PyString_AsString( pData.get() ), PyString_Size( pData.get() ) )
{}


/**
 *	Streams some data out of this buffer into the provided output stream.
 */
void StringDataDownload::read( BinaryOStream &os, int nBytes )
{
	int n = std::min( nBytes, stream_.remainingLength() );
	os.addBlob( stream_.retrieve( n ), n );
}


/**
 *	This factory method is used to create StringDataDownload instances.
 */
DataDownload * StringDataDownloadFactory::onCreate( PyObjectPtr pDesc,
		uint16 id, DataDownloads &dls )
{
	if (!PyString_Check( pData_.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Data argument must be a string." );
		return NULL;
	}

	return new StringDataDownload( pData_, pDesc, id, dls );
}


// -----------------------------------------------------------------------------
// Section: FileDataDownload
// -----------------------------------------------------------------------------

/**
 *	A file being sent with streamFileToClient().
 */
class FileDataDownload : public DataDownload
{
public:
	FileDataDownload( const BW::string &path, PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls );

	virtual ~FileDataDownload();

protected:
	virtual void read( BinaryOStream &os, int nBytes );

	virtual int available() const;
	virtual bool done() const;
	virtual bool good() const;

	FileStreamingJob * pJob_;
};


/**
 *	Constructor.
 */
FileDataDownload::FileDataDownload( const BW::string &path,
	PyObjectPtr pDesc, uint16 id, DataDownloads &dls ) :
	DataDownload( pDesc, id, dls ),
	pJob_( NULL )
{
	// Don't start up the streaming job if there was an error in DataDownload()
	if (good_)
	{
		pJob_ = new FileStreamingJob( path );
	}
}


/**
 *	Destructor.
 */
FileDataDownload::~FileDataDownload()
{
	if (pJob_ != NULL)
	{
		pJob_->deleteSelf();
	}
}


/*
 *	Override from DataDownload.
 */
void FileDataDownload::read( BinaryOStream &os, int nBytes )
{
	pJob_->read( os, nBytes );
}


/*
 *	Override from DataDownload.
 */
int FileDataDownload::available() const
{
	return pJob_->size();
}


/*
 *	Override from DataDownload.
 */
bool FileDataDownload::done() const
{
	return pJob_->done();
}


/*
 *	Override from DataDownload.
 */
bool FileDataDownload::good() const
{
	return good_ && pJob_ && pJob_->good();
}


/**
 *	This factory method is used to create FileDataDownload instances.
 */
DataDownload * FileDataDownloadFactory::onCreate( PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls )
{
	return new FileDataDownload( path_, pDesc, id, dls );
}

BW_END_NAMESPACE

// data_downloads.cpp
