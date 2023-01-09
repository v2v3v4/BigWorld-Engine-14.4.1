#ifndef BW_STREAM_DATA_HANDLER
#define BW_STREAM_DATA_HANDLER

#include "network/basictypes.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the interface for stream data handlers
 */
class BWStreamDataHandler
{
public:
	virtual ~BWStreamDataHandler() {}

	/**
	 * This method receives a notification when data has been streamed
	 * from the server in unutilised bandwidth.
	 */
	virtual void onStreamDataComplete( uint16 streamID,
		const BW::string & rDescription, BinaryIStream & rData ) = 0;
};

BW_END_NAMESPACE

#endif // BW_STREAM_DATA_HANDLER
