#include "pch.hpp"

#include "math/math_extra.hpp"


BW_BEGIN_NAMESPACE

uint32 my_largerPow2( uint32 number )
{
	uint32 i = 1;
	uint prevI = 0;
	while (i < number && i > prevI) 
	{
		prevI = i;
		i  *= 2;
	}
	if (i < prevI) 
	{
		return prevI;
	}
	return i;
}

uint32 my_smallerPow2( uint32 number )
{
	return my_largerPow2(number) /2;
}

TEST( POW2 )
{	
	for (uint32 i = 0 ; i < (2 << 15); i ++) 
	{
		CHECK( my_smallerPow2(i) == BW::smallerPow2(i));
		CHECK( my_largerPow2(i) == BW::largerPow2(i));
	}
	for (uint32 i = 0 ; ;i = (uint32)(i * 2/1.5) + 1)
	{
		CHECK( my_smallerPow2(i) == BW::smallerPow2(i));
		CHECK( my_largerPow2(i) == BW::largerPow2(i));
		if (i > powf(2,31))
		{
			break;
		}
	}
}

BW_END_NAMESPACE

// test_math_extra.cpp
