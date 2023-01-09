#ifndef REVIVER_SUBJECT_HPP
#define REVIVER_SUBJECT_HPP

#include "reviver_common.hpp"

#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to handle the ping messages sent by the reviver process.
 */
class ReviverSubject : public Mercury::InputMessageHandler
{
public:
	ReviverSubject();
	void init( Mercury::NetworkInterface * pInterface,
				const char * componentName );
	void fini();

	static ReviverSubject & instance() { return instance_; }

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	Mercury::NetworkInterface *		pInterface_;
	Mercury::Address	reviverAddr_;
	uint64				lastPingTime_;
	ReviverPriority		priority_;

	int					msTimeout_;

	static ReviverSubject instance_;
};

#define MF_REVIVER_PING_MSG()	\
		MERCURY_VARIABLE_MESSAGE( reviverPing, 2, &ReviverSubject::instance() )

BW_END_NAMESPACE

#endif // REVIVER_SUBJECT_HPP
