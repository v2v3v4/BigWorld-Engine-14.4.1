#include "pch.hpp"

#include "netmask.hpp"

#include "cstdmf/debug.hpp"

#include <stdio.h>

BW_BEGIN_NAMESPACE

/**
 *	Default constructor.
 */
NetMask::NetMask() :
	mask_(0),
	bits_(0)
{
}


/**
 *	This method parses a netmask in the format "a.b.c.d/bits". Bits is the
 *	number of bits that must match. So for a class B address, it is 16, for a
 *	class C address it is 24, etc. To match any address, set bits to zero.
 *
 *	@param str	The netmask as a string
 *
 *	@return true if parsed successfully.
 */
bool NetMask::parse( const char* str )
{
	int a, b, c, d;

	if (sscanf( str, "%d.%d.%d.%d/%d", &a, &b, &c, &d, &bits_ ) != 5)
	{
		bits_ = 0; // match any address
		return false;
	}

	// TODO: Make endian independent
//	mask_ = (a << 24) | (b << 16) | (c << 8) | d;
	mask_ = (d << 24) | (c << 16) | (b << 8) | a;
	return true;
}


/**
 *	This method returns true if the given address is a member of this subnet.
 *
 *	@param addr	The address, in network byte order.
 *
 *	@return true if the address is a member of this subnet.
 */
bool NetMask::containsAddress( uint32 addr ) const
{
	if (bits_ == 0)
		return true; // Need to do this because n << 32 == n

	uint32 bitsOfInterest = (0xffffffff >> (32 - bits_));

	return (addr & bitsOfInterest) == (mask_ & bitsOfInterest);
}


/**
 *	This method clears this netmask.
 */
void NetMask::clear()
{
	bits_ = 0;
	mask_ = 0;
}

BW_END_NAMESPACE

// netmask.cpp
