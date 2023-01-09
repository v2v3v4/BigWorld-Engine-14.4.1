#ifndef URL_REQUEST_HPP
#define URL_REQUEST_HPP

#include "urlrequest/misc.hpp"
#include "urlrequest/string_list.hpp"

#include "curl/curl.h"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace URL
{

class Manager;
class Response;
typedef SmartPointer< Response > ResponsePtr;

/**
 *	This class wraps a CURL * handle.
 */
class Request
{
public:
	Request();
	~Request();

	size_t onRead( const char * data, size_t len );
	void onDone( CURLcode result );

	void cancel();

	bool fetchURL( const BW::string & url, 
		Response * pResponse, Headers * pHeaders,
		Method method, float connectTimeoutInSeconds, PostData * pPostData );

	void setManager( Manager * pManager ) { pManager_ = pManager; }

	/**
	 *	This method sets a curl option on the handle.
	 */
	template < typename T >
	CURLcode setCurlOption( CURLoption option, T param )
	{
		return curl_easy_setopt( curlHandle_, option, param );
	}

	/**
	 *	This method gets info from a curl handle.
	 */
	template< typename T >
	CURLcode getCurlInfo( CURLINFO info, T * pParam ) const
	{
		return curl_easy_getinfo( curlHandle_, info, pParam );
	}

	bool setURL( const BW::string & url );

	long responseCode() const;

	/**
	 *	Return the underlying cURL handle.
	 */
	CURL * handle() const { return curlHandle_; }

	static const BW::string curlEasyErrorToString( CURLcode code );

private:
	// Disallow copying.
	Request( const Request & );
	Request & operator=( const Request & );
	struct curl_httppost * encodeFormData( const FormData & FormData );
	void attachPostData(  PostData * pPostData );

	CURL * curlHandle_;

	ResponsePtr pResponse_;
	Manager * pManager_;
	StringList headers_;
	long responseCode_;
	bool shouldAbort_;
	bool haveReadContentType_;
	struct curl_httppost * curlFormData_;
};

} // namespace URL

BW_END_NAMESPACE

#endif // URL_REQUEST_HPP
