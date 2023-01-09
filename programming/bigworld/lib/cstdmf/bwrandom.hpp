#ifndef BWRANDOM_HPP
#define BWRANDOM_HPP

#include "stdmf.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class provides a random number generator based upon the Mersenne 
 *	twister:
 *
 *	http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 *
 *	It is about one and a half-times faster than rand(), is more random,
 *	has a nicer interface (it is easier to set bounds on the random numbers)
 *	and can allow multiple random number generators at once.
 */
class CSTDMF_DLL BWRandom
{
public:
    BWRandom();
    explicit BWRandom(uint32 seed);
    BWRandom(uint32 const *seed, size_t size);

    uint32 operator()();
    int operator()(int min, int max);
	float operator()(float min, float max);

	// make it a singleton because it might be
	// called from other static initialisations
	static BWRandom& instance()
	{
		static BWRandom s_theRandom;
		return s_theRandom;
	}
protected:
    void init(uint32 seed);
    void init(uint32 const *seed, size_t size);

    uint32 generate();

private:
    uint32                  mt[624]; // state vector
    int                     mti;
};


/**
 *	You can use this, rather than create a Random of your own.  
 */
#define bw_random (BWRandom::instance())

BW_END_NAMESPACE

#endif // BWRANDOM_HPP
