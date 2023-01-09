#ifndef __BOUNDING_H_included_
#define __BOUNDING_H_included_

#include <d3dx9.h>
#include <d3dx9math.h>
#include <math.h>

#include "cstdmf/bw_vector.hpp"


namespace math_helpers
{

	struct ConvexHull2D
	{
		BW::vector<D3DXVECTOR2> hull;

		ConvexHull2D();
		explicit ConvexHull2D( const ConvexHull2D& other );
		ConvexHull2D( const D3DXVECTOR3* vertices, DWORD nVerts );

		static float isLeft( const D3DXVECTOR2& p0, const D3DXVECTOR2& p1, const D3DXVECTOR2& p2 )
		{
			return (p1.x-p0.x)*(p2.y-p0.y) - (p2.x-p0.x)*(p1.y-p0.y);
		}

		struct isLeftSort
		{
			static D3DXVECTOR2 pivotPt;
			bool operator()( D3DXVECTOR2& p1, D3DXVECTOR2& p2 )
			{
				return ( isLeft(pivotPt, p1, p2)>=0 );
			}
		};
	};

	struct BoundingBox
	{
		D3DXVECTOR3 minPt;
		D3DXVECTOR3 maxPt;

		BoundingBox(): minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f) { }
		BoundingBox( const BoundingBox& other ): minPt(other.minPt), maxPt(other.maxPt) { }
	    
		explicit BoundingBox( const D3DXVECTOR3* points, UINT n ): minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
		{
			for ( unsigned int i=0; i<n; i++ )
				Merge( &points[i] );
		}

		explicit BoundingBox( const BW::vector<D3DXVECTOR3>* points): minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
		{
			for ( unsigned int i=0; i<points->size(); i++ )
				Merge( &(*points)[i] );
		}
		explicit BoundingBox( const BW::vector<BoundingBox>* boxes ): minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f) 
		{
			for (unsigned int i=0; i<boxes->size(); i++) 
			{
				Merge( &(*boxes)[i].maxPt );
				Merge( &(*boxes)[i].minPt );
			}
		}
		void Centroid( D3DXVECTOR3* vec) const { *vec = 0.5f*(minPt+maxPt); }
		void Merge( const D3DXVECTOR3* vec );
		bool Intersect( float* hitDist, const D3DXVECTOR3* origPt, const D3DXVECTOR3* dir );

		D3DXVECTOR3 Point(int i) const { return D3DXVECTOR3( (i&1)?minPt.x:maxPt.x, (i&2)?minPt.y:maxPt.y, (i&4)?minPt.z:maxPt.z );  }
	};

	struct BoundingSphere
	{
		D3DXVECTOR3 centerVec;
		float       radius;
	    
		BoundingSphere(): centerVec(), radius(0.f) { }
		BoundingSphere( const BoundingSphere& other ): centerVec(other.centerVec), radius(other.radius) { }
		explicit BoundingSphere( const BW::vector<D3DXVECTOR3>* points );
		explicit BoundingSphere( const BoundingBox* box )
		{
			D3DXVECTOR3 radiusVec;
			centerVec = 0.5f * (box->maxPt + box->minPt);
			radiusVec = box->maxPt - centerVec;
			float len = D3DXVec3Length(&radiusVec);
			radius = len;
		}
	};

	struct BoundingCone
	{
		D3DXVECTOR3 direction;
		D3DXVECTOR3 apex;
		float       fovy;
		float       fovx;
		float       fNear;
		float       fFar;
		D3DXMATRIX  m_LookAt;

		BoundingCone(): direction(0.f, 0.f, 1.f), apex(0.f, 0.f, 0.f), fovx(0.f), fovy(0.f), fNear(0.001f), fFar(1.f) { }
		BoundingCone(const BW::vector<BoundingBox>* boxes, const D3DXMATRIX* projection, const D3DXVECTOR3* _apex);
		BoundingCone(const BW::vector<BoundingBox>* boxes, const D3DXMATRIX* projection, const D3DXVECTOR3* _apex, const D3DXVECTOR3* _direction);
		BoundingCone(const BW::vector<D3DXVECTOR3>* points, const D3DXVECTOR3* _apex, const D3DXVECTOR3* _direction);
	};

}

#endif // __BOUNDING_H_included_