#ifndef UDP_BUNDLE_PROCESSOR_HPP
#define UDP_BUNDLE_PROCESSOR_HPP

#include "basictypes.hpp"
#include "misc.hpp"
#include "packet.hpp"
#include "unpacked_message_header.hpp"
#include "interface_element.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

class UDPChannel;
class InterfaceTable;
class NetworkInterface;
class Packet;
class ProcessSocketStatsHelper;


/**
 *	This class is used to process a sequence of received packets that represent
 *	a Bundle of messages.
 */
class UDPBundleProcessor
{
public:
	UDPBundleProcessor( Packet * packetChain, bool isEarly = false );

	Reason dispatchMessages( InterfaceTable & interfaceTable,
			const Address & addr, UDPChannel * pChannel,
			NetworkInterface & networkInterface,
			ProcessSocketStatsHelper * pStatsHelper ) const;

	void dumpMessages( const InterfaceTable & interfaceTable ) const;

	/**
	 *	@internal
	 *	This class is used to iterate over the messages in a bundle.
	 *	Mercury uses this internally when unpacking a bundle and
	 *	delivering messages to the client.
	 */
	class iterator
	{
	public:
		iterator(Packet * first);
		iterator(const iterator & i);
		~iterator();

		const iterator & operator=( iterator i );

		MessageID msgID() const;

		// Note: You have to unpack before you can call
		// 'data' or 'operator++'

		UnpackedMessageHeader & unpack( const InterfaceElement & ie );
		const char * data();

		bool isUnpacked() const { return isUnpacked_; }

		void operator++(int);
		bool operator==(const iterator & x) const;
		bool operator!=(const iterator & x) const;

		friend void swap( iterator & a, iterator & b );

	private:
		void nextPacket();

		Packet *	cursor_;
		bool		isUnpacked_;
		uint16		bodyEndOffset_;
		uint16		offset_;
		uint16		dataOffset_;
		int			dataLength_;
		char *		dataBuffer_;

		uint16	nextRequestOffset_;

		UnpackedMessageHeader	curHeader_;
		InterfaceElement updatedIE_;
	};

	/**
	 * Get some iterators
	 */
	iterator begin() const;
	iterator end() const;

private:
	PacketPtr pFirstPacket_;		///< The first packet in the bundle
	const bool isEarly_;
};

} // namespace Mercury


BW_END_NAMESPACE


#endif // UDP_BUNDLE_PROCESSOR_HPP
