#include "script/first_include.hpp"

#include "client_entity_mailbox.hpp"
#include "remote_client_method.hpp"

#include "proxy.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "entitydef/method_description.hpp"
#include "entitydef/script_data_source.hpp"

#include "server/event_history_stats.hpp"

#include <memory>

BW_BEGIN_NAMESPACE


namespace // (anonymous)
{

/**
 *	This method implements the tp_repr and tp_str functions in the Python type
 *	object for RemoteClientMethod.
 */
PyObject * pyReprRemoteClientMethod( PyObject * self )
{
	return reinterpret_cast< RemoteClientMethod * >( self )->pyRepr();
}

} // end namespace (anonymous)

PY_TYPEOBJECT_SPECIALISE_REPR_AND_STR( RemoteClientMethod, 
	&pyReprRemoteClientMethod, &pyReprRemoteClientMethod )

PY_TYPEOBJECT_WITH_CALL( RemoteClientMethod )

PY_BEGIN_METHODS( RemoteClientMethod )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( RemoteClientMethod )
	PY_ATTRIBUTE( entityID )
	PY_ATTRIBUTE( methodName )
	PY_ATTRIBUTE( isRecording )
	PY_ATTRIBUTE( isRecordingOnly )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 *
 *	@param pMailBox 				The mailbox to the destination entity on
 *									the client to call method calls on.
 *	@param entityTypeName 			The name of the entity type of the
 *									destination entity.
 *	@param pMethodDescription 		The method to call on the destination
 *									entity.
 *	@param cellChannel 				The entity's cell channel.
 *	@param recordingOption			The recording option, see RecordingOption
 *									enum.
 *	@param pType 					The type object to use to initialise the
 *									Python object.
 */
RemoteClientMethod::RemoteClientMethod( PyEntityMailBox * pMailBox,
			const BW::string & entityTypeName,
			const MethodDescription * pMethodDescription,
			Mercury::Channel & cellChannel,
			RecordingOption recordingOption,
				/* = RECORDING_OPTION_METHOD_DEFAULT */
			PyTypeObject * pType /* = RemoteClientMethod::s_type_ */ ) :
		PyObjectPlus( pType ),
		pMailBox_( pMailBox ),
		entityTypeName_( entityTypeName ),
		pMethodDescription_( pMethodDescription ),
		pRecordingEntityChannel_( &cellChannel ),
		isRecordingOnly_( recordingOption == RECORDING_OPTION_RECORD_ONLY )
{
	if (!pMethodDescription->shouldRecord( recordingOption ))
	{
		pRecordingEntityChannel_ = NULL;
	}
}


/**
 *	This method is called when a script wants to call this method
 *	on a remote script handler (entity/base).
 */
PyObject * RemoteClientMethod::pyCall( PyObject * args )
{
	ScriptObject pArgs( args, ScriptObject::FROM_BORROWED_REFERENCE );
	if (!pMethodDescription_->areValidArgs( true, pArgs, true ))
	{
		return NULL;
	}

	BinaryOStream * pMailBoxStream = NULL;
	int sizeBefore = 0;

	if (!isRecordingOnly_)
	{
		pMailBoxStream = pMailBox_->getStream( *pMethodDescription_ );

		if (pMailBoxStream == NULL)
		{
			WARNING_MSG( "RemoteClientMethod::pyCall: %s(%d).%s: "
					"Could not get stream to call (no attached client?)\n",
				entityTypeName_.c_str(), 
				pMailBox_->id(),
				pMethodDescription_->name().c_str() );

			// TODO: Should consider throwing an exception here to inform
			// script that the method didn't get called.
			Py_RETURN_NONE;
		}

		sizeBefore = pMailBoxStream->size();
	}
	else // isRecordingOnly_
	{
		MF_ASSERT( pRecordingEntityChannel_ );
	}

	// Check if it's still alive
	if (pRecordingEntityChannel_ && !pRecordingEntityChannel_->isConnected())
	{
		ERROR_MSG( "RemoteClientMethod::pyCall: %s(%d).%s"
				"No cell entity channel to send method recording data\n",
			entityTypeName_.c_str(),
			pMailBox_->id(),
			pMethodDescription_->name().c_str() );

		pRecordingEntityChannel_ = NULL;
		PyErr_SetString( PyExc_ValueError, "No cell to record method" );
		return NULL;
	}

	ScriptDataSource source( pArgs );

	if (pRecordingEntityChannel_)
	{
		std::auto_ptr< MemoryOStream > pRecordingStream( new MemoryOStream() );
		if (!pMethodDescription_->addToClientStream( source, *pRecordingStream,
			pMailBox_->id() ))
		{
			return NULL;
		}

		Mercury::Bundle & bundle = pRecordingEntityChannel_->bundle();
		bundle.startMessage( CellAppInterface::recordClientMethod );

		bundle << pMailBox_->id() << pMethodDescription_->internalIndex();

		bundle.transfer( *pRecordingStream, 
			pRecordingStream->remainingLength() );

		if (!isRecordingOnly_)
		{
			pRecordingStream->rewind();
			pMailBoxStream->transfer( *pRecordingStream, 
				pRecordingStream->remainingLength() );
		}
	}
	else
	{
		bool result = pMethodDescription_->addToClientStream( source,
			*pMailBoxStream, pMailBox_->id() );

		if (!result)
		{
			return NULL;
		}
	}

	if (!pRecordingEntityChannel_ || !isRecordingOnly_)
	{
		MF_ASSERT( pMailBoxStream );

		const int bytesAdded = pMailBoxStream->size() - sizeBefore;

		int streamSize = pMethodDescription_->streamSize( true );

		// This doesn't always hold true, due to Bundle's implementation
		// of BinaryOStream::size()
		//MF_ASSERT_DEV( (streamSize < 0) || (streamSize == bytesAdded) );

		pMethodDescription_->stats().countSentToOwnClient( bytesAdded );

		EventHistoryStats & stats = Proxy::privateClientStats();

		if (stats.isEnabled())
		{
			ClientEntityMailBox& clientMB =
					static_cast< ClientEntityMailBox& >( *pMailBox_ );
			stats.trackEvent( clientMB.getEntityDescription().name(),
					pMethodDescription_->name(), bytesAdded, streamSize );
		}

		pMailBox_->sendStream();
	}

	Py_RETURN_NONE;
}


/**
 *	This method implements the Python __str__() and __repr__() operators.
 */
PyObject * RemoteClientMethod::pyRepr() const
{
	return PyString_FromFormat( "Remote method: %s(%d).ownClient%s.%s",
		entityTypeName_.c_str(),
		pMailBox_->id(),
		(pRecordingEntityChannel_ ? 
			(isRecordingOnly_ ? 
				"( shouldExposeForReplay=True, shouldRecordOnly=True )" : 
				"( shouldExposeForReplay=True )") :
			"( shouldExposeForReplay=False )"),
		pMethodDescription_->name().c_str() );
}


/**
 *	This method implements the methodName Python attribute accessor.
 */
PyObject * RemoteClientMethod::pyGet_methodName()
{
	return Script::getData( pMethodDescription_->name() );
}


BW_END_NAMESPACE


// remote_client_method.cpp
