#include "script/first_include.hpp"

#include "blobbed_sequence_mapping.hpp"

#include "../utils.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

BlobbedSequenceMapping::BlobbedSequenceMapping( const Namer & namer,
								const BW::string & propName,
								PropertyMappingPtr child,
								int size,
								int dbLen ) :
		StringLikeMapping( namer, propName, INDEX_TYPE_NONE, dbLen )
{
	// Set the default value
	MemoryOStream strm;
	defaultSequenceToStream( strm, size, child );
	int len = strm.remainingLength();
	defaultValue_.assign( (char *) strm.retrieve(len), len );
}

BW_END_NAMESPACE

// blobbed_sequence_mapping.cpp
