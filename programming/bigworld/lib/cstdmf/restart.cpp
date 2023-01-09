#include "pch.hpp"
#ifndef _XBOX360
	#include "cstdmf/cstdmf_windows.hpp"
#endif
#include "cstdmf/bw_vector.hpp"
#include <sstream>
#include "restart.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

#ifndef _XBOX360

static BW::wstring s_commandLine = GetCommandLine();
static const BW::wstring s_commandLineToken = L"/wait_for_process:";


/*********************************************************************
 * This function will return immediately if the application is not
 * started by calling restart. Otherwise it will wait until the previous
 * application is ended.
*********************************************************************/
void waitForRestarting()
{
	BW::string::size_type off = s_commandLine.find( s_commandLineToken.c_str() );
	if (off != s_commandLine.npos)
	{
		DWORD pid = (DWORD)_wtoi64( s_commandLine.c_str() + off + s_commandLineToken.size() );
		HANDLE ph = OpenProcess( SYNCHRONIZE, FALSE, pid );
		if (ph)
		{
			WaitForSingleObject( ph, INFINITE );
			CloseHandle( ph );
		}
		s_commandLine.resize( off );
	}
}


/*********************************************************************
 * This function start a new instance of this application with the
 * exact paramter and environment. This function won't terminate the
 * current process and it is the caller's responsibility to do that.
*********************************************************************/
void startNewInstance()
{
	BW::vector<wchar_t> exeName( 1024 );
	while (GetModuleFileName( NULL, &exeName[0], ( DWORD )exeName.size() ) + 1 > exeName.size())
	{
		exeName.resize( exeName.size() * 2 );
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	GetStartupInfo( &si );

	BW::wostringstream oss;
	oss << s_commandLine << ' ' << s_commandLineToken << GetCurrentProcessId();

	BW::wstring str = oss.str();
	BW::vector<wchar_t> commandLine( str.begin(), str.end() );
	commandLine.push_back( 0 );

	CreateProcess( NULL, &commandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
}

BW_END_NAMESPACE

#endif


