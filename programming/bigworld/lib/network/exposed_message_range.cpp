#include "pch.hpp"

#include "exposed_message_range.hpp"

#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ExposedMessageRange
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ExposedMessageRange::ExposedMessageRange( int firstMsgID, int lastMsgID ) :
	firstMsgID_( firstMsgID ),
	lastMsgID_( lastMsgID )
{
}


// -----------------------------------------------------------------------------
// Section: ExposedMethodMessageRange
// -----------------------------------------------------------------------------

/**
 *	This method is used to retrieve the message id and potentially sub-id from
 *	an exposed method index.
 */
void ExposedMethodMessageRange::msgIDFromExposedID( int exposedID,
		int numExposed, int16 & exposedMsgID, int16 & exposedSubMsgID ) const
{
	const int numRemainingSlots = this->numRemainingSlots( numExposed );

	if (exposedID < numRemainingSlots)
	{
		exposedMsgID = int16( firstMsgID_ + exposedID );
		exposedSubMsgID = -1;
	}
	else
	{
		const int overflow = exposedID - numRemainingSlots;
		const int firstOverflowSlot = firstMsgID_ + numRemainingSlots;

		exposedMsgID = firstOverflowSlot + overflow/256;
		exposedSubMsgID = overflow % 256;
	}
}


/**
 *	This method return the exposed id for a given message id. An extra byte may
 *	need to be read from the stream if the number of exposed methods exceeds
 *	the range's size.
 */
int ExposedMethodMessageRange::exposedIDFromMsgID( Mercury::MessageID msgID,
		BinaryIStream & data, int numExposed ) const
{
	const int numRemainingSlots = this->numRemainingSlots( numExposed );

	const int offset = msgID - firstMsgID_;

	if (offset < numRemainingSlots)
	{
		return offset;
	}
	else
	{
		uint8 subIndex;
		data >> subIndex;

		return numRemainingSlots + 256*(offset - numRemainingSlots) + subIndex;
	}
}


/**
 *	This method returns whether an exposed method id would need a sub-message id
 *	when sending between client and server.
 */
bool ExposedMethodMessageRange::needsSubID( int exposedID,
		int numExposed ) const
{
	return exposedID >= this->numRemainingSlots( numExposed );
}


/**
 *	This method returns the number to top-level slots (message ids) remaining if
 *	this range needs to handle numExposed different method ids.
 */
int ExposedMethodMessageRange::numRemainingSlots( int numExposed ) const
{
	return this->numSlots() - this->numSubSlots( numExposed );
}


/**
 *	This method returns the number of message ids that are allocated to overflow
 *	indexes if this range needs to handle numExposed different method ids.
 */
int ExposedMethodMessageRange::numSubSlots( int numExposed ) const
{
	int numExcess = numExposed - this->numSlots();

	return (numExcess > 0) ? (numExcess / 255) + 1 : 0;
}

BW_END_NAMESPACE

// exposed_message_range.cpp
