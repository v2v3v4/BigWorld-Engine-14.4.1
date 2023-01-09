
#include "entity_bound_levels.hpp"

#include "server/balance_config.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method returns the entity bounds associated with a given level. If no
 *	level is passed, the maximum offload level is used.
 */
const BW::Rect & EntityBoundLevels::entityBounds( int level ) const
{
	return entityBounds_[ Math::clamp(
			0, level, (int)entityBounds_.size() - 1 ) ];
}


/**
 *	This method returns the load limits associated with a given level. If no
 *	level is passed, the maximum offload level is used.
 */
const BW::Rect & EntityBoundLevels::entityLoads( int level ) const
{
	return entityLoads_[ Math::clamp(
			0, level, (int)entityLoads_.size() - 1 ) ];
}


/**
 * This method merges bound levels data from left and right nodes
 * into this one
 */
void EntityBoundLevels::merge( const EntityBoundLevels & left,
		const EntityBoundLevels & right,
		bool isHorizontal, float partitionPosition )
{
	// Is the partition line horizontal?
	if (isHorizontal)
	{
		this->mergeTwoBranches( left, right, false, false );
		this->mergeTwoBranches( left, right, false, true );

		this->takeSingleBranch( left, true, false, partitionPosition );
		this->takeSingleBranch( right, true, true, partitionPosition );
	}
	else
	{
		this->mergeTwoBranches( left, right, true, false );
		this->mergeTwoBranches( left, right, true, true );

		this->takeSingleBranch( left, false, false, partitionPosition );
		this->takeSingleBranch( right, false, true, partitionPosition );
	}
}


/**
 * This method simply takes the data from the given branch
 */
void EntityBoundLevels::takeSingleBranch(
		const EntityBoundLevels & single,
		bool isY, bool isMax, float partitionPosition )
{
	MF_ASSERT( entityBounds_.size() == entityLoads_.size() );

	for (size_t level = 0; level < entityBounds_.size(); ++level)
	{
		float boundToTake =
				single.entityBounds( level ).range1D( isY )[ isMax ];

		// We don't let it go beyond the current partition line
		boundToTake = isMax ? std::max( partitionPosition, boundToTake ) :
							std::min( partitionPosition, boundToTake );

		entityBounds_[ level ].range1D( isY )[ isMax ] = boundToTake;

		entityLoads_[ level ].range1D( isY )[ isMax ] =
				single.entityLoads( level ).range1D( isY )[ isMax ];
	}
}


/**
 * This method merges bounds and loads from two branches
 */
void EntityBoundLevels::mergeTwoBranches(
		const EntityBoundLevels & left,
		const EntityBoundLevels & right,
		bool isY, bool isMax )
{
	MF_ASSERT( entityBounds_.size() == entityLoads_.size() );

	const int mergedArraySize = entityBounds_.size();

	float bounds[ mergedArraySize ];
	float loads[ mergedArraySize ];
	int currWrittenLevel = 0;

	int leftCurrLevel = left.entityBounds_.size() - 1;
	int rightCurrLevel = right.entityBounds_.size() - 1;

	const float OUT_OF_BOUNDS_LIMIT = isMax ?
			-std::numeric_limits<float>().max() :
			std::numeric_limits<float>().max();

	while ((leftCurrLevel >= 0 || rightCurrLevel >= 0) &&
			currWrittenLevel < mergedArraySize)
	{
		const float leftCurrBound = leftCurrLevel >= 0 ?
				left.entityBounds( leftCurrLevel ).range1D( isY )[ isMax ] :
				OUT_OF_BOUNDS_LIMIT;
		const float rightCurrBound = rightCurrLevel >= 0 ?
				right.entityBounds( rightCurrLevel ).range1D( isY )[ isMax ] :
				OUT_OF_BOUNDS_LIMIT;

		const float leftCurrLoad = leftCurrLevel >= 0 ?
				left.entityLoads( leftCurrLevel ).range1D( isY )[ isMax ] :
				0.f;
		const float rightCurrLoad = rightCurrLevel >= 0 ?
				right.entityLoads( rightCurrLevel ).range1D( isY )[ isMax ] :
				0.f;

		const bool needTakeLeft = isMax ? leftCurrBound > rightCurrBound :
										leftCurrBound < rightCurrBound;

		const float boundToTake = needTakeLeft ? leftCurrBound : rightCurrBound;
		const float loadToTake = std::max( leftCurrLoad, rightCurrLoad );

		const float minCalculatedLoad = BalanceConfig::cpuOffloadForLevel(
				mergedArraySize - currWrittenLevel - 1 );


		if (needTakeLeft)
		{
			if (leftCurrLevel < 0)
			{
				// Means we're not going to progress any further
				break;
			}
			leftCurrLevel--;
		}
		else
		{
			if (rightCurrLevel < 0)
			{
				// Means we're not going to progress any further
				break;
			}
			rightCurrLevel--;
		}

		if (currWrittenLevel == 0 ||
				((loads[ currWrittenLevel - 1 ] < loadToTake) &&
				(loadToTake >= minCalculatedLoad)))
		{
			bounds[ currWrittenLevel ] = boundToTake;
			loads[ currWrittenLevel ] = loadToTake;
			++currWrittenLevel;
		}
	}

	float lastBound = OUT_OF_BOUNDS_LIMIT;
	float lastLoad = 0.f;

	for (int level = 0; level < (int)entityBounds_.size(); ++level)
	{
		if (level < currWrittenLevel)
		{
			lastBound = bounds[ level ];
			lastLoad = loads[ level ];
		}

		const int reversedLevel = (int)entityBounds_.size() - level - 1;
		entityBounds_[ reversedLevel ].range1D( isY )[ isMax ] = lastBound;
		entityLoads_[ reversedLevel ].range1D( isY )[ isMax ] = lastLoad;
	}
}


/**
 *	This method reads in the entity bounds from a stream.
 */
void EntityBoundLevels::updateFromStream( BinaryIStream & data )
{
	// This needs to match CellApp's Space::writeEntityBounds.
	for (int isMax = 0; isMax <= 1; ++isMax)
	{
		for (int isY = 0; isY <= 1; ++isY)
		{
			for (int level = entityBounds_.size() - 1; level >= 0; --level )
			{
				float pos;
				float load;
				data >> pos >> load;

				entityBounds_[ level ].range1D( isY )[ isMax ] = pos;
				entityLoads_[ level ].range1D( isY )[ isMax ] = load;
			}
		}
	}
}


/**
 * This method streams in the required portion of data
 */
void EntityBoundLevels::addToStream( BinaryOStream & stream ) const
{
	stream << entityBounds_;
}


/**
 *	This method returns the entity bound best suited to offload a desired
 *	amount. The largest level less than this load amount is chosen.
 */
float EntityBoundLevels::entityBoundForLoadDiff( float loadDiff,
		bool isHorizontal, bool isMax, float defaultPosition ) const
{
	const float absLoadDiff = fabs( loadDiff );
	for (int level = 0;
			level < (int)entityLoads_.size();
			++level)
	{
		if (entityLoads_[ level ].range1D(
				isHorizontal )[ isMax ] <= absLoadDiff)
		{
			return entityBounds_[ level ].range1D( isHorizontal )[ isMax ];
		}
	}

	return defaultPosition;
}

BW_END_NAMESPACE


// entity_bound_levels.cpp

