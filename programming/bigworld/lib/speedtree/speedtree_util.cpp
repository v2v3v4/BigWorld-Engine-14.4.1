#include "pch.hpp"

#include "speedtree_util.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_utils.hpp"
#include "math/quat.hpp"

BW_BEGIN_NAMESPACE

namespace speedtree
{

	/**
	 *	Creates a string that uniquely identifies a tree loaded from 
	 *	the given filename and generated with the provided seed number.
	 *
	 *	@param	filename	full path to the SPT file used to load the tree.
	 *	@param	seed		seed number used to generate the tree.
	 *	@param	dst			pointer to the destination buffer.
	 *	@param	dstSize		destination buffer size.
	 */
	void createTreeDefName( const BW::StringRef & filename, uint seed, char * dst,
		size_t dstSize )
	{
		BW_GUARD;
		char seedStr[16];
		ultoa( seed, seedStr, 10 );
		const size_t seedStrLen = bw_str_len( seedStr );
		MF_ASSERT( filename.size() + 1 + seedStrLen < dstSize );
		bw_str_copy( dst, dstSize, filename );
		bw_str_append( dst, dstSize, "/", 1 );
		bw_str_append( dst, dstSize, seedStr, seedStrLen );
	}

	//----------------------------------------------------------------------------------------------
	float LineInBox(const Vector3 &vMin, const Vector3 &vMax, const Vector3 &vStart, const Vector3 &vEnd)
	{
		bool bClip = false;
		float tMin = FLT_MAX;
		float tMax = -FLT_MAX;
		Vector3 vBounds[2] = { vMin, vMax };

		Vector3 vDir = vEnd - vStart;

		for( int iBound = 0; iBound < 2; iBound++ )
		{
			for( int iAxis = 0; iAxis < 3; iAxis++ )
			{
				if ( fabsf( vDir[iAxis] ) < 0.001f ) continue;
				float t = ( vBounds[iBound][iAxis] - vStart[iAxis] ) / vDir[iAxis];

				if ( t > 0 && t < 1 )
				{
					Vector3 v = vStart + t * vDir;
					int iNextAxis = ( iAxis + 1 ) % 3;
					int iPrevAxis = ( iAxis + 2 ) % 3;
					if ( v[iNextAxis] > vBounds[0][iNextAxis] && v[iNextAxis] < vBounds[1][iNextAxis] &&
						v[iPrevAxis] > vBounds[0][iPrevAxis] && v[iPrevAxis] < vBounds[1][iPrevAxis] )
					{
						tMin = min( tMin, t );
						tMax = max( tMax, t );
						bClip = true;
					}

				}
			}
		}

		if ( bClip )
		{
			if ( tMin == tMax )
				tMax = 1.0f;
			return ( tMax - tMin );
		}

		return 0.0f;
	}

	//----------------------------------------------------------------------------------------------
	float LineInEllipsoid(const Vector3 &vSize, const Vector3 &vStart, const Vector3 &vEnd)
	{
		//vStart - v0	vDir - D

		Vector3 V0 = vStart;
		V0.x /= vSize.x;
		V0.y /= vSize.y;
		V0.z /= vSize.z;
		Vector3 D = vEnd - vStart;
		D.x /= vSize.x;
		D.y /= vSize.y;
		D.z /= vSize.z;

		float a = D.dotProduct( D );
		float b = 2 * D.dotProduct( V0 );
		float c = V0.dotProduct( V0 ) - 1.0f;

		float d = b * b - 4 * a * c;
		if ( d > 0 )
		{
			d = sqrtf( d );
			float t0 = ( -b + d ) / ( 2.0f * a );
			float t1 = ( -b - d ) / ( 2.0f * a );
			t0 = Math::clamp( 0.0f, t0, 1.0f );
			t1 = Math::clamp( 0.0f, t1, 1.0f );
			return max( t0, t1 ) - min( t0, t1 );
		}
		else return 0;
	}

	//----------------------------------------------------------------------------------------------
	void decomposeMatrix(const Matrix& iMat, Quaternion& oRotQuat, Vector3& oTranslation, Vector3& oScale)
	{
		Matrix m = iMat;

		Vector3& row0 = m[0];
		Vector3& row1 = m[1];
		Vector3& row2 = m[2];
		Vector3& row3 = m[3];

		oScale.x = XPVec3Length( &row0 );
		oScale.y = XPVec3Length( &row1 );
		oScale.z = XPVec3Length( &row2 );

		row0 *= 1.f / oScale.x;
		row1 *= 1.f / oScale.y;
		row2 *= 1.f / oScale.z;

		Vector3 in;

		XPVec3Cross( &in, &row0, &row1 );
		if( XPVec3Dot( &in, &row2 ) < 0 )
		{
			row2 *= -1;
			oScale.z *= -1;
		}

		oTranslation = row3;

		XPQuaternionRotationMatrix(&oRotQuat, &m);
	}

} //-- speedtree

BW_END_NAMESPACE

// speedtree_util.cpp

