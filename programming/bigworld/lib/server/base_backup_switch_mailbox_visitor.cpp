#include "script/first_include.hpp"

#include "base_backup_switch_mailbox_visitor.hpp"
#include "backup_hash_chain.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Switches mailboxes over to use the new backup location when a BaseApp has
 *	shut down.
 */
void BaseBackupSwitchMailBoxVisitor::onMailBox( PyEntityMailBox * pMailBox )
{
	EntityMailBoxRef ref;
	PyEntityMailBox::reduceToRef( pMailBox, &ref );

	if (ref.component() == EntityMailBoxRef::SERVICE)
	{
		// Service fragments are not restored, and so should not be remapped.
		return;
	}

	if ((deadAddr_ == Mercury::Address::NONE) || 
			(pMailBox->address() == deadAddr_))
	{
		pMailBox->address( hashChain_.addressFor( 
			pMailBox->address(), pMailBox->id() ) );
	}
}

BW_END_NAMESPACE

// base_backup_switch_mailbox_visitor.cpp

