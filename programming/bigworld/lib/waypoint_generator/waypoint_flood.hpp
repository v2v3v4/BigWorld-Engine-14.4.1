#ifndef WAYPOINT_FLOOD_HEADER
#define WAYPOINT_FLOOD_HEADER

#include "math/vector3.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class Chunk;

/*
store one of these for each grid point at each height. 
*/
union AdjGridElt
{
	// return the adjacency info for angle a.
	uint32 angle( uint a )
		{ return (all >> (a<<2)) & 15; }

	// set the adjacency info for angle a. 
	void angle( uint a, uint32 adj )
		{ all = (all & ~(15 << (a<<2))) | adj << (a<<2); }

	uint32	all;
	struct
	{
		uint32	u:4;	// 0
		uint32	ur:4;	// 1
		uint32	r:4;	// 2
		uint32	dr:4;	// 3
		uint32	d:4;	// 4
		uint32	dl:4;	// 5
		uint32	l:4;	// 6
		uint32	ul:4;	// 7
	} each;
};

#define DROP_FUDGE (0.1f)

struct IPhysics
{
	virtual Vector3 getGirth() const = 0;
	virtual float getScrambleHeight() const = 0;
	virtual bool findDropPoint(const Vector3& pos, float& y) = 0;
	virtual bool isUnblocked(const Vector3& src, const float anotherY)
	{
		float y;

		if (almostEqual( src.y, anotherY ))
		{
			return true;
		}

		if( src.y < anotherY )
		{
			if( findDropPoint( Vector3( src.x, anotherY + DROP_FUDGE, src.z ), y ) )
			{
				if( y <= src.y + DROP_FUDGE )
					return true;
			}
			return false;
		}
		else if( src.y > anotherY )
		{
			if( findDropPoint( Vector3( src.x, src.y + DROP_FUDGE, src.z ), y ) )
			{
				if( y <= anotherY + DROP_FUDGE )
					return true;
			}
			return false;
		}
		return true;
	}
	virtual void adjustMove(const Vector3& src, const Vector3& dst,
		Vector3& dst2) = 0;
};

/**
 *	This class performs a flood fill on a collision scene, sampling it
 *	with a given resolution, in order to generate an adjacency bitmap.
 */
class WaypointFlood
{
public:
	struct IProgress
	{
		/**
		 *	returns true to exit the flood loop early.
		 */
		virtual bool filled( int npoints ) = 0;
	};
	
	WaypointFlood();
	~WaypointFlood();

	bool setArea(const Vector3& min, const Vector3& max, float resolution);
	void setPhysics(IPhysics* pPhysicsInterface);
	void setChunk( Chunk * pChunk );
	void clear();
	int fill(const Vector3& seedPoint, IProgress * pProgress = NULL, bool debug = false );
	void flashFlood( float height );
	void postfilteradd();
	void postfilterremove();
	void shrink();
	bool writeTGA(const char* filename) const;

	Vector3			min()			{ return min_; }		 // min bb point.
	Vector3			max()			{ return max_; }		 // max bb point.
	float			resolution()	{ return resolution_; }  // sampling resolution.
	int				xsize()			{ return xsize_; }
	int				zsize()			{ return zsize_; }

	AdjGridElt **	adjGrids()		{ return adjGrids_; }
	float **		hgtGrids()		{ return hgtGrids_; }

	static const uint MAX_HEIGHTS = 16;

private:
	Vector3 		min_;         // min bounding box point.
	Vector3 		max_;         // max bounding box point.
	float			resolution_;  // sampling resolution.
	int				xsize_;       // number pixels x dimension.
	int				zsize_;       // number pixels z dimension.
	int				size_;        // area x*z size
	// there are MAX_HEIGHTS heights per grid location. they are stored here.
	float*			hgtGrids_[MAX_HEIGHTS];
	// for each height on each grid point store a AdjGridElt (see above).
	AdjGridElt *	adjGrids_[MAX_HEIGHTS];

	IPhysics*		pPhysics_;

	Chunk * pChunk_; // current chunk.

	bool checkBounds( Vector3 src, Vector3 dst );
	bool checkGridConsistency();

	BW::set<std::pair<int, int> > boundarySet_;

	int				smallestHeight_;
};	

BW_END_NAMESPACE

#endif
