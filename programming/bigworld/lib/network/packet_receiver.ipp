#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

#include "network_interface.hpp"


namespace Mercury
{


/**
 *	This method returns whether we are at normal verbosity level.
 */
INLINE bool PacketReceiver::isVerbose() const
{
	return networkInterface_.isVerbose();
}


/**
 *	This method returns whether we are at debug verbosity level.
 */
INLINE bool PacketReceiver::isDebugVerbose() const
{
	return networkInterface_.isDebugVerbose();
}


} // end namespace Mercury


// packet_receiver.ipp
