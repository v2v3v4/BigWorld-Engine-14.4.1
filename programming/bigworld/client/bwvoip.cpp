#include "pch.hpp"

#include "bwvoip.hpp"
#include "voip_app.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

PY_MODULE_ATTRIBUTE( BigWorld, VOIP, new PyVOIP );

PY_TYPEOBJECT( PyVOIP )

PY_BEGIN_METHODS( PyVOIP )
	PY_METHOD( initialise )
	PY_METHOD( finalise )
	PY_METHOD( command )
	PY_METHOD( getStatistics )
	PY_METHOD( setHandler )
	PY_METHOD( login )
	PY_METHOD( logout )
	PY_METHOD( createChannel )
	PY_METHOD( joinChannel )
	PY_METHOD( leaveChannel )
	PY_METHOD( queryChannels )
	PY_METHOD( inviteUser )
	PY_METHOD( kickUser )
	PY_METHOD( enableActiveChannel )
	PY_METHOD( disableActiveChannel )
	PY_METHOD( enablePositional )
	PY_METHOD( disablePositional )
	PY_METHOD( enableMicrophone )
	PY_METHOD( disableMicrophone )
	PY_METHOD( setMasterVolume )
	PY_METHOD( setChannelVolume )
	PY_METHOD( setMicrophoneVolume )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyVOIP )
PY_END_ATTRIBUTES()


int PyVOIP_token = 1;


/**
 *	This is an immutable implementation of the VOIPDataDict interface.
 *	It provides the ability to conveniently construct it from a python dictionary. 
 */
class BWVOIPDataDict : public VOIPDataDict
{
public:
	BWVOIPDataDict( PyObject * pyDict );

	virtual const char * get( const char * key ) const;
	virtual bool get( unsigned int index, const char ** key, const char ** value ) const;
	virtual size_t size() const;

private:
	BW::map<BW::string, BW::string> dict_;
};



/**
 *	Constructor
 *
 *	Populates the data dictionary with every string:string entry in the provided python dictionary. 
 */
BWVOIPDataDict::BWVOIPDataDict( PyObject * pyDict )
{
	BW_GUARD;
	MF_ASSERT_DEV( pyDict == NULL || PyDict_Check( pyDict ) );

	if( pyDict )
	{
		PyObject * pyKey;
		PyObject * pyValue;
		Py_ssize_t pos = 0;

		while ( PyDict_Next( pyDict, &pos, &pyKey, &pyValue ) )
		{
			if( PyString_Check( pyKey ) && PyString_Check( pyValue ) )
			{
				BW::string key( PyString_AsString( pyKey ) );
				BW::string value( PyString_AsString( pyValue ) );

				dict_.insert( std::pair<BW::string,BW::string>( key, value ) );
			}
		}
	}
}


/**
 *	Returns the string value associated with the given key. 
 *	If the key is not found NULL is returned.
 *
 *	@param	key			The key/name of the value to retrieve.
 *
 *	@return				A pointer to the value corresponding to the given key. NULL if not found.
 */
const char * BWVOIPDataDict::get( const char * key ) const
{
	BW_GUARD;
	BW::map<BW::string, BW::string>::const_iterator result;
	result = dict_.find( key );

	if( result != dict_.end() )
		return result->second.c_str();
	else
		return NULL;
}


/**
 *	This function retrieves a key value pair based on a given 
 *	index less than size(). The pairs need not be in any order. 
 *	The intended purpose being to handle situations where the 
 *	dictionary needs to be cloned into another format such as 
 *	a python object.
 *
 *	@param	index		An index less than size() that uniquely 
 *						identifies a key value pair.
 *	@param	key			Output. On success this pointer will 
 *						point to a string containing the key. 
 *						This pointer will be left untouched on error.
 *	@param	value		Output. On success this pointer will 
 *						point to a string containing the value. 
 *						This pointer will be left untouched on error.
 *
 *	@return				Returns true on success, false on failure
 */
bool BWVOIPDataDict::get( unsigned int index, const char ** key, const char ** value ) const
{
	BW_GUARD;
	if( index >= dict_.size() )
		return false;

	BW::map<BW::string, BW::string>::const_iterator result = dict_.begin();

	std::advance( result, index );

	if( result != dict_.end() )
	{
		*key = result->first.c_str();
		*value = result->second.c_str();
		return true;
	}

	return false;
}


/**
 *	Returns the number key value pairs held by the object.
 *
 *	@return				The number of pairs.
 */
size_t BWVOIPDataDict::size() const
{
	return dict_.size();
}



/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the VOIP module.
 */
PyVOIP::PyVOIP( PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	response_( NULL )
{

}


/**
 *	Destructor
 */
PyVOIP::~PyVOIP()
{
	BW_GUARD;
	bw_safe_delete(response_);
}


/*~ function BigWorld.VOIP.initialise
 *
 *	This function initialises the VoIP client.
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Initialises the VoIP client.
 */
PyObject * PyVOIP::py_initialise( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "|O:VOIP.initialise", &pyData ))
	{
		return NULL;
	}
	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.initialise: "
			"expected dict for argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().initialise( &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.finalise
 *
 *	This function instructs the VoIP client to clean up all its connections and resources.
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Cleans up resources allocated by the VoIP client
 */
PyObject * PyVOIP::py_finalise( PyObject * args )
{
	BW_GUARD;
	VOIPApp::getVOIPClient().finalise();

	Py_RETURN_NONE;
}


/*~	function BigWorld.VOIP.command
 *
 *	This is a generic function for handling implementation specific 
 *	commands not exposed in the general VoIP interface.
 *
 *	@param	data			(dictionary)	Additional information in the form of string value pairs
 */
/**
 *	Provides a generic way to pass commands to the VoIP client.
 */
PyObject * PyVOIP::py_command( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "O:VOIP.command", &pyData ))
	{
		return NULL;
	}

	if (!PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.command: "
			"expecting dict for argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().command( &data );

	Py_RETURN_NONE;
}


/*~	function BigWorld.VOIP.getStatistics
 *
 *	Requests an implementation specific a statistics report that 
 *	will be sent back to the application via the response callback mechanism.
 */
/**
 *	Requests a statistics report to be returned using the callback mechanism.
 */
PyObject * PyVOIP::py_getStatistics( PyObject * args )
{
	BW_GUARD;
	VOIPApp::getVOIPClient().getStatistics();

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.setHandler
 *
 *	Sets a callback function that can be used by the VOIPClient 
 *	implementation. The function augments will be of the form (int, dictionary).
 *
 *	@param	callback	(callable)		A callable object that can handle
 *										arguments of the form (int, dictionary)
 */
/**
 *	Sets the callback handler.
 */
PyObject * PyVOIP::py_setHandler( PyObject * args )
{
	BW_GUARD;
	PyObject * pCallback = NULL;

	if (!PyArg_ParseTuple( args, "O:VOIP.setHandler", &pCallback ))
	{
		return NULL;
	}

	if (pCallback != Py_None && !PyCallable_Check( pCallback ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.setHandler: "
			"expecting a callable object or None for argument 1" );
		return NULL;
	}

	// PyVOIPResponse INCREFs and DECREFs callback
	bw_safe_delete(response_);

	if( pCallback )
	{
		response_ = new PyVOIPResponse( &VOIPApp::getVOIPClient(), pCallback );
	}

	VOIPApp::getVOIPClient().setHandler( response_ );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.login
 *
 *	This function logs the client into the VoIP service.
 *
 *	@param	username		(string)		The string that uniquely identifies the user's account
 *	@param	password		(string)		The corresponding password to access the account
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Logs the user into the VoIP service.
 */
PyObject * PyVOIP::py_login( PyObject * args )
{
	BW_GUARD;
	const char * username = NULL;
	const char * password = NULL;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "ss|O:VOIP.login", &username, &password, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.login: "
			"expecting dict for optional argument 3 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().login( username, password, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.logout
 *
 *	This function logs the client out of the VoIP service.
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Logs the user out of the VoIP service.
 */
PyObject * PyVOIP::py_logout( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "|O:VOIP.logout", &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.logout: "
			"expecting dict for optional argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().logout( &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.createChannel
 *
 *	Creates a new channel with the given name.
 *	The creating user automatically joins the newly created channel.
 *	The channel will be destroyed when all users leave the channel.
 *	Default password is "".
 *
 *	@param	channelName		(string)		The string that uniquely identifies the new channel
 *	@param	password		(string)		Optional: The password that will be needed to access the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Creates a new channel that other users can join.
 */
PyObject * PyVOIP::py_createChannel( PyObject * args )
{
	BW_GUARD;
	const char * channel = NULL;
	const char * password = "";
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "s|sO:VOIP.createChannel",
			&channel, &password, &pyData ))
	{
		return NULL;
	}
	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.createChannel: "
			"expecting dict for optional argument 3 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().createChannel( channel, password, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.joinChannel
 *
 *	Joins an existing channel. Default password is "".
 *
 *	@param	channelName		(string)		The string identifying the channel
 *	@param	password		(string)		Optional: The password that will be needed to access the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Joins a previously created channel.
 */
PyObject * PyVOIP::py_joinChannel( PyObject * args )
{
	BW_GUARD;
	const char * channel = NULL;
	const char * password = "";
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "s|sO:VOIP.joinChannel",
			&channel, &password, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.joinChannel: "
			"expecting dict for optional argument 3 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().joinChannel( channel, password, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.leaveChannel
 *
 *	Leaves a channel that the client is already connected to.
 *
 *	@param	channelName		(string)		The string identifying the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Leaves a channel the user has previously joined or created.
 */
PyObject * PyVOIP::py_leaveChannel( PyObject * args )
{
	BW_GUARD;
	const char * channel = NULL;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "s|O:VOIP.leaveChannel", &channel, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.leaveChannel: "
			"expecting dict for optional argument 2 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().leaveChannel( channel, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.queryChannels
 *
 *	Returns the list of channels the client is connected to.
 *
 *	@return					[(string),...]	The list of channel names
 */
/**
 *	Retrieves the name of each channel the user is connected to.
 */
PyObject * PyVOIP::py_queryChannels( PyObject * args )
{
	BW_GUARD;
	int number = VOIPApp::getVOIPClient().numberChannels();

	PyObject* channels = PyList_New( 0 );

	for( int i = 0; i < number; i++ )
	{
		PyList_Append( channels, PyString_FromString( VOIPApp::getVOIPClient().getChannel( i ) ) );
	}

	return channels;
}


/*~ function BigWorld.VOIP.inviteUser
 *
 *	Sends an invite to another VoIP user to join a channel.
 *
 *	@param	userName		(string)		The string identifying the user's VoIP account
 *	@param	channelName		(string)		The string identifying the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Sends an invite for another user to join a channel.
 */
PyObject * PyVOIP::py_inviteUser( PyObject * args )
{
	BW_GUARD;
	const char * user = NULL;
	const char * channel = NULL;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "ss|O:VOIP.inviteUser",
			&user, &channel, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.inviteUser: "
			"expecting dict for optional argument 3 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().inviteUser( user, channel, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.kickUser
 *
 *	Kicks a user from a channel
 *
 *	@param	userName		(string)		The string identifying the user's VoIP account
 *	@param	channelName		(string)		The string identifying the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Kicks a user from a channel.
 */
PyObject * PyVOIP::py_kickUser( PyObject * args )
{
	BW_GUARD;
	const char * user = NULL;
	const char * channel = NULL;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "ss|O:VOIP.kickUser", &user, &channel, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.inviteUser: "
			"expecting dict for optional argument 3 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().kickUser( user, channel, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.enableActiveChannel
 *
 *	Enables voice transmission on the specified channel.
 *
 *	@param	channelName		(string)		The string identifying the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Enables voice transmission on the specified channel.
 */
PyObject * PyVOIP::py_enableActiveChannel( PyObject * args )
{
	BW_GUARD;
	const char * channel = NULL;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "s|O:VOIP.enableActiveChannel",
			&channel, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.enableActiveChannel: "
			"expecting dict for optional argument 2 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().enableActiveChannel( channel, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.disableActiveChannel
 *
 *	Disables voice transmission on the specified channel.
 *
 *	@param	channelName		(string)		The string identifying the channel
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Disables voice transmission on the specified channel.
 */
PyObject * PyVOIP::py_disableActiveChannel( PyObject * args )
{
	BW_GUARD;
	const char * channel = NULL;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "s|O:VOIP.disableActiveChannel",
			&channel, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.disableActiveChannel: "
			"expecting dict for optional argument 2 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().disableActiveChannel( channel, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.enablePositional
 *
 *	Enables positional audio filtering
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Enables positional audio filtering
 */
PyObject * PyVOIP::py_enablePositional( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "|O:VOIP.enablePositional", &pyData ))
	{
		return NULL;
	}
	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.enablePositional: "
			"expecting dict for optional argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().enablePositional( &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.disablePositional
 *
 *	Disables positional audio filtering
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Disables positional audio filtering
 */
PyObject * PyVOIP::py_disablePositional( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "|O:VOIP.disablePositional", &pyData ))
	{
		return NULL;
	}
	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.disablePositional: "
			"expecting dict for optional argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().disablePositional( &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.enableMicrophone
 *
 *	Enables microphone input.
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Enables microphone input.
 */
PyObject * PyVOIP::py_enableMicrophone( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "|O:VOIP.enableMicrophone", &pyData ))
	{
		return NULL;
	}
	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.enableMicrophone: "
			"expecting dict for optional argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().enableMicrophone( &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.disableMicrophone
 *
 *	Disables microphone input.
 *
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Disables microphone input.
 */
PyObject * PyVOIP::py_disableMicrophone( PyObject * args )
{
	BW_GUARD;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "|O:VOIP.disableMicrophone", &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.disableMicrophone: "
			"expecting dict for optional argument 1 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().disableMicrophone( &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.setMasterVolume
 *
 *	Sets the overall volume at which voice communications will be played.
 *
 *	@param	attenuation		(float)			The new speaker volume attenuation in decibels(dB)
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Sets the overall volume at which voice communications will be played.
 */
PyObject * PyVOIP::py_setMasterVolume( PyObject * args )
{
	BW_GUARD;
	float attenuation = 0.0f;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "f|O:VOIP.setMasterVolume",
			&attenuation, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.setMasterVolume: "
			"expecting dict for optional argument 2 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().setMasterVolume( attenuation, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.setChannelVolume
 *
 *	Sets the volume at which voice communications on the given channel play.
 *	This attenuation is in addition to that applied by the master volume.
 *
 *	@param	attenuation		(float)			The new speaker volume attenuation in decibels(dB)
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Sets the volume at which voice communications on the given channel play.
 *	This attenuation is in addition to that applied by the master volume.
 */
PyObject * PyVOIP::py_setChannelVolume( PyObject * args )
{
	BW_GUARD;
	const char * channelName = NULL;
	float attenuation = 0.0f;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "sf|O:VOIP.setChannelVolume",
			&channelName, &attenuation, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.setChannelVolume: "
				"expecting dict for optional argument 3 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().setChannelVolume( channelName, attenuation, &data );

	Py_RETURN_NONE;
}


/*~ function BigWorld.VOIP.setMicrophoneVolume
 *
 *	Sets the attenuation applied to microphone capture.
 *
 *	@param	attenuation		(float)			The new microphone attenuation in decibels(dB)
 *	@param	data			(dictionary)	Optional: Additional information in the form of string value pairs
 *
 *	@return					(none)
 */
/**
 *	Sets the attenuation applied to microphone capture.
 */
PyObject * PyVOIP::py_setMicrophoneVolume( PyObject * args )
{
	BW_GUARD;
	float attenuation = 0.0f;
	PyObject * pyData = NULL;

	if (!PyArg_ParseTuple( args, "f|O:VOIP.setMicrophoneVolume",
			&attenuation, &pyData ))
	{
		return NULL;
	}

	if (pyData != NULL && !PyDict_Check( pyData ))
	{
		PyErr_SetString( PyExc_TypeError, "VOIP.setMicrophoneVolume: "
			"expecting dict for optional argument 2 (data)" );
		return NULL;
	}

	BWVOIPDataDict data( pyData );

	VOIPApp::getVOIPClient().setMicrophoneVolume( attenuation, &data );

	Py_RETURN_NONE;
}


/**
 *	A tick function called each frame to allow positional 
 *	audio filtering and unthreaded implementations.
 *	Note: The given location is in world space
 *
 *	@param	x			The location of the player in on the X axis. +X is right
 *	@param	y			The location of the player in on the Y axis. +Y is up
 *	@param	z			The location of the player in on the Z axis. +Z is forward
 *	@param	yaw			The current yaw of the player. 0.0f yaw is forward, +/- PI is backard
 *	@param	pitch		The current pitch of the player. 0.0f pitch is level, +ve pitch is up (clockwise in Z-Y plane)
 *	@param	roll		
 *	@param	spaceID		An integer uniquely identifying the space in which the player currently resides
 */
void PyVOIP::tick( float x, float y, float z, float yaw, float pitch, float roll, int spaceID )
{
	BW_GUARD;
	VOIPApp::getVOIPClient().tick( x, y, z, yaw, pitch, roll, spaceID );
}


/**
 *	Constructor
 *
 *	@param	voip		The VOIPClient the response will be handling callbacks to.
 *	@param	callback	A callable python object that accepts an argument list of the form (int, dictionary).
 */
PyVOIPResponse::PyVOIPResponse( VOIPClient * voip, PyObject * callback ) :
	callback_( callback )
{
	BW_GUARD;
	Py_XINCREF( callback_ );
}


/**
 *	Destructor
 */
PyVOIPResponse::~PyVOIPResponse()
{
	BW_GUARD;
	Py_XDECREF( callback_ );
}


/**
 *	This function passes the callback on to the python function 
 *	given when the PyVOIPResponce was constructed.
 *
 *	@param	message		An integer with a VoIP implementation defined meaning.
 *	@param	data		A set of string value pairs that will be passed into 
 *						the script as a python dictionary.
 */
void PyVOIPResponse::response( int message, const VOIPDataDict * data )
{
	BW_GUARD;
	PyObject * pyDict = PyDict_New();

	if( data )
	{
		const size_t dataSize = data->size();
		for( uint i = 0; i < dataSize; i++ )
		{
			const char * key;
			const char * value;

			if( data->get( i, &key, &value ) )
			{
				PyObject * pyKey = PyString_FromString( key );
				PyObject * pyValue = PyString_FromString( value );

				PyDict_SetItem( pyDict, pyKey, pyValue );

				Py_XDECREF( pyKey );
				Py_XDECREF( pyValue );
			}
		}
	}

	PyObject* args = Py_BuildValue( "(i,N)", message, pyDict );
	PyObject* result = PyObject_CallObject( callback_, args );

	Py_XDECREF( result );
	Py_XDECREF( args );
}

BW_END_NAMESPACE

// bwvoip.cpp
