#include "pch.hpp"

#include "space_interfaces.hpp"

namespace BW
{
namespace SpaceInterfaces
{

bool PortalSceneView::setClosestPortalState( const Vector3 & point,
	bool isPermissive, CollisionFlags collisionFlags )
{
	for ( ProviderCollection::iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ProviderInterface* pProvider = *iter;

		if (pProvider->setClosestPortalState( 
			point, isPermissive, collisionFlags))
		{
			return true;
		}
	}

	// We didn't managed to succeed in fulfilling the request.
	// But we shouldn't allow the command to be requeued if there
	// is no hope of the request being fulfilled in the future.
	// So if there are no providers, then count this as a success.
	return providers_.size() == 0;
};

} // namespace SpaceInterfaces
} // namespace BW