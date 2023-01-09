#ifndef PORTAL_OBSTACLE_HPP
#define PORTAL_OBSTACLE_HPP

#include "chunk_boundary.hpp"
#include "chunk_item.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PortalObstacle
// -----------------------------------------------------------------------------

/**
 *	This class is the obstacle that a ChunkPortal puts in the collision scene
 */
class PortalChunkItem : public ChunkItem
{
public:
	PortalChunkItem( ChunkBoundary::Portal * pPortal );

	virtual void toss( Chunk * pChunk );
	virtual void draw( Moo::DrawContext& drawContext );

	ChunkBoundary::Portal * pPortal() const		{ return pPortal_; }

private:
	ChunkBoundary::Portal * pPortal_;
};

BW_END_NAMESPACE

#endif // PORTAL_OBSTACLE_HPP
