#ifndef SPACE_HPP
#define SPACE_HPP

#include "forward_declarations.hpp"
#include "space_dll.hpp"

#include "math/boundbox.hpp"
#include "moo/moo_math.hpp"

namespace BW
{

namespace Moo
{
	class LightContainer;
	typedef BW::SmartPointer< LightContainer > LightContainerPtr;

	class DirectionalLight;
	typedef BW::SmartPointer< DirectionalLight > DirectionalLightPtr;
}

class SpaceDataMapping;
class Scene;
class CollisionCallback;
extern CollisionCallback & CollisionCallback_s_default;
class EnviroMinder;
class Vector3;
class GeometryMapping;
class DataSection;
class BoundingBox;
class WorldTriangle;
class WaterSceneRenderer;

typedef SmartPointer< class ClientSpace > ClientSpacePtr;

class SPACE_API ClientSpace : public SafeReferenceCount
{
public:

	typedef uint64 GeometryMappingID;

	ClientSpace( SpaceID id );
	virtual ~ClientSpace();

	SpaceID id() const;

	const Scene & scene() const;
	Scene & scene();

	const SpaceDataMapping& spaceData() const;
	SpaceDataMapping& spaceData();

	bool addMapping( GeometryMappingID mappingID,
		Matrix& transform, const BW::string & path, 
		const SmartPointer< DataSection >& pSettings = NULL );
	void delMapping( GeometryMappingID mappingID );

	float loadStatus( float distance = -1.f ) const;

	void clear();
	bool cleared() const;

	void tick( float dTime );
	void updateAnimations( float dTime );

	EnviroMinder & enviro();

	AABB bounds() const;
	Vector3 clampToBounds( const Vector3& position ) const;

	float collide( const Vector3 & start, const Vector3 & end,
		CollisionCallback & cc = CollisionCallback_s_default ) const;
	float collide( const WorldTriangle & triangle, const Vector3 & translation,
		CollisionCallback & cc = CollisionCallback_s_default ) const;
	Vector3 findDropPoint( const Vector3 & position ) const;
	float findTerrainHeight( const Vector3 & position ) const;

	const Moo::DirectionalLightPtr & sunLight() const;
	const Moo::Colour & ambientLight() const;
	const Moo::LightContainerPtr & lights() const;

	bool lightsInArea( const AABB & worldBB, 
		Moo::LightContainerPtr* allLights );

protected:

	virtual bool doAddMapping( GeometryMappingID mappingID,
		Matrix& transform, const BW::string & path, 
		const SmartPointer< DataSection >& pSettings ) = 0;
	virtual void doDelMapping( GeometryMappingID mappingID ) = 0;
	virtual EnviroMinder & doEnviro() = 0;
	virtual float doGetLoadStatus( float distance ) const = 0;

	// Space interface
	virtual void doClear() = 0;
	virtual void doTick( float dTime ) = 0;
	virtual void doUpdateAnimations( float dTime ) = 0;

	// Collision Scene View Interface
	virtual AABB doGetBounds() const = 0;
	virtual Vector3 doClampToBounds( const Vector3& position ) const = 0;
	virtual float doCollide( const Vector3 & start, const Vector3 & end,
		CollisionCallback & cc ) const = 0;
	virtual float doCollide( const WorldTriangle & triangle, 
		const Vector3 & translation, 
		CollisionCallback & cc = CollisionCallback_s_default ) const = 0;
	virtual float doFindTerrainHeight( const Vector3 & position ) const = 0;

	// Light scene view interface
	virtual const Moo::DirectionalLightPtr & doGetSunLight() const = 0;
	virtual const Moo::Colour & doGetAmbientLight() const = 0;
	virtual const Moo::LightContainerPtr & doGetLights() const = 0;
	virtual bool doGetLightsInArea( const AABB& worldBB, 
		Moo::LightContainerPtr* allLights ) = 0;

protected:
	EnviroMinder* environment_;

private:
	SpaceID id_;
	Scene* pScene_;
	SpaceDataMapping* pSpaceData_;
	
	bool isCleared_;
};

} // namespace BW

#endif // SPACE_HPP
