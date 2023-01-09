#include "reviver.hpp"

#include "component_reviver.hpp"
#include "reviver_config.hpp"
#include "reviver_interface.hpp"

#include "cstdmf/build_config.hpp"
#include "cstdmf/intrusive_object.hpp"

#include "network/event_dispatcher.hpp"
#include "network/machined_utils.hpp"
#include "network/nub_exception.hpp"
#include "network/portmap.hpp"
#include "network/watcher_nub.hpp"

#include "resmgr/bwresource.hpp"
#include "server/bwconfig.hpp"
#include "server/reviver_common.hpp"


DECLARE_DEBUG_COMPONENT2( "Reviver", 0 )


BW_BEGIN_NAMESPACE

/// Reviver Singleton.
BW_SINGLETON_STORAGE( Reviver )


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Reviver::Reviver( Mercury::EventDispatcher & mainDispatcher,
	   Mercury::NetworkInterface & interface ) :
	ServerApp( mainDispatcher, interface ),
	shuttingDown_( false ),
	isDirty_( true )
{
}


/**
 *	Destructor.
 */
Reviver::~Reviver()
{
	timerHandle_.cancel();
	tickTimer_.cancel();
}


/**
 *	This method initialises the reviver.
 */
bool Reviver::init( int argc, char * argv[] )
{
	if (!this->ServerApp::init( argc, argv ))
	{
		return false;
	}

	// Shouldn't be initialised yet.
	MF_ASSERT( components_.empty() );

	PROC_IP_INFO_MSG( "Internal address = %s\n", interface_.address().c_str() );

	ReviverInterface::registerWithInterface( interface_ );

	if (ReviverInterface::registerWithMachined( interface_, 0 ) !=
		Mercury::REASON_SUCCESS)
	{
		NETWORK_WARNING_MSG( "Reviver::init: Unable to register interface\n" );
		return false;
	}

	// TODO: Make it configurable which processes this is able to revive.
	if (g_pComponentRevivers == NULL)
	{
		ERROR_MSG( "Reviver::init: No component revivers\n" );
		return false;
	}

	components_ = *g_pComponentRevivers;

	if (!this->queryMachinedSettings())
	{
		return false;
	}

	this->ServerApp::addWatchers( Watcher::rootWatcher() );

	BW_REGISTER_WATCHER( 0, "reviver", "reviver",
		mainDispatcher_, interface_.address() );

	ComponentRevivers::iterator endIter = components_.end();

	bool isFirstAdd = true;
	bool isFirstDel = true;

	for (int i = 1; i < argc - 1; ++i)
	{
		const bool isAdd = (strcmp( argv[i], "--add" ) == 0);
		const bool isDel = (strcmp( argv[i], "--del" ) == 0);

		if (isAdd || isDel)
		{
			++i;

			if (isAdd && isFirstAdd)
			{
				isFirstAdd = false;
				ComponentRevivers::iterator iter = components_.begin();

				while (iter != endIter)
				{
					(*iter)->isEnabled( false );

					++iter;
				}
			}

			{
				bool found = false;
				ComponentRevivers::iterator iter = components_.begin();

				while (iter != endIter)
				{
					if (((*iter)->configName() == argv[i]) ||
							// Support the lower-case version too.
							strcmp( (*iter)->createName(), argv[i] ) == 0)
					{
						found = true;
						(*iter)->isEnabled( isAdd );
					}

					++iter;
				}

				if (!found)
				{
					ERROR_MSG( "Reviver::init: Invalid command line. "
							"No such component %s\n", argv[i] );
					return false;
				}
			}
		}
	}

	if (!isFirstAdd && !isFirstDel)
	{
		ERROR_MSG( "Reviver::init: "
					"Cannot mix --add and --del command line options\n" );
		return false;
	}

	// Initialise the ComponentRevivers.
	{
		ComponentRevivers::iterator iter = components_.begin();

		while (iter != endIter)
		{
			if ((*iter)->isEnabled())
			{
				(*iter)->init( mainDispatcher_, interface_ );
			}

			++iter;
		}
	}

	// Information about which types are supported.
	{
		CONFIG_INFO_MSG( "Monitoring the following component types:\n" );
		ComponentRevivers::iterator iter = components_.begin();

		while (iter != endIter)
		{
			if ((*iter)->isEnabled())
			{
				CONFIG_INFO_MSG( "\t%s\n", (*iter)->name().c_str() );
				CONFIG_INFO_MSG( "\t\tPing Period = %.1f seconds\n",
						double((*iter)->pingPeriod())/1000000.f );
				CONFIG_INFO_MSG( "\t\tTimeout     = %d pings\n",
						(*iter)->maxPingsToMiss() );
			}

			++iter;
		}
	}

	// Activate the ComponentRevivers.
	{
		ReviverPriority priority = 0;

		ComponentRevivers::iterator iter = components_.begin();

		while (iter != endIter)
		{
			if ((*iter)->isEnabled())
			{
				(*iter)->activate( ++priority );
			}

			++iter;
		}
	}

	timerHandle_ = mainDispatcher_.addTimer(
					int(Config::reattachPeriod() * 1000000),
					this, (void *)TIMEOUT_REATTACH );

	tickTimer_ = mainDispatcher_.addTimer(
					1000000/Config::updateHertz(),
					this, (void *)TIMEOUT_TICK,
					"TickTimer" );

	return true;
}


/**
 *
 */
bool Reviver::TagsHandler::onTagsMessage( TagsMessage &tm, uint32 addr )
{
	if (tm.exists_)
	{
		Tags &tags = tm.tags_;
		ComponentRevivers::iterator iter = reviver_.components_.begin();
		ComponentRevivers::iterator endIter = reviver_.components_.end();

		while (iter != endIter)
		{
			ComponentReviver & component = **iter;

			if (std::find( tags.begin(), tags.end(), component.createName() )
				!= tags.end() ||
				std::find( tags.begin(), tags.end(), component.configName() )
				!= tags.end())
			{
				component.isEnabled( true );
			}
			else
			{
				CONFIG_INFO_MSG( "\t%s disabled via bwmachined's "
							"Components tags\n",
						component.name().c_str() );
				component.isEnabled( false );
			}

			++iter;
		}
	}
	else
	{
		CONFIG_ERROR_MSG( "Reviver::init: "
			"BWMachined has no Components tags\n" );
	}

	return false;
}


/**
 *	This method queries the local bwmachined for its tags associated with
 *	Components. This is the set of Component types that the machine can run.
 *	This is used to restrict the types of components that this can revive.
 */
bool Reviver::queryMachinedSettings()
{
	TagsMessage query;
	query.tags_.push_back( BW::string( "Components" ) );

	TagsHandler handler( *this );
	int reason;

	if ((reason = query.sendAndRecv( 0, LOCALHOST, &handler )) !=
		Mercury::REASON_SUCCESS)
	{
		NETWORK_ERROR_MSG( "Reviver::queryMachinedSettings: "
				"MGM query failed (%s)\n",
			Mercury::reasonToString( (Mercury::Reason&)reason ) );
		return false;
	}

	return true;
}


/**
 *	This method handles timer events.
 */
void Reviver::handleTimeout( TimerHandle handle, void * arg )
{
	typedef BW::map< ReviverPriority, ComponentReviver * > Map;

	switch (uintptr(arg))
	{
		case TIMEOUT_REATTACH:
		{
			Map activeSet;
			ComponentRevivers deactive;

			components_ = *g_pComponentRevivers;
			ComponentRevivers::iterator iter = components_.begin();
			ComponentRevivers::iterator endIter = components_.end();

			while (iter != endIter)
			{
				if ((*iter)->isEnabled())
				{
					ReviverPriority priority = (*iter)->priority();

					if (priority > 0)
					{
						activeSet[ priority ] = (*iter);
					}
					else
					{
						deactive.push_back( *iter );
					}
				}

				++iter;
			}

			// Adjust priorities
			{
				Map::iterator mapIter = activeSet.begin();
				ReviverPriority priority = 0;

				while (mapIter != activeSet.end())
				{
					++priority;
					if (mapIter->first != priority)
					{
						mapIter->second->priority( priority );
					}

					++mapIter;
				}

				std::random_shuffle( deactive.begin(), deactive.end() );

				iter = deactive.begin();
				endIter = deactive.end();

				while (iter != endIter)
				{
					(*iter)->activate( ++priority );
					++iter;
				}
			}

			if (isDirty_)
			{
				INFO_MSG( "---- Attached components summary ----\n" );

				if (!activeSet.empty())
				{

					Map::iterator mapIter = activeSet.begin();

					while (mapIter != activeSet.end())
					{
						INFO_MSG( "%d: (%s) %s\n",
							mapIter->second->priority(),
							mapIter->second->addr().c_str(),
							mapIter->second->name().c_str() );

						++mapIter;
					}
				}
				else
				{
					INFO_MSG( "No attached components\n" );
				}

				isDirty_ = false;
			}
		}
		break;

		case TIMEOUT_TICK:
		{
			this->advanceTime();
			break;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/**
 *	This method runs the main loop of this process.
 */
bool Reviver::run()
{
	if (!this->hasEnabledComponents())
	{
		INFO_MSG( "Reviver::run:"
				"No components enabled to revive. Shutting down.\n" );
	}

	return this->ServerApp::run();
}


/**
 *	This method shuts this process down.
 */
void Reviver::shutDown()
{
	shuttingDown_ = true;
	mainDispatcher_.breakProcessing();

	ComponentRevivers::iterator iter = components_.begin();
	while (iter != components_.end())
	{
		if ((*iter)->isEnabled())
		{
			(*iter)->deactivate();
		}

		++iter;
	}
}


/**
 *	This method sends a message to machined so that the input process is revived.
 */
void Reviver::revive( const char * createComponent )
{
	if (shuttingDown_)
	{
		INFO_MSG( "Reviver::revive: "
			"Trying to revive a process while shutting down.\n" );
		return;
	}


	CreateMessage cm;
	cm.uid_ = getUserId();
	cm.recover_ = 1;
	cm.name_ = createComponent;
	cm.config_ = BW_COMPILE_TIME_CONFIG;

	// TODO: The original output forwarding will not be preserved by this, and
	// we aren't ever using multicast

	// TODO: It would be good to know if machined actually successfully starts
	// the process. One reason that it can fail is if machined.conf is not
	// set up correctly.

	uint32 srcaddr = 0, destaddr = htonl( 0x7f000001U );
	if (cm.sendAndRecv( srcaddr, destaddr ) != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "ComponentReviver::revive: Could not send request.\n" );
	}

	if (Config::shutDownOnRevive())
	{
		shuttingDown_ = true;
		this->shutDown();
	}
}


/**
 * This method checks if there any enabled components
 */
bool Reviver::hasEnabledComponents() const
{
	ComponentRevivers::const_iterator iter = components_.begin();
	ComponentRevivers::const_iterator endIter = components_.end();

	while (iter != endIter)
	{
		if ((*iter)->isEnabled())
		{
			return true;
		}
		iter++;
	}

	return false;
}

BW_END_NAMESPACE

// reviver.cpp
