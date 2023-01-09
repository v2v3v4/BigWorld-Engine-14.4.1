#include "pch.hpp"

#include "entity.hpp"

#include "cstdmf/bw_map.hpp"
#include "chunk/chunk_manager.hpp"
#include "scene/scene.hpp"
#include "space/client_space.hpp"
#include "space/space_interfaces.hpp"

BW_BEGIN_NAMESPACE

namespace PortalStateChanger
{

/**
 *	This class is responsible for repeatedly attempting to change the state of
 *	a portal until success.
 */
class PortalStateChange : public ReferenceCount
{
public:
	PortalStateChange( Entity * pEntity = NULL,
			bool isPermissive = true,
			WorldTriangle::Flags collisionFlags = 0 ) :
		pEntity_( pEntity ),
		isPermissive_( isPermissive ),
		collisionFlags_( collisionFlags ),
		point_( pEntity->position() )
	{
	}

	bool apply()
	{
		if (pEntity_->isDestroyed())
		{
			return true;
		}

		ClientSpacePtr pSpace = pEntity_->pSpace();
		if (pSpace == NULL)
		{
			return false;
		}

		SpaceInterfaces::PortalSceneView* pPortalScene = 
			pSpace->scene().getView<SpaceInterfaces::PortalSceneView>();

		return pPortalScene->setClosestPortalState( point_,
							isPermissive_, collisionFlags_ );
	}

private:
	EntityPtr pEntity_;
	bool isPermissive_;
	WorldTriangle::Flags collisionFlags_;
	Vector3 point_;
};

typedef SmartPointer< PortalStateChange > PortalStateChangePtr;

typedef BW::map< EntityPtr, PortalStateChangePtr > PendingChanges;
PendingChanges g_pendingChanges;


/**
 *	This method changes the state of a portal. If it does not succeed initially
 *	because the chunk is not yet loaded, it retries periodically until
 *	successful.
 */
void changePortalCollisionState(
		Entity * pEntity, bool isPermissive, WorldTriangle::Flags flags )
{
	// TODO: Should have a static method to try initially to avoid allocation
	// in the common case of initial success.
	PortalStateChangePtr pStateChange =
		new PortalStateChange( pEntity, isPermissive, flags );

	if (pStateChange->apply())
	{
		g_pendingChanges.erase( pEntity );
	}
	else
	{
		g_pendingChanges[ pEntity ] = pStateChange;
	}
}


/**
 *	This method should be called periodically to retry any outstanding portal
 *	changes.
 */
void tick( float dTime )
{
	const float CHECK_PERIOD = 1.f;

	static float remaining = CHECK_PERIOD;

	if (remaining > dTime)
	{
		remaining -= dTime;
		return;
	}

	remaining = CHECK_PERIOD;

	PendingChanges::iterator outerIter = g_pendingChanges.begin();

	while (outerIter != g_pendingChanges.end())
	{
		PendingChanges::iterator iter = outerIter;
		++outerIter;

		if (iter->second->apply())
		{
			g_pendingChanges.erase( iter );
		}
	}
}


/**
 *	This class is responsible for making sure that no references are kept to
 *	entities after script has finalised.
 */
class ScriptShutdown : public Script::FiniTimeJob
{
public:
	virtual void fini()
	{
		g_pendingChanges.clear();
	}
};

ScriptShutdown g_scriptShutdown;

} // namespace PortalStateChanger

BW_END_NAMESPACE

// portal_state_changer.cpp
