#include "pch.hpp"
#include "asset_client.hpp"

#include "asset_lock.hpp"
#include "asset_pipe.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/launch_windows_process.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/string_utils.hpp"

#include <Shlwapi.h>
#include <Psapi.h>

BW_BEGIN_NAMESPACE

namespace
{
	struct ScopedMutex
	{
		ScopedMutex( HANDLE mutex )
		{
			mutex_ = mutex;
			WaitForSingleObject( mutex_, INFINITE );
		}

		~ScopedMutex()
		{
			ReleaseMutex( mutex_ );
		}

		HANDLE mutex_;
	};
}

AssetClient::AssetClient()
: hPipe_( INVALID_HANDLE_VALUE )
, hConnectionEvent_( INVALID_HANDLE_VALUE )
, hRequestEvent_( INVALID_HANDLE_VALUE )
, hCommandMutex_( INVALID_HANDLE_VALUE )
, hProcessMutex_( INVALID_HANDLE_VALUE )
, lockCount_( 0 )
, locked_( false )
, disabled_( false )
, terminating_( false )
{
	AssetLock::setAssetClient( this );

	LPSTR commandLine = ::GetCommandLineA();
	if (strstr( commandLine, "-noConversion" ))
	{
		disable();
		return;
	}

	hConnectionEvent_ = CreateEvent( 0, true, false, NULL );
	hRequestEvent_ = CreateEvent( 0, true, true, NULL );
	SimpleThread::init( clientThreadFunc, this, "AssetClient" );
}

AssetClient::~AssetClient()
{
	terminating_ = true;
	// Wake up the client thread and wait for it to terminate
	SetEvent( hRequestEvent_ );
	WaitForSingleObject( handle(), INFINITE );
	CloseHandle( hRequestEvent_ );
	CloseHandle( hConnectionEvent_ );
}

void AssetClient::waitForConnection()
{
	if (disabled_)
	{
		return;
	}

	::WaitForSingleObject( hConnectionEvent_, INFINITE );
}

void AssetClient::disable()
{
	disabled_ = true;

	SetEvent( hConnectionEvent_ );
}

void AssetClient::requestAsset( const StringRef & asset, bool wait )
{
	if (locked_)
	{
		ERROR_MSG( "Tried to request asset %s from a locked assert server.\n",
			asset.to_string().c_str() );
		return;
	}

	sendRequest( asset, wait );
}

void AssetClient::lock()
{
	if (InterlockedIncrement( &lockCount_ ) == 1)
	{
		locked_ = true;
		sendCommand( ASSET_PIPE_LOCK );
	}
}

void AssetClient::unlock()
{
	if (InterlockedDecrement( &lockCount_ ) == 0)
	{
		sendCommand( ASSET_PIPE_UNLOCK );
		locked_ = false;
	}
}

bool AssetClient::attemptConnection()
{
	if (locked_ || disabled_)
	{
		// don't try to connect if we are locked or disabled
		return true;
	}

	if (hPipe_ != INVALID_HANDLE_VALUE)
	{
		// already connected, just return
		return true;
	}

    BW::wstring pipeId = generatePipeId();
	
	BW::wstring pipeName = L"";
	if (!pipeId.empty())
	{
		pipeName = AssetPipe::s_PipePath + pipeId;

		BW::wstring commandMutex = AssetPipe::s_LocalPath + pipeId + 
			AssetPipe::s_CommandMutex;
		hCommandMutex_ = CreateMutexW( NULL, false, commandMutex.c_str() );
		if (hCommandMutex_ == NULL)
		{
			return false;
		}

		BW::wstring processMutex = AssetPipe::s_LocalPath + pipeId + 
			AssetPipe::s_ProcessMutex;
		hProcessMutex_= CreateMutexW( NULL, false, processMutex.c_str() );
		if (hProcessMutex_ == NULL)
		{
			return false;
		}
	}

	// try to find a pipe of the specified name
	static TimeStamp ts = 0;
	if (WaitNamedPipe( pipeName.c_str(), NMPWAIT_USE_DEFAULT_WAIT ) == false)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND ||
			pipeName.empty())
		{
			// if the pipe does not exist, launch an asset server
			if (!launchDefaultAssetServer())
			{
				return false;
			}

			if (ts == 0)
			{
				ts = timestamp();
			}
			if (ts.ageInSeconds() > ASSET_PIPE_TIMEOUT)
			{
				ts = 0;
				// if we still cant find the pipe after launching an asset server
				// there is nothing more we can do
				return false;
			}
		}
		return true;
	}
	ts = 0;

	// grab the mutex - we are going to modify hPipe_
	SimpleMutexHolder mutexHolder( mutex_ );

	// connect to the pipe
	hPipe_ = ::CreateFileW( pipeName.c_str(),
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		NULL,
		OPEN_EXISTING,
		0, 
		NULL );

	// check if we were able to connect to the pipe or if the pipe is busy
	if (hPipe_ == INVALID_HANDLE_VALUE &&
		GetLastError() != ERROR_PIPE_BUSY)
	{
		return false;
	}

	return true;
}

void AssetClient::resetConnection( bool clearRequests )
{
	SimpleMutexHolder mutexHolder( mutex_ );

	// Reset the connection event
	if (!disabled_)
	{
		ResetEvent( hConnectionEvent_ );
	}

	if (clearRequests)
	{
		// Clear all pending requests
		for ( RequestQueue::iterator it = requestQueue_.begin(); 
			it != requestQueue_.end(); ++it )
		{
			if (it->second != NULL)
			{
				it->second->event_.set();
			}
		}
		requestQueue_.clear();

		for ( ResponseQueue::iterator it = awaitingResponse_.begin(); 
			it != awaitingResponse_.end(); ++it )
		{
			MF_ASSERT( it->second.second != NULL );
			it->second.second->event_.set();
		}
		awaitingResponse_.clear();
	}

	// Close and reset the pipe
	CloseHandle( hPipe_ );
	CloseHandle( hCommandMutex_ );
	CloseHandle( hProcessMutex_ );
	hPipe_ = INVALID_HANDLE_VALUE;
	hCommandMutex_ = INVALID_HANDLE_VALUE;
	hProcessMutex_ = INVALID_HANDLE_VALUE;
}

void AssetClient::sendRequest( const StringRef & request, bool wait )
{
	if (disabled_)
	{
		return;
	}

	SmartPointer<RefCountEvent> event = NULL;
	{
		SimpleMutexHolder mutexHolder( mutex_ );

		// check if the request is already queued
		bool alreadyQueued = false;
		for ( RequestQueue::iterator it = requestQueue_.begin();
			it != requestQueue_.end(); ++it )
		{
			if (it->first == request)
			{
				if (wait && it->second == NULL)
				{
					// create an event if the previous request was non blocking
					event = new RefCountEvent();
					it->second = event.get();
				}
				else if (wait)
				{
					// copy the event if the previous request was blocking
					event = it->second;
				}
				alreadyQueued = true;
				break;
			}
		}

		if (!alreadyQueued)
		{
			if (wait)
			{
				// we want to wait for this asset to be built so we need to wait on an event
				ResponseQueue::iterator it = awaitingResponse_.find( 
					request.to_string() );
				if (it == awaitingResponse_.end())
				{
					// create a new ref counted event
					event = new RefCountEvent();
					requestQueue_.push_back( std::make_pair( 
						request.to_string(), event.get() ) );
					SetEvent( hRequestEvent_ );
				}
				else
				{
					// an event for this asset already exists
					MF_ASSERT( it->second.second != NULL );
					event = it->second.second;
				}
			}
			else
			{
				// just push back the request without an event
				requestQueue_.push_back( std::make_pair( 
					request.to_string(), event.get() ) );
				SetEvent( hRequestEvent_ );
			}
		}
	}

	if (event.hasObject())
	{
		// block and wait on the asset server
		event->event_.wait();
	}
}

void AssetClient::sendCommand( const StringRef & command )
{
	if (disabled_)
	{
		return;
	}

	// change to a mutex
	// only one command can be issued at any time by any process
	ScopedMutex commandMutex( hCommandMutex_ );
	char buffer[256];
	bw_snprintf( buffer, 256, "%s%s", ASSET_PIPE_COMMAND, 
		command.to_string().c_str() );
	// send a request and wait for the response
	sendRequest( buffer, true );
}

bool AssetClient::processRequests()
{
	SimpleMutexHolder mutexHolder( mutex_ );

	// go through all the requests awaiting a response and re-request anything older
	// than a second
	for ( ResponseQueue::iterator
		it = awaitingResponse_.begin(); it != awaitingResponse_.end(); ++it )
	{
		MF_ASSERT( it->second.second != NULL );
		if (it->second.first.ageInSeconds() < 1)
		{
			continue;
		}
		requestQueue_.push_back( std::make_pair( it->first, it->second.second ) );
	}

	// go through our request queue and pass them to the asset server
	while (!requestQueue_.empty())
	{
		RequestQueue::const_reference request = requestQueue_.front();

		if (!AssetPipe::writePipe( hPipe_, request.first ))
		{
			return false;
		}

		if (request.second != NULL)
		{
			// this request requires a response
			ResponseQueue::iterator it = awaitingResponse_.find( request.first );
			if (it != awaitingResponse_.end())
			{
				MF_ASSERT( it->second.second == request.second );
				it->second.first = timestamp();
			}
			else
			{
				awaitingResponse_[request.first] = std::make_pair( timestamp(), request.second );
			}
		}
		requestQueue_.pop_front();
	}

	return true;
}

bool AssetClient::handleResponses()
{
	char buffer[ASSET_PIPE_SIZE];
	DWORD bufferOffset = 0;
	while (true)
	{
		typedef BW::vector< BW::StringRef > StringArray;
		StringArray responses;
		if (!AssetPipe::readPipe( hPipe_, responses, buffer, bufferOffset ))
		{
			return false;
		}

		if (responses.empty())
		{
			return true;
		}

		// notify any events waiting on this response
		{
			SimpleMutexHolder mutexHolder( mutex_ );
			for (StringArray::const_iterator responseIt = responses.begin(); 
				responseIt != responses.end(); ++responseIt)
			{
				ResponseQueue::const_iterator it = awaitingResponse_.find( 
					responseIt->to_string() );
				if (it != awaitingResponse_.end())
				{
					MF_ASSERT( it->second.second != NULL );
					it->second.second->event_.set();
					awaitingResponse_.erase( it );
				}
			}
		}
	}
}

BW::wstring AssetClient::generatePipeId()
{
	// The pipe id is the hash of the asset server path.
	BW::string path = BWResolver::resolveFilename( AssetPipe::s_ServerPath.value() );
	path = BWUtil::normalisePath( path );
	BW::uint64 hash = BW::Hash64::compute(
		BWResource::correctCaseOfPath( 
		BWResource::removeExtension( path ) ) );
	wchar_t wpath[18];
	bw_snwprintf( wpath, 17, L"%.16X", hash );
	return AssetPipe::s_AssetPipelineId + wpath;
}

bool AssetClient::launchDefaultAssetServer()
{
	static HANDLE hProcess = NULL;
	if (hProcess != NULL)
	{
		// We already tried to launch a process
		// check if it is still running
		DWORD dwRet = S_OK;
		if (GetExitCodeProcess( hProcess, &dwRet ) &&
			dwRet == STILL_ACTIVE)
		{
			return true;
		}

		hProcess = NULL;
	}

	ScopedMutex processMutex( hProcessMutex_ );
	BW::string path = BWResolver::resolveFilename( AssetPipe::s_ServerPath.value() );
	path = BWUtil::normalisePath( path );
	path = BWResource::correctCaseOfPath( 
		   BWResource::removeExtension( path ) );

	// Attempt to find a running asset server process.
	DWORD processes[1024], needed;
	if (!EnumProcesses( processes, sizeof(processes), &needed ))
	{
		return false;
	}

	const int numProcesses = needed / sizeof(DWORD);
	for (int i = 0; i < numProcesses; ++i)
	{
		if (processes[i] == NULL)
		{
			continue;
		}

		hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, processes[i] );
		if (hProcess == NULL)
		{
			continue;
		}

		char buffer[MAX_PATH];
		GetModuleFileNameExA( hProcess, NULL, buffer, ARRAY_SIZE( buffer ) );
		if (path == BWResource::correctCaseOfPath( 
					BWResource::removeExtension( buffer ) ))
		{
			return true;
		}

		hProcess = NULL;
	}

	// No running asset server was found so try to launch a new asset server
	ProcessHandles ph;
	if (launchProcess( path, 
		AssetPipe::s_ServerParams.value(), ph, false, true ) != S_OK)
	{
		return false;
	}

	hProcess = ph.process;
	return true;
}

void AssetClient::clientThreadFunc( void * arg )
{
	AssetClient & assetClient = *static_cast< AssetClient * >( arg );

	while (!assetClient.terminating_ && !assetClient.disabled_)
	{
		::WaitForSingleObject( assetClient.hRequestEvent_, 1000 );

		if (!assetClient.attemptConnection())
		{
			if (CStdMf::checkUnattended())
			{
				ERROR_MSG( "Could not connect to asset server. Terminating.\n" );
				ExitProcess( -1 );
			}

			int res = ::MessageBox( NULL, 
				L"Could not connect to an asset server.\n\n"
				L"Cancel: Terminate application.\n"
				L"Try Again: Reattempt connection.\n"
				L"Continue: Continue without asset server."
				L" Assets may load incorrectly.\n",
				L"AssetPipeline",
				MB_CANCELTRYCONTINUE );
			switch (res)
			{
			case IDCANCEL:
				ExitProcess( -1 );

			case IDCONTINUE:
				assetClient.disable();
				break;
			}
		}

		if (assetClient.hPipe_ == INVALID_HANDLE_VALUE)
		{
			// pipe is invalid. try again
			continue;
		}

		// Set the connection event
		SetEvent( assetClient.hConnectionEvent_ );

		if (!assetClient.processRequests() ||
			!assetClient.handleResponses())
		{
			// If we encountered an error while processing requests
			// or handling responses, reset the connection and continue.
			// Don't clear the pending requests as at this point we
			// haven't completely given up on connecting to an asset server.
			assetClient.resetConnection( false );
			continue;
		}

		// if we have no requests or are not awaiting responses,
		// we can put ourselves to sleep
		SimpleMutexHolder mutexHolder( assetClient.mutex_ );
		if (assetClient.requestQueue_.empty() &&
			assetClient.awaitingResponse_.empty())
		{
			ResetEvent( assetClient.hRequestEvent_ );
		}
	}

	// clean up the connection
	assetClient.resetConnection();
}

BW_END_NAMESPACE
