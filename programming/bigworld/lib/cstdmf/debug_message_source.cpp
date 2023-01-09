#include "pch.hpp"

#include "stdmf.hpp"

#include "debug_message_source.hpp"

BW_BEGIN_NAMESPACE

static const char * messageSources[] =
{
	"C++",
	"Script"
};

static const char * invalidMessageSource = "(Unknown)";

const char * messageSourceAsCString( DebugMessageSource sourceIndex )
{
	if ((sourceIndex < 0) ||
		((size_t)sourceIndex >= (size_t)ARRAY_SIZE( messageSources )))
	{
		return invalidMessageSource;
	}

	return messageSources[ sourceIndex ];
}

BW_END_NAMESPACE

// debug_message_source.cpp
