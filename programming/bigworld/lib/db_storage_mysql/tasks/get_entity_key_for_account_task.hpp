#ifndef GET_ENTITY_KEY_FOR_ACCOUNT_TASK_HPP
#define GET_ENTITY_KEY_FOR_ACCOUNT_TASK_HPP

#include "background_task.hpp"
#include "cstdmf/value_or_null.hpp"
#include "db_storage/billing_system.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class encapsulates a mapLoginToEntityKey() operation so that it can
 *	be executed in a separate thread.
 */
class GetEntityKeyForAccountTask : public MySqlBackgroundTask
{
public:
	GetEntityKeyForAccountTask( const BW::string & logOnName,
		const BW::string & password,
		bool shouldAcceptUnknownUsers,
		bool shouldRememberUnknownUsers,
		EntityTypeID entityTypeIDForUnknownUsers,
		IGetEntityKeyForAccountHandler & handler );

	// MySqlBackgroundTask overrides
	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

protected:
	void onException( const DatabaseException & e );

private:
	// Input values
	BW::string			logOnName_;
	BW::string			password_;

	// Output values
	LogOnStatus			loginStatus_;
	ValueOrNull< EntityTypeID >		entityTypeID_;
	EntityTypeID					defaultEntityTypeID_;
	ValueOrNull< DatabaseID >		databaseID_;

	bool shouldAcceptUnknownUsers_;
	bool shouldRememberUnknownUsers_;

	IGetEntityKeyForAccountHandler & handler_;
};

BW_END_NAMESPACE

#endif // GET_ENTITY_KEY_FOR_ACCOUNT_TASK_HPP
