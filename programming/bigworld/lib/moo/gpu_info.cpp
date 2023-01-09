#include "pch.hpp"
#include "moo/gpu_info.hpp"

///////////////////////////////////////////////////////////////////////////////
// Private implementation. Avoid polluting global namespace
///////////////////////////////////////////////////////////////////////////////
#include <SetupAPI.h>
#include <NTSecAPI.h>
#include <Psapi.h>
#include "d3dkmt.h"

// From: ntddvdeo.h in Windows DDK
// Interface used by anyone listening for arrival of the display device
// {1CA05180-A699-450A-9A0C-DE4FBE3DDD89}
//
const GUID GUID_DISPLAY_DEVICE_ARRIVAL	= { 0x1CA05180, 0xA699, 0x450A, { 0x9A, 0x0C, 0xDE, 0x4F, 0xBE, 0x3D, 0xDD, 0x89 } };

// From: ntdef.h
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

BW_BEGIN_NAMESPACE

namespace Moo
{
	namespace Private
	{
		class GpuInfoImpl 
		{
		public:
			/// constructor
			GpuInfoImpl();

			/// destructor
			~GpuInfoImpl();

			/// returns information about current memory usage of the gpu
			bool getMemInfo( GpuInfo::MemInfo * memInfo, uint32 adapterID ) const;

		private:
			enum SegmentType
			{
				ST_INVALID,
				ST_SHARED,
				ST_DEDICATED
			};

			struct SegmentInfo
			{
				SegmentInfo() :
					type_( ST_INVALID ),
					commitLimit_( 0 ),
					bytesCommitted_( 0 )
				{}

				SegmentType type_;
				ULONG64		commitLimit_;
				ULONG64		bytesCommitted_;

			};

			struct DeviceInfo
			{
				BW::wstring deviceName_;
				BW::string deviceDescription_;
			};

			struct AdapterInfo
			{
				AdapterInfo() :
					adapterHandle_( -1 ),
					nodeCount_( 0 ),
					segmentCount_( 0 ),
					totalSharedMemory_( 0 ),
					totalDedicatedMemory_( 0 )
				{
					bw_zero_memory( &adapterLuid_, sizeof( adapterLuid_ ) );
				}

				BW::wstring  deviceName_;
				D3DKMT_HANDLE adapterHandle_;
				LUID		  adapterLuid_;
				ULONG		  nodeCount_;
				ULONG         segmentCount_;
				BW::vector<SegmentInfo> segments_;
				ULONG64		  totalSharedMemory_;
				ULONG64       totalDedicatedMemory_;
			};

		private:
			bool getDevices( BW::vector<DeviceInfo> * adapters ) const;
			bool getDeviceRegistryString( HDEVINFO deviceSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property, BW::string * result ) const;
			void initQuery( const AdapterInfo & adapter, D3DKMT_QUERYSTATISTICS_TYPE queryType, D3DKMT_QUERYSTATISTICS * query ) const;
			bool performQuery( D3DKMT_QUERYSTATISTICS * query ) const;

		private:
			PFND3DKMT_OPENADAPTERFROMDEVICENAME d3dOpenAdapterFromDeviceNameFunc_;
			PFND3DKMT_CLOSEADAPTER				d3dCloseAdapterFunc_;
			PFND3DKMT_QUERYSTATISTICS			d3dQueryStatisticsFunc_;
			BW::vector<DeviceInfo>				devices_;
			BW::vector<AdapterInfo>			adapters_;
			bool							    runningWindows8_;
		};

		//-------------------------------------------------------------------------
		GpuInfoImpl::GpuInfoImpl() :
			d3dOpenAdapterFromDeviceNameFunc_( NULL ),
			d3dCloseAdapterFunc_( NULL ),
			d3dQueryStatisticsFunc_( NULL ),
			runningWindows8_( false )
		{
			// get function pointers to d3d kernel functions
			HMODULE gdiModule = GetModuleHandle(L"gdi32.dll");
			d3dOpenAdapterFromDeviceNameFunc_ = (PFND3DKMT_OPENADAPTERFROMDEVICENAME)GetProcAddress(gdiModule, "D3DKMTOpenAdapterFromDeviceName");
			d3dCloseAdapterFunc_              = (PFND3DKMT_CLOSEADAPTER)GetProcAddress(gdiModule, "D3DKMTCloseAdapter");
			d3dQueryStatisticsFunc_           = (PFND3DKMT_QUERYSTATISTICS)GetProcAddress(gdiModule, "D3DKMTQueryStatistics");
		
			if (!d3dOpenAdapterFromDeviceNameFunc_ || !d3dCloseAdapterFunc_ || !d3dQueryStatisticsFunc_)
			{
				//TODO : error msg
				return;
			}

			// check what version of windows we are running
			OSVERSIONINFO osVersionInfo;
			::GetVersionEx( &osVersionInfo );
			if (osVersionInfo.dwMajorVersion == 6 && osVersionInfo.dwMinorVersion >= 2)
				runningWindows8_ = true;

			if( !getDevices( & devices_ ) )
				return;

			// get system adapter information
			for (size_t deviceIndex = 0; deviceIndex < devices_.size(); ++deviceIndex)
			{
				const DeviceInfo & device = devices_[deviceIndex];

				// now try to open the D3D device
				D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;
						
				openAdapterFromDeviceName.pDeviceName = device.deviceName_.c_str();
				if ( NT_ERROR( d3dOpenAdapterFromDeviceNameFunc_( &openAdapterFromDeviceName ) ) )
					continue;

				AdapterInfo info;
				info.adapterHandle_ = openAdapterFromDeviceName.hAdapter;
				info.adapterLuid_   = openAdapterFromDeviceName.AdapterLuid;
				info.deviceName_    = openAdapterFromDeviceName.pDeviceName;

				D3DKMT_QUERYSTATISTICS queryStatistics;
				initQuery( info, D3DKMT_QUERYSTATISTICS_ADAPTER, &queryStatistics );
				if (performQuery( &queryStatistics ))
				{
					info.nodeCount_    = queryStatistics.QueryResult.AdapterInformation.NodeCount;
					info.segmentCount_ = queryStatistics.QueryResult.AdapterInformation.NbSegments;

					// enumerate all segments
					for (ULONG segmentId = 0; segmentId < info.segmentCount_; ++segmentId)
					{
						initQuery( info, D3DKMT_QUERYSTATISTICS_SEGMENT, &queryStatistics );
						queryStatistics.QuerySegment.SegmentId = segmentId;
						if (performQuery( &queryStatistics ))
						{
							SegmentInfo segmentInfo;
							ULONG aperture = 0;
							ULONG64 commitLimit = 0;

							if (runningWindows8_)
							{
								commitLimit = queryStatistics.QueryResult.SegmentInformation.CommitLimit;
								aperture    = queryStatistics.QueryResult.SegmentInformation.Aperture;
							}
							else
							{
								commitLimit = queryStatistics.QueryResult.SegmentInformationV1.CommitLimit;
								aperture    = queryStatistics.QueryResult.SegmentInformationV1.Aperture;										
							}
										
							if (aperture)
							{
								segmentInfo.type_ = ST_SHARED;
								info.totalSharedMemory_ += commitLimit;
							}
							else
							{
								segmentInfo.type_ = ST_DEDICATED;
								info.totalDedicatedMemory_ += commitLimit;
							}

							segmentInfo.commitLimit_ = commitLimit;
							info.segments_.push_back( segmentInfo );
						}
					}

					adapters_.push_back( info );
				}
			}
		}

		GpuInfoImpl::~GpuInfoImpl()
		{
			for (size_t i = 0; i < adapters_.size(); ++i)
			{
				D3DKMT_CLOSEADAPTER closeAdapter;
				closeAdapter.hAdapter = adapters_[i].adapterHandle_;
				d3dCloseAdapterFunc_( &closeAdapter );
			}
			adapters_.clear();
		}

		bool GpuInfoImpl::getDevices( BW::vector<DeviceInfo> * devices ) const
		{
			// enumerate display devices
			HDEVINFO deviceSet = ::SetupDiGetClassDevs( &GUID_DISPLAY_DEVICE_ARRIVAL, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );

			if (deviceSet == INVALID_HANDLE_VALUE)
			{
				return false;
			}
		
			SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
			deviceInterfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA );
			DWORD curMemberIndex = 0;

			while (SetupDiEnumDeviceInterfaces( deviceSet, NULL, &GUID_DISPLAY_DEVICE_ARRIVAL, curMemberIndex, &deviceInterfaceData ))
			{
				SP_DEVINFO_DATA deviceInfoData = { 0 };
				deviceInfoData.cbSize = sizeof( SP_DEVINFO_DATA );
				DWORD requiredSize = 0;

				// get size of buffer needed 
				BOOL result = SetupDiGetDeviceInterfaceDetail( deviceSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL );

				if (!result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					// allocate space for the detailed data
					PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)::malloc( requiredSize );
					bw_zero_memory( detailData, requiredSize );
					detailData->cbSize = sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA );
					if (SetupDiGetDeviceInterfaceDetail( deviceSet, &deviceInterfaceData, detailData, requiredSize, NULL, &deviceInfoData ))
					{
						DeviceInfo info;
						info.deviceName_ = detailData->DevicePath;

						// get some nice information about the device
						getDeviceRegistryString( deviceSet, &deviceInfoData, SPDRP_DEVICEDESC,   &info.deviceDescription_ );

						devices->push_back( info );
					}

					::free( detailData );
				}

				// next device member
				++curMemberIndex;
			}	

			return !devices->empty();
		}

		bool GpuInfoImpl::getDeviceRegistryString( HDEVINFO deviceSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property, BW::string * result ) const
		{
			DWORD requiredSize = 0;
			SetupDiGetDeviceRegistryPropertyA( deviceSet, deviceInfoData, property, NULL, NULL, 0, &requiredSize );				
			result->resize( requiredSize );
			return SetupDiGetDeviceRegistryPropertyA( deviceSet, deviceInfoData, property, NULL, (PBYTE)&result->operator[](0), (DWORD)result->size(), NULL ) == TRUE;
		}


		void GpuInfoImpl::initQuery( const AdapterInfo & adapter, D3DKMT_QUERYSTATISTICS_TYPE queryType, D3DKMT_QUERYSTATISTICS * query ) const
		{
			bw_zero_memory( query, sizeof( query ) );
			query->hProcess = ::GetCurrentProcess();
			query->Type = queryType;
			query->AdapterLuid = adapter.adapterLuid_;

		}

		bool GpuInfoImpl::performQuery( D3DKMT_QUERYSTATISTICS * query ) const
		{
			return NT_SUCCESS( d3dQueryStatisticsFunc_( query ) ) == TRUE;
		}

		bool GpuInfoImpl::getMemInfo( GpuInfo::MemInfo * outMemInfo, uint32 adapterID ) const
		{
			if (adapters_.empty())
				return false;
		
			const AdapterInfo & adapter = adapters_[adapterID];

			bw_zero_memory( outMemInfo, sizeof( outMemInfo ) );
			outMemInfo->dedicatedMemTotal_ = adapters_[0].totalDedicatedMemory_;
			outMemInfo->sharedMemTotal_ = adapter.totalSharedMemory_;

			// query process
			D3DKMT_QUERYSTATISTICS queryStatistics;
			initQuery( adapter, D3DKMT_QUERYSTATISTICS_PROCESS, &queryStatistics );
			if (performQuery( & queryStatistics ))
			{
				outMemInfo->systemMemReserved_ = queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesReserved;
				outMemInfo->systemMemUsed_     = queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesAllocated;
			}

			// private usage.
			PROCESS_MEMORY_COUNTERS_EX procMemCounters;
			::GetProcessMemoryInfo( ::GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&procMemCounters, sizeof( procMemCounters ) );
			outMemInfo->privateUsage_ = procMemCounters.PrivateUsage;

			// unreserved and uncommited memory in the user-mode portion of virtual address space.
			MEMORYSTATUSEX memoryStatus = { sizeof( memoryStatus ) };
			GlobalMemoryStatusEx( &memoryStatus );
			outMemInfo->virtualAddressSpaceTotal_ = memoryStatus.ullTotalVirtual;
			outMemInfo->virtualAddressSpaceUsage_ = memoryStatus.ullTotalVirtual - memoryStatus.ullAvailVirtual;
			
			
			// grab shared + dedicated from each segment.
			for (size_t i = 0; i < adapter.segmentCount_; ++i)
			{
				SegmentInfo segment = adapter.segments_[i];

				// query this segment
				initQuery( adapter, D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT, &queryStatistics );
				queryStatistics.QueryProcessSegment.SegmentId = (ULONG)i;
				if (performQuery( & queryStatistics ))
				{
					ULONGLONG commitedBytes = 0;
					commitedBytes = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;

					if (!runningWindows8_)
					{
						commitedBytes = (ULONG)commitedBytes;
					}

					if (segment.type_ == ST_SHARED)
					{
						outMemInfo->sharedMemCommitted_ += commitedBytes;
					}
					else
					{
						outMemInfo->dedicatedMemCommitted_ += commitedBytes;
					}
				}
			}

			return true;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// GpuInfo implementation
	///////////////////////////////////////////////////////////////////////////////

	GpuInfo::GpuInfo()
	{
		pimpl_ = new Private::GpuInfoImpl();
	}

	GpuInfo::~GpuInfo()
	{
		delete pimpl_;
	}

	bool GpuInfo::getMemInfo( MemInfo * outMemInfo, uint32 adapterID ) const
	{
		return pimpl_->getMemInfo( outMemInfo, adapterID );
	}

} // end namespace
BW_END_NAMESPACE
