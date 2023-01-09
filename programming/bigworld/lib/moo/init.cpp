#include "pch.hpp"
#include "init.hpp"

#include "render_context.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

// Is this library initialised?
static bool	    s_initialised	= false;

// Library globals
RenderContext*  g_RC			= NULL;

bool isInitialised()
{
	return s_initialised;
}

bool init( bool d3dExInterface, bool assetProcessingOnly )
{
	BW_GUARD;
	if ( !s_initialised )
	{
		// Create render context
		MF_ASSERT_DEV( g_RC == NULL );
		if( g_RC == NULL )
		{
			g_RC = new RenderContext();
			REGISTER_SINGLETON_FUNC( RenderContext, &Moo::rc )
		}
		s_initialised = g_RC->init( d3dExInterface, assetProcessingOnly );

		return s_initialised;
	}

	return true;
}

void fini()
{
	BW_GUARD;
	if ( s_initialised )
	{
		// Destroy render context
		g_RC->fini();
		bw_safe_delete(g_RC);

		s_initialised = false;
	}
}

} // namespace Moo

BW_END_NAMESPACE

// init.cpp
