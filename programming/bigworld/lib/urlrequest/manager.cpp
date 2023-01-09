#include "manager.hpp"
#include "request.hpp"
#include "response.hpp"

#include "script/init_time_job.hpp"

#include "network/frequent_tasks.hpp"
#include "network/event_dispatcher.hpp"

#include <errno.h>
#include <string.h>


BW_BEGIN_NAMESPACE

namespace URL
{
Manager * Manager::s_managerInstance_ = NULL;

namespace // (anonymous)
{

class ScriptFini : public Script::FiniTimeJob
{
public:
	virtual void fini()
	{
		if (Manager::instance() != NULL)
		{
			Manager::fini();
		}
	}
};

ScriptFini g_scriptFini;


/**
 *	This class is a frequent task to check the handles on a Manager.
 */
class CheckHandlesTask : public Mercury::FrequentTask
{
public:
	CheckHandlesTask( Manager & manager, 
		Mercury::EventDispatcher & dispatcher );
	~CheckHandlesTask();

	virtual void doTask();

private:
	Manager & manager_;
	Mercury::EventDispatcher & dispatcher_;
};


/**
 *	Constructor.
 *
 *	@param manager 		The URL::Manager to check handles for.
 *	@param dispatcher	The application main event dispatcher.
 */
CheckHandlesTask::CheckHandlesTask( Manager & manager, 
			Mercury::EventDispatcher & dispatcher ):
		Mercury::FrequentTask( "URLRequest_CheckHandlesTask" ),
	manager_( manager ),
	dispatcher_( dispatcher )
{
	dispatcher_.addFrequentTask( this );
}


/**
 *	Destructor.
 */
CheckHandlesTask::~CheckHandlesTask()
{
	dispatcher_.cancelFrequentTask( this );
}


/**
 *	Override from Mercury::FrequentTask.
 */
void CheckHandlesTask::doTask()
{
	manager_.checkHandles();
}


} // end namespace (anonymous)


/**
 *	Constructor.
 *
 *	@param dispatcher 	The application dispatcher.
 */
Manager::Manager( Mercury::EventDispatcher & dispatcher ) :
		pCheckHandlesTask_( NULL ),
		curlMultiState_( curl_multi_init() ),
		requests_(),
		numRunning_( 0 )
{
	pCheckHandlesTask_ = new CheckHandlesTask( *this, dispatcher );
}

/**
 *	Destructor.
 */
Manager::~Manager()
{
	curl_multi_cleanup( curlMultiState_ );
	delete pCheckHandlesTask_;
}


bool Manager::init( Mercury::EventDispatcher & dispatcher )
{
	if (s_managerInstance_)
	{
		WARNING_MSG( "URL::Manager::init: Already initialised\n" );
		return false;
	}

	s_managerInstance_ = new URL::Manager( dispatcher );

	return true;
}


void Manager::fini()
{
	bw_safe_delete( s_managerInstance_ );
}


/**
 *	This method registers a new request to manage.
 *
 *	@param request 		The request to manage.
 */
void Manager::registerRequest( Request & request )
{
	request.setManager( this );
	CURLMcode ret = curl_multi_add_handle( curlMultiState_, request.handle() );

	if (ret != CURLM_OK)
	{
		ERROR_MSG( "URL::Manager::registerRequest failed: %s\n", 
			curl_multi_strerror( ret ) );
		return;
	}
}


bool Manager::fetchURL( const BW::string & url, 
	Response * pResponse, Headers * pHeaders,
	Method method, float connectTimeoutInSeconds,
	PostData * pPostData, Request ** ppOutRequest /*= NULL*/ )
{
	Request * pRequest = new Request();

	if (ppOutRequest != NULL)
	{
		*ppOutRequest = pRequest;
	}

	pRequest->fetchURL( url, pResponse,
		pHeaders, method, connectTimeoutInSeconds, pPostData );

	this->registerRequest( *pRequest );

	requests_[ pRequest->handle() ] = pRequest;

	CURLMcode ret = curl_multi_perform( curlMultiState_, &numRunning_ );

	if (ret != CURLM_OK)
	{
		ERROR_MSG( "URL::Manager::registerRequest: failed to perform: %s\n",
			curl_multi_strerror( ret ) );

		return false;
	}

	this->processEvents();

	return true;
}


/**
 *	This method deregisters the given request from this manager.
 *
 *	@param request 	The request to deregister.
 */
void Manager::deregisterRequest( Request & request )
{
	Requests::iterator iter = requests_.find( request.handle() );

	if (iter != requests_.end())
	{
		requests_.erase( iter );
		curl_multi_remove_handle( curlMultiState_, request.handle() );
	}
	else
	{
		ERROR_MSG( "URL::Manager::deregisterRequest: No request found\n" );
	}
}


/**
 *	This method cancels and deregisters the given request from this manager.
 *
 *	@param pRequest 	The request to cancel and deregister.
 */
void Manager::cancelRequest( Request * pRequest )
{
	pRequest->cancel();
}


/**
 *	This method checks all managed requests for completion.
 */
void Manager::checkHandles( )
{
	if (numRunning_ <= 0)
	{
		return;
	}

	CURLMcode rcode = curl_multi_perform( curlMultiState_, &numRunning_ );

	if (rcode != CURLM_OK)
	{
		ERROR_MSG( "URL::Manager::checkHandles: Failed on multi: %s\n",
				curl_multi_strerror( rcode ) );
		return;
	}

	this->processEvents();
}


/**
 * This method processes any events from cURL
 */
void Manager::processEvents()
{
	int messagesInQueue = 0;
	CURLMsg * msg = NULL;
	while ((msg = curl_multi_info_read( 
			curlMultiState_, &messagesInQueue )) != NULL)
	{
		if (msg->msg == CURLMSG_DONE)
		{
			Requests::iterator iter = requests_.find( msg->easy_handle );

			if (iter != requests_.end())
			{
				iter->second->onDone( msg->data.result );
			}
			else
			{
				ERROR_MSG( "URL::Manager::processEvents: "
						"No request found (%p)\n", msg->easy_handle );
			}
		}
	}
}


/**
 *  This method is used to URL-encode characters in the given string.
 *
 *  @param in	The string to URL-encode.
 */
BW::string escape( const BW::string & in )
{
	// Unfortunately, the recommended way to urlencode using cURL is to use
	// curl_easy_escape(), which takes a curl handle as an input parameter.
	static Request request;

	char * outCString = curl_easy_escape( request.handle(),
			in.c_str(), static_cast<int>(in.size()) );
	BW::string out( outCString );
	curl_free( outCString );

	return out;
}


/**
 *	This method is used to decode URL-encoded strings.
 *
 *	@param in	The string to decode.
 */
BW::string unescape( const BW::string & in )
{
	// Unfortunately, the recommended way to urlencode using cURL is to use
	// curl_easy_escape(), which takes a curl handle as an input parameter.
	static Request request;

	int outLength = 0;
	char * outCString = curl_easy_unescape( request.handle(),
			in.c_str(), static_cast<int>(in.size()), &outLength );
	BW::string out( outCString, outLength );
	curl_free( outCString );

	return out;
}

} // namespace URL

BW_END_NAMESPACE

// url_request_manager.cpp
