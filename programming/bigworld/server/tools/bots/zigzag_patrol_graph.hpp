#ifndef ZIGZAG_PATROL_GRAPH_HPP
#define ZIGZAG_PATROL_GRAPH_HPP

#include "patrol_graph.hpp"


BW_BEGIN_NAMESPACE

using namespace Patrol;

namespace ZigzagPatrol
{
	// This could probably derive from GraphTraverser itself, but I want to do
	// it as a standalone class as a learning experience
	class ZigzagGraphTraverser : public GraphTraverser
	{
	public:
		static const int DEFAULT_CORRIDOR_WIDTH;

		ZigzagGraphTraverser (const Graph &graph, float & speed,
							  Vector3 &startPosition,
							  float corridorWidth = DEFAULT_CORRIDOR_WIDTH);
		virtual bool nextStep (float & speed,
							   float dTime,
							   Vector3 &pos,
							   Direction3D &dir);

	protected:
		void setZigzagPos (const Vector3 &currPos);

		float corridorWidth_;
		Vector3 sourcePos_;
		Vector3 zigzagPos_;
	};
};

BW_END_NAMESPACE

#endif // ZIGZAG_PATROL_GRAPH_HPP
