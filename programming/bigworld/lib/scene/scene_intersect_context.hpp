#ifndef SCENE_INTERSECT_CONTEXT_HPP
#define SCENE_INTERSECT_CONTEXT_HPP

#include "cstdmf/stdmf.hpp"

namespace BW
{

class Scene;

class SceneIntersectContext
{
public:
	SceneIntersectContext( const Scene & scene );

	const Scene & scene() const { return scene_; }
	
	bool onlyShadowCasters() const;
	void onlyShadowCasters( bool include );
	bool includeDynamicObjects() const;
	void includeDynamicObjects( bool include );
	bool includeStaticObjects() const;
	void includeStaticObjects( bool include );

private:

	enum Flags
	{
		INCLUDE_DYNAMIC_OBJECTS = 1 << 0,
		INCLUDE_STATIC_OBJECTS = 1 << 1,
		ONLY_SHADOW_CASTERS = 1 << 2
	};

	uint32 flags_;
	const Scene & scene_;
};

} // namespace BW

#endif // SCENE_INTERSECT_CONTEXT_HPP
