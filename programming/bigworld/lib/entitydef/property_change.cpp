#include "pch.hpp"

#include "property_change.hpp"

#include "data_type.hpp"
#include "property_owner.hpp"
#include "script_data_sink.hpp"
#include "script_data_source.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bit_reader.hpp"
#include "cstdmf/bit_writer.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Helper functions
// -----------------------------------------------------------------------------


/*
 *	This method implements simple streaming of ChangePath. It is not compressed.
 */
void PropertyChange::writePathSimple( BinaryOStream & stream ) const
{
	stream << uint8( path_.size() );

	ChangePath::const_iterator iter = path_.begin();
	while (iter != path_.end())
	{
		stream << iter->first;
		++iter;
	}
}


// -----------------------------------------------------------------------------
// Section: PropertyChange
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param type	The type of the value changing.
 */
PropertyChange::PropertyChange( const DataType & type ) :
	type_( type ),
	path_(),
	isNestedChange_( false ),
	rootIndex_( -1 )
{
}


/**
 *	This method adds the path to the owner of the changing property to a stream.
 */
void PropertyChange::addSimplePathToStream( BinaryOStream & stream ) const
{
	this->writePathSimple( stream );
	this->addExtraBits( stream );
}


/**
 *	This method adds the path to the owner of the changing property to a stream.
 */
void PropertyChange::addCompressedPathToStream( BinaryOStream & stream,
			int clientServerID, int numClientServerProperties ) const
{
	BitWriter bits;

	if (!this->isNestedChange())
	{
		bits.add( 1, 0 ); // add a 0 bit to say: "stop"
		bits.add( BitReader::bitsRequired( 
			numClientServerProperties ), clientServerID);
	}
	else
	{
		bits.add( 1, 1 ); // add a 1 bit to say: "not done yet"
		bits.add( BitReader::bitsRequired( 
			numClientServerProperties ), clientServerID);

		ChangePath::const_iterator iter = path_.begin();

		while (iter != path_.end())
		{
			int curIdx = iter->first;
			int indexLength = iter->second;
	
			MF_ASSERT( indexLength >= 0 );
	
			bits.add( 1, 1 ); // add a 1 bit to say: "not done yet"
			bits.add( BitReader::bitsRequired( indexLength ), curIdx );

			++iter;
		}

		bits.add( 1, 0 ); // add a 0 bit to say: "stop"

		this->addExtraBits( bits );
	}

	// and put it on the stream (to the nearest byte)
	int used = bits.usedBytes();

	memcpy( stream.reserve( used ), bits.bytes(), used );
}

/**
 *	This method adds this the property change flags to the stream
 *
 *	@param stream	The stream to add to.
 */
void PropertyChange::addInternalFlags( BinaryOStream & stream ) const
{
	int8 flags = 0;

	if (this->isSlice())
	{
		flags |= FLAG_IS_SLICE;
	}
	if (this->isNestedChange())
	{
		flags |= FLAG_IS_NESTED;
	}

	stream << uint8( flags );
}


/**
 *	This method adds this property change to a stream.
 *
 *	@param stream	The stream to add to.
 */
void PropertyChange::addToInternalStream( BinaryOStream & stream ) const
{
	this->addInternalFlags( stream );

	if (this->isNestedChange())
	{
		this->addSimplePathToStream( stream );
	}

	this->addValueToStream( stream );
}


/**
 *	This method adds this property change to a stream.
 *
 *	@param stream	The stream to add to.
 *	@param pOwner	The top-level owner of the property.
 *	@param clientServerID	The top-level id used for client-server.
 *
 *	@return The message id that should be used to send this change.
 */
void PropertyChange::addToExternalStream( BinaryOStream & stream,
			int clientServerID, int numClientServerProperties ) const
{
	this->addCompressedPathToStream( stream,
		clientServerID, numClientServerProperties );

	this->addValueToStream( stream );
}


// -----------------------------------------------------------------------------
// Section: SinglePropertyChange
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param leafIndex	The index of the changing property.
 *	@param type			The type of the property changing.
 */
SinglePropertyChange::SinglePropertyChange( int leafIndex,
			int leafSize,
			const DataType & type ) :
		PropertyChange( type ),
		leafIndex_( leafIndex ),
		leafSize_( leafSize ),
		pValue_( NULL )
{
}


/**
 *
 */
void SinglePropertyChange::addValueToStream( BinaryOStream & stream ) const
{
	ScriptDataSource source( pValue_ );
	MF_VERIFY( type_.addToStream( source, stream, /* isPersistent */ false ) );
}


/**
 *	This method adds extra data specific to this property change. This version
 *	is called when streaming internally to the server.
 */
void SinglePropertyChange::addExtraBits( BinaryOStream & stream ) const
{
	stream << leafIndex_;
}


/**
 *	This method adds extra data specific to this property change. This version
 *	is called when streaming to the client.
 */
void SinglePropertyChange::addExtraBits( BitWriter & writer ) const
{
	writer.add( BitReader::bitsRequired( leafSize_ ), leafIndex_ );
}


// -----------------------------------------------------------------------------
// Section: SlicePropertyChange
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param startIndex The index of the first value to change.
 *	@param endIndex   One greater than the index of the last value to change.
 *	@param newValues  The new values to add.
 *	@param type       The type of the new values.
 */
SlicePropertyChange::SlicePropertyChange(
		size_t startIndex, size_t endIndex,
		size_t originalLeafSize,
		const BW::vector< ScriptObject > & newValues,
		const DataType & type ) :
	PropertyChange( type ),
	startIndex_( startIndex ),
	endIndex_( endIndex ),
	originalLeafSize_( originalLeafSize ),
	newValues_( newValues )
{
	MF_ASSERT( startIndex <= originalLeafSize_ );
	MF_ASSERT( endIndex <= originalLeafSize_ );
}


/**
 *	This method adds the slice values to the stream.
 */
void SlicePropertyChange::addValueToStream( BinaryOStream & stream ) const
{
	// Read until stream is empty. In PyArrayDataInstance::setOwnedSlice.

	BW::vector< ScriptObject >::const_iterator iter = newValues_.begin();

	while (iter != newValues_.end())
	{
		ScriptDataSource source( *iter );
		MF_VERIFY( type_.addToStream( source, stream, false ) );
		++iter;
	}
}


/**
 *	This method adds extra data specific to this property change. This version
 *	is called when streaming internally to the server.
 */
void SlicePropertyChange::addExtraBits( BinaryOStream & stream ) const
{
	stream << uint32(startIndex_) << uint32(endIndex_);
}


/**
 *	This method adds extra data specific to this property change. This version
 *	is called when streaming to the client.
 */
void SlicePropertyChange::addExtraBits( BitWriter & writer ) const
{
	// The value of slice indexes is [0, size], not [0, size - 1].
	// NOTE: This is incorrect, the maximum value is original leaf
	// size not the original leaf size + 1
	const int numBitsRequired = BitReader::bitsRequired(
		static_cast<uint>(originalLeafSize_) + 1 );

	writer.add( numBitsRequired, static_cast<int>(startIndex_) );
	writer.add( numBitsRequired, static_cast<int>(endIndex_) );
}

BW_END_NAMESPACE

// property_change.cpp
