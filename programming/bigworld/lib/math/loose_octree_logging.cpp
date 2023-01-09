#include "pch.hpp"

#include "loose_octree_logging.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/concurrency.hpp"
#include "math/planeeq.hpp"
#include "math/convex_hull.hpp"
#include "math/sphere.hpp"
#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE

namespace LooseOctreeQueryLogging
{
	const char * g_logFileName = "octree_query_log.log";

	FILE* openLog( uint32 queryType )
	{
		FILE* file = BW::bw_fopen( g_logFileName, "a" );
		fwrite( &queryType, sizeof(uint32), 1, file );
		return file;
	}

	void logQuery( const BW::BoundingBox& box )
	{
		FILE* file = openLog( QUERY_BOX );

		BW::Vector3 min = box.minBounds();
		BW::Vector3 max = box.maxBounds();

		fwrite( &min, sizeof(Vector3), 1, file );
		fwrite( &max, sizeof(Vector3), 1, file );
		
		fclose( file );
	}

	void logQuery( const BW::Vector3 & start, const BW::Vector3 & stop)
	{
		FILE* file = openLog( QUERY_RAY );

		fwrite( &start, sizeof(Vector3), 1, file );
		fwrite( &stop, sizeof(Vector3), 1, file );

		fclose( file );
	}

	void logQuery( const BW::ConvexHull& hull )
	{
		FILE* file = openLog( QUERY_HULL );

		uint32 numPlanes = static_cast<uint32>(hull.size());
		fwrite( &numPlanes, sizeof(uint32), 1, file );

		for (size_t i = 0; i < hull.size(); ++i)
		{
			BW::PlaneEq plane = hull.plane( i );
			fwrite( &plane, sizeof(PlaneEq), 1, file );
		}

		fclose( file );
	}
}

BW_END_NAMESPACE