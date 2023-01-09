#include "script/first_include.hpp"

#include "archiver.hpp"

#include "base.hpp"
#include "baseapp_config.hpp"
#include "baseappmgr_gateway.hpp"
#include "bases.hpp"
#include "sqlite_database.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "db/db_config.hpp"

#include "network/channel_owner.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
Archiver::Archiver() :
	archiveIndex_( INT_MAX ),
	basesToArchive_(),
	deadBaseAppAddr_( Mercury::Address::NONE ),
	commitSecondaryDB_( false ),
	flipSecondaryDB_( false ),
	timeSinceLastCommit_( 0 )
{
}



/**
 * This method is called even during the shutdown.
 * It tick the secondary DB and does commits
 */
void Archiver::tickSecondaryDB( SqliteDatabase * pSecondaryDB )
{

	if (!pSecondaryDB)
	{
		return;
	}
	pSecondaryDB->tick();

	const uint maxCommitPeriodInTicks =
		DBConfig::get().secondaryDB.maxCommitPeriodInTicks();

	if (commitSecondaryDB_ ||
			(timeSinceLastCommit_ >= maxCommitPeriodInTicks))
	{
		pSecondaryDB->commit( flipSecondaryDB_ );
		timeSinceLastCommit_ = 0;
		commitSecondaryDB_ = false;
		flipSecondaryDB_ = false;
	}

	++timeSinceLastCommit_;
}

/**
 *	This method archives some of the base entities.
 *	It is called from a normal application loop,
 *	not during the shutdown phase
 */
void Archiver::tick( DBApp & dbApp, BaseAppMgrGateway & baseAppMgr,
		Bases & bases, SqliteDatabase * pSecondaryDB )
{
	int archivePeriod = BaseAppConfig::archivePeriodInTicks();

	if (archivePeriod <= 0)
	{
		// Do nothing since archiving is disabled.
		return;
	}

	if (!pSecondaryDB &&
		float( dbApp.channel().sendWindowUsage() ) >
		dbApp.channel().windowSize() *
			BaseAppConfig::archiveEmergencyThreshold())
	{
		// Make sure that we do not use more than half the available
		// window size.
		ERROR_MSG( "Archiver::tick: "
				"Skipping archiving. DBApp channel usage is %d/%d\n",
			dbApp.channel().sendWindowUsage(),
			dbApp.channel().windowSize() );
	}
	else
	{
		if (archiveIndex_ >= archivePeriod)
		{
			if (pSecondaryDB)
			{
				commitSecondaryDB_ = true;
				flipSecondaryDB_ = true;
			}

			if (deadBaseAppAddr_ != Mercury::Address::NONE)
			{
				baseAppMgr.informOfArchiveComplete( deadBaseAppAddr_ );
				deadBaseAppAddr_ = Mercury::Address::NONE;
			}

			this->restartArchiveCycle( bases );
		}

		int size = basesToArchive_.size();
		int startIndex = size * archiveIndex_ / archivePeriod;
		++archiveIndex_;
		int endIndex   = size * archiveIndex_ / archivePeriod;

		for (int i = startIndex; i < endIndex; ++i)
		{
			Base * pBase = bases.findEntity( basesToArchive_[i] );

			if ((pBase != NULL) && pBase->hasWrittenToDB())
			{
				pBase->autoArchive();
			}
		}
	}
}


void Archiver::handleBaseAppDeath( const Mercury::Address & addr,
		Bases & bases, SqliteDatabase * pSecondaryDB )
{
	deadBaseAppAddr_ = addr;

	if (pSecondaryDB)
	{
		this->restartArchiveCycle( bases );
	}
}


/**
 *	This method restarts archiving.
 */
void Archiver::restartArchiveCycle( Bases & bases )
{
	archiveIndex_ = 0;
	basesToArchive_.clear();

	Bases::const_iterator iBase = bases.begin();
	while (iBase != bases.end())
	{
		if (iBase->second->hasWrittenToDB())
		{
			basesToArchive_.push_back( iBase->first );
		}
		++iBase;
	}

	// Randomise the list of entities to archive so we don't
	// hit the database with too many of the same entity type,
	// if they have been added in a big block.
	std::random_shuffle( basesToArchive_.begin(),
		basesToArchive_.end() );

	// TODO: It would be nicer if we maintained the random
	// order. Currently, it would be possible for an entity not
	// to be archived for twice the archive period.
}


/**
 *	This method returns whether archiving is enabled.
 */
bool Archiver::isEnabled()
{
	return BaseAppConfig::archivePeriodInTicks() > 0;
}

BW_END_NAMESPACE

// archiver.cpp
