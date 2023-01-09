#ifndef MESSAGE_WITH_DESTINATION_HPP
#define MESSAGE_WITH_DESTINATION_HPP

#include "network/machine_guard.hpp"
#include "network/endpoint.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class provides a "mix-in" for MachineGuardMessage sub-classes
 *  to allow them to be held and sent later in time, when the intended
 *  destination can no longer be identified.
 *  The resulting class can be otherwise treated as the parent class for
 *  all intents and purposes, and will behave no differently.
 */
// MGM should be a MachineGuardMessage
template< class MGM >
class MessageWithDestination : public MGM
{
public:
	/**
	 *	Destructor.
	 */
	virtual ~MessageWithDestination()
	{
		ep_.detach();
	}

	/**
	 *	This method sets the values to be fed into MGM->sendto() later.
	 */
	void target( const Endpoint & ep, const uint16 port,
		const uint32 addr = BROADCAST, const uint8 packFlags = 0 )
	{
		// XXX: Copying the Endpoint. We never expose this again, and if
		// it's shut down later, we should simply get an error on sendto()
		ep_ = ep;
		port_ = port;
		addr_ = addr;
		packFlags_ = packFlags;
	}


	/**
	 *	This method calls MGM->sendto() with the previously-set values.
	 *
	 *	@return true on success, false otherwise.
	 */
	bool sendToTarget()
	{
		return this->MGM::sendto( ep_, port_, addr_, packFlags_ );
	}

private:
	Endpoint ep_;
	uint16 port_;
	uint32 addr_;
	uint8 packFlags_;
};

// Convenience typedefs
typedef MessageWithDestination<HighPrecisionMachineMessage> HighPrecisionMachineMessageWithDestination;
typedef MessageWithDestination<WholeMachineMessage> WholeMachineMessageWithDestination;
typedef MessageWithDestination<ProcessMessage> ProcessMessageWithDestination;
typedef MessageWithDestination<ProcessStatsMessage> ProcessStatsMessageWithDestination;
typedef MessageWithDestination<ListenerMessage> ListenerMessageWithDestination;
typedef MessageWithDestination<CreateMessage> CreateMessageWithDestination;
typedef MessageWithDestination<SignalMessage> SignalMessageWithDestination;
typedef MessageWithDestination<TagsMessage> TagsMessageWithDestination;
typedef MessageWithDestination<UserMessage> UserMessageWithDestination;
typedef MessageWithDestination<PidMessage> PidMessageWithDestination;
typedef MessageWithDestination<ResetMessage> ResetMessageWithDestination;
typedef MessageWithDestination<ErrorMessage> ErrorMessageWithDestination;
typedef MessageWithDestination<MachinedAnnounceMessage> MachinedAnnounceMessageWithDestination;
typedef MessageWithDestination<QueryInterfaceMessage> QueryInterfaceMessageWithDestination;
typedef MessageWithDestination<CreateWithArgsMessage> CreateWithArgsMessageWithDestination;

BW_END_NAMESPACE

#endif // MESSAGE_WITH_DESTINATION_HPP
