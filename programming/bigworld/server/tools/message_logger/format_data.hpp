#ifndef FORMAT_DATA_HPP
#define FORMAT_DATA_HPP

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

#pragma pack( push, 1 )

class FormatData
{
public:
	FormatData() {}
	FormatData( char type, int cflags, int base,
		int min, int max, int flags, int vflags ) :
		type_( type ), cflags_( cflags ), vflags_( vflags ), base_( base ),
		min_( min ), max_( max ), flags_( flags ) {}

	char type_;
	unsigned cflags_:4;
	unsigned vflags_:4;
	uint8 base_;
	int min_;
	int max_;
	int flags_;
};
#pragma pack( pop )

BW_END_NAMESPACE

#endif // FORMAT_DATA_HPP
