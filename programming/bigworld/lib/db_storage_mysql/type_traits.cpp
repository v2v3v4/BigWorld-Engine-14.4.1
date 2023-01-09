#include "type_traits.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class MySqlTypeTraits
// -----------------------------------------------------------------------------

const BW::string MySqlTypeTraits<BW::string>::TINYBLOB( "TINYBLOB" );
const BW::string MySqlTypeTraits<BW::string>::BLOB( "BLOB" );
const BW::string MySqlTypeTraits<BW::string>::MEDIUMBLOB( "MEDIUMBLOB" );
const BW::string MySqlTypeTraits<BW::string>::LONGBLOB( "LONGBLOB" );

BW_END_NAMESPACE

// type_traits.cpp
