#ifndef REMOTE_CLIENT_METHOD_HPP
#define REMOTE_CLIENT_METHOD_HPP


#include "entitydef/mailbox_base.hpp"
#include "network/channel.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "server/recording_options.hpp"


class MethodDescription;


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RemoteClientMethod
// -----------------------------------------------------------------------------

/**
 *  This class implements a simple helper Python type. Objects of this type are
 *  used to represent methods that the base can call on another script object
 *  (e.g. the cell, the client, or another base).
 */
class RemoteClientMethod : public PyObjectPlus
{
	Py_Header( RemoteClientMethod, PyObjectPlus )

public:
	RemoteClientMethod( PyEntityMailBox * pMailBox, 
		const BW::string & entityTypeName,
		const MethodDescription * pMethodDescription, 
		Mercury::Channel & cellChannel, 
		RecordingOption recordingOption = RECORDING_OPTION_METHOD_DEFAULT,
		PyTypeObject * pType = &s_type_ );

	/** Destructor. */
	~RemoteClientMethod() {}
	
	PY_RO_ATTRIBUTE_DECLARE( pMailBox_->id(), entityID );

	PyObject * PY_ATTR_SCOPE pyGet_methodName();
	PY_RO_ATTRIBUTE_SET( methodName )

	PY_RO_ATTRIBUTE_DECLARE( pRecordingEntityChannel_.get() != NULL, 
		isRecording );
	PY_RO_ATTRIBUTE_DECLARE( isRecordingOnly_, isRecordingOnly );

	PY_METHOD_DECLARE( pyCall )

	PyObject * pyRepr() const;

private:
	SmartPointer< PyEntityMailBox > 	pMailBox_;

	BW::string 							entityTypeName_;
	const MethodDescription * 			pMethodDescription_;
	Mercury::ChannelPtr 				pRecordingEntityChannel_;
	bool 								isRecordingOnly_;
};

BW_END_NAMESPACE


#endif // REMOTE_CLIENT_METHOD_HPP
