#include "cstdmf/bw_string.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/allocator.hpp"

#include <iostream>


BW_BEGIN_NAMESPACE

namespace BWUnitTest
{

int runTest( const BW::string & testName, int argc, char* argv[] );

int unitTestError( const char * _Format, ... );
int unitTestInfo( const char * _Format, ... );

}

BW_END_NAMESPACE

// unit_test.hpp
