#ifndef GRID_TRAVERSAL_HPP
#define GRID_TRAVERSAL_HPP

#include "cstdmf/guard.hpp"
#include "cstdmf/static_array.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This private class is used to traverse a line through a uniform
 *	grid, such as that imposed on a chunk space.
 *
 *	cellSTravel and cellETravel are correct at all times
 *	for the cell at sx, sz.
 */
class SpaceGridTraversal
{
public:
	SpaceGridTraversal( const Vector3 & source, const Vector3 & extent_i,
			const Vector3 & range, float gridResF ) :
		extent( extent_i ),
		gridResF_( gridResF ),
		altCells( 0 )
	{
		BW_GUARD;
		// first find what column 'source' and 'extent' are in
		sx = int(source.x / gridResF_);	if (source.x < 0.f) sx--;
		sz = int(source.z / gridResF_);	if (source.z < 0.f) sz--;

		ex = int(extent.x / gridResF_);	if (extent.x < 0.f) ex--;
		ez = int(extent.z / gridResF_);	if (extent.z < 0.f) ez--;

		// find out which way we're going
		dir = extent - source;
		xsense = dir.x < 0 ? -1 : dir.x > 0 ? 1 : 0;
		zsense = dir.z < 0 ? -1 : dir.z > 0 ? 1 : 0;

		// record how big the the thing we're moving through is
		xrange = range.x;
		zrange = range.z;
		xseep = gridResF_ - range.x;
		zseep = gridResF_ - range.z;

		// find length of line
		fullDist = dir.length();
		dir /= fullDist;

		// prepare for first call of next
		land = source;
		cellETravelPrivate = 0.f;
		nx = sx;
		nz = sz;
		allOver = false;

		onRight = land.x - nx * gridResF_ > xseep;
		onAbove = land.z - nz * gridResF_ > zseep;
		nearSTravel[0][0] = 0.f, nearSTravel[0][1] = 0.f;
		nearSTravel[1][0] = 0.f, nearSTravel[1][1] = 0.f;
		offNear( sx, sz );
		nearCount[0][0] = 0, nearCount[0][1] = 0;
		nearCount[1][0] = 0, nearCount[1][1] = 0;
		incNear( 0, 0, 0.f );
		incNear( 0, onAbove, 0.f );
		incNear( onRight, 0, 0.f );
		incNear( onRight, onAbove, 0.f );

		// calculate cast and land for this square, and nx and nz.
		this->calcNext();

		MF_VERIFY( this->popAltCell() );

		//dprintf( "Start sx %d sz %d ex %d ez %d dir(%f,%f,%f) xsense %d zsense %d\n",
		//	sz, sz, ex, ez, dir.x, dir.y, dir.z, xsense, zsense );
	}

	bool next()
	{
		BW_GUARD;
		if (this->popAltCell()) return true;

		if (allOver) return false;

		sx = nx;
		sz = nz;

		//onRight = nnRight;
		//onAbove = nnAbove;

		this->calcNext();

		/*
		dprintf( "Collide sx %d sz %d\n", sx, sz );
		if (nx == sx && nz == sz && !(sx == ex && sz == ez))
		{
			dprintf( "xsense %d zsense %d\n", xsense, zsense );
			dprintf( "cast (%f,%f,%f), dir (%f,%f,%f)\n",
				cast.x, cast.y, cast.z, dir.x, dir.y, dir.z );
			dprintf( "columnTravel %f, fullDist %f\n", columnTravel, fullDist );
			CRITICAL_MSG( "Infinite loop detected! sx %d sz %d ex %d ez %d\n",
				sx, sz, ex, ez );
		}
		*/

		MF_VERIFY( this->popAltCell() );

		return true;
	}

	Vector3	extent;
	Vector3	dir;

	float	gridResF_;

	float	fullDist;
	bool	allOver;

	int		xsense;
	int		zsense;

	int		sx;
	int		sz;
	int		ex;
	int		ez;

	float	cellSTravel;
	float	cellETravel;

private:
	Vector3	cast;
	Vector3	land;

	float	cellETravelPrivate;

	float	xrange;
	float	zrange;
	float	xseep;
	float	zseep;

	int		nx;
	int		nz;

	int		gx;
	int		gz;

	int		bx;
	int		bz;

	bool	onRight;
	bool	onAbove;
	//bool	nnRight;
	//bool	nnAbove;

	char	nearCount[2][2];
	float	nearSTravel[2][2];

	struct CellSpec
	{
		int		x;
		int		z;
		float	sTravel;
		float	eTravel;
	};
	StaticArray<CellSpec, 1024> altCells;

	inline void incNear( int r, int a, float sTrav )
	{
		r ^= bx;
		a ^= bz;

		if (!nearCount[r][a]++)
		{
			nearSTravel[r][a] = sTrav;
		}
	}

	inline void offNear( int x, int z )
	{
		gx = x;
		gz = z;
		bx = x&1;
		bz = z&1;
	}

	inline void decNear( int r, int a, float eTrav )
	{
		r ^= bx;
		a ^= bz;
		if (!--nearCount[r][a])
		{
			CellSpec cs = { gx+(r^bx), gz+(a^bz), nearSTravel[r][a], eTrav };
			altCells.push_back( cs );
		}
	}

	void calcNext()
	{
		BW_GUARD;
		cast = land;
		land = extent;

		cellETravel = cellETravelPrivate;
		cellSTravel = cellETravel;

		// before returning, set up nearCount for nx, nz, and add any
		// alternate cells that need to be looked at before next call

		// find next point on the face of a column
		float tryDist;
		float tryOther;
		if (xsense != 0)
		{
			float	xedge = ( xsense < 0 ? sx : sx+1 ) * gridResF_;
			tryDist = (xedge - cast.x) / dir.x;
			tryOther = cast.z + tryDist * dir.z;
			if (tryOther >= sz * gridResF_ &&
				tryOther <= (sz+1) * gridResF_ &&
				cellETravel + tryDist < fullDist)
			{
				land.x = xedge;
				land.y = cast.y + tryDist * dir.y;
				land.z = tryOther;
				nx = sx + xsense;
				cellETravel += tryDist;

				// update our cell counters
				if (xsense < 0)	// moving left
				{
					// take out right points
					decNear( onRight, 0, cellETravel );
					decNear( onRight, onAbove, cellETravel );

					// offset grid
					offNear( nx, nz );
					onRight = xrange > 0.f;	// since pt on extreme right

					// move left points
					incNear( 0, 0, cellETravel );
					incNear( 0, onAbove, cellETravel );
					decNear( 1, 0, cellETravel );
					decNear( 1, onAbove, cellETravel );

					// put back right points
					incNear( onRight, 0, cellETravel );
					incNear( onRight, onAbove, cellETravel );
				}
				else			// moving right
				{
					// move left points
					incNear( 1, 0, cellETravel );
					incNear( 1, onAbove, cellETravel );
					decNear( 0, 0, cellETravel );
					decNear( 0, onAbove, cellETravel );

					// take out right points
					decNear( onRight, 0, cellETravel );
					decNear( onRight, onAbove, cellETravel );

					// offset grid
					offNear( nx, nz );
					onRight = 0;			// since pt on extreme left

					// put back right points
					incNear( onRight, 0, cellETravel );
					incNear( onRight, onAbove, cellETravel );
				}
			}
		}
		if (zsense != 0)
		{
			float	zedge = ( zsense < 0 ? sz : sz+1 ) * gridResF_;
			tryDist = (zedge - cast.z) / dir.z;
			tryOther = cast.x + tryDist * dir.x;
			if (tryOther >= sx * gridResF_ &&
				tryOther <= (sx+1) * gridResF_ &&
				cellETravel + tryDist < fullDist)
			{
				land.x = tryOther;
				land.y = cast.y + tryDist * dir.y;
				land.z = zedge;
				nz = sz + zsense;
				if (nx == sx) cellETravel += tryDist;

				// update our cell counters
				if (zsense < 0)	// moving down
				{
					// take out above points
					decNear( 0, onAbove, cellETravel );
					decNear( onRight, onAbove, cellETravel );

					// offset grid
					offNear( nx, nz );
					onAbove = zrange > 0.f;	// since pt on extreme top

					// move below points
					incNear( 0, 0, cellETravel );
					incNear( onRight, 0, cellETravel );
					decNear( 0, 1, cellETravel );
					decNear( onRight, 1, cellETravel );

					// put back above points
					incNear( 0, onAbove, cellETravel );
					incNear( onRight, onAbove, cellETravel );
				}
				else			// moving up
				{
					// move below points
					incNear( 0, 1, cellETravel );
					incNear( onRight, 1, cellETravel );
					decNear( 0, 0, cellETravel );
					decNear( onRight, 0, cellETravel );

					// take out above points
					decNear( 0, onAbove, cellETravel );
					decNear( onRight, onAbove, cellETravel );

					// offset grid
					offNear( nx, nz );
					onAbove = 0;			// since pt on extreme bottom

					// put back above points
					incNear( 0, onAbove, cellETravel );
					incNear( onRight, onAbove, cellETravel );
				}
			}
		}

		// if we didn't move to another x cell, see if the right
		// corners are in a different region (i.e. has onRight changed)
		if (sx == nx)
		{
			bool nnRight = land.x - nx * gridResF_ > xseep;
			if (nnRight != onRight)
			{
				decNear( onRight, 0, cellETravel );
				decNear( onRight, onAbove, cellETravel );
				onRight = nnRight;
				incNear( onRight, 0, cellSTravel );
				incNear( onRight, onAbove, cellSTravel );
			}
		}
		if (sz == nz)
		{
			bool nnAbove = land.z - nz * gridResF_ > zseep;
			if (nnAbove != onAbove)
			{
				decNear( 0, onAbove, cellETravel );
				decNear( onRight, onAbove, cellETravel );
				onAbove = nnAbove;
				incNear( 0, onAbove, cellSTravel );
				incNear( onRight, onAbove, cellSTravel );
			}
		}

		// set etravel if there's no next cell
		if (nx == sx && nz == sz)
		{
			cellETravel = fullDist;

			// remove the four corners
			//  intentionally after sx == nx, etc. tests above
			decNear( 0, 0, cellETravel );
			decNear( 0, onAbove, cellETravel );
			decNear( onRight, 0, cellETravel );
			decNear( onRight, onAbove, cellETravel );

			// and don't come back!
			allOver = true;
		}

		cellETravelPrivate = cellETravel;
	}

	bool popAltCell()
	{
		BW_GUARD;
		if (altCells.empty()) return false;

		CellSpec cs = altCells.back();
		altCells.pop_back();

		sx = cs.x;
		sz = cs.z;
		cellSTravel = cs.sTravel;
		cellETravel = cs.eTravel;
		return true;
	}
};

BW_END_NAMESPACE


#endif // GRID_TRAVERSAL_HPP

