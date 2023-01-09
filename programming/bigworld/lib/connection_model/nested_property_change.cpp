#include "pch.hpp"

#include "nested_property_change.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param changeType 	Whether it is a single-element or slice change.
 *	@param data 		The data stream.
 *
 *	@see NestedPropertyChange::ChangeType
 */
NestedPropertyChange::NestedPropertyChange( ChangeType changeType, 
			BinaryIStream & data ):
		changeType_( changeType ),
		headerBits_( data ),
		data_( data ),
		isFinished_( false ),
		path_(),
		oldSlice_( -1, -1 ),
		newSlice_( -1, -1 )
{
}


/**
 *	Destructor.
 */
NestedPropertyChange::~NestedPropertyChange()
{}


/**
 *	This method reads the next index in the change path, and returns it.
 *
 *	@param propertyNumChildren 	The number of (sub-)properties at this
 *								particular level.
 */
int NestedPropertyChange::readNextIndex( uint propertyNumChildren )
{
	MF_ASSERT( !isFinished_ );

	isFinished_ = (headerBits_.get( 1 ) == 0);

	return this->readNextIndexInternal( propertyNumChildren );
}


int NestedPropertyChange::readNextIndexInternal( uint propertyNumChildren )
{
	int32 index = headerBits_.get( 
		BitReader::bitsRequired( propertyNumChildren ) );
	path_.push_back( index );

	return index;
}


/**
 *	This private method reads the slice range to modify in this change.
 *
 *	@param propertyNumChildren 	The number of (sub-)properties at this
 *								particular level.
 */
void NestedPropertyChange::readOldSlice( uint propertyNumChildren )
{
	// Extra index at the end.
	int numBits = BitReader::bitsRequired( propertyNumChildren + 1 );
	oldSlice_.first = headerBits_.get( numBits );
	oldSlice_.second = headerBits_.get( numBits );
}

BW_END_NAMESPACE

// nested_property_change.cpp
