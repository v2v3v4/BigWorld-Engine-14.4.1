#include "pch.hpp"

#include "tcp_bundle_processor.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/profiler.hpp"

#include "network/interface_element.hpp"
#include "network/interface_table.hpp"
#include "network/message_filter.hpp"
#include "network/misc.hpp"
#include "network/packet.hpp"
#include "network/tcp_channel.hpp"
#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{


/**
 *	Constructor.
 *
 *	@param data 	The bundle data.
 */
TCPBundleProcessor::TCPBundleProcessor( BinaryIStream & data ) : 
		data_( data )
{
}


/**
 *	This method dispatches messages as read from the bundle data.
 *
 *	@param channel 			The TCPChannel the bundle data was received on.
 *	@param interfaceTable 	The interface table.
 *
 *	@return 				REASON_SUCCESS on success, otherwise the
 *							appropriate error code.
 */
Reason TCPBundleProcessor::dispatchMessages( TCPChannel & channel, 
		InterfaceTable & interfaceTable )
{
	ChannelPtr pChannel = &channel;

	const char * bundleStart = 
		reinterpret_cast< const char * >( data_.retrieve( 0 ) );

	const int bundleLength = data_.remainingLength();

	uint offset = 0;
	UnpackedMessageHeader header;
	bool shouldBreakLoop = false;
	header.pBreakLoop = &shouldBreakLoop;
	TCPBundle::Offset nextRequestOffset = static_cast< TCPBundle::Offset >(-1);
	MessageFilterPtr pMessageFilter = channel.pMessageFilter();

	TCPBundle::Flags flags;
	data_ >> flags;

	if (flags & ~TCPBundle::FLAG_MASK)
	{
		ERROR_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
				"Discarding bundle after invalid flags: %02x\n",
			channel.c_str(), flags );
		return REASON_CORRUPTED_PACKET;
	}

	if (flags & TCPBundle::FLAG_HAS_REQUESTS)
	{
		data_ >> nextRequestOffset;

		if (static_cast< int >(nextRequestOffset) > bundleLength)
		{
			ERROR_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
					"Discarding bundle after out-of-bounds first request "
					"offset (got offset %u > %d bundle length)\n",
				channel.c_str(),
				nextRequestOffset,
				data_.remainingLength() );
			return REASON_CORRUPTED_PACKET;
		}
	}

	while (data_.remainingLength() && !shouldBreakLoop)
	{
		offset = static_cast< uint >(
			reinterpret_cast< const char * >( data_.retrieve( 0 ) ) - 
				bundleStart );
		void * pHeaderData = const_cast< void * >( data_.retrieve( 0 ) );

		MessageID messageID;
		data_ >> messageID;

		InterfaceElementWithStats & ie = interfaceTable[ messageID ];

		if (ie.pHandler() == NULL)
		{

			ERROR_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
				"Discarding bundle after hitting unhandled message ID %hu\n",
				channel.c_str(), (unsigned short)messageID );
			data_.finish();
			return REASON_NONEXISTENT_ENTRY;
		}

		header.identifier = messageID;
		header.pInterfaceElement = &ie;
		header.pChannel = &channel;
		header.pInterface = &(channel.networkInterface());

		bool isRequest = (offset == nextRequestOffset);

		InterfaceElement updatedIE = ie;
		if (!updatedIE.updateLengthDetails( pChannel->networkInterface(),
				pChannel->addr() ))
		{
			ERROR_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
					"Discarding bundle after failure to update length "
					"details for message ID %hu\n",
				channel.c_str(), (unsigned short)messageID );
			return REASON_CORRUPTED_PACKET;
		}

		header.length = updatedIE.expandLength( pHeaderData, NULL, isRequest );

		if (updatedIE.lengthStyle() == VARIABLE_LENGTH_MESSAGE)
		{
			data_.retrieve( updatedIE.lengthParam() );

			if (header.length == -1)
			{
				uint32 specialLengthHeader;
				data_ >> specialLengthHeader;
				header.length = specialLengthHeader;

			}
		}

		if (data_.remainingLength() < header.length)
		{
			ERROR_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
					"Discarding rest of bundle since chain too short for data "
					"of message ID %hu length %d (have %d bytes)\n",
				channel.addr().c_str(), (unsigned short)messageID, header.length,
				data_.remainingLength() );

			return REASON_CORRUPTED_PACKET;
		}

		if (isRequest)
		{
			data_ >> header.replyID;
			data_ >> nextRequestOffset;

			if (static_cast< int >(nextRequestOffset) > bundleLength)
			{
				ERROR_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
						"Discarding rest of bundle due to out-of-bounds "
						"next request offset "
						"(got offset %u > %d byte bundle length)\n",
					channel.addr().c_str(), nextRequestOffset, bundleLength );

				return REASON_CORRUPTED_PACKET;
			}
		}
		else
		{
			header.replyID = REPLY_ID_NONE;
		}

		ie.startProfile();

		MemoryIStream mis( data_.retrieve( header.length ), header.length );

		{
			PROFILER_SCOPED_DYNAMIC_STRING( ie.c_str() );
			if (pMessageFilter)
			{
				pMessageFilter->filterMessage( channel.addr(), header, mis, 
					ie.pHandler() );
			}
			else
			{
				ie.pHandler()->handleMessage( channel.addr(), header, mis );
			}
		}

		ie.stopProfile( header.length );

		if (mis.remainingLength() != 0)
		{
			if (header.identifier == REPLY_MESSAGE_IDENTIFIER)
			{
				WARNING_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
						"Handler for reply left %d bytes\n",
					channel.c_str(), mis.remainingLength() );

			}
			else
			{
				WARNING_MSG( "TCPBundleProcessor::dispatchMessages( %s ): "
						"Handler for message %s (ID %hu) left %d bytes\n",
					channel.c_str(), ie.name(),
					(unsigned short)header.identifier, mis.remainingLength() );
			}
			mis.finish();
		}
	}

	if (shouldBreakLoop)
	{
		data_.finish();
	}

	return REASON_SUCCESS;
}


} // end namespace Mercury


BW_END_NAMESPACE

// tcp_bundle_processor.cpp

