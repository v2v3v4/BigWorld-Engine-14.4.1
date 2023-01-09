#ifndef CHUNK_FLOODER_HPP
#define CHUNK_FLOODER_HPP

#include "math/vector3.hpp"
#include "girth.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class AdjacentChunkSet;
class WaypointFlood;
union AdjGridElt;
class PhysicsHandler;

/**
 *	This class flood fills a chunk with physics tests to make
 *	passable path pixmap, which is used by a waypoint generator.
 */
class ChunkFlooder
{
public:
	ChunkFlooder( Chunk * pChunk, const BW::string& floodResultPath );
	~ChunkFlooder();

	bool flood(
		Girth gSpec,
		const BW::vector<Vector3>& entityPts,
		bool (*progressCallback)( int npoints ) = NULL,
		int nshrink = 0,
        bool writeTGAs = true );

	Vector3	minBounds() const;
	Vector3	maxBounds() const;
	float	resolution() const;
	int		width() const;
	int		height() const;
	AdjGridElt **	adjGrids() const;
	float **		hgtGrids() const;

private:
	ChunkFlooder( const ChunkFlooder& );
	ChunkFlooder& operator=( const ChunkFlooder& );

	void reset();

	void getSeedPoints( BW::vector<Vector3> & pts, PhysicsHandler& ph );
	bool flashFlood( const Vector3 & seedPt );

	Chunk *					pChunk_;
	WaypointFlood *			pWF_;

	BW::string				floodResultPath_;
};

BW_END_NAMESPACE

#endif // CHUNK_FLOODER_HPP
