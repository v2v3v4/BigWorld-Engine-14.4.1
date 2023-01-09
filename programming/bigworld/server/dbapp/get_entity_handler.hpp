#ifndef GET_ENTITY_HANDLER_HPP
#define GET_ENTITY_HANDLER_HPP

#include "db_storage/idatabase.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is meant to replace IDatabase::IGetEntityHandler as the base
 * 	class for IDatabase::getEntity() callbacks. It allows us to muck around
 * 	with the results before passing them on to the actual handler.
 */
class GetEntityHandler : public IDatabase::IGetEntityHandler
{
public:
	// Intercepts the result callback. Derived classes should NOT implement
	// this method.
	virtual void onGetEntityComplete( bool isOK,
				const EntityDBKey & entityKey,
				const EntityMailBoxRef * pBaseEntityLocation );

	// Derived classes should implement this method instead of
	// onGetEntityComplete() - note the extra 'd'.
	virtual void onGetEntityCompleted( bool isOK,
				const EntityDBKey & entityKey,
				const EntityMailBoxRef * pBaseEntityLocation ) = 0;

	static bool isActiveMailBox( const EntityMailBoxRef * pBaseRef );
};

BW_END_NAMESPACE

#endif // GET_ENTITY_HANDLER_HPP
