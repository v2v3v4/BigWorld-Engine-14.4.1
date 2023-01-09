#include "pch.hpp"

#include "chunk/chunk_item.hpp"
#include "chunk/editor_chunk_item.hpp"
#include "array_properties_helper.hpp"
#include "resmgr/xml_section.hpp"
#include "entitydef/data_types.hpp"
#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"

BW_BEGIN_NAMESPACE

/**
 *	The constructor for this class only initialises it's pointer members.
 */
ArrayPropertiesHelper::ArrayPropertiesHelper() :
	BasePropertiesHelper(),
	pSeq_( NULL )
{
}


/**
 *	The destructor for this class only decrements the refcnt of pSeq_.
 */
ArrayPropertiesHelper::~ArrayPropertiesHelper()
{
	BW_GUARD;

	Py_XDECREF( pSeq_ );
}


/**
 *	This method initialises the helper with the values it needs to be able
 *	to manage an array of properties.
 *
 *	@param pItem			Chunk item that owns the properties.
 *	@param dataType			Data type of the array itself.
 *	@param pSeq				Py object that contains the array as a Py sequence.
 *	@param changedCallback	Optional callback, called when a property changes.
 */
void ArrayPropertiesHelper::init(
	EditorChunkItem* pItem,
	DataTypePtr dataType,
	PyObject* pSeq,
	BWBaseFunctor1<int>* changedCallback /*=NULL*/ )
{
	BW_GUARD;

	pItem_ = pItem;
	dataType_ = dataType;
	pSeq_ = pSeq;
	Py_XINCREF( pSeq_ ); // init needs a new reference.
	changedCallback_ = changedCallback;
}


/**
 *	This method returns the python sequence object that contains the array's
 *	items.
 *
 *	@return		Python sequence object that contains the array's items.
 */
PyObject* ArrayPropertiesHelper::pSeq() const
{
	return pSeq_;
}


/**
 *	This method adds an item to the array.
 *
 *	@return		true if successful.
 */
bool ArrayPropertiesHelper::addItem()
{
	BW_GUARD;

	ScriptDataSink sink;
	if (!dataType_->getDefaultValue( sink ))
	{
		return false;
	}
	ScriptObject defaultValue = sink.finalise();
	PyObject* newItem = Py_BuildValue( "[O]", defaultValue.getObject() );

	return
		PySequence_InPlaceConcat( pSeq_, newItem ) != NULL;
}


/**
 *	This method deletes an item from the array.
 *
 *	@param index	index of the item to delete.
 *	@return			true if successful.
 */
bool ArrayPropertiesHelper::delItem( int index )
{
	BW_GUARD;

	return PySequence_DelItem( pSeq_, index ) != -1;
}


/**
 *	This method returns the name of the property at the passed index.
 *
 *	@param	index	The index of the property in the array.
 *	@return	The name of the property at the passed index.
 */
BW::string ArrayPropertiesHelper::propName( PropertyIndex index )
{
	BW_GUARD;

	BW::stringstream ss;
	ss << "[" << index.valueAt(0) << "]";
	return ss.str();
}


/**
 *	This method returns the number of properties in the array.
 *
 *	@return	The number of properties in the array.
 */
int ArrayPropertiesHelper::propCount() const
{
	BW_GUARD;

    return (int)PySequence_Size( pSeq_ );
}


/**
 *	This method returns the index of the named property.
 *
 *	@param	name	The name of the property.
 *	@return	The index of the named property.
 */
PropertyIndex ArrayPropertiesHelper::propGetIdx( const BW::string& name ) const
{
	BW_GUARD;

	PropertyIndex result;

	BW::string::size_type openBracket = name.find_first_of( '[' );
	if ( openBracket == BW::string::npos )
		return result;

	BW::string::size_type closeBracket = name.find_first_of( ']', openBracket );
	if ( closeBracket == BW::string::npos || closeBracket <= openBracket )
		return result;

	BW::string index = name.substr( openBracket+1, closeBracket - openBracket - 1 );
	result.append( atoi( index.c_str() ) );

	return result;
}


/**
 *	This method returns the property in PyObject form (New Reference).
 *
 *	@param	index	The index of the property in the array.
 *	@return	The property in PyObject form (New Reference).
 */
PyObject* ArrayPropertiesHelper::propGetPy( PropertyIndex index )
{
	BW_GUARD;

	MF_ASSERT( index.valueAt(0) < propCount() );

	return PySequence_GetItem( pSeq_, index.valueAt(0) );
}


/**
 *	This method sets the PyObject form of a property.
 *
 *	@param	index	The index of the property in the array.
 *	@param	pObj	The PyObject of the property.
 *	@return	Boolean success or failure.
 */
bool ArrayPropertiesHelper::propSetPy( PropertyIndex index, PyObject * pObj )
{
	BW_GUARD;

	MF_ASSERT( index.valueAt(0) < (int)propCount() );
	if ( PySequence_SetItem( pSeq_, index.valueAt(0), pObj ) == -1 )
	{
		ERROR_MSG( "ArrayPropertiesHelper::propSetPy: Failed to set value\n" );
		PyErr_Print();
		return false;
	}

	return true;
}


/**
 *	This method returns the property in datasection form.
 *
 *	@param	index	The index of the property in the array.
 *	@return	The property in datasection form.
 */
DataSectionPtr ArrayPropertiesHelper::propGet( PropertyIndex index )
{
	BW_GUARD;

	MF_ASSERT( index.valueAt(0) < (int)propCount() );
	ScriptObject pValue( PySequence_GetItem( pSeq_, index.valueAt(0) ),
		ScriptObject::FROM_NEW_REFERENCE );

	if (!pValue)
	{
		PyErr_Clear();
		return NULL;
	}

	DataSectionPtr pTemp = new XMLSection( "temp" );

	ScriptDataSource source( pValue );

	dataType_->addToSection( source, pTemp );

	return pTemp;
}


/**
 *	This method sets the datasection form of a property.
 *
 *	@param	index	The index of the property in the array.
 *	@param	pTemp	The datasection form of property.
 *	@return	Boolean success or failure.
 */
bool ArrayPropertiesHelper::propSet( PropertyIndex index, DataSectionPtr pTemp )
{
	BW_GUARD;

	MF_ASSERT( index.valueAt(0) < (int)propCount() );

	ScriptDataSink sink;

	if (!dataType_->createFromSection( pTemp, sink ))
	{
		return false;
	}

	PyObjectPtr pNewVal = sink.finalise();

	if (!pNewVal)
	{
		PyErr_Clear();
		return false;
	}

	PySequence_SetItem( pSeq_, index.valueAt(0), pNewVal.get() );

	if ( changedCallback_ != NULL )
		(*changedCallback_)( index.valueAt(0) );

	return true;
}


/**
 *	This method sets the property to its default value.
 *
 *	@param	index	The index of the property in pType.
 */
void ArrayPropertiesHelper::propSetToDefault( int index )
{
	BW_GUARD;

	ScriptDataSink sink;
	if (dataType_->getDefaultValue( sink ))
	{
		this->propSetPy( index, sink.finalise().getObject() );
	}
}


/**
 *	Finds out if a property is a UserDataObject-to-UserDataObject link.
 *
 *	@param index	Property index.
 *	@return			true if it's a UserDataObject-to-UserDataObject link.
 */
bool ArrayPropertiesHelper::isUserDataObjectLink( int index )
{
	BW_GUARD;

	return dataType_->typeName() == "UDO_REF";
}


/**
 *	Finds if a property is an array of UserDataObject-to-UserDataObject links.
 *
 *	@param index	Property index.
 *	@return			true if it's an array of UserDataObject-to-UserDataObject links.
 */
bool ArrayPropertiesHelper::isUserDataObjectLinkArray( int index )
{
	// TODO: support arrays of arrays
	return false;
}
BW_END_NAMESPACE

