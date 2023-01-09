#include "pch.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/watcher.hpp"

#include "entitydef/property_change.hpp"
#include "entitydef/property_owner.hpp"
#include "entitydef/script_data_sink.hpp"

#include "simple_client_entity.hpp"

DECLARE_DEBUG_COMPONENT2( "Connect", 0 )


BW_BEGIN_NAMESPACE

namespace SimpleClientEntity
{
bool g_verbose = false;

class StaticWatcherInitialiser
{
public:
	StaticWatcherInitialiser()
	{
		BW_GUARD;
		MF_WATCH( "Debug/dumpEntityMessages", g_verbose,
			Watcher::WT_READ_WRITE,
			"If true, all entity property and method messages from the server "
				"are printed to the Debug console." );
	}
};
StaticWatcherInitialiser s_initialiser;


/**
 *	Helper class to cast entity as a property owner
 */
class EntityPropertyOwner : public TopLevelPropertyOwner
{
public:
	EntityPropertyOwner( ScriptObject e, const EntityDescription & edesc ) :
		e_( e ), edesc_( edesc ) { }

	// called going to the root of the tree
	virtual bool onOwnedPropertyChanged( PropertyChange & change )
	{
		BW_GUARD;

		// unimplemented
		return true;
	}

	virtual bool getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner )
	{
		BW_GUARD;
		return true;
	}

	// called going to the leaves of the tree
	virtual int getNumOwnedProperties() const
	{
		BW_GUARD;
		return edesc_.clientServerPropertyCount();
	}

	virtual PropertyOwnerBase * getChildPropertyOwner( int ref ) const
	{
		BW_GUARD;
		const DataDescription * pDD = edesc_.clientServerProperty( ref );

		ScriptObject pPyObj(
			PyObject_GetAttrString( e_.get(), (char*)pDD->name().c_str() ),
			ScriptObject::STEAL_REFERENCE );

		if (!pPyObj)
		{
			PyErr_Clear();
			return NULL;
		}

		return pDD->dataType()->asOwner( pPyObj );
	}

	virtual ScriptObject setOwnedProperty( int ref, BinaryIStream & data )
	{
		BW_GUARD;
		const DataDescription * pDD = edesc_.clientServerProperty( ref );
		if (pDD == NULL) return ScriptObject();

		ScriptDataSink sink;
		if (!pDD->createFromStream( data, sink, /* isPersistentOnly */ false ))
		{
			ERROR_MSG( "Entity::handleProperty: "
				"Error streaming off new property value\n" );
			return ScriptObject();
		}

		ScriptObject pNewObj = sink.finalise();

		ScriptObject pOldObj(
			PyObject_GetAttrString( e_.get(), (char*)pDD->name().c_str() ),
			ScriptObject::STEAL_REFERENCE );
		if (!pOldObj)
		{
			PyErr_Clear();
			pOldObj = Py_None;
		}

		int err = PyObject_SetAttrString(
			e_.get(), (char*)pDD->name().c_str(), pNewObj.get() );
		if (err == -1)
		{
			ERROR_MSG( "Entity::handleProperty: "
				"Failed to set new property into Entity\n" );
			PyErr_PrintEx(0);
		}

		return pOldObj;
	}

private:
	ScriptObject e_;
	const EntityDescription & edesc_;
};


/**
 *	Update the identified property on the given entity. Returns true if
 *	the property was found to update.
 */
bool propertyEvent( ScriptObject pEntity, const EntityDescription & edesc,
	int propertyID, BinaryIStream & data, bool shouldUseCallback )
{
	BW_GUARD;
	EntityPropertyOwner king( pEntity, edesc );

	ScriptObject pOldValue = king.setOwnedProperty( propertyID, data );

	if (!pOldValue)
	{
		return false;
	}

	if (shouldUseCallback)
	{
		const DataDescription * pDataDescription =
			edesc.clientServerProperty( propertyID );
		MF_ASSERT_DEV( pDataDescription != NULL );

		BW::string methodName = "set_" + pDataDescription->name();
		Script::call(
			PyObject_GetAttrString( pEntity.get(), (char*)methodName.c_str() ),
			PyTuple_Pack( 1, pOldValue.get() ),
			"Entity::propertyEvent: ",
			/*okIfFunctionNull:*/true );
	}

	return true;
}


namespace
{
class ResetVisitor : public IDataDescriptionVisitor
{
public:
	ResetVisitor( ScriptObject & rEntity, const EntityDescription & rEdesc,
			BinaryIStream & rData ) :
		rEntity_( rEntity ),
		rEdesc_ ( rEdesc ),
		rData_( rData )
	{};

private:
	virtual bool visit( const DataDescription& propDesc )
	{
		EntityPropertyOwner king( rEntity_, rEdesc_ );

		// Discard base properties from the stream
		if (propDesc.isBaseData())
		{
			ScriptDataSink sink;
			if (!propDesc.createFromStream( rData_, sink,
				/* isPersistentOnly */ false ))
			{
				ERROR_MSG( "ResetVisitor::visit: Error discarding base "
					"property reset of %s from Entity\n",
					(char*)propDesc.name().c_str() );
				return false;
			}

			return true;
		}

		ScriptObject oldValue =
			king.setOwnedProperty( propDesc.clientServerFullIndex(), rData_ );

		if (!oldValue)
		{
			return false;
		}

		BW::string methodName = "reset_" + propDesc.name();

		if (!rEntity_.hasAttribute( methodName.c_str() ))
		{
			// Fallback from reset_ method to set_ method
			methodName = "set_" + propDesc.name();
			if (!rEntity_.hasAttribute( methodName.c_str() ))
			{
				// No implicit property callback exists, early-out successfully.
				return true;
			}
		}

		ScriptErrorPrint errorHandler( "Entity::resetPropertiesEvent: " );

		ScriptObject newValue =
			rEntity_.getAttribute( propDesc.name().c_str(), errorHandler );

		if (!newValue)
		{
			ERROR_MSG( "ResetVisitor::visit: "
					"Failed to get new value of property %s from Entity\n",
				propDesc.name().c_str() );
			return false;
		}

		// A failed comparison will appear as "changed" but log the
		// error to the Python log.
		if (!propDesc.dataType()->hasChanged( oldValue, newValue ))
		{
			// New value is unchanged, early-out successfully.
			return true;
		}

		ScriptArgs args = ScriptArgs::create( oldValue );

		ScriptObject result =
			rEntity_.callMethod( methodName.c_str(), args, errorHandler );

		return (bool)result;
	}

	ScriptObject & rEntity_;
	const EntityDescription & rEdesc_;
	BinaryIStream & rData_;
};
}


/**
 *	Reset the properties of the entity from the given data stream of
 *	properties. Returns true if we successfully reset all the properties.
 *
 *	Ignores Base properties in the stream, as we don't support updating Base
 *	properties for active entities.
 */
bool resetPropertiesEvent( ScriptObject pEntity,
	const EntityDescription & edesc, BinaryIStream & data )
{
	BW_GUARD;
	ResetVisitor visitor( pEntity, edesc, data );
	return edesc.visit( EntityDescription::CLIENT_DATA, visitor );
}


/**
 *	Update the identified property on the given entity. Returns true if
 *	the property was found to update.
 */
bool nestedPropertyEvent( ScriptObject pEntity, const EntityDescription & edesc,
	BinaryIStream & data, bool shouldUseCallback, bool isSlice )
{
	BW_GUARD;
	EntityPropertyOwner king( pEntity, edesc );

	ScriptObject * ppOldValue = NULL;
	ScriptList * ppChangePath = NULL;

	ScriptObject pOldValue = ScriptObject::none();
	ScriptList pChangePath;

	if (shouldUseCallback)
	{
		ppOldValue = &pOldValue;
		ppChangePath = &pChangePath;
	}

	int topLevelIndex = king.setNestedPropertyFromExternalStream( data, isSlice,
					ppOldValue, ppChangePath );

	// if this was a top-level property then call the set handler for it
	if (shouldUseCallback)
	{
		const DataDescription * pDataDescription =
			edesc.clientServerProperty( topLevelIndex );
		MF_ASSERT_DEV( pDataDescription != NULL );

		pDataDescription->callSetterCallback( pEntity, pOldValue, pChangePath, 
			isSlice );
	}

	return true;
}


/**
 *	Call the identified method on the given entity. Returns true if the
 *	method description was found.
 */
bool methodEvent( ScriptObject pEntity, const EntityDescription & edesc,
	int methodID, BinaryIStream & data )
{
	BW_GUARD;
	const MethodDescription * pMethodDescription =
		edesc.client().exposedMethod( methodID );

	if (pMethodDescription == NULL)
	{
		ERROR_MSG( "SimpleClientEntity::methodEvent: No method with id %d\n",
				methodID );
		return false;
	}

	if (g_verbose)
	{
		DEBUG_MSG( "SimpleClientEntity::methodEvent: %s.%s - %d bytes\n",
			edesc.name().c_str(),
			pMethodDescription->name().c_str(),
			data.remainingLength() );
	}

	pMethodDescription->callMethod( pEntity, data );
	return true;
}

} // namespace SimpleClientEntity

BW_END_NAMESPACE

// simple_client_entity.cpp
