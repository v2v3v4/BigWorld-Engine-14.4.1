#include "pch.hpp"

#include "machined_utils.hpp"

#include "blocking_reply_handler.hpp"
#include "portmap.hpp"
#include "udp_bundle.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/bwversion.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

namespace Mercury
{

namespace MachineDaemon
{

// ProcessMessage Handler for registerWithMachined to determine whether it
// received a valid response.
class ProcessMessageHandler : public MachineGuardMessage::ReplyHandler
{
public:
	ProcessMessageHandler() { hasResponded_ = false; }
	bool onProcessMessage( ProcessMessage &pm, uint32 addr );

	bool hasResponded_;
};


/**
 *  Handler to notify registerWithMachined() of the receipt of an expected
 *  message.
 */
bool ProcessMessageHandler::onProcessMessage(
		ProcessMessage &pm, uint32 addr )
{
	hasResponded_ = true;
	return false;
}


/**
 *	This function registers a socket with BWMachined.
 */
Reason registerWithMachined( const Address & srcAddr,
		const BW::string & name, int id, bool isRegister )
{
	if (name.empty())
	{
		return REASON_SUCCESS;
	}

#ifdef MF_SERVER
	// Do not call blocking reply handler after registering with bwmachined as
	// other processes can now find us and send other messages.
	BlockingReplyHandler::safeToCall( false );
#endif

	ProcessMessage pm;

	pm.param_ = (isRegister ? pm.REGISTER : pm.DEREGISTER) |
		pm.PARAM_IS_MSGTYPE;
	pm.category_ = ProcessMessage::SERVER_COMPONENT;
	pm.port_ = srcAddr.port;
	pm.name_ = name;
	pm.id_ = id;

	pm.majorVersion_ = BWVersion::majorNumber();
	pm.minorVersion_ = BWVersion::minorNumber();
	pm.patchVersion_ = BWVersion::patchNumber();

	ProcessMessageHandler pmh;

	// send and wait for the reply
	const uint32 destAddr = LOCALHOST;

	Reason response = pm.sendAndRecv( srcAddr.ip, destAddr, &pmh );

	return pmh.hasResponded_ ? response : REASON_TIMER_EXPIRED;
}


/**
 *	This function deregisters a socket with BWMachined.
 */
Reason deregisterWithMachined( const Address & srcAddr,
		const BW::string & name, int id )
{
	return name.empty() ?
		REASON_SUCCESS :
		registerWithMachined( srcAddr, name, id, /*isRegister:*/ false );
}


// -----------------------------------------------------------------------------
// Section: Birth and Death
// -----------------------------------------------------------------------------

/**
 *	This method is used to register a birth or death listener with machined.
 */
Reason registerListener( const Address & srcAddr,
		UDPBundle & bundle, int addrStart,
		const char * ifname, bool isBirth, bool anyUID = false )
{
	// finalise the bundle first
	bundle.finalise();
	const Packet * p = bundle.pFirstPacket();

	MF_ASSERT( p->flags() == 0 );

	// prepare the message for machined
	ListenerMessage lm;
	lm.param_ = (isBirth ? lm.ADD_BIRTH_LISTENER : lm.ADD_DEATH_LISTENER) |
		lm.PARAM_IS_MSGTYPE;
	lm.category_ = lm.SERVER_COMPONENT;
	lm.uid_ = anyUID ? lm.ANY_UID : getUserId();
	lm.pid_ = mf_getpid();
	lm.port_ = srcAddr.port;
	lm.name_ = ifname;

	const int addrLen = 6;
	unsigned int postSize = p->totalSize() - addrStart - addrLen;

	lm.preAddr_ = BW::string( p->data(), addrStart );
	lm.postAddr_ = BW::string( p->data() + addrStart + addrLen, postSize );

	const uint32 destAddr = LOCALHOST;
	return lm.sendAndRecv( srcAddr.ip, destAddr, NULL );
}


/**
 *  This method registers a callback with machined to be called when a certain
 *	type of process is started.
 *
 *	@note This needs to be fixed up if rebind is called on this nub.
 */
Reason registerBirthListener( const Address & srcAddr,
		UDPBundle & bundle, int addrStart, const char * ifname )
{
	return registerListener( srcAddr, bundle, addrStart, ifname, true );
}


/**
 *  This method registers a callback with machined to be called when a certain
 *	type of process stops unexpectedly.
 *
 *	@note This needs to be fixed up if rebind is called on this nub.
 */
Reason registerDeathListener( const Address & srcAddr,
		UDPBundle & bundle, int addrStart, const char * ifname )
{
	return registerListener( srcAddr, bundle, addrStart, ifname, false );
}


/**
 *	This method registers a callback with machined to be called when a certain
 *	type of process is started.
 *
 *	@param srcAddr	The address of the process registering interest.
 *	@param ie		The interface element of the callback message. The message
 *				must be callable with one parameter of type Mercury::Address.
 *	@param ifname	The name of the interface to watch for.
 */
Reason registerBirthListener( const Address & srcAddr,
		const InterfaceElement & ie, const char * ifname )
{
	Mercury::UDPBundle bundle;

	bundle.startMessage( ie, RELIABLE_NO );
	int startOfAddress = bundle.size();
	bundle << Mercury::Address::NONE;

	return registerBirthListener( srcAddr, bundle, startOfAddress, ifname );
}


/**
 *  This method registers a callback with machined to be called when a certain
 *	type of process stops unexpectedly.
 *
 *	@param srcAddr	The address of the process registering interest.
 *	@param ie		The interface element of the callback message. The message
 *				must be callable with one parameter of type Mercury::Address.
 *	@param ifname	The name of the interface to watch for.
 */
Reason registerDeathListener( const Address & srcAddr,
		const InterfaceElement & ie, const char * ifname )
{
	Mercury::UDPBundle bundle;

	bundle.startMessage( ie, RELIABLE_NO );
	int startOfAddress = bundle.size();
	bundle << Mercury::Address::NONE;

	return registerDeathListener( srcAddr, bundle, startOfAddress, ifname );
}


/**
 *	This function asks the machined process at the destination IP address to
 *	send a signal to the BigWorld process at the specified port.
 */
bool sendSignalViaMachined( const Mercury::Address & dest, int sigNum )
{
	SignalMessage sm;
	sm.signal_ = sigNum;
	sm.port_ = dest.port;
	sm.param_ = sm.PARAM_USE_PORT;

	Endpoint tempEP;
	tempEP.socket( SOCK_DGRAM );

	if (tempEP.good() && tempEP.bind() == 0)
	{
		sm.sendto( tempEP, htons( PORT_MACHINED ), dest.ip );
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Section: Search
// -----------------------------------------------------------------------------


/**
 * Constructor
 */
IFindInterfaceHandler::IFindInterfaceHandler() :
	MachineGuardMessage::ReplyHandler(),
	hasError_( false )
{
}


/*
 * Override from MachineGuardMessage::ReplyHandler
 */
bool IFindInterfaceHandler::onProcessStatsMessage(
		ProcessStatsMessage & psm, uint32 ip )
{
	if (psm.pid_ == 0)
	{
		return true;
	}

	if ((psm.interfaceVersion_ != 0) &&
		(MERCURY_INTERFACE_VERSION != psm.interfaceVersion_))
	{
		ERROR_MSG( "Interface %s has version %d. Expected %d\n",
				psm.name_.c_str(), psm.interfaceVersion_,
				MERCURY_INTERFACE_VERSION );

		hasError_ = true;
		return false;
	}

	if (!psm.username_.empty() &&
			(psm.username_ != getUsername()))
	{
		ERROR_MSG( "Interface %s has username %s. Expected %s\n",
				psm.name_.c_str(), psm.username_.c_str(),
				getUsername() );

		hasError_ = true;
		return false;
	}

	Address addr;
	addr.ip = ip;
	addr.port = psm.port_;
	addr.salt = 0;

	DEBUG_MSG( "IFindInterfaceHandler::onProcessStatsMessage: "
			"Found interface %s at %s:%d\n",
		psm.name_.c_str(), inet_ntoa( (in_addr&)addr.ip ),
		ntohs( addr.port ) );

	return this->onProcessMatched( addr, psm );
}


/**
 *	Finds first interface.
 */
class FindFirstInterfaceHandler : public IFindInterfaceHandler
{
public:
	FindFirstInterfaceHandler() :
		IFindInterfaceHandler(),
		addr_( Address::NONE ) {}

	/* Override from IFindInterfaceHandler */
	bool onProcessMatched( Address & addr,
			const ProcessStatsMessage & psm ) /* override */
	{
		addr_ = addr;
		return /* shouldContinue */ false;
	}

	/* Override from IFindInterfaceHandler */
	Address result() const /* override */
	{
		return addr_;
	}

private:
	Address addr_;
};


/**
 * 	This method finds a specified interface on the network.
 * 	WARNING: This function always blocks.
 *
 *	@param name			Only interfaces with this name are considered.
 *	@param id			Only interfaces with this ID are considered, if
 *						negative all are considered.
 *	@param addr			Output address of the found interface.
 *	@param retries		The number of retries if no interface is found.
 *	@param verboseRetry	Flag for versbose output on retry.
 *	@param pHandler		Handler to process ProcessStatsMessages responses. If
 *						NULL, the default FindFirstInterfaceHandler is used.
 *
 * 	@return	A Mercury::REASON_SUCCESS if an interface was found, a
 * 	Mercury::REASON_TIMER_EXPIRED if an interface was not found, other
 * 	Mercury::Reasons are returned if there is an error.
 */
Reason findInterface( const char * name, int id,
		Address & addr, int retries, bool verboseRetry,
		IFindInterfaceHandler * pHandler )
{
	ProcessStatsMessage pm;
	pm.param_ = pm.PARAM_USE_CATEGORY |
		pm.PARAM_USE_UID |
		pm.PARAM_USE_NAME |
		(id <= 0 ? 0 : pm.PARAM_USE_ID);
	pm.category_ = pm.SERVER_COMPONENT;
	pm.uid_ = getUserId();
	pm.name_ = name;
	pm.id_ = id;

	IFindInterfaceHandler * pDefaultHandler = NULL;

	if (!pHandler)
	{
		pDefaultHandler = new FindFirstInterfaceHandler();
		pHandler = pDefaultHandler;
	}

	int attempt = 0;

	retries = std::max( retries, 1 );

	while (++attempt <= retries)
	{
		Reason reason = pm.sendAndRecv( 0, BROADCAST, pHandler );

		if (reason != REASON_SUCCESS)
		{
			return reason;
		}

		if (pHandler->hasError())
		{
			return REASON_GENERAL_NETWORK;
		}

		Address result = pHandler->result();

		if (result != Address::NONE)
		{
			addr = result;
			return REASON_SUCCESS;
		}

		if (verboseRetry)
		{
			INFO_MSG( "MachineDaemon::findInterface: "
					"Failed to find %s for UID %d on attempt %d.\n",
				name, pm.uid_, attempt );
		}

		// Sleep a little because sendAndReceiveMGM() is too fast now! :)
#if defined( PLAYSTATION3 )
		sys_timer_sleep( 1 );
#elif !defined( _WIN32 )
		sleep( 1 );
#else
		Sleep( 1000 );
#endif
	}

	if (pDefaultHandler)
	{
		bw_safe_delete( pDefaultHandler );
	}

	return REASON_TIMER_EXPIRED;
}


/**
 *	This class is used by queryForInternalInterface.
 */
class QueryInterfaceHandler : public MachineGuardMessage::ReplyHandler
{
public:
	QueryInterfaceHandler( int requestType );
	bool onQueryInterfaceMessage( QueryInterfaceMessage & message,
			uint32 addr );

	bool hasResponded() const	{ return hasResponded_; }
	u_int32_t address() const	{ return address_; }

private:
	bool hasResponded_;
	u_int32_t address_;
	char request_;
};


/**
 *	Constructor.
 *
 *	@param requestType The type of request to make.
 */
QueryInterfaceHandler::QueryInterfaceHandler( int requestType ) :
	hasResponded_( false ),
	request_( requestType )
{
}


/**
 *	This method is called when a reply is received from bwmachined from our
 *	request.
 */
bool QueryInterfaceHandler::onQueryInterfaceMessage(
	QueryInterfaceMessage & message, uint32 addr )
{
	address_ = message.address_;
	hasResponded_ = true;

	return false;
}


/**
 *	This function queries the bwmachined daemon for what it thinks is the
 *	internal interface.
 *
 *	@param addr A reference to where the address will be written on success.
 *
 *	@return True on success.
 */
bool queryForInternalInterface( u_int32_t & addr )
{
	Endpoint ep;
	u_int32_t ifaddr = LOCALHOST;

	ep.socket( SOCK_DGRAM );

	// First we have to send a message over the local interface to
	// bwmachined to ask for which interface to treat as 'internal'.
	if (ep.getInterfaceAddress( "lo", ifaddr ) != 0)
	{
		WARNING_MSG( "MachineDaemon::queryForInternalInterface: "
			"Could not get 'lo' by name, defaulting to 127.0.0.1\n" );
		ifaddr = LOCALHOST;
	}

	QueryInterfaceMessage message;
	QueryInterfaceHandler handler( QueryInterfaceMessage::INTERNAL );

	if (REASON_SUCCESS != message.sendAndRecv( ep, ifaddr, &handler ))
	{
		ERROR_MSG( "MachineDaemon::queryForInternalInterface: "
			"Failed to send interface discovery message to bwmachined.\n" );
		return false;
	}

	if (handler.hasResponded())
	{
		addr = handler.address();
		return true;
	}

	return false;
}

} // namespace MachineDaemon

} // namespace Mercury

BW_END_NAMESPACE

// machined_utils.cpp
