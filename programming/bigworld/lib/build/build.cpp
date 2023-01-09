#include "build.hpp"

#include "build_revision.hpp"

BW_BEGIN_NAMESPACE

Build::Revision Build::revision()
{
	return REVISION_NUMBER;
}

BW_END_NAMESPACE

// build.cpp
