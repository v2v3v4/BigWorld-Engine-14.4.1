#ifndef CSTDMF_FOURCC_HPP
#define CSTDMF_FOURCC_HPP

#include "cstdmf/stdmf.hpp"

// Compile-time fourcc DWORD.
#define DECLARE_FOURCC(ch0, ch1, ch2, ch3)                    	\
	((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |       \
	((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))

namespace BW {

struct FourCC
{
	FourCC() : value_(0) {}
	FourCC( uint32 value ) : value_(value) {}
	FourCC( const char fourcc[4] )
	{
		fourcc_[0] = fourcc[0];
		fourcc_[1] = fourcc[1];
		fourcc_[2] = fourcc[2];
		fourcc_[3] = fourcc[3];
	}
	FourCC( char a, char b, char c, char d )
	{
		fourcc_[0] = a;
		fourcc_[1] = b;
		fourcc_[2] = c;
		fourcc_[3] = d;
	}

	union
	{
		uint32	value_;
		char	fourcc_[4];
	};

	bool operator==(const FourCC& other) const { return value_ == other.value_; }
	bool operator!=(const FourCC& other) const { return value_ != other.value_; }
};

} // namespace BW


#endif // CSTDMF_FOURCC_HPP
