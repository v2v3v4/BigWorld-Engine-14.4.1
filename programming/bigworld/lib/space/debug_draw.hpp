#ifndef SPACE_DEBUG_DRAW_HPP
#define SPACE_DEBUG_DRAW_HPP

#include "forward_declarations.hpp"

#define ENABLE_SPACE_DEBUG_DRAW (ENABLE_WATCHERS)

namespace BW
{

/*
	A namespace to store debug draw helpers for space systems.
*/

namespace SpaceDebugDraw
{
	void drawBoundingBox( const AABB & box, uint32 colour );
	void drawBoundingBoxes( AABB* pBoxes, size_t numBoxes, 
		uint32 colour );
	
	class OctreeDrawVisitor
	{
	public:
		OctreeDrawVisitor( size_t level, uint32 colour );

		bool visit( size_t index, AABB & bb, size_t level, 
			uint32 dataReference );

	private:
		size_t level_;
		uint32 colour_;
	};

	template <typename OctreeType>
	void drawOctreeBoundingBoxes( OctreeType & tree, size_t level, 
		uint32 colour)
	{
		OctreeDrawVisitor visitor( level, colour );
		tree.traverse( visitor );
	}

}

} // namespace BW

#endif // SPACE_DEBUG_DRAW_HPP