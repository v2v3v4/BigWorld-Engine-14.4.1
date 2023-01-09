#include "pch.hpp"

#include "perlin_noise.hpp"
#include "mathdef.hpp"

BW_BEGIN_NAMESPACE

#define B 0x100
#define BM 0xff

static int p[B + B + 2];
static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];
static bool start = true;

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

#define s_curve(t) ( t * t * (3.0f - 2.0f * t) )

#define setup(i,b0,b1,r0,r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.0f;


float cosInterp( float a, float b, float t )
{
	float ft = t * 3.1415927f;
	float f = ( 1 - cosf( ft ) ) * 0.5f;
	return a * ( 1 - f ) + b * f;
}

float PerlinNoise::noise1( float arg )
{
	int bx0, bx1;
	float rx0, rx1, sx, t, u, v, vec[1];

	vec[0] = arg;
	if (start) {
		start = false;
		init();
	}

	setup(0, bx0,bx1, rx0,rx1);

	sx = s_curve(rx0);

	u = rx0 * g1[ p[ bx0 ] ];
	v = rx1 * g1[ p[ bx1 ] ];

    return Math::lerp(sx, u, v);
}


float PerlinNoise::noise2( const Vector2 & vec )
{
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
	register int i, j;

	if (start) {
		start = false;
		init();
	}

	setup(0, bx0,bx1, rx0,rx1);
	setup(1, by0,by1, ry0,ry1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

	q = g2[ b00 ] ; u = at2(rx0,ry0);
	q = g2[ b10 ] ; v = at2(rx1,ry0);
	a = Math::lerp(sx, u, v);

	q = g2[ b01 ] ; u = at2(rx0,ry1);
	q = g2[ b11 ] ; v = at2(rx1,ry1);
	b = Math::lerp(sx, u, v);

	return Math::lerp(sy, a, b);
}


float PerlinNoise::noise3( float vec[3] )
{
	int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
	register int i, j;

	if (start) {
		start = false;
		init();
	}

	setup(0, bx0,bx1, rx0,rx1);
	setup(1, by0,by1, ry0,ry1);
	setup(2, bz0,bz1, rz0,rz1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	t  = s_curve(rx0);
	sy = s_curve(ry0);
	sz = s_curve(rz0);

#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

	q = g3[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
	q = g3[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
	a = Math::lerp(t, u, v);

	q = g3[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
	q = g3[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
	b = Math::lerp(t, u, v);

	c = Math::lerp(sy, a, b);

	q = g3[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
	q = g3[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
	a = Math::lerp(t, u, v);

	q = g3[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
	q = g3[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
	b = Math::lerp(t, u, v);

	d = Math::lerp(sy, a, b);

	return Math::lerp(sz, c, d);
}

void PerlinNoise::normalise2(float v[2])
{
	float s;

	s = sqrtf(v[0] * v[0] + v[1] * v[1]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
}

void PerlinNoise::normalise3( float v[3] )
{
	float s;

	s = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
	v[2] = v[2] / s;
}

void PerlinNoise::init( void )
{
	int i, j, k;

	for (i = 0 ; i < B ; i++) {
		p[i] = i;

		g1[i] = (float)((rand() % (B + B)) - B) / B;

		for (j = 0 ; j < 2 ; j++)
			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalise2(g2[i]);

		for (j = 0 ; j < 3 ; j++)
			g3[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalise3(g3[i]);
	}

	while (--i) {
		k = p[i];
		p[i] = p[j = rand() % B];
		p[j] = k;
	}

	for (i = 0 ; i < B + 2 ; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		for (j = 0 ; j < 2 ; j++)
			g2[B + i][j] = g2[i][j];
		for (j = 0 ; j < 3 ; j++)
			g3[B + i][j] = g3[i][j];
	}
}


/**
 *	Produces a weighted sum of multiple harmonics of basic Perlin noise.
 *	It tries to return a value between 0.0 and 1.0
 *
 *	@param	vec				the input coordinates
 *	@param	persistence		the cumulative relative weighting for each harmonic.
 *	@param	frequency		the frequency of the harmonics
 *	@param	nIterations		the number of harmonics to blend
 */
float PerlinNoise::sumHarmonics2( const Vector2 & vec, float persistence, float frequency, float nIterations )
{
	Vector2 curr(vec);
	curr *= frequency;
	Vector2 norm(vec);
	norm.normalise();
	norm *= frequency;

	float sum = 0.f;
	float scale = 1.f;
	float i = 0.f;

	while (i < nIterations)
	{
		sum += (scale * this->noise2(curr));
		scale *= persistence;
		norm += norm;	//1, 2, 4, 8, 16 etc.
		i += 1.f;
		curr += norm;
	}

	return sum + 0.5f;
}

float IntRand( int x )
{
	x = (x<<13) ^ x;
	return ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f / 2.0f;
}

MyPerlin::MyPerlin()
{
	srand( 0 );
	for( int n = 0; n < 3; n++ )
	{
		for( int i = 0; i < 256; i++ )
			m_aafNoise[n][i] = (float)rand() / RAND_MAX;
	}

	m_afAmp[0] = 1.0f;
	m_afAmp[1] = 15731;
	m_afAmp[2] = 1.0f;

	m_nOctaves = 6;
	m_fPersistence = 0.25f;
}

float MyPerlin::noise1( float t )
{
	float totalAmp = 0;
	float amp = m_fPersistence;
	float freq = 1.0f;

	float noise = 0;
	for( int iOctave = 0; iOctave < m_nOctaves; iOctave++ )
	{
		float curt = t * freq;
		int i0 = (int)curt;
		float k = curt - floor( curt );
		float tr = cosInterp( IntRand( i0 ), IntRand( i0 + 1 ), k );
		noise += tr * amp;
		totalAmp += amp;

		amp *= m_fPersistence;
		freq *= 2.0f;
	}

	return noise / totalAmp;
}

// Undefined so blob builds work
#undef B
#undef BM
#undef N
#undef NP
#undef NM

BW_END_NAMESPACE

// perlin_noise.cpp
