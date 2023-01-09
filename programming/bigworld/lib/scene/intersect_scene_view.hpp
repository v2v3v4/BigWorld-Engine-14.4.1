#ifndef INTERSECT_SCENE_VIEW_HPP
#define INTERSECT_SCENE_VIEW_HPP

#include "math/forward_declarations.hpp"
#include "forward_declarations.hpp"
#include "scene_provider.hpp"
#include "scene_type_system.hpp"

namespace BW
{

class ConvexHull;
class SceneIntersectContext;

class IIntersectSceneViewProvider
{
public:

	virtual ~IIntersectSceneViewProvider(){}

	virtual size_t intersect( const SceneIntersectContext& context,
		const ConvexHull& hull, 
		IntersectionSet & intersection) const = 0;
	
};


class IntersectSceneView : public 
	SceneView<IIntersectSceneViewProvider>
{
public:

	size_t intersect( const SceneIntersectContext& context,
		const ConvexHull& hull, 
		IntersectionSet & intersection) const;

	size_t cull( const SceneIntersectContext& context,
		const Matrix & worldViewProjectionMat, 
		IntersectionSet & intersection ) const;


};

} // namespace BW

#endif // INTERSECT_SCENE_VIEW_HPP
