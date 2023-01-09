#include "pch.hpp"

#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS

#include "stat_watcher_creator.hpp"
#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

namespace
{

/**
 *	This class helps with the creation of Watchers for StatWithRatesOfChange instances.
 */
class StatWatcherCreatorImpl
{
public:

	/**
	 *	This method initialises a collection of stat instances.
	 */
	template <class TYPE>
	void initRatesOfChangeForStats( const TYPE & stats ) const
	{
		typename TYPE::const_iterator iter = stats.begin();

		while (iter != stats.end())
		{
			this->initRatesOfChangeForStat( **iter );

			++iter;
		}
	}

	/**
	 *	This method initialises a single stat instance.
	 */
	template <class TYPE>
	void initRatesOfChangeForStat( TYPE & stat ) const
	{
		Entries::const_iterator iter = entries_.begin();

		while (iter != entries_.end())
		{
			stat.monitorRateOfChange( iter->second );

			++iter;
		}
	}

	/**
	 *	This method populates the input watcher with watchers that are
	 *	appropriate to inspect the input stat.
	 */
	template <class TYPE>
	void addWatchers( WatcherPtr pWatcher, const char * name, TYPE & stat )
	{
		this->addTotalWatcher( pWatcher, name, stat );
		this->addAvgWatchers( pWatcher, name, stat );
	}

protected:

	/**
	 *	This method is used to add a rate of change. Stats that are initialised
	 *	with this instance will have an appropriate rate added with the watcher
	 *	appropriately named.
	 *
	 *	@param name The name of the directory to place the Watcher in.
	 */
	void initRateOfChange( const char * name, float numSamples )
	{
		entries_.push_back( std::make_pair( name, numSamples ) );
	}

private:
	/**
	 *	This method adds a Watcher that inspects the total value of a stat.
	 */
	template <class TYPE>
	void addTotalWatcher( WatcherPtr pWatcher, const char * name, TYPE & stat )
	{
		char buf[ 256 ];
		bw_snprintf( buf, sizeof( buf ), "totals/%s", name );
		pWatcher->addChild( buf, makeWatcher( stat, &TYPE::total ) );
	}

	/**
	 *	This method adds Watcher instances that inspects the rates of change
	 *	associated with a stat.
	 */
	template <class TYPE>
	void addAvgWatchers( WatcherPtr pWatcher, const char * name, TYPE & stat )
	{
		typedef double (TYPE::*FunctionPtr)() const;
		FunctionPtr fns[] =
		{
			&TYPE::getRateOfChange0,
			&TYPE::getRateOfChange1,
			&TYPE::getRateOfChange2,
			&TYPE::getRateOfChange3,
			&TYPE::getRateOfChange4 // More than needed
		};

		MF_ASSERT( sizeof( fns )/sizeof( fns[0] ) >= entries_.size() );

		for (Entries::size_type i = 0; i < entries_.size(); ++i)
		{
			Entry & entry = entries_[i];

			char buf[ 256 ];

			bw_snprintf( buf, sizeof( buf ), "%s/%sPerSecond",
					entry.first.c_str(), name );
			pWatcher->addChild( buf, makeWatcher( stat,
						fns[i] ) );
		}
	}

	typedef std::pair< BW::string, float > Entry;
	typedef BW::vector< Entry > Entries;
	Entries entries_;
};


/**
 *	This class is the standard for initialising StatWithRatesOfChange instances.
 */
class StandardWatcherCreator :
	public StatWatcherCreatorImpl
{
public:
	StandardWatcherCreator()
	{
		this->initRateOfChange( "averages/lastSample", 0.f );
		this->initRateOfChange( "averages/short", 60.f );	// 1 minute at 95%
		this->initRateOfChange( "averages/medium", 600.0f );	// 10 minutes at 95%
		this->initRateOfChange( "averages/long", 3600.0f );	// 60 minutes at 95%
	}
};

StandardWatcherCreator g_watcherCreator;

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: StatWatcherCreator
// -----------------------------------------------------------------------------

namespace StatWatcherCreator
{
void initRatesOfChangeForStats(
		const UintStatWithRatesOfChangeContainer & stats )
{
	g_watcherCreator.initRatesOfChangeForStats( stats );
}

void initRatesOfChangeForStat( UintStatWithRatesOfChange & stat )
{
	g_watcherCreator.initRatesOfChangeForStat( stat );
}

void addWatchers( WatcherPtr pWatcher,
		const char * name, UintStatWithRatesOfChange & stat )
{
	g_watcherCreator.addWatchers( pWatcher, name, stat );
}

} // namespace StatWatcherCreator

BW_END_NAMESPACE

#endif // ENABLE_WATCHERS

// stat_watcher_creator.cpp
