#ifdef _WIN32
/*
 *  Copyright (c) 1995 Microsoft Corporation
 *
 *  Module Name:  service.cpp
 *
 *  Abstract:   CService class implementation - CService is a
 *              pure, virtual base class which simplifies implemtation
 *              of an NT service
 *
 *  Author:     Craig Link (craigli)
 *
 *  Revision History:
 *              4/07/95    craigli     Created
 */


#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <windows.h>
#include <cassert>
#include "service.hpp"

#define ASSERT assert

// global service variable.  Used by static functions to ref the object
CService* g_pService = NULL;


// notifies the Service Control Manager (SCM) that progress is being made
// while the service is in a "pending" state
#ifdef __cplusplus
extern "C" {
#endif

BOOL ServiceIsStopping()
{
    if ( g_pService )
        return g_pService->isStopping();
    else
        return TRUE;
}

DWORD ServiceCheckpoint( DWORD dwWaitHint )
{
    if ( g_pService )
        return g_pService->Checkpoint( dwWaitHint );
    else
        return NO_ERROR;
}

    // notifies the SCM that an error has occcured, or the service has
    // entered a new state
DWORD ServiceUpdateStatus(
     DWORD dwState,
     DWORD dwWaitHint,
     DWORD dwError )
{
    if ( g_pService )
        return g_pService->UpdateStatus( dwState, dwWaitHint, dwError );
    else
        return NO_ERROR;
}

#ifdef __cplusplus
}
#endif


// The TRACE_SERVICE flags allows for tracing and debugging of the
// service code, before the service is "running".  A log file is
// created in the root of the %system_root% drive named
// <ServiceName>.log
#define TRACE_SERVICE 0
#if  TRACE_SERVICE
HANDLE g_hFile = INVALID_HANDLE_VALUE;
LPTSTR g_pServiceName = NULL;
#endif // TRACE_SERVICE





/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api void | CService::CService | Constructor for the CService class
 *
 * @parm LPTSTR | pServiceName | Internal name of the service to be stored
 * in the registry
 *
 * @parm LPTSTR | pServiceDisplayName = NULL | "Friendly" named of the service, as
 * displayed by the Services Applet and 'net start'.  If pServiceDisplay name is
 * NULL, it assumes the same string a pServiceName 
 *
 *
 * @rdesc None.
 *
 * @comm Constructs an instance of the object and initializes member variables.
 *
 * @xref CService::GetName CService::GetDisplayName
 */

CService::CService( LPTSTR pServiceName, LPTSTR pServiceDisplayName /* = NULL */, DWORD svcType /*= SERVICE_WIN32_OWN_PROCESS*/ ) :
    m_isService( FALSE ), m_isInteractive( FALSE ), m_neverService( FALSE ),
    m_StatusHandle( 0 ), m_dwControlsAccepted( SERVICE_ACCEPT_STOP ),
    dwArgc(0), lpArgv(NULL), m_pDispatchTable( NULL )
{
    // set name of service
    ASSERT( pServiceName );
    m_pServiceName = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*(lstrlen(pServiceName)+1) );
    lstrcpy( m_pServiceName, pServiceName );

    // is pServiceDisplayName was not provided, use pServiceName
    if ( pServiceDisplayName != NULL )
    {
        m_pServiceDisplayName = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*(lstrlen(pServiceDisplayName)+1) );
        lstrcpy( m_pServiceDisplayName, pServiceDisplayName );
    }
    else
    {
        m_pServiceDisplayName = m_pServiceName;
    }


    // initialize member data
    m_Status.dwServiceType = svcType;
    m_Status.dwCurrentState = SERVICE_START_PENDING;
    m_Status.dwControlsAccepted = 0;
    m_Status.dwWin32ExitCode = NO_ERROR;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 3000;

#if TRACE_SERVICE
    if ( g_hFile == INVALID_HANDLE_VALUE )
    {
        g_pServiceName = m_pServiceName;

        TCHAR szFileName[100];
        wsprintf( szFileName, TEXT("\\%s.log"), m_pServiceName );

        g_hFile = CreateFile( szFileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
        if ( g_hFile != INVALID_HANDLE_VALUE )
            SetFilePointer( g_hFile, 0, NULL, FILE_END );
    }
#endif // TRACE_SERVICE

}

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api void | CService::~CService | Destructor for the CService class
 *
 * @parm None
 *
 * @rdesc None.
 *
 * @comm Frees up the instance of the object
 *
 * @xref 
 */
CService::~CService()
{
#if TRACE_SERVICE
    if( ( g_hFile != INVALID_HANDLE_VALUE ) &&
        ( g_pServiceName == m_pServiceName ) )
        CloseHandle( g_hFile );
#endif // TRACE_SERVICE

    // check to see whether the use supply pServiceDisplayName on construction
    if ( m_pServiceDisplayName != m_pServiceName )
        HeapFree( GetProcessHeap(), 0, m_pServiceDisplayName );

    HeapFree( GetProcessHeap(), 0, m_pServiceName );
}

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api DWORD | CService::Checkpoint | Notifies the Service Control Manager (SCM)
 * that progress is being made while the service is in a "pending" state.
 *
 * @parm DWORD | dwWaitHint = 0 | The estimated time that will elapse before
 * the next ::Checkpoint or ::UpdateStatus.  If dwWaitHint equals 0, then the
 * value of the previous dwWaitHint is used.
 *
 * @rdesc NO_ERROR
 *        SetServiceStatus error codes ( Win32 API )
 *
 * @comm If the process is not running as a NT service, the function just returns.
 *
 * @xref CService::UpdateStatus SerServiceStatus(Win32)
 */
DWORD CService::Checkpoint( DWORD dwWaitHint /* = 0 */ )
{
    if ( m_isService )
    {

        if ( dwWaitHint != 0 )
            m_Status.dwWaitHint = dwWaitHint;

        m_Status.dwCheckPoint++;

        if ( !::SetServiceStatus(m_StatusHandle, &m_Status) ) 
            return ::GetLastError();
    }

    return(NO_ERROR);

}

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api DWORD | CService::UpdateStatus | Notifies the Service Control Manager (SCM)
 * that the service has changed state or encountered an error
 *
 * @parm DWORD | dwState = SERVICE_START_PENDING | The new state of the service
 *
 * @parm DWORD | dwWaitHint = 0 | The estimated time that will elapse before
 * the next ::Checkpoint or ::UpdateStatus
 *
 * @parm DWORD | dwError = NO_ERROR | The error that occurred within the service
 *
 * @rdesc NO_ERROR
 *        SetServiceStatus error codes ( Win32 API )
 *
 * @comm While the service is in a SERVICE_START_PENDING state, all accepted service
 * control functions are disabled.
 *
 * If reporting an error, dwState must either be SERVICE_STOP_PENDING or 
 * SERVCE_STOPPED
 *
 * If more than one error is reported, only the first error code is maintained
 *
 * If a service is reported as SERVICE_RUNNING or SERVICE_STOPPED, the checkpoint
 * is set to 0
 *
 * @xref CService::Checkpoint SetServiceStatus(Win32) SERVICE_STATUS(Win32)
 */
DWORD CService::UpdateStatus( DWORD dwState /* = SERVICE_START_PENDING */,
                              DWORD dwWaitHint /* = 0 */, 
                              DWORD dwError /* = NO_ERROR */ )
{

    ASSERT ( dwError == NO_ERROR ||
             ( ( dwState == SERVICE_STOP_PENDING ) || 
               ( dwState == SERVICE_STOPPED ) ) &&
             "Service reported an error, but was not stopping!" );

    if ( dwState == SERVICE_START_PENDING )
        m_Status.dwControlsAccepted = 0;
    else
        m_Status.dwControlsAccepted = m_dwControlsAccepted;

    if ( ( m_Status.dwCurrentState != SERVICE_STOP_PENDING ) &&
         ( dwState == SERVICE_STOP_PENDING ) )
    {
        m_Status.dwCheckPoint = (DWORD) -1; // will be inc. in CheckPoint()
    }
    m_Status.dwCurrentState = dwState;

    // set dwWaitHint here instead of Checkpoint() call so that we can
    // have a possible wait hint of 0
    m_Status.dwWaitHint = dwWaitHint; 

    if ( m_Status.dwWin32ExitCode == NO_ERROR )
        m_Status.dwWin32ExitCode = dwError;

    if ( ( dwState == SERVICE_RUNNING ) ||
         ( dwState == SERVICE_STOPPED ) )
    {
        m_Status.dwCheckPoint = (DWORD) -1; // will be inc. in CheckPoint()
    }

    return Checkpoint();

}

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api DWORD | CService::Start | Begins the real startup work of the service
 *
 * @parm int | argc | The argument count as passed into main
 *
 * @parm char** | argv | The argument list as passed into main
 *
 *
 * @rdesc NO_ERROR
 *        various Win32 error codes ( Service, Registery, Security )
 *
 * @comm The first task of Start is to determine whether the process is running
 * as a NT service or a console process, and whether it is interactive.  If the 
 * process is interactive, a console control handle is installed which causes
 * Ctrl+C to trigger CService::onStop instead of terminating the process.
 *
 * If the process is running as an NT service, control is passed of to the 
 * NT service control dispatcher which eventually tiggers CService::Main().  If
 * the process is not running as a service, Main is called directly.
 *
 * @xref CService::CheckSIDs CService::onStop CService::Main
 */
DWORD CService::Start( int argc, char** argv )
{
    // set global pointer to service for use by static functions
    ASSERT( g_pService == NULL ); // only allow one active service object per process
    g_pService = this;

    DWORD dwError = NO_ERROR;

#if TRACE_SERVICE
    {
        char szBuf[256];
        DWORD cbWritten;
        wsprintf( szBuf, "CService::Start\n" );
        WriteFile( g_hFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL );
    }
#endif // TRACE_SERVICE

    // determine if service is running as a console process
    dwError = CheckSIDs();
    if ( dwError == ERROR_CALL_NOT_IMPLEMENTED )  // Win95
        m_isInteractive = TRUE;

    if ( m_isInteractive )
    {
        ::SetConsoleCtrlHandler( control_handler, TRUE );
    }

	if ( m_isService )
    {
		SERVICE_TABLE_ENTRY dispatchTable[] =
        {
            { m_pServiceName, (LPSERVICE_MAIN_FUNCTION)service_main },
            { NULL, NULL }
        };

		if ( !m_pDispatchTable )
        {
            m_pDispatchTable = dispatchTable;
        }

		if ( !::StartServiceCtrlDispatcher(m_pDispatchTable) )
            return ::GetLastError();
	}
    else
    {
#ifdef UNICODE
        lpArgv = ::CommandLineToArgvW(::GetCommandLineW(), &(dwArgc) );
#else
        dwArgc = argc;
        lpArgv = argv;
#endif

        Main();
	}

    // reset global pointer to service
    ASSERT( g_pService );
    g_pService = NULL;

    return NO_ERROR;
}


BOOL CService::SetDispatchEntries( SERVICE_TABLE_ENTRY* pDispatch )
{
    if ( m_pDispatchTable )
        return FALSE;
    else
        m_pDispatchTable = pDispatch;
    return TRUE;
}

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api void | CService::StartWithExternalDispatch | Begins the real startup work of the service
 *
 * @parm DWORD | argc | The argument count as passed into service_main
 *
 * @parm LPTSTR* | argv | The argument list as passed into service_main
 *
 *
 * @rdesc NO_ERROR
 *        various Win32 error codes ( Service, Registery, Security )
 *
 *
 * @xref CService::CheckSIDs CService::onStop CService::service_main
 */
void CService::StartWithExternalDispatch( DWORD dwArgc, LPTSTR* lpArgv)
{
    // set global pointer to service for use by static functions
    ASSERT( g_pService == NULL ); // only allow one active service object per process
	ASSERT( m_neverService == FALSE );
    g_pService = this;

    m_isService = TRUE;
    CService::service_main( dwArgc, lpArgv );
}


void CService::RegisterExternalHandler( LPHANDLER_FUNCTION handler )
{
    // register our service control handler:
    m_StatusHandle = ::RegisterServiceCtrlHandler( GetName(), handler);
}


/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api virtual void | CService::Main | PURE VIRTUAL function which implements
 * the actual body of the process
 *
 * @parm None
 *
 * @rdesc None
 *
 * @comm Derived classes are responible for calling Checkpoint while this service
 * starting and stopping.  They are also responsible for calling UpdateStatus
 * with SERVICE_RUNNING when the service is actually running.  The function must 
 * also return after it has been signalled to stop
 * 
 * @xref CService::onStop CService::UpdateStatus CService::Checkpoint CService::Start
 */

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api virtual void | CService::onStop | PURE VIRTUAL function which implements
 * to code to signal CService::Main to stop
 *
 * @parm None
 *
 * @rdesc None
 *
 * @comm Derived classes are responible for setting the service status
 * to SERVICE_STOP_PENDING via UpdateStatus before returning.  Implementations
 * should also not take longer than 3 seconds before returning, otherwise, the
 * SCM will believe that the service is not responding to controls and timeout.
 *
 * Other service control handlers should be implemented with the same thoughts in mind.
 * In addition, a control handler may what to change the controls that the service
 * accepts via SetControlsAccepted
 * 
 * @xref CService::onStop CService::UpdateStatus CService::Checkpoint CService::Start
 * CService::SetControlsAccepted
 */

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api virtual void | CService::Main | PURE VIRTUAL function which implements
 * the actual body of the process
 *
 * @parm None
 *
 * @rdesc None
 *
 * @comm Derived classes are responible for calling Checkpoint while this service
 * starting and stopping.  They are also responsible for calling UpdateStatus
 * with SERVICE_RUNNING when the service is actually running.
 * 
 * @xref CService::onStop CService::SetControlsAccepted CService::UpdateStatus
 */

/*
 * @doc ALL INTERNAL CSERVICE
 *
 * @api static void | CService::service_main | Internal wrapper for CService::Main 
 * which is called by the service control dispatcher
 *
 * @parm DWORD | dwArgc | The argument count as passed to the service
 *
 * @parm LPTSTR* | lpArgv | The argaument list as passed to the service
 *
 *
 * @rdesc None
 *
 * @comm service_main first registers CService::service_ctrl() as the
 * service's control handler.  Once registered, CService::Main() is called.
 * Upon return, the service state is set to SERVICE_STOPPED.
 * 
 * @xref CService::onStop CService::Main CService::service_ctrl
 */
void WINAPI CService::service_main( DWORD dwArgc, LPTSTR* lpArgv )
{
    ASSERT( g_pService );
    ASSERT( g_pService->m_isService );

#if TRACE_SERVICE
    {
        char szBuf[256];
        DWORD cbWritten;
        wsprintf( szBuf, "Beginning CService::service_main %s\n", g_pService->m_pServiceName  );
        WriteFile( g_hFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL );
    }
#endif // TRACE_SERVICE

    // register our service control handler:
    g_pService->m_StatusHandle = ::RegisterServiceCtrlHandler( g_pService->m_pServiceName,
                                                               CService::service_ctrl);
    if ( !g_pService->m_StatusHandle )
        return;

#if TRACE_SERVICE
    {
        char szBuf[256];
        DWORD cbWritten;
        wsprintf( szBuf, "Service successfully registered\n" );
        WriteFile( g_hFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL );
    }
#endif // TRACE_SERVICE

    g_pService->Checkpoint( 3000 );

    g_pService->dwArgc = dwArgc;
    g_pService->lpArgv = lpArgv;
    g_pService->Main();

    g_pService->UpdateStatus( SERVICE_STOPPED );

#if TRACE_SERVICE
    {
        char szBuf[256];
        DWORD cbWritten;
        wsprintf( szBuf, "Ending CService::service_main\n" );
        WriteFile( g_hFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL );
    }
#endif // TRACE_SERVICE

}

/*
 * @doc ALL INTERNAL CSERVICE
 *
 * @api static void | CService::service_ctrl | Internal wrapper for the service
 * control functions
 *
 * @parm DWORD | dwCtrlCode | The service control code requiring handling
 *
 *
 * @rdesc None
 *
 * @comm This functions just passes the control code of the the various onXXX service
 * control handlers.  All service control handlers are responsible for changing
 * state via UpdateStatus or changing the accepted service control list via
 * SetControlsAccepted themselves.
 * 
 * @xref CService::onStop CService::SetControlsAccepted CService::UpdateStatus
 */
void WINAPI CService::service_ctrl( DWORD dwCtrlCode )
{
    ASSERT( g_pService );
    g_pService->ServiceCtrl(dwCtrlCode);
}

void WINAPI CService::ServiceCtrl( DWORD dwCtrlCode )

{

    switch( dwCtrlCode )
    {
        case SERVICE_CONTROL_STOP:
            onStop();
            break;

        case SERVICE_CONTROL_PAUSE:
            onPause();
            break;

        case SERVICE_CONTROL_CONTINUE:
            onContinue();
            break;

        case SERVICE_CONTROL_INTERROGATE:
            onInterrogate();
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            onShutdown();
            break;

        default:
            onUserControl( dwCtrlCode );
            break;
    }

    // must always call SetServiceStatus for a Ctrl Request
    // The handle is responsible for setting states, etc
    ::SetServiceStatus(m_StatusHandle, &(m_Status));
}



///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//

/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api DWORD | CService::Install | Installs the process as an NT service
 *
 * @parm LPTSTR | pDependencies | The list of services upon which this service
 * is dependent.
 *
 *
 * @rdesc NO_ERROR on success, otherwise
 *        various Win32 errors ( Registry, Service )
 *
 * @comm If the service is already installed, a error will occur.
 *
 * The pDependencies string may either be a double NULL terminated string as
 * defined by the Win32 SDK, or it may consist a single NULL terminated string
 * of the following syntax which is translated into a double NULL terminated
 * string.
 *   For imbedded NULL place a '~' character in the string
 *   For SC_GROUP_IDENTIFIER characters, place '+' in the string
 * 
 * @xref CService::CService CService::Remove
 */
DWORD CService::Install(LPTSTR pDependencies /* = NULL */,
                        DWORD  svcType /* = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS */,
                        LPTSTR pszPath /*= NULL*/ )
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    TCHAR szPath[512];

    SetLastError( NO_ERROR );

    if ( pszPath == NULL )
    {
        if ( GetModuleFileName( NULL, szPath, 512 ) == 0 )
        {
            TCHAR szBuf[256];
            DWORD bytes;
            wsprintf( szBuf, TEXT("Unable to install %s - 0x%x\n"), m_pServiceDisplayName, GetLastError() );
            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
            return ::GetLastError();
        }
        pszPath = szPath;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        LPTSTR psz = pDependencies;
        while ( psz && *psz )
        {
            if ( *psz == TEXT('+') )
                *psz = SC_GROUP_IDENTIFIER;
            else if ( *psz == TEXT('~') )
                *psz = TEXT('\0');
            psz++;
        }

        schService = CreateService(
            schSCManager,               // SCManager database
            m_pServiceName,             // name of service
            m_pServiceDisplayName,      // name to display
            SERVICE_ALL_ACCESS,         // desired access
            svcType,                    // service type
            SERVICE_AUTO_START,         // start type
            SERVICE_ERROR_NORMAL,       // error control type
            pszPath,                    // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            pDependencies,              // dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if ( schService )
        {
			SERVICE_DESCRIPTION desc;
			desc.lpDescription = "BigWorld Daemon";
			ChangeServiceConfig2( schService, SERVICE_CONFIG_DESCRIPTION, &desc );

            TCHAR szBuf[256];
            DWORD bytes;

			int r = StartService( schService, 0, NULL );
			if ( r != 0 )
			{
	            wsprintf( szBuf, TEXT("%s installed and started.\n"), m_pServiceDisplayName  );
			}
			else
			{
	            wsprintf( szBuf, TEXT("%s installed but startup failed.\n"), m_pServiceDisplayName  );
			}

            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );

			CloseServiceHandle(schService);
        }
        else
        {
            TCHAR szBuf[256];
            DWORD bytes;
            wsprintf( szBuf, TEXT("CreateService failed - 0x%x\n"), GetLastError() );
            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
        }

        CloseServiceHandle(schSCManager);
    }
    else
    {
        TCHAR szBuf[256];
        DWORD bytes;
        wsprintf( szBuf, TEXT("OpenSCManager failed - 0x%x\n"), GetLastError() );
        WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
    }
    return ::GetLastError();
}



/*
 * @doc ALL EXTERNAL CSERVICE
 *
 * @api DWORD | CService::Remove | Unregisters the process as an NT service
 *
 * @parm None
 *
 * @rdesc NO_ERROR on success, otherwise
 *        various Win32 errors ( Registry, Service )
 *
 * @comm If the service is running, Remove will attempt to stop the service
 * before unregistering it.
 *
 * @xref CService::CService CService::Install
 */
DWORD CService::Remove()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR szBuf[256];
    DWORD bytes;

    SetLastError(NO_ERROR);

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, m_pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &m_Status ) )
            {

                wsprintf( szBuf, TEXT("Stopping %s."), m_pServiceDisplayName  );
                WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
                Sleep( 500 );

                while( QueryServiceStatus( schService, &m_Status ) )
                {
                    if ( m_Status.dwCurrentState == SERVICE_STOP_PENDING )
                    {
                        lstrcpy( szBuf, TEXT(".") );
                        WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
                        Sleep( 500 );
                    }
                    else
                        break;
                }

                if ( m_Status.dwCurrentState == SERVICE_STOPPED )
                    wsprintf( szBuf, TEXT("\n%s stopped.\n"), m_pServiceDisplayName );
                else
                    wsprintf( szBuf, TEXT("\n%s failed to stop.\n"), m_pServiceDisplayName );

                WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );

            }

            // now remove the service
            if( DeleteService(schService) )
                wsprintf( szBuf, TEXT("%s removed.\n"), m_pServiceDisplayName );
            else
                wsprintf( szBuf, TEXT("DeleteService failed - 0x%x\n"), GetLastError());

            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );

            CloseServiceHandle(schService);
        }
        else
        {
            wsprintf( szBuf, TEXT("OpenService failed - 0x%x\n"), GetLastError());
            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
        }
        CloseServiceHandle(schSCManager);
    }
    else
    {
        wsprintf( szBuf, TEXT("OpenSCManager failed - 0x%x\n"), GetLastError());
        WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
    }
    return ::GetLastError();

}


/*
 * @doc ALL INTERNAL CSERVICE
 *
 * @api DWORD | CService::CheckSIDs | Determines whether a process is running
 * as an NT service or console process.  Also determines whether or not the
 * process has interactive access
 *
 * @parm None
 *
 * @rdesc NO_ERROR on success, otherwise
 *        various Win32 errors ( Security, Service )
 *
 * @comm CService::CheckSIDs() determines whether the current process is executing
 * as a console process or a service by enumerating the process token.  If the
 * process token contains a SECURITY_INTERACTIVE_RID SID, then the process is running
 * as a console process.  If the token contains a SECURITY_SERVICE_RID SID, then the 
 * process is running as a service in a user account.  If neither of these cases are
 * true, then the process is running as a service in the LOCAL_SYSTEM account.  If
 * the service is running under the LOCAL_SYSTEM account, it may have access to the
 * desktop which is determined by checking the service's service_type for
 * SERVICE_INTERACTIVE_PROCESS.  If the service is interactive, a console is allocated
 * and stdout and stderr redirected to it
 *
 * @xref CService::Start
 */
DWORD CService::CheckSIDs(void)
{    
    HANDLE hProcessToken = NULL;
    DWORD groupLength = 50;
    PTOKEN_GROUPS groupInfo = NULL;

    SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
    PSID pInteractiveSid = NULL;
    PSID pServiceSid = NULL;

    DWORD dwRet = NO_ERROR;

    DWORD ndx;

    // open the token
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    // allocate a buffer of default size
    groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
    if (groupInfo == NULL)
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    // try to get the info
    if (!::GetTokenInformation(hProcessToken, TokenGroups, groupInfo, groupLength, &groupLength))
    {
        // if buffer was too small, allocate to proper size, otherwise error
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            dwRet = ::GetLastError();
            goto closedown;
        }

        ::LocalFree(groupInfo);

        groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
        if (groupInfo == NULL)
        {
            dwRet = ::GetLastError();
            goto closedown;
        }

        if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo, groupLength, &groupLength)) 
        {
            dwRet = ::GetLastError();
            goto closedown;
        }
    }

    //
    //    We now know the groups associated with this token.  We want to look to see if
    //  the interactive group is active in the token, and if so, we know that
    //  this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know we're a service.
    //
    //    The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).
    //

    // create comparison sids
    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0, 0, 0, 0, 0, 0, 0, &pInteractiveSid))
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &pServiceSid))
    {
        dwRet = ::GetLastError();
        goto closedown;
    }


    // reset flags
    m_isInteractive = FALSE;
    m_isService = FALSE;

    // try to match sids
    for (ndx = 0; ndx < groupInfo->GroupCount ; ndx += 1)
    {
        SID_AND_ATTRIBUTES sanda = groupInfo->Groups[ndx];
        PSID pSid = sanda.Sid;

        //
        //    Check to see if the group we're looking at is one of
        //    the two groups we're interested in.
        //

        if (::EqualSid(pSid, pInteractiveSid))
        {
            //
            //    This process has the Interactive SID in its
            //  token.  This means that the process is running as
            //  a console process
            //
            m_isInteractive = TRUE;
            m_isService = FALSE;
            break;
        }
        else if (::EqualSid(pSid, pServiceSid))
        {
            //
            //    This process has the Service SID in its
            //  token.  This means that the process is running as
            //  a service running in a user account ( not local system ).
            //
			ASSERT( m_neverService == FALSE );
            m_isService = TRUE;
            m_isInteractive = FALSE;
            break;
        }
    }

    if ( !( m_isService || m_isInteractive ) )
    {
        //
        //  Neither Interactive or Service was present in the current users token,
        //  This implies that the process is running as a service, most likely
        //  running as LocalSystem.
        //
        m_isService = !m_neverService;

        // determine if the local system service is interactive.  We do this by
        // looking in the service's registry and checking the Type value for the
        // SERVICE_INTERACTIVE_PROCESS bit

        HKEY  hkey = NULL;

        TCHAR szKey[256];
        
        wsprintf( szKey, TEXT("SYSTEM\\CurrentControlSet\\Services\\%s"), m_pServiceName );
        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           szKey,
                           0,
                           KEY_READ,
                           &hkey ) == ERROR_SUCCESS )
        {
            DWORD dwType = 0;
            DWORD dwSize = sizeof(DWORD);
            if ( RegQueryValueEx(hkey,
                                 TEXT("Type"),
                                 NULL,
                                 NULL,
                                 (LPBYTE) &dwType,
                                 &dwSize ) == ERROR_SUCCESS )
            {
                if ( dwType & SERVICE_INTERACTIVE_PROCESS )
                {
                    m_isInteractive = TRUE;

                    // service was given permission to interact with the
                    // with the desktop, so allocate a console
                    DWORD dwAllocConsole = 0;
                    DWORD dwAllocSize = sizeof( dwAllocConsole );
                    if ( RegQueryValueEx(hkey,
                                         TEXT("AllocConsole"),
                                         NULL,
                                         NULL,
                                         (LPBYTE) &dwAllocConsole,
                                         &dwAllocSize ) == ERROR_SUCCESS )
                    {
                        if ( dwAllocConsole )
                        {
                            FreeConsole();
                            if ( AllocConsole() )
                            {
                                // Now re-map the C Runtime STDIO handles
                                int hCrt = ::_open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);

                                *stdout = *(::_fdopen(hCrt, "w"));
                                ::setvbuf(stdout, NULL, _IONBF, 0);

                                *stderr = *(::_fdopen(hCrt, "w"));
                                ::setvbuf(stderr, NULL, _IONBF, 0);

                            }
                        }
                    }

                    DWORD dwDebug = 0;
                    DWORD dwDbgSize = sizeof(dwDebug);
                    TCHAR szWindbgPath[256];
                    DWORD dwPathSize = sizeof(szWindbgPath);
                    if (
                       ( RegQueryValueEx(hkey,
                                         TEXT("Debug"),
                                         NULL,
                                         NULL,
                                         (LPBYTE) &dwDebug,
                                         &dwDbgSize ) == ERROR_SUCCESS )
                       &&
                       ( RegQueryValueEx(hkey,
                                         TEXT("WindbgPath"),
                                         NULL,
                                         NULL,
                                         (LPBYTE) szWindbgPath,
                                         &dwPathSize ) == ERROR_SUCCESS )
                       )
                    {
                        if( dwDebug )  // 1 if debug
                        {
                            PROCESS_INFORMATION pi;
                            STARTUPINFO         si;        // startup information

                            si.cb          = sizeof( STARTUPINFO );
                            si.lpReserved  = NULL;
                            si.lpDesktop   = NULL;
                            si.lpTitle     = NULL;
                            si.dwX         = 0;
                            si.dwY         = 0;
                            si.dwXSize     = 0;
                            si.dwYSize     = 0;
                            si.dwFlags     = STARTF_USESHOWWINDOW;
                            si.wShowWindow = SW_SHOWMINNOACTIVE;
                            si.cbReserved2 = 0;
                            si.lpReserved2 = NULL;

                            pi.hProcess  = NULL;

                            TCHAR szCmd[300];
                            wsprintf( szCmd, TEXT("%s -g -p 0x%x"), szWindbgPath, GetCurrentProcessId() );
                            //-- now create the windbg process
                            if( CreateProcess(
                                   NULL, szCmd,
                                   (LPSECURITY_ATTRIBUTES) NULL,
                                   (LPSECURITY_ATTRIBUTES) NULL,
                                   FALSE,
                                   DETACHED_PROCESS,
                                   (LPVOID) NULL,
                                   (LPTSTR) NULL,
                                   &si, &pi ) )
                            {
                                CloseHandle( pi.hThread );
                                CloseHandle( pi.hProcess );
                            }
                        }
                    }

                }
            }

            RegCloseKey( hkey );
        }
        

    }


    closedown:
        if ( pServiceSid )
            ::FreeSid( pServiceSid );

        if ( pInteractiveSid )
            ::FreeSid( pInteractiveSid );

        if ( groupInfo )
            ::LocalFree( groupInfo );

        if ( hProcessToken )
            ::CloseHandle( hProcessToken );

#if TRACE_SERVICE
    {
        char szBuf[256];
        DWORD cbWritten;
        wsprintf( szBuf, "CService::CheckSIDs - Interactive:%ld  Service:%ld  Error:%lu\n", m_isInteractive, m_isService, dwRet );
        WriteFile( g_hFile, szBuf, lstrlen( szBuf ), &cbWritten, NULL );
    }
#endif // TRACE_SERVICE

    return dwRet;
}


/*
 * @doc ALL INTERNAL CSERVICE
 *
 * @api BOOL | CService::control_handler | Implements the console control handler
 *
 * @parm DWORD | dwCtrlType | Console control signal to be handled
 *
 * @rdesc TRUE - If the control signal is handled
 *        FALSE - If the control signal is NOT handled
 *
 * @comm 
 *
 * @xref CService::Start
 */
BOOL WINAPI CService::control_handler ( DWORD dwCtrlType )
{
    ASSERT( g_pService );
    char szBuf[256];
    DWORD bytes;

    switch( dwCtrlType )
    {
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            wsprintf( szBuf, TEXT("Stopping %s.\n"), g_pService->m_pServiceDisplayName);
            WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlen(szBuf)+1, &bytes, NULL );
            g_pService->onStop();
            return TRUE;
            break;

    }
    return FALSE;
}


#endif
