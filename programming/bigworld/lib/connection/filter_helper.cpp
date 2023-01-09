#include "pch.hpp"

#include "filter_helper.hpp"

#include "filter_environment.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
FilterHelper::FilterHelper( FilterEnvironment & environment ):
		pEnvironment_( &environment )
{}


bool FilterHelper::filterDropPoint( SpaceID spaceID,
		const Position3D & fall, Position3D & land, Vector3 * pGroundNormal )
{
	return pEnvironment_->filterDropPoint( spaceID, fall, land,
		pGroundNormal );
}


bool FilterHelper::resolveOnGroundPosition( Position3D & position,
		bool & isOnGround )
{
	return pEnvironment_->resolveOnGroundPosition( position, isOnGround );
}


void FilterHelper::transformIntoCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction )
{
	pEnvironment_->transformIntoCommon( spaceID, vehicleID, position,
		direction );
}


void FilterHelper::transformFromCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction )
{
	pEnvironment_->transformFromCommon( spaceID, vehicleID, position,
		direction );
}

BW_END_NAMESPACE

// filter_helper.cpp

