#ifndef BACKUP_SENDER_HPP
#define BACKUP_SENDER_HPP

#include "server/backup_hash.hpp"


BW_BEGIN_NAMESPACE

class Base;
class BaseApp;
class BaseAppMgrGateway;
class Bases;
namespace Mercury
{
class BundleSendingMap;
class ChannelOwner;
class NetworkInterface;
class ReplyMessageHandler;
}


/**
 *	This class is responsible for backing up the base entities to other
 *	BaseApps.
 */
class BackupSender
{
public:
	BackupSender( BaseApp & baseApp );

	void tick( const Bases & bases,
			   Mercury::NetworkInterface & networkInterface );

	Mercury::Address addressFor( EntityID entityID ) const
	{
		return entityToAppHash_.addressFor( entityID );
	}

	void addToStream( BinaryOStream & stream );
	void handleBaseAppDeath( const Mercury::Address & addr );
	void setBackupBaseApps( BinaryIStream & data,
	   Mercury::NetworkInterface & networkInterface );

	void restartBackupCycle( const Bases & bases );

	void startOffloading() { isOffloading_ = true; }
	bool isOffloading() const { return isOffloading_; }

	bool autoBackupBase( Base & base,
					 Mercury::BundleSendingMap & bundles,
					 Mercury::ReplyMessageHandler * pHandler = NULL );
	bool backupBase( Base & base,
					 Mercury::BundleSendingMap & bundles,
					 Mercury::ReplyMessageHandler * pHandler = NULL );
private:
	void ackNewBackupHash();

	int					offloadPerTick_;

	// Fractional overflow of backup for next tick
	float				backupRemainder_;
	typedef BW::vector<EntityID> BasesToBackUp;
	BasesToBackUp		basesToBackUp_;

	BackupHash			entityToAppHash_;	// The up-to-date hash
	BackupHash			newEntityToAppHash_; // The hash we are trying to switch to.
	bool				isUsingNewBackup_;
	bool				isOffloading_;
	int					ticksSinceLastSuccessfulOffload_;
	BaseApp	&			baseApp_;
};

BW_END_NAMESPACE

#endif // BACKUP_SENDER_HPP
