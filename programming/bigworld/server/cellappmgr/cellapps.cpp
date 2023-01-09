#include "cellapps.hpp"

#include "cellapp.hpp"
#include "cell_app_death_handler.hpp"
#include "cell_app_group.hpp"
#include "cellappmgr_config.hpp"
#include "login_conditions_config.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE


namespace // (anonymous)
{

float getCellAppTimeoutBias( float updateHertz )
{
	// latest 300 sec account for 95% of the average
	return EMA::calculateBiasFromNumSamples( 300 * updateHertz );	
}

} // end namespace (anonymous)


/**
 *  CellApps constructor. Expects a bias parameter to configure
 *  maxCellAppTimeout average.
 */
CellApps::CellApps( float updateHertz ) :
	maxCellAppTimeout_( getCellAppTimeoutBias( updateHertz ) )
{
}


/**
 *	This method creates a new CellApp.
 */
CellApp * CellApps::add( const Mercury::Address & addr, uint16 viewerPort,
		CellAppID appID )
{
	// Make sure not already added.
	MF_ASSERT( map_.find( addr ) == map_.end() );

	// We may assign a label to the Cell Applications but for now, just using
	// their address is fine.
	CellApp * pApp = new CellApp( addr, viewerPort, appID );
	map_[ addr ] = pApp;

	return pApp;
}


/**
 *	This method deletes a CellApp.
 */
void CellApps::delApp( const Mercury::Address & addr )
{
	Map::iterator iter = map_.find( addr );

	if (iter != map_.end())
	{
		delete iter->second;
		map_.erase( iter );
	}
	else
	{
		ERROR_MSG( "CellAppMgr::delApp: WAS NOT ABLE TO REMOVE %s\n",
				addr.c_str() );
	}
}


/**
 *	This method finds the CellApp with the given address.
 */
CellApp * CellApps::find( const Mercury::Address & addr ) const
{
	Map::const_iterator iter = map_.find( addr );

	return (iter != map_.end()) ? iter->second : NULL;
}


/**
 *	This method returns a CellApp that is not the input one.
 */
CellApp * CellApps::findAlternateApp( CellApp * pDeadApp ) const
{
	CellApp * pBest = NULL;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pCurr = iter->second;

		if (pCurr != pDeadApp)
		{
			if (pBest == NULL)
			{
				pBest = pCurr;
			}
			else
			{
				// Prefer the application that is not on the same machine as the
				// input application.
				bool bestOnDead = (pBest->addr().ip == pDeadApp->addr().ip);
				bool currOnDead = (pCurr->addr().ip == pDeadApp->addr().ip);

				if (bestOnDead == currOnDead)
				{
					if (pCurr->smoothedLoad() < pBest->smoothedLoad())
						pBest = pCurr;
				}
				else if (!currOnDead)
				{
					pBest = pCurr;
				}
			}
		}

		++iter;
	}

	return pBest;
}


/**
 *	This method returns the least loaded CellApp.
 */
CellApp * CellApps::leastLoaded( CellApp * pExclude )
{
	Map::iterator iter = map_.begin();
	Map::iterator endIter = map_.end();
	CellApp * pBest = NULL;

	while (iter != endIter)
	{
		CellApp * pCurr = iter->second;
		if (pCurr->addr().ip != pExclude->addr().ip)
		{
			if (!pBest ||
				(pCurr->smoothedLoad() < pBest->smoothedLoad()))
			{
				pBest = pCurr;
			}
		}

		++iter;
	}

	return pBest;
}


/**
 *	This method creates groups of connected CellApps. The CellApps are
 *	partitioned into a list of Group object. Each group is the set of CellApps
 *	that can balance their load via normal load balancing.
 */
void CellApps::identifyGroups( BW::list< CellAppGroup > & groups ) const
{
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pApp = iter->second;

		if (!pApp->isEmpty())
		{
			if (pApp->pGroup() == NULL)
			{
				groups.push_back( CellAppGroup() );
				pApp->markGroup( &groups.back() );
			}
		}

		++iter;
	}
}


/**
 *	This method deletes all CellApps.
 */
void CellApps::deleteAll()
{
	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pApp = iter->second;
		// pApp->shutDown();
		delete pApp;

		iter++;
	}

	map_.clear();
}


/**
 *	This method is used to visit each CellApp.
 */
void CellApps::visit( CellAppVisitor & visitor ) const
{
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		visitor.onVisit( iter->second );

		++iter;
	}
}


/**
 *	This method returns the number of CellApps that currently handle spaces.
 */
int CellApps::numActive() const
{
	int count = 0;
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		if (iter->second->numCells() > 0)
		{
			count++;
		}
		iter++;
	}

	return count;
}


/**
 *	This method returns the total number of entities over all CellApps.
 */
int CellApps::numEntities() const
{
	int count = 0;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		count += iter->second->numEntities();
		++iter;
	}

	return count;
}


/**
 *	This method returns the minimum load of all the CellApps.
 */
float CellApps::minLoad() const
{
	float load = map_.empty() ? 0.f : FLT_MAX;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		if (!iter->second->isRetiring())
		{
			load = std::min( load, iter->second->smoothedLoad() );
		}
		++iter;
	}

	return load;
}


/**
 *	This method returns the average load of all the CellApps.
 */
float CellApps::avgLoad() const
{
	float load = 0.f;
	int count = 0;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		if (!iter->second->isRetiring())
		{
			load += iter->second->smoothedLoad();
			++count;
		}
		++iter;
	}

	return (count != 0) ? (load/count) : 0.f;
}


/**
 *	This method returns the maximum load of all the known CellApps.
 */
float CellApps::maxLoad() const
{
	float load = 0.f;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		if (!iter->second->isRetiring())
		{
			load = std::max( load, iter->second->smoothedLoad() );
		}
		++iter;
	}

	return load;
}


/**
 *	This method returns the maximum timeout from all the known CellApps.
 *  This is a value decaying over time.
 */
float CellApps::maxCellAppTimeout() const
{
	return maxCellAppTimeout_.average();
}


/**
 *	This method updates the weighted average for the CellAppTimeout value.
 *  It should be called from the CellAppMgr class every tick.
 */
void CellApps::updateCellAppTimeout()
{
	if (map_.empty())
	{
		maxCellAppTimeout_.resetAverage( 0.f );
		return;
	}

	uint64 currTime = timestamp();
	double longestImmediateTimeout = 0.0;
	Map::const_iterator iter = map_.begin();
	while (iter != map_.end())
	{
		uint64 lastReceivedTime = iter->second->channel().lastReceivedTime();
		double t   = TimeStamp::toSeconds( currTime - lastReceivedTime );

		longestImmediateTimeout = std::max( longestImmediateTimeout, t );
		++iter;
	}

	// accept spikes without bias
	maxCellAppTimeout_.highWaterSample( longestImmediateTimeout );
}


/**
 *	This method returns whether any CellApps are overloaded.
 */
bool CellApps::areAnyOverloaded() const
{
	bool isOverloaded = map_.empty();

	Map::const_iterator iter = map_.begin();
	Map::const_iterator endIter = map_.end();

	while ((iter != endIter) && !isOverloaded)
	{
		CellApp * pApp = iter->second;

		if (pApp->smoothedLoad() > LoginConditionsConfig::maxLoad())
		{
			isOverloaded = true;
		}

		++iter;
	}

	return isOverloaded;
}


/**
 *	This method returns whether CellApps are overloaded in average.
 */
bool CellApps::isAvgOverloaded() const
{
	if (map_.empty())
	{
		return true;
	}

	return (this->avgLoad() > LoginConditionsConfig::avgLoad());
}


/**
 *	This method returns the best CellApp to be used for a new cell in the given
 *	space. The chosen CellApp must not be in the given group.
 */
CellApp * CellApps::findBestCellApp( const Space * pSpace,
		const CellAppGroup * pExcludedGroup ) const
{
	MF_ASSERT( pSpace );
	CellApp * pBest = NULL;
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pApp = iter->second;

		if (((pExcludedGroup == NULL) || (pApp->pGroup() != pExcludedGroup)) &&
				!pApp->isRetiring())
		{
			if (cellAppComparer_.isValidApp( pApp, pSpace ) &&
				cellAppComparer_.isABetterCellApp( pBest, pApp, pSpace ))
			{
				pBest = pApp;
			}
		}

		++iter;
	}

	return pBest;
}


/**
 *	This method returns the CellApp with the least estimated load.
 */
CellApp * CellApps::findLeastLoadedCellApp() const
{
	CellApp * pBest = NULL;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		if (!pBest || (iter->second->estimatedLoad() < pBest->estimatedLoad()))
		{
			pBest = iter->second;
		}

		++iter;
	}

	return pBest;
}


/**
 *	This method check whether any CellApps are considered dead because the
 *	CellAppMgr has not heard from them in a while.
 */
CellApp * CellApps::checkForDeadCellApps( uint64 cellAppTimeoutPeriod ) const
{
	uint64 currTime = timestamp();
	uint64 lastHeardTime = 0;
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		lastHeardTime = std::max( lastHeardTime,
							iter->second->channel().lastReceivedTime() );

		++iter;
	}

	const uint64 timeSinceAnyHeard = currTime - lastHeardTime;

	// The logic behind the following block of code is that if we
	// haven't heard from any CellApp in a long time, the CellAppMgr is
	// probably the misbehaving app and we shouldn't start forgetting
	// about CellApps.
	if (timeSinceAnyHeard > cellAppTimeoutPeriod /2)
	{
		INFO_MSG( "CellApps::checkForDeadCellApps: "
			"Last inform time not recent enough %f\n",
			double((int64)timeSinceAnyHeard)/stampsPerSecondD() );
		return NULL;
	}

	iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pApp = iter->second;

		if (pApp->hasTimedOut( currTime, cellAppTimeoutPeriod ) &&
			(!CellAppMgrConfig::shutDownServerOnBadState() ||
				(map_.size() > 1)) &&
			!Mercury::UDPChannel::allowInteractiveDebugging())
		{
			// Only handle one timeout per check because the above call
			// will likely change the collection we are iterating over.
			return pApp;
		}

		iter++;
	}

	return NULL;
}


/**
 *	This method sends on all channels to CellApps.
 */
void CellApps::sendToAll() const
{
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		iter->second->send();

		++iter;
	}
}


/**
 *	This method sends the current game time to all CellApps.
 */
void CellApps::sendGameTime( GameTime gameTime ) const
{
	CellAppInterface::setGameTimeArgs setGameTimeArgs;
	setGameTimeArgs.gameTime = gameTime;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pCellApp = iter->second;

		pCellApp->bundle() << setGameTimeArgs;
		pCellApp->send();
		++iter;
	}
}


/**
 *	This method sends some general information to all CellApps.
 */
void CellApps::sendCellAppMgrInfo( float maxCellAppLoad ) const
{
	CellAppInterface::cellAppMgrInfoArgs args;
	args.maxCellAppLoad = maxCellAppLoad;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pCellApp = iter->second;
		pCellApp->bundle() << args;
		pCellApp->send();

		++iter;
	}
}


/**
 *
 */
void CellApps::startAll( const Mercury::Address & baseAppAddr ) const
{
	CellAppInterface::startupArgs args;
	args.baseAppAddr = baseAppAddr;

	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pCellApp = iter->second;

		Mercury::Bundle & bundle = pCellApp->bundle();
		bundle << args;
		pCellApp->send();

		++iter;
	}
}


/**
 *
 */
void CellApps::setBaseApp( const Mercury::Address & baseAppAddr )
{
	CellAppInterface::setBaseAppArgs args;
	args.baseAppAddr = baseAppAddr;

	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pCellApp = iter->second;
		pCellApp->bundle() << args;
		pCellApp->send();

		++iter;
	}
}


/**
 *	This method updates each CellApp's DBApp Alpha address.
 *
 *	@param dbAppAlphaAddr	The address of DBApp Alpha.
 */
void CellApps::setDBAppAlpha( const Mercury::Address & dbAppAlphaAddr )
{
	CellAppInterface::setDBAppAlphaArgs args;
	args.addr = dbAppAlphaAddr;

	for (Map::iterator iter = map_.begin();
			iter != map_.end();
			++iter)
	{
		CellApp * pCellApp = iter->second;
		pCellApp->bundle() << args;
		pCellApp->send();
	}
}


/**
 *
 */
void CellApps::handleBaseAppDeath( const void * pBlob, int length ) const
{
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		CellApp * pCellApp = iter->second;
		Mercury::Bundle & cellAppBundle = pCellApp->bundle();
		cellAppBundle.startMessage( CellAppInterface::handleBaseAppDeath );
		cellAppBundle.addBlob( pBlob, length );

		pCellApp->send();

		++iter;
	}
}


/**
 *
 */
void CellApps::handleCellAppDeath( const Mercury::Address & addr,
		CellAppDeathHandler * pCellAppDeathHandler )
{
	this->delApp( addr );

	if (map_.empty())
	{
		pCellAppDeathHandler->finish();
	}
	else
	{
		Map::iterator iter = map_.begin();

		while (iter != map_.end())
		{
			CellApp * pCellApp = iter->second;
			Mercury::Bundle & bundle = pCellApp->bundle();

			CellAppInterface::handleCellAppDeathArgs & args =
				CellAppInterface::handleCellAppDeathArgs::start( bundle );

			args.addr = addr;

			pCellApp->send();
			pCellAppDeathHandler->addWaiting( iter->second->addr() );
			++iter;
		}
	}
}


/**
 *	This method informs all CellApps to shut down.
 */
void CellApps::shutDownAll()
{
	Map::iterator appIter = map_.begin();

	while (appIter != map_.end())
	{
		CellApp * pApp = appIter->second;
		pApp->shutDown();

		++appIter;
	}
}


/**
 *
 */
void CellApps::controlledShutDown( ShutDownStage stage, GameTime shutDownTime )
{
	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		iter->second->controlledShutDown( stage, shutDownTime );

		++iter;
	}
}


/**
 *
 */
void CellApps::setSharedData( uint8 dataType, const BW::string & key,
		const BW::string & value, const Mercury::Address & originalSrcAddr )
{
	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		Mercury::Bundle & bundle = iter->second->bundle();
		bundle.startMessage( CellAppInterface::setSharedData );
		bundle << dataType << key << value << originalSrcAddr;
		iter->second->send();

		++iter;
	}
}


/**
 *
 */
void CellApps::delSharedData( uint8 dataType, const BW::string & key,
		const Mercury::Address & originalSrcAddr )
{
	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		Mercury::Bundle & bundle = iter->second->bundle();
		bundle.startMessage( CellAppInterface::delSharedData );
		bundle << dataType << key << originalSrcAddr;
		iter->second->send();

		++iter;
	}
}


/**
 *	This method informs all CellApps that a service fragment has been added.
 *
 *	@param serviceName 			The name of the service to add a fragment for.
 *	@param fragmentMailBox		The mailbox pointing to the service fragment.
 */
void CellApps::addServiceFragment( const BW::string & serviceName, 
	const EntityMailBoxRef & fragmentMailBox )
{
	Map::iterator iCellApp = map_.begin();

	while (iCellApp != map_.end())
	{
		Mercury::Bundle & bundle = iCellApp->second->bundle();
		bundle.startMessage( CellAppInterface::addServiceFragment );
		bundle << serviceName << fragmentMailBox;
		iCellApp->second->send();

		++iCellApp;
	}
}


/**
 *	This method informs all CellApps that a service fragment has been removed.
 * 
 *	@param serviceName 		The name of the service for which to delete the
 *							fragment.
 *	@param fragmentAddress 	The address of the fragment to remove.
 */
void CellApps::delServiceFragment( const BW::string & serviceName,
	const Mercury::Address & fragmentAddress )
{
	Map::iterator iCellApp = map_.begin();

	while (iCellApp != map_.end())
	{
		Mercury::Bundle & bundle = iCellApp->second->bundle();
		bundle.startMessage( CellAppInterface::delServiceFragment );
		bundle << serviceName << fragmentAddress;
		iCellApp->second->send();

		++iCellApp;
	}
}


/**
 *	This method returns a watcher that can be used to inspect this object.
 */
WatcherPtr CellApps::pWatcher()
{
	WatcherPtr pWatcher = new MapWatcher< Map >( map_ );
	pWatcher->addChild( "*",
			new BaseDereferenceWatcher( CellApp::pWatcher() ) );

	return pWatcher;
}

BW_END_NAMESPACE

// cellapps.cpp
