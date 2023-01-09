#ifndef ASSET_PIPE
#define ASSET_PIPE

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "resmgr/auto_config.hpp"

#define ASSET_PIPE_SIZE 4096
#define ASSET_PIPE_TOKEN "|"
#define ASSET_PIPE_COMMAND ":"
#define ASSET_PIPE_LOCK "Lock"
#define ASSET_PIPE_UNLOCK "Unlock"
#define ASSET_PIPE_TIMEOUT 10

BW_BEGIN_NAMESPACE

class AssetPipe
{
public:
	static BW::vector< BW::wstring > getBaseResourcePaths();
	static bool writePipe( 
		HANDLE hPipe, 
		const StringRef & msg );
	static bool readPipe( 
		HANDLE hPipe, 
		BW::vector< BW::StringRef > & msgs,
		char (&buffer)[ASSET_PIPE_SIZE],
		DWORD & bufferOffset );

	static BW::wstring s_LocalPath;
	static BW::wstring s_PipePath;
	static BW::wstring s_AssetPipelineId;
	static BW::wstring s_CommandMutex;
	static BW::wstring s_ProcessMutex;
	static AutoConfigString s_ServerPath;
	static AutoConfigString s_ServerParams;
};

BW_END_NAMESPACE

#endif // ASSET_PIPE