#ifndef SET_ENTITY_KEY_FOR_ACCOUNT_HPP
#define SET_ENTITY_KEY_FOR_ACCOUNT_HPP

#include "background_task.hpp"

#include "db_storage/idatabase.hpp"


BW_BEGIN_NAMESPACE

class EntityKey;
class ISetEntityKeyForAccountHandler;
// -----------------------------------------------------------------------------
// Section: SetEntityKeyForAccountTask
// -----------------------------------------------------------------------------

/**
 *	This class encapsulates the setLoginMapping() operation so that it can be
 *	executed in a separate thread.
 */
class SetEntityKeyForAccountTask : public MySqlBackgroundTask
{
public:
	SetEntityKeyForAccountTask( const BW::string & username,
			const BW::string & password, const EntityKey & ekey,
			ISetEntityKeyForAccountHandler & handler );

	// MySqlBackgroundTask overrides
	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );
	virtual void onException( const DatabaseException & e );

private:
	BW::string username_;
	BW::string password_;
	EntityKey entityKey_;
	ISetEntityKeyForAccountHandler & handler_;
};

BW_END_NAMESPACE

#endif // SET_ENTITY_KEY_FOR_ACCOUNT_HPP
