#ifndef CPUINFO_HPP
#define CPUINFO_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

// a small wrapper around GetLogicalProcessorInformation & GetCurrentProcessorNumber so including windows.h isn't required
class CSTDMF_DLL CpuInfo
{
public:
	CpuInfo();
	~CpuInfo();

	int32	numberOfSystemCores();
	int32	numberOfLogicalCores();
	int32	getPhysicalCore(int32 coreID);
	bool	isLogical(int32 coreID);
	static int32 getCurrentProcessorNumber();

private:
	CpuInfo(const CpuInfo&);
	CpuInfo& operator=(const CpuInfo&);

	void	detectCores();

	struct CoreInfo
	{
		int coreID;
		int physicalCore;
		bool isLogical;
	};
	
	CoreInfo* cores_;
	int numCoreInfos_;
	int numSystemCores_;
	int numLogicalCores_;
	static bool supportChecked;
	static void checkCurrentProcessorNumberSupport();
};

BW_END_NAMESPACE

#endif	// CPUINFO_HPP
