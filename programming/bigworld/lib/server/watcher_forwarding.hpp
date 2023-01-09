#ifndef WATCHER_FORWARDING_HPP
#define WATCHER_FORWARDING_HPP

#include "cstdmf/watcher.hpp"

#include "watcher_forwarding_collector.hpp"
#include "watcher_forwarding_types.hpp"


BW_BEGIN_NAMESPACE

class WatcherPathRequestV2;

/**
 * This class implements a special Watcher for component managers. Watcher
 * requests sent to the watcher path associated with this watcher can be
 * sent via Mercury to components being mangaged by the manager.
 */
class ForwardingWatcher : public Watcher
{
public:

	/**
	 * Indicators for watcher paths that are sent via the forwarding
	 * watcher as to which components being managed that should receive
	 * the watcher request.
	 */
	enum ExposeHints
	{
		WITH_ENTITY = 0, //!< The component owns a specific entity.
		CELL_APPS, //!< All CellApps owned by the manager.
		WITH_SPACE, //!< All components with a specific space.
		LEAST_LOADED, //!< The least loaded component.
		LOCAL_ONLY, //!< Used only by Python tools
		BASE_APPS, //!< All BaseApps owned by the manager.
		SERVICE_APPS, //!< All ServiceApps owned by the manager.
		BASE_SERVICE_APPS, //!< All Base and Service Apps owned by the manager.
	};

	/**
	 * Create a new ForwardingCollector, initialised with all neccessary
	 * information, and store it in our list of collectors.
	 *
	 * @param pathRequest The WatcherPathRequest the collector should report
	 *                    back to.
	 * @param destWatcher The watcher path to be used on the receiving
	 *                    component.
	 * @param targetInfo  The target information extracted from the watcher
	 *                    path to be used in determining which components
	 *                    should be forwarded to.
	 *
	 * @returns A pointer to a new ForwardingCollector on success,
	 *          NULL on error.
	 */
	virtual ForwardingCollector *newCollector(
		WatcherPathRequestV2 & pathRequest,
		const BW::string & destWatcher,
		const BW::string & targetInfo ) = 0;


	/*
	 * Overridden from Watcher
	 */

	/**
	 * The Forwarding Watcher doesn't support the version 1 protocol.
	 *
	 * @returns Always returns false.
	 */
	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Mode & mode ) const
	{
		return false;
	}

	/**
	 * The Forwarding Watcher doesn't support the version 1 protocol.
	 *
	 * @returns Always returns false.
	 */
	virtual bool setFromString( void * base, const char * path,
		const char * valueStr )
	{
		return false;
	}


	virtual bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest );

	virtual bool getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) const;

protected:
	static const BW::string TARGET_CELL_APPS;
	static const BW::string TARGET_BASE_APPS;
	static const BW::string TARGET_SERVICE_APPS;
	static const BW::string TARGET_BASE_SERVICE_APPS;
	static const BW::string TARGET_LEAST_LOADED;

	ComponentIDList getComponentIDList( const BW::string & targetInfo );
};

BW_END_NAMESPACE

#endif // WATCHER_FORWARDING_HPP
