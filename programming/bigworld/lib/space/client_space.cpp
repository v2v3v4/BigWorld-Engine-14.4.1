#include "pch.hpp"

#include "client_space.hpp"
#include "space_manager.hpp"

#include <scene/scene.hpp>

// TODO: Remove dependency on environment
#include "deprecated_space_helpers.hpp"
#include <romp/enviro_minder.hpp>
#include <chunk/chunk_obstacle.hpp>

// TODO: Remove this dependency
// (By moving SpaceDataMapping to Space?)
#include <network/space_data_mapping.hpp>

#include <resmgr/datasection.hpp>


namespace BW
{

ClientSpace::ClientSpace( SpaceID id ) :
	environment_( NULL ),
	id_( id ),
	pScene_( new Scene() ),
	pSpaceData_( new SpaceDataMapping() ),
	isCleared_( false )
{
}


ClientSpace::~ClientSpace()
{
	SpaceManager::instance().delSpace( this );

	bw_safe_delete( pSpaceData_ );
	bw_safe_delete( pScene_ );
}


SpaceID ClientSpace::id() const
{
	return id_;
}


const Scene & ClientSpace::scene() const
{
	return *pScene_;
}


Scene & ClientSpace::scene()
{
	return *pScene_;
}


const SpaceDataMapping& ClientSpace::spaceData() const
{
	return *pSpaceData_;
}


SpaceDataMapping& ClientSpace::spaceData()
{
	return *pSpaceData_;
}


EnviroMinder & ClientSpace::enviro()
{
	return doEnviro();
}

AABB ClientSpace::bounds() const
{
	return doGetBounds();
}

Vector3 ClientSpace::clampToBounds( const Vector3& position ) const
{
	return doClampToBounds( position );
}

float ClientSpace::collide( const Vector3 & start, const Vector3 & end,
	CollisionCallback & cc ) const
{
	return doCollide( start, end, cc );
}


float ClientSpace::collide( const WorldTriangle & triangle, 
	const Vector3 & translation,
	CollisionCallback & cc ) const
{
	return doCollide( triangle, translation, cc );
}


/**
 *	This method performs a collision test down the Y axis to find the nearest
 *	possible location. This is intended primarily for use with dropping
 *	navigation onto the ground.
 */
Vector3 ClientSpace::findDropPoint( const Vector3 & position ) const
{
	const float COLLIDE_Y_POSITION_FUDGE = 0.1f;
	const float COLLIDE_MIN_HEIGHT = -13000.f;

	Vector3 newPosition = position;

	// Raise the entity position slightly in case they are right on the ground.
	newPosition.y += COLLIDE_Y_POSITION_FUDGE;

	if (newPosition.y < COLLIDE_MIN_HEIGHT)
	{
		return position;
	}

	float distance = collide( newPosition,
		Vector3( newPosition.x, COLLIDE_MIN_HEIGHT, newPosition.z ),
		ClosestObstacle::s_default );

	if (distance <= 0.f)
	{
		return position;
	}

	return newPosition - Vector3( 0.f, distance, 0.f );
}


float ClientSpace::findTerrainHeight( const Vector3 & position ) const
{
	return doFindTerrainHeight( position );
}


void ClientSpace::clear()
{
	// Clearing the space will generally involve
	// destroying a lot of references to this space
	// maintain a smart pointer to this space while
	// that happens.
	ClientSpacePtr pSpace = this;

	isCleared_ = true;
	doClear();

	if (this == DeprecatedSpaceHelpers::cameraSpace())
	{
		DeprecatedSpaceHelpers::cameraSpace( NULL );
	}
}


bool ClientSpace::cleared() const
{
	return isCleared_;
}


void ClientSpace::tick( float dTime )
{
	doTick( dTime );
}


void ClientSpace::updateAnimations( float dTime )
{
	doUpdateAnimations( dTime );
}


bool ClientSpace::addMapping( GeometryMappingID mappingID,
	Matrix& transform, const BW::string & path, 
	const SmartPointer< DataSection >& pSettings)
{
	return doAddMapping( mappingID, transform, path, pSettings );
}


void ClientSpace::delMapping( GeometryMappingID mappingID )
{
	doDelMapping( mappingID );
}


float ClientSpace::loadStatus( float distance ) const
{
	return doGetLoadStatus( distance );
}


const Moo::DirectionalLightPtr & ClientSpace::sunLight() const
{
	return doGetSunLight();
}


const Moo::Colour & ClientSpace::ambientLight() const
{
	return doGetAmbientLight();
}


const Moo::LightContainerPtr & ClientSpace::lights() const
{
	return doGetLights();
}


bool ClientSpace::lightsInArea( const AABB & worldBB, 
	Moo::LightContainerPtr* allLights )
{
	return doGetLightsInArea( worldBB, allLights );
}

} // namespace BW