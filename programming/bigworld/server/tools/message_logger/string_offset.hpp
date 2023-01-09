#ifndef STRING_OFFSET_HPP
#define STRING_OFFSET_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class StringOffset;
typedef BW::vector< StringOffset > StringOffsetList;


#pragma pack( push, 1 )
class StringOffset
{
public:
	StringOffset() {}
	StringOffset( int start, int end ) : start_( start ), end_( end ) {}

	uint16 start_;
	uint16 end_;
};
#pragma pack( pop )

BW_END_NAMESPACE

#endif // STRING_OFFSET_HPP

