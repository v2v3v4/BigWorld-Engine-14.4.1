#import "IOSClientFilterEnvironment.h"


#include "connection_model/bw_entities.hpp"


/**
 *	Constructor.
 *
 *	@param entities		The entities collection.
 */
IOSClientFilterEnvironment::IOSClientFilterEnvironment(
			const BW::BWEntities & entities ) :
		entities_( entities )
{
}


/**
 *	Destructor.
 */
IOSClientFilterEnvironment::~IOSClientFilterEnvironment()
{
}


/*
 *	Override from FilterEnvironment.
 */
bool IOSClientFilterEnvironment::filterDropPoint(
		BW::SpaceID spaceID,
		const BW::Position3D & fall, BW::Position3D & land, BW::Vector3 * pGroundNormal )
{
	return true;
}


/*
 *	Override from FilterEnvironment.
 */bool IOSClientFilterEnvironment::resolveOnGroundPosition(
		BW::Position3D & position, bool & isOnGround )
{
	return true;
}


/*
 *	Override from FilterEnvironment.
 */
void IOSClientFilterEnvironment::transformIntoCommon( BW::SpaceID spaceID,
		 BW::EntityID vehicleID, BW::Position3D & position,
		 BW::Direction3D & direction )
{
	if (vehicleID != 0)
	{
		BW::BWEntity * pVehicle = entities_.find( vehicleID );
		if (pVehicle != NULL)
		{
			pVehicle->transformVehicleToCommon( position, direction );
		}
	}
}


/*
 *	Override from FilterEnvironment.
 */
void IOSClientFilterEnvironment::transformFromCommon( BW::SpaceID spaceID,
		BW::EntityID vehicleID, BW::Position3D & position,
		BW::Direction3D & direction )
{
	if (vehicleID != 0)
	{
		BW::BWEntity * pVehicle = entities_.find( vehicleID );
		if (pVehicle != NULL)
		{
			pVehicle->transformCommonToVehicle( position, direction );
		}
	}
}


// IOSClientFilterEnvironment.cpp
