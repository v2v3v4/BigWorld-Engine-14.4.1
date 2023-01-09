#ifndef XML_SPECIAL_CHARS_HPP
#define XML_SPECIAL_CHARS_HPP

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class is used to convert between special XML characters and plain
 *  text. An example of this is converting the "less-than" character between
 *  the plain text character "<" and the XML escape sequence "&lt;".
 */
class XmlSpecialChars
{
public:
	// Converts ampersand-based sequences into the actual characters.
	static void reduce( char* buf );

	// Converts the invalid characters to ampersand-based sequences.
	// buf must have enough room for the expanded characters to fit.
	static BW::string expand( const char* buf );
};

BW_END_NAMESPACE

#endif // XML_SPECIAL_CHARS_HPP
