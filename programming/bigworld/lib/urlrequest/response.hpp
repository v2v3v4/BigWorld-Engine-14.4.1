#ifndef URL_RESPONSE_HPP
#define URL_RESPONSE_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include <sstream>

BW_BEGIN_NAMESPACE

namespace URL
{

static const int RESPONSE_ERR_OK = 200;

const char * httpResponseCodeToString( long responseCode );

/**
 *	Interface for a handler for a URL request.
 */
class Response : public ReferenceCount
{
public:
	/**
	 *	This method is called when the HTTP response code is known.
	 */
	virtual bool onResponseCode( long responseCode ) { return true; }

	/**
	 *	This method is called when the Content-Type HTTP header is known.
	 *
	 *	@param contentType 	The reported Content-Type, or empty string if none
	 *						was supplied.
	 */
	virtual bool onContentTypeHeader( const BW::string & contentType )
	{
		return true;
	}

	/**
	 *	This method is called when body data has been read.
	 *
	 *	@param data		A chunk of the body data.
	 *	@param len		The size of the body data chunk.
	 */
	virtual bool onData( const char * data, size_t len ) = 0;

	/**
	 *	This method is called when the request is completed.
	 */
	virtual void onDone( int responseCode, const char * error ) = 0;
};

typedef SmartPointer< Response > ResponsePtr;


/**
 *	This convenience class writes out all received data into a
 *	string stream member, accessible by subclasses.
 */
class SimpleResponse : public Response
{
public:
	/** Constructor. */
	SimpleResponse() :
		body_()
	{}

	/** Destructor. */
	virtual ~SimpleResponse() {}

	/*
	 *	Override from Response.
	 */
	virtual bool onData( const char * data, size_t len )
	{
		body_.write( data, len );
		return true;
	}

protected:
	BW::ostringstream body_;
};

} // namespace URL

BW_END_NAMESPACE

#endif // URL_RESPONSE_HPP
