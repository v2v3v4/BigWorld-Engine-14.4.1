#include "pch.hpp"
#include "dprintf.hpp"
#include "callstack.hpp"
#include "string_builder.hpp"

#if BW_ENABLE_STACKTRACE

#pragma warning(push)
#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(pop)

// whether symbol resolution is available
static bool s_symbolResolutionAvailable = false;

// Windows stack helper function definitions
typedef USHORT (__stdcall* RtlCaptureStackBackTraceFuncType)(ULONG FramesToSkip, ULONG FramesToCapture, PVOID* BackTrace, PULONG BackTraceHash);
typedef VOID   (__stdcall* RtlCaptureContextFuncType)( PCONTEXT ContextRecord );

// DbgHelp functions definitions
typedef BOOL	(__stdcall *SymInitializeFuncType)(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess);
typedef DWORD	(__stdcall *SymGetOptionsFuncType)(VOID);	 
typedef DWORD	(__stdcall *SymSetOptionsFuncType)(DWORD SymOptions);	
typedef BOOL	(__stdcall *SymGetSearchPathFuncType)(HANDLE hProcess,	PSTR SearchPath, DWORD SearchPathLength);	  
typedef DWORD64 (__stdcall *SymLoadModule64FuncType)(HANDLE hProcess, HANDLE hFile,	PSTR ImageName, PSTR ModuleName, DWORD64 BaseOfDll,	DWORD SizeOfDll);	  
typedef BOOL	(__stdcall *StackWalk64FuncType)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame,	
									PVOID ContextRecord,	 
									PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,	 
									PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,	
									PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,	  
									PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);	  
typedef BOOL (__stdcall *SymGetSymFromAddr64FuncType)(HANDLE hProcess, DWORD64 qwAddr, PDWORD64 pdwDisplacement, PIMAGEHLP_SYMBOL64 Symbol);
typedef BOOL (__stdcall *SymGetLineFromAddr64FuncType)(HANDLE hProcess, DWORD64 qwAddr,	PDWORD pdwDisplacement,	PIMAGEHLP_LINE64 Line64);	
typedef BOOL (__stdcall *SymCleanupFuncType)(HANDLE hProcess);
typedef BOOL (__stdcall *SymFromAddrFuncType)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);
typedef PVOID (__stdcall *SymFunctionTableAccess64FuncType)(HANDLE hProcess, DWORD64 AddrBase);
typedef DWORD64 (__stdcall *SymGetModuleBase64FuncType)(HANDLE hProcess, DWORD64 AddrBase);  


// Functions loaded from kernel32.dll
RtlCaptureStackBackTraceFuncType RtlCaptureStackBackTraceFunc;
RtlCaptureContextFuncType		 RtlCaptureContextFunc;

// Functions loaded from dbghlp.dll
SymInitializeFuncType           SymInitializeFunc;
SymCleanupFuncType              SymCleanupFunc;
SymGetOptionsFuncType			SymGetOptionsFunc;
SymSetOptionsFuncType			SymSetOptionsFunc;
SymGetSearchPathFuncType		SymGetSearchPathFunc;
SymLoadModule64FuncType			SymLoadModule64Func;
StackWalk64FuncType				StackWalk64Func;
SymGetSymFromAddr64FuncType		SymGetSymFromAddr64Func;
SymGetLineFromAddr64FuncType	SymGetLineFromAddr64Func;
SymFromAddrFuncType             SymFromAddrFunc;
SymFunctionTableAccess64FuncType SymFunctionTableAccess64Func;
SymGetModuleBase64FuncType		SymGetModuleBase64Func;

#define GET_PROC( name_, module_ ) \
	name_##Func = (name_##FuncType)::GetProcAddress( module_, #name_ ); \
	if (name_##Func == NULL) return;

#define GET_KERNEL32_PROC(name_) GET_PROC( name_, kernel32 )
#define GET_DBGHELP_PROC(name_)  GET_PROC( name_, dbghelp )

namespace BW
{

//------------------------------------------------------------------------------------

void initStacktraces()
{
	static bool init = true;
		
	if (init)
	{
		init = false;

		HMODULE kernel32 = ::LoadLibraryA( "kernel32.dll" );
		HMODULE dbghelp  = ::LoadLibraryA( "dbghelp.dll" );

		MF_ASSERT( kernel32 );
		if (dbghelp == NULL)
		{
			dprintf( "Failed to initialise symbol lookups : Cannot load dbghelp.dll\n" );
			return;
		}

		GET_KERNEL32_PROC( RtlCaptureStackBackTrace );
		GET_KERNEL32_PROC( RtlCaptureContext );
		GET_DBGHELP_PROC( SymInitialize );
		GET_DBGHELP_PROC( SymCleanup );
		GET_DBGHELP_PROC( SymGetOptions );
		GET_DBGHELP_PROC( SymSetOptions );
		GET_DBGHELP_PROC( SymGetSearchPath );
		GET_DBGHELP_PROC( SymLoadModule64 );
		GET_DBGHELP_PROC( StackWalk64 );
		GET_DBGHELP_PROC( SymGetSymFromAddr64 );
		GET_DBGHELP_PROC( SymGetLineFromAddr64 );
		GET_DBGHELP_PROC( SymFromAddr );
		GET_DBGHELP_PROC( SymFunctionTableAccess64 );
		GET_DBGHELP_PROC( SymGetModuleBase64 );

		char symbolsPath[2048] = { 0 };
		BW::StringBuilder builder( symbolsPath, ARRAY_SIZE( symbolsPath ) );

		// build PDB path that should be the same as executable path
		{
			char path[_MAX_PATH] = { 0 };
			GetModuleFileNameA( 0, path, _MAX_PATH );        

			// check that the pdb actually exists and is accessible, if it doesn't then SymInitialize will raise an obscure error dialog
			// so just disable complete callstacks if it is not there
			char* pend = strrchr( path, '.');
			*pend = 0;
			strcat( path, ".pdb" );
			FILE *f = fopen( path, "rb" );
			if ( f == NULL )
			{
				dprintf( "Failed to initialise symbol lookups : %s does not exist or is not accessible\n", path );
				return;
			}
			fclose( f );

			pend = strrchr(path, '\\');
			*pend = 0;
			builder.append( path );
		}

		// append the working directory.
		builder.append( ";.\\" );

		// append %SYSTEMROOT% and %SYSTEMROOT%\system32.
		char * env = getenv( "SYSTEMROOT" );
		if (env) 
		{
			builder.append( ";" );
			builder.append( env );
			builder.append( ";" );
			builder.append( env );
			builder.append( "\\system32" );
		}

		// append %_NT_SYMBOL_PATH% and %_NT_ALT_SYMBOL_PATH%.
		if ((env = getenv( "_NT_SYMBOL_PATH" )))
		{
			builder.append( ";" );
			builder.append( env );
		}
		if ((env = getenv( "_NT_ALT_SYMBOL_PATH" )))
		{
			builder.append( ";" );
			builder.append( env );
		}

		SymSetOptionsFunc( SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME );
		if (SymInitializeFunc( ::GetCurrentProcess(), (PSTR)builder.string(), TRUE) ) 
		{
			s_symbolResolutionAvailable = true;
		}
		else
		{
			dprintf( "Failed to initialise symbol lookups\n" );
		}
	}
}

//------------------------------------------------------------------------------------

void finiStacktraces()
{
	SymCleanupFunc( ::GetCurrentProcess() );
}

//------------------------------------------------------------------------------------

bool stacktraceSymbolResolutionAvailable()
{
	return s_symbolResolutionAvailable;
}

//------------------------------------------------------------------------------------

int stacktrace( CallstackAddressType * trace, size_t depth, size_t skipFrames /*= 0*/ )
{
	MF_ASSERT( depth <= ULONG_MAX );
	initStacktraces();
	size_t r = RtlCaptureStackBackTraceFunc( 1 + (ULONG)skipFrames,
		(ULONG)depth, trace, NULL );
	return (int)r;
}

//------------------------------------------------------------------------------------


#if (_MSC_VER == 1500)
// RtlCaptureContextFunc crashes in hybrid on Visual Studio 2008 if optimisations are
// enabled for this function
#pragma optimize("", off)
#pragma warning(push)
#pragma warning(disable : 4748)
#endif 

int stacktraceAccurate( CallstackAddressType * trace, size_t depth,
	size_t skipFrames /*= 0*/, int threadID /*= 0*/ )
{	
	initStacktraces();
	if (!stacktraceSymbolResolutionAvailable())
		return 0;

	// Get appropriate handles
	bool isCurrentThread = 
		static_cast< DWORD >( threadID ) == GetCurrentThreadId();

	HANDLE process = GetCurrentProcess();
	HANDLE thread = NULL;

	if (threadID != 0)
	{
		const DWORD permissions = THREAD_GET_CONTEXT | READ_CONTROL | 
			THREAD_QUERY_INFORMATION;
		thread = OpenThread( permissions, FALSE, (DWORD)threadID );
		MF_ASSERT_NOALLOC( thread != INVALID_HANDLE_VALUE );
	}
	else
	{
		// Call to CloseHandle at end of function is safe:
		// From MSDN: "Calling the CloseHandle function with this handle has no
		// effect."
		thread = GetCurrentThread();
	}

	MF_ASSERT_NOALLOC( thread != NULL );

#if defined(_M_IX86)
	const DWORD Architecture = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_X64)
	const DWORD Architecture = IMAGE_FILE_MACHINE_AMD64;
#endif

	// number of stack frames to omit 
	const size_t skipCount = 2 + skipFrames;

	// initialize the STACKFRAME64 structure.
	CONTEXT context;
	memset( &context, 0, sizeof(context) );
	context.ContextFlags = CONTEXT_FULL;

	if (isCurrentThread)
	{
		RtlCaptureContextFunc( &context );
	}
	else
	{
		GetThreadContext( thread, &context );
	}

	STACKFRAME64            stackFrame;
	memset( &stackFrame, 0, sizeof(stackFrame) );

#ifdef _WIN64
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrFrame.Offset = context.Rbp;
	stackFrame.AddrStack.Offset = context.Rsp;
#else
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrFrame.Offset = context.Ebp;
	stackFrame.AddrStack.Offset = context.Esp;
#endif

	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Mode = AddrModeFlat;

	// Walk the stack.
	size_t count = depth + skipCount;
	size_t getCount = 0;
	size_t index = 0;
	int sc = 0;
	for (; count; --count, ++getCount)
	{
		if (!StackWalk64Func( Architecture, 
				process, 
				thread, 
				&stackFrame, 
				Architecture == IMAGE_FILE_MACHINE_I386 ? NULL : &context,
				NULL, 
				SymFunctionTableAccess64Func, 
				SymGetModuleBase64Func, 
				NULL ))
		{
			// couldn't trace back through any more frames.
			break;
		}
		if (stackFrame.AddrFrame.Offset == 0)
		{
			// end of stack.
			break;
		}
		if (getCount < skipCount)
		{
			// eliminate two Get() calls
			continue;
		}

		trace[sc++] = (CallstackAddressType)stackFrame.AddrPC.Offset;
	}

	CloseHandle( thread );

	return sc;
}

#if (_MSC_VER == 1500)
#pragma warning(pop)
#endif 

//------------------------------------------------------------------------------------

bool convertAddressToFunction( CallstackAddressType address, char * nameBuf, size_t nameBufSize, char * fileBuf, size_t fileBufSize, int * line )
{
	if (!stacktraceSymbolResolutionAvailable())
		return 0;

	IMAGEHLP_LINE64 source_info;
	SYMBOL_INFO *function_info;
	const int MaxSymbolNameLength = 256;
	const int SymbolBufferSize = sizeof( SYMBOL_INFO ) + (MaxSymbolNameLength * sizeof( TCHAR )) - 1;
	unsigned char sym_buf[SymbolBufferSize];
	DWORD	displacement = 0;
	DWORD64 displacement64 = 0;
	HANDLE process = ::GetCurrentProcess();

	// initialise symbol buffer
	function_info = reinterpret_cast<SYMBOL_INFO*>(sym_buf);
	::ZeroMemory( function_info, SymbolBufferSize );
	function_info->SizeOfStruct = sizeof(SYMBOL_INFO);
	function_info->MaxNameLen = MaxSymbolNameLength;
	::ZeroMemory( &source_info, sizeof(IMAGEHLP_LINE64) );
	source_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	// lookup file and line info for this address, but only if user wants the information
	if ((fileBuf || line) && SymGetLineFromAddr64Func( process, (DWORD64)address, &displacement, &source_info )) 
	{
		if (fileBuf && source_info.FileName != 0)
		{
			strncpy_s( fileBuf, fileBufSize, source_info.FileName, _TRUNCATE );
		}

		if (line)
		{
			*line = static_cast<int>(source_info.LineNumber);
		}
	}

	// lookup function 
	if (nameBuf && SymFromAddrFunc( process, (DWORD64)address, &displacement64, function_info ))
	{
		if (function_info->Name != 0)
		{	
			strncpy_s( nameBuf, nameBufSize, function_info->Name, _TRUNCATE );
		}
	}
	else if (nameBuf)
	{
		// Unable to find the name, so lets get the module or address
		MEMORY_BASIC_INFORMATION mbi;
		char fullPath[MAX_PATH];
		if (VirtualQuery( address, &mbi, sizeof(mbi) ) && 
			GetModuleFileNameA( (HMODULE)mbi.AllocationBase, fullPath, sizeof(fullPath) ))
		{
			// Get base name of DLL
			char * filename = strrchr( fullPath, '\\' );
			strncpy_s( nameBuf, nameBufSize, filename == NULL ? fullPath : (filename + 1), _TRUNCATE );
		}
		else
		{
			sprintf_s( nameBuf, nameBufSize, "0x%p", address );
		}
	}

	return true;
}

} // namespace BW

#endif // BW_ENABLE_STACKTRACE

