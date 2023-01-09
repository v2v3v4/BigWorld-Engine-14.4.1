#ifndef ONCE_OFF_PACKET_HPP
#define ONCE_OFF_PACKET_HPP

#include "basictypes.hpp"
#include "fragmented_bundle.hpp"
#include "misc.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class EventDispatcher;
class OnceOffSender;
class PacketSender;

// -----------------------------------------------------------------------------
// Section: OnceOffReceipt
// -----------------------------------------------------------------------------

/**
 *	This struct is used to store details of once-off packets that we have
 *	received.
 */
class OnceOffReceipt
{
public:
	OnceOffReceipt( const Address & addr, SeqNum footerSequence ) :
		addr_( addr ),
		footerSequence_( footerSequence )
	{
	}

	// Overloaded operators declared outside the class:
	//bool operator==( const OnceOffReceipt & oor1, 
	//	const OnceOffReceipt & oor2 );
	//bool operator<( const OnceOffReceipt & oor1, 
	//	const OnceOffReceipt & oor2 );

	Address addr_;
	SeqNum footerSequence_;
};


/**
 *	Equality operator for OnceOffReceipt instances.
 */
inline bool operator==( const OnceOffReceipt & oor1, 
		const OnceOffReceipt & oor2 )
{
	return (oor1.footerSequence_ == oor2.footerSequence_) && 
		(oor1.addr_ == oor2.addr_);
}


/**
 *	Comparison operator for OnceOffReceipt instances.
 */
inline bool operator<( const OnceOffReceipt & oor1,
		const OnceOffReceipt & oor2 )
{
	return (oor1.footerSequence_ < oor2.footerSequence_) ||
		((oor1.footerSequence_ == oor2.footerSequence_) && 
			(oor1.addr_ < oor2.addr_));
}

typedef BW::set< OnceOffReceipt > OnceOffReceipts;


// -----------------------------------------------------------------------------
// Section: OnceOffReceiver
// -----------------------------------------------------------------------------

class OnceOffReceiver : public TimerHandler
{
public:
	OnceOffReceiver();
	~OnceOffReceiver();

	void init( EventDispatcher & dispatcher );
	void fini();

	bool onReliableReceived( EventDispatcher & dispatcher,
			const Address & addr, SeqNum seq );

	void onceOffReliableCleanup();

	// TODO: Remove this
	FragmentedBundles & fragmentedBundles() { return fragmentedBundles_; }

private:
	virtual void handleTimeout( TimerHandle handle, void * arg );

	void initOnceOffPacketCleaning( EventDispatcher & dispatcher );

	OnceOffReceipts currOnceOffReceipts_;
	OnceOffReceipts prevOnceOffReceipts_;

	FragmentedBundles fragmentedBundles_;
	// TODO: eventually allocate FragmentedBundleInfo's from
	// a rotating list; when it gets full drop old fragments.

	TimerHandle clearFragmentedBundlesTimerHandle_;
	TimerHandle onceOffPacketCleaningTimerHandle_;
};

// -----------------------------------------------------------------------------
// Section: OnceOffPacket
// -----------------------------------------------------------------------------

/**
 *
 */
class OnceOffPacket : public TimerHandler, public OnceOffReceipt
{
public:
	OnceOffPacket( const Address & addr, SeqNum footerSequence,
					OnceOffSender & onceOffSender,
					Packet * pPacket = NULL );

	void registerTimer( EventDispatcher & dispatcher,
			PacketSender & packetSender,
			int period );

	virtual void handleTimeout( TimerHandle handle, void * arg );

	PacketPtr pPacket_;
	OnceOffSender & onceOffSender_;
	TimerHandle timerHandle_;
	int retries_;
};
typedef BW::set< OnceOffPacket > OnceOffPackets;


// -----------------------------------------------------------------------------
// Section: OnceOffSender
// -----------------------------------------------------------------------------

class OnceOffSender
{
public:
	OnceOffSender();
	~OnceOffSender();

	void addOnceOffResendTimer( const Address & addr, SeqNum seq,
			Packet * p,
			PacketSender & packetSender,
			EventDispatcher & dispatcher );

	void delOnceOffResendTimer( const Address & addr, SeqNum seq );
	void delOnceOffResendTimer( OnceOffPackets::iterator & iter );

	void expireOnceOffResendTimer( OnceOffPacket & packet,
									PacketSender & packetSender );

	void onAddressDead( const Address & addr );

	int onceOffResendPeriod() const
	{	return onceOffResendPeriod_; }

	void onceOffResendPeriod( int microseconds )
	{	onceOffResendPeriod_ = microseconds; }

	int onceOffMaxResends() const
	{	return onceOffMaxResends_; }

	void onceOffMaxResends( int retries )
	{	onceOffMaxResends_ = retries; }

	bool hasUnackedPackets() const { return !onceOffPackets_.empty(); }

private:
	OnceOffPackets onceOffPackets_;

	int onceOffMaxResends_;
	int onceOffResendPeriod_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // ONCE_OFF_PACKET_HPP
