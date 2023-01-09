#ifndef BIT_WRITER_HPP
#define BIT_WRITER_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BitWriter
// -----------------------------------------------------------------------------

/**
 *	This class is used to manage writing to a stream of bits.
 */
class CSTDMF_DLL BitWriter
{
public:
	BitWriter();

	void add( int numBits, int bits );

	int		usedBytes() const 		{ return byteCount_ + (bitsLeft_ != 8); }
	const void * bytes() const		{ return bytes_; }

private:
	int		byteCount_;
	int		bitsLeft_;

	// TODO: Remove magic number. Maybe take a BinaryOStream instead.
	uint8	bytes_[224];
};

BW_END_NAMESPACE

#endif // BIT_WRITER_HPP
