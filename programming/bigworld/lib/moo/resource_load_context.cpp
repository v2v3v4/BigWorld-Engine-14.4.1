
#include "pch.hpp"
#include "resource_load_context.hpp"
#include "cstdmf/concurrency.hpp"


//#define RESLOADCTX_DEBUG_MORE 1

BW_BEGIN_NAMESPACE

namespace
{

const uint MAX_STACK_DEPTH = 10;
const int MAX_REQUESTER_SIZE = 64;


struct ResourceContext
{
	char requester_[ MAX_REQUESTER_SIZE + 1 ];
};


THREADLOCAL( ResourceContext ) s_stack[ MAX_STACK_DEPTH ];
THREADLOCAL( uint ) s_stackPos = 0;


// Must use this so the DLL version of THREADLOCAL compiles (for example, in
// the client's PyModule configuration). This causes the TreadLocal<T> class to
// use it's "operator T &", instead of thinking that ".requester_" is a member
// of TreadLocal<T>.
ResourceContext & stackItem( uint pos )
{
	MF_ASSERT( pos < MAX_STACK_DEPTH );
	return s_stack[ pos ];
}


} // anonymous namespace


namespace Moo
{

/*static*/ void ResourceLoadContext::push( const BW::StringRef & requester )
{
#ifdef RESLOADCTX_DEBUG_MORE
	INFO_MSG( "Loading '%s'\n", requester.to_string().c_str() );
#endif

	if (s_stackPos < MAX_STACK_DEPTH)
	{
		size_t length = std::min<size_t>( requester.size(), MAX_REQUESTER_SIZE );
		memcpy( stackItem( s_stackPos ).requester_, requester.data(), length );
		stackItem( s_stackPos ).requester_[ length ] = '\0';
	}
	++s_stackPos;
}


/*static*/ void ResourceLoadContext::pop()
{
	--s_stackPos;

#ifdef RESLOADCTX_DEBUG_MORE
	if (s_stackPos < MAX_STACK_DEPTH)
	{
		INFO_MSG( "Finished loading '%s'\n", stackItem( s_stackPos ).requester_ );
	}
#endif
}


/*static*/ BW::string ResourceLoadContext::formattedRequesterString()
{
	BW::string ret;

	if (s_stackPos > 0)
	{
		uint pos = std::min( s_stackPos - 1, uint(MAX_STACK_DEPTH - 1) );

		if (strlen( stackItem( pos ).requester_ ) > 0)
		{
			ret.append( " requested by " );
			ret.append( stackItem( pos ).requester_ );
		}
	}

	return ret;
}


} // namespace Moo

BW_END_NAMESPACE

// resource_load_context.cpp

