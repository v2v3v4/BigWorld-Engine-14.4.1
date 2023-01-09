#ifndef MACHINED_UTIL_HPP
#define MACHINED_UTIL_HPP

#include "endpoint.hpp"
#include "machine_guard.hpp"
#include "misc.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPBundle;

namespace MachineDaemon
{
	/**
	 * Abstract class for finding an interface from ProcessStatsMessages.
	 */
	class IFindInterfaceHandler : public MachineGuardMessage::ReplyHandler
	{
	public:
		IFindInterfaceHandler();

		/**
		 *	This method returns the address of the found interface.
		 */
		virtual Address result() const = 0;

		/**
		 *	This method returns true if the next ProcessStatsMessages response
		 *	should be processed.
		 */
		virtual bool onProcessMatched( Address & addr,
				const ProcessStatsMessage & psm ) = 0;

		bool hasError() { return hasError_; }

	protected:
		bool onProcessStatsMessage(
				ProcessStatsMessage & psm, uint32 ip ); /* override */

	private:
		bool hasError_;

	};

	Reason registerWithMachined( const Address & srcAddr,
			const BW::string & name, int id, bool isRegister = true );

	Reason deregisterWithMachined( const Address & srcAddr,
			const BW::string & name, int id );

	Reason registerBirthListener( const Address & srcAddr,
			UDPBundle & bundle, int addrStart, const char * ifname );

	Reason registerDeathListener( const Address & srcAddr,
			UDPBundle & bundle, int addrStart, const char * ifname );

	Reason registerBirthListener( const Address & srcAddr,
			const InterfaceElement & ie, const char * ifname );

	Reason registerDeathListener( const Address & srcAddr,
			const InterfaceElement & ie, const char * ifname );

	Reason findInterface( const char * name, int id, Address & addr,
			int retries = 0, bool verboseRetry = true,
			IFindInterfaceHandler * pHandler = NULL );

	bool queryForInternalInterface( u_int32_t & addr );

	bool sendSignalViaMachined( const Mercury::Address & dest, int sigNum );

} // namespace MachineDaemon

} // namespace Mercury

BW_END_NAMESPACE

#endif // MACHINED_UTIL_HPP
