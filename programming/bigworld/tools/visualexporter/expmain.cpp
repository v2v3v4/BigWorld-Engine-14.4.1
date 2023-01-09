#include "mfxexp.hpp"

#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

//Global variables.

BW_BEGIN_NAMESPACE
HINSTANCE hInstance;
int controlsInit = FALSE;

//Global functions
extern ClassDesc* GetMFXExpDesc();

#ifdef BW_EXPORTER
extern char* g_exporterErrorMsg;
#endif
BW_END_NAMESPACE

/*
 * The dll entrypoint function
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	BW_NAMESPACE hInstance = hinstDLL;

	// Initialise the custom controls. This should be done only once.
	if (!BW_NAMESPACE controlsInit)
	{
		BW_NAMESPACE controlsInit = TRUE;
#ifndef MAX_RELEASE_R10 // GetEffectFilename was deprecated in max 2008
		InitCustomControls(hInstance);
#endif
		InitCommonControls();

		// Add the tools res directory
#ifdef _UNICODE
		WCHAR buffer[2048];
		WCHAR currentDir[1000];

		GetModuleFileName( hinstDLL, buffer, sizeof( buffer ) );
		GetCurrentDirectory( 1000, currentDir );

		BW::string toolsPath = BW_NAMESPACE bw_wtoutf8( buffer );
		BW::string exporterPath = BW_NAMESPACE bw_wtoutf8( currentDir );
#else
		char buffer[2048];
		char currentDir[1000];

		GetModuleFileName( hinstDLL, buffer, sizeof( buffer ) );
		GetCurrentDirectory( 1000, currentDir );

		BW::string toolsPath = buffer;
		BW::string exporterPath = currentDir;
#endif

		

		BW::string::size_type pos = toolsPath.find_last_of( "\\" );
		if (pos != BW::string::npos)
		{
			toolsPath = toolsPath.substr( 0, pos );
			exporterPath = toolsPath;
			pos = toolsPath.find_last_of( "\\" );
			if (pos != BW::string::npos)
			{
				toolsPath = toolsPath.substr( 0, pos );
				// Check we aren't running from the 3dsmax directory
				// we really want to be in bigworld/tools/exporter
				if (toolsPath.find( "3dsmax" ) != BW::string::npos)
				{
#ifdef _UNICODE
					MessageBox( GetForegroundWindow(),
						L"Exporter must be run from bigworld/tools/exporter/***, "
						L"not copied into the 3dsmax plugins folder\n",
						L"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#else
					MessageBox( GetForegroundWindow(),
						"Exporter must be run from bigworld/tools/exporter/***, "
						"not copied into the 3dsmax plugins folder\n",
						"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#endif
					return FALSE;
				}

				pos = toolsPath.find_last_of( "\\" );
				if (pos != BW::string::npos)
					toolsPath = toolsPath.substr( 0, pos );
			}
		}

#ifdef _UNICODE
		SetCurrentDirectory( bw_utf8tow( exporterPath ).c_str() );
#else
		SetCurrentDirectory( exporterPath.c_str() );
#endif
		BW_NAMESPACE BWResource::init( exporterPath + "\\", false );
		if ( BW_NAMESPACE g_exporterErrorMsg )
		{
#ifdef _UNICODE
			MessageBox( GetForegroundWindow(), BW_NAMESPACE bw_utf8tow( BW_NAMESPACE g_exporterErrorMsg ).c_str(),
				L"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#else
			MessageBox( GetForegroundWindow(), BW_NAMESPACE g_exporterErrorMsg,
				"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#endif
			
			return FALSE;
		}
		SetCurrentDirectory( currentDir );
	}

	// Don't write out xml attributes in the exporter as we want to remain compatible with
	// older versions of BW
	BW_NAMESPACE XMLSection::shouldWriteXMLAttributes( false );

	return TRUE;
}

/*
 * Returns the description of the library (stored in the string table)
 */
__declspec( dllexport ) const TCHAR* LibDescription() 
{
	return BW_NAMESPACE GetString(IDS_BWVE_LIBDESCRIPTION);
}

/*
 * Returns the number of classes in the Library.
 */
__declspec( dllexport ) int LibNumberClasses() 
{
	return 1;
}

/*
 * Returns the class description for the requested class.
 */
__declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i) 
	{
	case 0: 
		return BW_NAMESPACE GetMFXExpDesc();
	default: 
		return 0;
	}
}

/*
 * Returns the library version. (defined in mfxexp.hpp)
 */
__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX;
}

BW_BEGIN_NAMESPACE
	
/*
 * The class description class.
 */
class MFXExpClassDesc:public ClassDesc {
public:
	int				IsPublic() { return 1; }
	void*			Create(BOOL loading = FALSE) { return new BW_NAMESPACE MFXExport; } 
	const TCHAR*	ClassName() { return GetString(IDS_MFEXP); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; } 
	Class_ID		ClassID() { return MFEXP_CLASS_ID; }
	const TCHAR*	Category() { return GetString(IDS_CATEGORY); }
};

/*
 * The class description instance.
 */
static MFXExpClassDesc MFXExpDesc;
	
/*
 * Returns the class description class for the mfx exporter.
 */
ClassDesc* GetMFXExpDesc()
{
	return &MFXExpDesc;
}

/*
 * Gets a string from the resource string table.
 */
TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}

BW_END_NAMESPACE

// expmain.cpp
