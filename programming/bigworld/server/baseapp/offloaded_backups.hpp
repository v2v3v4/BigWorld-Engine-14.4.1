#ifndef OFFLOADED_BACKUPS_HPP
#define OFFLOADED_BACKUPS_HPP

#include "backed_up_base_app.hpp"
#include "network/basictypes.hpp"
#include "server/backup_hash_chain.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is responsible for holding entity backup data for entities that
 *	have been offloaded to other BaseApps. During retirement, the BaseApp
 *	becomes the backup for any entities that it offloads to other BaseApps.
 */
class OffloadedBackups
{
public:
	OffloadedBackups();

	bool wasOffloaded( EntityID entityID ) const;

	void backUpEntity( const Mercury::Address & srcAddr,
					   BinaryIStream & data );
	void handleBaseAppDeath( const Mercury::Address & deadAddr,
							 const BackupHashChain & backedUpHashChain );
	void stopBackingUpEntity( EntityID entityID );
	bool isEmpty()	{ return apps_.empty(); }
private:
	typedef BW::map< Mercury::Address, BackedUpEntities > Container;
	Container apps_;
};

BW_END_NAMESPACE

#endif // OFFLOADED_BACKUPS_HPP
