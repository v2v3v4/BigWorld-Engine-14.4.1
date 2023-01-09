#include "pch.hpp"

#include "debug_draw.hpp"
#include "math/boundbox.hpp"
#include "moo/debug_draw.hpp"

namespace BW
{

namespace SpaceDebugDraw
{

void drawBoundingBox( const AABB & box, uint32 colour )
{
	DebugDraw::bboxAdd( box, colour );
}

void drawBoundingBoxes( AABB* pBoxes, size_t numBoxes, 
	uint32 colour )
{
	for (size_t i = 0; i < numBoxes; ++i)
	{
		drawBoundingBox( pBoxes[i], colour );
	}
}

OctreeDrawVisitor::OctreeDrawVisitor( size_t level, uint32 colour ) : 
	level_( level ),
	colour_( colour )
{
}

bool OctreeDrawVisitor::visit( size_t index, AABB & bb, size_t level, 
	uint32 dataReference )
{
	if (level == level_)
	{
		drawBoundingBox( bb, colour_ );
		return false;
	}
	else
	{
		return true;
	}
}


} // namespace SpaceDebugDraw
} // namespace BW