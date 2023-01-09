#include "pch.hpp"

#include "packet_generator.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

namespace Util
{


template< typename T >
unsigned char * setInStream( void * vdata, size_t offset, 
		const T & value )
{
	unsigned char * data = reinterpret_cast< unsigned char * >( vdata );
	T * pValue = reinterpret_cast< T * >( data + offset );
	*pValue = value;
	return data + offset + sizeof( T );
}


void insertInStream( unsigned char * dest, size_t destLen, 
		const void * src, size_t srcLen )
{
	char buf[destLen];
	if (destLen > 0)
	{
		memcpy( buf, dest, destLen );
	}

	memcpy( dest, src, srcLen );

	if (destLen > 0)
	{
		memcpy( dest + srcLen, buf, destLen );
	}
} // namespace Util


template< typename T >
void insertInStream( unsigned char * data, size_t & tailLen, 
		const T & value )
{
	Util::insertInStream( data, tailLen, &value, sizeof( value ) );
	tailLen += sizeof( value );
}


} // end namespace (anonymous)


PacketGenerator::PacketGenerator():
		flags_( 0 ),
		seq_( Mercury::SEQ_NULL ),
		fragBegin_( Mercury::SEQ_NULL ),
		fragEnd_( Mercury::SEQ_NULL ),
		channelID_( Mercury::CHANNEL_ID_NULL ),
		channelVersion_( 0 ),
		messages_()
{

}


PacketGenerator::~PacketGenerator()
{

}


void PacketGenerator::setIndexedChannel( Mercury::ChannelID channelID,
		Mercury::ChannelVersion version )
{
	channelID_ = channelID;

	if (channelID != Mercury::CHANNEL_ID_NULL)
	{
		flags_ |= Mercury::Packet::FLAG_INDEXED_CHANNEL;
		channelVersion_ = version;
	}
	else
	{
		flags_ &= ~Mercury::Packet::FLAG_INDEXED_CHANNEL;
		channelVersion_ = 0;
	}
}


void PacketGenerator::setReliable( Mercury::SeqNum seq, bool isReliable )
{
	seq_ = seq;
	if (seq_ != Mercury::SEQ_NULL)
	{
		flags_ |= Mercury::Packet::FLAG_HAS_SEQUENCE_NUMBER;
	}
	else
	{
		flags_ &= ~Mercury::Packet::FLAG_HAS_SEQUENCE_NUMBER;
	}

	if (isReliable)
	{
		flags_ |= Mercury::Packet::FLAG_IS_RELIABLE;
	}
	else
	{
		flags_ &= ~Mercury::Packet::FLAG_IS_RELIABLE;
	}
}


void PacketGenerator::setFragment( Mercury::SeqNum fragBegin, 
		Mercury::SeqNum fragEnd )
{
	if (fragBegin != Mercury::SEQ_NULL || fragEnd != Mercury::SEQ_NULL)
	{
		flags_ |= Mercury::Packet::FLAG_IS_FRAGMENT;
	}
	else
	{
		flags_ &= ~Mercury::Packet::FLAG_IS_FRAGMENT;
	}

	fragBegin_ = fragBegin;
	fragEnd_ = fragEnd;
}


void PacketGenerator::setOnChannel( bool isCreate )
{
	flags_ |= Mercury::Packet::FLAG_ON_CHANNEL;

	if (isCreate)
	{
		flags_ |= Mercury::Packet::FLAG_CREATE_CHANNEL;
	}
	else
	{
		flags_ &= ~Mercury::Packet::FLAG_CREATE_CHANNEL;
	}
}


void PacketGenerator::addFixedLengthMessage( Mercury::MessageID messageID,
		const void * data, uint len )
{
	this->addMessageData( messageID );
	this->addMessageBlob( data, len );
}


void PacketGenerator::addVariableLengthMessage( Mercury::MessageID messageID, 
		uint8 varLengthLen, const void * data, uint len )
{
	shared_ptr< MemoryOStream > pStream( new MemoryOStream() );

	*pStream << messageID;
	switch (varLengthLen)
	{
		case 1:
		{
			uint8 len8 = uint8( len );
			*pStream << len8;
			break;
		}
		case 2:
		{
			uint16 len16 = uint16( len );
			*pStream << len16;
			break;
		}
		case 4:
		{
			uint32 len32 = uint32( len );
			*pStream << len32;
			break;
		}
		default:
			CRITICAL_MSG( "Invalid variable length length\n" );
			break;
	}

	pStream->addBlob( data, len );

	messages_.push_back( pStream );
}


void PacketGenerator::addMessageBlob( const void * data, uint len )
{
	shared_ptr< MemoryOStream > pStream( new MemoryOStream() );

	pStream->addBlob( data, len );

	messages_.push_back( pStream );
}


Mercury::Packet * PacketGenerator::createPacket() const
{
	static Endpoint s_source;
	static Endpoint s_destination;
	static bool inited = false;

	if (!inited)
	{
		s_source.socket( SOCK_DGRAM );
		s_source.bind();

		s_destination.socket( SOCK_DGRAM );
		s_destination.bind();

		inited = true;
	}

	Mercury::Packet * pPacket = new Mercury::Packet();

	char packetBuffer[PACKET_MAX_SIZE];

	size_t len = PACKET_MAX_SIZE;

	if (!this->addToStream( packetBuffer, len ))
	{
		return NULL;
	}

	Mercury::Address destAddr = s_destination.getLocalAddress();
	Mercury::Address sourceAddr = s_source.getLocalAddress();

	s_source.sendto( packetBuffer, len, destAddr.port, destAddr.ip );

	pPacket->recvFromEndpoint( s_destination, sourceAddr );

	return pPacket;
}


bool PacketGenerator::addToStream( void * vdata, size_t & maxLen ) const
{
	uint8 * data = reinterpret_cast< uint8 *>( vdata );
	uint8 * origData = data;

	if (maxLen < sizeof( flags_ ))
	{
		return false;
	}

	data = Util::setInStream( data, 0, flags_ );


	for (Messages::const_iterator iMessage = messages_.begin();
			iMessage != messages_.end();
			++iMessage)
	{
		size_t len = (*iMessage)->remainingLength();
		if (maxLen - (data - origData) < len)
		{
			return false;
		}

		// Need to const_cast because data() is non-const.
		MemoryOStream & messageStream = 
			const_cast< MemoryOStream & >( **iMessage );

		memcpy( data, messageStream.data(), len );
		data += len;
	}

	size_t footerLen = 0;

	if (flags_ & Mercury::Packet::FLAG_INDEXED_CHANNEL)
	{
		if (maxLen - (data - origData + footerLen) < 
				sizeof( channelID_ ) + sizeof( channelVersion_ ))
		{
			return false;
		}

		Util::insertInStream( data, footerLen, channelID_ );
		Util::insertInStream( data, footerLen, channelVersion_ );
	}

	if (flags_ & Mercury::Packet::FLAG_HAS_SEQUENCE_NUMBER)
	{
		if (maxLen - (data - origData + footerLen) < sizeof( seq_ ))
		{
			return false;
		}

		Util::insertInStream( data, footerLen, seq_ );
	}

	if (flags_ & Mercury::Packet::FLAG_IS_FRAGMENT)
	{
		if (maxLen - (data - origData + footerLen) < 
				sizeof( fragBegin_ ) + sizeof( fragEnd_ ))
		{
			return false;
		}
		Util::insertInStream( data, footerLen, fragEnd_ );
		Util::insertInStream( data, footerLen, fragBegin_ );
	}

	maxLen = data - origData + footerLen;

	return true;
}

BW_END_NAMESPACE

// packet_generator.cpp

