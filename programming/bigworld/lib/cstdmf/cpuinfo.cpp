#include "pch.hpp"
#include "cpuinfo.hpp"
#include "debug.hpp"
#include "guard.hpp"

BW_BEGIN_NAMESPACE

bool CpuInfo::supportChecked = false;

CpuInfo::CpuInfo()
: cores_(0),
	numCoreInfos_(0),
	numSystemCores_(0),
	numLogicalCores_(0)
{
	detectCores();
}

CpuInfo::~CpuInfo()
{
	bw_safe_delete_array( cores_ );
	numCoreInfos_ = 0;
}

#if defined(_WINDOWS_)

// Windows implementation
typedef DWORD (WINAPI *GetCurrentProcessorNumberFunc)();	// typedef for GetCurrentProcessorNumber()
GetCurrentProcessorNumberFunc getCurrentProcessorFunc = NULL;

typedef BOOL (WINAPI *GetLogicalProcessorInformationFunc)(
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
	DWORD i;

	for (i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest)?1:0);
		bitTest/=2;
	}

	return bitSetCount;
}

void CpuInfo::detectCores()
{
	GetLogicalProcessorInformationFunc getLogicalProcessorInformationFunc =
		(GetLogicalProcessorInformationFunc) GetProcAddress(
			GetModuleHandle(TEXT("kernel32")),
			"GetLogicalProcessorInformation" );
	if (!getLogicalProcessorInformationFunc)
	{
		ERROR_MSG( "GetLogicalProcessorInformation is not supported\n" );
		numCoreInfos_ = 1;
		numSystemCores_ = 1;
		numLogicalCores_ = 1;
		return;
	}

	bool done = false;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	DWORD returnLength = 0;
	DWORD byteOffset = 0;

	while (!done)
	{
		DWORD rc = getLogicalProcessorInformationFunc( buffer, &returnLength );

		if (FALSE == rc) 
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
			{
				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)alloca(
					returnLength);

				if (buffer == NULL) 
				{
					return;
				}
			} 
			else 
			{
				return;
			}
		} 
		else
		{
			done = TRUE;
		}
	}

	ptr = buffer;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) 
	{
		switch (ptr->Relationship) 
		{
		case RelationNumaNode:
			// Non-NUMA systems report a single record of this type.
			break;

		case RelationProcessorCore:
			numSystemCores_++;

			// A hyper-threaded core supplies more than one logical processor.
			numLogicalCores_ += CountSetBits(ptr->ProcessorMask);
			break;

		case RelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			break;

		case RelationProcessorPackage:
			// Logical processors share a physical package.
			break;

		default:
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	// set up info structures
	numCoreInfos_ = numLogicalCores_ + numSystemCores_;
	cores_ = new CoreInfo[numCoreInfos_];
	ptr = buffer;
	byteOffset = 0;
	int currentCoreID = 0;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) 
	{
		switch (ptr->Relationship) 
		{
		case RelationProcessorCore:
			{
				cores_[currentCoreID].coreID = currentCoreID;
				cores_[currentCoreID].physicalCore = currentCoreID;
				cores_[currentCoreID].isLogical = false;

				// A hyper-threaded core supplies more than one logical processor.
				int numLogic = CountSetBits(ptr->ProcessorMask);
				for (int i=0; i<numLogic; ++i)
				{
					cores_[currentCoreID+i+1].coreID = currentCoreID + i + 1;
					cores_[currentCoreID+i+1].physicalCore = currentCoreID;
					cores_[currentCoreID+i+1].isLogical = true;
				}
				currentCoreID += numLogic + 1;
			}
			break;

		default:
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

}

int32 CpuInfo::getPhysicalCore(int32 core)
{
	BW_GUARD;

	MF_ASSERT(core >= 0 && core < numCoreInfos_ );

	return cores_[core].physicalCore;
}

bool CpuInfo::isLogical(int32 coreID)
{
	BW_GUARD;

	if ( coreID >= 0 && coreID < numCoreInfos_ )
		return cores_[coreID].isLogical;

	return true;
}

int32 CpuInfo::numberOfSystemCores()
{
	return numSystemCores_;
}

int32 CpuInfo::numberOfLogicalCores()
{
	return numLogicalCores_;
}

void CpuInfo::checkCurrentProcessorNumberSupport()
{
	// check to see if getCurrentProcessorNumber is supported
	getCurrentProcessorFunc = (GetCurrentProcessorNumberFunc) GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),
		"GetCurrentProcessorNumber");
	if (getCurrentProcessorFunc == NULL)
	{
		DEBUG_MSG("GetCurrentProcessorNumber is not supported\n");
	}
	supportChecked = true;
}	


int32 CpuInfo::getCurrentProcessorNumber()
{
	if (supportChecked == false)
		checkCurrentProcessorNumberSupport();

	if (getCurrentProcessorFunc)
		return getCurrentProcessorFunc();

	return 1;	// GetCurrentProcessorNumber not supported so return default value
}

#else

// default implementation

void CpuInfo::detectCores()
{

}

int32 CpuInfo::numberOfSystemCores()
{
	return 1;
}

int32 CpuInfo::numberOfLogicalCores()
{
	return 1;
}

int32 CpuInfo::getCurrentProcessorNumber()
{
	return 1;
}

void  CpuInfo::checkCurrentProcessorNumberSupport()
{
}

int32 CpuInfo::getPhysicalCore(int32 core)
{
	return 0;
}

bool CpuInfo::isLogical(int32 coreID)
{
	return false;
}


#endif

BW_END_NAMESPACE
