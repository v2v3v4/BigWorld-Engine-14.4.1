#include "watcher_forwarding_cellapp.hpp"

#include "cellapp.hpp"
#include "cellapps.hpp"
#include "cellappmgr.hpp"

#include "cellapp/cellapp_interface.hpp"
#include "cstdmf/watcher_path_request.hpp"
#include "server/watcher_forwarding_collector.hpp"


BW_BEGIN_NAMESPACE

class CollectAddresses : public CellAppVisitor
{
public:
	CollectAddresses( AddressList & list ) : list_( list ) {}

	void onVisit( CellApp * pCellApp )
	{
		if (this->shouldAdd( pCellApp ))
		{
			list_.push_back( AddressPair( pCellApp->addr(),
						Component( CELL_APP, pCellApp->id() ) ) );
		}
	}

	virtual bool shouldAdd( CellApp * pApp ) const { return true; }

private:
	AddressList & list_;
};

class CollectSomeAddresses : public CollectAddresses
{
public:
	CollectSomeAddresses( const ComponentIDList & targetList,
			AddressList & list ) :
		CollectAddresses( list ),
		targetList_( targetList ) {}

	virtual bool shouldAdd( CellApp * pCellApp ) const
	{
		return std::find( targetList_.begin(), targetList_.end(), pCellApp->id() )
			!= targetList_.end();
	}

private:
	const ComponentIDList & targetList_;
};



// Overridden from ForwardingCollector
ForwardingCollector *CAForwardingWatcher::newCollector(
	WatcherPathRequestV2 & pathRequest,
	const BW::string & destWatcher,
	const BW::string & targetInfo )
{
	ForwardingCollector *collector =
		new ForwardingCollector( pathRequest, destWatcher );
	if (collector == NULL)
	{
		ERROR_MSG( "newCollector: Failed to create new watcher "
					"ForwardingCollector.\n" );
		return NULL;
	}

	AddressList *targetList = this->getComponentAddressList( targetInfo );

	if (!collector->init( CellAppInterface::callWatcher,
		CellAppMgr::instance().interface(), targetList ))
	{
		ERROR_MSG( "CAForwardingWatcher::newCollector: Failed to initialise "
					"ForwardingCollector.\n" );
		delete collector;
		collector = NULL;

		if (targetList)
			delete targetList;
	}

	return collector;
}



/**
 * Generate a list of CellApp(s) (Component ID, Address pair) as determined by
 * the target information for the forwarding watchers.
 *
 * @param targetInfo A string describing which component(s) to add to the list.
 *
 * @returns A list of AddressPair's to forward a watcher request to.
 */
AddressList * CAForwardingWatcher::getComponentAddressList(
	const BW::string & targetInfo )
{
	AddressList * list = new AddressList;
	if (list == NULL)
	{
		ERROR_MSG( "CAForwardingWatcher::getComponentList: Failed to "
					"generate component list.\n" );
		return list;
	}

	CellApps & cellApps = CellAppMgr::instance().cellApps();

	if (targetInfo == TARGET_LEAST_LOADED)
	{
		const CellApps & cellApps = CellAppMgr::instance().cellApps();
		CellApp * pCellApp = cellApps.findLeastLoadedCellApp();

		if (pCellApp)
		{
			list->push_back(
					AddressPair( pCellApp->addr(),
						Component( CELL_APP, pCellApp->id() ) ) );
		}

	}
	else if (targetInfo == TARGET_CELL_APPS)
	{
		CollectAddresses visitor( *list );
		cellApps.visit( visitor );
	}
	else
	{
		// TODO: possible optimisation here
		ComponentIDList targetList = this->getComponentIDList( targetInfo );
		CollectSomeAddresses visitor( targetList, *list );
		cellApps.visit( visitor );
	}

	return list;
}

BW_END_NAMESPACE

// watcher_forwarding_cellapp.cpp
