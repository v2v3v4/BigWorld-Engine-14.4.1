#include "pch.hpp"
#include "asset_pipe.hpp"
#include "asset_server.hpp"

#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

namespace AssetServer_Locals
{
	struct PipeInfo 
	{
		HANDLE hPipe_;
		AssetServer * assetServer_;
	};
}

AssetServer::AssetServer()
: terminating_( false )
, hCommandMutex_( INVALID_HANDLE_VALUE )
{
	SimpleThread::init( serverThreadFunc, this, "AssetServer" );
}

AssetServer::~AssetServer()
{
	// forcefully terminate the server thread
	terminating_ = true;
	{
		SimpleMutexHolder smh( mutex_ );
		TerminateThread( handle(), 0 );
	}

	// allow all the individual pipe threads to finish
	for (;;)
	{
		SimpleThread * pipeThread = NULL;
		{
			SimpleMutexHolder smh( mutex_ );
			if (!pipeThreads_.empty())
			{
				pipeThread = pipeThreads_.begin()->second;
			}
		}
		if (pipeThread != NULL)
		{
			delete pipeThread;
		}
		else
		{
			break;
		}
	}
}

void AssetServer::broadcastAsset( const StringRef & asset )
{
	SimpleMutexHolder smh( mutex_ );

	// write the built asset to all pipes
	for ( BW::map<HANDLE, SimpleThread *>::iterator
		it = pipeThreads_.begin(); it != pipeThreads_.end(); )
	{
		if (!AssetPipe::writePipe( it->first, asset ))
		{
			DisconnectNamedPipe( it->first );
			it = pipeThreads_.erase( it );
		}
		else
		{
			++it;
		}
	}
}

void AssetServer::processCommand( HANDLE hPipe, const StringRef & command )
{
	// The request was a command
	if (strncmp( command.data(), 
		ASSET_PIPE_LOCK, strlen( ASSET_PIPE_LOCK ) ) == 0)
	{
		lock( hPipe );
	}
	else if (strncmp( command.data(),
		ASSET_PIPE_UNLOCK, strlen( ASSET_PIPE_UNLOCK ) ) == 0)
	{
		unlock( hPipe );
	}
}

void AssetServer::lock( HANDLE hPipe )
{
	SimpleMutexHolder smh( mutex_ );

	for ( BW::vector<HANDLE>::const_iterator it = lockedPipes_.begin();
		it != lockedPipes_.end(); ++it )
	{
		if (*it == hPipe)
		{
			return;
		}
	}

	lockedPipes_.push_back( hPipe );
	if (lockedPipes_.size() == 1)
	{
		lock();
	}
}

void AssetServer::unlock( HANDLE hPipe )
{
	SimpleMutexHolder smh( mutex_ );

	for ( BW::vector<HANDLE>::iterator it = lockedPipes_.begin();
		it != lockedPipes_.end(); ++it )
	{
		if (*it == hPipe)
		{
			lockedPipes_.erase( it );
			if (lockedPipes_.empty())
			{
				unlock();
			}
			return;
		}
	}
}

BW::wstring AssetServer::generatePipeId()
{
	char path[BW_MAX_PATH];
	MF_VERIFY( BWUtil::getExecutablePath( path, ARRAY_SIZE( path ) ) );
	BW::uint64 hash = BW::Hash64::compute( 
		BWResource::correctCaseOfPath( 
		BWResource::removeExtension( path ) ) );
	wchar_t wpath[18];
	bw_snwprintf( wpath, 17, L"%.16X", hash );
	return AssetPipe::s_AssetPipelineId + wpath;
}

void AssetServer::serverThreadFunc( void * arg )
{
	AssetServer & assetServer = *static_cast< AssetServer * >( arg );

	BW::wstring pipeId = assetServer.generatePipeId();
	MF_ASSERT( !pipeId.empty() );

	BW::wstring pipeName = AssetPipe::s_PipePath + pipeId;
	BW::wstring commandMutex = AssetPipe::s_LocalPath + pipeId + 
		AssetPipe::s_CommandMutex;
	assetServer.hCommandMutex_ = CreateMutexW( NULL, false, commandMutex.c_str() );
	if (assetServer.hCommandMutex_ == NULL)
	{
		CRITICAL_MSG( "Failed to create AssetPipeline command mutex %ls\n",
			commandMutex.c_str() );
		return;
	}

	while (true)
	{
		// create a pipe
		HANDLE hPipe = CreateNamedPipe( pipeName.c_str(), 
										PIPE_ACCESS_DUPLEX,
										PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
										PIPE_UNLIMITED_INSTANCES, 
										ASSET_PIPE_SIZE,
										ASSET_PIPE_SIZE, 
										0,
										NULL); 

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			return;
		}

		// wait for a client to connect
		if (ConnectNamedPipe( hPipe, NULL ) == false &&
			GetLastError() != ERROR_PIPE_CONNECTED)
		{
			CloseHandle( hPipe );
			continue;
		}

		{
			SimpleMutexHolder smh( assetServer.mutex_ );

			// create a read/write thread for the pipe
			AssetServer_Locals::PipeInfo pInfo;
			pInfo.hPipe_ = hPipe;
			pInfo.assetServer_ = &assetServer;
			std::auto_ptr< SimpleThread > pipeThread( new SimpleThread( pipeThreadFunc, &pInfo ) );
			if (pipeThread->handle() == NULL)
			{
				// if the thread failed, disconnect the pipe
				DisconnectNamedPipe( hPipe );
				CloseHandle( hPipe );
			}
			else
			{
				assetServer.pipeThreads_[hPipe] = pipeThread.release();
			}
		}
	}
}

void AssetServer::pipeThreadFunc( void * arg )
{
	AssetServer_Locals::PipeInfo & pInfo = *static_cast< AssetServer_Locals::PipeInfo * >( arg );
	HANDLE hPipe = pInfo.hPipe_;
	AssetServer & assetServer = *pInfo.assetServer_;

	char buffer[ASSET_PIPE_SIZE];
	DWORD bufferOffset = 0;
	while (!assetServer.terminating_)
	{
		typedef BW::vector< BW::StringRef > StringArray;
		StringArray requests;
		if (!AssetPipe::readPipe( hPipe, requests, buffer, bufferOffset ))
		{
			break;
		}

		if (requests.empty())
		{
			continue;
		}

		for (StringArray::const_iterator 
			requestIt = requests.begin(); requestIt != requests.end(); ++requestIt)
		{
			if (strncmp( requestIt->begin(), ASSET_PIPE_COMMAND, 1 ) == 0)
			{
				StringRef command( requestIt->data() + 1, requestIt->length() );
				assetServer.processCommand( hPipe, command );
				AssetPipe::writePipe( hPipe, *requestIt );
			}
			else
			{
				assetServer.onAssetRequested( *requestIt );
			}
		}
	}

	assetServer.unlock( hPipe );

	// disconnect the pipe
	{
		SimpleMutexHolder smh( assetServer.mutex_ );
		DisconnectNamedPipe( hPipe );
		CloseHandle( hPipe );
		assetServer.pipeThreads_.erase( hPipe );
	}
}

BW_END_NAMESPACE
