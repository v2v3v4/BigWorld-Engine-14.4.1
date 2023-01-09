// string_list.cpp
#include "string_list.hpp"

#include "cstdmf/debug.hpp"
#include "curl/curl.h"


BW_BEGIN_NAMESPACE

namespace URL
{


void StringList::add( const BW::string & s )
{
	pValue_ = curl_slist_append( pValue_, s.c_str() );
}

void StringList::clear()
{ 
	curl_slist_free_all( pValue_ );
	pValue_ = NULL;
}

} // namespace URL

BW_END_NAMESPACE

// string_list.cpp

