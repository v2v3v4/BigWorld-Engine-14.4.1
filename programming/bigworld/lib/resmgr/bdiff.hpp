#ifndef BDIFF_HPP
#define BDIFF_HPP

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

bool performDiff( const BW::vector<unsigned char>& first, 
				  const BW::vector<unsigned char>& second, 
				  FILE * diff );

BW_END_NAMESPACE

#endif
