#ifndef RESOURCE_CALLBACKS_HPP
#define RESOURCE_CALLBACKS_HPP

#include "cstdmf/bw_string_ref.hpp"

BW_BEGIN_NAMESPACE

struct ResourceCallbacks
{
	virtual void purgeResource( const StringRef & resourceId );
};

BW_END_NAMESPACE

#endif