#ifndef SPACE_INTERFACES_HPP
#define SPACE_INTERFACES_HPP

#include "forward_declarations.hpp"
#include "scene/scene_provider.hpp"

namespace BW
{

/*
	A namespace to temporarily store interfaces that need to be implemented
	in the space refactor. These interfaces break away parts of algorithms from
	the space types and implementations. 
*/

namespace SpaceInterfaces
{

class IPortalSceneViewProvider
{
public:
	typedef uint16 CollisionFlags;

	virtual bool setClosestPortalState( const Vector3 & point,
		bool isPermissive, CollisionFlags collisionFlags ) = 0;
};

class PortalSceneView : public 
	SceneView<IPortalSceneViewProvider>
{
public:
	typedef IPortalSceneViewProvider::CollisionFlags CollisionFlags;

	bool setClosestPortalState( const Vector3 & point,
		bool isPermissive, CollisionFlags collisionFlags );
};



}

} // namespace BW

#endif // SPACE_INTERFACES_HPP