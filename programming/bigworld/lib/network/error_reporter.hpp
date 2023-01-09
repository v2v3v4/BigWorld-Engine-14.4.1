#ifndef ERROR_REPORTER_HPP
#define ERROR_REPORTER_HPP

#include "basictypes.hpp"
#include "interfaces.hpp"

#include "cstdmf/bw_map.hpp"
#include <utility> // For std::pair

BW_BEGIN_NAMESPACE

namespace Mercury
{

class EventDispatcher;

/**
 *  Accounting structure for keeping track of the number of exceptions reported
 *  in a given period.
 */
struct ErrorReportAndCount
{
	ErrorReportAndCount() :
		lastReportStamps( 0 ),
		lastRaisedStamps( 0 ),
		count( 0 )
	{ }
	uint64 lastReportStamps;	//< When this error was last reported
	uint64 lastRaisedStamps;	//< When this error was last raised
	uint count;					//< How many of this exception have been
								//	reported since
};


/**
 *	Key type for ErrorsAndCounts.
 */

typedef std::pair< Address, Reason > AddressAndReason;

/**
 *	Accounting structure that keeps track of counts of Mercury exceptions
 *	in a given period per pair of address and error message.
 *
 */
typedef BW::map< AddressAndReason, ErrorReportAndCount >
	ErrorsAndCounts;


/**
 *	This class is responsible for handling error messages that may be delayed
 *	and aggregated.
 */
class ErrorReporter : public TimerHandler
{
public:
	ErrorReporter( EventDispatcher & dispatcher );
	~ErrorReporter();

	void reportException( Reason reason, const Address & addr = Address::NONE );
	void reportPendingExceptions( bool reportBelowThreshold = false );

private:
	void addReport( const Address & address, Reason error );

	virtual void handleTimeout( TimerHandle handle, void * arg );

	TimerHandle reportLimitTimerHandle_;
	ErrorsAndCounts errorsAndCounts_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // ERROR_REPORTER_HPP
