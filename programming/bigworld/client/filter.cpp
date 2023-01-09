#include "pch.hpp"

#include "filter.hpp"

#include "app.hpp"
#include "entity_manager.hpp"
#include "py_filter.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 */
Filter::Filter( PyFilter * pOwner ) :
	MovementFilter( EntityManager::instance().filterEnvironment() ),
	wpOwner_( pOwner )
{
}


/**
 *	Destructor
 */
Filter::~Filter()
{
	if (wpOwner_.exists())
	{
		wpOwner_->onFilterDestroyed( this );
	}
}


BW_END_NAMESPACE

// filter.cpp
