
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{
	/**
	 *	This method returns the height of this terrain block at the input
	 *	position. Returns NO_TERRAIN if there is a hole there or if the
	 *	point is outside the size of the block.
	 */
	INLINE
	float BaseTerrainBlock::heightAt( float xf, float zf ) const
	{
		if (allHoles_)
			return NO_TERRAIN;

		xf /= spacing_;
		zf /= spacing_;
		float xr = floorf( xf );
		float zr = floorf( zf );
		xf -= xr;
		zf -= zr;

#ifndef _XBOX
		uint32 x = uint32(xr+1);
		uint32 z = uint32(zr+1);
		uint32 index = x + ( z * width_ );

		if (uint32(x-1) >= blocksWidth_ || uint32(z-1) >= blocksWidth_) return NO_TERRAIN;
		if (holes_[x-1 + (z-1) * blocksWidth_]) return NO_TERRAIN;

		float h1 = heightMap_[index];
		float h2 = heightMap_[index+1];
		float h3 = heightMap_[index+width_];
		float h4 = heightMap_[index+width_+1];
#else
		uint32 x = uint32(xr);
		uint32 z = uint32(zr);
		uint32 index = x + ( z * verticesWidth_ );

		uint32 hind = x + z * blocksWidth_;
		if (x >= blocksWidth_ || z >= blocksWidth_) return NO_TERRAIN;
		if (!noHoles_ && (hind >= holes_.size() || holes_[hind])) return NO_TERRAIN;

		float h1 = pVerts_[index].y_;
		float h2 = pVerts_[index+1].y_;
		float h3 = pVerts_[index+verticesWidth_].y_;
		float h4 = pVerts_[index+verticesWidth_+1].y_;
#endif
		float height;
		if( zf > xf )
		{
			zf = 1.f - zf;
			height = h3 + ( ( h4 - h3 ) * xf ) + ( ( h1 - h3 ) * zf );
		}
		else
		{
			xf = 1.f - xf;
			height = h2 + ( ( h1 - h2 ) * xf ) + ( ( h4 - h2 ) * zf );
		}

		return height;
	}


	INLINE
	uint8 BaseTerrainBlock::detailID( float xf, float zf ) const
	{
		xf = floorf( xf / spacing_ );
		zf = floorf( zf / spacing_ );

		uint32 x = uint32(xf+1) % detailWidth_;
		uint32 z = uint32(zf+1) % detailHeight_;
		uint32 index = x + ( z * detailWidth_ );

		return detailIDs_[index];
	}

	INLINE
	float BaseTerrainBlock::heightPole( uint32 x, uint32 z ) const
	{
#ifndef _XBOX
		x = ( x % verticesWidth_ ) + 1;
		z = ( z % verticesHeight_ ) + 1;
		return heightMap_[ x + (z * width_) ];
#else
		x = ( x % verticesWidth_ );
		z = ( z % verticesHeight_ );
		return pVerts_[ x + (z * verticesWidth_) ].y_;
#endif
	}

	INLINE
	uint32 BaseTerrainBlock::heightPoleBlend( uint32 x, uint32 z ) const
	{
#ifndef _XBOX
		x = ( x % verticesWidth_ ) + 1;
		z = ( z % verticesHeight_ ) + 1;
		return blendValues_[ x + (z * width_) ];
#else
		x = ( x % verticesWidth_ );
		z = ( z % verticesHeight_ );
		return pVerts_[ x + (z * verticesWidth_) ].diffuse_;
#endif
	}

	INLINE
	uint32 BaseTerrainBlock::heightPoleShadow( uint32 x, uint32 z ) const
	{
#ifndef _XBOX
		x = ( x % verticesWidth_ ) + 1;
		z = ( z % verticesHeight_ ) + 1;
		return uint32(shadowValues_[ x + (z * width_) ]) << 8;
#else
		x = ( x % verticesWidth_ );
		z = ( z % verticesHeight_ );
		return uint32(pVerts_[ x + (z * verticesWidth_) ].shadow_) << 8;
#endif
	}

	INLINE
	Vector3 BaseTerrainBlock::heightPoleNormal( uint32 x, uint32 z ) const
	{
#ifndef _XBOX
		x = ( x % verticesWidth_ ) + 1;
		z = ( z % verticesHeight_ ) + 1;
		uint32 index = x + ( z * width_ );

		float diagonalDistance = sqrtf( spacing_ * spacing_ * 2 );

		Vector3 ret;
		ret.y = ( diagonalDistance * 4 ) + ( spacing_ * 4 );
		ret.x = heightMap_[ index - 1 ] - heightMap_[ index + 1 ];
		ret.z = heightMap_[ index - width_ ] - heightMap_[ index + width_ ];

		const float diagonalMultiplier = 0.7071067811f;// sqrt( 0.5 )

		float val = heightMap_[ index - 1 - width_ ] -
					heightMap_[ index + 1 + width_ ];
		val *= diagonalMultiplier;

		ret.x += val;
		ret.z += val;

		val = heightMap_[ index - 1 + width_ ] -
				heightMap_[ index + 1 - width_ ];
		val *= diagonalMultiplier;

		ret.x += val;
		ret.z -= val;
		ret.normalise();
#else
		x = ( x % verticesWidth_ );
		z = ( z % verticesHeight_ );

		uint32 packed = pVerts_[ x + (z * verticesWidth_) ].normal_;
		Vector3 ret(
			float(int32(packed << 21) >> 21) / 1023.f,
			float(int32(packed << 10) >> 21) / 1023.f,
			float(int32(packed) >> 22) / 511.f
			);
#endif
		return ret;
	}

	INLINE
	Vector3 BaseTerrainBlock::normalAt( float x, float z ) const
	{
		float xf = x / spacing_;
		float zf = z / spacing_;

		Vector3 ret;
		float xFrac = xf - floorf( xf );
		float zFrac = zf - floorf( zf );

		xf -= xFrac;
		zf -= zFrac;

		uint32 xfi = uint32(xf);
		uint32 zfi = uint32(zf);
		Vector3 n1 = heightPoleNormal( xfi, zfi );
		Vector3 n2 = heightPoleNormal( xfi + 1, zfi );
		Vector3 n3 = heightPoleNormal( xfi, zfi + 1 );
		Vector3 n4 = heightPoleNormal( xfi + 1, zfi + 1 );

		n1 *= ( 1 - xFrac ) * ( 1 - zFrac );
		n2 *= (     xFrac ) * ( 1 - zFrac );
		n3 *= ( 1 - xFrac ) * (		zFrac );
		n4 *= (		xFrac ) * (		zFrac );

		ret = n1;
		ret += n2;
		ret += n3;
		ret += n4;

		ret.normalise();
		return ret;
	}


	// TODO: Move to TerrainBlock
#if 0
	INLINE
	uint32 BaseTerrainBlock::shadowAt( float x, float z ) const
	{
		float rx = x / spacing_;
		float rz = z / spacing_;

		float xFrac = rx - floorf( rx );
		float zFrac = rz - floorf( rz );

		rx -= xFrac;
		rz -= zFrac;

		uint32 rxi = uint32(rx);
		uint32 rzi = uint32(rz);
		Colour s1(heightPoleShadow( rxi, rzi ));
		Colour s2(heightPoleShadow( rxi + 1, rzi ));
		Colour s3(heightPoleShadow( rxi, rzi + 1 ));
		Colour s4(heightPoleShadow( rxi + 1, rzi + 1 ));

		uint32 ret = 0;

		if (zFrac > xFrac)
		{
			zFrac = 1 - zFrac;
			ret = s3 + ((s1 - s3) * zFrac) + ((s4 - s3) * xFrac);
		}
		else
		{
			xFrac = 1 - xFrac;
			ret = s2 + ((s4 - s2) * zFrac) + ((s1 - s2) * xFrac);
		}
		return ret;

	}
#endif

	// -------------------------------------------------------------------------
	// Section: TerrainFinder
	// -------------------------------------------------------------------------

	/**
	 *	This method returns the outside terrain block associated with the input
	 *	position. It also returns related information such as the terrain's
	 *	transform.
	 */
	INLINE
	BaseTerrainBlock::FindDetails
		BaseTerrainBlock::findOutsideBlock( const Vector3 & pos )
	{
		MF_ASSERT( pTerrainFinder_ != NULL );

		return pTerrainFinder_->findOutsideBlock( pos );
	}


	/**
	 *	This method sets the object that will be used for finding terrain
	 *	blocks.
	 */
	INLINE
	void BaseTerrainBlock::setTerrainFinder( TerrainFinder & terrainFinder )
	{
		MF_ASSERT( pTerrainFinder_ == NULL );

		pTerrainFinder_ = &terrainFinder;
	}



} //namespace Moo

// base_terrain_block.ipp
