/**
 *	This file contains functions used in multiple unit tests that depend
 *	on the resmgr library.
 */

#pragma once
#ifndef RESMGR_TEST_UTILS_HPP
#define RESMGR_TEST_UTILS_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace ResMgrTest
{
	bool eraseResource( const BW::string & resourceName );
}

BW_END_NAMESPACE

#endif //RESMGR_TEST_UTILS_HPP

