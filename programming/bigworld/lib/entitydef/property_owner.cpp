#include "pch.hpp"

#include "property_owner.hpp"
#include "property_change.hpp"
#include "property_change_reader.hpp"

#include "data_type.hpp"

#include "cstdmf/binary_stream.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PropertyOwnerBase
// -----------------------------------------------------------------------------

/**
 *	This method changes a property owned by this one. It propagates this change
 *	to the top-level owner.
 */
bool PropertyOwnerBase::changeOwnedProperty( ScriptObject & rpOldValue,
		ScriptObject pNewValue, const DataType & dataType, int index,
		bool forceChange )
{
	if (dataType.canIgnoreAssignment( rpOldValue, pNewValue ))
	{
		return true;
	}

	bool changed = forceChange ||
		dataType.hasChanged( rpOldValue, pNewValue );

	SinglePropertyChange change( index, this->getNumOwnedProperties(), dataType );
	PropertyOwnerBase * pTopLevelOwner = NULL;

	if (changed)
	{
		if (!this->getTopLevelOwner( change, pTopLevelOwner ))
		{
			return false;
		}
	}

	// TODO: attach() should be const
	ScriptObject pRealNewValue =
		const_cast< DataType & >( dataType ).attach( pNewValue, this, index );

	if (!pRealNewValue)
	{
		return false;
	}

	const_cast< DataType & >( dataType ).detach( rpOldValue );
	rpOldValue = pRealNewValue;

	if (pTopLevelOwner != NULL)
	{
		change.setValue( pRealNewValue );
		if (!pTopLevelOwner->onOwnedPropertyChanged( change ))
		{
			return false;
		}
	}

	return true;
}



// -----------------------------------------------------------------------------
// Section: TopLevelPropertyOwner
// -----------------------------------------------------------------------------

namespace
{

SinglePropertyChangeReader g_singleReader;
SlicePropertyChangeReader g_sliceReader;

PropertyChangeReader * getPropertyChangeReader( bool isSlice )
{
	if (isSlice)
	{
		return &g_sliceReader;
	}
	else
	{
		return &g_singleReader;
	}
}

} // anonymous namespace


/**
 *	This method sets an owned property from a stream that has been sent within
 *	the server.
 */
bool TopLevelPropertyOwner::setPropertyFromInternalStream(
		BinaryIStream & stream,
		ScriptObject * ppOldValue,
		ScriptList * ppChangePath,
		int rootIndex,
		bool * pIsSlice)
{
	int8 flags;
	stream >> flags;
	bool isSlice = (flags & PropertyChange::FLAG_IS_SLICE) != 0;
	bool isNested = (flags & PropertyChange::FLAG_IS_NESTED) != 0;

	if (pIsSlice)
	{
		*pIsSlice = isSlice;
	}

	if (isNested)
	{	
		PropertyChangeReader * pReader = getPropertyChangeReader( isSlice );

		return pReader->readSimplePathAndApply( stream, 
			this->getChildPropertyOwner( rootIndex ),
			ppOldValue, ppChangePath );
	}

	MF_ASSERT( !isSlice );

	// See PropertyChangeReader::doApply and SinglePropertyChangeReader::apply()
	ScriptObject pOldValue = this->setOwnedProperty( rootIndex, stream );
	if (!pOldValue)
	{
		ERROR_MSG( "TopLevelPropertyOwner::setPropertyFromInternalStream: "
			"Old value is NULL\n" );
	}
	else if (ppOldValue)
	{
		*ppOldValue = pOldValue;
	}

	return true;
}


/**
 *	This method sets an owned property from a stream that has been sent from the
 *	server to a client.
 */
int TopLevelPropertyOwner::setNestedPropertyFromExternalStream(
		BinaryIStream & stream, bool isSlice,
		ScriptObject * ppOldValue,
		ScriptList * ppChangePath )
{
	PropertyChangeReader * pReader = getPropertyChangeReader( isSlice );


	return pReader->readCompressedPathAndApply( stream, this,
			ppOldValue, ppChangePath );
}

BW_END_NAMESPACE

// property_owner.cpp
