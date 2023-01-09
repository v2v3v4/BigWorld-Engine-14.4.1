// Module Interface
#include "pch.hpp"
#include "graphics_settings_picker.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/unique_id.hpp"
#include <d3d9.h>
#include <fstream>
#include <intrin.h>


#define _WIN32_DCOM
#include <Wbemidl.h>

#include <dxgi.h>
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#include <comdef.h>
# pragma comment(lib, "wbemuuid.lib")

typedef HRESULT ( WINAPI* LPCREATEDXGIFACTORY )( REFIID, void** );

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

static AutoConfigString s_graphicsSettingsConfigName( "system/graphicsSettingsPresets" );
static AutoConfigString s_autoConfigSettingsFileName( "system/autoConfigSettings" );
static AutoConfigString s_engineConfigXML("system/engineConfigXML");



/*
 *	This class stores a VendorID and deviceID pair and allows you to 
 *	compare them it is a helper class for checking for a device 
 *	matches.
 */
class VendorDeviceIDPair
{
public:
	/*
	 *	Constructor that reads the id's from a datasection
	 *	@param pIDSection data section that contains the  VendorID 
	 *	and DeviceID pair
	 */
	VendorDeviceIDPair( DataSectionPtr pIDSection )
	{
		BW_GUARD;
		vendorID_ = pIDSection->readInt( "VendorID" );
		deviceID_ = pIDSection->readInt( "DeviceID" );
	}

	/*
	 *	Constructor that takes the id's as arguments
	 *	@param vendorID the vendor id
	 *	@param deviceID the device id
	 */
	VendorDeviceIDPair( uint32 vendorID, uint32 deviceID ) :
	vendorID_(vendorID),
	deviceID_(deviceID)
	{
	}

	/*
	 * Comparison operator
	 * @return true if the objects are identical
	 */
	bool operator == (const VendorDeviceIDPair& other) const
	{
		BW_GUARD;
		return other.vendorID_ == vendorID_ && 
			other.deviceID_ == deviceID_;
	}
private:
	uint32 vendorID_;
	uint32 deviceID_;
};

/*
 * This class is a helper class that matches a set of predefined strings
 * to a device description string.
 */
class DeviceDescriptionMatcher
{
public:
	/*
	 *	Constructor loads up a number of substrings from the provided datasection
	 */
	DeviceDescriptionMatcher( DataSectionPtr pDescriptionMatcherSection )
	{
		BW_GUARD;
		DataSectionIterator it = pDescriptionMatcherSection->begin();
		for (;it != pDescriptionMatcherSection->end();++it)
		{
			BW::string substring = (*it)->asString();
			std::transform( substring.begin(), substring.end(), 
				substring.begin(), tolower );
			subStrings_.push_back( substring );
		}
	}

	/*
	 *	This method checks a device description against the substrings 
	 *	stored in this object
	 *	@param deviceDescription the device description string to check
	 *		this is assumed to be lower case
	 *	@return true if all the substrings defined in this object are
	 *		contained in the device description
	 */
	bool check( const BW::string& deviceDescription ) const
	{
		BW_GUARD;
		for (uint32 i = 0; i < subStrings_.size(); ++i)
		{
			if (deviceDescription.find(subStrings_[i]) == BW::string::npos)
				return false;
		}
		return subStrings_.size() != 0;
	}

private:
	BW::vector<BW::string> subStrings_;
};

namespace Moo
{

/*
 *	This class handles a group of settings, it contains the actual graphics 
 *	settings for the group as well as any matching criteria.
 */
class SettingsGroup : public ReferenceCount
{
public:
	SettingsGroup() :
	  isDefault_( false )
	{
	}
	/*
	 *	This method inits the settings group from a data section
	 *	@param pSection the data section to init the group from
	 *	@return true if we have both matching criteria and settings
	 */
	bool init( DataSectionPtr pSection )
	{
		BW_GUARD;
		// Read our GUID matches
		BW::vector<BW::string> guidMatches;
		pSection->readStrings( "GUIDMatch", guidMatches );
		for (uint32 i = 0; i < guidMatches.size(); ++i)
		{
			guidMatches_.push_back( UniqueID( guidMatches[i] ) );
		}

		// Read our device and vendor id matches
		BW::vector<DataSectionPtr> vendorDeviceIDMatchSections;
		pSection->openSections( "VendorDeviceIDMatch", vendorDeviceIDMatchSections );
		for (uint32 i = 0; i < vendorDeviceIDMatchSections.size(); ++i)
		{
			vendorDeviceIDMatches_.push_back( VendorDeviceIDPair( vendorDeviceIDMatchSections[i] ) );
		}

		// Read our device description matches
		BW::vector<DataSectionPtr> deviceDescriptionMatchSections;
		pSection->openSections( "DeviceDescriptionMatch", deviceDescriptionMatchSections );

		for (uint32 i = 0; i < deviceDescriptionMatchSections.size(); ++i)
		{
			deviceDescriptionMatches_.push_back( DeviceDescriptionMatcher(deviceDescriptionMatchSections[i]) );
		}

		// Read the actual graphics settings
		BW::vector<DataSectionPtr> settingsSections;
		pSection->openSections( "entry", settingsSections );

		for (uint32 i = 0; i < settingsSections.size(); ++i)
		{
			DataSectionPtr pSettingSection = settingsSections[i];
			BW::string label = pSettingSection->readString( "label" );
			uint32 activeOption = pSettingSection->readInt( "activeOption" );
			settings_.push_back(std::make_pair( label, activeOption ) );
		}

		// Read the default setting
		isDefault_ = pSection->readBool( "defaultSetting", false );

		return (!guidMatches_.empty() || 
				!vendorDeviceIDMatches_.empty() || 
				!deviceDescriptionMatches_.empty() ||
				isDefault_) &&
			   !settings_.empty();
	}

	/*
	 *	This method checks if this SettingsGroup matches the adapter id
	 *	@param adapterID the adapter id of the device we want to use
	 *	@return the matching level, if the GUID matches, the return value
	 *		is 4 if the vendor and device id matches the return value is 3
	 *		if the device description matches the return value is 2, if there
	 *		is no match but this the default settings group, the return value 
	 *		is one, otherwise the return value is 0
	 */
	uint32 check( const D3DADAPTER_IDENTIFIER9& adapterID ) const
	{
		BW_GUARD;
		// First check if we have a GUID match, the GUID match is highest priority 
		// match
		const UniqueID& deviceId = (const UniqueID&)adapterID.DeviceIdentifier;
		for (uint32 i = 0; i < guidMatches_.size(); ++i)
		{
			// If the GUID matches, return 4
			if (guidMatches_[i] == deviceId)
				return 4;
		}

		// Check if the vendor and device id matches
		// this is the second highest match
		VendorDeviceIDPair currentVendorDeviceID( adapterID.VendorId, adapterID.DeviceId );
		for (uint32 i = 0; i < vendorDeviceIDMatches_.size(); i++)
		{
			// If the id's match, return 3
			if (vendorDeviceIDMatches_[i] == currentVendorDeviceID)
				return 3;
		}

		// Check if the device description matches, this is weakest actual match
		// We make sure the description is lower case before checking
		BW::string deviceDescription = adapterID.Description;
		std::transform( deviceDescription.begin(), deviceDescription.end(), 
			deviceDescription.begin(), tolower );
		for (uint32 i = 0; i < deviceDescriptionMatches_.size(); ++i)
		{
			// If the description matches, return 2
			if (deviceDescriptionMatches_[i].check( deviceDescription ))
				return 2;
		}

		// If no matches were found, return 1 if we are the default
		// otherwise return 0, i.e. no matches found
		return isDefault_ ? 1 : 0;
	}

	const SettingsVector& settings() const { return settings_; }

private:
	BW::vector<UniqueID> guidMatches_;
	BW::vector<VendorDeviceIDPair> vendorDeviceIDMatches_;
	BW::vector<DeviceDescriptionMatcher> deviceDescriptionMatches_;
	SettingsVector settings_;

	bool isDefault_;
};


/**
 *	This method inits the graphics settings picker, it loads the graphics settings
 *	config from the .xml file so that we can match it with the currently set device
 *	@return true if the settings load successfully
 */
bool GraphicsSettingsPicker::init()
{
	BW_GUARD;
	DataSectionPtr pConfig = BWResource::instance().openSection( s_graphicsSettingsConfigName.value() );
	if (pConfig.exists())
	{
		DataSectionIterator it = pConfig->begin();
		for (;it != pConfig->end(); ++it)
		{
			addSettingGroup( *it );
		}
	}
	return pConfig.exists();
}

/**
 *	This method gets the settings for a specific device
 *	@param adapterID the id of the current adapter
 *	@param out return the settings that have been selected for the adapter, 
		if no settings are found, this value remains unchanged
 *	@return true if settings for the current device were found, false 
		if not
 */
bool GraphicsSettingsPicker::getSettings( const D3DADAPTER_IDENTIFIER9& adapterID, 
														  SettingsVector& out ) const
{
	BW_GUARD;
	bool res = false;
	uint32 settingsWeight = 0;
	SettingsGroupPtr pSelected;

	// Iterate over all our setiings groups and see if they match our device
	for (uint32 i = 0; i < settingsGroups_.size(); i++)
	{
		// Get the group and check if the weight of the group
		// is higher than the currently selected weight,
		// we have found a better match
		SettingsGroupPtr pGroup = settingsGroups_[i];
		uint32 weight = pGroup->check( adapterID );
		if (weight > settingsWeight)
		{
			settingsWeight = weight;
			pSelected = pGroup;
		}
	}

	// If a settings group has been selected, return the settings from
	// this group.
	if (pSelected.exists())
	{
		out = pSelected->settings();
		res = true;
	}

	return res;
}

/*
 *	This method loads and adds a settings group to our list
 *	@param pSection the datasection
 */
void GraphicsSettingsPicker::addSettingGroup( const DataSectionPtr & pSection )
{
	BW_GUARD;
	// Load the settings group, if the load succeeded, add it to our 
	// list of settings groups
	SettingsGroupPtr pSetting = new SettingsGroup;
	if (pSetting->init(pSection))
	{
		settingsGroups_.push_back( pSetting );
	}
}

GraphicsSettingsPicker::GraphicsSettingsPicker()
{
}

GraphicsSettingsPicker::~GraphicsSettingsPicker()
{
}

bool GraphicsSettingsAutoDetector::init()
{
	BW_GUARD;
	if (s_graphicsSettingsConfigName.value().empty())
	{
		return false;
	}

	DataSectionPtr pConfig = BWResource::instance().openSection( s_graphicsSettingsConfigName.value() );
	if (pConfig.exists())
	{
		addSettingsGroup( pConfig, "MAX", MAX );
		addSettingsGroup( pConfig, "HIGH", HIGH );
		addSettingsGroup( pConfig, "MEDIUM", MEDIUM );
		addSettingsGroup( pConfig, "LOW", LOW );
		addSettingsGroup( pConfig, "MIN", MIN );
	}
	
	initConfig();
	return pConfig.exists();
}

void GraphicsSettingsAutoDetector::initConfig()
{
	BW_GUARD;
	initDefaultConfig();

	DataSectionPtr pConfig = BWResource::instance().openSection( s_autoConfigSettingsFileName.value() );
	if (pConfig.exists() && !s_autoConfigSettingsFileName.value().empty())
	{
		readConfigSetting(pConfig, "MAX", MAX);
		readConfigSetting(pConfig, "HIGH", HIGH);
		readConfigSetting(pConfig, "MEDIUM", MEDIUM);
		readConfigSetting(pConfig, "LOW", LOW);
		readConfigSetting(pConfig, "MIN", MIN);
	}
	else
		memcpy_s(&bounds, sizeof(PerfBounds)*PERF_LEVEL_COUNT, &defaultBounds, sizeof(PerfBounds)*PERF_LEVEL_COUNT);

	bPrintSettings = false;

	BW::string filename = s_engineConfigXML.value();
	DataSectionPtr configRoot =
		BWResource::instance().openSection(filename);	
	if (configRoot != NULL)
	{
		bPrintSettings = configRoot->readBool("printAutoDetect", false);
	}
}

void GraphicsSettingsAutoDetector::readVendorSetting(const DataSectionPtr & vendorSection, SystemPerformance level, const char* vendorName, Vendors vendor)
{
	BW::string name = vendorSection->readString(vendorName, "");
	if(name.length() > 0)
	{
		DeviceInfo& di = bounds[level].deviceInfo[vendor];
		size_t pos = name.find_first_of(' ');
		if(pos != BW::string::npos)
		{
			di.prefix = name.substr(0, pos);
			sscanf_s(name.c_str() + pos + 1, "%d", &di.number);
		}
		else
			sscanf_s(name.c_str(), "%d", &di.number);

		di.need_check = true;
	}
}

void GraphicsSettingsAutoDetector::readConfigSetting( const DataSectionPtr & pConfig, const char* name, SystemPerformance level )
{
	BW::vector<DataSectionPtr> sections;
	pConfig->openSections("graphicsPreferences", sections);
	BW::vector<DataSectionPtr>::iterator it = sections.begin();
	if(sections.size())
	{
		for ( ; it!= sections.end(); ++it)
		{
			if ((*it)->asString() == name)
				break;
		}
	}
	if(it!=sections.end())
	{
		bounds[level].CPUFrequency = (*it)->readInt("CPUFrequency", defaultBounds[level].CPUFrequency);
		bounds[level].GPUMemory = (*it)->readInt("GPUMemory", defaultBounds[level].GPUMemory);
		bounds[level].RAM = (*it)->readInt("RAM", defaultBounds[level].RAM);
		bounds[level].AvailableVirtualMemory = (*it)->readInt("AvailableVirtualMemory", defaultBounds[level].AvailableVirtualMemory);
		bounds[level].RAMFreq = (*it)->readInt("RAMFrequency", defaultBounds[level].RAMFreq);
		bounds[level].PixelShaderVersion = (*it)->readFloat("PixelShaderVersion", defaultBounds[level].PixelShaderVersion);
		bounds[level].VertexShaderVersion = (*it)->readFloat("VertexShaderVersion", defaultBounds[level].VertexShaderVersion);
		bounds[level].NotebookCompartible = (*it)->readBool("NotebookCompartible", defaultBounds[level].NotebookCompartible);

		DataSectionPtr vendorSection = (*it)->openSection("GPUDeviceId");
		if(vendorSection.exists())
		{
			readVendorSetting(vendorSection, level, "NVIDIA", NVIDIA);
			readVendorSetting(vendorSection, level, "ATI", ATI);
			readVendorSetting(vendorSection, level, "INTEL", INTEL);

			if(bounds[level].deviceInfo[ATI].need_check) // sign known prefixes
			{
				if (bounds[level].deviceInfo[ATI].prefix == "HD")
					bounds[level].deviceInfo[ATI].number *= 10000;
				else if (bounds[level].deviceInfo[ATI].prefix == "X")
					bounds[level].deviceInfo[ATI].number *= 100;
			}
			if(bounds[level].deviceInfo[INTEL].need_check) // sign known prefixes
			{
				if (bounds[level].deviceInfo[INTEL].prefix == "HD")
					bounds[level].deviceInfo[INTEL].number *= 10000;
				else if (bounds[level].deviceInfo[INTEL].prefix == "GMA")
					bounds[level].deviceInfo[INTEL].number *= 100;
			}
			if(bounds[level].deviceInfo[NVIDIA].need_check) // sign newer xxx series in order to be more than xxxx older series
			{
				if (bounds[level].deviceInfo[NVIDIA].number >= 100 && bounds[level].deviceInfo[NVIDIA].number < 1000)
					bounds[level].deviceInfo[NVIDIA].number *= 10000;
			}
		}
	}
	else
		bounds[level] = defaultBounds[level];
}

void GraphicsSettingsAutoDetector::initDefaultConfig()
{
	defaultBounds[MAX].CPUFrequency = 5000;
	defaultBounds[MAX].GPUMemory = 800;
	defaultBounds[MAX].RAM = 3000;
	defaultBounds[MAX].AvailableVirtualMemory = 3000;
	defaultBounds[MAX].RAMFreq = 1600;
	defaultBounds[MAX].PixelShaderVersion = 3.0f;
	defaultBounds[MAX].VertexShaderVersion = 3.0f;
	defaultBounds[MAX].NotebookCompartible = false;

	defaultBounds[HIGH].CPUFrequency = 3000;
	defaultBounds[HIGH].GPUMemory = 600;
	defaultBounds[HIGH].RAM = 2000;
	defaultBounds[HIGH].AvailableVirtualMemory = 3000;
	defaultBounds[HIGH].RAMFreq = 1000;
	defaultBounds[HIGH].PixelShaderVersion = 3.0f;
	defaultBounds[HIGH].VertexShaderVersion = 3.0f;
	defaultBounds[HIGH].NotebookCompartible = false;

	defaultBounds[MEDIUM].CPUFrequency = 2000;
	defaultBounds[MEDIUM].GPUMemory = 450;
	defaultBounds[MEDIUM].RAM = 2000;
	defaultBounds[MEDIUM].AvailableVirtualMemory = 2000;
	defaultBounds[MEDIUM].RAMFreq = 600;
	defaultBounds[MEDIUM].PixelShaderVersion = 3.0f;
	defaultBounds[MEDIUM].VertexShaderVersion = 3.0f;
	defaultBounds[MEDIUM].NotebookCompartible = false;

	defaultBounds[LOW].CPUFrequency = 1500;
	defaultBounds[LOW].GPUMemory = 250;
	defaultBounds[LOW].RAM = 1000;
	defaultBounds[LOW].AvailableVirtualMemory = 1000;
	defaultBounds[LOW].RAMFreq = 400;
	defaultBounds[LOW].PixelShaderVersion = 2.0f;
	defaultBounds[LOW].VertexShaderVersion = 2.0f;
	defaultBounds[LOW].NotebookCompartible = true;

	defaultBounds[MIN].CPUFrequency = 1500;
	defaultBounds[MIN].GPUMemory = 250;
	defaultBounds[MIN].RAM = 1000;
	defaultBounds[MIN].AvailableVirtualMemory = 1000;
	defaultBounds[MIN].RAMFreq = 400;
	defaultBounds[MIN].PixelShaderVersion = 2.0f;
	defaultBounds[MIN].VertexShaderVersion = 2.0f;
	defaultBounds[MIN].NotebookCompartible = true;
}

void GraphicsSettingsAutoDetector::addSettingsGroup( const DataSectionPtr & pSection, const char* name, SystemPerformance quality )
{
	BW_GUARD;
	BW::vector<DataSectionPtr> sections;
	pSection->openSections("graphicsPreferences", sections);
	BW::vector<DataSectionPtr>::iterator it = sections.begin();
	if(sections.size())
	{
		for ( ; it!= sections.end(); ++it)
		{
			if ((*it)->asString() == name)
				break;
		}
	}
	if(it!=sections.end())
	{
		SettingsGroupPtr pSetting = new SettingsGroup;
		pSetting->init(*it);
		settingsGroups_[quality] = pSetting;
	}
}

bool GraphicsSettingsAutoDetector::getSettings( SettingsVector& out, int overrideSettingsGroup )
{
	BW_GUARD;
	SystemPerformance performanceQuality = 
		static_cast<SystemPerformance>(overrideSettingsGroup);
	
	// Check to see if the override is a valid selection
	if (overrideSettingsGroup < 0 ||
		overrideSettingsGroup >= PERF_LEVEL_COUNT)
	{
		performanceQuality = detectBestSettings();
	}
	else
	{
		INFO_MSG( "Performance overridden with %u\n", (UINT)performanceQuality );
	}
	
	out = settingsGroups_[performanceQuality]->settings();
	return true;
}

#define MINOR_SHADER_VER(quality, ShaderType) ((BYTE)((bounds[quality].ShaderType##ShaderVersion - (BYTE)bounds[quality].ShaderType##ShaderVersion) * 10.0f))
#define MAJOR_SHADER_VER(quality, ShaderType) ((BYTE)bounds[quality].ShaderType##ShaderVersion)
#define PS_VERSION(quality) D3DPS_VERSION(MAJOR_SHADER_VER(quality, Pixel),MINOR_SHADER_VER(quality, Pixel))
#define VS_VERSION(quality) D3DVS_VERSION(MAJOR_SHADER_VER(quality, Vertex),MINOR_SHADER_VER(quality, Vertex))

#define CHECK_PARAMETER(param) \
	if ( param < bounds[MIN].param)		\
		{ INFO_MSG( "Warning! %s amount %" PRI64 " is too low! Must be at least %u\n", #param, param, bounds[MIN].param ); quality = MIN;} \
	else if ( param < bounds[LOW].param )			\
		quality = MIN;								\
	else if ( param < bounds[MEDIUM].param )		\
		quality = max(LOW, quality);				\
	else if ( param < bounds[HIGH].param )			\
		quality = max(MEDIUM, quality);				\
	if(param < bounds[MAX].param)					\
		quality = max(HIGH, quality);


GraphicsSettingsAutoDetector::SystemPerformance GraphicsSettingsAutoDetector::detectBestSettings(bool forceNewDetection)
{
    static const char * messages[] = {
        "Checking Notebook",
        "GPU Memory: ",
        "RAM: ",
        "Virtual Memory: ",
        "RAM Frequency:	",
        "CPU Frequency:	",
        "Vertex Shader Ver",
        "Pixel Shader Ver",
        "Check VideoAdapter"
    };

	if(!forceNewDetection && lastDetection != PERF_LEVEL_COUNT)
		return lastDetection;

	GraphicsSettingsAutoDetector::SystemPerformance qualitiesAfterEachParameter[9] = {MAX};
	__int64 systemSettings[9] = {0};

	UINT indexForQualities = 0;
	UINT indexForSettings = 1; 
	
	INFO_MSG( "Detecting performance...\n" );

	GraphicsSettingsAutoDetector::SystemPerformance quality = MAX;

	quality = max(quality, getQualityForNotebooks());
	qualitiesAfterEachParameter[indexForQualities++] = quality;
	
	//GPU memory check
	const int64 GPUMemory = getVideoMemory() / ( 1024 * 1024 );
	CHECK_PARAMETER(GPUMemory);
	systemSettings[indexForSettings++] = GPUMemory;
	qualitiesAfterEachParameter[indexForQualities++] = quality;

	//RAM amount check
	MEMORYSTATUSEX ms;
	ZeroMemory( &ms, sizeof( ms ) );
	ms.dwLength = sizeof( ms );
	GlobalMemoryStatusEx( &ms );
	__int64 RAM = ms.ullTotalPhys / ( 1024 * 1024 );
	CHECK_PARAMETER(RAM);
	systemSettings[indexForSettings++] = RAM;
	qualitiesAfterEachParameter[indexForQualities++] = quality;

	//VirtualMemory amount check
	__int64 AvailableVirtualMemory = ms.ullTotalVirtual / ( 1024 * 1024 );
	CHECK_PARAMETER(AvailableVirtualMemory);
	systemSettings[indexForSettings++] = AvailableVirtualMemory;
	qualitiesAfterEachParameter[indexForQualities++] = quality;

	//RAM frequency check
	const int64 RAMFreq = GetRAMFrequency();
	CHECK_PARAMETER(RAMFreq);
	systemSettings[indexForSettings++] = RAMFreq;
	qualitiesAfterEachParameter[indexForQualities++] = quality;
	
	//CPU frequency check
	const int64 iFreq = GetCPUFrequency() / 1000000;
	SYSTEM_INFO si;
	GetSystemInfo( &si );
	UINT numProcessors = si.dwNumberOfProcessors;
	const int64 CPUFrequency = iFreq * numProcessors;
	CHECK_PARAMETER(CPUFrequency);
	systemSettings[indexForSettings++] = CPUFrequency;
	qualitiesAfterEachParameter[indexForQualities++] = quality;
	

	//shader version check
	IDirect3D9* pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
	if( pD3D9 )
	{
		D3DCAPS9 Caps;
		HRESULT hr = pD3D9->GetDeviceCaps(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&Caps);
		if(SUCCEEDED(hr))
		{
			if(Caps.PixelShaderVersion < PS_VERSION(MIN) ||	Caps.VertexShaderVersion < VS_VERSION(MIN))
			{
				INFO_MSG( "Warning! Shader version is too low! Must be at least %f for vertex and %f for pixel shader version.\n", 
				bounds[MIN].VertexShaderVersion, bounds[MIN].PixelShaderVersion );
				quality = MIN;
			}
			else if(Caps.PixelShaderVersion < PS_VERSION(LOW) ||
					Caps.VertexShaderVersion < VS_VERSION(LOW))
				quality = MIN;
			else if(Caps.PixelShaderVersion < PS_VERSION(MEDIUM) ||
					Caps.VertexShaderVersion < VS_VERSION(MEDIUM))
				quality = max(quality, LOW);
			else if(Caps.PixelShaderVersion < PS_VERSION(HIGH) ||
					Caps.VertexShaderVersion < VS_VERSION(HIGH))
				quality = max(quality, MEDIUM);
			else if(Caps.PixelShaderVersion < PS_VERSION(MAX) ||
					Caps.VertexShaderVersion < VS_VERSION(MAX))
				quality = max(quality, HIGH); 
			qualitiesAfterEachParameter[indexForQualities++] = quality;
			qualitiesAfterEachParameter[indexForQualities++] = quality;
		}
		pD3D9->Release();
	}
	
	//	Check by video card model.
	//	Note: now works only for Nvidia GPUs
	const Moo::DeviceInfo& di = rc().deviceInfo( 0 );
	if(di.identifier_.VendorId == 0x10DE) // NVIDIA
		quality = max(quality, CheckNvidiaGPUId(di.identifier_.Description)); 
	else if(di.identifier_.VendorId == 0x1002) // ATI
		quality = max(quality, CheckAtiGPUId(di.identifier_.Description)); 
	else if(di.identifier_.VendorId == 0x8086) // Intel
		quality = max(quality, CheckIntelGPUId(di.identifier_.Description)); 
	qualitiesAfterEachParameter[indexForQualities++] = quality;
	
	INFO_MSG( "Performance detected as %u\n", (UINT)quality );

	lastDetection = quality;

	if (bPrintSettings)
	{
		std::ofstream out("SystemProperties.txt", std::ios::app);
		out << "NEW RECORD" << std::endl;
		for (int i = 0; i < 9; ++i)
		{
			out << messages[i];
			if (i > 0 && i < 6)
				out << systemSettings[i];
			out << ";	Quality: " << qualitiesAfterEachParameter[i] << std::endl; 
		}
		out.close();
	}
	
	return quality;

}

Moo::GraphicsSettingsAutoDetector::SystemPerformance Moo::GraphicsSettingsAutoDetector::getQualityForNotebooks()
{
	GraphicsSettingsAutoDetector::SystemPerformance quality = MAX;
	if (IsNotebook())
	{
		bool flag = false;
		for (int i = 0; i <= MIN && !flag; ++i)
		{
			if (bounds[i].NotebookCompartible)
			{
				quality = (SystemPerformance)i;
				flag = true;
			}
		}
		if (!flag)
			quality = MIN;
	}
	return quality;
}

int64 Moo::GraphicsSettingsAutoDetector::GetCPUFrequency()
{
	const DWORD dwSleepTime = 500;
	int64 t = GetTicks( );
	Sleep( dwSleepTime );
	t = GetTicks() - t;
	return t * 1000 / dwSleepTime;
}


int64 Moo::GraphicsSettingsAutoDetector::getVideoMemory()
{
	IDirect3D9* pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
	
	if (pD3D9)
	{
		const HMONITOR hMonitor = pD3D9->GetAdapterMonitor(0);

		SIZE_T DedicatedVideoMemory = 0;
		SIZE_T DedicatedSystemMemory = 0;
		SIZE_T SharedSystemMemory = 0;
		const HRESULT hr = GetVideoMemoryViaDXGI(
			hMonitor,
			&DedicatedVideoMemory, &DedicatedSystemMemory, &SharedSystemMemory);
				
		pD3D9->Release();

		if ( SUCCEEDED( hr ) )
		{
			return static_cast<int64>( DedicatedVideoMemory );
		}
	}
	return getVideoMemoryViaWMI();
}


HRESULT GraphicsSettingsAutoDetector::GetVideoMemoryViaDXGI(
	HMONITOR hMonitor, SIZE_T* pDedicatedVideoMemory,
	SIZE_T* pDedicatedSystemMemory, SIZE_T* pSharedSystemMemory )
{
	HRESULT hr = 0;
	bool bGotMemory = false;
	*pDedicatedVideoMemory = 0;
	*pDedicatedSystemMemory = 0;
	*pSharedSystemMemory = 0;

	HINSTANCE hDXGI = LoadLibrary( L"dxgi.dll" );
	if( hDXGI )
	{
		LPCREATEDXGIFACTORY pCreateDXGIFactory = NULL;
		IDXGIFactory* pDXGIFactory = NULL;

		pCreateDXGIFactory = ( LPCREATEDXGIFACTORY )GetProcAddress( hDXGI, "CreateDXGIFactory" );
		pCreateDXGIFactory( __uuidof( IDXGIFactory ), ( LPVOID* )&pDXGIFactory );

		for( int index = 0; ; ++index )
		{
			bool bFoundMatchingAdapter = false;
			IDXGIAdapter* pAdapter = NULL;
			hr = pDXGIFactory->EnumAdapters( index, &pAdapter );
			if( FAILED( hr ) ) // DXGIERR_NOT_FOUND is expected when the end of the list is hit
				break;

			for( int iOutput = 0; ; ++iOutput )
			{
				IDXGIOutput* pOutput = NULL;
				hr = pAdapter->EnumOutputs( iOutput, &pOutput );
				if( FAILED( hr ) ) // DXGIERR_NOT_FOUND is expected when the end of the list is hit
					break;

				DXGI_OUTPUT_DESC outputDesc;
				ZeroMemory( &outputDesc, sizeof( DXGI_OUTPUT_DESC ) );
				if( SUCCEEDED( pOutput->GetDesc( &outputDesc ) ) )
				{
					if( hMonitor == outputDesc.Monitor )
						bFoundMatchingAdapter = true;

				}
				SAFE_RELEASE( pOutput );
			}

			if( bFoundMatchingAdapter )
			{
				DXGI_ADAPTER_DESC desc;
				ZeroMemory( &desc, sizeof( DXGI_ADAPTER_DESC ) );
				if( SUCCEEDED( pAdapter->GetDesc( &desc ) ) )
				{
					bGotMemory = true;
					*pDedicatedVideoMemory = desc.DedicatedVideoMemory;
					*pDedicatedSystemMemory = desc.DedicatedSystemMemory;
					*pSharedSystemMemory = desc.SharedSystemMemory;
				}
			}

			SAFE_RELEASE(pAdapter);
			if(bFoundMatchingAdapter)
				break;
		}

		SAFE_RELEASE(pDXGIFactory);

		FreeLibrary( hDXGI );
	}

	if( bGotMemory )
		return S_OK;
	else
		return E_FAIL;
}

/**
 *	@return The current value of the CPU's time stamp counter.
 *		Note that the meaning of this is model-specific. On modern CPUs it
 *		measures time; the frequency at which it increments will not change
 *		as the CPU throttles. On older CPUs it might measure clock cycles, but
 *		on some such older systems it's not consistent between cores/sockets.
 */
int64 GraphicsSettingsAutoDetector::GetTicks()
{
	return static_cast<int64>(__rdtsc());
};

GraphicsSettingsAutoDetector::GraphicsSettingsAutoDetector() :
	bPrintSettings(false)
{

}

GraphicsSettingsAutoDetector::~GraphicsSettingsAutoDetector()
{

}


#define MEMORY_TO_ALLOC 10000000
int64 GraphicsSettingsAutoDetector::GetRAMFrequency(int cnt)
{
	double averageFreq = 0.0;
	void* adr = malloc(MEMORY_TO_ALLOC);
	for(int i = 0; i < cnt; ++i)
	{
		int64 t = GetTicks();
		memset(adr, 7, MEMORY_TO_ALLOC);
		t = GetTicks() - t;
		averageFreq += ((double)t / ((double)GetCPUFrequency()*0.001f));
		Sleep(100);
	}
	free(adr);
	return (int64)((double)(MEMORY_TO_ALLOC * cnt) / (averageFreq*1850));
}

int64 GraphicsSettingsAutoDetector::getVideoMemoryViaWMI()
{
	try
	{
		IWbemServices* pSvc = NULL;
		IWbemLocator* pLoc = NULL;
		HRESULT hres = InitWMI(&pSvc, &pLoc);
		if(SUCCEEDED(hres))
		{
			// Step 6: --------------------------------------------------
			// Use the IWbemServices pointer to make requests of WMI ----

			// For example, get the name of the operating system
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = pSvc->ExecQuery(
				bstr_t("WQL"), 
				bstr_t("SELECT * FROM Win32_VideoController"),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
				NULL,
				&pEnumerator);

			if (FAILED(hres))
			{
				pSvc->Release();
				pLoc->Release();
				CoUninitialize();
				return 0;               // Program has failed.
			}

			// Step 7: -------------------------------------------------
			// Get the data from the query in step 6 -------------------
			int64 res = 0;
			IWbemClassObject *pclsObj = NULL;
			ULONG uReturn = 0;
			while (pEnumerator)
			{
				HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
					&pclsObj, &uReturn);
				if(0 == uReturn)
				{
					break;
				}

				VARIANT vtProp = {};

				// Get the name of the section
				hr = pclsObj->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
				if( SUCCEEDED(hr) )
				{
					res = static_cast<int64>(vtProp.intVal);
				}
				VariantClear(&vtProp);
				pclsObj->Release();

			}

			// Cleanup
			// ========

			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return res;   
		}
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}


bool GraphicsSettingsAutoDetector::GetDeviceEnumerator(IWbemServices *pSvc,
															IWbemLocator *pLoc, 
															BSTR wmiQuery, 
															IEnumWbemClassObject** pEnumerator)
{
	HRESULT hres = pSvc->ExecQuery(
		bstr_t("WQL"), 
		wmiQuery,
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
		NULL,
		pEnumerator);

	if (FAILED(hres))
	{
		return false;               // Program has failed.
	}
	return true;
}


void GraphicsSettingsAutoDetector::CheckEnclosureType(INT & result, VARIANT vtProp)
{
	if (vtProp.puiVal[0] >= 8 && vtProp.puiVal[0] <= 14)
		result += 1;
}

void GraphicsSettingsAutoDetector::CheckBatteryType(INT & result, VARIANT vtProp)
{
	if ( wcsstr(vtProp.bstrVal, L"UPS") == NULL )
		result += 1;
}

void GraphicsSettingsAutoDetector::CheckPhysMemoryType(INT & result, VARIANT vtProp)
{
	if ( vtProp.uintVal == 12 )
		result += 1;
}

INT GraphicsSettingsAutoDetector::GetDeviceObjectsInfo(IEnumWbemClassObject* pEnumerator, wchar_t* nameOfTheInfo, CHECK_FUNCTION myComparator)
{
	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;
	INT result = 0;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
			&pclsObj, &uReturn);
		if(uReturn > 0)
		{
			VARIANT vtProp;
			if (SUCCEEDED(pclsObj->Get( nameOfTheInfo, 0L, &vtProp, NULL, NULL )))
			{
				myComparator(result, vtProp);
			}
			pclsObj->Release();
		}
		else
			break;
	}
	return result;
}


bool GraphicsSettingsAutoDetector::IsNotebook()
{
	HRESULT hres;
	try
	{
		IWbemServices* pSvc = NULL;
		IWbemLocator* pLoc = NULL;
		hres = InitWMI(&pSvc, &pLoc);
		if(SUCCEEDED(hres))
		{
			// Step 6: --------------------------------------------------
			// Use the IWbemServices pointer to make requests of WMI ----

			// For example, get the name of the operating system
			IEnumWbemClassObject* pEnumerator = NULL;
			if ( !GetDeviceEnumerator(pSvc, pLoc, bstr_t("SELECT * FROM Win32_SystemEnclosure"), &pEnumerator) )
			{
				pSvc->Release();
				pLoc->Release();
				CoUninitialize();
				return false;
			}

			// Step 7: -------------------------------------------------
			// Get the data from the query in step 6 -------------------
			bool res = false;
			INT result = 0;
			if (GetDeviceObjectsInfo(pEnumerator, L"ChassisTypes", CheckEnclosureType ) > 0)
				result += 1;

			pEnumerator->Release(), pEnumerator = NULL;
			if ( GetDeviceEnumerator(pSvc, pLoc, bstr_t("SELECT * FROM Win32_Battery"), &pEnumerator) )
			{
				if (GetDeviceObjectsInfo(pEnumerator, L"Name", CheckBatteryType ) > 0)
					result += 1;
				pEnumerator->Release(), pEnumerator = NULL;
			}

			if ( GetDeviceEnumerator(pSvc, pLoc, bstr_t("SELECT * FROM Win32_PhysicalMemory"), &pEnumerator) )
			{
				if (GetDeviceObjectsInfo(pEnumerator, L"FormFactor", CheckPhysMemoryType ) > 0)
					result += 1;
				pEnumerator->Release(), pEnumerator = NULL;
			}
			if (result >= 1)
				res = true;

			// Cleanup
			// ========

			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return res;   
		}
		return false;
	}
	catch(...)
	{
		return false;
	}
}

HRESULT GraphicsSettingsAutoDetector::InitWMI( IWbemServices **pSvc, IWbemLocator **pLoc )
{
	HRESULT hres;
	try
	{
		// Step 1: --------------------------------------------------
		// Initialize COM. ------------------------------------------

		//We need COM initialization. Second parameter should be COINIT_APARTMENTTHREADED because of 
		//calling that method in ImeUi.cpp
		CoInitializeEx(0, COINIT_APARTMENTTHREADED); 

		// Step 2: --------------------------------------------------
		// Set general COM security levels --------------------------
		// Note: If you are using Windows 2000, you need to specify -
		// the default authentication credentials for a user by using
		// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
		// parameter of CoInitializeSecurity ------------------------

		hres =  CoInitializeSecurity(
			NULL, 
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities 
			NULL                         // Reserved
			);


		//if (FAILED(hres))
		//{
		//	CoUninitialize();
		//	return hres;                    // Program has failed.
		//}

		// Step 3: ---------------------------------------------------
		// Obtain the initial locator to WMI -------------------------
		*pLoc = NULL;

		hres = CoCreateInstance(
			CLSID_WbemLocator,             
			0, 
			CLSCTX_INPROC_SERVER, 
			IID_IWbemLocator, (LPVOID *) pLoc);

		if (FAILED(hres))
		{
			CoUninitialize();
			return hres;                 // Program has failed.
		}

		// Step 4: -----------------------------------------------------
		// Connect to WMI through the IWbemLocator::ConnectServer method

		*pSvc = NULL;

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		hres = (*pLoc)->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (e.g. Kerberos)
			0,                       // Context object 
			pSvc					 // pointer to IWbemServices proxy
			);

		if (FAILED(hres))
		{
			(*pLoc)->Release();     
			CoUninitialize();
			return hres;                // Program has failed.
		}

		// Step 5: --------------------------------------------------
		// Set security levels on the proxy -------------------------

		hres = CoSetProxyBlanket(
			*pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name 
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities 
			);

		if (FAILED(hres))
		{
			(*pSvc)->Release();
			(*pLoc)->Release();     
			CoUninitialize();
			return hres;               // Program has failed.
		}
		return hres;
	}
	catch(...)
	{
		return E_FAIL;
	}
}

__int64 GraphicsSettingsAutoDetector::getTotalVirtualMemory()
{
	MEMORYSTATUSEX ms;
	ZeroMemory( &ms, sizeof( ms ) );
	ms.dwLength = sizeof( ms );
	GlobalMemoryStatusEx( &ms );
	return ms.ullTotalVirtual / ( 1024 * 1024 );
}

/*static*/ int64 GraphicsSettingsAutoDetector::getTotalVideoMemory()
{
	return getVideoMemory() / (1024 * 1024);
}

GraphicsSettingsAutoDetector::SystemPerformance GraphicsSettingsAutoDetector::CheckNvidiaGPUId( const char* deviceId, SystemPerformance curPerformance )
{
	if(!deviceId)
		return curPerformance;

	if(!bounds[curPerformance].deviceInfo[NVIDIA].need_check)
	{
		if(curPerformance == MAX)
		{
			return MAX;
		}
		else
		{
			return CheckNvidiaGPUId(deviceId, (SystemPerformance)(curPerformance - 1));
		}
	}

	const char* device = deviceId;
	bool isNumber = false;
	int number = 0;
	while(true)
	{
		number = strtol(device, NULL, 10);
		if(number != 0)
			break;
		if((device = strstr(device, " ")) == 0)
			break;
		device+=1;
	}
	if(number != 0 && number != LONG_MAX && number != LONG_MIN)
	{
		if(number>=100 && number <= 1000)
			number *= 100;

		if( bounds[curPerformance].deviceInfo[NVIDIA].number < number )
			return (curPerformance == MAX ? MAX : CheckNvidiaGPUId(device, (SystemPerformance)(curPerformance - 1)));
	}

	return (curPerformance < MIN) ? (SystemPerformance)(curPerformance + 1) : (curPerformance);
}

GraphicsSettingsAutoDetector::SystemPerformance GraphicsSettingsAutoDetector::CheckAtiGPUId( const char* deviceId, SystemPerformance curPerformance /*= MIN*/ )
{
	if(!deviceId)
		return curPerformance;

	if(!bounds[curPerformance].deviceInfo[ATI].need_check)
	{
		if(curPerformance == MAX)
		{
			return MAX;
		}
		else
		{
			return CheckAtiGPUId(deviceId, (SystemPerformance)(curPerformance - 1));
		}
	}

	const char* device = deviceId;
	bool isNumber = false;
	int number = 0;
	int multiplier = 1;
	while(true)
	{
		if(device[0] == 'X')
		{
			multiplier = 100;
			device++;
		}
		else if(device[0] == 'H' && device[1] == 'D')
		{
			multiplier = 10000;
			device+=2;
		}
		number = strtol(device, NULL, 10);
		if(number != 0)
			break;
		if((device = strstr(device, " ")) == 0)
			break;
		device+=1;
	}
	number *= multiplier;
	if(number != 0 && number != LONG_MAX && number != LONG_MIN)
	{
		if( bounds[curPerformance].deviceInfo[ATI].number < number )
			return (curPerformance == MAX ? MAX : CheckAtiGPUId(deviceId, (SystemPerformance)(curPerformance - 1)));
	}

	return (curPerformance < MIN) ? (SystemPerformance)(curPerformance + 1) : (curPerformance);
}

GraphicsSettingsAutoDetector::SystemPerformance GraphicsSettingsAutoDetector::CheckIntelGPUId( const char* deviceId, SystemPerformance curPerformance /*= MIN*/ )
{
	if(!deviceId)
		return curPerformance;

	if(!bounds[curPerformance].deviceInfo[INTEL].need_check)
	{
		if(curPerformance == MAX)
		{
			return MAX;
		}
		else
		{
			return CheckIntelGPUId(deviceId, (SystemPerformance)(curPerformance - 1));
		}
	}

	const char* device = deviceId;
	bool isNumber = false;
	int number = 0;
	int multiplier = 1;
	while(true)
	{
		if(device[0] == 'G' && device[1] == 'M' && device[2] == 'A')
		{
			multiplier = 100;
			device+=3;
		}
		else if(device[0] == 'H' && device[1] == 'D')
		{
			multiplier = 10000;
			device+=2;
		}
		number = strtol(device, NULL, 10);
		if(number != 0)
			break;
		if((device = strstr(device, " ")) == 0)
			break;
		device+=1;
	}
	number *= multiplier;
	if(number != 0 && number != LONG_MAX && number != LONG_MIN)
	{
		if( bounds[curPerformance].deviceInfo[INTEL].number < number )
			return (curPerformance == MAX ? MAX : CheckIntelGPUId(deviceId, (SystemPerformance)(curPerformance - 1)));
	}

	return (curPerformance < MIN) ? (SystemPerformance)(curPerformance + 1) : (curPerformance);
}

GraphicsSettingsAutoDetector::SystemPerformance GraphicsSettingsAutoDetector::lastDetection = GraphicsSettingsAutoDetector::PERF_LEVEL_COUNT;

} // namespace Moo

BW_END_NAMESPACE

// graphics_settings_picker.cpp
