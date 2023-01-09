#ifndef WATCHER_PATH_REQUEST_HPP
#define WATCHER_PATH_REQUEST_HPP

#include "cstdmf/config.hpp"
#if ENABLE_WATCHERS

#include "binary_stream.hpp"
#include "memory_stream.hpp"

BW_BEGIN_NAMESPACE

class WatcherPathRequestNotification;

class Watcher;


/**
 * WatcherPathRequest is a handler class to allow an asyncronous request
 * of a single watcher path.
 */
class WatcherPathRequest
{
public:
	WatcherPathRequest( const BW::string & path ) :
		pParent_( NULL ),
		requestPath_( path )
	{ }

	WatcherPathRequest(  ) :
		pParent_( NULL )
	{ }

	virtual ~WatcherPathRequest() {}

	/**
	 * Initiate watcher value retreival for the current path request.
	 */
	virtual void fetchWatcherValue() { }

	/**
	 * Initiate a watcher path set operation after data to use
	 * has been placed into the WatcherPathRequest via @see setResult().
	 *
	 * @returns true on success, false on failure.
	 */
	virtual bool setWatcherValue() { return false; }


	/**
	 * Accessor method for the watcher path associated with the
	 * current instance.
	 */
	const BW::string & getPath() const
	{
		return requestPath_;
	}


	/**
	 * Setup the pointer back to the creator of this class to notify
	 * on completion.
	 */
	void setParent( WatcherPathRequestNotification *parent )
	{
		pParent_ = parent;
	}

	/**
	 * Check if we have been told about a creator, and notify them of our
	 * completion.
	 */
	virtual void notifyParent( int32 replies=1 );

	/**
	 * Notification method called by visitChildren with the number of children
	 * that we will be told about.
	 */
	virtual void addWatcherCount( int32 count ) {}

	/**
	 * Notification of a child watcher entry as called from visitChildren.
	 */
	virtual bool addWatcherPath( const void *base, const char *path,
								 BW::string & label, Watcher &watcher )
	{
		return false;
	}


	/**
	 * Return a pointer to the complete data set that should be used
	 * to create the watcher packet response.
	 */
	virtual const char *getData() { return NULL; }

	/**
	 * Return the size of the data that is returned by getData().
	 */
	virtual int32 getDataSize() { return 0; }


protected:
	WatcherPathRequestNotification *pParent_; //!< The creator of the path request.

	BW::string requestPath_; //!< The path of the requested watcher
};



/**
 * WatcherPathRequest for watcher protocol version 2.
 */
class WatcherPathRequestV2 : public WatcherPathRequest
{
public:

	CSTDMF_DLL WatcherPathRequestV2( const BW::string & path );
	~WatcherPathRequestV2();

	bool setPacketData( uint32 size, const char *data );
	bool setPacketData( BinaryIStream & data );

	void setSequenceNumber( uint32 seqNum );

	/*
	 * Overriden from WatcherPathRequest
	 */
	virtual bool setWatcherValue();
	virtual void fetchWatcherValue();

	const char *getData();
	int32 getDataSize();

	virtual void setResult( const BW::string & desc,
							const Watcher::Mode & mode,
							const Watcher * watcher, const void *base );

	/*
	 * Methods used by visitChildren
	 */
	void addWatcherCount( int32 count );
	bool addWatcherPath( const void *base, const char *path,
						 BW::string & label, Watcher & watcher );

	/*
	 * Accessors
	 */
	MemoryOStream & getResultStream() { return result_; }
	BinaryIStream * getValueStream() { return setStream_; }

private:
	MemoryOStream result_; //!< Output stream for the watcher path requests data.
	BW::string originalRequestPath_; //!< Original path requested via watcher nub, used when visiting directories and requestPath_ may be altered.

	bool hasSeqNum_; //!< True when the sequence number for the path request has already been added to the result stream.

	bool visitingDirectories_; //!< True when the path request is visiting children and shouldn't notify parent of completion.

	char *streamData_; //!< Data obtained from the watcher nub to use in a set operation.
	MemoryIStream *setStream_; //!< Stream representation of the @see streamData.

	WatcherPathRequestNotification *parent; //!< Our creator to notify on completion.
};




/**
 * WatcherPathRequest for watcher protocol version 1.
 */
class WatcherPathRequestV1 : public WatcherPathRequest, public WatcherVisitor
{
public:

	CSTDMF_DLL WatcherPathRequestV1( const BW::string & path );

	void setValueData( const char *valueStr );
	void useDescription( bool shouldUse ) { useDescription_ = shouldUse; }

	/*
	 * Overidden from WatcherPathRequest
	 */
	virtual bool setWatcherValue();
	virtual void fetchWatcherValue();

	const char *getData();
	int32 getDataSize();


	/*
	 * Overriden from WatcherVisitor
	 */
	bool visit(  Watcher::Mode /*mode*/,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & valueStr );

private:
	MemoryOStream resultStream_; //!< Output stream for the watcher path requests data.
	BW::string setValue_; //!< String used in set operations for this path request
	int32 containedReplies_; //!< Number of watcher path replies contained in this path request.
	bool useDescription_; //!< True if the watcher request reply should contain a description.
};


/**
 * Wrapper around WatcherVisitor so that visitChildren
 * still works for the client
 */
class WatcherPathRequestWatcherVisitor : public WatcherPathRequest
{
public:

	WatcherPathRequestWatcherVisitor( WatcherVisitor & visitor ) :
		pVisitor_( &visitor )
	{
	}


	bool addWatcherPath( const void *base, const char *path, BW::string & label,
			Watcher &watcher )
	{
		BW::string valueStr;
		BW::string desc;
		Watcher::Mode resMode;

		watcher.getAsString( base, path, valueStr, desc, resMode );

		return (pVisitor_->visit( resMode, label, desc, valueStr ));
	}

private:
	WatcherVisitor *pVisitor_;

	BW::string result;
	BW::string desc;
	Watcher::Mode mode;

};



/**
 * Interface to allow a WatcherPathRequest to notify a calling context
 * of successfull completion.
 */
class WatcherPathRequestNotification : public ReferenceCount
{
public:

	/**
	 * Notify the implementing class of a path request completion.
	 *
	 * @param pathRequest The WatcherPathRequest that has been completed.
	 * @param count		  The number of replies contained in the path request.
	 */
	virtual void notifyComplete( WatcherPathRequest & pathRequest, int32 count ) = 0;

	/**
	 * Create a new WatcherPathRequest associated with the current packet.
	 *
	 * @param path The watcher path the request will query.
	 *
	 * @returns A pointer to a WatcherPathRequest on success, NULL on error.
	 */
	virtual WatcherPathRequest * newRequest( BW::string & path ) = 0;
};

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

#endif

