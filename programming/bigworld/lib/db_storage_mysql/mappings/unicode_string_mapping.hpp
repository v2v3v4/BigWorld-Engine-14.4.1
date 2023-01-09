#ifndef MYSQL_UNICODE_STRING_MAPPING_HPP
#define MYSQL_UNICODE_STRING_MAPPING_HPP

#include "string_like_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class maps the UNICODE_STRING type to the database.
 *
 *	Unicode strings are stored as UTF-8 in the database for which MySQL
 *	requires 3 bytes being set aside for every encoded character.
 */
class UnicodeStringMapping : public StringLikeMapping
{
private:
	// For calculating UTF8 character start offsets.
	typedef BW::vector< uint > Offsets;

public:
	UnicodeStringMapping( const Namer & namer, const BW::string & propName,
			ColumnIndexType indexType, uint charLength,
			DataSectionPtr pDefaultValue );

	static void getUTF8CharOffsets( const BW::string & s, Offsets & offsets );

	virtual bool isBinary() const	{ return false; }
	virtual enum_field_types getColumnType() const;
};

BW_END_NAMESPACE

#endif // MYSQL_UNICODE_STRING_MAPPING_HPP
