#ifndef ARCHIVER_HPP
#define ARCHIVER_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BaseAppMgrGateway;
class Bases;
class SqliteDatabase;

namespace Mercury
{
class ChannelOwner;
}

typedef Mercury::ChannelOwner DBApp;

/**
 *	This class is responsible for periodically archiving this BaseApp's
 *	entities.
 */
class Archiver
{
public:
	Archiver();

	void tick( DBApp & dbApp, BaseAppMgrGateway & baseAppMgr,
		Bases & bases, SqliteDatabase * pSecondaryDB );

	void tickSecondaryDB( SqliteDatabase * pSecondaryDB );

	void handleBaseAppDeath( const Mercury::Address & addr,
		Bases & bases, SqliteDatabase * pSecondaryDB );

	void commitSecondaryDB() { commitSecondaryDB_ = true; }

	static bool isEnabled();

private:
	void restartArchiveCycle( Bases & bases );

	// Used for which tick in archivePeriodInTicks we are up to.
	int					archiveIndex_;
	BW::vector<EntityID> 	basesToArchive_;

	Mercury::Address	deadBaseAppAddr_;

	bool commitSecondaryDB_;
	bool flipSecondaryDB_;
	GameTime timeSinceLastCommit_;
};

BW_END_NAMESPACE

#endif // ARCHIVER_HPP
