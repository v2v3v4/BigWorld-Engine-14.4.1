#include "bwtracer.hpp"

#include "connection/client_interface.hpp"
#include "connection/baseapp_ext_interface.hpp"

#include "server/bwconfig.hpp"

#include "network/network_interface.hpp"
#include "network/packet_monitor.hpp"

#ifndef _WIN32  // WIN32PORT
#else //ifndef _WIN32  // WIN32PORT
#include <time.h>
#endif //ndef _WIN32  // WIN32PORT

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BWTracer declaration
// -----------------------------------------------------------------------------

/**
 *	This class is used to monitor the packets sent and received.
 */
class BWTracer : public Mercury::PacketMonitor
{
public:
	BWTracer();

	void startLogging( Mercury::NetworkInterface & interface,
			const char* filename );
	void stopLogging();

	virtual void packetIn( const Mercury::Address & addr,
			const Mercury::Packet & packet);
	virtual void packetOut( const Mercury::Address& addr,
			const Mercury::Packet & packet);

	void setFlushMode( bool flushMode )		{ flushMode_ = flushMode; }
	void setHexMode( bool hexMode )			{ hexMode_ = hexMode; }
	void setFilterAddr( uint32 filterAddr )	{ filterAddr_ = filterAddr; }

private:

	// Helper functions
	void dumpTimeStamp();
	void dumpHeader();
	void dumpMessages();
	void hexDump();
	int32 readVarLength(int bytes);

	// Client message handlers
	int clientBandwidthNotification();
	int clientTickSync();
	int clientEnterAoI();
	int clientLeaveAoI();
	int clientCreateEntity();

	// Proxy message handlers
	int proxyAuthenticate();
	int proxyAvatarUpdateImplicit();
	int proxyAvatarUpdateExplicit();
	int proxyRequestEntityUpdate();
	int proxyEnableEntities();

	// General message handlers
	int entityMessage();
	int replyMessage();
	int piggybackMessage();

private:

	FILE* 			pFile_;	
	Mercury::NetworkInterface *	pInterface_;
	const char* 	pData_;
	int				len_;
	uint8			msgId_;
	bool			flushMode_;
	bool			hexMode_;
	uint32			filterAddr_;

	typedef int (BWTracer::*MsgHandler)();

	struct MsgDef
	{
		MsgHandler 	handler;
		int			minBytes;
	};

	MsgDef*			msgTable_;
	MsgDef			proxyMsg_[256];
	MsgDef			clientMsg_[256];
};


// -----------------------------------------------------------------------------
// Section: BWTracerHolder implementation
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
BWTracerHolder::BWTracerHolder() : pImpl_( NULL )
{
}


/**
 *	Destructor.
 */
BWTracerHolder::~BWTracerHolder()
{
	if (pImpl_)
	{
		pImpl_->stopLogging();
		delete pImpl_;
	}
}

void BWTracerHolder::init( Mercury::NetworkInterface & networkInterface )
{
	if (BWConfig::get( "baseApp/packetLog/enable", false ))
	{
		TRACE_MSG( "BaseApp: Logging packets to proxy.log\n" );
		pImpl_ = new BWTracer();
		pImpl_->startLogging( networkInterface, "proxy.log" );
		pImpl_->setFlushMode(
				BWConfig::get( "baseApp/packetLog/flushMode", false ));
		pImpl_->setHexMode(
				BWConfig::get( "baseApp/packetLog/hexMode", false ) );

		BW::string filterAddr = BWConfig::get( "baseApp/packetLog/addr" );

		if (filterAddr != "")
		{
			pImpl_->setFilterAddr(inet_addr(filterAddr.c_str()));
		}
	}
}


// -----------------------------------------------------------------------------
// Section: BWTracer implementation
// -----------------------------------------------------------------------------

/**
 * Constructor
 * Sets up message handling tables
 */
BWTracer::BWTracer() :
	pFile_(NULL),
	pInterface_(NULL),
	flushMode_(false),
	hexMode_(false),
	filterAddr_( 0 )
{
	memset(&proxyMsg_, 0, sizeof(proxyMsg_));
	memset(&clientMsg_, 0, sizeof(clientMsg_));
	MsgDef* m;

	m = &clientMsg_[ClientInterface::bandwidthNotification.id()];
	m->handler = &BWTracer::clientBandwidthNotification;
	m->minBytes = sizeof(ClientInterface::bandwidthNotificationArgs);

	m = &clientMsg_[ClientInterface::tickSync.id()];
	m->handler = &BWTracer::clientTickSync;
	m->minBytes = sizeof(ClientInterface::tickSyncArgs);

	m = &clientMsg_[ClientInterface::enterAoI.id()];
	m->handler = &BWTracer::clientEnterAoI;
	m->minBytes = sizeof(ClientInterface::enterAoIArgs);

	m = &clientMsg_[ClientInterface::leaveAoI.id()];
	m->handler = &BWTracer::clientLeaveAoI;
	m->minBytes = 2;

	m = &clientMsg_[ClientInterface::createEntity.id()];
	m->handler = &BWTracer::clientCreateEntity;
	m->minBytes = 2;

	clientMsg_[254].handler = &BWTracer::piggybackMessage;
	clientMsg_[254].minBytes = 4;
	clientMsg_[255].handler = &BWTracer::replyMessage;
	clientMsg_[255].minBytes = 4;

	m = &proxyMsg_[BaseAppExtInterface::authenticate.id()];
	m->handler = &BWTracer::proxyAuthenticate;
	m->minBytes = sizeof(BaseAppExtInterface::authenticateArgs);

	m = &proxyMsg_[BaseAppExtInterface::avatarUpdateImplicit.id()];
	m->handler = &BWTracer::proxyAvatarUpdateImplicit;
	m->minBytes = sizeof(BaseAppExtInterface::avatarUpdateImplicitArgs);

	m = &proxyMsg_[BaseAppExtInterface::avatarUpdateExplicit.id()];
	m->handler = &BWTracer::proxyAvatarUpdateExplicit;
	m->minBytes = sizeof(BaseAppExtInterface::avatarUpdateExplicitArgs);

	m = &proxyMsg_[BaseAppExtInterface::requestEntityUpdate.id()];
	m->handler = &BWTracer::proxyRequestEntityUpdate;
	m->minBytes = 2; // sizeof(BaseAppExtInterface::requestEntityUpdateArgs);

	m = &proxyMsg_[BaseAppExtInterface::enableEntities.id()];
	m->handler = &BWTracer::proxyEnableEntities;
	m->minBytes = 0;

	proxyMsg_[254].handler = &BWTracer::piggybackMessage;
	proxyMsg_[254].minBytes = 4;
	proxyMsg_[255].handler = &BWTracer::replyMessage;
	proxyMsg_[255].minBytes = 4;

	for(int i = 128; i < 253; i++)
	{
		proxyMsg_[i].handler = &BWTracer::entityMessage;
		proxyMsg_[i].minBytes = 2;
		clientMsg_[i].handler = &BWTracer::entityMessage;
		clientMsg_[i].minBytes = 2;
	}

}

/**
 * Open logfile and tell mercury to send us message
 */
void BWTracer::startLogging( Mercury::NetworkInterface & interface,
		const char* filename)
{
	MF_ASSERT(pFile_ == NULL);
	MF_ASSERT(pInterface_ == NULL);
	pFile_ = fopen(filename, "w");

	if(!pFile_)
	{
		WARNING_MSG("BWTracer: Failed to open %s\n", filename);
		return;
	}

	pInterface_ = &interface;
	pInterface_->setPacketMonitor( this );
}

/**
 * Close logfile and tell mercury to stop sending us messages
 */
void BWTracer::stopLogging()
{
	if (pFile_)
	{
		fclose( pFile_ );
		pFile_ = NULL;
	}

	if (pInterface_)
	{
		pInterface_->setPacketMonitor( NULL );
		pInterface_ = NULL;
	}
}

#if 0
/**
 * Update message to the client
 */
int BWTracer::clientAvatarUpdate()
{
	ClientInterface::avatarUpdateArgs args;
	memcpy(&args, pData_, sizeof(args));

	fprintf(pFile_, "avatarUpdate: id=%ld pos=(%f, %f, %f) dir=%d\n",
			args.id, args.pos.x, args.pos.y, args.pos.z, args.dir);

	return sizeof(args);
}
#endif



/**
 * Bandwidth to client
 */
int BWTracer::clientBandwidthNotification()
{
	ClientInterface::bandwidthNotificationArgs args;
	memcpy(&args, pData_, sizeof(args));
	fprintf(pFile_, "bandwidthNotification: bps=%u\n", args.bps);
	return sizeof(args);
}

/**
 * Client sequence number
 */
int BWTracer::clientTickSync()
{
	ClientInterface::tickSyncArgs args;
	memcpy(&args, pData_, sizeof(args));
	fprintf(pFile_, "tickSync: seq=%d\n", args.tickByte);
	return sizeof(args);
}

/**
 * An entity has entered the client's AoI
 */
int BWTracer::clientEnterAoI()
{
	ClientInterface::enterAoIArgs args;
	memcpy(&args, pData_, sizeof(args));
	fprintf(pFile_, "enterAoI: id=%d\n", args.id);
	return sizeof(args);
}

/**
 * An entity has left the client's AOI
 */
int BWTracer::clientLeaveAoI()
{
	int len = this->readVarLength( 2 );
	EntityID id;
	memcpy( &id, pData_ + 2, sizeof(id) );
	fprintf(pFile_, "leaveAoI: id=%d\n", id );

	return len + 2;
}

/**
 *	This structure is used by BWTracer::clientCreateEntity to help interpret the
 *	ClientInterface::createEntity message.
 */
struct createEntityMsg
{
	EntityID id;
	EntityTypeID entityTypeId;
	float x, y, z;
};

/**
 * Properties for an entity
 */
int BWTracer::clientCreateEntity()
{
	int len = this->readVarLength(2);
	createEntityMsg args;
	memcpy(&args, pData_+2, sizeof(args));

	fprintf(pFile_, "createEntity: id=%d type=%d pos=(%f, %f, %f) len %d\n",
			args.id, args.entityTypeId, args.x, args.y, args.z, len);
	return len + 2;
}

/**
 * This is sent at the beginning of all bundles to the proxy
 */
int BWTracer::proxyAuthenticate()
{
	BaseAppExtInterface::authenticateArgs args;
	memcpy(&args, pData_, sizeof(args));
	fprintf(pFile_, "proxyAuthenticate: key=%d\n", args.key);
	return sizeof(args);
}

/**
 * The client's position has changed
 */
int BWTracer::proxyAvatarUpdateImplicit()
{
	BaseAppExtInterface::avatarUpdateImplicitArgs args;
	memcpy(&args, pData_, sizeof(args));
	float yaw, pitch, roll;
	args.dir.get( yaw, pitch, roll );
	fprintf( pFile_, "avatarUpdate: pos=(%f, %f, %f) yaw=%f pitch=%f roll=%f\n",
		args.pos.x, args.pos.y, args.pos.z, yaw, pitch, roll );
	return sizeof(args);
}

/**
 * The client's position has changed
 */
int BWTracer::proxyAvatarUpdateExplicit()
{
	BaseAppExtInterface::avatarUpdateExplicitArgs args;
	memcpy(&args, pData_, sizeof(args));
	float yaw, pitch, roll;
	args.dir.get( yaw, pitch, roll );
	fprintf( pFile_, "avatarUpdate: pos=(%f, %f, %f) yaw=%f pitch=%f roll=%f\n",
		args.pos.x, args.pos.y, args.pos.z, yaw, pitch, roll );
	return sizeof(args);
}

/**
 * Request the latest properties for an entity
 */
int BWTracer::proxyRequestEntityUpdate()
{
#if 0
	BaseAppExtInterface::requestEntityUpdateArgs args;
	memcpy(&args, pData_, sizeof(args));
	fprintf(pFile_, "requestEntityUpdate: id=%ld lastUpdate=%ld\n",
			args.id, args.lastUpdate);
	return sizeof(args);
#endif
	int len = this->readVarLength(2);
	EntityID id;
	memcpy( &id, pData_+2, sizeof(id));
	fprintf(pFile_, "requestEntityUpdate: id=%d\n", id );

	return len + 2;
}

/**
 * Tell the cell to start sending us updates
 */
int BWTracer::proxyEnableEntities()
{
	fprintf( pFile_, "enableEntities:\n" );
	return 0;
}

/**
 * A variable length entity message. We don't yet display these.
 */
int BWTracer::entityMessage()
{
	int len = this->readVarLength(2);
	fprintf(pFile_, "entityMessage %02X: len %d\n", msgId_, len);
	return len + 2;
}

/**
 * A variable length reply message. We don't yet display these.
 */
int BWTracer::replyMessage()
{
	int len = this->readVarLength(4);
	fprintf(pFile_, "replyMessage: len %d\n", len);
	return len + 4;
}

/**
 * A variable length reply message. We don't yet display these.
 * However, they are processed recursively. So output may be a little
 * weird.
 */
int BWTracer::piggybackMessage()
{
	int len = this->readVarLength(4);
	fprintf(pFile_, "piggybackMessage: len %d\n", len);
	return len + 4;
}

/**
 * Read a variable length
 */
int32 BWTracer::readVarLength(int bytes)
{
	int32 len = 0;

	for(int i = 0; i < bytes; i++)
	{
		len <<= 8;
		len += *(uint8*)&pData_[i];
	}
	return len;
}


/**
 * Display the header component of the current packet
 * Adjust pData_ and len_ to remove the header (and the footer).
 */
void BWTracer::dumpHeader()
{
	uint8 flags = (uint8)*pData_;
	const char* backPack = pData_ + len_;
	int i;

	fprintf(pFile_, "Flags: ");

	if(!flags)
		fprintf(pFile_, "None");
	if(flags & Mercury::Packet::FLAG_HAS_REQUESTS)
		fprintf(pFile_, "HAS_REQUESTS ");
	if(flags & Mercury::Packet::FLAG_HAS_ACKS)
		fprintf(pFile_, "HAS_ACKS ");
	if(flags & Mercury::Packet::FLAG_ON_CHANNEL)
		fprintf(pFile_, "ON_CHANNEL ");
	if(flags & Mercury::Packet::FLAG_IS_RELIABLE)
		fprintf(pFile_, "IS_RELIABLE ");
	if(flags & Mercury::Packet::FLAG_IS_FRAGMENT)
		fprintf(pFile_, "IS_FRAGMENT ");
	if(flags & Mercury::Packet::FLAG_HAS_SEQUENCE_NUMBER)
		fprintf(pFile_, "HAS_SEQUENCE_NUMBER ");

	fprintf(pFile_, "\n");

	if(flags & Mercury::Packet::FLAG_HAS_ACKS)
	{
		fprintf(pFile_, "Acks: ");
		int numAcks = *(char*)(backPack-=sizeof(char));

		for(i = 0; i < numAcks; i++)
			fprintf(pFile_, "%d ", *(uint32*)(backPack-=sizeof(int32)));
		fprintf(pFile_, "\n");
	}

	if(flags & Mercury::Packet::FLAG_HAS_SEQUENCE_NUMBER)
		fprintf(pFile_, "Seq: %d\n", *(int*)(backPack-=sizeof(int)));

	if(flags & Mercury::Packet::FLAG_HAS_REQUESTS)
	{
		// Remove the request chain pointer
		backPack -= sizeof(uint16);
	}

	pData_++;
	len_ = backPack - pData_;
};


/**
 * Display all messages in the bundle
 */
void BWTracer::dumpMessages()
{
	while(len_)
	{
		msgId_ = *(uint8*)pData_++;
		len_--;

		if(len_ < msgTable_[msgId_].minBytes)
		{
			fprintf(pFile_, "Message %d truncated.\n", msgId_);
			return;
		}

		if(!msgTable_[msgId_].handler)
		{
			fprintf(pFile_, "No handler for message %d\n", msgId_);
			return;
		}

		int consumed = (this->*(msgTable_[msgId_].handler))();
		pData_ += consumed;
		len_ -= consumed;
	}
}

/**
 * Display the entire packet in hex
 */
void BWTracer::hexDump()
{
	int i, j;

	for(i = 0; i < len_; i += 16)
	{
		fprintf(pFile_, "%04X: ", i);

		for(j = 0; j < 16; j++)
		{
			if(i+j < len_)
				fprintf(pFile_, "%02X ", (uint8)pData_[i+j]);
			else
				fprintf(pFile_, "   ");
		}

		fprintf(pFile_, " ");

		for(j = 0; (j < 16) && (i+j < len_); j++)
		{
			if(pData_[i+j] >= ' ' && pData_[i+j] <= '~')
				fprintf(pFile_, "%c", pData_[i+j]);
			else
				fprintf(pFile_, ".");
		}

		fprintf(pFile_, "\n");
	}

	fprintf(pFile_, "\n");
}

/**
 * 	This method dumps the current timestamp in localtime to the logfile.
 */
void BWTracer::dumpTimeStamp()
{
	time_t t;
	struct tm* tm;
	time(&t);
	tm = localtime(&t);
	fprintf(pFile_, "%02d:%02d:%02d: ", tm->tm_hour, tm->tm_min, tm->tm_sec);
}



/**
 * Handle a packet from the client to the proxy
 */
void BWTracer::packetIn(const Mercury::Address& addr, const Mercury::Packet& packet)
{
	if(filterAddr_ && addr.ip != filterAddr_)
		return;

	pData_ = packet.data();
	len_ = packet.totalSize();
	msgTable_ = proxyMsg_;

	this->dumpTimeStamp();
	fprintf(pFile_, "Incoming packet from %s (%d bytes)\n", addr.c_str(), len_);

	if(hexMode_)
		this->hexDump();

	this->dumpHeader();
	this->dumpMessages();
	fprintf(pFile_, "------------------------------------------------------------------------------\n\n");

	if(flushMode_)
		fflush(pFile_);
}

/**
 * Handle a packet from the proxy to the client
 */
void BWTracer::packetOut(const Mercury::Address& addr, const Mercury::Packet& packet)
{
	if(filterAddr_ && addr.ip != filterAddr_)
		return;

	pData_ = packet.data();
	len_ = packet.totalSize();
	msgTable_ = clientMsg_;

	this->dumpTimeStamp();
	fprintf(pFile_, "Outgoing packet to %s (%d bytes)\n", addr.c_str(), len_);

	if(hexMode_)
		this->hexDump();

	this->dumpHeader();
	this->dumpMessages();
	fprintf(pFile_, "------------------------------------------------------------------------------\n\n");

	if(flushMode_)
		fflush(pFile_);
}

BW_END_NAMESPACE

// bwtracer.cpp
