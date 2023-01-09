#ifndef LOOSE_OCTREE_LOGGING_HPP
#define LOOSE_OCTREE_LOGGING_HPP

#define FORCE_ENABLE_LOOSE_OCTREE_QUERY_LOGGING 0

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class Sphere;
class BoundingBox;
class Vector3;
class ConvexHull;

namespace LooseOctreeQueryLogging
{
	enum QueryTypes
	{
		QUERY_BOX,
		QUERY_HULL,
		QUERY_RAY
	};

	void logQuery( const BW::BoundingBox& box );
	void logQuery( const BW::Vector3 & start, const BW::Vector3 & stop);
	void logQuery( const BW::ConvexHull& hull );
}

BW_END_NAMESPACE

// Define macros to enable logging queries
#define ENABLE_LOOSE_OCTREE_QUERY_LOGGING (!CONSUMER_CLIENT_BUILD && FORCE_ENABLE_LOOSE_OCTREE_QUERY_LOGGING)

#if ENABLE_LOOSE_OCTREE_QUERY_LOGGING
#define LOG_QUERY( ... ) BW::LooseOctreeQueryLogging::logQuery( __VA_ARGS__ );
#else
#define LOG_QUERY( ... ) 
#endif

#endif
