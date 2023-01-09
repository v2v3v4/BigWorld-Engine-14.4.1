#ifndef MYSQL_BLOB_MAPPING_HPP
#define MYSQL_BLOB_MAPPING_HPP

#include "cstdmf/bw_namespace.hpp"

#include "string_like_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class maps the BLOB type to the database.
 */
class BlobMapping : public StringLikeMapping
{
public:
	BlobMapping( const Namer & namer, const BW::string & propName,
			ColumnIndexType indexType, int length, 
			DataSectionPtr pDefaultValue );

	virtual bool isBinary() const	{ return true; }

	// This method gets the section data as a base64 encoded string
	// and decodes it, placing the result in output.
	static void decodeSection( BW::string & output, DataSectionPtr pSection );
};

BW_END_NAMESPACE

#endif // MYSQL_BLOB_MAPPING_HPP
