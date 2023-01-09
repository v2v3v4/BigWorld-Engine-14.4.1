#ifndef PORTAL_STATE_CHANGER_HPP
#define PORTAL_STATE_CHANGER_HPP

#include "physics2/worldtri.hpp"


BW_BEGIN_NAMESPACE

class Entity;

namespace PortalStateChanger
{
	void changePortalCollisionState(
		Entity * pEntity, bool isPermissive, WorldTriangle::Flags flags );
	void tick( float dTime );
}

BW_END_NAMESPACE

#endif // PORTAL_STATE_CHANGER_HPP
