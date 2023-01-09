#include "pch.hpp"
#include "math/simplex_noise.hpp"
#include <cmath>

BW_BEGIN_NAMESPACE

namespace
{
    const float F2      = 0.366025404f; // 0.5*(sqrt(3.0) - 1.0)
    const float G2      = 0.211324865f; // (3.0 - sqrt(3.0))/6.0

	// Vertices on the 3d cube.
    const int grad3[][3] = 
    {
        {+1, +1,  0}, {-1, +1,  0}, {+1, -1,  0}, {-1, -1,  0},
        {+1,  0, +1}, {-1,  0, +1}, {+1,  0, -1}, {-1,  0, -1},
        { 0, +1, +1}, { 0, -1, +1}, { 0, +1, -1}, { 0, -1, -1}
    };

	// The numbers 0..255 randomly permutted. 
    int perm[] = 
    {
        151, 160, 137,  91,  90,  15, 131,  13, 
        201,  95,  96,  53, 194, 233,   7, 225, 
        140,  36, 103,  30,  69, 142,   8,  99, 
         37, 240,  21,  10,  23, 190,   6, 148, 
        247, 120, 234,  75,   0,  26, 197,  62, 
         94, 252, 219, 203, 117,  35,  11,  32, 
         57, 177,  33,  88, 237, 149,  56,  87,
        174,  20, 125, 136, 171, 168,  68, 175, 
         74, 165,  71, 134, 139,  48,  27, 166, 
         77, 146, 158, 231,  83, 111, 229, 122, 
         60, 211, 133, 230, 220, 105,  92,  41, 
         55,  46, 245,  40, 244, 102, 143,  54, 
         65,  25,  63, 161,   1, 216,  80,  73, 
        209,  76, 132, 187, 208,  89,  18, 169,
        200, 196, 135, 130, 116, 188, 159,  86,
        164, 100, 109, 198, 173, 186,   3,  64,
         52, 217, 226, 250, 124, 123,   5, 202,
         38, 147, 118, 126, 255,  82,  85, 212,
        207, 206,  59, 227,  47,  16,  58,  17, 
        182, 189,  28,  42, 223, 183, 170, 213, 
        119, 248, 152,   2,  44, 154, 163,  70,
        221, 153, 101, 155, 167,  43, 172,   9,
        129,  22,  39, 253,  19,  98, 108, 110,
         79, 113, 224, 232, 178, 185, 112, 104, 
        218, 246,  97, 228, 251,  34, 242, 193, 
        238, 210, 144,  12, 191, 179, 162, 241, 
         81,  51, 145, 235, 249,  14, 239, 107, 
         49, 192, 214,  31, 181, 199, 106, 157,
        184,  84, 204, 176, 115, 121,  50,  45,
        127,   4, 150, 254, 138, 236, 205,  93,
        222, 114,  67,  29,  24,  72, 243, 141,
        128, 195,  78,  66, 215,  61, 156, 180
    };

	// Apparently a faster version of the floor function.
    inline int fastFloor(float x)
    {
        return x > 0.0f ? (int)x : (int)x - 1;
    }

	// Integer,float dot product
    inline float dot(int const *g, float x, float y)
    {
        return g[0]*x + g[1]*y;
    }
}


/**
 *  This constructs an empty octave of noise.
 */
SimplexNoise::Octave::Octave():
    waveLength_(0.0f),
    weight_(0.0f),
    seed_(0),
    frequency_(0.0f)
{
}


/**
 *  This constructs an octave of noise.
 *
 *  @param waveLength   The wavelength of the noise.
 *  @param weight       The relative weight of this octave.
 *  @param seed         The seed of this octave.
 */
SimplexNoise::Octave::Octave(float wavelength, float weight, int seed /*= 0*/):
    waveLength_(wavelength),
    weight_(weight),
    seed_(seed),
    frequency_(wavelength > 0.0f ? 1.0f/wavelength : 0.0f)
{
}


/**
 *  Calculate the simplex noise at xin, yin.
 *
 *  @param xin      The x-coordinate.
 *  @param yin      The y-coordinate.
 *  @param seed     The seed of the noise.
 *  @returns        The noise in the range [0, 1] at the point (xin, yin).
 */
/*static*/ float SimplexNoise::generate(float xin, float yin, int seed)
{
    // Skew into the simplex cell:
    float s  = (xin + yin)*F2;
    int   i  = fastFloor(xin + s);
    int   j  = fastFloor(yin + s);
    float t  = (i + j)*G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = xin - X0;
    float y0 = yin - Y0;

    // Determine which triangle we are in:
    int i1, j1;
    if (x0 > y0)
    {
        i1 = 1; j1 = 0; // lower triangle
    }
    else
    {
        i1 = 0; j1 = 1; // upper triangle
    }

	// Unskew:
    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f*G2;
    float y2 = y0 - 1.0f + 2.0f*G2;

    // Work out the hashed gradient:
    int ii  = i & 255;
    int jj  = j & 255;
    int gi0 = perm[(seed + ii      + perm[(seed + jj     ) & 255]) & 255]%12;
    int gi1 = perm[(seed + ii + i1 + perm[(seed + jj + j1) & 255]) & 255]%12;
    int gi2 = perm[(seed + ii +  1 + perm[(seed + jj +  1) & 255]) & 255]%12; 

    // Calculate the contribution from the corners:
    float n0, n1, n2;
    float t0 = 0.5f - x0*x0 - y0*y0;
    if (t0 < 0.0f)
    {
        n0 = 0.0f;
    }
    else
    {
        t0 *= t0;
        n0  = t0*t0*dot(grad3[gi0], x0, y0);
    }
    float t1 = 0.5f - x1*x1 - y1*y1;
    if (t1 < 0.0f)
    {
        n1 = 0.0f;
    }
    else
    {
        t1 *= t1;
        n1 = t1*t1*dot(grad3[gi1], x1, y1);
    }
    float t2 = 0.5f - x2*x2 - y2*y2;
    if (t2 < 0.0f)
    {
        n2 = 0.0f;
    }
    else
    {
        t2 *= t2;
        n2 = t2*t2*dot(grad3[gi2], x2, y2);
    }

    // Add the contributions and normalise to the range [0.0, 1.0]
    return 35.0f*(n0 + n1 + n2) + 0.5f;
}


/**
 *  This creates an empty SimplexNoise.
 */
SimplexNoise::SimplexNoise() :
	totalWeight_( 0.f )
{
}


/**
 *	This compares this SimplexNoise against another. 
 *
 *	@param other	The SimplexNoise to compare against.
 *	@return			True if this SimplexNoise is the same as other, false
 *					otherwise.
 */
bool SimplexNoise::operator==( const SimplexNoise & other ) const
{
	if (octaves_.size() != other.octaves_.size())
	{
		return false;
	}

	for (size_t i = 0; i < octaves_.size(); ++i)
	{
		const SimplexNoise::Octave & myOct    = octaves_[ i ];
		const SimplexNoise::Octave & otherOct = other.octaves_[ i ];

		if (myOct.waveLength_ != otherOct.waveLength_ 
			||
			myOct.weight_ / totalWeight_ != otherOct.weight_ / other.totalWeight_
			||
			myOct.seed_       != otherOct.seed_ )
		{
			return false;
		}
	}

	return true;
}


/**
 *	This compares this SimplexNoise against another. 
 *
 *	@param other	The SimplexNoise to compare against.
 *	@return			False if this SimplexNoise is the same as other, true
 *					otherwise.
 */
bool SimplexNoise::operator!=( const SimplexNoise & other ) const
{
	return !operator==( other );
}


/**
 *  This evaluates the noise at the given location.
 *
 *  @param x        The x coordinate.
 *  @param y        The y coordinate.
 *  @return         The value of the noise at (x, y).  The result is in the 
 *					range [0.0, 1.0].
 */
float SimplexNoise::operator()(float x, float y) const
{
    float value = 0.0f;
    for (size_t i = 0; i < octaves_.size(); ++i)
    {
        Octave const &octave = octaves_[i];
        value += octave.weight_ / totalWeight_ * generate( 
				octave.frequency_ * x, octave.frequency_ * y, octave.seed_ );
    }
    return value;
}


/**
 *  This sets the octaves used to generate the noise.  We ignore octaves whose
 *  frequency or weight is negative.  We also reweight the octaves so that the
 *  sum of the weights is one.
 *
 *  @param oct      The new octaves.
 */
void SimplexNoise::octaves(OctaveVec const &oct)
{
    octaves_.clear();
    octaves_.reserve(oct.size());
	totalWeight_ = 0.f;

    // Add the valid octaves and sum the weights:
    for (size_t i = 0; i < oct.size(); ++i)
    {
        Octave o = oct[i];
        if (o.waveLength_ > 0.0f && o.weight_ > 0.0f)
        {
            octaves_.push_back(o);
            totalWeight_ += o.weight_;
        }
    }

    // Calculate the frequencies:
    for (size_t i = 0; i < octaves_.size(); ++i)
    {
        Octave &o = octaves_[i];
        o.frequency_ = 1.0f/o.waveLength_;
    }
}


/**
 *  This gets the octaves used to generate the noise.
 *
 *  @return         The octaves used to generate the noise.
 */
SimplexNoise::OctaveVec const &SimplexNoise::octaves() const
{
    return octaves_;
}

BW_END_NAMESPACE

// simplex_noise.cpp
