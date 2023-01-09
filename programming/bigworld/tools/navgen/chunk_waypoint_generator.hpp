#ifndef CHUNK_WAYPOINT_GENERATOR_HPP
#define CHUNK_WAYPOINT_GENERATOR_HPP

#include "chunk/chunk.hpp"
#include "chunk_flooder.hpp"
#include "girth.hpp"
#include "waypoint_generator/waypoint_generator.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Helper class to generate waypoints for a chunk
 */
class ChunkWaypointGenerator
{
public:
	ChunkWaypointGenerator( Chunk * pChunk,
		const BW::string& floodResultPath );
	virtual ~ChunkWaypointGenerator();
	bool modified()	const	{ return modified_; }
	bool ready() const;

	int maxFloodPoints() const;
	void flood( bool (*progressCallback)( int npoints ), Girth gSpec, bool writeTGAs );
	void generate( bool annotate, Girth gSpec );
	void output( float girth, bool firstGirth );
	void outputDirtyFlag( bool dirty = false );
    Chunk *chunk() const { return pChunk_; }

    static bool canProcess(Chunk *chunk);

private:
	Chunk * pChunk_;

	ChunkFlooder		flooder_;
	WaypointGenerator	gener_;
	BW::vector<Vector3>	entityPts_;

	bool				modified_;
};

BW_END_NAMESPACE

#endif

