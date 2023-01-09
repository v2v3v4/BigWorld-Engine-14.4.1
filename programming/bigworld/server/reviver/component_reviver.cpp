#include "component_reviver.hpp"

#include "reviver_config.hpp"
#include "reviver_interface.hpp"
#include "reviver.hpp"

#include "network/machined_utils.hpp"
#include "network/nub_exception.hpp"
#include "network/udp_bundle.hpp"

#include "server/bwconfig.hpp"


DECLARE_DEBUG_COMPONENT2( "Reviver", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ComponentReviver
// -----------------------------------------------------------------------------

ComponentRevivers * g_pComponentRevivers;

/**
 *	Constructor.
 */
ComponentReviver::ComponentReviver( const char * configName, const char * name,
		const char * interfaceName, const char * createParam ) :
	IntrusiveObject< ComponentReviver >( g_pComponentRevivers ),
	pBirthMessage_( NULL ),
	pDeathMessage_( NULL ),
	pPingMessage_( NULL ),
	pDispatcher_( NULL ),
	addr_( 0, 0 ),
	configName_( configName ),
	name_( name ),
	interfaceName_( interfaceName ),
	createParam_( createParam ),
	priority_( 0 ),
	timerHandle_(),
	pingsToMiss_( 0 ),
	maxPingsToMiss_( 3 ),
	isAttached_( false ),
	isEnabled_( true )
{
}


/**
 *	Destructor.
 */
ComponentReviver::~ComponentReviver()
{
	timerHandle_.cancel();
}


/**
 *
 */
bool ComponentReviver::init( Mercury::EventDispatcher & dispatcher,
		Mercury::NetworkInterface & interface )
{
	bool isOkay = true;

	MF_ASSERT( pDispatcher_ == NULL );
	pDispatcher_ = &dispatcher;
	pInterface_ = &interface;

	BW::string prefix = "reviver/";

	float pingPeriodInSeconds =
		BWConfig::get( (prefix + configName_ + "/pingPeriod").c_str(), 
			ReviverConfig::pingPeriod() );

	if (pingPeriodInSeconds > 
			BWConfig::get( (prefix + configName_ + "/subjectTimeout").c_str(),
				ReviverConfig::subjectTimeout() ))
	{
		CRITICAL_MSG( "ComponentReviver::init: "
			"The revier/subjectTimeout must be larger than "
			"reviver/pingPeriod." );
	}

	pingPeriod_ = int( pingPeriodInSeconds * 1000000 );

	maxPingsToMiss_ =
		BWConfig::get( (prefix + configName_ + "/timeoutInPings").c_str(),
							ReviverConfig::timeoutInPings() );

	// This initialisation of the interface elements needs to be delayed
	// because VC++ has troubles getting pointers to the values before they
	// have been created globally.
	this->initInterfaceElements();

	if (Mercury::MachineDaemon::findInterface( interfaceName_.c_str(), 0,
					addr_, 4 ) != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "ComponentReviver::init: "
			"failed to find %s\n", interfaceName_.c_str() );
		isOkay = false;
	}

	Mercury::MachineDaemon::registerBirthListener( interface.address(),
			*pBirthMessage_, const_cast<char *>( interfaceName_.c_str() ) );
	Mercury::MachineDaemon::registerDeathListener( interface.address(),
			*pDeathMessage_, const_cast<char *>( interfaceName_.c_str() ) );

	return isOkay;
}


/**
 *
 */
void ComponentReviver::revive()
{
	bool wasAttached = isAttached_;

	this->deactivate();
	addr_.ip = 0;
	addr_.port = 0;

	if (wasAttached)
	{
		INFO_MSG( "Reviving %s\n", name_.c_str() );
		Reviver::pInstance()->revive( createParam_ );
	}
}


/**
 *
 */
bool ComponentReviver::activate( ReviverPriority priority )
{
	isAttached_ = false;

	if (!timerHandle_.isSet() && (addr_.ip != 0))
	{
		pingsToMiss_ = maxPingsToMiss_;
		timerHandle_ = pDispatcher_->addTimer( pingPeriod_, this, 
			NULL, "ComponentReviver" );
		priority_ = priority;
		return true;
	}

	return false;
}


/**
 *
 */
bool ComponentReviver::deactivate()
{
	if (isAttached_)
	{
		Reviver::pInstance()->markAsDirty();
		INFO_MSG( "ComponentReviver: %s (%s) has detached\n",
			addr_.c_str(), name_.c_str() );
		isAttached_ = false;
	}

	if (timerHandle_.isSet())
	{
		timerHandle_.cancel();
		priority_ = 0;
		return true;
	}

	return false;
}


/**
 *	This method handles the death message.
 */
void ComponentReviver::handleMessage( const Mercury::Address & source,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	MF_ASSERT( (header.identifier == pBirthMessage_->id()) ||
				(header.identifier == pDeathMessage_->id()) );

	Mercury::Address addr;
	data >> addr;

	if (header.identifier == pBirthMessage_->id())
	{
		addr_ = addr;
		INFO_MSG( "ComponentReviver::handleMessage: "
				"%s at %s has started.\n",
			name_.c_str(),
			addr.c_str() );
		return;
	}

	INFO_MSG( "ComponentReviver::handleMessage: %s at %s has died.\n",
		name_.c_str(),
		addr.c_str() );

	if (addr == addr_)
	{
		this->revive();
	}
	else if (isAttached_)
	{
		BW::string currAddrStr = addr_.c_str();
		ERROR_MSG( "ComponentReviver::handleMessage: "
				"%s component died at %s. Expected %s\n",
			name_.c_str(),
			addr.c_str(),
			currAddrStr.c_str() );
	}
}


/**
 *
 */
void ComponentReviver::handleMessage( const Mercury::Address & source,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	void * arg )
{
	uint8 returnCode;
	data >> returnCode;
	if (returnCode == REVIVER_PING_YES)
	{
		pingsToMiss_ = maxPingsToMiss_;

		if (!isAttached_)
		{
			Reviver::pInstance()->markAsDirty();
			INFO_MSG( "ComponentReviver: %s (%s) has attached.\n",
				addr_.c_str(), name_.c_str() );
			isAttached_ = true;
		}
	}
	else
	{
		this->deactivate();
	}
}


/**
 *
 */
void ComponentReviver::handleTimeout( TimerHandle /*handle*/, void * /*arg*/ )
{
	if (pingsToMiss_ > 0)
	{
		--pingsToMiss_;
		Mercury::UDPBundle bundle;
		bundle.startRequest( *pPingMessage_, this );
		bundle << priority_;
		pInterface_->send( addr_, bundle );
	}
	else
	{
		INFO_MSG( "ComponentReviver::handleTimeout: Missed too many\n" );
		this->revive();
	}
}

void ComponentReviver::handleException( const Mercury::NubException & ne,
	void * /*arg*/ )
{
	// We should really be detached if we get an exception.
	if (isAttached_)
	{
		ERROR_MSG( "ReviverReplyHandler::handleMessage: "
									"%s got an exception (%s).\n",
				name_.c_str(),
				Mercury::reasonToString( ne.reason() ) );
	}
}

// -----------------------------------------------------------------------------
// Section: ComponentReviver specialisation
// -----------------------------------------------------------------------------

BW_END_NAMESPACE

// Interface includes
#include "db/dbapp_interface.hpp"
#include "db/dbappmgr_interface.hpp"
#include "cellappmgr/cellappmgr_interface.hpp"
#include "baseappmgr/baseappmgr_interface.hpp"
#include "loginapp/login_int_interface.hpp"

BW_BEGIN_NAMESPACE


#define MF_REVIVER_HANDLER( CONFIG, COMPONENT, CREATE_WHAT )				\
	MF_REVIVER_HANDLER2( CONFIG, COMPONENT, COMPONENT, CREATE_WHAT )

#define MF_REVIVER_HANDLER2( CONFIG, COMPONENT, COMPONENT2, CREATE_WHAT )	\
/** @internal */															\
class COMPONENT##Reviver : public ComponentReviver							\
{																			\
public:																		\
	COMPONENT##Reviver() :													\
		ComponentReviver( #CONFIG, #COMPONENT, #COMPONENT2 "Interface",		\
				CREATE_WHAT )												\
	{}																		\
	virtual void initInterfaceElements()									\
	{																		\
		pBirthMessage_ = &ReviverInterface::handle##COMPONENT##Birth;		\
		pDeathMessage_ = &ReviverInterface::handle##COMPONENT##Death;		\
		pPingMessage_ = &COMPONENT2##Interface::reviverPing;				\
	}																		\
} g_reviverOf##COMPONENT;													\


MF_REVIVER_HANDLER( cellAppMgr, CellAppMgr, "cellappmgr" )
MF_REVIVER_HANDLER( baseAppMgr, BaseAppMgr, "baseappmgr" )
MF_REVIVER_HANDLER( dbAppMgr,   DBAppMgr,	"dbappmgr" )
MF_REVIVER_HANDLER( dbApp,      DBApp,		"dbapp" )
MF_REVIVER_HANDLER2( loginApp,   Login, LoginInt,   "loginapp" )

BW_END_NAMESPACE


// -----------------------------------------------------------------------------
// Section: Interfaces
// -----------------------------------------------------------------------------

#include "loginapp/login_int_interface.hpp"
#define DEFINE_INTERFACE_HERE
#include "loginapp/login_int_interface.hpp"

// We serve this interface
#define DEFINE_SERVER_HERE
#include "reviver_interface.hpp"

// component_reviver.cpp
