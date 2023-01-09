#ifndef PACKET_GENERATOR_HPP
#define PACKET_GENERATOR_HPP

#include "cstdmf/shared_ptr.hpp"

#include "network/endpoint.hpp"
#include "network/packet.hpp"


BW_BEGIN_NAMESPACE

class Endpoint;
class MemoryOStream;

/**
 *	Class to generate Mercury packets, as if they had just been read from a
 *	socket.
 */
class PacketGenerator
{
public:
	PacketGenerator();
	~PacketGenerator();

	void setIndexedChannel( Mercury::ChannelID channelID, 
		Mercury::ChannelVersion version );
	void setReliable( Mercury::SeqNum seq, bool isReliable = true );
	void setFragment( Mercury::SeqNum fragBegin, Mercury::SeqNum fragEnd );

	void setOnChannel( bool isCreate = false );

	// TODO:
	// void setCumulativeAck( Mercury::SeqNum seq );
	// void addAck( Mercury::SeqNum seq );
	// void addRequest( ... );

	void addFixedLengthMessage( Mercury::MessageID messageID, 
		const void * data, uint len );

	void addVariableLengthMessage( Mercury::MessageID messageID, 
		uint8 varLengthLen, const void * data, uint len );

	template< typename T >
	void addMessageData( const T & value )
		{ this->addMessageBlob( &value, sizeof( value ) ); }

	void addMessageBlob( const void * data, uint len );

	Mercury::Packet * createPacket() const;

	Mercury::Packet::Flags flags() const 
		{ return flags_; }

	void clearMessageData()
		{ messages_.clear(); }
private:

	bool addToStream( void * data, size_t & maxLen ) const;

	Mercury::Packet::Flags flags_;

	Mercury::SeqNum seq_;

	Mercury::SeqNum fragBegin_;
	Mercury::SeqNum fragEnd_;

	Mercury::ChannelID channelID_;
	Mercury::ChannelVersion channelVersion_;

	typedef BW::vector< shared_ptr< MemoryOStream > > Messages;
	Messages messages_;

	static Endpoint s_source_;
	static Endpoint s_destination_;
};

BW_END_NAMESPACE

#endif // PACKET_GENERATOR_HPP

