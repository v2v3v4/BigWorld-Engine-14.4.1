#ifndef IOSCLIENT_FILTER_ENVIRONMENT_HPP
#define IOSCLIENT_FILTER_ENVIRONMENT_HPP

#include "connection/filter_environment.hpp"

namespace BW
{
	class BWEntities;
}


/**
 *	This class implements the filter environment for the iOS client.
 */ 
class IOSClientFilterEnvironment : public BW::FilterEnvironment
{
public:
	IOSClientFilterEnvironment( const BW::BWEntities & entities );
	virtual ~IOSClientFilterEnvironment();
	
	// Overrides from FilterEnvironment
	virtual bool filterDropPoint( BW::SpaceID spaceID,
		const BW::Position3D & fall, BW::Position3D & land,
		BW::Vector3 * pGroundNormal = NULL );

	virtual bool resolveOnGroundPosition( BW::Position3D & position, 
		bool & isOnGround );
	
	virtual void transformIntoCommon( BW::SpaceID spaceID, BW::EntityID vehicleID,
		BW::Position3D & position, BW::Direction3D & direction );
	
	virtual void transformFromCommon( BW::SpaceID spaceID, BW::EntityID vehicleID,
		BW::Position3D & position, BW::Direction3D & direction );

private:
	const BW::BWEntities & entities_;
};



#endif // IOSCLIENT_FILTER_ENVIRONMENT_HPP
