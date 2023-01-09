#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class AABB;
class WorldTriangle;
class WorldPolygon;
class Vector3;

namespace DebugDraw
{
void enabled( bool enabled );
bool enabled();
void triAdd( const WorldTriangle & wt, uint32 col );
void arrowAdd( const Vector3 & start, const Vector3 & end, uint32 col );
void polyAdd( const WorldPolygon & wp, uint32 col );
void lineAdd( const Vector3 & p1, const Vector3 & p2, uint32 col );
void bboxAdd( const AABB & bbox, uint32 col );
void draw();
} // namespace DebugDraw

BW_END_NAMESPACE

#endif // DEBUG_DRAW_HPP
