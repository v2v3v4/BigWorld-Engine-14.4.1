#ifndef COMPONENT_REVIVER_HPP
#define COMPONENT_REVIVER_HPP

#include "cstdmf/intrusive_object.hpp"

#include "network/event_dispatcher.hpp"
#include "network/interfaces.hpp"
#include "network/network_interface.hpp"

#include "server/reviver_common.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ComponentReviver
// -----------------------------------------------------------------------------

/**
 *	This class is used to monitor and revive a single, server component.
 */
class ComponentReviver : public Mercury::ShutdownSafeReplyMessageHandler,
	public TimerHandler,
	public Mercury::InputMessageHandler,
	public IntrusiveObject< ComponentReviver >
{
public:
	ComponentReviver( const char * configName, const char * name,
			const char * interfaceName, const char * createParam );
	~ComponentReviver();

	bool init( Mercury::EventDispatcher & dispatcher,
			Mercury::NetworkInterface & interface );

	void revive();

	bool activate( ReviverPriority priority );
	bool deactivate();

	// Handles the death message
	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	// Reply handler
	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );

	virtual void handleTimeout( TimerHandle /*handle*/, void * /*arg*/ );

	virtual void handleException( const Mercury::NubException & ne,
			void * arg );

	ReviverPriority priority() const	{ return priority_; }
	void priority( ReviverPriority p )	{ priority_ = p; }

	bool isAttached() const				{ return isAttached_; }
	const BW::string & name() const	{ return name_; }
	const Mercury::Address & addr() const	{ return addr_; }

	bool isEnabled() const				{ return isEnabled_; }
	void isEnabled( bool v )			{ isEnabled_ = v; }

	const BW::string & configName() const		{ return configName_; }
	const char * createName() const				{ return createParam_; }

	int maxPingsToMiss() const					{ return maxPingsToMiss_; }
	int pingPeriod() const						{ return pingPeriod_; }

protected:
	virtual void initInterfaceElements() = 0;

	const Mercury::InterfaceElement * pBirthMessage_;
	const Mercury::InterfaceElement * pDeathMessage_;
	const Mercury::InterfaceElement * pPingMessage_;

private:
	Mercury::EventDispatcher * pDispatcher_;
	Mercury::NetworkInterface * pInterface_;
	Mercury::Address addr_;

	BW::string configName_;
	BW::string name_;
	BW::string interfaceName_;
	const char * createParam_;

	ReviverPriority priority_;

	TimerHandle timerHandle_;
	int pingsToMiss_;
	int maxPingsToMiss_;
	int pingPeriod_;

	// Indicates that we are active and have received a positive response.
	bool isAttached_;

	bool isEnabled_;
};

BW_END_NAMESPACE

#endif // COMPONENT_REVIVER_HPP
