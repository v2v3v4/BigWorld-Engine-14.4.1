#ifndef BACKED_UP_BASE_APPS_HPP
#define BACKED_UP_BASE_APPS_HPP

#include "backed_up_base_app.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class BackedUpBaseApp;
class MiniBackupHash;

namespace Mercury
{
class UnpackedMessageHeader;
}

/**
 *	This class handles the backing up of other BaseApps by this BaseApp.
 */
class BackedUpBaseApps
{
public:
	void startAppBackup( const Mercury::Address & realBaseAppAddr,
		uint32 index, uint32 hashSize, uint32 prime, bool isInitial );

	void stopAppBackup( const Mercury::Address	& realBaseAppAddr,
		uint32 index, uint32 hashSize, uint32 prime, bool isPending );


	void backUpEntity( const Mercury::Address & srcAddr,
			EntityID entityID, BinaryIStream & data );

	void stopEntityBackup( const Mercury::Address & srcAddr,
			EntityID entityID );

	void handleBaseAppDeath( const Mercury::Address & deadAddr );

	void onloadedEntity( const Mercury::Address & srcAddr, 
						 EntityID entityID );

	bool isBackingUpOthers() const { return !apps_.empty(); }

	static WatcherPtr pWatcher();

private:
	size_t numEntitiesBackedUp() const;

	typedef BW::map< Mercury::Address, BackedUpBaseApp > Container;
	Container apps_;
};

BW_END_NAMESPACE

#endif // BACKED_UP_BASE_APPS_HPP
