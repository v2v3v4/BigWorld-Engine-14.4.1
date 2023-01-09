#ifndef BWCLIENT_FILTER_ENVIRONMENT_HPP
#define BWCLIENT_FILTER_ENVIRONMENT_HPP

#include "connection/filter_environment.hpp"

BW_BEGIN_NAMESPACE

class BWClientFilterEnvironment : public FilterEnvironment
{
public:
	BWClientFilterEnvironment( float maxFilterDropPointDistance = 100.f );
	virtual ~BWClientFilterEnvironment() {}


	// Overrides from FilterEnvironment
	virtual bool filterDropPoint( SpaceID spaceID,
			const Position3D & fall, Position3D & land,
		Vector3 * pGroundNormal = NULL );

	virtual bool resolveOnGroundPosition( Position3D & position,
		bool & isOnGround );

	virtual void transformIntoCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction );

	virtual void transformFromCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction );


private:
	float maxFilterDropPointDistance_;

};

BW_END_NAMESPACE

#endif // BWCLIENT_FILTER_ENVIRONMENT_HPP
