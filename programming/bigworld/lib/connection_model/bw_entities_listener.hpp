#ifndef BW_ENTITIES_LISTENER_HPP
#define BW_ENTITIES_LISTENER_HPP

BW_BEGIN_NAMESPACE

class BWEntity;

/**
 *	This class is an interface for listeners to BWEntities.
 */
class BWEntitiesListener
{
public:
	virtual void onEntityEnter( BWEntity * pEntity ) = 0;
	virtual void onEntityLeave( BWEntity * pEntity ) = 0;

	virtual void onEntitiesReset() = 0;
};

BW_END_NAMESPACE

#endif // BW_ENTITIES_LISTENER_HPP
