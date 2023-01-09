#ifndef VOLATILE_INFO_HPP
#define VOLATILE_INFO_HPP

#include "bwentity/bwentity_api.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to describe what information of an entity changes
 *	frequently and should be sent frequently.
 */
class VolatileInfo
{
public:
	VolatileInfo( float positionPriority = -1.f, 
		float yawPriority = -1.f, 
		float pitchPriority = -1.f, 
		float rollPriority = -1.f );

	bool parse( DataSectionPtr pSection );

	bool shouldSendPosition() const	{ return positionPriority_ > 0.f; }
	int dirType( float priority ) const;

	bool isLessVolatileThan( const VolatileInfo & info ) const;
	bool isValid() const;
	bool hasVolatile( float priority ) const;

	// Overloaded operators that are declared outside the class:
	//bool operator==( const VolatileInfo & volatileInfo1, 
	//	const VolatileInfo & volatileInfo2 );
	//bool operator!=( const VolatileInfo & volatileInfo1, 
	//	const VolatileInfo & volatileInfo2 );

	BWENTITY_API static const float ALWAYS;

	BWENTITY_API float positionPriority() const { return positionPriority_; }
	BWENTITY_API float yawPriority() const 		{ return yawPriority_; }
	BWENTITY_API float pitchPriority() const 	{ return pitchPriority_; }
	BWENTITY_API float rollPriority() const 	{ return rollPriority_; }

private:
	float asPriority( DataSectionPtr pSection ) const;

	float	positionPriority_;
	float	yawPriority_;
	float	pitchPriority_;
	float	rollPriority_;
};


BWENTITY_API INLINE bool operator==( const VolatileInfo & volatileInfo1,
	const VolatileInfo & volatileInfo2 );

BWENTITY_API INLINE bool operator!=( const VolatileInfo & volatileInfo1,
	const VolatileInfo & volatileInfo2 );

#ifdef CODE_INLINE
#include "volatile_info.ipp"
#endif

BW_END_NAMESPACE

#endif // VOLATILE_INFO_HPP
