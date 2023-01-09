#include "pch.hpp"

#include "py_player_avatar_filter.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PyPlayerAvatarFilter )

PY_BEGIN_METHODS( PyPlayerAvatarFilter )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyPlayerAvatarFilter )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyPlayerAvatarFilter )

/*~ function BigWorld.PlayerAvatarFilter
 *
 *	This function creates a new PlayerAvatarFilter.  This updates the position
 *	and yaw of the entity to those specified by the most recent server update.
 *
 *	@return	A new PlayerAvatarFilter object
 */
PY_FACTORY_NAMED( PyPlayerAvatarFilter, "PlayerAvatarFilter", BigWorld )


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 */
PyPlayerAvatarFilter::PyPlayerAvatarFilter( PyTypeObject * pType ) :
	PyFilter( pType )
{
	BW_GUARD;
}


/**
 *	This method returns the PlayerAvatarFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
PlayerAvatarFilter * PyPlayerAvatarFilter::pAttachedFilter()
{
	BW_GUARD;

	return static_cast< PlayerAvatarFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	This method returns the const PlayerAvatarFilter * we created, if not yet
 *	lost, and NULL otherwise.
 */
const PlayerAvatarFilter * PyPlayerAvatarFilter::pAttachedFilter() const
{
	BW_GUARD;

	return static_cast< const PlayerAvatarFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	Generate a new PlayerAvatarFilter for the given Entity
 */
PlayerAvatarFilter * PyPlayerAvatarFilter::getNewFilter()
{
	BW_GUARD;

	return new PlayerAvatarFilter( this );
}


BW_END_NAMESPACE

// py_player_avatar_filter.cpp
