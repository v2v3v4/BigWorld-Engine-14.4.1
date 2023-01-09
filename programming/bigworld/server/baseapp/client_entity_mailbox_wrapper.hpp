#ifndef CLIENT_ENTITY_MAILBOX_WRAPPER_HPP
#define CLIENT_ENTITY_MAILBOX_WRAPPER_HPP

#include "client_entity_mailbox.hpp"
#include "remote_client_method.hpp"

#include "pyscript/pyobject_plus.hpp"

#include "server/recording_options.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class wraps a ClientEntityMailBox to create client method callers that
 *	will also send the invocation data to the cell entity for recording.
 */
class ClientEntityMailBoxWrapper : public PyObjectPlus
{
	Py_Header( ClientEntityMailBoxWrapper, PyObjectPlus );

public:
	ClientEntityMailBoxWrapper( ClientEntityMailBox & mailBox, 
		RecordingOption recordingOption );
	~ClientEntityMailBoxWrapper();

	ScriptObject pyGetAttribute( const ScriptString & attrObj );

	PY_KEYWORD_METHOD_DECLARE( pyCall )

	PyObject * pyRepr() const;

private:
	ClientEntityMailBox * pMailBox_;

	RecordingOption recordingOption_;
};


BW_END_NAMESPACE


#endif // CLIENT_ENTITY_MAILBOX_WRAPPER_HPP
