#include "pch.hpp"

#include "base_properties_helper.hpp"
#include "editor_views.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/editor_chunk_item.hpp"
#include "resmgr/xml_section.hpp"

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
// Section: PropertyIndex
///////////////////////////////////////////////////////////////////////////////

/**
 *	Default constructor
 */
PropertyIndex::PropertyIndex()
{
}


/**
 *	Constructor to create a new object from an int.
 *
 *	@param propIdx		index of the property.
 */
PropertyIndex::PropertyIndex( int propIdx )
{
	BW_GUARD;

	indices_.push_back( propIdx );
}


/**
 *	This method is used to determine if the object has no indices.
 *
 *	@return		true if it contains no indices, false otherwise.
 */
bool PropertyIndex::empty() const
{
	return count() == 0;
}


/**
 *	This method returns the number of indices contained in this PropertyItem
 *	object.
 *
 *	@return		the number of indices contained in this object.
 */
int PropertyIndex::count() const
{
	return (int)indices_.size();
}


/**
 *	This method returns all the indices but the first.
 *
 *	@return 	a PropertyIndex containing the last indices of the object.
 */
PropertyIndex PropertyIndex::tail() const
{
	BW_GUARD;

	PropertyIndex result;
	if ( !indices_.empty() )
		result.indices_.assign( indices_.begin()+1, indices_.end() );
	return result;
}


/**
 *	This method adds a PropertyIndex object (with 0, 1 or more indices) at the
 *	then end of the indices vector of the 'this' object.
 *
 *	@param PropertyIndex	object with the indices to append
 */
void PropertyIndex::append( const PropertyIndex& i )
{
	BW_GUARD;

	for( BW::vector<int>::const_iterator it = i.indices_.begin();
		it != i.indices_.end(); ++it )
	{
		indices_.push_back( (*it) );
	}
}


/**
 *	This method returns the index corresponding to position 'i' of the internal
 *	indices vector.
 *
 *	@param i	position of the desired index in the internal indices vector.
 *	@return		the desired index, or -1 if 'i' is invalid.
 */
int PropertyIndex::valueAt( int i )
{
	BW_GUARD;

	if ( i >= 0 && i < (int)indices_.size() )
		return indices_[i];
	else
		return -1;
}


/**
 *	This method sets the index at position 'i' in the internal indices vector
 *	to the provided value 'propIdx'.
 *
 *	@param i		position of the index in the internal indices vector.
 *	@param propIdx	new index to set at position 'i' in the indices vector.
 *	@return			true if successful, false otherwise.
 */
bool PropertyIndex::valueAt( int i, int propIdx )
{
	BW_GUARD;

	MF_ASSERT( propIdx >= 0 );
	if ( i < 0 || i >= (int)indices_.size() )
		return false;

	indices_.push_back( propIdx );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Section: BasePropertiesHelper
///////////////////////////////////////////////////////////////////////////////

/**
 *	The constructor for this class only initialises it's pointer members.
 */
BasePropertiesHelper::BasePropertiesHelper() :
	pItem_( NULL )
{
}


/**
 *	This method returns the chunk item that owns this property helper.
 *
 *	@return		Chunk item that owns this property helper
 */
EditorChunkItem* BasePropertiesHelper::pItem() const
{
	return pItem_;
}

/**
 *	This method reloads the owner and calls the 'resetSelUpdate' Python method.
 */
void BasePropertiesHelper::resetSelUpdate( bool keepSelection /*= false*/ )
{
	BW_GUARD;

	int curSel;
	if ( keepSelection )
	{
		curSel = PropTable::table()->propertyList()->GetCurSel();
	}

	pItem_->edMainThreadLoad();
	PyObject* pModule = PyImport_ImportModule( "WorldEditorDirector" );
	if (pModule != NULL)
	{
		PyObject* pScriptObject = PyObject_GetAttrString( pModule, "bd" );

		if (pScriptObject != NULL)
		{
			Script::call(
				PyObject_GetAttrString( pScriptObject, "resetSelUpdate" ),
				Py_BuildValue( "(i)", keepSelection ? 1 : 0 ),
				"Reloading item");
			Py_DECREF( pScriptObject );
		}
		Py_DECREF( pModule );
	}

	if ( keepSelection )
	{
		PropTable::table()->propertyList()->selectItem( curSel, false );
	}
}


/**
 *	This method refreshes the item by performing a toss and untoss operation.
 */
void BasePropertiesHelper::refreshItem()
{
	BW_GUARD;

	// Cache the chunk
	Chunk* chunk = pItem()->chunk();
	pItem()->toss(0);
	pItem()->toss(chunk);
}


/**
 *	This method is used to know if a property is using the default value.
 *
 *	@param index	Top level property index
 *	@return			True if the property at 'index' is using the default value,
 *					false otherwise.
 */
bool BasePropertiesHelper::propUsingDefault( int index )
{
	BW_GUARD;

	if ( index < 0 || index >= (int)usingDefault_.size() )
	{
		WARNING_MSG( "BasePropertiesHelper::propUsingDefault (get): index out of bounds.\n" );
		return false;
	}

	// If it's a sequence, it's using the default value if it's empty.
	PyObjectPtr ob( propGetPy( index ), PyObjectPtr::STEAL_REFERENCE );
	if ( PySequence_Check( ob.getObject() ) )
		return ( PySequence_Size( ob.getObject() ) == 0 );

	return usingDefault_[ index ];
}


/**
 *	This method is used to know if a property is using the default value.
 *
 *	@param index			Top level property index
 *	@param usingDefault		True to point out that the property is using the
 *							default value, false otherwise.
 */
void BasePropertiesHelper::propUsingDefault( int index, bool usingDefault )
{
	BW_GUARD;

	if ( index < 0 || index >= (int)usingDefault_.size() )
	{
		WARNING_MSG( "BasePropertiesHelper::propUsingDefault (set): index out of bounds.\n" );
		return;
	}

	usingDefault_[ index ] = usingDefault;
}


/**
 *	This method is used to know if a property is using the default value.
 *
 *	@param usingDefault		Vector containing a bool for each property that
 *							indicates whether it is using the default.
 */
void BasePropertiesHelper::propUsingDefaults( BW::vector<bool> usingDefault  )
{
	BW_GUARD;

	if ( (int)usingDefault.size() != propCount() )
	{
		WARNING_MSG( "BasePropertiesHelper::propUsingDefaults: input vector is of different size than the properties.\n" );
	}

	usingDefault_ = usingDefault;
}


/**
 *	This method returns the property in integer form.
 *
 *	@param	index	The index of the property.
 *	@return	The property in integer form.
 */
int BasePropertiesHelper::propGetInt( PropertyIndex index )
{
	BW_GUARD;

	return propGet( index )->asInt();
}


/**
 *	This method sets the integer form of a property.
 *
 *	@param	index	The index of the property.
 *	@param	i		The integer form of property.
 *	@return	Boolean success or failure.
 */
bool BasePropertiesHelper::propSetInt( PropertyIndex index, int i )
{
	BW_GUARD;

	DataSectionPtr pTemp = new XMLSection( "temp" );
	pTemp->setInt( i );
	return propSet( index, pTemp );
}


/**
 *	This method returns the property in integer form.
 *
 *	@param	index	The index of the property.
 *	@return	The property in integer form.
 */
uint32 BasePropertiesHelper::propGetUInt( PropertyIndex index )
{
	BW_GUARD;

	return propGet( index )->asUInt();
}


/**
 *	This method sets the integer form of a property.
 *
 *	@param	index	The index of the property.
 *	@param	i		The integer form of property.
 *	@return	Boolean success or failure.
 */
bool BasePropertiesHelper::propSetUInt( PropertyIndex index, uint32 i )
{
	BW_GUARD;

	DataSectionPtr pTemp = new XMLSection( "temp" );
	pTemp->setUInt( i );
	return propSet( index, pTemp );
}


/**
 *	This method returns the property in float form.
 *
 *	@param	index	The index of the property.
 *	@return	The property in float form.
 */
float BasePropertiesHelper::propGetFloat( PropertyIndex index )
{
	BW_GUARD;

	return propGet( index )->asFloat();
}


/**
 *	This method sets the float form of a property.
 *
 *	@param	index	The index of the property.
 *	@param	f		The float form of property.
 *	@return	Boolean success or failure.
 */
bool BasePropertiesHelper::propSetFloat( PropertyIndex index, float f )
{
	BW_GUARD;

	DataSectionPtr pTemp = new XMLSection( "temp" );
	pTemp->setFloat( f );
	return propSet( index, pTemp );
}


/**
 *	This method returns the property in string form.
 *
 *	@param	index	The index of the property.
 *	@return	The property in string form.
 */
BW::string BasePropertiesHelper::propGetString( PropertyIndex index ) const
{
	BW_GUARD;

    BasePropertiesHelper *myself =
		const_cast<BasePropertiesHelper *>(this);
	return myself->propGet( index )->asString();
}


/**
 *	This method sets the string form of a property.
 *
 *	@param	index	The index of the property.
 *	@param	s		The string form of property.
 *	@return	Boolean success or failure.
 */
bool BasePropertiesHelper::propSetString( PropertyIndex index, const BW::string & s )
{
	BW_GUARD;

	DataSectionPtr pTemp = new XMLSection( "temp" );
	pTemp->setString( s );
	return propSet( index, pTemp );
}


/**
 *	This method returns the property in Vector2 form.
 *
 *	@param	index	The index of the property.
 *	@return	The property in Vector2 form.
 */
Vector2 BasePropertiesHelper::propGetVector2( PropertyIndex index )
{
	BW_GUARD;

	return propGet( index )->asVector2();
}


/**
 *	This method sets the Vector2 form of a property.
 *
 *	@param	index	The index of the property.
 *	@param	f		The Vector2 form of property.
 *	@return	Boolean success or failure.
 */
bool BasePropertiesHelper::propSetVector2( PropertyIndex index, Vector2 v )
{
	BW_GUARD;

	DataSectionPtr pTemp = new XMLSection( "temp" );
	pTemp->setVector2( v );
	return propSet( index, pTemp );
}


/**
 *	This method returns the property in Vector4 form.
 *
 *	@param	index	The index of the property.
 *	@return	The property in Vector4 form.
 */
Vector4 BasePropertiesHelper::propGetVector4( PropertyIndex index )
{
	BW_GUARD;

	return propGet( index )->asVector4();
}


/**
 *	This method sets the Vector4 form of a property.
 *
 *	@param	index	The index of the property.
 *	@param	f		The Vector4 form of property.
 *	@return	Boolean success or failure.
 */
bool BasePropertiesHelper::propSetVector4( PropertyIndex index, Vector4 v )
{
	BW_GUARD;

	DataSectionPtr pTemp = new XMLSection( "temp" );
	pTemp->setVector4( v );
	return propSet( index, pTemp );
}
BW_END_NAMESPACE

