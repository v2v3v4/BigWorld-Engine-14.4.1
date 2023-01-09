#include "pch.hpp"

#include "array_data_type.hpp"

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/array_data_instance.hpp"
#endif

#include "cstdmf/md5.hpp"
#include "resmgr/datasection.hpp"

#include "entitydef/data_description.hpp"
#include "entitydef/property_owner.hpp"

#include "entitydef/script_data_sink.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ScriptListPropertyOwner
// -----------------------------------------------------------------------------

class ScriptListPropertyOwner : public PropertyOwnerBase
{
public:
	ScriptListPropertyOwner() :
		list_(),
		pSequenceDataType_( NULL )
	{
	}

	void init( ScriptList list, const SequenceDataType * pSequenceDataType )
	{
		list_ = list;
		pSequenceDataType_ = pSequenceDataType;
	}

	virtual int getNumOwnedProperties() const
	{
		return (int)list_.size();
	}

	virtual PropertyOwnerBase * getChildPropertyOwner( int childIndex ) const
	{
		ScriptObject childProperty = list_.getItem( childIndex );

		if (!childProperty)
		{
			return NULL;
		}

		return pSequenceDataType_->getElemType().asOwner( childProperty );
	}

	virtual ScriptObject setOwnedProperty( int childIndex,
			BinaryIStream & data )
	{
		const DataType & dataType = pSequenceDataType_->getElemType();

		ScriptObject oldValue = list_.getItem( childIndex );

		ScriptDataSink sink;
		// TODO: Error handling
		dataType.createFromStream( data, sink, /* isPersistentOnly:*/false );

		list_.setItem( childIndex, sink.finalise() );

		return oldValue;
	}

	virtual ScriptObject setOwnedSlice( int startIndex, int endIndex,
			BinaryIStream & data )
	{
		MF_ASSERT( !"ScriptListPropertyOwner::setOwnedSlice: "
				"Not yet implemented\n" );

		return ScriptObject();
	}

	virtual ScriptObject getPyIndex( int index ) const
	{
		return ScriptObject::createFrom( index );
	}

private:
	ScriptList list_;
	const SequenceDataType * pSequenceDataType_;
};


// -----------------------------------------------------------------------------
// Section: ArrayDataType
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param pMeta	The MetaDataType associated with this DataType.
 *	@param elementType	The data type of the elements of this array type.
 *	@param size	The size of the array. If size is 0, the array is of
 *				variable size.
 *	@param dbLen	The database length of the associated property definition.
 */
ArrayDataType::ArrayDataType( MetaDataType * pMeta, DataTypePtr elementType,
		int size, int dbLen ) :
	SequenceDataType( pMeta, elementType, size, dbLen, /*isConst:*/false )
{
}


DataType * ArrayDataType::construct( MetaDataType * pMeta,
	DataTypePtr elementType, int size, int dbLen )
{
	return new ArrayDataType( pMeta, elementType, size, dbLen );
}


bool ArrayDataType::startSequence( DataSink & sink, size_t count ) const
{
	return sink.beginArray( this, count );
}


int ArrayDataType::compareDefaultValue( const DataType & other ) const
{
	const ArrayDataType& otherArray =
		static_cast< const ArrayDataType& >( other );

	if (pDefaultSection_)
	{
		return pDefaultSection_->compare( otherArray.pDefaultSection_ );
	}

	return (otherArray.pDefaultSection_) ? -1 : 0;
}


void ArrayDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}


bool ArrayDataType::getDefaultValue( DataSink & output ) const
{
	if (!pDefaultSection_)
	{
		return this->createDefaultValue( output );
	}
	return this->createFromSection( pDefaultSection_, output );
}


DataSectionPtr ArrayDataType::pDefaultSection() const
{
	return pDefaultSection_;
}

/**
 *	Attach to the given owner; or copy the object if we already have one
 *	(or it is foreign and should be copied anyway).
 */
ScriptObject ArrayDataType::attach( ScriptObject pObject,
	PropertyOwnerBase * pOwner, int ownerRef )
{
	if (!this->DataType::attach( pObject, pOwner, ownerRef ))
	{
		return ScriptObject();
	}

#if defined( SCRIPT_PYTHON )
	PyArrayDataInstancePtr pInst = PyArrayDataInstancePtr::create( pObject );

	// it's easy if it's the right python + entitydef type
	if (pInst.exists() && pInst->dataType() == this)
	{
		if (pInst->hasOwner())
		{	// note: up to caller to check that prop isn't being set back
			MF_ASSERT( ScriptSequence::check( pInst ) );	// into itself
			ScriptSequence pInstSeq( pInst );
			pInst = PyArrayDataInstancePtr(
				new PyArrayDataInstance( this, pInstSeq.size() ),
				PyArrayDataInstancePtr::FROM_NEW_REFERENCE );
			pInst->setInitialSequence( pInstSeq );
		}
	}
	// otherwise it must be a sequence with the correct types
	else	// (since base class method calls isSameType)
	{
		ScriptSequence pObjectSeq( pObject );
		pInst = PyArrayDataInstancePtr( new PyArrayDataInstance( this,
			pObjectSeq.size() ), PyArrayDataInstancePtr::FROM_NEW_REFERENCE );
		pInst->setInitialSequence( pObjectSeq );
	}

	pInst->setOwner( pOwner, ownerRef );
	return pInst;
#else
	return pObject;
#endif
}


void ArrayDataType::detach( ScriptObject pObject )
{
#if defined( SCRIPT_PYTHON )
	PyArrayDataInstancePtr pInst = PyArrayDataInstancePtr::create( pObject );
	IF_NOT_MF_ASSERT_DEV( pInst.exists() )
	{
		return;
	}
	pInst->disowned();
#endif
}


/**
 *	This method returns a property owner that can be used to modify an instance
 *	of an array data type.
 */
PropertyOwnerBase * ArrayDataType::asOwner( ScriptObject pObject ) const
{
#if defined( SCRIPT_PYTHON )
	if (PyArrayDataInstance::Check( pObject ))
	{
		return PyArrayDataInstancePtr( pObject ).get();
	}
#endif

	if (ScriptList::check( pObject ))
	{
		// This is slightly dodgy to be returning a static instance. The caller
		// must not use the returned result after this method is called again.
		// This avoids having all calling code deallocate this.
		static ScriptListPropertyOwner listOwner;
		listOwner.init( ScriptList( pObject ), this );
		return &listOwner;
	}

	ERROR_MSG( "ArrayDataType::asOwner: Not implemented for iOS. "
		"Need to implement an appropriate ProperyOwner to wrap an NSArray\n" );

	return NULL;
}

void ArrayDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "Array", sizeof( "Array" ) );
	this->SequenceDataType::addToMD5( md5 );
}

BW_END_NAMESPACE

// array_data_type.cpp
