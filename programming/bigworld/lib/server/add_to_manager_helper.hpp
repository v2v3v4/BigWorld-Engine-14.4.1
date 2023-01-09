#ifndef ADD_TO_MANAGER_HELPER
#define ADD_TO_MANAGER_HELPER

#include "network/event_dispatcher.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class is used by CellApps and BaseApps to add themselves to their
 *  managers.  It handles the replies from the manager process and handles
 *  sending and resending the add message until it succeeds.
 *
 *  This object deletes itself.
 */
class AddToManagerHelper :
	public Mercury::ShutdownSafeReplyMessageHandler,
	public TimerHandler
{
public:
	AddToManagerHelper( Mercury::EventDispatcher & dispatcher );
	virtual ~AddToManagerHelper();

protected:
	/**
	 *  This method is called when communication to the requested Manager
	 *  fails.
	 */
	virtual void handleFatalTimeout() { }


	/**
	 *  This method is called when a non-empty reply is received from the
	 *  manager.  The data on the stream should be the *AppInitData struct for
	 *  this app pair.
	 */
	virtual bool finishInit( BinaryIStream & data ) = 0;


	/**
	 *  This method is called to initiate communication with the Manager. It is
	 *  responsible for managing the fatal timeout timer as well.
	 */
	void send();


	/**
	 *  Derived classes must implement this method to send the add message to
	 *  the manager.
	 */
	virtual void doSend() = 0;

	Mercury::EventDispatcher & dispatcher_;

private:
	void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );


	void handleException( const Mercury::NubException & exception, void * arg );
	void handleTimeout( TimerHandle handle, void * arg );

	enum AddToManagerTimeouts
	{
		TIMEOUT_RESEND,
		TIMEOUT_FATAL
	};

	TimerHandle resendTimerHandle_;
	TimerHandle fatalTimerHandle_;
};

BW_END_NAMESPACE


#endif // ADD_TO_MANAGER_HELPER
