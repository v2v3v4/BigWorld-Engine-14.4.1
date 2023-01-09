#include "pch.hpp"

#include "bounding.h"
//#include "Util.h"
//#include "Common.h"
#include <assert.h>
#include <algorithm>

#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L)==0)

using namespace math_helpers;

/////////////////////////////////////////////////////////////////

D3DXVECTOR2 ConvexHull2D::isLeftSort::pivotPt;

ConvexHull2D::ConvexHull2D()
{
    hull.clear();
}

ConvexHull2D::ConvexHull2D( const ConvexHull2D& other )
{
    hull.clear();
    hull.reserve( other.hull.size() );
    BW::vector<D3DXVECTOR2>::const_iterator it = other.hull.begin();
    
    while ( it != other.hull.end() )
        hull.push_back( D3DXVECTOR2(*it++) );
}

ConvexHull2D::ConvexHull2D( const D3DXVECTOR3* vertices, DWORD nVerts )
{
    hull.clear();
    hull.reserve( nVerts );             // generous over-allocation
    D3DXVECTOR2 pivot( vertices[0].x, vertices[0].y );
    BW::vector<D3DXVECTOR2> pointSet;  // stores all points except the pivot
    pointSet.reserve( nVerts - 1 );

    for ( DWORD i=1; i<nVerts; i++ )
    {
        D3DXVECTOR2 tmp( vertices[i].x, vertices[i].y );
        if ( (tmp.y<pivot.y) || (tmp.y==pivot.y && tmp.x>pivot.x) )
        {
            pointSet.push_back( D3DXVECTOR2(pivot) );
            pivot = tmp;
        }
        else
            pointSet.push_back( D3DXVECTOR2(tmp) );
    }

    isLeftSort::pivotPt = pivot;

    BW::vector<D3DXVECTOR2>::iterator ptEnd = std::unique(pointSet.begin(), pointSet.end());
    pointSet.erase( ptEnd, pointSet.end() );

    std::sort( pointSet.begin(), pointSet.end(), isLeftSort() );

    hull.push_back( D3DXVECTOR2(pivot) );
    hull.push_back( D3DXVECTOR2(pointSet[0]) );

    DWORD cnt=1;

    while ( cnt < pointSet.size() )
    {
        //DBG_ASSERT( hull.size() >= 2 );
        const D3DXVECTOR2& pT1 = hull[hull.size()-1];
        const D3DXVECTOR2& pT2 = hull[hull.size()-2];
        const D3DXVECTOR2& pK  = pointSet[cnt];
        float leftTest = isLeft(pT2, pT1, pK);
        if ( leftTest>0.f )
        {
            hull.push_back( D3DXVECTOR2(pK) );
            cnt++;
        }
        else if ( leftTest == 0.f )
        {
            cnt++;
            D3DXVECTOR2 diffVec0 = pK - pT2;
            D3DXVECTOR2 diffVec1 = pT1 - pT2;
            if ( D3DXVec2Dot(&diffVec0, &diffVec0) > D3DXVec2Dot(&diffVec1, &diffVec1) )
            {
                hull[hull.size()-1] = pK;
            }
        }
        else
        {
            hull.pop_back();
        }
    }
}


/////////////////////////////////////////////////////////////////
//  find (near) minimum bounding sphere enclosing the list of points

BoundingSphere::BoundingSphere( const BW::vector<D3DXVECTOR3>* points )
{
    assert(points->size() > 0);
    BW::vector<D3DXVECTOR3>::const_iterator ptIt = points->begin();

    radius = 0.f;
    centerVec = *ptIt++;

    while ( ptIt != points->end() )
    {
        const D3DXVECTOR3& tmp = *ptIt++;
        D3DXVECTOR3 cVec = tmp - centerVec;
        float d = D3DXVec3Dot( &cVec, &cVec );
        if ( d > radius*radius )
        {
            d = sqrtf(d);
            float r = 0.5f * (d+radius);
            float scale = (r-radius) / d;
            centerVec = centerVec + scale*cVec;
            radius = r;
        }
    }
}

////////////////////////////////////////////////////////////////

void BoundingBox::Merge(const D3DXVECTOR3* vec)
{
    minPt.x = min(minPt.x, vec->x);
    minPt.y = min(minPt.y, vec->y);
    minPt.z = min(minPt.z, vec->z);
    maxPt.x = max(maxPt.x, vec->x);
    maxPt.y = max(maxPt.y, vec->y);
    maxPt.z = max(maxPt.z, vec->z);
}

bool BoundingBox::Intersect(float* hitDist, const D3DXVECTOR3* origPt, const D3DXVECTOR3* dir)
{
    D3DXPLANE sides[6] = { D3DXPLANE( 1, 0, 0,-minPt.x), D3DXPLANE(-1, 0, 0, maxPt.x),
                           D3DXPLANE( 0, 1, 0,-minPt.y), D3DXPLANE( 0,-1, 0, maxPt.y),
                           D3DXPLANE( 0, 0, 1,-minPt.z), D3DXPLANE( 0, 0,-1, maxPt.z) };

    *hitDist = 0.f;  // safe initial value
    D3DXVECTOR3 hitPt = *origPt;

    bool inside = false;

    for ( int i=0; (i<6) && !inside; i++ )
    {
        float cosTheta = D3DXPlaneDotNormal( &sides[i], dir );
        float dist = D3DXPlaneDotCoord ( &sides[i], origPt );
        
        //  if we're nearly intersecting, just punt and call it an intersection
        if ( ALMOST_ZERO(dist) ) return true;
        //  skip nearly (&actually) parallel rays
        if ( ALMOST_ZERO(cosTheta) ) continue;
        //  only interested in intersections along the ray, not before it.
        *hitDist = -dist / cosTheta;
        if ( *hitDist < 0.f ) continue;

        hitPt = (*hitDist)*(*dir) + (*origPt);

        inside = true;
        
        for ( int j=0; (j<6) && inside; j++ )
        {
            if ( j==i )
                continue;
            float d = D3DXPlaneDotCoord( &sides[j], &hitPt );
            
            inside = ((d + 0.00015) >= 0.f);
        }
    }

    return inside;        
}

BoundingCone::BoundingCone(const BW::vector<BoundingBox>* boxes, const D3DXMATRIX* projection, const D3DXVECTOR3* _apex, const D3DXVECTOR3* _direction): apex(*_apex), direction(*_direction)
{
    const D3DXVECTOR3 yAxis(0.f, 1.f, 0.f);
    const D3DXVECTOR3 zAxis(0.f, 0.f, 1.f);
    D3DXVec3Normalize(&direction, &direction);
    
    D3DXVECTOR3 axis = yAxis;

    if ( fabsf(D3DXVec3Dot(&yAxis, &direction))>0.99f )
        axis = zAxis;
    
	const D3DXVECTOR3 at(apex+direction);
    D3DXMatrixLookAtLH(&m_LookAt, &apex, &at, &axis);
    
    float maxx = 0.f, maxy = 0.f;
    fNear = 1e32f;
    fFar =  0.f;

    D3DXMATRIX concatMatrix;
    D3DXMatrixMultiply(&concatMatrix, projection, &m_LookAt);

    for (unsigned int i=0; i<boxes->size(); i++)
    {
        const BoundingBox& bbox = (*boxes)[i];
        for (int j=0; j<8; j++)
        {
            D3DXVECTOR3 vec = bbox.Point(j);
            D3DXVec3TransformCoord(&vec, &vec, &concatMatrix);
            maxx = max(maxx, fabsf(vec.x / vec.z));
            maxy = max(maxy, fabsf(vec.y / vec.z));
            fNear = min(fNear, vec.z);
            fFar  = max(fFar, vec.z);
        }
    }
    fovx = atanf(maxx);
    fovy = atanf(maxy);
}

BoundingCone::BoundingCone(const BW::vector<BoundingBox>* boxes, const D3DXMATRIX* projection, const D3DXVECTOR3* _apex) : apex(*_apex)
{
    const D3DXVECTOR3 yAxis(0.f, 1.f, 0.f);
    const D3DXVECTOR3 zAxis(0.f, 0.f, 1.f);
    const D3DXVECTOR3 negZAxis(0.f, 0.f, -1.f);
    switch (boxes->size())
    {
    case 0: 
    {
        direction = negZAxis;
        fovx = 0.f;
        fovy = 0.f;
        D3DXMatrixIdentity(&m_LookAt);
        break;
    }
    default:
    {
        unsigned int i, j;


        //  compute a tight bounding sphere for the vertices of the bounding boxes.
        //  the vector from the apex to the center of the sphere is the optimized view direction
        //  start by xforming all points to post-projective space
        BW::vector<D3DXVECTOR3> ppPts;
        ppPts.reserve(boxes->size() * 8);

        for (i=0; i<boxes->size(); i++) 
        {
            for (j=0; j<8; j++)
            {
                D3DXVECTOR3 tmp = (*boxes)[i].Point(j);
                D3DXVec3TransformCoord(&tmp, &tmp, projection);

                ppPts.push_back(tmp);
            }
        }

        //  get minimum bounding sphere
        BoundingSphere bSphere( &ppPts );

        float min_cosTheta = 1.f;
        
        direction = bSphere.centerVec - apex;
        D3DXVec3Normalize(&direction, &direction);

        D3DXVECTOR3 axis = yAxis;

        if ( fabsf(D3DXVec3Dot(&yAxis, &direction)) > 0.99f )
            axis = zAxis;

		const D3DXVECTOR3 at(apex+direction);
        D3DXMatrixLookAtLH(&m_LookAt, &apex, &at, &axis);

        fNear = 1e32f;
        fFar = 0.f;

        float maxx=0.f, maxy=0.f;
        for (i=0; i<ppPts.size(); i++)
        {
            D3DXVECTOR3 tmp;
            D3DXVec3TransformCoord(&tmp, &ppPts[i], &m_LookAt);
            maxx = max(maxx, fabsf(tmp.x / tmp.z));
            maxy = max(maxy, fabsf(tmp.y / tmp.z));
            fNear = min(fNear, tmp.z);
            fFar  = max(fFar, tmp.z);
        }

        fovx = atanf(maxx);
        fovy = atanf(maxy);
        break;
    }
    } // switch
}