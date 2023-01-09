#ifndef GRAPHICS_SETTINGS_PICKER_HPP
#define GRAPHICS_SETTINGS_PICKER_HPP

struct IWbemServices;
struct IWbemLocator;
struct IEnumWbemClassObject;

BW_BEGIN_NAMESPACE

typedef SmartPointer<class DataSection> DataSectionPtr;

namespace Moo
{


class SettingsGroup;
typedef SmartPointer<SettingsGroup> SettingsGroupPtr;

typedef std::pair<BW::string, uint32> Setting;
typedef BW::vector<Setting> SettingsVector;

/**
 *	This class is a helper class that helps with picking 
 *	appropriate graphics settings for a specific device.
 *	The settings and matching criteria for the device are
 *	defined in xml files and are matched agains the 
 *	D3DADAPTER_IDENTIFIER9 struct which is queried from
 *	IDirect3D9 object, there are three tiers of
 *	matching criteria. Listed from most important to least
 *	important they are:
 *	The first one is GUID which is an exact match on a 
 *	chipset, manufacturer and driver version. 
 *	The second tier is the chipset and manufacturer using 
 *	the VendorID and DeviceID of the current graphics card.
 *	The third is a string based compare of the device
 *	description.
 */

class GraphicsSettingsPicker
{
public:
	GraphicsSettingsPicker();
	~GraphicsSettingsPicker();
	bool init();

	bool getSettings( const D3DADAPTER_IDENTIFIER9& AdapterID, SettingsVector& out ) const;

private:
	BW::vector<SettingsGroupPtr> settingsGroups_;
	
	void addSettingGroup( const DataSectionPtr & pSection );
};

// This class auto detects the system configuration and chooses the best matching options for the game
class GraphicsSettingsAutoDetector
{
	enum Vendors
	{
		NVIDIA,
		ATI,
		INTEL,
		VENDOR_COUNT
	};
public:
	enum SystemPerformance
	{
		MAX = 0, HIGH, MEDIUM, LOW, MIN, PERF_LEVEL_COUNT
	};

	enum SystemPerformanceOverride
	{
		NO_OVERRIDE = -1
	};

	GraphicsSettingsAutoDetector();
	~GraphicsSettingsAutoDetector();
	bool init();
	bool getSettings( SettingsVector& out, int overrideSettingsGroup = NO_OVERRIDE );
	void readConfigSetting(const DataSectionPtr & pConfig, const char* name, SystemPerformance level);
	SystemPerformance detectBestSettings(bool forceNewDetection = false);

	//-- return total virtual and video memory in mega-bytes (Mb).
	static int64 getTotalVirtualMemory();
	static int64  getTotalVideoMemory();
private:
	void initConfig();
	SystemPerformance getQualityForNotebooks();
	void initDefaultConfig();
	void addSettingsGroup( const DataSectionPtr & pSection, const char* name, SystemPerformance quality );
	static int64 getVideoMemory();
	int64 GetCPUFrequency();
	int64 GetTicks();
	static HRESULT GetVideoMemoryViaDXGI(	HMONITOR hMonitor, SIZE_T* pDedicatedVideoMemory,
		SIZE_T* pDedicatedSystemMemory, SIZE_T* pSharedSystemMemory );
	static int64 getVideoMemoryViaWMI();

	static HRESULT InitWMI(IWbemServices **pSvc, IWbemLocator **pLoc );
	int64 GetRAMFrequency(int cnt = 10);
	bool IsNotebook();

	typedef void (*CHECK_FUNCTION)(INT &, VARIANT); 
	bool GetDeviceEnumerator(IWbemServices *pSvc, IWbemLocator *pLoc, BSTR wmiQuery, IEnumWbemClassObject** pEnumerator);
	INT GetDeviceObjectsInfo(IEnumWbemClassObject* pEnumerator, wchar_t* nameOfTheInfo, CHECK_FUNCTION myComparator);


	static void CheckEnclosureType(INT & result, VARIANT vtProp);
	static void CheckBatteryType(INT & result, VARIANT vtProp);
	static void CheckPhysMemoryType(INT & result, VARIANT vtProp);

	SystemPerformance CheckNvidiaGPUId(const char* deviceId, SystemPerformance curPerformance = MIN);
	SystemPerformance CheckAtiGPUId(const char* deviceId, SystemPerformance curPerformance = MIN);
	SystemPerformance CheckIntelGPUId(const char* deviceId, SystemPerformance curPerformance = MIN);
	void readVendorSetting(const DataSectionPtr & pSection, SystemPerformance level, const char* name, Vendors vendor);
private:
	SettingsGroupPtr settingsGroups_[PERF_LEVEL_COUNT];

	struct DeviceInfo
	{
		DeviceInfo() :
			number(),
			need_check( false )
		{}
		
		BW::string prefix;
		int number;
		bool need_check;
	};

	struct PerfBounds 
	{
		UINT CPUFrequency;
		UINT GPUMemory;
		UINT RAM;
		UINT AvailableVirtualMemory;
		UINT RAMFreq;
		float PixelShaderVersion;
		float VertexShaderVersion;
		bool NotebookCompartible;
		DeviceInfo deviceInfo[VENDOR_COUNT];
	};
	PerfBounds bounds[PERF_LEVEL_COUNT];
	PerfBounds defaultBounds[PERF_LEVEL_COUNT];
	static SystemPerformance lastDetection;
	bool bPrintSettings;
};

} //namespace Moo

BW_END_NAMESPACE

#endif // GRAPHICS_SETTINGS_REGISTRY_HPP
