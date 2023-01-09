#ifndef MIGRATE_MAILBOX_VISITOR_HPP
#define MIGRATE_MAILBOX_VISITOR_HPP

#include "entitydef/mailbox_base.hpp"


BW_BEGIN_NAMESPACE

class MigrateMailBoxVisitor : public PyEntityMailBoxVisitor
{
public:
	MigrateMailBoxVisitor() {}
	virtual ~MigrateMailBoxVisitor() {}

	virtual void onMailBox( PyEntityMailBox * pMailBox )
	{
		pMailBox->migrate();
	}
};

BW_END_NAMESPACE

#endif // MIGRATE_MAILBOX_VISITOR_HPP

