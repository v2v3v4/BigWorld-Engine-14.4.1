#ifndef ASSET_CLIENT
#define ASSET_CLIENT

#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stringmap.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/bw_deque.hpp"

BW_BEGIN_NAMESPACE

class AssetClient : SimpleThread
{
public:
	AssetClient();
	virtual ~AssetClient();

	void waitForConnection();
	void requestAsset( const StringRef & asset, bool wait );
	void lock();
	void unlock();
	void disable();

private:
	bool attemptConnection();
	void resetConnection( bool clearRequests = true );
	void sendRequest( const StringRef & request, bool wait );
	void sendCommand( const StringRef & command );
	bool processRequests();
	bool handleResponses();
	BW::wstring generatePipeId();
	bool launchDefaultAssetServer();

	static void clientThreadFunc( void * arg );

private:
	class RefCountEvent : public SafeReferenceCount
	{
	public:
		SimpleEvent event_;
	};

	SimpleMutex mutex_;
	typedef BW::deque<std::pair<BW::string, RefCountEvent*>> RequestQueue;
	RequestQueue requestQueue_;
	typedef StringHashMap<std::pair<TimeStamp, RefCountEvent*>> ResponseQueue;
	ResponseQueue awaitingResponse_;
	HANDLE hPipe_;
	HANDLE hConnectionEvent_;
	HANDLE hRequestEvent_;
	HANDLE hCommandMutex_;
	HANDLE hProcessMutex_;
	long lockCount_;
	bool locked_;
	bool disabled_;
	bool terminating_;
};

BW_END_NAMESPACE

#endif // ASSET_CLIENT
