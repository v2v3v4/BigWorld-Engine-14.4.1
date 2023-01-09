#ifndef PERLIN_NOISE_HPP
#define PERLIN_NOISE_HPP

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "vector2.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the Perlin noise.
 *	What is Perlin noise: Perlin noise is a noise function (seeded random
 *	number generator) which is just several Interpolated Noise functions at different
 *	scales/frequencies added together. 
 */
class PerlinNoise
{
public:
	//herein lies Perlin's original implementation
	/* coherent noise function over 1, 2 or 3 dimensions */
	/* (copyright Ken Perlin) */
	void init(void);
	float noise1( float arg );
	float noise2( const Vector2 & vec );
	float noise3( float vec[3] );
	void normalise2( float v[2] );
	void normalise3( float v[3] );

	//hereafter lies MF's original extensions
	float sumHarmonics2( const Vector2 & vec, float persistence,
						 float frequency, float nIterations );
private:
};

class MyPerlin
{
public:
	MyPerlin();

	float noise1( float t );

	void octaves( int n ) { m_nOctaves = n; }
	void persistence( float f ) { m_fPersistence = f; }
protected:
	float m_aafNoise[3][256];

	float m_fPersistence;
	int m_nOctaves;

	float m_afAmp[3];

	template< int N >
	float tNoise( float t );
};

BW_END_NAMESPACE


#endif
