#pragma once
#ifndef BUILD_CONFIG_HPP
#define BUILD_CONFIG_HPP

#include "bw_string.hpp"

BW_BEGIN_NAMESPACE

const BW::string BW_CONFIG_HYBRID( "hybrid" );
const BW::string BW_CONFIG_DEBUG( "debug" );

const BW::string BW_CONFIG_HYBRID_OLD( "Hybrid" );
const BW::string BW_CONFIG_DEBUG_OLD( "Debug" );

const BW::string BW_COMPILE_TIME_CONFIG(
#if defined (_HYBRID )
	BW_CONFIG_HYBRID
#elif defined( _DEBUG )
	BW_CONFIG_DEBUG
#else
	"Unknown Config"
#error "Unknown build config"
#endif
);

BW_END_NAMESPACE

#endif // BUILD_CONFIG_HPP
