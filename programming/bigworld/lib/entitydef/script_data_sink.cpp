#include "pch.hpp"

#include "script_data_sink.hpp"

#if !defined( EDITOR_ENABLED )
#if defined( SCRIPT_PYTHON )
#include "chunk/user_data_object.hpp"
#endif // defined( SCRIPT_PYTHON )
#endif // !defined( EDITOR_ENABLED )

#include "cstdmf/debug.hpp"
#if !defined( EDITOR_ENABLED )
#include "cstdmf/unique_id.hpp"
#endif // !defined( EDITOR_ENABLED )

#if defined( MF_SERVER )
#include "entitydef/mailbox_base.hpp"
#endif // defined( MF_SERVER )

#include "entitydef/data_types/array_data_type.hpp"
#include "entitydef/data_types/class_data_type.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#include "entitydef/data_types/tuple_data_type.hpp"
#if defined( SCRIPT_PYTHON )
#include "entitydef/data_types/user_data_type.hpp"
#endif // defined( SCRIPT_PYTHON )

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/array_data_instance.hpp"
#include "entitydef/data_instances/class_data_instance.hpp"
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#endif // defined( SCRIPT_PYTHON )

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

#include "network/basictypes.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


namespace BW
{

namespace // Anonymous namespace for static functions
{

/**
 *	This function populates the given object reference from the given reference.
 *
 *	@param	object	A ScriptObject reference to populate. Must not exist.
 *	@param	value	A value to write into the ScriptObject.
 *
 *	@return			true unless the object already existed, or the value could not be
 *					coerced into a ScriptObject.
 */
template< typename inputT >
inline static bool setObject( ScriptObject & object, const inputT & value )
{
	if (object.exists())
	{
		ERROR_MSG( "setObject<>: Tried to set value for already-set object\n" );
		return false;
	}

	object = ScriptObject::createFrom( value );

	if (!object.exists())
	{
		ERROR_MSG( "setObject<>: Failed to set value\n" );
		return false;
	}

	return true;
}


#if defined( MF_SERVER )
/**
 *	This function populates the given object reference from the given reference.
 *
 *	@param	object	A ScriptObject reference to populate. Must not exist.
 *	@param	value	A value to write into the ScriptObject.
 *
 *	@return			true unless the object already existed, or the value could not be
 *					coerced into a ScriptObject.
 */
inline static bool setObject( ScriptObject & object,
	const EntityMailBoxRef & value )
{
	ScriptObject mailbox( PyEntityMailBox::constructFromRef( value ),
					ScriptObject::FROM_NEW_REFERENCE );
	if (!mailbox.exists())
	{
		return false;
	}

	object = mailbox;

	return true;
}
#endif // defined( MF_SERVER )


#if !defined( EDITOR_ENABLED )
/**
 *	This function populates the given object reference from the given reference.
 *
 *	@param	object	A ScriptObject reference to populate. Must not exist.
 *	@param	value	A value to write into the ScriptObject.
 *
 *	@return			true unless the object already existed, or the value could not be
 *					coerced into a ScriptObject.
 */
inline static bool setObject( ScriptObject & object,
	const UniqueID & value )
{
#if defined( SCRIPT_PYTHON ) && !defined( BWENTITY_DLL )
	// SCRIPT_PYTHON creates a UserDataObject or None.
	if (value == UniqueID::zero())
	{
		// This shouldn't ever happen, UserDataObjectLinkDataType::createFromStream
		// and UserDataObjectLinkDataType::createFromSection already handle this.
		object = ScriptObject::none();
	}
	else
	{
		ScriptObject udo( UserDataObject::createRef( value ),
			ScriptObject::FROM_NEW_REFERENCE );
		if (!udo.exists())
		{
			return false;
		}
		object = udo;
	}
#else // defined( SCRIPT_PYTHON ) && !defined( BWENTITY_DLL )
	// Otherwise, we get a ScriptBlob containing the stringified GUID,
	// even if None.
	ScriptBlob udo = ScriptBlob::create( value.toString() );
	if (!udo.exists())
	{
		return false;
	}
	object = udo;
#endif // defined( SCRIPT_PYTHON ) && !defined( BWENTITY_DLL )
	return true;
}
#endif // !defined( EDITOR_ENABLED )


#if defined( SCRIPT_PYTHON )
/**
 *	This function populates the given object using the custom class of the
 *	given FixedDictDataType with the given stream.
 *	@param	object				The object to populate
 *	@param	type				The FixedDictDataType with the custom class
 *	@param	stream				The stream to read
 *	@param	isPersistentOnly	Whether the data stream only contains
 *								persistent data.
 */
inline static bool setObjectFromStream( ScriptObject & object,
	const FixedDictDataType & type, BinaryIStream & stream,
	bool isPersistentOnly )
{
	MF_ASSERT( type.hasCustomClass() );

	if (!isPersistentOnly && type.hasCustomCreateFromStream())
	{
		object = type.createFromStreamCustom( stream, isPersistentOnly );
	}
	else
	{
		PyFixedDictDataInstancePtr instance(
			type.createInstanceFromStream( stream, isPersistentOnly ) );

		if (!instance.exists())
		{
			ERROR_MSG( "setObjectFromStream: "
					"FixedDictDataType::createInstanceFromStream failed\n" );
			return false;
		}

		object = type.createCustomClassFromInstance( instance.get() );
	}

	return object.exists();
}


/**
 *	This function populates the given object using the custom class of the
 *	given UserDataType with the given stream.
 *	@param	object				The object to populate
 *	@param	type				The UserDataType with the custom class
 *	@param	stream				The stream to read
 *	@param	isPersistentOnly	Whether the data stream only contains
 *								persistent data.
 */
inline static bool setObjectFromStream( ScriptObject & object,
	const UserDataType & type, BinaryIStream & stream,
	bool isPersistentOnly )
{
	return type.createObjectFromStream( object, stream, isPersistentOnly );
}



/**
 *	This function populates the given object using the custom class of the
 *	given FixedDictDataType with the given DataSection.
 *	@param	object				The object to populate
 *	@param	type				The FixedDictDataType with the custom class
 *	@param	section				The DataSection to read from
 */
inline static bool setObjectFromSection( ScriptObject & object,
	const FixedDictDataType & type, DataSection & section )
{
	MF_ASSERT( type.hasCustomClass() );

	PyFixedDictDataInstancePtr instance(
		type.createInstanceFromSection( &section ) );

	if (!instance.exists())
	{
		ERROR_MSG( "setObjectFromSection: "
				"FixedDictDataType::createInstanceFromSection failed\n" );
		return false;
	}

	object = type.createCustomClassFromInstance( instance.get() );

	return object.exists();
}


/**
 *	This function populates the given object using the custom class of the
 *	given UserDataType with the given DataSection.
 *	@param	object				The object to populate
 *	@param	type				The UserDataType with the custom class
 *	@param	section				The DataSection to read from
 */
inline static bool setObjectFromSection( ScriptObject & object,
	const UserDataType & type, DataSection & section )
{
	return type.createObjectFromSection( object, DataSectionPtr( &section ) );
}
#endif // defined( SCRIPT_PYTHON )


} // Anonymous namespace for static functions

// -----------------------------------------------------------------------------
// Section: ScriptDataSink
// -----------------------------------------------------------------------------
/**
 *	Constructor
 */
ScriptDataSink::ScriptDataSink() :
	DataSink(),
	root_(),
	parentHolder_(),
	parents_( parentHolder_ ),
	buildingType_( None )
{
	MF_ASSERT( !root_.exists() );
	MF_ASSERT( !this->currentObject().exists() );
	MF_ASSERT( this->parentType() == None );
}


/**
 *	This method completes creation of the ScriptObject and returns it.
 *	@return		The ScriptObject we created.
 */
ScriptObject ScriptDataSink::finalise()
{
	if (buildingType_ == Custom)
	{
		// TODO: Clean up if our top level object is a container, as per
		// leaveItem
		return ScriptObject();
	}
	return root_;
}


/**
 *	This method writes a float into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const float & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a double into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const double & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a uint8 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const uint8 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a uint16 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const uint16 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a uint32 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const uint32 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a uint64 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const uint64 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a int8 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const int8 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a int16 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const int16 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a int32 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const int32 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a int64 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const int64 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a BW::string into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const BW::string & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a BW::wstring into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const BW::wstring & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a BW::Vector2 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const BW::Vector2 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a BW::Vector3 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const BW::Vector3 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method writes a BW::Vector4 into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const BW::Vector4 & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


#if defined( MF_SERVER )
/**
 *	This method writes an EntityMailBoxRef into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const EntityMailBoxRef & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}
#endif // defined( MF_SERVER )


#if !defined( EDITOR_ENABLED )
/**
 *	This method writes a BW::UniqueID into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const BW::UniqueID & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}
#endif // !defined( EDITOR_ENABLED )


/**
 *	This method writes a None into the current empty node if isNone
 *	is true, and does nothing otherwise.
 *	@param	isNone	true if we are to write a None value
 *	@return			true unless the value could not be written
 */
bool ScriptDataSink::writeNone( bool isNone )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::writeNone: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	if (!isNone)
	{
		// We're going to get another value to hold, so if we
		// already have a value, something's wrong.
		return !this->currentObject().exists();
	}

	return setObject( this->currentObject(), ScriptObject::none() );
}


/**
 *	This method writes a blob of data into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::writeBlob( const BW::string & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::writeBlob: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), ScriptBlob::create( value ) );
}


/**
 *	This method creates a ScriptList or PyArrayDataInstance at the current
 *	empty node.
 *	@param	pType				The ArrayDataType being written
 *	@param	count				The number of elements to hold
 *	@return						true unless the list could not be written
 */
bool ScriptDataSink::beginArray( const ArrayDataType * pType, size_t count )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::beginArray: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	ScriptObject newArray;

	if (pType != NULL && pType->getSize() != 0 &&
		(size_t)pType->getSize() != count)
	{
		ERROR_MSG( "ScriptDataSink::beginArray: Bad element count.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	if (pType != NULL)
	{
		newArray = ScriptObject( new PyArrayDataInstance( pType, count ),
			ScriptObject::FROM_NEW_REFERENCE );
	}
	else
#endif // defined( SCRIPT_PYTHON )
	{
		newArray = ScriptList::create( count );
	}

	if (!newArray.exists())
	{
		return false;
	}

	buildingType_ = Array;

	ScriptObject & leaf = this->currentObject();

	return setObject( leaf, newArray );
}


/**
 *	This method creates a ScriptTuple at the current empty node.
 *	@param	pType				The TupleDataType being written
 *	@param	count				The number of elements to hold
 *	@return						true unless the tuple could not be written
 */
bool ScriptDataSink::beginTuple( const TupleDataType * pType, size_t count )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::beginTuple: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	if (pType != NULL && pType->getSize() != 0 &&
		(size_t)pType->getSize() != count)
	{
		ERROR_MSG( "ScriptDataSink::beginTuple: Bad element count.\n" );
		return false;
	}

	ScriptTuple newTuple = ScriptTuple::create( count );

	IF_NOT_MF_ASSERT_DEV( newTuple.exists() )
	{
		return false;
	}

#if defined( SCRIPT_PYTHON )
	if (pType != NULL)
	{
		for (size_t i = 0; i < count; i++)
		{
			ScriptDataSink defaultSink;
			MF_VERIFY( pType->getElemType().
					getDefaultValue( defaultSink ) );
			ScriptObject defaultValue = defaultSink.finalise();
			newTuple.setItem( i, defaultValue );
		}
	}
#endif // defined( SCRIPT_PYTHON )

	ScriptObject & leaf = this->currentObject();

	buildingType_ = Tuple;

	return setObject( leaf, newTuple );
}


/**
 *	This method makes an element of a sequence the current empty node.
 *	@param	item	The element index
 *	@return			true unless we are not writing a sequence
 */
bool ScriptDataSink::enterItem( size_t item )
{
	if (buildingType_ != Array && buildingType_ != Tuple)
	{
		ERROR_MSG( "ScriptDataSink::enterItem: Not currently building a "
				"sequence.\n" );
		return false;
	}

	parents_.push_back( Node( buildingType_ ) );
	this->parentItemIndex() = item;
	buildingType_ = None;

	MF_ASSERT( !this->currentObject().exists() );
	MF_ASSERT( this->parentType() == Array || this->parentType() == Tuple );

	return true;
}


/**
 *	This method puts the current node into its place in the sequence we're
 *	currently writing.
 *	@return	true unless we aren't writing a sequence or the node is empty.
 */
bool ScriptDataSink::leaveItem()
{
	if (this->parentType() != Array &&
		this->parentType() != Tuple)
	{
		ERROR_MSG( "ScriptDataSink::leaveItem: Not currently writing a "
				"sequence.\n" );
		return false;
	}

	ScriptObject & parent = this->parentObject();
	ScriptObject & leaf = this->currentObject();
	size_t item = this->parentItemIndex();
	buildingType_ = this->parentType();

	MF_ASSERT( buildingType_ == Array || buildingType_ == Tuple );

	if (!leaf.exists())
	{
		ERROR_MSG( "ScriptDataSink::leaveItem: Didn't write a sequence "
				"element.\n" );
		return false;
	}

	switch (this->parentType())
	{
		case Array:
		{
#if defined( SCRIPT_PYTHON )
			if (PyArrayDataInstance::Check( parent.get() ))
			{
				PyArrayDataInstance * pParent =
					static_cast< PyArrayDataInstance * >( parent.get() );
				pParent->setInitialItem( (int)item, leaf );
			}
			else
#endif // defined( SCRIPT_PYTHON )
			if (parent.exists() && ScriptList::check( parent ))
			{
				ScriptList list( parent );
				if (!list.setItem( item, leaf ))
				{
					ERROR_MSG( "ScriptDataSink::leaveItem: Failed to store "
							"new value.\n" );
					return false;
				}
			}
			else
			{
				ERROR_MSG( "ScriptDataSink::leaveItem: Parent is not a "
					"sequence.\n" );
				return false;
			}
			break;
		}
		case Tuple:
		{
			ScriptTuple tuple( parent );
			if (!tuple.setItem( item, leaf ))
			{
				ERROR_MSG( "ScriptDataSink::leaveItem: Failed to store new "
						"value.\n" );
				return false;
			}
			break;
		}
		default:
			MF_ASSERT( false );
	}

	// Invalidates all our references except parent
	parents_.pop_back();

	MF_ASSERT( this->currentObject() == parent );

	return true;
}


/**
 *	This method creates a PyClassDataInstance at the current empty node.
 *	@param	pType				The TupleDataType being written
 *	@return						true unless the tuple could not be written
 */
bool ScriptDataSink::beginClass( const ClassDataType * pType )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::beginClass: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

#if defined( SCRIPT_PYTHON )
	if (pType == NULL)
	{
		// There's no type built-in to Python that we could inherit from,
		// so there's no clear way to react usefully to this.
		// We'd need the equivalent of `type( "Typeless", (object,), {} )()` to
		// create the instance.
		// NULL pType is not defined of the Source/Sink API anyway, so this
		// isn't a problem yet.
		ERROR_MSG( "ScriptDataSink::beginClass: Tried to write a value but"
				"did not provide a ClassDataType for it.\n" );
		return false;
	}

	ScriptObject newClass = ScriptObject( new PyClassDataInstance( pType ),
		ScriptObject::FROM_NEW_REFERENCE );

	if (!newClass.exists())
	{
		return false;
	}

	ScriptObject & leaf = this->currentObject();

	buildingType_ = Class;

	return setObject( leaf, newClass );
#else // defined( SCRIPT_PYTHON )
	// ClassDataType is not supported without SCRIPT_PYTHON
	return false;
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	This method creates a ScriptDict or PyFixedDictDataInstance at the current
 *	empty node.
 *	@param	pType				The TupleDataType being written
 *	@return						true unless the tuple could not be written
 */
bool ScriptDataSink::beginDictionary( const FixedDictDataType * pType )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::beginDictionary: Tried to write a value "
				"when already writing a structure.\n" );
		return false;
	}

	ScriptObject newDict;

#if defined( SCRIPT_PYTHON )
	if (pType != NULL)
	{
		const_cast< FixedDictDataType * >( pType )->initCustomClassImplOnDemand();

		if (pType->hasCustomClass())
		{
			buildingType_ = Custom;
			// TODO: Is this ever needed?
			ERROR_MSG( "Not yet supporting dict-based custom FixedDict writing\n" );
			return false;
		}
		else
		{
			buildingType_ = Dictionary;
		}

		PyFixedDictDataInstancePtr pInst( new PyFixedDictDataInstance(
				const_cast< FixedDictDataType * >( pType ) ),
			ScriptObject::FROM_NEW_REFERENCE );
		newDict = pInst;

		for (FixedDictDataType::Fields::size_type i = 0;
			i < pType->getNumFields();
			++i)
		{
			// TODO: Error handling
			ScriptDataSink defaultSink;
			MF_VERIFY( pType->getFieldDataType( static_cast< int >( i ) ).
				getDefaultValue( defaultSink ) );
			ScriptObject defaultValue = defaultSink.finalise();
			pInst->initFieldValue( static_cast< int >( i ), defaultValue );
		}
	}
	else
#endif // defined( SCRIPT_PYTHON )
	{
		buildingType_ = Dictionary;
		newDict = ScriptDict::create();
	}

	IF_NOT_MF_ASSERT_DEV( newDict.exists() )
	{
		return false;
	}


	ScriptObject & leaf = this->currentObject();

	return setObject( leaf, newDict );
}


/**
 *	This method makes an element of a class or dictionary the current empty
 *	node.
 *	@param	name	The element name
 *	@return			true unless we are not writing a class or dictionary
 */
bool ScriptDataSink::enterField( const BW::string & name )
{
	if (buildingType_ != Class && buildingType_ != Dictionary &&
		buildingType_ != Custom)
	{
		ERROR_MSG( "ScriptDataSink::enterField: Not currently building a "
				"class or dictionary.\n" );
		return false;
	}

	parents_.push_back( Node( buildingType_ ) );
	this->parentFieldName() = name;
	buildingType_ = None;

	MF_ASSERT( !this->currentObject().exists() );
	MF_ASSERT( this->parentType() == Class ||
		this->parentType() == Dictionary ||
		this->parentType() == Custom );

	return true;
}


/**
 *	This method puts the current node into its place in the class or dictionary
 *	we're currently writing.
 *	@return	true unless we aren't writing a class or dictionary, or the node is
 *	empty.
 */
bool ScriptDataSink::leaveField()
{
	if (this->parentType() != Class &&
		this->parentType() != Dictionary &&
		this->parentType() != Custom)
	{
		ERROR_MSG( "ScriptDataSink::leaveField: Not currently writing a "
				"field.\n" );
		return false;
	}

	// buildingType_ here is the buildingType_ of currentObject()
	if (buildingType_ == Custom)
	{
		return false;
		/*
		// We need to convert the DataInstance we just created into the
		// custom class
		// TODO: We need pType... If it was a UserDataType, then we don't
		// get to keep pType in the DataInstance.
		// TODO: We need to load this instance into the custom class:
		// FIXED_DICT pType->createCustomClassFromInstance( instance.get() )
		// in leaveItem and finalise.
		if (!finaliseCustomType( this->currentObject() ))
		{
			ERROR_MSG( "ScriptDataSink::leaveField: Error finalising a custom "
					"type.\n" );
			return false;
		}
		*/
	}

	ScriptObject & parent = this->parentObject();
	ScriptObject & leaf = this->currentObject();
	BW::string field = this->parentFieldName();
	buildingType_ = this->parentType();

	MF_ASSERT( buildingType_ == Class || buildingType_ == Dictionary ||
		buildingType_ == Custom );

	if (!leaf.exists())
	{
		ERROR_MSG( "ScriptDataSink::leaveField: Didn't write a field "
				"value.\n" );
		return false;
	}

	switch (this->parentType())
	{
		case Class:
		{
#if defined( SCRIPT_PYTHON )
			MF_ASSERT( PyClassDataInstance::Check( parent.get() ) );

			PyClassDataInstance * pInst =
				static_cast< PyClassDataInstance * >( parent.get() );
			int index = pInst->dataType()->fieldIndexFromName( field.c_str() );

			if (index == -1)
			{
				return false;
			}

			pInst->setFieldValue( index, leaf );
			break;
#else // defined( SCRIPT_PYTHON )
			// Class is not supported without SCRIPT_PYTHON
			return false;
#endif // defined( SCRIPT_PYTHON )
		}
		case Custom:
		case Dictionary:
		{
#if defined( SCRIPT_PYTHON )
			if (PyFixedDictDataInstancePtr::check( parent ) )
			{
				PyFixedDictDataInstancePtr pInst( parent );
				int index = pInst->dataType().getFieldIndex( field.c_str() );

				if (index == -1)
				{
					ERROR_MSG( "ScriptDataSink::leaveField: Failed to find "
							"index for key: '%s'.\n", field.c_str() );
					return false;
				}

				pInst->initFieldValue( index, leaf );
				break;
			}
			else
#endif // defined( SCRIPT_PYTHON )
			if (parent.exists() && ScriptDict::check( parent ))
			{
				ScriptDict dict( parent );
				if (!dict.setItem( field.c_str(), leaf, ScriptErrorPrint() ))
				{
					ERROR_MSG( "ScriptDataSink::leaveField: Failed to store "
							"new value.\n" );
					return false;
				}
				break;
			}
			else
			{
				ERROR_MSG( "ScriptDataSink::leaveField: Parent is not a "
					"dictionary.\n" );
				return false;
			}
		}
		default:
			MF_ASSERT( false );
	}

	// Invalidates all our references except parent
	parents_.pop_back();

	MF_ASSERT( this->currentObject() == parent );

	return true;
}


/**
 *	This method receives a FIXED_DICT in a stream, and turns it into an
 *	appropriate object at the current empty node.
 *
 *	@param	type				The FixedDictDataType being written.
 *	@param	stream				The stream to read from
 *	@param	isPersistentOnly	true if this stream contains only persistent
 *								data.
 *	@return						True unless the write fails (i.e. we were
 *								not expecting a dictionary in a stream or
 *								the stream could not be decoded)
 */
bool ScriptDataSink::writeCustomType( const FixedDictDataType & type,
	BinaryIStream & stream, bool isPersistentOnly )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
				"FIXED_DICT stream when already writing a structure.\n" );
		return false;
	}

	if (this->currentObject().exists())
	{
		return false;
	}

#if defined( SCRIPT_PYTHON )
	const_cast< FixedDictDataType & >( type ).initCustomClassImplOnDemand();

	if (type.hasCustomClass())
	{
		return setObjectFromStream( this->currentObject(), type, stream,
			isPersistentOnly );
	}
	ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
			"FIXED_DICT stream without a custom implementation.\n" );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
			"FIXED_DICT stream without Python support.\n" );
#endif // defined( SCRIPT_PYTHON )

	return false;
}


/**
 *	This method receives a FIXED_DICT in a DataSection, and turns it into an
 *	appropriate object at the current empty node.
 *
 *	@param	type				The FixedDictDataType being written.
 *	@param	section				The DataSection to read from
 *	@return						True unless the write fails (i.e. we were
 *								not expecting a dictionary in a stream or
 *								the stream could not be decoded)
 */
bool ScriptDataSink::writeCustomType( const FixedDictDataType & type,
	DataSection & section )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
				"FIXED_DICT DataSection when already writing a structure.\n" );
		return false;
	}

	if (this->currentObject().exists())
	{
		return false;
	}

#if defined( SCRIPT_PYTHON )
	const_cast< FixedDictDataType & >( type ).initCustomClassImplOnDemand();

	if (type.hasCustomClass())
	{
		return setObjectFromSection( this->currentObject(), type, section );
	}
	ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
			"FIXED_DICT DataSection without a custom implementation.\n" );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
			"FIXED_DICT DataSection without Python support.\n" );
#endif // defined( SCRIPT_PYTHON )

	return false;
}


/**
 *	This method receives a USER_TYPE in a stream, and turns it into an
 *	appropriate object at the current empty node.
 *
 *	@param	type				The UserDataType being written.
 *	@param	stream				The stream to read from
 *	@param	isPersistentOnly	true if this stream contains only persistent
 *								data.
 *	@return						True unless the write fails (i.e. we were
 *								not expecting a dictionary in a stream or
 *								the stream could not be decoded)
 */
bool ScriptDataSink::writeCustomType( const UserDataType & type,
	BinaryIStream & stream, bool isPersistentOnly )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
				"USER_TYPE stream when already writing a structure.\n" );
		return false;
	}

	if (this->currentObject().exists())
	{
		return false;
	}

#if defined( SCRIPT_PYTHON )
	return setObjectFromStream( this->currentObject(), type, stream,
		isPersistentOnly );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
			"USER_TYPE stream without Python support.\n" );
	return false;
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	This method receives a USER_TYPE in a DataSection, and turns it into an
 *	appropriate object at the current empty node.
 *
 *	@param	type				The UserDataType being written.
 *	@param	section				The DataSection to read from
 *	@return						True unless the write fails (i.e. we were
 *								not expecting a dictionary in a stream or
 *								the stream could not be decoded)
 */
bool ScriptDataSink::writeCustomType( const UserDataType & type,
	DataSection & section )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
				"USER_TYPE DataSection when already writing a structure.\n" );
		return false;
	}

	if (this->currentObject().exists())
	{
		return false;
	}

#if defined( SCRIPT_PYTHON )
	return setObjectFromSection( this->currentObject(), type, section );
#else // defined( SCRIPT_PYTHON )
	ERROR_MSG( "ScriptDataSink::writeCustomType: Tried to write a "
			"USER_TYPE DataSection without Python support.\n" );
	return false;
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	This method writes a ScriptObject into the current empty node
 *	@param	value	The value to write to the ScriptObject
 *	@return			true unless the value could not be writen
 */
bool ScriptDataSink::write( const ScriptObject & value )
{
	if (buildingType_ != None)
	{
		ERROR_MSG( "ScriptDataSink::write: Tried to write a value when"
				"already writing a structure.\n" );
		return false;
	}

	return setObject( this->currentObject(), value );
}


/**
 *	This method returns a reference to the current node
 */
ScriptObject & ScriptDataSink::currentObject()
{
	if (parents_.empty())
	{
		return root_;
	}

	return parents_.back().object;
}


/**
 *	This method returns a reference to the parent collection of the current
 *	node.
 */
ScriptObject & ScriptDataSink::parentObject()
{
	MF_ASSERT( !parents_.empty() );

	size_t parentSize = parents_.size();

	if (parentSize == 1)
	{
		return root_;
	}

	return parents_[ parentSize - 2 ].object;
}


/**
 *	This method returns a reference to the CollectionType of the parent
 *	collection of the current node.
 */
ScriptDataSink::CollectionType ScriptDataSink::parentType() const
{
	if( parents_.empty() )
	{
		return None;
	}

	return parents_.back().collectionType;
}


/**
 *	This method returns a reference to the sequence index of the current node in
 *	the parent sequence.
 */
size_t & ScriptDataSink::parentItemIndex()
{
	MF_ASSERT( !parents_.empty() );

	MF_ASSERT( this->parentType() == Array || this->parentType() == Tuple );

	return parents_.back().item;
}


/**
 *	This method returns a reference to the field or attribute name of the
 *	current node in the parent mapping or object.
 */
BW::string & ScriptDataSink::parentFieldName()
{
	MF_ASSERT( !parents_.empty() );

	MF_ASSERT( this->parentType() == Class ||
		this->parentType() == Dictionary ||
		this->parentType() == Custom );

	return parents_.back().field;
}

ScriptDataSink::VectorOfNodesHolder::VectorOfNodesHolder() :
	data_( new VectorOfNodes() )
{
}

ScriptDataSink::VectorOfNodesHolder::~VectorOfNodesHolder()
{
	delete data_;
}

ScriptDataSink::VectorOfNodesHolder::operator ScriptDataSink::VectorOfNodes & ()
{
	return *data_;
}

} // namespace BW

// script_data_sink.cpp
