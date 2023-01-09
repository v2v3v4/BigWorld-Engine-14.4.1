#ifndef EXPOSED_MESSAGE_RANGE_HPP
#define EXPOSED_MESSAGE_RANGE_HPP

#include "misc.hpp"

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to manage a range of message ids. This is used when a
 *	single handler handles a range of message ids. For example, property
 *	changes and method calls sent from the server to the client.
 */
class ExposedMessageRange
{
public:
	ExposedMessageRange( int firstMsgID, int lastMsgID );

	bool contains( Mercury::MessageID msgID ) const
	{
		return (firstMsgID_ <= msgID) && (msgID <= lastMsgID_);
	}

	int numSlots() const	{ return lastMsgID_ - firstMsgID_ + 1; }

	int simpleExposedIDFromMsgID( Mercury::MessageID msgID ) const
	{
		return msgID - firstMsgID_;
	}

protected:
	int firstMsgID_;
	int lastMsgID_;
};


/**
 *	This class is used to handle a range of message ids that are used for
 *	calling exposed methods.
 */
class ExposedMethodMessageRange : public ExposedMessageRange
{
public:
	ExposedMethodMessageRange( int firstMsgID, int lastMsgID ) :
		ExposedMessageRange( firstMsgID, lastMsgID )
	{
	}

	void msgIDFromExposedID( int exposedID, int numExposed,
			int16 & exposedMsgID, int16 & exposedSubMsgID ) const;

	int exposedIDFromMsgID( Mercury::MessageID msgID,
			BinaryIStream & data, int numExposed ) const;

	bool needsSubID( int exposedID, int numExposed ) const;

private:
	int numSubSlots( int numExposed ) const;
	int numRemainingSlots( int numExposed ) const;
};


/**
 *	This class is used to handle a range of message ids that are used for
 *	sending property updates from the server to the client.
 */
class ExposedPropertyMessageRange : public ExposedMessageRange
{
public:
	ExposedPropertyMessageRange( int firstMsgID, int lastMsgID ) :
		ExposedMessageRange( firstMsgID, lastMsgID )
	{
	}

	/**
	 *	This method return the exposed property id for a message id.
	 */
	int exposedIDFromMsgID( Mercury::MessageID msgID ) const
	{
		return this->simpleExposedIDFromMsgID( msgID );
	}

	/**
	 *	This method returns the message id assoicated with an exposed id.
	 *
	 *	@return The message id of the property change or -1 if the value does
	 *		not fit in this range.
	 */
	int16 msgIDFromExposedID( int exposedID ) const
	{
		return (exposedID < this->numSlots()) ?
			static_cast<int16>(exposedID + firstMsgID_) : -1;
	}
};

BW_END_NAMESPACE

#endif // EXPOSED_MESSAGE_RANGE_HPP
