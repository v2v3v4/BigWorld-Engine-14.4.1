#ifndef PY_PLAYER_AVATAR_FILTER_HPP
#define PY_PLAYER_AVATAR_FILTER_HPP

#include "player_avatar_filter.hpp"
#include "py_filter.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.PlayerAvatarFilter
 *
 *	This class inherits from Filter.  It updates the position and
 *	yaw of the entity to the most recent update received from the
 *	server.
 *
 *	A new PlayerAvatarFilters can be created using the
 *	BigWorld.PlayerAvatarFilter function.
 */
/**
 *	This is a Python object to manage an PlayerAvatarFilter instance, matching
 *	the lifecycle of a Python object to the (shorter) lifecycle of a
 *	MovementFilter.
 */
class PyPlayerAvatarFilter : public PyFilter
{
	Py_Header( PyPlayerAvatarFilter, PyFilter );

public:
	PyPlayerAvatarFilter( PyTypeObject * pType = &s_type_ );
	virtual ~PyPlayerAvatarFilter() {}

	// Python Interface
	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( PyPlayerAvatarFilter, END );

	// Implementation of PyFilter
	virtual PlayerAvatarFilter * pAttachedFilter();
	virtual const PlayerAvatarFilter * pAttachedFilter() const;

protected:
	// Implementation of PyFilter
	virtual PlayerAvatarFilter * getNewFilter();
};

PY_SCRIPT_CONVERTERS_DECLARE( PyPlayerAvatarFilter );

BW_END_NAMESPACE

#endif // PY_PLAYER_AVATAR_FILTER_HPP
