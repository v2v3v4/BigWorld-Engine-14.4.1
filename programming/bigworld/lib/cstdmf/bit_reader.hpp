#ifndef BIT_READER_HPP
#define BIT_READER_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;

/**
 *	This class is used to read from a stream of bits.
 */
class BitReader
{
public:
	CSTDMF_DLL BitReader( BinaryIStream & data );
	CSTDMF_DLL ~BitReader();

	CSTDMF_DLL int get( int nbits );
	CSTDMF_DLL int getSigned( int nbits );

	CSTDMF_DLL static int bitsRequired( uint numValues );

private:
	BinaryIStream & data_;
	int	bitsLeft_;
	uint8 byte_;
};

BW_END_NAMESPACE

#endif // BIT_READER_HPP
