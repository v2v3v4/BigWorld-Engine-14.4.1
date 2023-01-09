#include "baseapp.hpp"
#include "baseappmgr.hpp"
#include "baseappmgr_config.hpp"

#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: BaseApp
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
BaseApp::BaseApp( BaseAppMgr & baseAppMgr, 
			const Mercury::Address & intAddr,
			const Mercury::Address & extAddr,
			int id,
			bool isServiceApp ) :
	ChannelOwner( baseAppMgr.interface(), intAddr ),
	baseAppMgr_( baseAppMgr ),
	externalAddr_( extAddr ),
	id_( id ),
	load_( 0.f ),
	numBases_( 0 ),
	numProxies_( 0 ),
	backupHash_(),
	newBackupHash_(),
	isRetiring_( false ),
	isOffloading_( false ),
	isServiceApp_( isServiceApp )
{
	this->channel().isLocalRegular( false );
	this->channel().isRemoteRegular( true );
}


/**
 *	This method estimates the cost of adding an entity to the BaseApp.
 */
void BaseApp::addEntity()
{
	// TODO: Make this configurable and consider having different costs for
	// different entity types.
	load_ += 0.01f;
}


/**
 *	This static method makes a watcher associated with this object type.
 */
WatcherPtr BaseApp::pWatcher()
{
	// generic watcher of a BaseApp structure
	WatcherPtr pWatcher = new DirectoryWatcher();
	BaseApp * pNullBaseApp = NULL;

	pWatcher->addChild( "id", makeWatcher( &BaseApp::id_ ) );
	pWatcher->addChild( "internalChannel",
			Mercury::ChannelOwner::pWatcher(),
			(ChannelOwner *)pNullBaseApp );
	pWatcher->addChild( "externalAddr", &Mercury::Address::watcher(),
		&pNullBaseApp->externalAddr_ );
	pWatcher->addChild( "load", makeWatcher( &BaseApp::load_ ) );
	pWatcher->addChild( "numBases", makeWatcher( &BaseApp::numBases_ ) );
	pWatcher->addChild( "numProxies", makeWatcher( &BaseApp::numProxies_ ) );
	pWatcher->addChild( "isServiceApp", makeWatcher( &BaseApp::isServiceApp_ ) );

	pWatcher->addChild( "backupHash",
			BackupHash::pWatcher(), &pNullBaseApp->backupHash_ );
	pWatcher->addChild( "newBackupHash",
			BackupHash::pWatcher(), &pNullBaseApp->newBackupHash_ );

	return pWatcher;
}


/**
 *	This method returns whether or not the Base App Manager has heard
 *	from this Base App in the timeout period.
 */
bool BaseApp::hasTimedOut( uint64 currTime, uint64 timeoutPeriod ) const
{
	bool result = false;

	uint64 diff = currTime - this->channel().lastReceivedTime();
	result = (diff > timeoutPeriod);

	if (result)
	{
		PROC_IP_INFO_MSG( "BaseApp::hasTimedOut: Timed out - %.2f (> %.2f) %s\n",
				double( (int64)diff )/stampsPerSecondD(),
				double( (int64)timeoutPeriod )/stampsPerSecondD(),
				this->addr().c_str() );
	}

	return result;
}


void BaseApp::retireApp()
{
	isRetiring_ = true;

	baseAppMgr_.onBaseAppRetire( *this );
	this->checkToStartOffloading();
}


void BaseApp::startBackup( const Mercury::Address & addr )
{
	backingUp_.insert( addr );
}


void BaseApp::stopBackup( const Mercury::Address & addr )
{
	backingUp_.erase( addr );
	this->checkToStartOffloading();
}


/**
 *  A retiring app must retain the same backup hash throughout the offload 
 *  process or the entities' positions may not match where they are expected to
 *  be. Thus there is an isOffloading_ flag which marks a baseapp as having an
 *  immutable backup hash. Before that is set, we must check if the backup hash
 *  is suitable for the entire offloading process.
 */
void BaseApp::checkToStartOffloading()
{
	if (!this->isRetiring() || this->isOffloading())
	{
		return;
	}

	const char* appName = this->isServiceApp() ? "ServiceApp" : "BaseApp";

	// Most importantly, the backup hash must be useful before we commit to it.
	if (backupHash_.empty())
	{
		WARNING_MSG( "%s::checkToStartOffloading: Unable to retire "
				"%s %s. Backup hash is empty.\n",
			appName, appName, this->addr().c_str() );
		return;
	}

	// If an app dies during retirement, its entities should be restored
	// where they would have been sent, to prevent two instances from emerging.
	if (!newBackupHash_.empty())
	{
		WARNING_MSG( "%s::checkToStartOffloading: Unable to retire "
				"%s %s. New backup hash is not empty.\n",
			appName, appName, this->addr().c_str() );
		return;
	}

	// The logic behind this is that we need to check to see if there is any
	// chance that a baseapp crashing during the offload sequence will need to
	// be restored to the offloading app and thereafter be offloaded to the
	// crashed app ad infinitum. This case prevents the system from finding 
	// anywhere logical to put it and should be avoided.
	BackingUp::iterator iter;
	for (iter = backingUp_.begin(); iter != backingUp_.end(); ++iter)
	{
		BaseApp * pBaseApp = BaseAppMgr::instance().findBaseApp( *iter );
		// An offloading app has nothing but offloading apps backing up to it.
		// since they are added sequentially there is no chance for a cycle.
		if (!pBaseApp)
		{
			WARNING_MSG( "%s::checkToStartOffloading: Unable to retire "
					"%s %s. Backing up %s %s not found.\n",
				appName, appName, this->addr().c_str(), appName, 
				iter->c_str() );
			return;
		}
		if (!pBaseApp->isOffloading())
		{
			WARNING_MSG( "%s::checkToStartOffloading: Unable to retire "
					"%s %s. %s %s is still using it for backups "
					"not offloads.\n",
				appName, appName, this->addr().c_str(), appName, 
				pBaseApp->addr().c_str() );
			return;
		}
	}

	// From here on in, we are committed to our current backup hash from the
	// first entity transfer to the final redirection of base mailboxes.
	isOffloading_ = true;

	this->bundle().startMessage( BaseAppIntInterface::startOffloading );
	this->send();
}


/**
 *	This method adjusts the backup locations for this BaseApp. It is called
 *	when a BaseApp is added or removed from BaseAppMgr.
 */
void BaseApp::adjustBackupLocations( const Mercury::Address & newAddr,
		BaseAppsIterator baseApps,
		AdjustBackupLocationsOp op,
		bool hadIdealBackup, bool hasIdealBackup )
{
	if (newBackupHash_.empty())
	{
		newBackupHash_ = this->backupHash();
	}
	else
	{
		// Stay with the previous newBackupHash
		WARNING_MSG( "BaseApp::adjustBackupLocations: "
					"%s was still transitioning to a new hash.\n",
				this->addr().c_str() );
	}

	if (!hadIdealBackup && hasIdealBackup)
	{
		// If we could previously be backed up to, we can assume that
		// it was because there were no ideal places to backup to
		MF_ASSERT( op == ADJUST_BACKUP_LOCATIONS_OP_ADD );
		newBackupHash_.clear();
	}
	else if (hadIdealBackup && !hasIdealBackup)
	{
		MF_ASSERT( op != ADJUST_BACKUP_LOCATIONS_OP_ADD );
		MF_ASSERT( this->isServiceApp() || newBackupHash_.size() <= 1 );

		newBackupHash_.clear();

		// If backing up to the same machine was prohibited previously,
		// make a fully connected set
		while (const BaseAppPtr & inner = baseApps.next())
		{
			if ((inner->addr() != newAddr) &&
				(inner->addr() != this->addr()) &&
				!inner->isRetiring())
			{
				newBackupHash_.push_back( inner->addr() );
			}
		}
	}

	switch (op)
	{
	case ADJUST_BACKUP_LOCATIONS_OP_ADD:
	{
		const bool isIdealMatch = (newAddr.ip != this->addr().ip);

		if (isIdealMatch || !hasIdealBackup)
		{
			newBackupHash_.push_back( newAddr );
		}
		break;
	}

	case ADJUST_BACKUP_LOCATIONS_OP_CRASH:
		if (this->backupHash().erase( newAddr ))
		{
			// The current backup is no longer valid.
			this->backupHash().clear();
		}
		// Fall through

	case ADJUST_BACKUP_LOCATIONS_OP_RETIRE:
		newBackupHash_.erase( newAddr );
		break;

	default:
		CRITICAL_MSG( "BaseApp::adjustBackupLocations: "
			"Invalid operation type: %d", op );
		break;
	}

	Mercury::Bundle & bundle = this->bundle();
	bundle.startMessage( BaseAppIntInterface::setBackupBaseApps );
	bundle << newBackupHash_;
	this->send();
}


/**
 *	This class is used to handle the changes to the hash once new hash has been
 *	primed.
 */
class FinishSetBackupDiffVisitor : public BackupHash::DiffVisitor
{
public:
	FinishSetBackupDiffVisitor( const Mercury::Address & realBaseAppAddr ) :
		realBaseAppAddr_( realBaseAppAddr )
	{}

	virtual void onAdd( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		BaseApp * pBaseApp = BaseAppMgr::instance().findBaseApp( addr );

		if (pBaseApp)
		{
			Mercury::Bundle & bundle = pBaseApp->bundle();
			BaseAppIntInterface::startBaseEntitiesBackupArgs & args =
				BaseAppIntInterface::startBaseEntitiesBackupArgs::start(
					bundle );

			args.realBaseAppAddr = realBaseAppAddr_;
			args.index = index;
			args.hashSize = virtualSize;
			args.prime = prime;
			args.isInitial = false;

			pBaseApp->send();

			pBaseApp->startBackup( realBaseAppAddr_ );
		}
		else
		{
			ERROR_MSG( "FinishSetBackupDiffVisitor::onAdd: No BaseApp for %s\n",
					addr.c_str() );
		}
	}

	virtual void onChange( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		this->onAdd( addr, index, virtualSize, prime );
	}

	virtual void onRemove( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		BaseApp * pBaseApp = BaseAppMgr::instance().findBaseApp( addr );

		if (pBaseApp)
		{
			Mercury::Bundle & bundle = pBaseApp->bundle();
			BaseAppIntInterface::stopBaseEntitiesBackupArgs & args =
				BaseAppIntInterface::stopBaseEntitiesBackupArgs::start(
					bundle );

			args.realBaseAppAddr = realBaseAppAddr_;
			args.index = index;
			args.hashSize = virtualSize;
			args.prime = prime;
			args.isPending = false;

			pBaseApp->send();
			pBaseApp->stopBackup( realBaseAppAddr_ );
		}
	}

private:
	Mercury::Address realBaseAppAddr_;
};


/**
 *	This class is used to observe if the are any differences between backup hashes.
 */
class SimpleDiffVisitor : public BackupHash::DiffVisitor
{
public:
	SimpleDiffVisitor() :
		differences_(0)
	{}

	virtual void onAdd( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		++differences_;
	}

	virtual void onChange( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		++differences_;
	}

	virtual void onRemove( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		++differences_;
	}

	bool different() const
	{
		return differences_ != 0;
	}

private:
	unsigned int differences_;	
};


/**
 *	This method handles a message from a BaseApp informing us that it is ready
 *	to use its new backup hash.
 */
void BaseApp::useNewBackupHash( BinaryIStream & data )
{
	BackupHash backupHash;
	BackupHash newBackupHash;

	data >> backupHash >> newBackupHash;

	FinishSetBackupDiffVisitor visitor( this->addr() );
	backupHash.diff( newBackupHash, visitor );
	backupHash_.swap( newBackupHash );

	// We only clear it if we have an ack that it is the most recent backup hash
	SimpleDiffVisitor simpleVisitor = SimpleDiffVisitor();
	backupHash_.diff( newBackupHash_, simpleVisitor );
	if (!simpleVisitor.different())
	{
		newBackupHash_.clear();
	}

	this->checkToStartOffloading();
}


/**
 *	This method sends pre-streamed hash data to a BaseApp.
 *
 *	@param data 	The pre-streamed hash data.
 */
void BaseApp::updateDBAppHashFromStream( BinaryIStream & data )
{
	Mercury::Bundle & bundle = this->channel().bundle();
	bundle.startMessage( BaseAppIntInterface::updateDBAppHash );
	bundle.transfer( data, data.remainingLength() );
	this->send();
}


BW_END_NAMESPACE

// baseapp.cpp
