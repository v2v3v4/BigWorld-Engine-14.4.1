#ifndef GRAHAM_SCAN_HPP
#define GRAHAM_SCAN_HPP

#include "cstdmf/bw_vector.hpp"
#include <stack>
#include "math/planeeq.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class finds a convex hull from the given array
 *	of Vector3 points, and a plane on which all the points lie.
 *	It uses the Graham Scan algorithm, which does the job in O(n log n) time.
 */
class GrahamScan
{
public:
	GrahamScan( BW::vector<Vector3>& pts ):
	  pts_( pts )
	{
		if (pts.size() > 3)
		{
			this->planeEquation(plane_);
			this->makeConvexHull();
		}
	}

	typedef std::stack<Vector3> Stack;

	void planeEquation( PlaneEq& result )
	{
		result.init( pts_[0], pts_[1], pts_[2], PlaneEq::SHOULD_NORMALISE );
	}

	// sorting function to find the point on the plane with the lowest Y value,
	// and given y values that are equal, the lowest X value.
	struct LowestYPlaneSort
	{
		bool operator()(Vector3 a, Vector3 b)
		{			
			if (MF_FLOAT_EQUAL(a.y,b.y))
				return a.x < b.x;
			return a.y < b.y;
		}	
	};


	// sorting function by angle
	// (ignores z, assumes they are projected onto a plane)
	// note this algorithm assumes incoming points are above the fulcrum.
	// This assumption holds because the points are sorted by y position first.
	struct LowestAnglePlaneSort
	{
		LowestAnglePlaneSort( const Vector3& fulcrum ):		
			fulcrum_(fulcrum)
		{
		}

		bool operator()(Vector3 a, Vector3 b)
		{
			Vector2 tanA( a.x - fulcrum_.x, a.y - fulcrum_.y) ;
			Vector2 tanB( b.x - fulcrum_.x, b.y - fulcrum_.y) ;

			return ( (tanA.x*tanB.y) > (tanB.x*tanA.y) );
		}
	private:		
		Vector3 fulcrum_;
	};


	//annoying bit of code to retrieve the top + second entries on a stack.
	void findStackTop2( Stack& stack, Vector3* pt )
	{		
		pt[1] = stack.top();		
		stack.pop();
		pt[0] = stack.top();
		stack.push(pt[1]);		
	}


	//Is pt on the left of the line made between pts[0] and pts[1] ?
	float isLeft( const Vector3* pts, const Vector3& pt )
	{
		return (pts[1].x - pts[0].x) * (pt.y - pts[0].y) - 
				(pt.x - pts[0].x)*(pts[1].y - pts[0].y);
	}


	//Convert all our points into 2D plane space
	void projectPoints()
	{
		for (uint32 i=0; i<pts_.size(); i++)
		{
			Vector2 prj = plane_.project(pts_[i]);
			pts_[i].x = prj.x;
			pts_[i].y = prj.y;
		}
	}


	//Convert all our points back into 3D space.
	void unprojectPoints()
	{
		for (uint32 i=0; i<pts_.size(); i++)
		{
			Vector2 param(pts_[i].x,pts_[i].y);
			pts_[i] = plane_.param(param);
		}
	}


	/**
	 *	This method is the entry point for the algorithm.  It takes a vector of
	 *	3D points, 
	 */
	void makeConvexHull()
	{
		//Graham scan algorithm.

		//0. early out, any triangle would already be a convex hull.
		if (pts_.size() <= 3)
			return;

		//From here in, we work in 2D space, with all points projected onto
		//the plane (GrahamScan only works on 2D hulls)
		this->projectPoints();

		//1. First find the fulcrum point, with the lowest Y projected onto the plane.
		std::sort( pts_.begin(), pts_.end(), LowestYPlaneSort() );
		
		//2. Sort the points by angle between them and the fulcrum point.
		std::sort( pts_.begin()+1, pts_.end(), LowestAnglePlaneSort(pts_[0]) );

		//3. Now go through the sorted list, discarding only left hand turns	
		Stack stack;
		stack.push(pts_[0]);
		stack.push(pts_[1]);

		Vector3 pts[2];
		size_t i=2;
		float cp;
		while (i<pts_.size())
		{		
			this->findStackTop2( stack, pts );

			cp = this->isLeft( pts, pts_[i] );
			if (cp>0)
			{
				stack.push(pts_[i]);
			}
			else
			{
				do
				{
					stack.pop();
					if (stack.size() > 2)
					{
						this->findStackTop2( stack, pts );						
						cp = this->isLeft(pts, pts_[i]);
					}
				}
				while (cp<=0 && stack.size()>2);
				
				stack.push(pts_[i]);
			}
			i++;
		}

		//Finally, check the last point is inside the hull.
		this->findStackTop2( stack, pts );
		cp = this->isLeft(pts,pts_[0]);
		if (cp <= 0)
			stack.pop();
		
		//Now, put the results back in the list
		pts_.clear();
		pts_.resize( stack.size() );
		i = stack.size() - 1;
		while (!stack.empty())
		{
			pts_[i] = stack.top();
			stack.pop();
			i--;
		}

		//And return to 3D space.
		this->unprojectPoints();
	}

private:
	PlaneEq	plane_;
	BW::vector<Vector3>& pts_;
};


BW_END_NAMESPACE

#endif
