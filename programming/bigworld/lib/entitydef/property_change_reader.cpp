#include "pch.hpp"

#include "property_change_reader.hpp"

#include "property_change.hpp" // Just for MAX_SIMPLE_PROPERTY_CHANGE_ID

#include "data_type.hpp"
#include "property_owner.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bit_reader.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PropertyChangeReader
// -----------------------------------------------------------------------------

/**
 *	This method reads and applies a property change.
 *
 *	@param stream  The stream to read from.
 *	@param pOwner  The top-level owner of the property.
 *	@param ppOldValue If not NULL, this is the old value of the property.
 *	@param ppChangePath If not NULL, this is the change path to use when
 *						applying the property update.
 *
 *	@return True on success, false otherwise
 */
bool PropertyChangeReader::readSimplePathAndApply( BinaryIStream & stream,
		PropertyOwnerBase * pOwner,
		ScriptObject * ppOldValue,
		ScriptList * ppChangePath )
{
	uint8 size;
	stream >> size;

	uint8 i = 0;

	while ((i < size) && pOwner)
	{
		int32 index;
		stream >> index;

		this->updatePath( ppChangePath, pOwner->getPyIndex( index ) );

		pOwner = pOwner->getChildPropertyOwner( index );

		++i;
	}

	if (!pOwner)
	{
		ERROR_MSG( "PropertyChangeReader::readAndApply: "
				"pOwner is NULL. %d/%d.\n",
			i, size );

		return false;
	}

	this->readExtraBits( stream );

	this->updatePath( ppChangePath );

	this->doApply( stream, pOwner, ppOldValue, ppChangePath );

	return true;
}


/**
 *	This method reads and applies a property change.
 *
 *	@param stream  The stream to read from.
 *	@param pOwner  The top-level owner of the property.
 *	@param ppOldValue If not NULL, this is set to the old value of the property.
 *	@param ppChangePath If not NULL, this is set to the path of the property
 *			that has changed.
 *
 *	@return The top-level index of the change.
 */
int PropertyChangeReader::readCompressedPathAndApply( BinaryIStream & stream,
		PropertyOwnerBase * pOwner,
		ScriptObject * ppOldValue,
		ScriptList * ppChangePath )
{
	int topLevelIndex = -1;

	BitReader bits( stream );

	while ((bits.get( 1 ) != 0) && pOwner)
	{
		int numProperties = pOwner->getNumOwnedProperties();
		int index = bits.get( BitReader::bitsRequired( numProperties ) );

		if (topLevelIndex == -1)
		{
			topLevelIndex = index;
		}
		else
		{
			this->updatePath( ppChangePath, pOwner->getPyIndex( index ) );
		}

		pOwner = pOwner->getChildPropertyOwner( index );
	}

	if (!pOwner)
	{
		ERROR_MSG( "PropertyChangeReader::readAndApply: Invalid path to owner. "
					"topLevelIndex = %d\n",
				topLevelIndex );

		return -1 - std::max( topLevelIndex, 0 );
	}

	int index = this->readExtraBits( bits, pOwner->getNumOwnedProperties() );

	if (topLevelIndex == -1)
	{
		topLevelIndex = index;
	}
	else
	{
		this->updatePath( ppChangePath );
	}

	this->doApply( stream, pOwner, ppOldValue, ppChangePath );

	return topLevelIndex;
} 


/**
 *	This helper method calls the apply virtual method and performs error
 *	checking.
 */
void PropertyChangeReader::doApply( BinaryIStream & stream,
		PropertyOwnerBase * pOwner, ScriptObject * ppOldValue,
		ScriptList * ppChangePath )
{
	ScriptObject pOldValue = this->apply( stream, pOwner,
			ppChangePath ? *ppChangePath : ScriptList() );

	if (!pOldValue)
	{
		ERROR_MSG( "PropertyChangeReader::readAndApply: Old value is NULL\n" );
	}
	else if (ppOldValue)
	{
		*ppOldValue = pOldValue;
	}
}


/**
 *	This method appends the input index to the change path.
 */
void PropertyChangeReader::updatePath( ScriptList * ppChangePath,
		ScriptObject pIndex ) const
{
	if (!ppChangePath)
	{
		return;
	}

	if (!(*ppChangePath))
	{
		*ppChangePath = ScriptList::create();
	}

	if (pIndex)
	{
		(*ppChangePath).append( pIndex );
	}
}


// -----------------------------------------------------------------------------
// Section: SinglePropertyChangeReader
// -----------------------------------------------------------------------------

/**
 *	This method applies this property change.
 *
 *	@param The stream containing the new value.
 *	@param The low-level owner of the property change.
 *
 *	@return The old value.
 */
ScriptObject SinglePropertyChangeReader::apply( BinaryIStream & stream,
		PropertyOwnerBase * pOwner, ScriptList pChangePath )
{
	if (pChangePath)
	{
		pChangePath.append( pOwner->getPyIndex( leafIndex_ ) );
	}

	return pOwner->setOwnedProperty( leafIndex_, stream );
}


/**
 *	This method reads the extra data specific to this PropertyChange type. This
 *	version is used for server-internal changes.
 */
int SinglePropertyChangeReader::readExtraBits( BinaryIStream & stream )
{
	stream >> leafIndex_;

	return leafIndex_;
}


/**
 *	This method reads the extra data specific to this PropertyChange type. This
 *	version is used for client-server changes.
 */
int SinglePropertyChangeReader::readExtraBits( BitReader & reader,
		int leafSize )
{
	const int numBitsRequired = BitReader::bitsRequired( leafSize );
	leafIndex_ = reader.get( numBitsRequired );

	return leafIndex_;
}


// -----------------------------------------------------------------------------
// Section: SlicePropertyChangeReader
// -----------------------------------------------------------------------------

/**
 *	This method applies this property change.
 *
 *	@param The stream containing the new value.
 *	@param The low-level owner of the property change.
 *
 *	@return The old value.
 */
ScriptObject SlicePropertyChangeReader::apply( BinaryIStream & stream,
		PropertyOwnerBase * pOwner, ScriptList pChangePath )
{
	int oldSize = pOwner->getNumOwnedProperties();

	ScriptObject pOldValues =
		pOwner->setOwnedSlice( startIndex_, endIndex_, stream );

	int newSize = pOwner->getNumOwnedProperties();

	if (pChangePath && pOldValues)
	{
		ScriptTuple index = ScriptTuple::create( 2 );
		index.setItem( 0, ScriptObject::createFrom( startIndex_ ) );
		index.setItem( 1, ScriptObject::createFrom(
											endIndex_ + newSize - oldSize ) );

		pChangePath.append( index );
	}

	return pOldValues;
}


/**
 *	This method reads the extra data specific to this PropertyChange type. This
 *	version is used for server-internal changes.
 */
int SlicePropertyChangeReader::readExtraBits( BinaryIStream & stream )
{
	stream >> startIndex_ >> endIndex_;

	return -1;
}


/**
 *	This method reads the extra data specific to this PropertyChange type. This
 *	version is used for client-server changes.
 */
int SlicePropertyChangeReader::readExtraBits( BitReader & reader,
		int leafSize )
{
	const int numBitsRequired = BitReader::bitsRequired( leafSize + 1 );
	startIndex_ = reader.get( numBitsRequired );
	endIndex_ = reader.get( numBitsRequired );

	return -1;
}

BW_END_NAMESPACE

// property_change_reader.cpp
