#ifndef RANGE_TRIGGER_HPP
#define RANGE_TRIGGER_HPP

#include "entity_range_list_node.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used for range triggers (traps). Its position is the same as
 *	the entity's position plus a range. Once another entity crosses the node, it
 *	will either trigger or untrigger it and it will notify its owner entity.
 */
class RangeTriggerNode : public RangeListNode
{
public:
	RangeTriggerNode( RangeTrigger * pRangeTrigger,
		float range,
		RangeListFlags wantsFlags,
		RangeListFlags makesFlags );

	float x() const;
	float z() const;

	float oldX() const;
	float oldZ() const;

	virtual BW::string debugString() const;

	float range() const			{ return range_; }
	void range( float r )		{ range_ = r; }		// don't call this
	void setRange( float range );					// call this instead

	float oldRange() const		{ return oldRange_; }
	void oldRange( float r )	{ oldRange_ = r; }
	void updateOldRange()		{ oldRange_ = range_; }

	virtual void crossedX( RangeListNode * node, bool positiveCrossing,
		float oldOthX, float oldOthZ );
	virtual void crossedZ( RangeListNode * node, bool positiveCrossing,
		float oldOthX, float oldOthZ );

	const RangeTrigger & rangeTrigger() const	{ return *pRange_; }

protected:
	void crossedXEntity( RangeListNode * node, bool positiveCrossing,
		float oldOthX, float oldOthZ );
	void crossedZEntity( RangeListNode * node, bool positiveCrossing,
		float oldOthX, float oldOthZ );

	void crossedXEntityRange( RangeListNode * node, bool positiveCrossing );
	void crossedZEntityRange( RangeListNode * node, bool positiveCrossing );

	RangeTrigger * pRange_;
	float range_;
	float oldRange_;
};


/**
 *	This class encapsulates a full range trigger. It contains a upper and lower
 *	bound trigger node.
 */
class RangeTrigger
{
public:
	RangeTrigger( RangeListNode * pCentralNode, float range,
			RangeListNode::RangeListFlags wantsFlagsLower,
			RangeListNode::RangeListFlags wantsFlagsUpper,
			RangeListNode::RangeListFlags makesFlagsLower,
			RangeListNode::RangeListFlags makesFlagsUpper );
	virtual ~RangeTrigger();

	void insert();
	void remove();
	void removeWithoutContracting();

	void shuffleXThenZ( float oldX, float oldZ );
	void shuffleXThenZExpand( bool xInc, bool zInc, float oldX, float oldZ );
	void shuffleXThenZContract( bool xInc, bool zInc, float oldX, float oldZ );

	void setRange( float range );

	virtual BW::string debugString() const;

	virtual void triggerEnter( Entity & entity ) = 0;
	virtual void triggerLeave( Entity & entity ) = 0;
	virtual Entity * pEntity() const = 0;

	bool contains( RangeListNode * pQuery ) const;
	bool containsInZ( RangeListNode * pQuery ) const;

	RangeListNode * pCentralNode() const		{ return pCentralNode_; }

	const RangeTriggerNode * pUpperTrigger() const		{ return &upperBound_; }
	const RangeTriggerNode * pLowerTrigger() const		{ return &lowerBound_; }

	bool wasInXRange( float x, float range ) const
	{
		float subX = oldEntityX_;
		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subX - range;
		volatile float upperBound = subX + range;
		return (lowerBound < x) && (x <= upperBound);
	}

	bool isInXRange( float x, float range ) const
	{
		float subX = pCentralNode_->x();

		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subX - range;
		volatile float upperBound = subX + range;

		return (lowerBound < x) && (x <= upperBound);
	}

	bool wasInZRange( float z, float range ) const
	{
		float subZ = oldEntityZ_;

		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subZ - range;
		volatile float upperBound = subZ + range;

		return (lowerBound < z) && (z <= upperBound);
	}

	bool isInZRange( float z, float range ) const
	{
		float subZ = pCentralNode_->z();

		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subZ - range;
		volatile float upperBound = subZ + range;

		return (lowerBound < z) && (z <= upperBound);
	}

	// ----- Range -----

	bool wasInXRange( float entityLower, float entityUpper, float range ) const
	{
		float subX = oldEntityX_;

		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subX - range;
		volatile float upperBound = subX + range;

		return (lowerBound <= entityUpper) && (entityLower <= upperBound);
	}

	bool isInXRange( float entityLower, float entityUpper, float range ) const
	{
		float subX = pCentralNode_->x();

		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subX - range;
		volatile float upperBound = subX + range;

		return (lowerBound <= entityUpper) && (entityLower <= upperBound);
	}

	bool wasInZRange( float entityLower, float entityUpper, float range ) const
	{
		float subZ = oldEntityZ_;
		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subZ - range;
		volatile float upperBound = subZ + range;

		return (lowerBound <= entityUpper) && (entityLower <= upperBound);
	}

	bool isInZRange( float entityLower, float entityUpper, float range ) const
	{
		float subZ = pCentralNode_->z();

		// These volatile values are important. If this is not done, the
		// calculation may be done with greater precision than what was used in
		// calculating the nodes position in the range list. This may cause
		// some triggers to be missed or to occur when they should not have.
		volatile float lowerBound = subZ - range;
		volatile float upperBound = subZ + range;

		return (lowerBound <= entityUpper) && (entityLower <= upperBound);
	}

	float range() const	{ return upperBound_.range(); }

	bool contains( float x, float z ) const
	{
		float range = this->range();

		return this->isInXRange( x, range ) &&
			this->isInZRange( z, range );
	}

// protected:
public:
	RangeListNode *			pCentralNode_;

	RangeTriggerNode		upperBound_;
	RangeTriggerNode		lowerBound_;

	// Old location of the entity
	float				oldEntityX_;
	float				oldEntityZ_;
};

BW_END_NAMESPACE

#endif // RANGE_TRIGGER_HPP
