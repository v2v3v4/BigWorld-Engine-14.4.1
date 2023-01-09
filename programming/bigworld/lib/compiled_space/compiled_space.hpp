#ifndef COMPILED_SPACE_HPP
#define COMPILED_SPACE_HPP

#include "compiled_space/forward_declarations.hpp"
#include "resmgr/datasection.hpp"

#include "cached_semi_dynamic_shadow_scene_provider.hpp"

#include "space/client_space.hpp"
#include "space/space_interfaces.hpp"
#include "space/dynamic_scene_provider.hpp"
#include "space/dynamic_light_scene_provider.hpp"
#include "space/enviro_minder_scene_provider.hpp"

#include "moo/directional_light.hpp"
#include "moo/moo_math.hpp"

#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE

class AssetClient;
class OutsideLighting;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class CompiledSpaceMapping;
class IEntityUDOFactory;

class COMPILED_SPACE_API CompiledSpace : public ClientSpace
{
public:
	static bool init( const DataSectionPtr& pConfigDS,
		IEntityUDOFactory* pEntityFactory,
		AssetClient* pAssetClient );

	static IEntityUDOFactory* entityUDOFactory();

	typedef CompiledSpaceMapping* (*MappingCreator)( 
		const BW::string& path,
		const Matrix& transform,
		const DataSectionPtr& pSettings,
		CompiledSpace& space,
		AssetClient* pAssetClient );

public:
	CompiledSpace( SpaceID id, AssetClient* pAssetClient );
	virtual ~CompiledSpace();
	
	DynamicSceneProvider& dynamicScene();
	DynamicLightSceneProvider& dynamicLightScene();

	void mappingCreator( MappingCreator func );

private:
	virtual bool doAddMapping( GeometryMappingID mappingID,
		Matrix& transform, const BW::string & path, 
		const SmartPointer< DataSection >& pSettings );
	virtual void doDelMapping( GeometryMappingID mappingID );
	
	virtual float doGetLoadStatus( float /* distance */ ) const;

	// Space interface
	virtual void doClear();
	virtual EnviroMinder & doEnviro();
	virtual void doTick( float dTime );
	virtual void doUpdateAnimations( float dTime );

	// Collision Scene View Interface
	virtual AABB doGetBounds() const;
	virtual Vector3 doClampToBounds( const Vector3& position ) const;
	virtual float doCollide( const Vector3 & start, const Vector3 & end,
		CollisionCallback & cc ) const;
	virtual float doCollide( const WorldTriangle & triangle, 
		const Vector3 & translation, 
		CollisionCallback & cc = CollisionCallback_s_default ) const;
	virtual float doFindTerrainHeight( const Vector3 & position ) const;

	// Light scene view interface
	virtual const Moo::DirectionalLightPtr & doGetSunLight() const;
	virtual const Moo::Colour & doGetAmbientLight() const;
	virtual const Moo::LightContainerPtr & doGetLights() const;
	virtual bool doGetLightsInArea( const AABB& worldBB, 
		Moo::LightContainerPtr* allLights );

private:
	size_t mappingIDToIndex( GeometryMappingID id );

	friend class CompiledSpaceMapping;
	void mappingLoadedBG( CompiledSpaceMapping* pLoader );
	void mappingLoaded( CompiledSpaceMapping* pLoader );
	void mappingUnloaded( CompiledSpaceMapping* pLoader );

	void updateSpaceBounds();

protected:
	AssetClient * pAssetClient_;

	struct Mapping
	{
		GeometryMappingID mappingID_;
		CompiledSpaceMapping* pLoader_;
	};

	typedef BW::vector<Mapping> Mappings;
	Mappings mappings_;
	MappingCreator mappingCreator_;

	AABB bounds_;

	DynamicSceneProvider dynamicScene_;
	DynamicLightSceneProvider dynamicLightScene_;
	EnviroMinderSceneProvider enviroMinderScene_;
	CachedSemiDynamicShadowSceneProvider shadowScene_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_HPP
