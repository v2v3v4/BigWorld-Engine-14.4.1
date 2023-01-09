#include "script/first_include.hpp"
#include "client_entity_mailbox.hpp"

#include "client_entity_mailbox_wrapper.hpp"
#include "proxy.hpp"
#include "remote_client_method.hpp"

#include "entitydef/method_description.hpp"

#include "network/bundle.hpp"

#include "server/event_history_stats.hpp"

#include "pyscript/keyword_parser.hpp"

#include <string.h>


BW_BEGIN_NAMESPACE


PY_TYPEOBJECT_WITH_CALL( ClientEntityMailBox )

PY_BEGIN_METHODS( ClientEntityMailBox)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClientEntityMailBox )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 *
 *	@param proxy 	The proxy for this mailbox.
 */
ClientEntityMailBox::ClientEntityMailBox( Proxy & proxy ) :
		PyEntityMailBox( &ClientEntityMailBox::s_type_ ),
		proxy_( proxy )
{}


/*
 *	Override from PyEntityMailBox.
 */
EntityID ClientEntityMailBox::id() const
{ 
	return proxy_.id(); 
}


/*
 *	Override from PyEntityMailBox.
 */
const Mercury::Address ClientEntityMailBox::address() const
{
	return proxy_.clientAddr();
}


// __kyl__(17/8/2007) Copied from PyEntityMailBox::pyGetAttribute to create a
// RemoteClientMethod instead of a RemoteEntityMethod.
/*
 *	Override from PyEntityMailBox.
 */
ScriptObject ClientEntityMailBox::pyGetAttribute( const ScriptString & attrObj )
{
	const MethodDescription * pDescription = 
		this->findMethod( attrObj.c_str() );
	if (pDescription != NULL)
	{
		return ScriptObject( new RemoteClientMethod( this, 
			this->getEntityDescription().name(),
			pDescription,
			proxy_.channel() ), ScriptObject::FROM_NEW_REFERENCE );
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}


/*
 *	Override from PyEntityMailBox.
 */
BinaryOStream *	ClientEntityMailBox::getStream( 
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	// Not supporting return values

	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' to Client",
				methodDesc.name().c_str() );
		return NULL;
	}

	if (!proxy_.hasClient())
	{
		PyErr_Format( PyExc_TypeError,
				"Error calling %s no client is available.",
				methodDesc.name().c_str() );
		return NULL;
	}

	Mercury::Bundle & bundle = proxy_.clientBundle();

	bundle.startMessage( ClientInterface::selectPlayerEntity );

	return proxy_.getStreamForEntityMessage(
					methodDesc.exposedMsgID(), methodDesc.streamSize( true ) );
}


/*
 *	Override from PyEntityMailBox.
 */
void ClientEntityMailBox::sendStream()
{
	// we don't actually send the stream here; we wait for the 'sendToClient'
	// message from the cell to send it off.
}


/**
 *	This method overrides the virtual findMethod function. It returns the
 *	client MethodDescription with the input name.
 */
const MethodDescription * ClientEntityMailBox::findMethod( 
	const char * attr ) const
{
	return proxy_.pType()->description().client().find( attr );
}


/**
 *	This method returns the mailbox reference for this object.
 */
EntityMailBoxRef ClientEntityMailBox::ref() const
{
    EntityMailBoxRef mbr; mbr.init(
        proxy_.id(), proxy_.clientAddr(),
		EntityMailBoxRef::CLIENT, proxy_.pType()->description().index() );
    return mbr;
}


/**
 *	This method returns the entity description of the proxy entity.
 */
const EntityDescription& ClientEntityMailBox::getEntityDescription() const
{
	return proxy_.pType()->description();
}


/**
 *	This method implements the tp_call operator for Python.
 */
PyObject * ClientEntityMailBox::pyCall( PyObject * args, PyObject * kwargs )
{
	RecordingOption recordingOption = RECORDING_OPTION_METHOD_DEFAULT;

	if (!this->parseRecordingOptionFromPythonCall( args, kwargs,
			recordingOption ))
	{
		return NULL;
	}

	if (recordingOption == RECORDING_OPTION_METHOD_DEFAULT)
	{
		this->incRef();
		return this;
	}

	return reinterpret_cast< PyObject * >( 
			new ClientEntityMailBoxWrapper( *this, recordingOption ) );
}


namespace // (anonymous)
{

/**
 *	This class does static initialisation for ClientEntityMailBox.
 */
class StaticIniter
{
public:
	StaticIniter()
	{
		PyEntityMailBox::registerMailBoxRefEquivalent(
			ClientEntityMailBox::Check, ClientEntityMailBox::static_ref );
	}
};

StaticIniter staticIniter;

} // end namespace (anonymous)

BW_END_NAMESPACE

// client_entity_mailbox.cpp
