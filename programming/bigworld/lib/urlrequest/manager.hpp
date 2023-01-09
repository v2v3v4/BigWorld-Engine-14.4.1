#ifndef URL_MANAGER_HPP
#define URL_MANAGER_HPP

#include "urlrequest/misc.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"


#include "cstdmf/bw_map.hpp"
#include <memory>


BW_BEGIN_NAMESPACE

typedef void CURLM;
typedef void CURL;

namespace Mercury
{
	class EventDispatcher;
	class FrequentTask;
}


namespace URL
{

class Request;
class Response;
class StringList;


/**
 *	This class is used to manage multiple URL requests simultaneously.
 */
class Manager
{
public:
	typedef URL::Method Method;

	Manager( Mercury::EventDispatcher & dispatcher );
	~Manager();

	bool fetchURL( const BW::string & url,
		Response * pResponse,
		Headers * pHeaders = NULL,
		Method method = METHOD_GET,
		float connectTimeoutInSeconds = 0.f,
		PostData * pPostData = NULL,
		Request ** pOutRequest = NULL );

	void checkHandles( );

	void deregisterRequest( Request & request );

	void cancelRequest ( Request * pRequest );

	static bool init( Mercury::EventDispatcher & dispatcher );
	static void fini();

	static Manager * instance()
	{
		return s_managerInstance_;
	}

private:
	void registerRequest( Request & request );

	void processEvents();

	Mercury::FrequentTask * pCheckHandlesTask_;

	CURLM * curlMultiState_;

	typedef BW::map< CURL *, Request * > Requests;
	Requests requests_;

	int numRunning_;

	static Manager * s_managerInstance_;
};

BW::string escape( const BW::string & in );
BW::string unescape( const BW::string & in );

} // namespace URL

BW_END_NAMESPACE

#endif // URL_MANAGER_HPP
