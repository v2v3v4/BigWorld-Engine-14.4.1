#include "request.hpp"

#include "manager.hpp"
#include "response.hpp"
#include "string_list.hpp"

#include "cstdmf/debug.hpp"


// Uncomment this to enable extra debugging of cURL errors
//#define BW_CURL_ENABLE_DEBUG

BW_BEGIN_NAMESPACE

namespace URL
{

// -----------------------------------------------------------------------------
// Section: CurlLibrary
// -----------------------------------------------------------------------------

namespace  // (anonymous)
{

/**
 *	Class for libcURL initialisation and cleanup.
 */
class CurlLibrary
{
public:
	/** Constructor. */
	CurlLibrary()
	{
		curl_global_init( CURL_GLOBAL_ALL );
	}	

	/** Destructor */
	~CurlLibrary()
	{
		curl_global_cleanup();
	}
};

CurlLibrary s_curlLibrary;


/**
 *	This is the write function registered with the cURL handle for
 *	CURLOPT_WRITEFUNCTION.
 */
size_t curl_write_function( void * ptr, size_t size, size_t nmemb, 
		void * userdata )
{
	Request * pRequest = reinterpret_cast< Request *>( userdata );


	// From CURL's point of view, it is writing data out somewhere. From the
	// application point of view, we are reading data from a URL. URLRequest
	// abstracts the curl crazy away, but this is why there is a function being
	// called here named read inside a function named write.
	return pRequest->onRead(
			reinterpret_cast< const char * >( ptr ), size * nmemb );
}


#if defined( BW_CURL_ENABLE_DEBUG )
int curl_debug_function ( CURL *handle, curl_infotype type, char *data,
						 size_t size, void *userptr ) 
{ 
	if (type == CURLINFO_TEXT) 
	{
		DEBUG_MSG( "curl_debug_function: %s", data );
	} 
	return 0; 
} 
#endif // defined ( BW_CURL_ENABLE_DEBUG )

} // end namespace (anonymous)


// -----------------------------------------------------------------------------
// Section: URL::Request
// -----------------------------------------------------------------------------

/** Constructor. */
Request::Request() :
	curlHandle_( curl_easy_init() ),
	pResponse_( NULL ),
	pManager_( NULL ),
	responseCode_( 0 ),
	shouldAbort_( false ),
	haveReadContentType_( false ),
	curlFormData_( NULL )
{
	this->setCurlOption( CURLOPT_WRITEDATA, (void*)this );
	this->setCurlOption( CURLOPT_WRITEFUNCTION, &curl_write_function );
	this->setCurlOption( CURLOPT_NOPROGRESS, 1 );
	this->setCurlOption( CURLOPT_FOLLOWLOCATION, 1 );
	this->setCurlOption( CURLOPT_MAXREDIRS, 10 );
#if defined( BW_CURL_ENABLE_DEBUG )
	this->setCurlOption( CURLOPT_DEBUGFUNCTION, &curl_debug_function );
	this->setCurlOption( CURLOPT_VERBOSE, 1 );
#endif // defined( BW_CURL_ENABLE_DEBUG )
}


/**
 *	Destructor.
 */
Request::~Request()
{
	if (pManager_)
	{
		pManager_->deregisterRequest( *this );
	}

	if (curlFormData_)
	{
		curl_formfree( curlFormData_ );
	}

	curl_easy_cleanup( curlHandle_ );
}


/**
 *	This method is called when there is data received for this request.
 */
size_t Request::onRead( const char * data, size_t len )
{
	if (shouldAbort_)
	{
		return CURL_READFUNC_ABORT;
	}

	if (pResponse_)
	{
		if (responseCode_ == 0 && this->getCurlInfo( CURLINFO_RESPONSE_CODE, 
				&responseCode_ ) == CURLE_OK)
		{
			if (!pResponse_->onResponseCode( responseCode_ ))
			{
				shouldAbort_ = true;
			}

			if (shouldAbort_)
			{
				return CURL_READFUNC_ABORT;
			}
		}

		if (!haveReadContentType_)
		{
			haveReadContentType_ = true;
			char * contentType = NULL;
			if (CURLE_OK == this->getCurlInfo( CURLINFO_CONTENT_TYPE, &contentType ))
			{
				if (!pResponse_->onContentTypeHeader( BW::string( (contentType != NULL) ? contentType : "" ) ))
				{
					shouldAbort_ = true;
					return CURL_READFUNC_ABORT;
				}
			}
		}

		if (!shouldAbort_ && !pResponse_->onData( data, len ))
		{
			shouldAbort_ = true;
			return CURL_READFUNC_ABORT;
		}
	}
	else
	{
		WARNING_MSG( "URL::Request::onRead: Read %zu with no response.\n",
				len );
	}

	return len;
}


/**
 *	This method is called to handle completion of this request.
 */
void Request::onDone( CURLcode code )
{
	MF_ASSERT( pResponse_ );

	BW::string errorMsg;
	const char * pErrMsg = NULL;

	if (code != CURLE_OK)
	{
		errorMsg = this->curlEasyErrorToString( code );
		pErrMsg = errorMsg.c_str();
	}

	pResponse_->onDone( this->responseCode(), pErrMsg );
	pResponse_ = NULL;

	delete this;
}


/**
 *	This method is called to start cancelling the request
 */
void Request::cancel()
{
	// Deleting the request will cause it to be cancelled.
	shouldAbort_ = true;
}


/**
 *	This method sets the URL of the underlying CURL handle.
 *
 *	@param url 	The URL to set.
 *
 *	@return 	true on success, otherwise false.
 */
bool Request::setURL( const BW::string & url )
{
	CURLcode result = this->setCurlOption( CURLOPT_URL, (void*)url.c_str() );
	return result == CURLE_OK;
}


/**
 * 	This method returns the response code for the last time this request
 * 	returned.
 *
 * 	@return The response code, or -1 if it could not be retrieved.
 */
long Request::responseCode() const
{
	if (responseCode_ != 0 &&
		CURLE_OK != this->getCurlInfo( CURLINFO_RESPONSE_CODE, &responseCode_ ))
	{
		return -1;
	}
	return responseCode_;
}


bool Request::fetchURL( const BW::string & url, 
	Response * pResponse, Headers * pHeaders,
	Method method, float connectTimeoutInSeconds, PostData * pPostData )
{
	// Multiple active fetches not yet supported.
	MF_ASSERT( pResponse_ == NULL );

	pResponse_ = pResponse;

	if (!this->setURL( url ))
	{
		ERROR_MSG( "Request::fetchURL: could not set URL: %s\n",
			url.c_str() );
		return false;
	}

	headers_.clear();

	if (pHeaders)
	{
		headers_.add( *pHeaders );
	}

	this->setCurlOption( CURLOPT_HTTPHEADER, headers_.value() );

	switch (method)
	{
	case METHOD_GET:
			this->setCurlOption( CURLOPT_HTTPGET, 1 );
			break;

	case METHOD_PUT:
			this->setCurlOption( CURLOPT_CUSTOMREQUEST, "PUT" );
			this->attachPostData( pPostData );
			break;

	case METHOD_DELETE:
			this->setCurlOption( CURLOPT_CUSTOMREQUEST, "DELETE" );
			break;

	case METHOD_POST:
			this->setCurlOption( CURLOPT_CUSTOMREQUEST, "POST" );
			this->attachPostData( pPostData );
			break;

	case METHOD_PATCH:
			this->setCurlOption( CURLOPT_CUSTOMREQUEST, "PATCH" );
			this->attachPostData( pPostData );
			break;

	default:
			ERROR_MSG( "URLRequest::init: unsupported HTTP method\n" );
			return false;
	}

	CURLcode ret;

	long connectTimeoutInMilliseconds = long( connectTimeoutInSeconds * 1000 );
	ret = this->setCurlOption( CURLOPT_CONNECTTIMEOUT_MS, 
					connectTimeoutInMilliseconds );

	if (ret != CURLE_OK)
	{
			ERROR_MSG( "URL::Request::fetchURL: could not set connect timeout: %s\n",
					this->curlEasyErrorToString( ret ).c_str() );
			return false;
	}

	ret = this->setCurlOption( CURLOPT_TIMEOUT_MS, 
					connectTimeoutInMilliseconds );

	if (ret != CURLE_OK)
	{
			ERROR_MSG( "URL::Request::fetchURL: could not set timeout: %s\n",
					this->curlEasyErrorToString( ret ).c_str() );
			return false;
	}

	return true;
}


/**
 *	This method returns a string describing the given CURLcode error code.
 */
const BW::string Request::curlEasyErrorToString( CURLcode code )
{
	return curl_easy_strerror( code );
}


/**
 * This method encode the PostData to multipart/form-data format.
 */
struct curl_httppost * Request::encodeFormData(const FormData & formData )
{
	struct curl_httppost * pFirstItem = NULL;
	struct curl_httppost * pLastItem = NULL;


	for (FormData::const_iterator iter = formData.begin();
			formData.end() != iter; iter++)
	{
		curl_formadd( &pFirstItem, &pLastItem, CURLFORM_COPYNAME, iter->first.c_str(),
					CURLFORM_COPYCONTENTS, iter->second.c_str(), CURLFORM_END );
	}

	this->curlFormData_ = pFirstItem;

	return pFirstItem;
}


/**
 * This method attaches the post data with the curl request 
 */
void Request::attachPostData( PostData * pPostData )
{
	if (NULL == pPostData) return;

	if (ENCODING_RAW == pPostData->encodingType)
	{
		this->setCurlOption( CURLOPT_POSTFIELDS, pPostData->rawData.c_str() );
	}

	else if (ENCODING_MULTIPART_FORM_DATA == pPostData->encodingType)
	{
		struct curl_httppost * postFormData =
				this->encodeFormData( pPostData->formData );
		this->setCurlOption( CURLOPT_HTTPPOST, postFormData );
	}
}


} // namespace URL

BW_END_NAMESPACE

// request.cpp
