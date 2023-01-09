#include "script/first_include.hpp"

#include "backup_sender.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "baseappmgr_gateway.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"
#include "cellapp/cellapp_interface.hpp"

#include "network/bundle.hpp"
#include "network/bundle_sending_map.hpp"
#include "network/channel_sender.hpp"

#include "server/bwconfig.hpp"
#include "server/backup_hash_chain.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: StartSetBackupDiffVisitor
// -----------------------------------------------------------------------------

/**
 *	This class is used to handle the changes to the hash initially before the
 *	new hash has been primed.
 */
class StartSetBackupDiffVisitor : public BackupHash::DiffVisitor
{
public:
	StartSetBackupDiffVisitor( Mercury::NetworkInterface & interface ) :
		interface_( interface ) {}

	virtual void onAdd( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		Mercury::ChannelSender sender( BaseApp::getChannel( addr ) );
		BaseAppIntInterface::startBaseEntitiesBackupArgs & args =
			BaseAppIntInterface::startBaseEntitiesBackupArgs::start(
				sender.bundle() );

		args.realBaseAppAddr = interface_.address();
		args.index = index;
		args.hashSize = virtualSize;
		args.prime = prime;
		args.isInitial = true;
	}

	virtual void onChange( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
		this->onAdd( addr, index, virtualSize, prime );
	}

	virtual void onRemove( const Mercury::Address & addr,
			uint32 index, uint32 virtualSize, uint32 prime )
	{
	}

private:
	Mercury::NetworkInterface & interface_;
};


// -----------------------------------------------------------------------------
// Section: BackupSender
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
BackupSender::BackupSender( BaseApp & baseApp ) :
	offloadPerTick_( 0 ),
	backupRemainder_( 0.f ),
	basesToBackUp_(),
	entityToAppHash_(),
	newEntityToAppHash_(),
	isUsingNewBackup_( false ),
	isOffloading_( false ),
	ticksSinceLastSuccessfulOffload_( 0 ),
	baseApp_( baseApp )
{
}


/**
 *	This method sends backups for as many base entities as we are supposed to
 *	each tick.
 *
 *	@param bases 				The collection of base entities.
 *	@param networkInterface 	The network interface to use to send backups
 *								through.
 *	
 */
void BackupSender::tick( const Bases & bases,
						 Mercury::NetworkInterface & networkInterface )
{
	int periodInTicks = BaseAppConfig::backupPeriodInTicks();

	if (periodInTicks == 0)
		return;

	if (!isUsingNewBackup_ && entityToAppHash_.empty())
		return;

	Mercury::BundleSendingMap bundles( networkInterface );

	// The number of entities to back up is calculated. A floating point
	// remainder is kept so that the backup period is roughly correct.
	float numToBackUpFloat =
		float(bases.size())/periodInTicks + backupRemainder_;
	int numToBackUp = int(numToBackUpFloat);
	backupRemainder_ = numToBackUpFloat - numToBackUp;

	if (isOffloading_)
	{
		if (offloadPerTick_ < numToBackUp)
		{
			offloadPerTick_ = numToBackUp;

			INFO_MSG( "BackupSender::tick: "
					"BaseApp is retiring, offloading at %d entities per tick\n",
				offloadPerTick_ );
		}
		else
		{
			numToBackUp = offloadPerTick_;
		}
	}

	if (basesToBackUp_.empty())
	{
		this->restartBackupCycle( bases );
	}

	bool madeProgress = false;
	while ((numToBackUp > 0) && !basesToBackUp_.empty())
	{
		Base * pBase = bases.findEntity( basesToBackUp_.back() );
		basesToBackUp_.pop_back();

		if (pBase && this->autoBackupBase( *pBase, bundles ) )
		{
			madeProgress = true;
			--numToBackUp;
		}
	}

	// Check if at least one base was backed up.
	if (madeProgress)
	{
		// Send all the backup data to the other baseapps.
		bundles.sendAll();
		ticksSinceLastSuccessfulOffload_ = 0;
	}
	else
	{
		if (isOffloading_ && !bases.empty())
		{
			++ticksSinceLastSuccessfulOffload_;
			// Log the number of pending entities every second.
			const std::div_t sec = std::div( ticksSinceLastSuccessfulOffload_,
				BaseAppConfig::updateHertz() );
			if (sec.rem == 0)
			{
				INFO_MSG( "BackupSender::tick: unable to offload %" PRIzu
						" entities during last %d seconds (possibly waiting "
						"for client connection)\n", bases.size(), sec.quot );
			}
		}
	}

	if (basesToBackUp_.empty() && isUsingNewBackup_)
	{
		// If we were updating a new backup, we are now finished. Inform the
		// BaseAppMgr and start using it.
		this->ackNewBackupHash();
	}
}


/**
 *	This method restarts the backup cycle. 
 *
 *	@param bases	The collection of bases to consider for backing up.
 */
void BackupSender::restartBackupCycle( const Bases & bases )
{
	basesToBackUp_.clear();

	Bases::const_iterator iBase = bases.begin();

	while (iBase != bases.end())
	{
		basesToBackUp_.push_back( (iBase++)->first );
	}

	// Randomise the backup so we do not load ourselves if contiguous
	// blocks of large entities exist in the bases collection.
	std::random_shuffle( basesToBackUp_.begin(), basesToBackUp_.end() );

	// TODO: It would be nicer if we maintained the random order. Currently,
	// it would be possible for an entity not to be backed up for twice the
	// archive period.
}


/**
 *	This method performs the automatic backup operation for a single base
 *	entity.
 */
bool BackupSender::autoBackupBase( Base & base,
		Mercury::BundleSendingMap & bundles,
		Mercury::ReplyMessageHandler * pHandler )
{
	// See if auto backup is turned on, which should be disregarded if we
	// are offloading.
	if (!isOffloading_ && !base.shouldAutoBackup())
	{
		return false;
	}

	bool success = this->backupBase( base, bundles, pHandler );

	if (success)
	{
		if (BaseAppConfig::shouldLogBackups())
		{
			DEBUG_MSG( "BackupSender::autoBackupBase: "
					"Base %d backed up.\n", base.id() );
		}

		if (base.shouldAutoBackup() == AutoBackupAndArchive::NEXT_ONLY)
		{
			base.shouldAutoBackup( AutoBackupAndArchive::NO );
		}
	}

	return success;
}


/**
 *	This method performs the backup operation for a single base entity.
 *
 *	@param base 		The base entity to backup.
 *	@param bundles 		Bundle sending map to be used for sending.
 *	@param pHandler 	The request handler for the backupEntity request
 *						operation on the peer BaseApp.
 *
 *	@return 			True if a base was actually backed up, false otherwise.
 */
bool BackupSender::backupBase( Base & base,
							   Mercury::BundleSendingMap & bundles,
							   Mercury::ReplyMessageHandler * pHandler )
{
	Mercury::Address addr = entityToAppHash_.addressFor( base.id() );

	if (isUsingNewBackup_)
	{
		// If the new hash is being updated, only send the backup for those
		// entities whose hash has changed. The destination already has a
		// backup for those entities that have not changed.
		const Mercury::Address & newAddr = 
			newEntityToAppHash_.addressFor( base.id() );


		if ((newAddr == addr) && base.hasBeenBackedUp())
		{
			return false;
		}
		else
		{
			addr = newAddr;
		}
	}

	if (addr == Mercury::Address::NONE)
	{
		return false;
	}

	if (isOffloading_)
	{
		if (base.isProxy())
		{
			const Proxy & proxy = static_cast<const Proxy &>( base );
			if (proxy.hasClient() && !proxy.isClientConnected())
			{
				// Wait for the client to connect so it can be transferred to
				// another BaseApp.
				DEBUG_MSG( "BackupSender::backupBase: client %s %d has not yet "
						"connected, offloading is delayed\n",
					proxy.pType()->name(), proxy.id() );
				return false;
			}
		}

		// If a baseapp is offloading, the hash is immutable, so if an address 
		// is dead, just find another baseapp to send stuff to.
		addr = baseApp_.backupHashChain().addressFor( addr, base.id() );
	}

	Mercury::Bundle & bundle = bundles[ addr ];

	base.backupTo( addr, bundle, isOffloading_, pHandler );

	return true;
}


/**
 *	This method is used to acknowledge to BaseAppMgr that a hash that we are
 *	transitioning to is now fully in use, i.e. all the base entities residing
 *	on this BaseApp have backups according to the new hash scheme.
 */
void BackupSender::ackNewBackupHash()
{
	INFO_MSG( "BackupSender::tick: New backup is ready.\n" );

	baseApp_.baseAppMgr().useNewBackupHash( entityToAppHash_, 
											newEntityToAppHash_ );

	entityToAppHash_.swap( newEntityToAppHash_ );
	newEntityToAppHash_.clear();
	isUsingNewBackup_ = false;
}


/**
 *	This method adds information about this object to the stream. It is used to
 *	send to a new BaseAppMgr.
 */
void BackupSender::addToStream( BinaryOStream & stream )
{
	stream << entityToAppHash_ << newEntityToAppHash_;
}


/**
 *	This method is called when a BaseApp has died.
 */
void BackupSender::handleBaseAppDeath( const Mercury::Address & addr )
{
	// If we're offloading, don't touch that hash!
	if (isOffloading_)
	{
		return;
	}

	// If we were backing up to the dead baseapp, no longer send to it.
	if (isUsingNewBackup_)
	{
		newEntityToAppHash_.clearAddress( addr );
	}
	else
	{
		entityToAppHash_.clearAddress( addr );
	}
}


/**
 *	This method handles a message informing us of which BaseApps this BaseApp
 *	should back up its entities to. 
 */
void BackupSender::setBackupBaseApps( BinaryIStream & data,
	   Mercury::NetworkInterface & networkInterface )
{
	MF_ASSERT( !isOffloading_ );

	if (isUsingNewBackup_)
	{
		// We were already transitioning to a new hash.
		for (uint i = 0; i < newEntityToAppHash_.size(); ++i)
		{
			const Mercury::Address & dstAddr = newEntityToAppHash_[ i ];

			if (dstAddr != Mercury::Address::NONE)
			{
				Mercury::ChannelSender sender( BaseApp::getChannel( dstAddr ) );

				BaseAppIntInterface::stopBaseEntitiesBackupArgs & args =
					BaseAppIntInterface::stopBaseEntitiesBackupArgs::start(
						sender.bundle() );

				args.realBaseAppAddr = networkInterface.address();
				args.index = i;
				args.hashSize = newEntityToAppHash_.virtualSizeFor( i );
				args.prime = newEntityToAppHash_.prime();
				args.isPending = true;
			}
		}
	}

	data >> newEntityToAppHash_;

	StartSetBackupDiffVisitor visitor( networkInterface );
	entityToAppHash_.diff( newEntityToAppHash_, visitor );
	isUsingNewBackup_ = true;

	this->restartBackupCycle( baseApp_.bases() );
}

BW_END_NAMESPACE

// backup_sender.cpp
