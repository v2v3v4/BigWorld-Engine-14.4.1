#ifndef PROPERTY_OWNER_HPP
#define PROPERTY_OWNER_HPP

#include "cstdmf/smartpointer.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;
class DataType;
class PropertyChange;

typedef uint8 PropertyChangeType;


/**
 *	This base class is an object that can own properties.
 */
class PropertyOwnerBase
{
public:
	virtual ~PropertyOwnerBase() { }

	/**
	 *	This method is called by a child PropertyOwnerBase to inform us that
	 *	a property has changed. Each PropertyOwner should pass this to their
	 *	parent, adding their index to the path, until the Entity is reached.
	 *
	 *	@return true if succeeded, false if an exception was raised
	 */
	virtual bool onOwnedPropertyChanged( PropertyChange & change )
	{
		return true;
	}

	virtual bool getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner ) { return true; }

	/**
	 *	This method returns the number of child properties this PropertyOwner
	 *	currently has.
	 */
	virtual int getNumOwnedProperties() const = 0;

	/**
	 *	This method returns the child PropertyOwner of this PropertyOwner. If
	 *	the child property is not a PropertyOwner, NULL is returned.
	 *
	 *	@param childIndex The index of the child to get.
	 */
	virtual PropertyOwnerBase *
		getChildPropertyOwner( int childIndex ) const = 0;

	/**
	 *	This method sets a child property to a new value.
	 *
	 *	@param childIndex	The index of the child property to change.
	 *	@param data			A stream containing the new value.
	 *
	 *	@return The previous value of the property.
	 */
	virtual ScriptObject setOwnedProperty( int childIndex,
			BinaryIStream & data ) = 0;


	/**
	 *	This method modifies a slice of an array property.
	 *
	 *	@param startIndex The start index of the slice to replace.
	 *	@param endIndex One greater than the end index of the slice to replace.
	 *	@param data A stream containing the new values.
	 */
	virtual ScriptObject setOwnedSlice( int startIndex, int endIndex,
			BinaryIStream & data ) { return ScriptObject(); }


	/**
	 *	This method returns a Python object representing the property with the
	 *	given index.
	 */
	virtual ScriptObject getPyIndex( int index ) const = 0;


	/**
	 *	This method modifies a property owned by this object.
	 *
	 *	@param rpOldValue A reference that will be populated with the old value.
	 *	@param pNewValue The new value to set the property to.
	 *	@param dataType The type of the property being changed.
	 *	@param index The index of the property being changed.
	 *	@param forceChange If true, the change occurs even if the old and new
	 *		values are the same.
	 */
	bool changeOwnedProperty( ScriptObject & rpOldValue, ScriptObject pNewValue,
								const DataType & dataType, int index,
								bool forceChange = false );
};


/**
 *	This class specialises PropertyOwnerBase to add functionality for top-level
 *	Property Owners. That is Entity.
 */
class TopLevelPropertyOwner : public PropertyOwnerBase
{
public:
	bool setPropertyFromInternalStream( BinaryIStream & stream,
			ScriptObject * ppOldValue, ScriptList * ppChangePath,
			int rootIndex, bool * pIsSlice );

	int setNestedPropertyFromExternalStream(
			BinaryIStream & stream, bool isSlice,
			ScriptObject * ppOldValue, ScriptList * ppChangePath );

private:
	virtual ScriptObject getPyIndex( int index ) const
	{
		MF_ASSERT( 0 );
		return ScriptObject();
	}
};


/**
 *	This is a handy linking class for objects that dislike virtual functions.
 */
template <class C>
class PropertyOwnerLink : public TopLevelPropertyOwner
{
public:
	PropertyOwnerLink( C & self ) : self_( self ) { }

	virtual bool onOwnedPropertyChanged( PropertyChange & change )
	{
		return self_.onOwnedPropertyChanged( change );
	}

	virtual bool getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner )
	{
		return self_.getTopLevelOwner( change, rpTopLevelOwner );
	}

	virtual int getNumOwnedProperties() const
	{
		return self_.getNumOwnedProperties();
	}

	virtual PropertyOwnerBase * getChildPropertyOwner( int ref ) const
	{
		return self_.getChildPropertyOwner( ref ) ;
	}

	virtual ScriptObject setOwnedProperty( int ref, BinaryIStream & data )
	{
		return self_.setOwnedProperty( ref, data );
	}

private:
	C & self_;
};


#if defined( SCRIPT_PYTHON )

BW_END_NAMESPACE

#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is the normal property owner for classes that can have a virtual
 *	function table.
 */
class PropertyOwner : public PyObjectPlus, public PropertyOwnerBase
{
protected:
	PropertyOwner( PyTypeObject * pType ) : PyObjectPlus( pType ) { }
};

#endif // SCRIPT_PYTHON

BW_END_NAMESPACE

#endif // PROPERTY_OWNER_HPP
