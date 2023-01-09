#ifndef _BWTRACER_HEADER
#define _BWTRACER_HEADER

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class BWTracer;

namespace Mercury
{
class NetworkInterface;
}


/**
 *	This simple class is used to initialise and manage the lifespan of the
 *	bwtracer.
 */
class BWTracerHolder
{
public:
	BWTracerHolder();
	~BWTracerHolder();

	void init( Mercury::NetworkInterface & networkInterface );

private:
	BWTracer * pImpl_;
};

BW_END_NAMESPACE

#endif
