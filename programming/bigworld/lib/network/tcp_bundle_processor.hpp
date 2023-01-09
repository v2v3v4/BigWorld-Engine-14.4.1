#ifndef TCP_BUNDLE_PROCESSOR
#define TCP_BUNDLE_PROCESSOR


#include "network/misc.hpp"
#include "network/tcp_bundle.hpp"


BW_BEGIN_NAMESPACE


class BinaryIStream;

namespace Mercury
{

class InterfaceTable;
class TCPChannel;


/**
 *	This class processes messages on a TCPBundle.
 */
class TCPBundleProcessor
{
public:
	TCPBundleProcessor( BinaryIStream & data );

	Reason dispatchMessages( TCPChannel & channel, 
		InterfaceTable & interfaceTable );

private:
	BinaryIStream & data_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // TCP_BUNDLE_PROCESSOR
