#ifndef STREAM_FILTER_FACTORY_HPP
#define STREAM_FILTER_FACTORY_HPP

#include "stream_filter.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{


/**
 *	This class is the interface for all stream filter factories. These objects
 *	are used to create and attach a stream filter when a TCPChannel has been
 *	created.
 */
class StreamFilterFactory
{
public:
	/**
	 *	Destructor.
 	 */
	virtual ~StreamFilterFactory() {}

	virtual StreamFilterPtr createFor( TCPChannel & channel ) = 0;

protected:
	StreamFilterFactory()
	{}

};

} // end namespace Mercury

BW_END_NAMESPACE

#endif // STREAM_FILTER_FACTORY_HPP
