#include "pch.hpp"

#include "bit_reader.hpp"

#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BitReader
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
BitReader::BitReader( BinaryIStream & data ) :
	data_( data ), bitsLeft_( 0 ), byte_( 0 )
{
}


/**
 *	Destructor.
 */
BitReader::~BitReader()
{
	if ((bitsLeft_ > 0) && (byte_ != 0))
	{
		ERROR_MSG( "BitReader::~BitReader: bitsLeft_ = %d. byte_ = 0x%x\n",
				bitsLeft_, byte_ );
	}
}


/**
 *	This method reads the specified number of bits from the bit stream and
 *	returns them as an integer.
 */
int BitReader::get( int nbits )
{
	int	ret = 0;

	int gbits = 0;
	while (gbits < nbits)	// not as efficient as the writer...
	{
		if (bitsLeft_ == 0)
		{
			byte_ = *(uint8*)data_.retrieve( 1 );
			bitsLeft_ = 8;
		}

		int bitsTake = std::min( nbits-gbits, bitsLeft_ );
		ret = (ret << bitsTake) | (byte_ >> (8-bitsTake));
		byte_ <<= bitsTake;
		bitsLeft_ -= bitsTake;
		gbits += bitsTake;
	}

	return ret;
}


/**
 *	This method reads the specified number of bits from the bit stream and
 *	returns them as an integer, treating it as a 2's-complement signed value.
 */
int BitReader::getSigned( int nbits )
{
	int	ret = 0;

	int gbits = 0;
	while (gbits < nbits)	// not as efficient as the writer...
	{
		if (bitsLeft_ == 0)
		{
			byte_ = *(uint8*)data_.retrieve( 1 );
			bitsLeft_ = 8;
		}

		int bitsTake = std::min( nbits-gbits, bitsLeft_ );
		ret = (ret << bitsTake) | (byte_ >> (8-bitsTake));
		byte_ <<= bitsTake;
		bitsLeft_ -= bitsTake;
		gbits += bitsTake;
	}

	const int intBits = sizeof( int ) * 8;
	if (nbits != intBits && (ret & ( 0x1 << (nbits-1))))
	{
		// Sign-extend this bit to the top of 'int'
		unsigned int signAdd = 0x1 << nbits;
		for (int i = nbits; i < intBits; ++i)
		{
			ret |= signAdd;
			signAdd <<= 1;
		}
	}

	return ret;
}


/**
 *	This function returns the number of bits required to identify the input
 *	number of values.
 *
 *	It is ceil( log( numValues, 2 ) ). That is, the log base 2 of the input
 *	number rounded up.
 */
int BitReader::bitsRequired( uint numValues )
{
	if (numValues <= 1)
	{
		return 0;
	}

	// get the expected bit width of the current index
	--numValues;

	register int nbits;
#if defined( __clang__ ) || defined( __ANDROID__ ) || defined(_WIN64) || \
		defined( EMSCRIPTEN )
	nbits = 0;

	while (numValues > 1)
	{
		numValues = numValues >> 1;
		++nbits;
	}
#elif defined( _WIN32 )
	_asm bsr eax, numValues	// Bit Scan Reverse(numValues)->eax
		_asm mov nbits, eax		// eax-> nbits
#else
	__asm__ (
		"bsr 	%1, %%eax" 	// Bit Scan Reverse(numValues)->eax
		:"=a"	(nbits) 	// output eax: nbits
		:"r"	(numValues)	// input %1: numValues
	);
#endif
	return nbits + 1;
}

BW_END_NAMESPACE

// bit_reader.cpp
