#include "pch.hpp"

#include "http_messages.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include <string>

#include <ctype.h>


BW_BEGIN_NAMESPACE


namespace Mercury
{


// -----------------------------------------------------------------------------
// Section: Utility functions for parsing HTTP messages
// -----------------------------------------------------------------------------
namespace HTTPUtil
{

bool isCRLF( const char * byteData );

bool isLinearWhitespace( const char * byteData, size_t length );

bool skipLinearWhitespace( const char ** pByteData, size_t length );

bool parseQuotedString( const char ** pByteData , size_t length,
	BW::string & unquotedString );

bool isControlCharacter( char ch );
bool isSeparatorCharacter( char ch );
bool isTokenCharacter( char ch );

bool isValidURI( const BW::string & uri );

bool parseStartLine( const BW::string & startLine, BW::string ** ppFields );

bool parseHTTPVersion( const BW::string & versionString, uint & major,
	uint & minor );

/**
 *	This function tests whether the given byte position contains a
 *	carriage-return and line-feed.
 *
 *	@return 	True if the byte data contains a carriage-return and line-feed,
 *				false otherwise.
 */
bool isCRLF( const char * byteData )
{
	return (byteData[0] == '\r') && (byteData[1] == '\n');
}


/**
 *	This function tests whether the given byte is a tab or a space.
 *
 *	@return 	True if the character is a tab or a space, false otherwise.
 */
bool isTabOrSpace( char ch )
{
	return (ch == ' ') || (ch == '\t');
}


/**
 *	This function tests whether the given byte pointer points to linear
 *	whitespace, as defined in RFC2616 section 2.2.
 *
 *	@param byteData 	The byte pointer.
 *	@param length		The length of the byte segment.
 *
 *	@return 			True if the byte segment points to linear whitespace,
 *						false otherwise.
 */
bool isLinearWhitespace( const char * byteData, size_t length )
{
	return ((length >= 1) && isTabOrSpace( *byteData )) ||
			((length >= 3) &&
					(isCRLF( byteData ) && isTabOrSpace( byteData[2] )));
}


/**
 *	This function moves the given byte pointer forward past linear whitespace it
 *	currently points to.
 *
 *	@param pByteData 	A pointer to the byte pointer to move.
 *	@param length 		The length of the byte segment to move within.
 *
 *	@return 			True on success, false if invalid linear whitespace was
 *						encountered.
 */
bool skipLinearWhitespace( const char ** pByteData, size_t length )
{
	const char * byteData =  *pByteData;
	const char * cursor = byteData;

	while ((cursor < (byteData + length)) &&
			isLinearWhitespace( cursor, length - (cursor - byteData) ))
	{
		if (cursor[0] == '\r')
		{
			cursor += 3;
		}
		else
		{
			++cursor;
		}
	}

	if (cursor >= byteData + length)
	{
		return false;
	}

	*pByteData = cursor;

	return true;
}


/**
 *	This function parses a quoted string.
 *
 *	@param pByteData 		The byte pointer. This will be moved on success to
 *							the end of the parsed quoted string.
 *	@param length			The length of the byte segment to parse.
 *	@param unquotedString 	The output unquoted string.
 *
 *	@return 				True on success, false otherwise. The pByteData
 *							pointer will be moved up past the terminating
 *							double quotes character, and quotedString will be
 *							filled with the quoted string without the enclosing
 *							double-quotes characters.
 */
bool parseQuotedString( const char ** pByteData, size_t length,
		BW::string & unquotedString )
{
	const char * byteData = *pByteData;
	const char * cursor = byteData;

	if (cursor[0] != '"')
	{
		return false;
	}

	++cursor;

	uint numBackslashes = 0;
	bool found = false;
	unquotedString.clear();

	while (cursor < (byteData + length) && !found)
	{
		if (*cursor == '\\')
		{
			if (++numBackslashes == 2)
			{
				unquotedString += '\\';
				numBackslashes = 0;
			}
		}
		else if (*cursor == '"')
		{
			if (numBackslashes == 0)
			{
				found = true;
			}
			else
			{
				numBackslashes = 0;
				unquotedString += '"';
			}
		}
		else
		{
			if (numBackslashes != 0)
			{
				return false;
			}

			unquotedString += *cursor;
		}

		++cursor;
	}

	if (cursor == byteData + length)
	{
		return false;
	}

	*pByteData = cursor;

	return true;
}


/**
 *	This function returns whether the given character is a control character,
 *	as defined in RFC2616.
 *
 *	@param ch 	The character to test.
 *
 *	@return 	True if the character is a control character, false otherwise.
 */
bool isControlCharacter( char ch )
{
	static const char MAX_CONTROL_CHAR = 31;
	static const char DELETE_CHAR = 127;
	return (ch <= MAX_CONTROL_CHAR) || (ch == DELETE_CHAR);
}


/**
 *	This function returns whether the given character is a token separator
 *	character, as defined in RFC2616.
 *
 *	@param ch 	The character to test.
 *
 *	@return 	True if the character is a token separator character, false
 *				otherwise.
 */
bool isSeparatorCharacter( char ch )
{
	switch (ch)
	{
	case '(': case ')':
	case '<': case '>':
	case '@':
	case ',':
	case ';':
	case ':':
	case '\\':
	case '"':
	case '/':
	case '[': case ']':
	case '?':
	case '=':
	case '{': case '}':
	case ' ':
	case '\t':
		return true;

	default:
		return false;
	}
}


/**
 *	This function returns whether the given character is a character suitable
 *	for inclusion in a token string.
 *
 *	@param ch 	The character to test.
 *
 *	@return 	True if the character is a token character, false otherwise.
 */
bool isTokenCharacter( char ch )
{
	return !isControlCharacter( ch ) && !isSeparatorCharacter( ch );
}


/**
 *	This function returns whether the given string is a valid URI.
 */
bool isValidURI( const BW::string & uri )
{
	// TODO: Implement full URI parsing.
	// Since this is not required by the WebSockets implementation, we just
	// make sure that there are no control characters present, so we can at
	// least output to log safely.

	const char * pChar = uri.data();
	for (BW::string::size_type pos = 0; pos < uri.size(); ++pos)
	{
		if (isControlCharacter( *pChar ))
		{
			return false;
		}
		++pChar;
	}

	return true;
}


/**
 *	This function parses the HTTP message start line, defined as a text fields
 *	separated by a single space.
 *
 *	@param startLine 	The start line, without the terminating CRLF.
 *	@param ppFields 	A NULL-terminated array of pointers to strings
 *						which will be filled with the parsed field contents.
 *
 *	@return 			True on success, false otherwise.
 */
bool parseStartLine( const BW::string & startLine, BW::string ** ppFields )
{
	const char * cursor = startLine.data();
	BW::string ** ppField = ppFields;

	const char * fieldStart = cursor;
	while ((cursor < (startLine.data() + startLine.size())) &&
			(*(ppField + 1) != NULL))
	{
		if (HTTPUtil::isControlCharacter( *cursor ))
		{
			return false;
		}

		if (*cursor == ' ')
		{
			(*ppField)->assign( fieldStart, cursor - fieldStart );
			++ppField;
			fieldStart = cursor + 1;
		}
		++cursor;
	}

	if (*ppField == NULL)
	{
		return false;
	}

	(*ppField)->assign( fieldStart, startLine.size() -
		(fieldStart - startLine.data()));

	return true;
}


/**
 *	This function parses and validates the HTTP version string into the major
 *	and minor version numbers.
 *
 *	@param httpVersion 	The version string.
 *	@param major 		This will be filled with the HTTP major version number.
 *	@param minor		This will be filled with the HTTP minor version number.
 *
 *	@return 			Whether the given version string parsed and validated
 *						correctly.
 */
bool parseHTTPVersion( const BW::string & httpVersion,
		uint & major, uint & minor )
{
	if (httpVersion.size() < 8)
	{
		return false;
	}

	BW::string prefix = httpVersion.substr( 0, 5 );

	if (bw_strnicmp( prefix.c_str(), "HTTP/", 5 ) != 0)
	{
		return false;
	}

	size_t majorMinorSeparator = httpVersion.find( '.', 5 );
	if (majorMinorSeparator == BW::string::npos)
	{
		return false;
	}

	const char * start = httpVersion.data() + 5;
	char * endPtr = NULL;
	major = strtoul( start, &endPtr, 10 );

	if (size_t( endPtr - httpVersion.data() ) != majorMinorSeparator)
	{
		return false;
	}

	minor = strtoul( httpVersion.data() + majorMinorSeparator + 1,
		&endPtr, 10 );

	if (size_t( endPtr - httpVersion.data() ) != httpVersion.size())
	{
		return false;
	}

	return true;
}


} // end namespace HTTPUtil


// -----------------------------------------------------------------------------
// Section: Case-insensitive string char_traits
// -----------------------------------------------------------------------------

/**
 *	This struct is used to implement strings that compare case-insensitively.
 */
struct case_insensitive_char_traits : public std::char_traits< char >
{
	/**
	 *	Return if the two characters are equivalent (under case-insensitivity).
	 */
	static bool eq( char a, char b )
	{ return tolower( a ) == tolower( b ); }

	/**
	 *	Return if the two characters are not equivalent (under
	 *	case-insensitivity).
	 */
	static bool ne( char a, char b )
	{ return !eq( a, b ); }

	/**
	 *	Return whether the order of one character is lexicographically and
	 *	case-insensitively less than the order of another character.
	 */
	static bool lt( char a, char b )
	{ return tolower( a ) < tolower( b ); }


	/**
	 *	Comparison function of two strings of a given size.
	 *
	 *	@return 	-1 if p is less than q, 0 if p is equivalent to q, and 1 
	 *				if p is greater than q.
	 */
	static int compare( const char * p, const char * q, size_t n )
	{
		if (n == 0)
		{
			return std::char_traits< char >::compare( p, q, n );
		}

		while (n-- != 0)
		{
			if ((*p == '\0') && (*q != '\0'))
			{
				return -1;
			}
			if ((*p != '\0')  && (*q == '\0'))
			{
				return 1;
			}
			if (lt( *p, *q ))
			{
				return -1;
			}
			if (lt( *q, *p ))
			{
				return 1;
			}
			++p;
			++q;
		}
		return 0;
	}


	/**
	 *	This method finds a character index within a string,
	 *	case-insensitively.
	 *
	 *	@param p 	The character string.
	 *	@param n	The size of the string.
	 *	@param a	The character to find.
	 */
	static const char * find( const char * p, size_t n, char a )
	{
		if (n == 0)
		{
			return std::char_traits< char >::find( p, n, a );
		}

		while ((n-- != 0) && ne( *p, a ))
		{
			++p;
		}
		return p;
	}
};

typedef std::basic_string< char, case_insensitive_char_traits, 
		BW::StlAllocator< char > >
	case_insensitive_string;


// -----------------------------------------------------------------------------
// Section: HTTPHeaders
// -----------------------------------------------------------------------------

/**
 *	This class is used to implement the HTTPHeaders class.
 */
class HTTPHeaders::Impl
{
public:
	Impl();
	Impl( const Impl & other );
	Impl& operator=( const Impl & other );
	~Impl();

	bool parse( const char ** pByteData, size_t length );

	/**
	 *	This method clears the parsed data.
	 */
	void clear() { container_.clear(); }


	bool contains( const BW::string & headerName ) const;
	const BW::string & valueFor( const BW::string & headerName ) const;

	bool valueCaseInsensitivelyEquals( const BW::string & headerName,
			const BW::string & headerValue ) const;
	bool valueCaseInsensitivelyContains( const BW::string & headerName,
			const BW::string & headerValue ) const;
private:

	bool parseHeaderLine( const char ** pByteData, size_t length );
	bool parseHeaderName( const char ** pByteData, size_t length,
		case_insensitive_string & headerName );
	bool parseHeaderValue( const char ** pByteData, size_t length,
		BW::string & headerValue );


	typedef BW::map< case_insensitive_string, BW::string > Container;
	Container container_;
};


/**
 *	Constructor.
 */
HTTPHeaders::Impl::Impl() :
		container_()
{}


/**
 *	Copy constructor.
 */
HTTPHeaders::Impl::Impl( const HTTPHeaders::Impl & other ) :
		container_( other.container_ )
{}


/**
 *	Assignment operator.
 */
HTTPHeaders::Impl & HTTPHeaders::Impl::operator=( 
		const HTTPHeaders::Impl & other )
{
	container_ = other.container_;
	return *this;
}


/**
 *	Destructor.
 */
HTTPHeaders::Impl::~Impl()
{}


/**
 *	This method is used to parse the headers from the given byte data.
 *
 *	@param pByteData 	The byte data pointer. This pointer will be moved on
 *						success to the end of the parsed data.
 *	@param length 		The length of the byte data to parse.
 *
 *	@return 			True on success, false otherwise.
 */
bool HTTPHeaders::Impl::parse( const char ** pByteData, size_t length )
{
	this->clear();

	const char * byteData = *pByteData;
	const char * cursor = byteData;

	while (cursor < (byteData + length) && ! HTTPUtil::isCRLF( cursor ))
	{
		if (!this->parseHeaderLine( &cursor, length - (cursor - byteData) ))
		{
			return false;
		}
	}

	if (HTTPUtil::isCRLF( cursor ))
	{
		cursor += 2;
		*pByteData = cursor;
		return true;
	}

	return false;
}


/**
 *	This method parses a single header line.
 *
 *	@param pByteData 	The byte data pointer, moved on success to past the end
 *						of the parsed line.
 *	@param length 		The length of the byte data.
 *
 *	@return 			True on success, false otherwise.
 */
bool HTTPHeaders::Impl::parseHeaderLine( const char ** pByteData,
		size_t length )
{
	const char * byteData = *pByteData;
	const char * cursor = byteData;

	case_insensitive_string headerName;
	if (!this->parseHeaderName( &cursor, length, headerName ))
	{
		return false;
	}

	BW::string headerValue;
	if (!this->parseHeaderValue( &cursor, length - (cursor - byteData),
			headerValue ))
	{
		return false;
	}

	std::pair< Container::iterator, bool > result = 
		container_.insert( Container::value_type( headerName, headerValue ) );

	if (!result.second)
	{
		// Multiple header instances are concatenated using a comma to separate
		// values, which is an alternative way of specifying the multiple
		// values under the HTTP spec.

		result.first->second += "," + headerValue;
	}

	*pByteData = cursor;
	return true;
}


/**
 *	This method parses the header field name.
 *
 *	@param pByteData 		The byte data pointer, moved on success to past the
 *							end of the header name/value separator.
 *	@param length 			The length of the byte data.
 *	@param headerName 		The output parsed header name.
 *
 *	@return 				True on success, false otherwise.
 */
bool HTTPHeaders::Impl::parseHeaderName( const char ** pByteData,
		size_t length, case_insensitive_string & headerName )
{
	const char * byteData = *pByteData;
	const char * cursor = byteData;

	const char * headerNameStart = cursor;
	while (cursor < (byteData + length) &&
			HTTPUtil::isTokenCharacter( *cursor ))
	{
		++cursor;
	}

	if ((cursor == (byteData + length)) || (cursor[0] != ':'))
	{
		return false;
	}

	headerName.assign( headerNameStart, cursor - headerNameStart );
	++cursor;
	*pByteData = cursor;
	return true;
}


/**
 *	This method parses the header field value.
 *
 *	Any non-leading linear whitespace will be replaced by a single space, as
 *	per RFC2616. Leading linear whitespace will be ignored.
 *
 *	@param pByteData 	The byte pointer, moved on success to past the end of
 *						the terminating CRLF.
 *	@param length 		The length of the byte data.
 *	@pawram headerValue The output parsed header value.
 *
 *	@return 			True on success, false otherwise.
 */
bool HTTPHeaders::Impl::parseHeaderValue( const char ** pByteData, 
		size_t length, BW::string & headerValue )
{
	const char * byteData = *pByteData;
	const char * cursor = byteData;

	const char * headerValueChunkStart = cursor;
	BW::ostringstream headerValueStream;
	bool leadingLWS = true;
	while (cursor < (byteData + length) &&
			!HTTPUtil::isCRLF( cursor ))
	{
		if (HTTPUtil::isLinearWhitespace( cursor,
				length - (cursor - byteData) ))
		{
			if (!leadingLWS)
			{
				// Replace any non-leading LWS with a single space, as per
				// RFC2616 section 4.2.
				headerValueStream.write( headerValueChunkStart,
					cursor - headerValueChunkStart );
				headerValueStream.put( ' ' );
			}

			if (!HTTPUtil::skipLinearWhitespace( &cursor,
					length - (cursor - byteData) ))
			{
				return false;
			}

			headerValueChunkStart = cursor;
		}
		else if (*cursor == '"')
		{
			BW::string quotedString;
			if (!HTTPUtil::parseQuotedString( &cursor,
					length - (cursor - byteData ), quotedString ))
			{
				return false;
			}

			headerValueStream << quotedString;
			headerValueChunkStart = cursor;
		}
		else if (HTTPUtil::isControlCharacter( *cursor ))
		{
			return false;
		}
		else
		{
			++cursor;
		}
		leadingLWS = false;
	}


	if (cursor >= (byteData + length))
	{
		return false;
	}

	headerValueStream.write( headerValueChunkStart,
		cursor - headerValueChunkStart );

	headerValueStream.str().swap( headerValue );

	*pByteData = cursor + 2;
	return true;
}


/**
 *	This method returns whether the given header name exists in the parsed
 *	header data.
 *
 *	@param headerName 	The name of the header, compared case-insensitively.
 *
 *	@return 			True if a parsed header exists with the given name,
 *						false otherwise.
 */
bool HTTPHeaders::Impl::contains( const BW::string & headerName ) const
{
	case_insensitive_string caseInsensitiveHeaderName( headerName.data(),
			headerName.size() );
	return (container_.count( caseInsensitiveHeaderName ) > 0);
}


/**
 *	This method returns the header value for the given name. If no such header
 *	exists, an empty string is returned.
 *
 *	@param headerName 	The name of the header, compared case-insensitively.
 *
 *	@return 			The header value for the given header name, or empty
 *						string if no such header exists with the given name.
 */
const BW::string & HTTPHeaders::Impl::valueFor(
		const BW::string & headerName ) const
{
	static const BW::string EMPTY_VALUE( "" );

	case_insensitive_string caseInsensitiveHeaderName( headerName.data(),
			headerName.size() );

	Container::const_iterator iHeaderLine =
		container_.find( caseInsensitiveHeaderName );

	if (iHeaderLine == container_.end())
	{
		return EMPTY_VALUE;
	}

	return iHeaderLine->second;
}


/**
 *	This method tests whether the given string is equal under
 *	case-insensitivity to the header value for the given header name.
 *
 *	@param headerNameString 		The header name.
 *	@param testHeaderValueString 	The string to compare the header value
 *									against, case-insensitively.
 *
 *	@return 						True if they compare equally under
 *									case-insensitivity, false otherwise.
 */
bool HTTPHeaders::Impl::valueCaseInsensitivelyEquals(
		const BW::string & headerNameString,
		const BW::string & testHeaderValueString ) const
{
	case_insensitive_string caseInsensitiveHeaderName( headerNameString.data(),
			headerNameString.size() );
	Container::const_iterator iHeaderLine =
		container_.find( caseInsensitiveHeaderName );

	if (iHeaderLine == container_.end())
	{
		return testHeaderValueString.empty();
	}

	case_insensitive_string foundHeaderValue( iHeaderLine->second.data(),
			iHeaderLine->second.size() );
	case_insensitive_string testHeaderValue( testHeaderValueString.data(),
			testHeaderValueString.size() );

	return (foundHeaderValue == testHeaderValue);
}


/**
 *	This method case-insensitively tests whether the given string is contained
 *	in the header value for the given header name.
 *
 *	@param headerNameString 		The header name.
 *	@param testHeaderValueString 	The string to compare the header value
 *									against, case-insensitively.
 *
 *	@return 						True if the header value contains the given
 *									string, false otherwise.
 */
bool HTTPHeaders::Impl::valueCaseInsensitivelyContains(
		const BW::string & headerNameString,
		const BW::string & testHeaderValueString ) const
{
	const BW::string & headerValue = this->valueFor( headerNameString );

	case_insensitive_string caseInsensitiveHeaderValue( headerValue.data(),
		headerValue.size() );

	case_insensitive_string testHeaderValue( testHeaderValueString.data(),
			testHeaderValueString.size() );

	return (caseInsensitiveHeaderValue.find( testHeaderValue ) != 
		case_insensitive_string::npos);
}


/**
 *	Constructor.
 */
HTTPHeaders::HTTPHeaders() :
		pImpl_( new Impl() )
{}


/**
 *	Copy constructor.
 */
HTTPHeaders::HTTPHeaders( const HTTPHeaders & other ) :
		pImpl_( new Impl( *(other.pImpl_) ) )
{}


/**
 *	Assignment operator.
 */
HTTPHeaders & HTTPHeaders::operator=( const HTTPHeaders & other )
{
	*pImpl_ = *(other.pImpl_);
	return *this;
}


/**
 *	Destructor.
 */
HTTPHeaders::~HTTPHeaders()
{
	bw_safe_delete( pImpl_ );
}


/**
 *	This method is used to parse the headers from the given byte data.
 *
 *	@param pByteData 	The byte data pointer. This pointer will be moved on
 *						success to the end of the parsed data.
 *	@param length 		The length of the byte data to parse.
 *
 *	@return 			True on success, false otherwise.
 */
bool HTTPHeaders::parse( const char ** pByteData, size_t length )
{
	return pImpl_->parse( pByteData, length );
}


/**
 *	This method clears the parsed data.
 */
void HTTPHeaders::clear()
{
	return pImpl_->clear();
}


/**
 *	This method returns whether the given header name exists in the parsed
 *	header data.
 *
 *	@param headerName 	The name of the header, compared case-insensitively.
 *
 *	@return 			True if a parsed header exists with the given name,
 *						false otherwise.
 */
bool HTTPHeaders::contains( const BW::string & headerName ) const
{
	return pImpl_->contains( headerName );
}


/**
 *	This method returns the header value for the given name. If no such header
 *	exists, an empty string is returned.
 *
 *	@param headerName 	The name of the header, compared case-insensitively.
 *
 *	@return 			The header value for the given header name, or empty
 *						string if no such header exists with the given name.
 */
const BW::string & HTTPHeaders::valueFor( const BW::string & headerName ) const
{
	return pImpl_->valueFor( headerName );
}


/**
 *	This method tests whether the given string is equal under
 *	case-insensitivity to the header value for the given header name.
 *
 *	@param headerNameString 		The header name.
 *	@param testHeaderValueString 	The string to compare the header value
 *									against, case-insensitively.
 *
 *	@return 						True if they compare equally under
 *									case-insensitivity, false otherwise.
 */
bool HTTPHeaders::valueCaseInsensitivelyEquals(
		const BW::string & headerNameString,
		const BW::string & testHeaderValueString ) const
{
	return pImpl_->valueCaseInsensitivelyEquals( headerNameString,
		testHeaderValueString );
}


/**
 *	This method case-insensitively tests whether the given string is contained
 *	in the header value for the given header name.
 *
 *	@param headerNameString 		The header name.
 *	@param testHeaderValueString 	The string to compare the header value
 *									against, case-insensitively.
 *
 *	@return 						True if the header value contains the given
 *									string, false otherwise.
 */
bool HTTPHeaders::valueCaseInsensitivelyContains(
		const BW::string & headerNameString,
		const BW::string & testHeaderValueString ) const
{
	return pImpl_->valueCaseInsensitivelyContains( headerNameString,
		testHeaderValueString );
}


// -----------------------------------------------------------------------------
// Section: HTTPMessage, HTTPRequest, HTTPResponse
// -----------------------------------------------------------------------------

/**
 *	This method parses an HTTP message from the given byte data.
 *
 *	@param pByteData 	The byte data pointer. On success, this will be moved
 *						to just after the parsed message data.
 *	@param length 		The length of the byte data.
 *
 *	@return 			True on success, false otherwise.
 */
bool HTTPMessage::parse( const char ** pByteData, size_t length )
{
	const char * byteData = *pByteData;
	const char * cursor = byteData;

	// Ignore empty lines as per RFC2616 section 4.1.
	while ((length - (cursor - byteData) >= 2) && HTTPUtil::isCRLF( cursor ))
	{
		cursor += 2;
	}

	if (cursor == (byteData + length))
	{
		return false;
	}

	const char * startLineStart = cursor;
	while (cursor < (byteData + length) && !HTTPUtil::isCRLF( cursor ))
	{
		++cursor;
	}
	if (cursor == (byteData + length))
	{
		return false;
	}

	startLine_.assign( startLineStart, cursor - startLineStart );

	cursor += 2;

	if (!headers_.parse( &cursor, length - (cursor - byteData) ))
	{
		return false;
	}

	*pByteData = cursor;
	return true;
}


/*
 *	Override from HTTPMessage.
 */
bool HTTPRequest::parse( const char ** pByteData, size_t length )
{
	if (!this->HTTPMessage::parse( pByteData, length ))
	{
		return false;
	}

	BW::string * fields[] = {
		&method_,
		&uri_,
		&httpVersion_,
		NULL
	};

	if (!HTTPUtil::parseStartLine( startLine_, fields ))
	{
		return false;
	}

	// Validate parsed method.
	const char * pChar = method_.data();
	for (BW::string::size_type pos = 0; pos < method_.size(); ++pos)
	{
		if (!HTTPUtil::isTokenCharacter( *pChar ))
		{
			return false;
		}
		++pChar;
	}

	if (!HTTPUtil::isValidURI( uri_ ))
	{
		return false;
	}

	uint major;
	uint minor;
	return HTTPUtil::parseHTTPVersion( httpVersion_, major, minor );
}


/**
 *	This method returns the HTTP version as reported from the HTTP request.
 *
 *	@param major 	The major version.
 *	@param minor 	The minor version.
 *
 *	@return 		Whether a valid HTTP version string was parsed.
 */
bool HTTPRequest::getHTTPVersion( uint & major, uint & minor ) const
{
	return HTTPUtil::parseHTTPVersion( httpVersion_, major, minor );
}


/*
 *	Override from HTTPMessage.
 */
bool HTTPResponse::parse( const char ** pByteData, size_t length )
{
	if (!this->HTTPMessage::parse( pByteData, length ))
	{
		return false;
	}

	BW::string * fields[] = {
		&httpVersion_,
		&statusCode_,
		&reasonPhrase_,
		NULL
	};

	if (!HTTPUtil::parseStartLine( startLine_, fields ))
	{
		return false;
	}

	const char * pChar = statusCode_.data();
	for (BW::string::size_type pos = 0; pos < statusCode_.size(); ++pos)
	{
		if (!isdigit( *pChar ))
		{
			return false;
		}
		++pChar;
	}

	uint major;
	uint minor;
	return HTTPUtil::parseHTTPVersion( httpVersion_, major, minor );
}


/**
 *	This method returns the HTTP version as reported from the HTTP response.
 *
 *	@param major 	The major version.
 *	@param minor 	The minor version.
 *
 *	@return 		Whether a valid HTTP version string was parsed.
 */
bool HTTPResponse::getHTTPVersion( uint & major, uint & minor ) const
{
	return HTTPUtil::parseHTTPVersion( httpVersion_, major, minor );
}


} // end namespace Mercury

BW_END_NAMESPACE


// http_messages.cpp
