#ifndef __BW_GPU_INFO__
#define __BW_GPU_INFO__

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
namespace Private
{
	class GpuInfoImpl;
}

class GpuInfo
{
public:
	struct MemInfo
	{
		MemInfo() :
			systemMemReserved_( 0 ),
			systemMemUsed_( 0 ),
			dedicatedMemTotal_( 0 ),
			dedicatedMemCommitted_( 0 ),
			sharedMemTotal_( 0 ),
			sharedMemCommitted_( 0 ),
			virtualAddressSpaceTotal_( 0 ),
			virtualAddressSpaceUsage_( 0 ),
			privateUsage_( 0 )
		{}

		uint64 systemMemReserved_;
		uint64 systemMemUsed_;
		uint64 dedicatedMemTotal_;
		uint64 dedicatedMemCommitted_;
		uint64 sharedMemTotal_;
		uint64 sharedMemCommitted_;
		uint64 virtualAddressSpaceTotal_;
		uint64 virtualAddressSpaceUsage_;
		uint64 privateUsage_;
	};

public:
	GpuInfo();
	~GpuInfo();

	bool getMemInfo(MemInfo* outMemInfo, uint32 adapterID) const;

private:
	Private::GpuInfoImpl* pimpl_;

	// noncopyable
	void operator=(const GpuInfo&);
};

}
BW_END_NAMESPACE

#endif // BW_GPU_INFO_