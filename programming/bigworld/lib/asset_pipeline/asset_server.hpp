#ifndef ASSERT_SERVER
#define ASSERT_SERVER

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/concurrency.hpp"

BW_BEGIN_NAMESPACE

class AssetServer : SimpleThread
{
public:
	AssetServer();
	virtual ~AssetServer();

	void broadcastAsset( const StringRef & asset );

protected:
	virtual void onAssetRequested( const StringRef & asset ) = 0;
	virtual void lock() = 0;
	virtual void unlock() = 0;

private:
	void processCommand( HANDLE hPipe, const StringRef & command );
	void lock( HANDLE hPipe );
	void unlock( HANDLE hPipe );
	BW::wstring generatePipeId();

	static void serverThreadFunc( void * arg );
	static void pipeThreadFunc( void * arg );

private:
	SimpleMutex mutex_;
	HANDLE hCommandMutex_;
	BW::map<HANDLE, SimpleThread*> pipeThreads_;
	BW::vector<HANDLE> lockedPipes_;
	bool terminating_;
};

BW_END_NAMESPACE

#endif // ASSERT_SERVER
