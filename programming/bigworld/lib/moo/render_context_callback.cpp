
#include "pch.hpp"
#include "render_context_callback.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{


// This flag is used to check that no new callbacks are created after fini.
/*static*/ bool RenderContextCallback::s_finalised_ = false;

// This vector holds pointers to all created callbacks to clear them on fini.
/*static*/ RenderContextCallback::Callbacks
										RenderContextCallback::s_callbacks_;


/**
 *	Constructor. Registers the created class so its resources can be
 *	released later.
 */
RenderContextCallback::RenderContextCallback()
{
	BW_GUARD;
	MF_ASSERT_DEV( !s_finalised_ );

	if( !s_finalised_ )
		s_callbacks_.push_back( this );
}


/**
 *	This static method calls the "clear" method of all registered classes
 *	in order to allow them to release their resources.
 */
/*static*/ void RenderContextCallback::fini()
{
	BW_GUARD;
	for (Callbacks::iterator it = s_callbacks_.begin();
		it != s_callbacks_.end(); ++it)
	{
		(*it)->renderContextFini();
	}
	s_callbacks_.clear();
	s_finalised_ = true;
}


} // namespace Moo

BW_END_NAMESPACE

// render_context_callback.cpp
