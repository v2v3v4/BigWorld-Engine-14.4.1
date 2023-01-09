#include "script/first_include.hpp"

#include "unicode_string_mapping.hpp"

#include "cstdmf/debug.hpp"
#include "resmgr/datasection.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UnicodeStringMapping::UnicodeStringMapping( const Namer & namer,
		const BW::string & propName,
		ColumnIndexType indexType,
		uint charLength,
		DataSectionPtr pDefaultValue ) :
	StringLikeMapping( namer, propName, indexType,
		/*charLength=*/ charLength,
		/*byteLength=*/ charLength * 3 )
			// MySQL requires 3 bytes for each character
{
	if (pDefaultValue)
	{
		defaultValue_ = pDefaultValue->as<BW::string>();
		Offsets offsets;

		this->getUTF8CharOffsets( defaultValue_, offsets );

		// The size of the offsets vector is the character length of the
		// UTF8 string.

		if (offsets.size() > charLength_)
		{
			// Truncate at the first byte of the first character above
			// the character length.
			defaultValue_.resize( offsets[charLength_] );
			WARNING_MSG( "StringMapping::StringMapping: "
					"Default value (UTF8) for property %s has been "
					"truncated to '%s'\n",
				propName.c_str(), defaultValue_.c_str() );
		}
	}
}


/**
 *	Calculate the start byte offsets of each UTF-8 encoded character in the
 *	given string.
 *
 *	@param s 		The UTF-8 encoded string.
 *	@param offsets 	A Offsets structure, which is cleared and sequentially
 *					filled with the offsets of each character in the
 *					encoded string.
 */
void UnicodeStringMapping::getUTF8CharOffsets( const BW::string & s,
		Offsets & offsets )
{
	offsets.clear();
	uint byteLength = 0;
	size_t i = 0;

	while (i < s.size())
	{
		// Determine length of this variable length encoded UTF8
		// character.
		uint8 ch = s[i];
		static const unsigned char UTF8_1_BYTE = 0x00;
		static const unsigned char UTF8_2_BYTES = 0xC0;
		static const unsigned char UTF8_3_BYTES = 0xE0;
		static const unsigned char UTF8_4_BYTES = 0xF0;

		if 		(uint8( ch & 0x80 ) == UTF8_1_BYTE)	 	byteLength = 1;
		else if (uint8( ch & 0xE0 ) == UTF8_2_BYTES) 	byteLength = 2;
		else if (uint8( ch & 0xF0 ) == UTF8_3_BYTES) 	byteLength = 3;
		else if (uint8( ch & 0xF8 ) == UTF8_4_BYTES) 	byteLength = 4;
		else
		{
			CRITICAL_MSG( "Invalid UTF-8 character start byte\n" );
		}

		offsets.push_back( i );
		i += byteLength;
	}
}


/*
 *	Override from StringLikeMapping.
 */
enum_field_types UnicodeStringMapping::getColumnType() const
{
	// __kyl__ (24/7/2006) Special handling of STRING < 255 characters
	// because this is how we magically pass the size of the name index
	// field. If type is not VARCHAR then index size is assumed to be
	// 255 (see createEntityTableIndex()).
	return (charLength_ < 256) ?
		MYSQL_TYPE_VAR_STRING :
		this->StringLikeMapping::getColumnType();
}

BW_END_NAMESPACE

// unicode_string_mapping.cpp
