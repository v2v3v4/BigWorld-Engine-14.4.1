#include "pch.hpp"

#include "script_data_source.hpp"

#if !defined( EDITOR_ENABLED )
#if defined( SCRIPT_PYTHON )
#include "chunk/user_data_object.hpp"
#endif // defined( SCRIPT_PYTHON )
#endif // !defined( EDITOR_ENABLED )

#include "cstdmf/debug.hpp"

#if defined( MF_SERVER )
#include "entitydef/mailbox_base.hpp"
#endif // defined( MF_SERVER )

#include "entitydef/data_types/fixed_dict_data_type.hpp"
#if defined( SCRIPT_PYTHON )
#include "entitydef/data_types/user_data_type.hpp"
#endif // defined( SCRIPT_PYTHON )

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/class_data_instance.hpp"
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#endif // defined( SCRIPT_PYTHON )

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


namespace BW
{

namespace // Anonymous namespace for static functions
{

/**
 *	This function populates the given reference from the given object reference.
 *
 *	@param	object	A ScriptObject reference to read from. Must exist.
 *	@param	value	A value reference to populate from the ScriptObject.
 *
 *	@return			true unless the object doesn't, or the value could not be
 *					coerced from the ScriptObject.
 */
template< typename inputT >
inline static bool getFromObject( const ScriptObject & object, inputT & value )
{
	if (!object.exists())
	{
		ERROR_MSG( "getFromObject<>: Tried to get value from unset object\n" );
		return false;
	}

	return object.convertTo( value, ScriptErrorPrint() );
}


/**
 *	This function populates the given reference from the given object reference.
 *
 *	@param	object	A ScriptObject reference to read from. Must exist.
 *	@param	value	A value reference to populate from the ScriptObject.
 *
 *	@return			true unless the object doesn't, or the value could not be
 *					coerced from the ScriptObject.
 */
inline static bool getFromObject( const ScriptObject & object, Vector2 & value )
{
	// Needed because VectorDataType accepts any ScriptSequence of the right
	// length, but ScriptObject::convertTo (via Script::setData) only accepts
	// ScriptTuples of the right length.

	if (!object.exists())
	{
		ERROR_MSG( "getFromObject<>: Tried to get value from unset object\n" );
		return false;
	}

	ScriptErrorPrint scriptErrorPrint( "getFromObject" );
	ScriptSequence sequence = ScriptSequence::create( object );

	if (sequence.exists() && sequence.size() == 2)
	{
		float x;
		float y;

		ScriptObject xObj = sequence.getItem( 0, scriptErrorPrint );

		if (!xObj.exists() || !xObj.convertTo( x, scriptErrorPrint ))
		{
			return false;
		}

		ScriptObject yObj = sequence.getItem( 1, scriptErrorPrint );

		if (!yObj.exists() || !yObj.convertTo( y, scriptErrorPrint ))
		{
			return false;
		}

		value.set( x, y );
	}
	else 
	{
		return object.convertTo( value, scriptErrorPrint );
	}

	return true;
}


/**
 *	This function populates the given reference from the given object reference.
 *
 *	@param	object	A ScriptObject reference to read from. Must exist.
 *	@param	value	A value reference to populate from the ScriptObject.
 *
 *	@return			true unless the object doesn't, or the value could not be
 *					coerced from the ScriptObject.
 */
inline static bool getFromObject( const ScriptObject & object, Vector3 & value )
{
	// Needed because VectorDataType accepts any ScriptSequence of the right
	// length, but ScriptObject::convertTo (via Script::setData) only accepts
	// ScriptTuples of the right length.

	if (!object.exists())
	{
		ERROR_MSG( "getFromObject<>: Tried to get value from unset object\n" );
		return false;
	}

	ScriptErrorPrint scriptErrorPrint( "getFromObject" );
	ScriptSequence sequence = ScriptSequence::create( object );

	if (sequence.exists() && sequence.size() == 3)
	{
		float x;
		float y;
		float z;

		ScriptObject xObj = sequence.getItem( 0, scriptErrorPrint );

		if (!xObj.exists() || !xObj.convertTo( x, scriptErrorPrint ))
		{
			return false;
		}

		ScriptObject yObj = sequence.getItem( 1, scriptErrorPrint );

		if (!yObj.exists() || !yObj.convertTo( y, scriptErrorPrint ))
		{
			return false;
		}

		ScriptObject zObj = sequence.getItem( 2, scriptErrorPrint );

		if (!zObj.exists() || !zObj.convertTo( z, scriptErrorPrint ))
		{
			return false;
		}

		value.set( x, y, z );
	}
	else
	{
		return object.convertTo( value, scriptErrorPrint );
	}

	return true;
}


/**
 *	This function populates the given reference from the given object reference.
 *
 *	@param	object	A ScriptObject reference to read from. Must exist.
 *	@param	value	A value reference to populate from the ScriptObject.
 *
 *	@return			true unless the object doesn't, or the value could not be
 *					coerced from the ScriptObject.
 */
inline static bool getFromObject( const ScriptObject & object, Vector4 & value )
{
	// Needed because VectorDataType accepts any ScriptSequence of the right
	// length, but ScriptObject::convertTo (via Script::setData) only accepts
	// ScriptTuples of the right length.

	if (!object.exists())
	{
		ERROR_MSG( "getFromObject<>: Tried to get value from unset object\n" );
		return false;
	}

	ScriptErrorPrint scriptErrorPrint( "getFromObject" );
	ScriptSequence sequence = ScriptSequence::create( object );

	if (sequence.exists() && sequence.size() == 4)
	{
		float x;
		float y;
		float z;
		float w;

		ScriptObject xObj = sequence.getItem( 0, scriptErrorPrint );

		if (!xObj.exists() || !xObj.convertTo( x, scriptErrorPrint ))
		{
			return false;
		}

		ScriptObject yObj = sequence.getItem( 1, scriptErrorPrint );

		if (!yObj.exists() || !yObj.convertTo( y, scriptErrorPrint ))
		{
			return false;
		}

		ScriptObject zObj = sequence.getItem( 2, scriptErrorPrint );

		if (!zObj.exists() || !zObj.convertTo( z, scriptErrorPrint ))
		{
			return false;
		}

		ScriptObject wObj = sequence.getItem( 3, scriptErrorPrint );

		if (!wObj.exists() || !wObj.convertTo( w, scriptErrorPrint ))
		{
			return false;
		}

		value.set( x, y, z, w );
	}
	else
	{
		return object.convertTo( value, scriptErrorPrint );
	}

	return true;
}


#if defined( MF_SERVER )
/**
 *	This function populates the given reference from the given object reference.
 *
 *	@param	object	A ScriptObject reference to read from. Must exist.
 *	@param	value	A value reference to populate from the ScriptObject.
 *
 *	@return			true unless the object doesn't, or the value could not be
 *					coerced from the ScriptObject.
 */
inline static bool getFromObject( const ScriptObject & object,
	EntityMailBoxRef & value )
{
	return PyEntityMailBox::reduceToRef( object.get(), &value );
}
#endif // defined( MF_SERVER )


#if !defined( EDITOR_ENABLED )
/**
 *	This function populates the given reference from the given object reference.
 *
 *	@param	object	A ScriptObject reference to read from. Must exist.
 *	@param	value	A value reference to populate from the ScriptObject.
 *
 *	@return			true unless the object doesn't, or the value could not be
 *					coerced from the ScriptObject.
 */
inline static bool getFromObject( const ScriptObject & object,
	BW::UniqueID & value )
{
#if defined( SCRIPT_PYTHON ) && !defined( BWENTITY_DLL )
	// SCRIPT_PYTHON requires a UserDataObject
	if (!UserDataObject::Check( object.get() ))
	{
		return false;
	}

	UserDataObject * pUDO = static_cast< UserDataObject *>( object.get() );
	value = pUDO->guid();
#else // defined( SCRIPT_PYTHON ) && !defined( BWENTITY_DLL )
	// Otherwise, we will accept any string containing a GUID.
	BW::string guidString;

	if (!getFromObject( object, guidString ) ||
		!UniqueID::isUniqueID( guidString ))
	{
		return false;
	}

	value = UniqueID( guidString );
#endif // defined( SCRIPT_PYTHON ) && !defined( BWENTITY_DLL )
	return true;
}
#endif // !defined( EDITOR_ENABLED )


#if defined( SCRIPT_PYTHON )
/**
 *	This function converts the given object into the given stream using the
 *	custom class of the given FixedDictDataType.
 *	@param	object				The object to read from
 *	@param	type				The FixedDictDataType with the custom class
 *	@param	stream				The stream to write the output to
 *	@param	isPersistentOnly	Whether the data stream is to contain persistent
 *								data only.
 */
inline static bool getStreamForObject( const ScriptObject object,
	const FixedDictDataType & type, BinaryOStream & stream,
	bool isPersistentOnly )
{
	MF_ASSERT( type.hasCustomClass() );

	if (!isPersistentOnly && type.hasCustomAddToStream())
	{
		// XXX: No way to tell if it succeeded.
		type.addToStreamCustom( object, stream, isPersistentOnly );
	}
	else
	{
		PyFixedDictDataInstancePtr instance(
			type.createInstanceFromCustomClass( object ) );

		if (!instance.exists())
		{
			ERROR_MSG( "getStreamForDictionary: FixedDictDataType::"
					"createInstanceFromCustomClass failed\n" );
			return false;
		}

		// XXX: No way to tell if it succeeded.
		type.addInstanceToStream( instance.get(), stream, isPersistentOnly );
	}

	return true;
}


/**
 *	This function converts the given object into the given stream using the
 *	custom class of the given UserDataType.
 *	@param	object				The object to read from
 *	@param	type				The UserDataType with the custom class
 *	@param	stream				The stream to write the output to
 *	@param	isPersistentOnly	Whether the data stream is to contain persistent
 *								data only.
 */
inline static bool getStreamForObject( const ScriptObject object,
	const UserDataType & type, BinaryOStream & stream,
	bool isPersistentOnly )
{
	return type.addObjectToStream( object, stream, isPersistentOnly );
}


/**
 *	This function converts the given object into the given stream using the
 *	custom class of the given FixedDictDataType.
 *	@param	object				The object to read from
 *	@param	type				The FixedDictDataType with the custom class
 *	@param	section				The DataSection to write the output to
 */
inline static bool getSectionForObject( const ScriptObject object,
	const FixedDictDataType & type, DataSection & section )
{
	MF_ASSERT( type.hasCustomClass() );

	PyFixedDictDataInstancePtr instance(
		type.createInstanceFromCustomClass( object ) );

	if (!instance.exists())
	{
		ERROR_MSG( "getStreamForDictionary: FixedDictDataType::"
				"createInstanceFromCustomClass failed\n" );
		return false;
	}

	return type.addInstanceToSection( instance.get(), &section );
}


/**
 *	This function converts the given object into the given stream using the
 *	custom class of the given UserDataType.
 *	@param	object				The object to read from
 *	@param	type				The UserDataType with the custom class
 *	@param	section				The DataSection to write the output to
 */
inline static bool getSectionForObject( const ScriptObject object,
	const UserDataType & type, DataSection & section )
{
	return type.addObjectToSection( object, DataSectionPtr( &section ) );
}
#endif // defined( SCRIPT_PYTHON )


} // Anonymous namespace for static functions

// -----------------------------------------------------------------------------
// Section: ScriptDataSource
// -----------------------------------------------------------------------------
/**
 *	Constructor
 */
ScriptDataSource::ScriptDataSource( const ScriptObject & object ) :
	DataSource(),
	root_( object ),
	parentHolder_(),
	parents_( parentHolder_ ),
	readingType_( None )
{
	MF_ASSERT( root_.exists() );
	MF_ASSERT( !this->hasParent() );
	MF_ASSERT( this->currentObject() == root_ );
}


/**
 *	This method reads a float from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( float & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a double from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( double & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a uint8 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( uint8 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a uint16 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( uint16 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a uint32 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( uint32 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a uint64 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( uint64 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a int8 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( int8 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a int16 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( int16 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a int32 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( int32 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a int64 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( int64 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a BW::string from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( BW::string & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a BW::wstring from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( BW::wstring & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a BW::Vector2 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( BW::Vector2 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a BW::Vector3 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( BW::Vector3 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method reads a BW::Vector4 from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( BW::Vector4 & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


#if defined( MF_SERVER )
/**
 *	This method reads an EntityMailBoxRef from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( EntityMailBoxRef & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}
#endif // defined( MF_SERVER )


#if !defined( EDITOR_ENABLED )
/**
 *	This method reads a BW::UniqueID from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( BW::UniqueID & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}
#endif // !defined( EDITOR_ENABLED )


/**
 *	This method tests if the current node is None.
 *	@param	isNone	This reference is set to true if the current node is None,
 *					and set to false otherwise.
 *	@return			true unless we cannot tell if we have None.
 */
bool ScriptDataSource::readNone( bool & isNone )
{
	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::readNone: Tried to get value from unset object\n" );
		return false;
	}
	isNone = this->currentObject().isNone();
	return true;
}


/**
 *	This method reads a blob of data from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::readBlob( BW::string & value )
{
	// If we can stringify it, that's good enough to be a Blob
	return this->read( value );
}


/**
 *	This method returns the count of elements in the sequence in the
 *	current node.
 *	@param	count	The reference to populate with the number of elements.
 *	@return			true unless the current node is not a sequence.
 */
bool ScriptDataSource::beginSequence( size_t & count )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::beginSequence: Tried to read a sequence "
				"when already reading a collection.\n" );
		return false;
	}

	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginSequence: Tried to read a non-existent "
				"object.\n" );
		return false;
	}

	ScriptSequence sequence = ScriptSequence::create( this->currentObject() );
	if (!sequence.exists())
	{
		ERROR_MSG( "ScriptDataSource::beginSequence: Tried to read a count "
				"from a non-sequence.\n" );
		return false;
	}

	count = sequence.size();
	readingType_ = Sequence;

	return true;
}


/**
 *	This method makes the current node the item'th element of the current
 *	node, if it is a sequence.
 *	@param	item	The item index to make the current node
 *	@return			true unless the current node is not a sequence
 */
bool ScriptDataSource::enterItem( size_t item )
{
	const ScriptObject leaf = this->currentObject();
	ScriptSequence sequence = ScriptSequence::create( leaf );
	
	if (!sequence || readingType_ != Sequence)
	{
		ERROR_MSG( "ScriptDataSource::enterItem: Tried to enter an item of a "
				"non-sequence.\n" );
		return false;
	}

	ScriptObject child = sequence.getItem( item, ScriptErrorPrint() );
	
	if (!child.exists())
	{
		return false;
	}

	parents_.push_back( std::make_pair( child, readingType_ ) );

	MF_ASSERT( child == this->currentObject() );
	MF_ASSERT( leaf == this->parentObject() );
	MF_ASSERT( readingType_ == this->parentType() );
	MF_ASSERT( this->hasParent() );

	readingType_ = None;

	return true;
}


/**
 *	This method makes the current node the container that holds the current
 *	node.
 *	@return	true unless we're not in a container.
 */
bool ScriptDataSource::leaveItem()
{
	if (!this->hasParent())
	{
		ERROR_MSG( "ScriptDataSource::leaveItem: Tried to leave an item of a "
				"non-sequence.\n" );
		return false;
	}

	ScriptObject parent = this->parentObject();
	readingType_ = this->parentType();

	parents_.pop_back();

	MF_ASSERT( this->currentObject() == parent );

	return true;
}


/**
 *	This method prepares us to read an attributed structure
 */
bool ScriptDataSource::beginClass()
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::beginClass: Tried to read a class "
				"when already reading a collection.\n" );
		return false;
	}

	// All extant ScriptObjects have attributes.
	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginClass: Tried to read a non-existent "
				"object.\n" );
		return false;
	}
#if defined( SCRIPT_PYTHON )
	// If we're reading a class, and it's not a PyClassDataInstance, then if it
	// happens to be a mapping, we read the fields instead of the attributes.
	if (!PyClassDataInstance::Check( this->currentObject().get() ) &&
		ScriptMapping::check( this->currentObject() ))
	{
		readingType_ = Dictionary;
	}
	else
#endif // defined( SCRIPT_PYTHON )
	{
		readingType_ = Class;
	}

	return true;
}


/**
 *	This method prepares us to read a mapping structure
 */
bool ScriptDataSource::beginDictionary( const FixedDictDataType * pType )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::beginDictionary: Tried to read a dictionary "
				"when already reading a collection.\n" );
		return false;
	}

	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginDictionary: Tried to read a non-existent "
				"object.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	if (pType != NULL)
	{
		// Why are these methods not const?
		FixedDictDataType & type =
			*const_cast< FixedDictDataType * >( pType );

		type.initCustomClassImplOnDemand();

		if (type.hasCustomClass())
		{
			// Replace currentObject() (The custom class) with a
			// PyFixedDictDataInstance representation of its data to allow reading
			// field-by-field.
			ScriptObject customClass = this->currentObject();
			this->currentObject() =
				type.createInstanceFromCustomClass( customClass );
			if (!this->currentObject().exists())
			{
				ERROR_MSG( "ScriptDataSource::beginDictionary: Failed to "
						"convert a custom-FIXED_DICT into a "
						"PyFixedDictDataInstance.\n" );
				return false;
			}
		}
	}
#endif // defined( SCRIPT_PYTHON )

	if (!ScriptMapping::check( this->currentObject() ))
	{
		ERROR_MSG( "ScriptDataSource::beginDictionary: Tried to read a "
				"non-mapping.\n" );
		return false;
	}

	readingType_ = Dictionary;

	return true;
}


/**
 *	This method makes the current node an element of the current node, if it is
 *	a map or object.
 *	@param	name	The element name
 *	@return			true unless the current node is not a map or object
 */
bool ScriptDataSource::enterField( const BW::string & name )
{
	if (readingType_ != Class && readingType_ != Dictionary)
	{
		ERROR_MSG( "ScriptDataSource::enterField: Tried to enter a field when "
				"not reading a map or object.\n" );
		return false;
	}

	const ScriptObject leaf = this->currentObject();

	if (!leaf.exists())
	{
		ERROR_MSG( "ScriptDataSource::enterField: Tried to enter a field of a "
				"non-existent object.\n" );
		return false;
	}

	ScriptObject child;

	switch (readingType_)
	{
		case Class:
		{
			child = leaf.getAttribute( name.c_str(), ScriptErrorPrint() );
			break;
		}
		case Dictionary:
		{
			ScriptMapping mapping = ScriptMapping::create( leaf );
			if (!mapping.exists())
			{
				ERROR_MSG( "ScriptDataSource::enterField: Tried to enter a "
						"field of a non-mapping.\n" );
				return false;
			}
			child = mapping.getItem( name.c_str(), ScriptErrorPrint() );
			break;
		}
		default:
			MF_ASSERT( false );
	}

	if (!child.exists())
	{
		return false;
	}

	parents_.push_back( std::make_pair( child, readingType_ ) );

	MF_ASSERT( child == this->currentObject() );
	MF_ASSERT( leaf == this->parentObject() );
	MF_ASSERT( readingType_ == this->parentType() );
	MF_ASSERT( this->hasParent() );

	readingType_ = None;

	return true;
}


/**
 *	This method makes the current node the container that holds the current
 *	node.
 *	@return	true unless we're not in a container.
 */
bool ScriptDataSource::leaveField()
{
	if (!this->hasParent())
	{
		ERROR_MSG( "ScriptDataSource::leaveField: Tried to leave an item of a "
				"non-sequence.\n" );
		return false;
	}

	ScriptObject parent = this->parentObject();
	readingType_ = this->parentType();

	parents_.pop_back();

	MF_ASSERT( this->currentObject() == parent );

	return true;
}


/**
 *	This method reads a FIXED_DICT with custom class into a stream.
 */
bool ScriptDataSource::readCustomType( const FixedDictDataType & type,
	BinaryOStream & stream, bool isPersistentOnly )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::readCustomType: Tried to read a "
				"FIXED_DICT into a stream when already reading a "
				"collection.\n" );
		return false;
	}

	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginDictionaryStream: Tried to read a "
				"non-existent object.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	const_cast< FixedDictDataType & >( type ).initCustomClassImplOnDemand();

	if (type.hasCustomClass())
	{
		return getStreamForObject( this->currentObject(), type, stream,
			isPersistentOnly );
	}
	ERROR_MSG( "ScriptDataSink::readCustomType: Tried to read a "
			"FIXED_DICT into a stream without a custom implementation.\n" );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::readCustomType: Tried to read a "
			"FIXED_DICT into a stream without Python support.\n" );
#endif // defined( SCRIPT_PYTHON )

	return false;
}


/**
 *	This method reads a FIXED_DICT with custom class into a DataSection.
 */
bool ScriptDataSource::readCustomType( const FixedDictDataType & type,
	DataSection & section )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::readCustomType: Tried to read a "
				"FIXED_DICT into a DataSection when already reading a "
				"collection.\n" );
		return false;
	}

	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginDictionaryStream: Tried to read a "
				"non-existent object.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	const_cast< FixedDictDataType & >( type ).initCustomClassImplOnDemand();

	if (type.hasCustomClass())
	{
		return getSectionForObject( this->currentObject(), type, section );
	}
	ERROR_MSG( "ScriptDataSink::readCustomType: Tried to read a "
			"FIXED_DICT into a DataSection without a custom "
			"implementation.\n" );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::readCustomType: Tried to read a "
			"FIXED_DICT into a DataSection without Python support.\n" );
#endif // defined( SCRIPT_PYTHON )

	return false;
}


/**
 *	This method reads a USER_TYPE with custom class into a stream.
 */
bool ScriptDataSource::readCustomType( const UserDataType & type,
	BinaryOStream & stream, bool isPersistentOnly )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::readCustomType: Tried to read a "
				"USER_TYPE into a stream when already reading a "
				"collection.\n" );
		return false;
	}

	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginDictionaryStream: Tried to read a "
				"non-existent object.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	return getStreamForObject( this->currentObject(), type, stream,
			isPersistentOnly );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::readCustomType: Tried to read a "
			"USER_TYPE into a DataSection without Python support.\n" );
	return false;
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	This method reads a USER_TYPE with custom class into a DataSection.
 */
bool ScriptDataSource::readCustomType( const UserDataType & type,
	DataSection & section )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::readCustomType: Tried to read a "
				"USER_TYPE into a DataSection when already reading a "
				"collection.\n" );
		return false;
	}

	if (!this->currentObject().exists())
	{
		ERROR_MSG( "ScriptDataSource::beginDictionaryStream: Tried to read a "
				"non-existent object.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	return getSectionForObject( this->currentObject(), type, section );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::readCustomType: Tried to read a "
			"USER_TYPE into a DataSection without Python support.\n" );
	return false;
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	This method reads a ScriptObject from the current node
 *	@param	value	The value to populate from the ScriptObject
 *	@return			true unless the value could not be converted
 */
bool ScriptDataSource::read( ScriptObject & value )
{
	if (readingType_ != None)
	{
		ERROR_MSG( "ScriptDataSource::read: Tried to read a value "
				"when already reading a collection.\n" );
		return false;
	}

	return getFromObject( this->currentObject(), value );
}


/**
 *	This method returns a reference to the current node
 */
ScriptObject & ScriptDataSource::currentObject()
{
	if (parents_.empty())
	{
		return root_;
	}

	return parents_.back().first;
}


/**
 *	This method returns a reference to the parent collection of the current
 *	node
 */
const ScriptObject & ScriptDataSource::parentObject() const
{
	MF_ASSERT( this->hasParent() );
	
	size_t parentSize = parents_.size();

	if (parentSize == 1)
	{
		return root_;
	}

	return parents_[ parentSize - 2 ].first;
}


/**
 *	This method returns a reference to the parent CollectionType of the current
 *	node
 */
ScriptDataSource::CollectionType ScriptDataSource::parentType() const
{
	if (!this->hasParent())
	{
		return None;
	}
	
	size_t parentSize = parents_.size();

	return parents_[ parentSize - 1 ].second;
}


/**
 *	This method indicates if a parent collection exists for the current node.
 */
bool ScriptDataSource::hasParent() const
{
	return !parents_.empty();
}

ScriptDataSource::VectorOfNodesHolder::VectorOfNodesHolder() :
	data_( new VectorOfNodes() )
{
}

ScriptDataSource::VectorOfNodesHolder::~VectorOfNodesHolder()
{
	delete data_;
}

ScriptDataSource::VectorOfNodesHolder::operator ScriptDataSource::VectorOfNodes & ()
{
	return *data_;
}

} // namespace BW

// script_data_source.cpp
