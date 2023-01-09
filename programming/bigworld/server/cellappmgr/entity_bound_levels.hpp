#ifndef ENTITY_BOUND_LEVELS_HPP_
#define ENTITY_BOUND_LEVELS_HPP_

#include "cstdmf/bw_vector.hpp"

#include "math/math_extra.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class maintains entity bound levels data
 * received from CellApps. It's supposed to be a property of
 * internal BSP nodes and leaves.
 */
class EntityBoundLevels
{
public:
	EntityBoundLevels( int numLevels ) :
		entityBounds_( numLevels ),
		entityLoads_( numLevels )
	{
	}

	// Data manipulation
	void updateFromStream( BinaryIStream & data );

	void addToStream( BinaryOStream & stream ) const;

	void merge( const EntityBoundLevels & left,
				const EntityBoundLevels & right,
				bool isHorizontal, float position );


	// Calculations
	float entityBoundForLoadDiff( float loadDiff,
								bool isHorizontal,
								bool isMax,
								float defaultPosition ) const;

private:
	// Accessors
	const BW::Rect & entityBounds( int level = 0 ) const;
	const BW::Rect & entityLoads( int level = 0 ) const;

	// Helpers
	void takeSingleBranch(
			const EntityBoundLevels & single,
			bool isY, bool isMax, float partitionPosition );

	void mergeTwoBranches(
			const EntityBoundLevels & left,
			const EntityBoundLevels & right,
			bool isY, bool isMax );

	typedef BW::vector< BW::Rect > Rects;

	Rects    entityBounds_;
	// entityLoads_ isn't used as an array of actual rectangles
	// 'left' loads may be greater than a 'right' ones and vice versa
	Rects    entityLoads_;
};

BW_END_NAMESPACE

#endif /* ENTITY_BOUND_LEVELS_HPP_ */
