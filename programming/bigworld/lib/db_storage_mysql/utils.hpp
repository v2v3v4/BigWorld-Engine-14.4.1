#include "cstdmf/bw_string.hpp"

#include "mappings/property_mapping.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;

BW::string buildCommaSeparatedQuestionMarks( int num );

void defaultSequenceToStream( BinaryOStream & strm, int seqSize,
		const PropertyMappingPtr & pChildMapping_ );

BW::string createInsertStatement( const BW::string & tbl,
		const PropertyMappings & properties );

BW::string createExplicitInsertStatement( const BW::string & tbl,
		const PropertyMappings & properties );

BW::string createUpdateStatement( const BW::string & tbl,
		const PropertyMappings & properties );

BW::string createSelectStatement( const BW::string & tbl,
		const PropertyMappings & properties,
		const BW::string & where );

BW_END_NAMESPACE

// mysql_utils.hpp
