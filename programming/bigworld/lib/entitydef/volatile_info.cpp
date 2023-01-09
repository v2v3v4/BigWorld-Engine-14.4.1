#include "pch.hpp"

#include "volatile_info.hpp"

#include <float.h>


BW_BEGIN_NAMESPACE

const float VolatileInfo::ALWAYS = FLT_MAX;

#ifndef CODE_INLINE
#include "volatile_info.ipp"
#endif

/**
 *	Constructor.
 */
VolatileInfo::VolatileInfo( float positionPriority, float yawPriority, 
		float pitchPriority, float rollPriority ) :
	positionPriority_( positionPriority ),
	yawPriority_( yawPriority ),
	pitchPriority_( pitchPriority ),
	rollPriority_( rollPriority )
{
}


/**
 *	This method sets up the info from a data section.
 */
bool VolatileInfo::parse( DataSectionPtr pSection )
{
	if (!pSection)
	{
		// If there is no "Volatile" section, it is assumed to have no volatile
		// information (set in the constructor) or the parent's info.
		return true;
	}

	positionPriority_ = this->asPriority( pSection->openSection( "position" ) );
	yawPriority_ =   this->asPriority( pSection->openSection( "yaw" ) );
	pitchPriority_ = this->asPriority( pSection->openSection( "pitch" ) );
	rollPriority_ =  this->asPriority( pSection->openSection( "roll" ) );

	if (!this->isValid())
	{
		const char * conditionStr =
			(yawPriority_ < pitchPriority_) ?  "yaw < pitch" : "pitch < roll";

		ERROR_MSG( "VolatileInfo::parse: Invalid direction priorities (%s). "
					"The values for yaw, pitch and roll must be descending.\n",
				conditionStr );
		return false;
	}

	return true;
}


/**
 *	This method converts a data section to a priority.
 */
float VolatileInfo::asPriority( DataSectionPtr pSection ) const
{
	if (pSection)
	{
		float value = pSection->asFloat( -1.f );
		return isEqual( value, -1.f ) ? ALWAYS : value * value;
	}

	return -1.f;
}


/**
 *	This method returns whether a detailed position needs to be sent when the
 *	volatile info changes.
 */
bool VolatileInfo::isLessVolatileThan( const VolatileInfo & info ) const
{
	return
		positionPriority_ < info.positionPriority_ ||
		yawPriority_ < info.yawPriority_ ||
		pitchPriority_ < info.pitchPriority_ ||
		rollPriority_ < info.rollPriority_;
}


/**
 *	This method returns whether or not this info is valid.
 */
bool VolatileInfo::isValid() const
{
	return yawPriority_ >= pitchPriority_ &&
		pitchPriority_ >= rollPriority_;
}

BW_END_NAMESPACE

// volatile_info.cpp

