#include "pch.hpp"
#include "wm_plugin_shell.hpp"

#include "bw_height_import.hpp"


/*
 *	Set up global access variable, this is required by world machine
 */
void SetupGlobalAccess(WMGlobalStruc *global) 
{
	WMGlobal = global;
};


/*
 *	Get the header version this plugin is built against, again required by world machine
 */
int  GetHeaderVersion() 
{
	return WM_PLUGIN_HEADER_VERSION;
};


/*
 *	This function is used to enumerate the devices in this plugin
 */
bool GetDevPlugin(const int i, DevPluginStruc *result) 
{
	if (!result)
	{
		return false;
	}

	DevPluginStruc dat;

	if (i == 0)
	{
		Device *dev =  new BWHeightImport;
		dat.lifedata = dev->GetLifeVars();
		strcpy_s(dat.name, dev->GetTypeName());
		dat.type = TYPE_GENERATOR;
		delete dev;
		*result = dat;
		return true;
	}

	return false;
};
