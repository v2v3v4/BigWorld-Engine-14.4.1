#include "reviver_subject.hpp"

#include "reviver_common.hpp"

#include "server/bwconfig.hpp"

#include "network/udp_bundle.hpp"

DECLARE_DEBUG_COMPONENT2( "Server", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Static data
// -----------------------------------------------------------------------------

ReviverSubject ReviverSubject::instance_;


// -----------------------------------------------------------------------------
// Section: Constructor/Destructor
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ReviverSubject::ReviverSubject() :
	pInterface_( NULL ),
	reviverAddr_( 0, 0 ),
	lastPingTime_( 0 ),
	priority_( 0xff ),
	msTimeout_( 0 )
{
}


/**
 *	This method initialises this object.
 *
 *	@param pInterface The interface that reply will be sent with.
 *	@param componentName The name of the component type being monitored.
 */
void ReviverSubject::init( Mercury::NetworkInterface * pInterface,
								const char * componentName )
{
	pInterface_ = pInterface;
	char buf[128];
	bw_snprintf( buf, sizeof(buf), "reviver/%s/subjectTimeout", componentName );

	msTimeout_ = int( BWConfig::get( buf,
				BWConfig::get( "reviver/subjectTimeout", 
					REVIVER_DEFAULT_SUBJECT_TIMEOUT ) ) * 1000 );
	INFO_MSG( "ReviverSubject::init: msTimeout_ = %d\n", msTimeout_ );

	bw_snprintf( buf, sizeof(buf), "reviver/%s/pingPeriod", componentName );
	if (int( BWConfig::get( buf,
			BWConfig::get( "reviver/pingPeriod",
				REVIVER_DEFAULT_PING_PERIOD ) ) * 1000 ) > msTimeout_)
	{
		CRITICAL_MSG( "ReviverSubject::init: "
			"The revier/subjectTimeout must be larger than "
			"reviver/pingPeriod." );
	}
}


/**
 *	This method finialises this object.
 */
void ReviverSubject::fini()
{
	pInterface_ = NULL;
}


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/**
 *	This method handles messages from revivers.
 */
void ReviverSubject::handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	if (pInterface_ == NULL)
	{
		ERROR_MSG( "ReviverSubject::handleMessage: "
						"ReviverSubject not initialised\n" );
		return;
	}

	uint64 currentPingTime = timestamp();

	ReviverPriority priority;
	data >> priority;

	bool accept = (reviverAddr_ == srcAddr);

	if (!accept)
	{
		if (priority < priority_)
		{
			if (priority_ == 0xff)
			{
				INFO_MSG( "ReviverSubject::handleMessage: "
							"Reviver is %s (Priority %d)\n",
						srcAddr.c_str(), priority );
			}
			else
			{
				INFO_MSG( "ReviverSubject::handleMessage: "
							"%s has a better priority (%d)\n",
						srcAddr.c_str(), priority );
			}
			accept = true;
		}
		else
		{
			uint64 delta = (currentPingTime - lastPingTime_) * uint64(1000);
			delta /= stampsPerSecond();	// careful - don't overflow the uint64
			int msBetweenPings = int(delta);

			if (msBetweenPings > msTimeout_)
			{
				BW::string oldAddr = reviverAddr_.c_str();
				INFO_MSG( "ReviverSubject::handleMessage: "
								"%s timed out (%d ms). Now using %s\n",
							oldAddr.c_str(), msBetweenPings, srcAddr.c_str() );
				accept = true;
			}
		}
	}

	Mercury::UDPBundle bundle;

	bundle.startReply( header.replyID );

	if (accept)
	{
		reviverAddr_ = srcAddr;
		lastPingTime_ = currentPingTime;
		priority_ = priority;
		bundle << REVIVER_PING_YES;
	}
	else
	{
		bundle << REVIVER_PING_NO;
	}

	pInterface_->send( srcAddr, bundle );
}

BW_END_NAMESPACE

// reviver_subject.cpp
