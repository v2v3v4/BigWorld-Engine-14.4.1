#include "script/first_include.hpp"

#include "py_cell_spatial_data.hpp"

#include "pyscript/script.hpp"
#include "script/py_script_object.hpp"

#include "server/cell_properties_names.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * PyCellSpatialData::s_subscript( PyObject * self, PyObject * key )
{
	MF_ASSERT( PyCellSpatialData::Check( self ) );
	
	ScriptString strKey = 
		ScriptString::create( 
			ScriptObject( key, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (!strKey)
	{
		PyErr_SetString( PyExc_TypeError, "key should be a string" );
		return NULL;
	}

	BW::string propName;
	strKey.getString(propName);
	ScriptObject val = static_cast<PyCellSpatialData*>( self )->get( propName );

	if (!val)
	{
		PyErr_Format( PyExc_KeyError, "%s", propName.c_str() );
		return NULL;
	}

	return val.newRef();
}


/**
 * 	This function returns the number of elements in this dictionary.
 */
Py_ssize_t PyCellSpatialData::s_length( PyObject * self )
{
	MF_ASSERT( PyCellSpatialData::Check( self ) );
	return ((PyCellSpatialData *) self)->size();
}

/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
static PyMappingMethods s_cellSpatialMapping =
{
	PyCellSpatialData::s_length,	// mp_length
	PyCellSpatialData::s_subscript,	// mp_subscript
	NULL							// mp_ass_subscript
};

/*~ class NoModule.PyCellSpatialData
 *  @components{ base }
 *  An instance of PyCellSpatialData emulates a dictionary of cell spatial 
 *	properties, such as position, direction, and space id, indexed by their name. 
 *	It does not support item assignment, but can be used with the subscript operator.
 *	Note that the key must be a string, and that a key error will be thrown 
 *	if the key does not exist in the dictionary.
 *  Instances of this class are used when a base entity is created from a template.
 */
PY_TYPEOBJECT_WITH_MAPPING( PyCellSpatialData, &s_cellSpatialMapping )

PY_BEGIN_METHODS( PyCellSpatialData )

	/*~ function PyCellSpatialData has_key
	 *  @components{ base }
	 *  has_key reports whether a spatial property is present.
	 *  @param key key is name of the property.
	 *  @return A boolean. True if the key was found, false if it was not.
	 */
	PY_METHOD( has_key )

	/*~ function PyCellSpatialData keys
	 *  @components{ base }
	 *  keys returns a list of the properties listed in this object.
	 *  @return A list containing all of the keys.
	 */
	PY_METHOD( keys )

	/*~ function PyCellSpatialData items
	 *  @components{ base }
	 *  items returns a list of the items, as (name, property) pairs, that are listed
	 *  in this object.
	 *  @return A list containing all of the (name, property) pairs, represented as
	 *  tuples containing a string name and a property value.
	 */
	PY_METHOD( items )

	/*~ function PyCellSpatialData values
	 *  @components{ base }
	 *  values returns a list of all the properties' values listed in
	 *  this object.
	 *  @return A list containing all of the properties' values.
	 */
	PY_METHOD( values )

	/*~ function PyCellSpatialData get
	 *  @components{ base }
	 *	get returns value of a property with given name or defaultValue if not found.
	 *	@param propName name of the property to find.
	 *	@param defaultValue a value that is returned in case if there is no requested property.
	 *	@return The property with the given name or defaultValue if not found.
	 */
	PY_METHOD( get )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyCellSpatialData )
PY_END_ATTRIBUTES()

/**
*	Constructs object of PyCellSpatialData with set of properties
*	@param position value of "position" property
*	@param direction value of "direction" property
*	@param spaceID value of "spaceID" property
*	@param isOnGround value of "isOnGround" property
*	@param templateID value of "templateID" property
*/
PyCellSpatialData::PyCellSpatialData( const Position3D & position,
									  const Direction3D & direction,
									  SpaceID spaceID,
									  bool isOnGround,
									  const BW::string & templateID ) :
	PyObjectPlus( &PyCellSpatialData::s_type_ ),
	position_( position ),
	direction_( direction ),
	spaceID_( spaceID ),
	isOnGround_( isOnGround ),
	templateID_( templateID )
{
}


/**
*  reports whether a spatial property is present.
*  @param propName name of the property.
*  @returns true if the key was found, false if it was not.
*/
bool PyCellSpatialData::has_key( const BW::string & propName ) const
{
	if (propName == POSITION_PROPERTY_NAME ||
		propName == DIRECTION_PROPERTY_NAME ||
		propName == SPACE_ID_PROPERTY_NAME ||
		propName == ON_GROUND_PROPERTY_NAME ||
		propName == TEMPLATE_ID_PROPERTY_NAME)
	{
		return true;
	}
		
	return false;
}


/**
*  returns a list of the properties listed in this object.
*  @return A list containing all of the keys.
*/
PyObject * PyCellSpatialData::keys() const
{
	ScriptList keys = ScriptList::create( this->size() );
	
	keys.setItem( 0, ScriptString::create( POSITION_PROPERTY_NAME ) );
	keys.setItem( 1, ScriptString::create( DIRECTION_PROPERTY_NAME ) );
	keys.setItem( 2, ScriptString::create( SPACE_ID_PROPERTY_NAME ) );
	keys.setItem( 3, ScriptString::create( ON_GROUND_PROPERTY_NAME ) );
	keys.setItem( 4, ScriptString::create( TEMPLATE_ID_PROPERTY_NAME ) );
	
	return keys.newRef();
}


/**
*  returns a list of all the Base Entities listed in this object.
*  @return A list containing all of the properties' values.
*/
PyObject * PyCellSpatialData::values() const
{
	ScriptList values = ScriptList::create( this->size() );
	
	values.setItem( 0, ScriptObject::createFrom( position_ ) );
	values.setItem( 1, ScriptObject::createFrom( direction_ ) );
	values.setItem( 2, ScriptObject::createFrom( spaceID_ ) );
	values.setItem( 3, ScriptObject::createFrom( isOnGround_ ) );
	values.setItem( 4, ScriptObject::createFrom( templateID_ ) );
	
	return values.newRef();
}


/**
*  returns a list of the items, as (name, property) pairs, that are listed
*  in this object.
*  @return A list containing all of the (name, property) pairs, represented as
*  tuples containing a string name and a property value.
*/
PyObject * PyCellSpatialData::items() const
{
	ScriptList items = ScriptList::create( this->size() );
	
	items.setItem( 0, ScriptArgs::create( POSITION_PROPERTY_NAME, position_ ) );
	items.setItem( 1, ScriptArgs::create( DIRECTION_PROPERTY_NAME, direction_ ) );
	items.setItem( 2, ScriptArgs::create( SPACE_ID_PROPERTY_NAME, spaceID_ ) );
	items.setItem( 3, ScriptArgs::create( ON_GROUND_PROPERTY_NAME, isOnGround_ ) );
	items.setItem( 4, ScriptArgs::create( TEMPLATE_ID_PROPERTY_NAME, templateID_ ) );
	
	return items.newRef();
}


/**
*	returns value of a property with given name or defaultValue if not found.
*	@param propName name of the property to find.
*	@param defaultValue a value that is returned in case if there is no requested property.
*	@return The property with the given name or defaultValue if not found.
*/
PyObject * PyCellSpatialData::get( const BW::string & propName, 
								  const ScriptObject & defaultValue ) const
{
	MF_ASSERT( defaultValue );

	ScriptObject val = this->get( propName );
	
	return val ? val.newRef() : defaultValue.newRef();
}


/**
*	returns value of a property with given name or NULL object if not found.
*	@param propName name of the property to find.
*	@return The property with the given name or NULL object if not found.
*/
ScriptObject PyCellSpatialData::get( const BW::string & propName ) const
{
	if (propName == POSITION_PROPERTY_NAME)
	{
		return ScriptObject::createFrom( position_ );
	}
	else if (propName == DIRECTION_PROPERTY_NAME)
	{
		return ScriptObject::createFrom( direction_ );
	}
	else if (propName == SPACE_ID_PROPERTY_NAME)
	{
		return ScriptObject::createFrom( spaceID_ );
	}
	else if (propName == ON_GROUND_PROPERTY_NAME)
	{
		return ScriptObject::createFrom( isOnGround_ );
	}
	else if (propName == TEMPLATE_ID_PROPERTY_NAME)
	{
		return ScriptObject::createFrom( templateID_ );
	}

	return ScriptObject();
}

size_t PyCellSpatialData::size() const 
{ 
	return 5; // position, direction, spaceID, isOnGround, templateID
}


BW_END_NAMESPACE

