
#include "shared_data.hpp"
#include "shared_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pickler.hpp"
#include "server/server_app_config.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/**
 *	This static function is used to implement operator[] for the scripting
 *	object.
 */
int SharedData::s_ass_subscript( PyObject * self,
		PyObject * index, PyObject * value )
{
	return ((SharedData *) self)->ass_subscript( index, value );
}

/**
 *	This static function is used to implement operator[] for the scripting
 *	object.
 */
PyObject * SharedData::s_subscript( PyObject * self, PyObject * index )
{
	return ((SharedData *) self)->subscript( index );
}


/**
 * 	This static function returns the number of entities in the system.
 */
Py_ssize_t SharedData::s_length( PyObject * self )
{
	return ((SharedData *) self)->length();
}


/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
static PyMappingMethods g_sharedDataMapping =
{
	SharedData::s_length,			// mp_length
	SharedData::s_subscript,		// mp_subscript
	SharedData::s_ass_subscript	// mp_ass_subscript
};


/*~ function SharedData has_key
 *  @components{ base, cell }
 *  has_key reports whether a value with a specific key is listed in this
 *	SharedData object.
 *  @param key The key to be searched for.
 *  @return A boolean. True if the key was found, false if it was not.
 */
/*~ function SharedData keys
 *  @components{ base, cell }
 *  keys returns a list of the keys of all of the shared data in this object.
 *  @return A list containing all of the keys.
 */
/*~ function SharedData items
 *  @components{ base, cell }
 *  items returns a list of the items, as (key, value) pairs.
 *  @return A list containing all of the (key, value) pairs.
 */
/*~ function SharedData values
 *  @components{ base, cell }
 *  values returns a list of all the values associated with this object.
 *  @return A list containing all of the values.
 */
/*~ function SharedData get
 *  @components{ base, cell }
 *  This method returns the value with the input key or the default value if not
 *	found.
 *
 *  @return The found value.
 */
PY_TYPEOBJECT_WITH_MAPPING( SharedData, &g_sharedDataMapping )

PY_BEGIN_METHODS( SharedData )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( items )
	PY_METHOD( values )
	PY_METHOD( get )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( SharedData )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	The constructor for SharedData.
 */
SharedData::SharedData( SharedDataType dataType,
		SharedData::SetFn setFn,
		SharedData::DelFn delFn,
		SharedData::OnSetFn onSetFn,
		SharedData::OnDelFn onDelFn,
		Pickler * pPickler,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	dataType_( dataType ),
	setFn_( setFn ),
	delFn_( delFn ),
	onSetFn_( onSetFn ),
	onDelFn_( onDelFn ),
	pPickler_( pPickler )
{
	pMap_ = PyDict_New();
}


/**
 *	Destructor.
 */
SharedData::~SharedData()
{
	Py_DECREF( pMap_ );
}


// -----------------------------------------------------------------------------
// Section: Script Mapping Related
// -----------------------------------------------------------------------------

/**
 *	This method finds the value associated with the input key.
 *
 * 	@param key 	The key of the value to find.
 *
 *	@return	A new reference to the object associated with the given key.
 */
PyObject * SharedData::subscript( PyObject * key )
{
	PyObject * pObject = PyDict_GetItem( pMap_, key );
	if (pObject == NULL)
	{
		PyErr_SetObject( PyExc_KeyError, key );
	}
	else
	{
		Py_INCREF( pObject );
	}

	return pObject;
}


/**
 *	This method sets a new shared data value.
 *
 * 	@param key 	The key of the value to set.
 * 	@param value 	The new value.
 *
 *	@return	0 on success, otherwise false.
 */
int SharedData::ass_subscript( PyObject* key, PyObject * value )
{
	BW::string pickledKey = this->pickle( ScriptObject( key,
			ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (PyErr_Occurred())
	{
		BW::string keyRepr = PyString_AsString( 
			PyObjectPtr( PyObject_Repr( key ), 
				PyObjectPtr::STEAL_REFERENCE ).get() );

		ERROR_MSG( "SharedData::ass_subscript( %s ): "
				"Failed to pickle key: %s\n",
			this->sharedDataTypeAsString(),
			keyRepr.c_str() );

		return -1;
	}

	ScriptObject pUnpickledKey = this->unpickle( pickledKey );

	if (!pUnpickledKey ||
			(PyObject_Compare( key, pUnpickledKey.get() ) != 0))
	{
		PyErr_SetString( PyExc_TypeError,
				"Unpickled key is not equal to original key" );
		return -1;
	}

	if (value == NULL)
	{
		int result = PyDict_DelItem( pMap_, key );

		if (result == 0)
		{
			(*delFn_)( pickledKey, dataType_ );
			this->changeOutstandingAcks( pickledKey, 1 /* delta */);
		}

		return result;
	}
	else
	{
		BW::string pickledValue = this->pickle( ScriptObject( value,
			ScriptObject::FROM_BORROWED_REFERENCE ) );

		if (PyErr_Occurred())
		{
			BW::string keyRepr = PyString_AsString( 
				PyObjectPtr( PyObject_Repr( key ), 
					PyObjectPtr::STEAL_REFERENCE ).get() );

			ERROR_MSG( "SharedData::ass_subscript( %s ): "
					"Failed to pickle value for key %s\n",
				this->sharedDataTypeAsString(),
				keyRepr.c_str() );
			return -1;
		}

		if ((pickledKey.size() + pickledValue.size()) > 
				ServerAppConfig::maxSharedDataValueSize())
		{
			BW::string keyRepr = PyString_AsString( 
				PyObjectPtr( PyObject_Repr( key ), 
					PyObjectPtr::STEAL_REFERENCE ).get() );

			WARNING_MSG( "SharedData::ass_subscript( %s ): "
						"Large change to shared data value "
						"(key: %zu bytes, value: %zu bytes). "
						"Recommended limit of %u bytes is set by the "
						"configuration option 'maxSharedDataValueSize'. "
						"Key: %s\n",
					this->sharedDataTypeAsString(),
					pickledKey.size(),
					pickledValue.size(),
					ServerAppConfig::maxSharedDataValueSize(),
					keyRepr.c_str() );
		}

		int result = PyDict_SetItem( pMap_, key, value );

		if (result == 0)
		{
			(*setFn_)( pickledKey, pickledValue, dataType_ );
			this->changeOutstandingAcks( pickledKey, 1 /* delta */);
		}

		return result;
	}
}


/**
 * 	This method returns the number of data entries.
 */
int SharedData::length()
{
	return PyDict_Size( pMap_ );
}


/**
 * 	This method returns true if the given entry exists.
 */
PyObject * SharedData::py_has_key( PyObject* args )
{
	return PyObject_CallMethod( pMap_, "has_key", "O", args );
}


/**
 * 	This method returns the value with the input key or the default value if not
 *	found.  
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * SharedData::py_get( PyObject* args )
{
	return PyObject_CallMethod( pMap_, "get", "O", args );
}


/**
 * 	This method returns a list of all the keys of this object.
 */
PyObject* SharedData::py_keys(PyObject* /*args*/)
{
	return PyDict_Keys( pMap_ );
}


/**
 * 	This method returns a list of all the values of this object.
 */
PyObject* SharedData::py_values( PyObject* /*args*/ )
{
	return PyDict_Values( pMap_ );
}


/**
 * 	This method returns a list of tuples of all the keys and values of this
 *	object.
 */
PyObject* SharedData::py_items( PyObject* /*args*/ )
{
	return PyDict_Items( pMap_ );
}


// -----------------------------------------------------------------------------
// Section: Misc.
// -----------------------------------------------------------------------------

/**
 *	This method sets a local SharedData entry from the input value.
 */
bool SharedData::setValue( const BW::string & key, const BW::string & value,
		bool isAck )
{
	if (isAck)
	{
		this->changeOutstandingAcks( key, -1 /* delta */ );
	}

	ScriptObject pKey = this->unpickle( key );
	ScriptObject pValue = this->unpickle( value );

	if (!pKey || !pValue)
	{
		ERROR_MSG( "SharedData::setValue( %s ): "
				"Unpickle failed. Invalid key(%s) or value(%s)\n",
			this->sharedDataTypeAsString(),
			key.c_str(), value.c_str() );

		PyErr_Print();
		return false;
	}

	if (!this->hasOutstandingAcks( key ))
	{
		if (PyDict_SetItem( pMap_, pKey.get(), pValue.get() ) == -1)
		{
			BW::string keyRepr = PyString_AsString( PyObjectPtr( 
				PyObject_Repr( pKey.get() ), 
					PyObjectPtr::STEAL_REFERENCE ).get() );

			ERROR_MSG( "SharedData::setValue( %s ): "
					"Failed to set value for key %s\n",
				this->sharedDataTypeAsString(), keyRepr.c_str() );
			PyErr_Print();
			return false;
		}
	}
	else
	{
		const char * msg = "SharedData::setValue( %s ): "
				"Received update (originally "
				"from %s app) for key %s before our local change has been "
				"acknowledged. Keeping local change.\n";

		ScriptObject pKeyRepr( PyObject_Repr( pKey.get() ),
				ScriptObject::STEAL_REFERENCE );
		const char * keyRepr = PyString_AsString( pKeyRepr.get() );
		const char * dataTypeString = this->sharedDataTypeAsString();

		if (isAck)
		{
			INFO_MSG( msg, dataTypeString, "this", keyRepr );
		}
		else
		{
			WARNING_MSG( msg, dataTypeString, "a remote", keyRepr );
		}
	}

	if (onSetFn_)
	{
		(*onSetFn_)( pKey.get(), pValue.get(), dataType_ );
	}

	return true;
}


/**
 *	This method deletes a local SharedData entry from the input value.
 */
bool SharedData::delValue( const BW::string & key, bool isAck )
{
	if (isAck)
	{
		this->changeOutstandingAcks( key, -1 );
	}

	ScriptObject pKey = this->unpickle( key );

	if (!pKey)
	{
		ERROR_MSG( "SharedData::delValue( %s ): Invalid key to delete (%s)\n",
			this->sharedDataTypeAsString(), key.c_str() );
		PyErr_Print();
		return false;
	}

	if (!this->hasOutstandingAcks( key ))
	{
		if (PyDict_GetItem( pMap_, pKey.get() ) && 
			PyDict_DelItem( pMap_, pKey.get() ) == -1)
		{
			ERROR_MSG( "SharedData::delValue( %s ): Failed to delete key.\n",
				this->sharedDataTypeAsString() );
			PyErr_Print();
			return false;
		}
	}
	else
	{
		const char * msg = "SharedData::delValue( %s ): "
			"Received update (originally from %s app) for key %s before our "
			"local change has been acknowledged. Keeping local change.\n";

		ScriptObject pKeyRepr( PyObject_Repr( pKey.get() ),
				ScriptObject::STEAL_REFERENCE );
		const char * keyRepr = PyString_AsString( pKeyRepr.get() );
		const char * dataTypeString = this->sharedDataTypeAsString();

		if (isAck)
		{
			INFO_MSG( msg, dataTypeString, "this", keyRepr );
		}
		else
		{
			WARNING_MSG( msg, dataTypeString, "a remote", keyRepr );
		}
	}

	if (onDelFn_)
	{
		(*onDelFn_)( pKey.get(), dataType_ );
	}

	return true;
}


/**
 *	This method adds the dictionary to the input stream.
 */
bool SharedData::addToStream( BinaryOStream & stream ) const
{
	uint32 size = PyDict_Size( pMap_ );
	stream << size;

	PyObject * pKey;
	PyObject * pValue;
	Py_ssize_t pos = 0;

	while (PyDict_Next( pMap_, &pos, &pKey, &pValue ))
	{
		stream << this->pickle( ScriptObject( pKey,
				ScriptObject::FROM_BORROWED_REFERENCE ) ) 
			<< this->pickle( ScriptObject( pValue,
				ScriptObject::FROM_BORROWED_REFERENCE ) );
	}

	MF_ASSERT( !PyErr_Occurred() );

	return true;
}


/**
 *	This method pickles the input object.
 */
BW::string SharedData::pickle( ScriptObject pObj ) const
{
	return pPickler_->pickle( pObj );
}


/**
 *	This method changes the number of outstanding acks. Whenever a shared
 *	data value is modified, this will be called with a delta of 1, and when
 *	the corresponding message is returned broadcasting the new value, this
 *	will be called with a delta of -1. A value of zero means there are no
 *	outstanding acks, and that entry will be removed.
 */
void SharedData::changeOutstandingAcks( const BW::string & key, int delta )
{
	int & entry = outstandingAcks_[ key ];

	entry += delta;
	MF_ASSERT( entry >= 0 );

	if (entry == 0)
	{
		outstandingAcks_.erase( key );
	}
}


/*
 *	This method returns true if the given key has outstanding acks, meaning
 *	we have not received the message containing a new value that we had
 *	previously set to it. This method returns false if there are no outstanding
 *	acks.
 */
bool SharedData::hasOutstandingAcks( const BW::string & key ) const
{
	return (outstandingAcks_.find( key ) !=
			outstandingAcks_.end());
}


/**
 *	This method unpickles the input data.
 */
ScriptObject SharedData::unpickle( const BW::string & str ) const
{
	return pPickler_->unpickle( str );
}


/**
 *	This method returns a human-readable string description for the data type
 *	of this instance.
 */
const char * SharedData::sharedDataTypeAsString() const
{
	switch (dataType_)
	{
	case SHARED_DATA_TYPE_CELL_APP:
		return "cellAppData";
	case SHARED_DATA_TYPE_BASE_APP:
		return "baseAppData";
	case SHARED_DATA_TYPE_GLOBAL:
		return "globalData";
	case SHARED_DATA_TYPE_GLOBAL_FROM_BASE_APP:
		return "globalData (from BaseApp)";
	default:
		return "(unknown)";
	}
}


BW_END_NAMESPACE

// shared_data.cpp
