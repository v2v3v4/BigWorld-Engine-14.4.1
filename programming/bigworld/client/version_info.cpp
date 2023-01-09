#include "pch.hpp"

#define INITGUID

#include "version_info.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/cstdmf_windows.hpp"

#include <windowsx.h>
#include <basetsd.h>

//#include <ddraw.h>
//#include <d3d8.h>
//#include <dinput.h>
//#include <dmusici.h>

/*typedef HRESULT(WINAPI * DIRECTDRAWCREATE)( GUID*, LPDIRECTDRAW*, IUnknown* );
typedef HRESULT(WINAPI * DIRECTDRAWCREATEEX)( GUID*, VOID**, REFIID, IUnknown* );
typedef HRESULT(WINAPI * DIRECT3DCREATE8)( UINT );*/
//typedef HRESULT(WINAPI * DIRECTINPUTCREATE)( HINSTANCE, DWORD, LPDIRECTINPUT*,
//                                             IUnknown* );



DECLARE_DEBUG_COMPONENT2( "App", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "version_info.ipp"
#endif

VersionInfo::VersionInfo()
:lastMemoryCheck_( 0 ),
 adapterDriver_( "" ),
 adapterDesc_( "" ),
 adapterDriverMajorVer_( 0 ),
 adapterDriverMinorVer_( 0 ),
 loadedDll_( NULL ),
 vi_getProcessMemoryInfo_( NULL )
{
	ZeroMemory( &processMemoryStatus_, sizeof( processMemoryStatus_ ) );
}


VersionInfo::~VersionInfo()
{
	if( loadedDll_ )
	{
		FreeLibrary( loadedDll_ );
	}
}


VersionInfo& VersionInfo::instance( void )
{
	static VersionInfo versionInfo;

	return versionInfo;
}


void
VersionInfo::queryAll( void )
{
	queryOS();
	queryDX();
}


/**
 *	This method queries the windows operating system for
 *	versioning information
 */
void
VersionInfo::queryOS( void )
{
	OSVERSIONINFO           osvi;

    // Initialise the OSVERSIONINFO structure.
    ZeroMemory( &osvi, sizeof( osvi ) );
    osvi.dwOSVersionInfoSize = sizeof( osvi );

    GetVersionEx( &osvi );

	osMajor_ = osvi.dwMajorVersion;
	osMinor_ = osvi.dwMinorVersion;

	if( osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
		osName_ = "Win9x";
	}
	else if( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		osName_ = "Winnt";
	}

	if( osvi.szCSDVersion )
		bw_wtoutf8( osvi.szCSDVersion, osServicePack_ );
	else
		osServicePack_ = "No service pack installed";
}


size_t
VersionInfo::totalPhysicalMemory( void ) const
{
	queryMemory();
	return memoryStatus_.dwTotalPhys;
}


size_t
VersionInfo::availablePhysicalMemory( void ) const
{
	queryMemory();
	return memoryStatus_.dwAvailPhys;
}


size_t
VersionInfo::totalVirtualMemory( void ) const
{
	queryMemory();
	return memoryStatus_.dwTotalVirtual;
}


size_t
VersionInfo::availableVirtualMemory( void ) const
{
	queryMemory();
	return memoryStatus_.dwAvailVirtual;
}


size_t
VersionInfo::totalPagingFile( void ) const
{
	queryMemory();
	return memoryStatus_.dwTotalPageFile;
}


size_t
VersionInfo::availablePagingFile( void ) const
{
	queryMemory();
	return memoryStatus_.dwAvailPageFile;
}

uint
VersionInfo::memoryLoad( void ) const
{
	queryMemory();
	return memoryStatus_.dwMemoryLoad;
}

uint VersionInfo::pageFaults( void ) const
{
	queryMemory();

	return processMemoryStatus_.PageFaultCount;
}

size_t VersionInfo::peakWorkingSet( void ) const
{
	queryMemory();
	return processMemoryStatus_.PeakWorkingSetSize;
}

size_t VersionInfo::workingSet( void ) const
{
	queryMemory();
	return processMemoryStatus_.WorkingSetSize;
}

size_t VersionInfo::quotaPeakedPagePoolUsage( void ) const
{
	queryMemory();
	return processMemoryStatus_.QuotaPeakPagedPoolUsage;	
}

size_t VersionInfo::quotaPagePoolUsage( void ) const
{
	queryMemory();
	return processMemoryStatus_.QuotaPagedPoolUsage;	
}

size_t	VersionInfo::quotaPeakedNonPagePoolUsage( void ) const
{
	queryMemory();
	return processMemoryStatus_.QuotaPeakNonPagedPoolUsage;
}

size_t	VersionInfo::quotaNonPagePoolUsage( void ) const
{
	queryMemory();
	return processMemoryStatus_.QuotaNonPagedPoolUsage;
}

size_t	VersionInfo::peakPageFileUsage( void ) const
{
	queryMemory();
	return processMemoryStatus_.PeakPagefileUsage;
}

size_t	VersionInfo::pageFileUsage( void ) const
{
	queryMemory();
	return processMemoryStatus_.PagefileUsage;
}

size_t VersionInfo::workingSetRefetched( void ) const
{
	lastMemoryCheck_ = 0;
	return this->workingSet();
}


void
VersionInfo::queryMemory( void ) const
{
	//Load the DLL, if we must
	if ( !loadedDll_ )
	{
		loadedDll_ = LoadLibrary( L"PSAPI.DLL" );

		//get pointers to dll functions
		vi_getProcessMemoryInfo_	= (VI_GETPROCESSMEMORYINFO)GetProcAddress( loadedDll_, VI_GETPROCESSMEMORYINFO_NAME );
	}


	unsigned long tickCount = ::GetTickCount();
	if ( ( tickCount - lastMemoryCheck_ ) > 1000 )
	{
		lastMemoryCheck_ = tickCount;

		int DIV = 1024;


		//Global memory status
		::GlobalMemoryStatus( &memoryStatus_ );

		//Convert to KBytes
		memoryStatus_.dwAvailPageFile /= DIV;
		memoryStatus_.dwAvailPhys /= DIV;
		memoryStatus_.dwAvailVirtual /= DIV;
		memoryStatus_.dwTotalPageFile /= DIV;
		memoryStatus_.dwTotalPhys /= DIV;
		memoryStatus_.dwTotalVirtual /= DIV;

		//Process memory status
		if ( vi_getProcessMemoryInfo_ )
		{
			vi_getProcessMemoryInfo_( ::GetCurrentProcess(), &processMemoryStatus_, sizeof( processMemoryStatus_ ) );

			//Convert to KBytes
			processMemoryStatus_.PeakWorkingSetSize /= DIV;
			processMemoryStatus_.WorkingSetSize /= DIV;
			processMemoryStatus_.QuotaPeakPagedPoolUsage /= DIV;
			processMemoryStatus_.QuotaPagedPoolUsage /= DIV;
			processMemoryStatus_.QuotaPeakNonPagedPoolUsage /= DIV;
			processMemoryStatus_.QuotaNonPagedPoolUsage /= DIV;
			processMemoryStatus_.PagefileUsage /= DIV;
			processMemoryStatus_.PeakPagefileUsage /= DIV;
		}

	}
}


/**
 *	This method queries DirectX for relevant driver information
 */
void
VersionInfo::queryDX( void )
{
	DWORD dxVer = queryDXVersion();

	switch( dxVer )
    {
        case 0x000:
            dxName_ = "No DirectX installed";
			dxMajor_ = 0;
			dxMinor_ = 0;
            break;
        case 0x100:
            dxName_ = "DirectX 1";
			dxMajor_ = 1;
			dxMinor_ = 0;
            break;
        case 0x200:
            dxName_ = "DirectX 2";
			dxMajor_ = 2;
			dxMinor_ = 0;
            break;
        case 0x300:
            dxName_ = "DirectX 3";
			dxMajor_ = 3;
			dxMinor_ = 0;
            break;
        case 0x500:
            dxName_ = "DirectX 5";
			dxMajor_ = 5;
			dxMinor_ = 0;
            break;
        case 0x600:
            dxName_ = "DirectX 6";
			dxMajor_ = 6;
			dxMinor_ = 0;
            break;
        case 0x601:
            dxName_ = "DirectX 6.1";
			dxMajor_ = 6;
			dxMinor_ = 1;
            break;
        case 0x700:
            dxName_ = "DirectX 7";
			dxMajor_ = 7;
			dxMinor_ = 0;
            break;
        case 0x800:
            dxName_ = "DirectX 8 or better";
			dxMajor_ = 8;
			dxMinor_ = 0;
            break;
        default:
            dxName_ = "Unknown version of DirectX";
			dxMajor_ = 0;
			dxMinor_ = 0;
            break;
    }
}


/**
 *	This method queries the hardware
 */
void
VersionInfo::queryHardware( void )
{
}


//-----------------------------------------------------------------------------
// Name: GetDXVersion()
// Desc: This function returns the DirectX version number as follows:
//          0x0000 = No DirectX installed
//          0x0100 = DirectX version 1 installed
//          0x0200 = DirectX 2 installed
//          0x0300 = DirectX 3 installed
//          0x0500 = At least DirectX 5 installed.
//          0x0600 = At least DirectX 6 installed.
//          0x0601 = At least DirectX 6.1 installed.
//          0x0700 = At least DirectX 7 installed.
//          0x0800 = At least DirectX 8 installed.
// 
//       Please note that this code is intended as a general guideline. Your
//       app will probably be able to simply query for functionality (via
//       QueryInterface) for one or two components.
//
//       Please also note:
//          "if( dwDXVersion != 0x500 ) return FALSE;" is VERY BAD. 
//          "if( dwDXVersion <  0x500 ) return FALSE;" is MUCH BETTER.
//       to ensure your app will run on future releases of DirectX.
//-----------------------------------------------------------------------------
DWORD VersionInfo::queryDXVersion()
{
//	return 8.0;
	return 0x0800;

    /*DIRECTDRAWCREATE     DirectDrawCreate   = NULL;
    DIRECTDRAWCREATEEX   DirectDrawCreateEx = NULL;
	DIRECT3DCREATE8		 Direct3DCreate8 = NULL;

//    DIRECTINPUTCREATE    DirectInputCreate  = NULL;
    HINSTANCE            hDDrawDLL          = NULL;
    HINSTANCE            hDInputDLL         = NULL;
    HINSTANCE            hD3D8DLL           = NULL;
    LPDIRECTDRAW         pDDraw             = NULL;
    LPDIRECTDRAW2        pDDraw2            = NULL;
    LPDIRECTDRAWSURFACE  pSurf              = NULL;
    LPDIRECTDRAWSURFACE3 pSurf3             = NULL;
    LPDIRECTDRAWSURFACE4 pSurf4             = NULL;
    DWORD                dwDXVersion        = 0;
    HRESULT              hr;

    // First see if DDRAW.DLL even exists.
    hDDrawDLL = LoadLibrary( "DDRAW.DLL" );
    if( hDDrawDLL == NULL )
    {
		OutputDebugString( "VersionInfo: Couldn't Find DDRAW.DLL\r\n" );
        dwDXVersion = 0;
        return dwDXVersion;
    }

    // See if we can create the DirectDraw object.
    DirectDrawCreate = (DIRECTDRAWCREATE)GetProcAddress( hDDrawDLL, "DirectDrawCreate" );
    if( DirectDrawCreate == NULL )
    {
        dwDXVersion = 0;
        FreeLibrary( hDDrawDLL );
        OutputDebugString( "VersionInfo: Couldn't GetProcAddress DirectDrawCreate\r\n" );
        return dwDXVersion;
    }

    hr = DirectDrawCreate( NULL, &pDDraw, NULL );
    if( FAILED(hr) )
    {
        dwDXVersion = 0;
        FreeLibrary( hDDrawDLL );
        OutputDebugString( "VersionInfo: Couldn't create DDraw\r\n" );
        return dwDXVersion;
    }

    // So DirectDraw exists.  We are at least DX1.
    dwDXVersion = 0x100;

    // Let's see if IID_IDirectDraw2 exists.
    hr = pDDraw->QueryInterface( IID_IDirectDraw2, (VOID**)&pDDraw2 );
    if( FAILED(hr) )
    {
        // No IDirectDraw2 exists... must be DX1
        pDDraw->Release();
        FreeLibrary( hDDrawDLL );
        OutputDebugString( "VersionInfo: Couldn't QI DDraw2\r\n" );
        return dwDXVersion;
    }

    // IDirectDraw2 exists. We must be at least DX2
    pDDraw2->Release();
    dwDXVersion = 0x200;


	//-------------------------------------------------------------------------
    // DirectX 3.0 Checks
	//-------------------------------------------------------------------------

    // DirectInput was added for DX3
    hDInputDLL = LoadLibrary( "DINPUT.DLL" );
    if( hDInputDLL == NULL )
    {
        // No DInput... must not be DX3
        OutputDebugString( "VersionInfo: Couldn't LoadLibrary DInput\r\n" );
        pDDraw->Release();
        return dwDXVersion;
    }*/

    /*DirectInputCreate = (DIRECTINPUTCREATE)GetProcAddress( hDInputDLL,
                                                        "DirectInputCreateA" );
    if( DirectInputCreate == NULL )
    {
        // No DInput... must be DX2
        FreeLibrary( hDInputDLL );
        FreeLibrary( hDDrawDLL );
        pDDraw->Release();
        OutputDebugString( "VersionInfo: Couldn't GetProcAddress DInputCreate\r\n" );
        return dwDXVersion;
    }*/

    // DirectInputCreate exists. We are at least DX3
    /*dwDXVersion = 0x300;
    FreeLibrary( hDInputDLL );

    // Can do checks for 3a vs 3b here


	//-------------------------------------------------------------------------
    // DirectX 5.0 Checks
	//-------------------------------------------------------------------------

    // We can tell if DX5 is present by checking for the existence of
    // IDirectDrawSurface3. First, we need a surface to QI off of.
    DDSURFACEDESC ddsd;
    ZeroMemory( &ddsd, sizeof(ddsd) );
    ddsd.dwSize         = sizeof(ddsd);
    ddsd.dwFlags        = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = pDDraw->SetCooperativeLevel( NULL, DDSCL_NORMAL );
    if( FAILED(hr) )
    {
        // Failure. This means DDraw isn't properly installed.
        pDDraw->Release();
        FreeLibrary( hDDrawDLL );
        dwDXVersion = 0;
        OutputDebugString( "VersionInfo: Couldn't Set coop level\r\n" );
        return dwDXVersion;
    }

    hr = pDDraw->CreateSurface( &ddsd, &pSurf, NULL );
    if( FAILED(hr) )
    {
        // Failure. This means DDraw isn't properly installed.
        pDDraw->Release();
        FreeLibrary( hDDrawDLL );
        dwDXVersion = 0;
        OutputDebugString( "VersionInfo: Couldn't CreateSurface\r\n" );
        return dwDXVersion;
    }

    // Query for the IDirectDrawSurface3 interface
    if( FAILED( pSurf->QueryInterface( IID_IDirectDrawSurface3,
                                       (VOID**)&pSurf3 ) ) )
    {
        pDDraw->Release();
        FreeLibrary( hDDrawDLL );
        return dwDXVersion;
    }

    // QI for IDirectDrawSurface3 succeeded. We must be at least DX5
    dwDXVersion = 0x500;


	//-------------------------------------------------------------------------
    // DirectX 6.0 Checks
	//-------------------------------------------------------------------------

    // The IDirectDrawSurface4 interface was introduced with DX 6.0
    if( FAILED( pSurf->QueryInterface( IID_IDirectDrawSurface4,
                                       (VOID**)&pSurf4 ) ) )
    {
        pDDraw->Release();
        FreeLibrary( hDDrawDLL );
        return dwDXVersion;
    }

    // IDirectDrawSurface4 was create successfully. We must be at least DX6
    dwDXVersion = 0x600;
    pSurf->Release();
    pDDraw->Release();*/


	//-------------------------------------------------------------------------
    // DirectX 6.1 Checks
	//-------------------------------------------------------------------------

    // Check for DMusic, which was introduced with DX6.1
    /*LPDIRECTMUSIC pDMusic = NULL;
    CoInitialize( NULL );
    hr = CoCreateInstance( CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER,
                           IID_IDirectMusic, (VOID**)&pDMusic );
    if( FAILED(hr) )
    {
        OutputDebugString( "VersionInfo: Couldn't create CLSID_DirectMusic\r\n" );
        FreeLibrary( hDDrawDLL );
        return dwDXVersion;
    }

    // DirectMusic was created successfully. We must be at least DX6.1
    dwDXVersion = 0x601;
    pDMusic->Release();
    CoUninitialize();*/


	//-------------------------------------------------------------------------
    // DirectX 7.0 Checks
	//-------------------------------------------------------------------------

    // Check for DirectX 7 by creating a DDraw7 object
    /*LPDIRECTDRAW7 pDD7;
    DirectDrawCreateEx = (DIRECTDRAWCREATEEX)GetProcAddress( hDDrawDLL,
                                                       "DirectDrawCreateEx" );
    if( NULL == DirectDrawCreateEx )
    {
        FreeLibrary( hDDrawDLL );
        return dwDXVersion;
    }

    if( FAILED( DirectDrawCreateEx( NULL, (VOID**)&pDD7, IID_IDirectDraw7,
                                    NULL ) ) )
    {
        FreeLibrary( hDDrawDLL );
        return dwDXVersion;
    }

    // DDraw7 was created successfully. We must be at least DX7.0
    dwDXVersion = 0x700;
    pDD7->Release();


	//-------------------------------------------------------------------------
    // DirectX 8.0 Checks
	//-------------------------------------------------------------------------

    // Simply see if D3D8.dll exists.
    hD3D8DLL = LoadLibrary( "D3D8.DLL" );
    if( hD3D8DLL == NULL )
    {
	    FreeLibrary( hDDrawDLL );
        return dwDXVersion;
    }

    // D3D8.dll exists. We must be at least DX8.0
    dwDXVersion = 0x800;

	// Get the video adapter codes
	Direct3DCreate8 = (DIRECT3DCREATE8)GetProcAddress( hD3D8DLL,
                                                       "Direct3DCreate8" );

	if ( Direct3DCreate8 )
	{
		LPDIRECT3D8 pD3D = NULL;

		pD3D = (LPDIRECT3D8)Direct3DCreate8(D3D_SDK_VERSION);

		if( pD3D != NULL)
		{
			D3DADAPTER_IDENTIFIER8 adapterInfo;
			if ( SUCCEEDED( pD3D->GetAdapterIdentifier( D3DADAPTER_DEFAULT, D3DENUM_NO_WHQL_LEVEL, &adapterInfo ) ) )
			{
				adapterDriverMajorVer_ = LOWORD( adapterInfo.DriverVersion.HighPart );
				adapterDriverMinorVer_ = HIWORD( adapterInfo.DriverVersion.LowPart );
				adapterDriver_ = adapterInfo.Driver;
				adapterDesc_ = adapterInfo.Description;
			}

			pD3D->Release();
		}
	}




	//-------------------------------------------------------------------------
    // End of checking for versions of DirectX 
	//-------------------------------------------------------------------------

    // Close open libraries and return
    FreeLibrary( hDDrawDLL );
    FreeLibrary( hD3D8DLL );

    return dwDXVersion;*/
}


std::ostream& operator<<(std::ostream& o, const VersionInfo& t)
{
	o << "VersionInfo\n";
	return o;
}

BW_END_NAMESPACE

/*version_info.cpp*/
