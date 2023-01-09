#ifndef __MOO_INIT_FINI_HPP__
#define __MOO_INIT_FINI_HPP__

BW_BEGIN_NAMESPACE

namespace Moo
{
	bool init( bool d3dExInterface = true, bool assetProcessingOnly = false );
	void fini();
	bool isInitialised();
} // namespace Moo

BW_END_NAMESPACE

#endif 
