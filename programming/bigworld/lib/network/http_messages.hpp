#ifndef HTTP_MESSAGES_HPP
#define HTTP_MESSAGES_HPP


#include "cstdmf/bw_string.hpp"

#if defined( __unix__ )
#include <unistd.h>
#endif

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{


/**
 *	This class is used to parse the headers in a HTTP message.
 */
class HTTPHeaders
{
public:
	HTTPHeaders();

	HTTPHeaders( const HTTPHeaders & other );

	HTTPHeaders & operator=( const HTTPHeaders & other );

	~HTTPHeaders();

	bool parse( const char ** pByteData, size_t length );
	void clear();

	bool contains( const BW::string & headerName ) const;
	const BW::string & valueFor( const BW::string & headerName ) const;
	bool valueCaseInsensitivelyEquals( const BW::string & headerName,
			const BW::string & headerValue ) const;
	bool valueCaseInsensitivelyContains( const BW::string & headerName,
			const BW::string & headerValue ) const;
private:
	class Impl;
	Impl * pImpl_;

};



/**
 *	This class is used to parse an HTTP message.
 */
class HTTPMessage
{
public:
	HTTPMessage() :
			startLine_(),
			headers_()
	{}

	virtual ~HTTPMessage()
	{}

	virtual bool parse( const char ** pByteData, size_t length );

	const BW::string & startLine() const
	{ return startLine_; }

	const HTTPHeaders & headers() const
	{ return headers_; }

protected:
	BW::string startLine_;
	HTTPHeaders headers_;
};


/*
 *	This class is used to parse an HTTP request message.
 */
class HTTPRequest : public HTTPMessage
{
public:
	HTTPRequest():
		HTTPMessage(),
		method_(),
		uri_(),
		httpVersion_()
	{}


	virtual ~HTTPRequest()
	{}

	virtual bool parse( const char ** pByteData, size_t length );

	const BW::string & method() const
	{ return method_; }

	const BW::string & uri() const
	{ return uri_; }

	const BW::string & httpVersionString() const
	{ return httpVersion_; }

	bool getHTTPVersion( uint & major, uint & minor ) const;

private:
	BW::string method_;
	BW::string uri_;
	BW::string httpVersion_;
};


/*
 *	This class is used to parse an HTTP response message.
 */
class HTTPResponse : public HTTPMessage
{
public:
	HTTPResponse():
		HTTPMessage(),
		httpVersion_(),
		statusCode_(),
		reasonPhrase_()
	{}

	virtual ~HTTPResponse()
	{}

	virtual bool parse( const char ** pByteData, size_t length );

	const BW::string & httpVersion() const
	{ return httpVersion_; }

	const BW::string & statusCode() const
	{ return statusCode_; }

	const BW::string & reasonPhrase() const
	{ return reasonPhrase_; }

	bool getHTTPVersion( uint & major, uint & minor ) const;

private:
	BW::string httpVersion_;
	BW::string statusCode_;
	BW::string reasonPhrase_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // HTTP_MESSAGES_HPP
