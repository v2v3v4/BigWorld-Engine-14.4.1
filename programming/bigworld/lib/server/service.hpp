#ifndef _SERVICE_H_
#define _SERVICE_H_


#ifdef _WIN32

class CService
{
  public:
    // constructor & destructor
    CService( LPTSTR pServiceName, LPTSTR pServiceDisplayName = NULL, DWORD svcType = SERVICE_WIN32_OWN_PROCESS );
    ~CService();

    // notifies the Service Control Manager (SCM) that progress is being made
    // while the service is in a "pending" state
    DWORD Checkpoint( DWORD dwWaitHint = 0 );

    // notifies the SCM that an error has occcured, or the service has
    // entered a new state
    DWORD UpdateStatus( DWORD dwState = SERVICE_START_PENDING,
                        DWORD dwWaitHint = 0,
                        DWORD dwError = NO_ERROR );

    // optional call: used for shared services in one process
    BOOL SetDispatchEntries( SERVICE_TABLE_ENTRY* pDispatch );

    // launches the main service code as either a service or console process
    DWORD Start( int argc, char** argv);

	// tell the class that we never want to act as a service
	void NeverService() { m_neverService = TRUE; }

    // launches the service without calling the service control dispatcher
    void StartWithExternalDispatch( DWORD dwArgc, LPTSTR* lpArgv);

    // call RegisterServiceCtrlHandler and sets up the m_statusHandle member
    void RegisterExternalHandler( LPHANDLER_FUNCTION handler );

    // service maintance routines
    // pDependencies may be standard format or contain
    // '~' for NULLs and '+' for SC_GROUP_IDENTIFIER
    DWORD Install( LPTSTR pDependencies = NULL,
                   DWORD svcType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                   LPTSTR pszPath = NULL );
    DWORD Remove();

    // the body of the service, where the real work is done
    virtual void Main() = 0;  // PURE VIRTUAL FUNCTION
    DWORD   dwArgc;
    TCHAR** lpArgv;

    // -- service control request handlers --
    //   all control handlers are responsible for calling UpdateStatus()
    //   for a State Change
    virtual void            onStop(void)=0;   // PURE VIRTUAL FUNCTION
    virtual void            onPause(void) {};
    virtual void            onContinue(void) {};
    virtual void            onInterrogate(void) {};
    virtual void            onShutdown() {};
    virtual void            onUserControl( DWORD dwCtrl ) {};

    void SetControlsAccepted( DWORD dwControlsAccepted = SERVICE_ACCEPT_STOP )
        { m_dwControlsAccepted = dwControlsAccepted; }

    DWORD GetControlsAccepted()
        { return m_dwControlsAccepted; }

    LPCTSTR GetName()
        { return m_pServiceName; }

    LPCTSTR GetDisplayName()
        { return m_pServiceDisplayName; }

    BOOL isService()
        { return m_isService; }

    BOOL isInteractive()
        { return m_isInteractive; }

    BOOL isStopping()
        { if ( ( m_Status.dwCurrentState == SERVICE_STOP_PENDING ) ||
               ( m_Status.dwCurrentState == SERVICE_STOPPED ) )
             return TRUE;
          else
             return FALSE;
        }

    static void WINAPI service_main( DWORD dwArgc, LPTSTR* lpArgv );


  protected:
    SERVICE_STATUS          m_Status;
    SERVICE_STATUS_HANDLE   m_StatusHandle;
    DWORD                   m_dwControlsAccepted;

    BOOL                    m_isService;
    BOOL                    m_isInteractive;
	BOOL					m_neverService;
    
    LPTSTR                  m_pServiceName;
    LPTSTR                  m_pServiceDisplayName;

    SERVICE_TABLE_ENTRY*    m_pDispatchTable;

    DWORD CheckSIDs(void);
    void WINAPI ServiceCtrl( DWORD dwCtrlCode );


    static void WINAPI service_ctrl( DWORD dwCtrlCode );

    static BOOL WINAPI control_handler( DWORD dwCtrlType );


};


#ifdef __cplusplus
extern "C" {
#endif

BOOL ServiceIsStopping();
DWORD ServiceCheckpoint( DWORD dwWaitHint );
DWORD ServiceUpdateStatus( DWORD dwState, DWORD dwWaitHint, DWORD dwError );

#ifdef __cplusplus
}
#endif

#else // ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#define SERVICE_RUNNING 0
inline DWORD ServiceCheckpoint( DWORD dwWaitHint ) { return 0; }
inline DWORD ServiceUpdateStatus( DWORD dwState, DWORD dwWaitHint, DWORD dwError ) { return 0; }

#ifdef __cplusplus
}
#endif

#endif //def _WIN32

#endif // _SERVICE_H_

