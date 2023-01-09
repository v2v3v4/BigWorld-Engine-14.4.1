#ifndef BASE_APP_HPP
#define BASE_APP_HPP

#include "util.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "cstdmf/bw_set.hpp"

#include "network/channel_owner.hpp"

#include "server/backup_hash.hpp"


BW_BEGIN_NAMESPACE

class BaseAppMgr;
class BaseAppsIterator;

class BaseApp: public Mercury::ChannelOwner
{
public:
	BaseApp( BaseAppMgr & baseAppMgr, const Mercury::Address & intAddr,
			const Mercury::Address & extAddr, int id, bool isServiceApp );

	float load() const { return load_; }

	void updateLoad( float load, int numBases, int numProxies )
	{
		load_ = load;
		numBases_ = numBases;
		numProxies_ = numProxies;
	}

	bool hasTimedOut( uint64 currTime, uint64 timeoutPeriod ) const;

	const Mercury::Address & externalAddr() const { return externalAddr_; }
	const Mercury::Address & internalAddr() const { return this->addr(); }

	int numBases() const	{ return numBases_; }
	int numProxies() const	{ return numProxies_; }

	BaseAppID id() const	{ return id_; }
	void id( int id )		{ id_ = id; }

	bool isServiceApp() const	{ return isServiceApp_; }

	void addEntity();

	static WatcherPtr pWatcher();

	const BackupHash & backupHash() const { return backupHash_; }
	BackupHash & backupHash() { return backupHash_; }

	const BackupHash & newBackupHash() const { return newBackupHash_; }
	BackupHash & newBackupHash() { return newBackupHash_; }

	bool isRetiring() const 		{ return isRetiring_; }
	bool isOffloading() const		{ return isOffloading_; }

	// Message handlers
	void retireApp();

	void startBackup( const Mercury::Address & addr );
	void stopBackup( const Mercury::Address & addr );

	void adjustBackupLocations( const Mercury::Address & newAddr,
			BaseAppsIterator baseApps,
			AdjustBackupLocationsOp op,
			bool hadIdealBackup,
			bool hasIdealBackup );

	void useNewBackupHash( BinaryIStream & data );

	template< typename HASH >
	void updateDBAppHash( const HASH & hash );
	void updateDBAppHashFromStream( BinaryIStream & stream );

private:
	void checkToStartOffloading();

	BaseAppMgr & 			baseAppMgr_;

	Mercury::Address		externalAddr_;
	BaseAppID				id_;

	float					load_;
	int						numBases_;
	int						numProxies_;

	BackupHash 				backupHash_;
	BackupHash 				newBackupHash_;

	bool					isRetiring_;
	bool					isOffloading_;
	bool					isServiceApp_;

	typedef BW::set< Mercury::Address > BackingUp;
	BackingUp				backingUp_;
};



/**
 *	This method sends DBApp hash data to a BaseApp.
 *
 *	@param hash 	The DBApp hash.
 */
template< typename HASH >
void BaseApp::updateDBAppHash( const HASH & hash )
{
	Mercury::Bundle & bundle = this->channel().bundle();
	bundle.startMessage( 
		BaseAppIntInterface::updateDBAppHash );
	bundle << hash;
	this->send();
}


BW_END_NAMESPACE

#endif // BASE_APP_HPP
