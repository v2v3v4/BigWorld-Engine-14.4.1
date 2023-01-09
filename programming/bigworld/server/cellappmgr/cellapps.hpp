#ifndef CELLAPPS_HPP
#define CELLAPPS_HPP

#include "cstdmf/watcher.hpp"
#include "network/basictypes.hpp"
#include "server/common.hpp"
#include "math/ema.hpp"

#include "cellapp_comparer.hpp"
#include "space.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class CellApp;
class CellAppDeathHandler;
class CellAppGroup;
class Watcher;

class CellAppVisitor
{
public:
	virtual void onVisit( CellApp * pCellApp ) = 0;
};


class CellApps
{
public:

	CellApps( float updateHertz );

	CellApp * add( const Mercury::Address & addr, uint16 viewerPort,
		CellAppID appID );

	void delApp( const Mercury::Address & addr );

	CellApp * find( const Mercury::Address & addr ) const;
	CellApp * findAlternateApp( CellApp * pDeadApp ) const;

	CellApp * leastLoaded( CellApp * pExclude );
	void identifyGroups( BW::list< CellAppGroup > & groups ) const;

	void deleteAll();

	void visit( CellAppVisitor & visitor ) const;

	int numActive() const;
	int numEntities() const;

	float minLoad() const;
	float avgLoad() const;
	float maxLoad() const;

	size_t size() const		{ return map_.size(); }
	bool empty() const		{ return map_.empty(); }

	bool areAnyOverloaded() const;
	bool isAvgOverloaded() const;

	float maxCellAppTimeout() const;

	CellApp * findBestCellApp( const Space * pSpace,
			const CellAppGroup * pGroup ) const;

	CellApp * findLeastLoadedCellApp() const;

	CellApp * checkForDeadCellApps( uint64 cellAppTimeoutPeriod ) const;

	void updateCellAppTimeout();

	void sendToAll() const;
	void sendGameTime( GameTime gameTime ) const;
	void sendCellAppMgrInfo( float maxCellAppLoad ) const;

	void startAll( const Mercury::Address & baseAppAddr ) const;
	void setBaseApp( const Mercury::Address & baseAppAddr );
	void setDBAppAlpha( const Mercury::Address & dbAppAlphaAddr );

	void handleBaseAppDeath( const void * pBlob, int length ) const;
	void handleCellAppDeath( const Mercury::Address & addr,
			CellAppDeathHandler * pCellAppDeathHandler );

	void shutDownAll();
	void controlledShutDown( ShutDownStage stage, GameTime shutDownTime );

	void runScript( const BW::string & script, bool allCellApps ) const;

	void setSharedData( uint8 dataType,	const BW::string & key,
			const BW::string & value,
			const Mercury::Address & originalSrcAddr );
	void delSharedData( uint8 dataType, const BW::string & key,
			const Mercury::Address & originalSrcAddr );

	void addServiceFragment( const BW::string & serviceName, 
		const EntityMailBoxRef & fragmentMailBox );
	void delServiceFragment( const BW::string & serviceName,
		const Mercury::Address & fragmentAddress );

	WatcherPtr pWatcher();

private:
	typedef BW::map< Mercury::Address, CellApp * > Map;
	Map map_;
	EMA maxCellAppTimeout_;

	CellAppComparer cellAppComparer_;
};

BW_END_NAMESPACE

#endif // CELLAPPS_HPP
