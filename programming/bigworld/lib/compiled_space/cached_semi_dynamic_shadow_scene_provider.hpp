#ifndef CACHED_SEMI_DYNAMIC_SHADOW_SCENE_PROVIDER_HPP
#define CACHED_SEMI_DYNAMIC_SHADOW_SCENE_PROVIDER_HPP

#include "cstdmf/lookup_table.hpp"

#include "scene/scene_provider.hpp"
#include "moo/semi_dynamic_shadow_scene_view.hpp"

namespace BW {
namespace CompiledSpace {

class StringTable;
class StaticSceneProvider;

class CachedSemiDynamicShadowSceneProvider:
	public SceneProvider,
	public Moo::ISemiDynamicShadowSceneViewProvider
{
public:
	CachedSemiDynamicShadowSceneProvider( );
	virtual ~CachedSemiDynamicShadowSceneProvider();

	void configure( const AABB& totalBounds, 
		float divisionSize );
	void configure( const Moo::IntBoundBox2& bounds, 
		float divisionSize );
	void flushCache();
	
private:
	virtual void * getView(
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

	virtual void updateDivisionConfig( Moo::IntBoundBox2& bounds, 
		float& divisionSize );
	virtual void addDivisionBounds( const Moo::IntVector2& divisionCoords, 
		AABB& divisionBounds );

	const AABB& cachedDivisionBounds( const Moo::IntVector2& divisionCoords );
	size_t calculateDivisionID( const Moo::IntVector2& divisionCoords );
	void generateDivisionBounds( const Moo::IntVector2& divisionCoords, AABB& bb );
	bool isInsideBounds( const Moo::IntVector2& divisionCoords );

	// Configuration data
	Moo::IntBoundBox2 divisionBounds_;
	float divisionSize_;

	// Cache data
	struct CacheData;
	vector<CacheData> cache_;
};

} // namespace CompiledSpace
} // namespace BW

#endif // CACHED_SEMI_DYNAMIC_SHADOW_SCENE_PROVIDER_HPP
