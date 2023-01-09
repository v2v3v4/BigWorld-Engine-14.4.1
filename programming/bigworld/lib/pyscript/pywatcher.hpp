#ifndef PYWATCHER_HPP
#define PYWATCHER_HPP 
#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS

#include "Python.h"

#include <iostream>

#include "network/network_interface.hpp"
#include "network/channel.hpp"
#include "network/bundle.hpp"
#include "cstdmf/watcher.hpp"
#include "py_to_stl.hpp"


BW_BEGIN_NAMESPACE

// Get a watcher for a PySequence (using PySequenceSTL)
Watcher & PySequence_Watcher();

// Get a watcher for a PyMapping (using PyMappingSTL)
Watcher & PyMapping_Watcher();



/**
 *	This class is used to watch a PyObjectPtrRef
 *
 *	It uses a registry to more specialised watchers to dispatch
 *	the handling to the appropriate watcher.
 *
 *	Note that these watchers all work with references of pointers,
 *	not references of PyObjects themselves. This is so that a
 *	'set' operation could conceivably work ... as many PyObjects
 *	are not mutable. e.g. changing the '1' object to actually be
 *	'2' would have dire consequences for the Python system - every
 *	other '1' would also change to a '2'! In 'set' operations,
 *	DECREF is used on the old object pointer, and a a new one
 *	(always of the same type) is put in its place.
 */
class PyObjectWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	PyObjectWatcher( PyObjectPtrRef & popr );
	//@}

	/// @name Overrides from Watcher
	//@{
	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode )
		const;

	virtual bool setFromString( void * base, const char * path,
		const char * valueStr );

	virtual bool getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest )
		const;

	virtual bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest );

	virtual bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest );

	virtual bool addChild( const char * path, WatcherPtr pChild,
		void * withBase = NULL );
	//@}

private:
	PyObjectPtrRef & popr_;

	static SmartPointer<DataWatcher<PyObjectPtrRef> >	fallback_;

	static Watcher * getSpecialWatcher( PyObject * pObject );

	static const char * tail( const char * path )
	{
		return DirectoryWatcher::tail( path );
	}
};


/**
 *	This class interprets the base as a python object and displays it.
 *
 */

class SimplePythonWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 *
	 */
	SimplePythonWatcher()
	{ }

	virtual bool getAsString( const void * base, const char * path,
							  BW::string & result, BW::string & desc, 
							  Watcher::Mode & mode ) const;
	virtual bool setFromString( void * base, const char * path,
								const char * valueStr );
	virtual bool getAsStream( const void * base, const char * path,
							  WatcherPathRequestV2 & pathRequest ) const;
	virtual bool setFromStream( void * base, const char * path,
								WatcherPathRequestV2 & pathRequest );
	virtual bool visitChildren( const void * base, const char * path,
								WatcherPathRequest & pathRequest );

private:
	static PyObject * pythonChildBase( PyObject * pPyObject, 
									   const char * path );
};


/**
 * This class handles asynchronous watcher requests.
 */
class DeferrableWatcherPathRequest : public WatcherPathRequestNotification
{
public:

	DeferrableWatcherPathRequest( const BW::string & path,
			Mercury::NetworkInterface & networkInterface,
			const Mercury::Address & srcAddr, const Mercury::ReplyID replyID ):
		interface_( networkInterface ),
		srcAddr_( srcAddr ),
		replyID_( replyID ),
		pPathRequest_( new WatcherPathRequestV2( path ) )
	{
	}

	virtual ~DeferrableWatcherPathRequest() {}

	virtual void notifyComplete( WatcherPathRequest & pathRequest, int32 count )
	{
		MF_ASSERT( &pathRequest == pPathRequest_ );

		Mercury::Channel & channel = interface_.findOrCreateChannel( srcAddr_ );

		Mercury::Bundle & bundle = channel.bundle();
		bundle.startReply( replyID_ );

		MemoryOStream & resultStream = pPathRequest_->getResultStream();
		if (resultStream.size())
		{
			bundle.addBlob( resultStream.data(), resultStream.size() );
		}

		bw_safe_delete( pPathRequest_ );
		delete this;
	}


	// Not implemented, return the request given to our constructor
	virtual WatcherPathRequest * newRequest( BW::string & path )
	{
		return pPathRequest_;
	}


	void setPacketData( BinaryIStream & data )
	{
		pPathRequest_->setPacketData( data );
	}


	void setWatcherValue()
	{
		pPathRequest_->setParent( this );

		if (!pPathRequest_->setWatcherValue())
		{
			this->notifyComplete( *pPathRequest_, 1 /*replies*/ );
		}
	}

private:

	Mercury::NetworkInterface & interface_;
	const Mercury::Address srcAddr_;
	const Mercury::ReplyID replyID_;

	WatcherPathRequestV2 * pPathRequest_;
};

BW_END_NAMESPACE

#endif // ENABLE_WATCHERS

#endif // PYWATCHER_HPP
