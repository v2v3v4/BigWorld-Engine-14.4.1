#ifndef MYSQL_BLOBBED_SEQUENCE_MAPPING_HPP
#define MYSQL_BLOBBED_SEQUENCE_MAPPING_HPP

#include "string_like_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This class maps sequences (like ARRAY and TUPLE) to a BLOB column in the
 * 	database.
 */
class BlobbedSequenceMapping : public StringLikeMapping
{
public:
	BlobbedSequenceMapping( const Namer & namer, const BW::string & propName,
				PropertyMappingPtr child, int size, int dbLen );

	virtual bool isBinary() const	{ return true; }
};

BW_END_NAMESPACE

#endif // MYSQL_BLOBBED_SEQUENCE_MAPPING_HPP
