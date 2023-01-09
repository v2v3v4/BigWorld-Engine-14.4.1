#include "pch.hpp"
#include "asset_pipe.hpp"

BW_BEGIN_NAMESPACE

BW::wstring AssetPipe::s_LocalPath = L"Local\\";
BW::wstring AssetPipe::s_PipePath = L"\\\\.\\pipe\\";
BW::wstring AssetPipe::s_AssetPipelineId = L"AssetPipeline";
BW::wstring AssetPipe::s_CommandMutex = L"Command";
BW::wstring AssetPipe::s_ProcessMutex = L"Process";
AutoConfigString AssetPipe::s_ServerPath( "assetServer/path" );
AutoConfigString AssetPipe::s_ServerParams( "assetServer/params" );

BW::vector< BW::wstring > AssetPipe::getBaseResourcePaths()
{
	// retrieve all resource paths
	BW::vector< BW::wstring > paths;
	const int numPaths = BWResource::getPathNum();
	for (int i = 0; i < numPaths; ++i)
	{
		BW::wstring path = bw_utf8tow( BWResource::getPath( i ) );
		//TODO possibly check these paths are not symbolic links
		std::transform( path.begin(), path.end(), path.begin(), towlower );
		paths.push_back( path );
	}
	// remove all subpaths
	for (BW::vector< BW::wstring >::iterator 
		it = paths.begin(); it != paths.end(); )
	{
		bool subPath = false;
		for (BW::vector< BW::wstring >::iterator 
			it2 = paths.begin(); it2 != paths.end(); ++it2 )
		{
			if (it == it2)
			{
				continue;
			}
			if (wcsncmp(it->c_str(), it2->c_str(), it2->size()) == 0)
			{
				subPath = true;
				break;
			}
		}
		it = subPath ? paths.erase( it ) : it + 1;
	}
	// sort the paths alphabetically
	std::sort( paths.begin(), paths.end() );
	return paths;
}

bool AssetPipe::writePipe( 
	HANDLE hPipe, 
	const StringRef & msg )
{
	DWORD written;
	if (!WriteFile( hPipe,
		msg.data(),
		(DWORD)msg.length(),
		&written,
		NULL ))
	{
		return false;
	}
	if (!WriteFile( hPipe,
		ASSET_PIPE_TOKEN,
		1,
		&written,
		NULL ))
	{
		return false;
	}
	return true;
}

bool AssetPipe::readPipe( 
	HANDLE hPipe, 
	BW::vector< BW::StringRef > & msgs,
	char (&buffer)[ASSET_PIPE_SIZE],
	DWORD & bufferOffset )
{
	DWORD available;
	if (!PeekNamedPipe( hPipe,
		NULL,
		0,
		NULL,
		&available,
		NULL ))
	{
		return false;
	}

	if (available == 0)
	{
		return true;
	}

	// read the request from the pipe
	DWORD read;
	if (!ReadFile( hPipe,
		&buffer[bufferOffset],
		ASSET_PIPE_SIZE - bufferOffset,
		&read,
		NULL ))
	{
		return false;
	}

	// handle the request
	MF_ASSERT( read != 0 );
	read = bufferOffset + read;
	if (read != ASSET_PIPE_SIZE)
	{
		buffer[read] = '\0';
	}

	bw_tokenise( BW::StringRef( buffer ), ASSET_PIPE_TOKEN, msgs );

	bufferOffset = 0;
	if (strncmp( &buffer[read - 1], ASSET_PIPE_TOKEN, 1 ) != 0)
	{
		MF_ASSERT( !msgs.empty() );
		bufferOffset = (DWORD)msgs.back().length();
		strncpy( buffer, &buffer[read - bufferOffset], bufferOffset );
		msgs.pop_back();
	}
	return true;
}

BW_END_NAMESPACE