#include "script/first_include.hpp"

#include "client_entity_mailbox_wrapper.hpp"

#include "proxy.hpp"
#include "remote_client_method.hpp"


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{


/**
 *	This function is used to implement the tp_repr and tp_str for the Python
 *	type object for ClientEntityMailBoxWrapper.
 */
PyObject * reprClientEntityMailBoxWrapper( PyObject * self )
{
	return reinterpret_cast< ClientEntityMailBoxWrapper * >( self )->pyRepr();
}

} // end namespace (anonymous)


PY_TYPEOBJECT_SPECIALISE_REPR_AND_STR( ClientEntityMailBoxWrapper, 
	&reprClientEntityMailBoxWrapper, &reprClientEntityMailBoxWrapper );
PY_TYPEOBJECT_WITH_CALL( ClientEntityMailBoxWrapper )

PY_BEGIN_ATTRIBUTES( ClientEntityMailBoxWrapper )
PY_END_ATTRIBUTES()


PY_BEGIN_METHODS( ClientEntityMailBoxWrapper )
PY_END_METHODS()


/**
 *	Constructor.
 *
 *	@param mailBox 				The mail box to wrap.
 *	@param recordingOption		See the RecordingOption enum.
 */
ClientEntityMailBoxWrapper::ClientEntityMailBoxWrapper( 
			ClientEntityMailBox & mailBox,
			RecordingOption recordingOption ) :
		PyObjectPlus( &ClientEntityMailBoxWrapper::s_type_ ),
		pMailBox_( &mailBox ),
		recordingOption_( recordingOption )
{
	pMailBox_->incRef();
}


/**
 *	Destructor.
 */
ClientEntityMailBoxWrapper::~ClientEntityMailBoxWrapper()
{
	pMailBox_->decRef();
}


/**
 *	This method implements the generic attribute accessor (__getattribute__).
 */
ScriptObject ClientEntityMailBoxWrapper::pyGetAttribute( 
	const ScriptString & attrObj )
{
	const MethodDescription * pDescription = pMailBox_->findMethod( 
		attrObj.c_str() );

	if (pDescription)
	{
		return ScriptObject( new RemoteClientMethod( pMailBox_, 
				pMailBox_->getEntityDescription().name(),
				pDescription, 
				pMailBox_->proxy().channel(),
				recordingOption_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return pMailBox_->pyGetAttribute( attrObj );
}


/**
 *  This method returns the Python string representation of instances of this
 *  class.
 */
PyObject * ClientEntityMailBoxWrapper::pyRepr() const
{
	PyObjectPtr pMailBoxStr( PyObject_Repr( pMailBox_ ), 
		PyObjectPtr::STEAL_REFERENCE );

	const EntityDescription & entityDescription = 
		pMailBox_->getEntityDescription();

	const bool shouldRecord = (recordingOption_ >= RECORDING_OPTION_RECORD);
	const bool shouldRecordOnly =
		(recordingOption_ == RECORDING_OPTION_RECORD_ONLY);

	return PyString_FromFormat( "Client MailBox %s(%d).ownClient%s",
		entityDescription.name().c_str(),
		pMailBox_->id(),
		shouldRecord ? 
			(shouldRecordOnly ? 
				"( shouldExposeForReplay=True, shouldRecordOnly=True )" : 
				"( shouldExposeForReplay=True )") :
			"( shouldExposeForReplay=False )" );
}


/**
 *	This method implements the tp_call operator for Python.
 */
PyObject * ClientEntityMailBoxWrapper::pyCall( PyObject * args, 
		PyObject * kwargs )
{
	return pMailBox_->pyCall( args, kwargs );
}

BW_END_NAMESPACE


// client_entity_mailbox_wrapper.cpp

