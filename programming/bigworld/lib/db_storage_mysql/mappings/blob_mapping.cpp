#include "script/first_include.hpp"

#include "blob_mapping.hpp"

#include "cstdmf/base64.h"
#include "cstdmf/debug.hpp"

#include "resmgr/datasection.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
BlobMapping::BlobMapping( const Namer & namer, const BW::string & propName,
		ColumnIndexType indexType, int length, 
		DataSectionPtr pDefaultValue ) :
	StringLikeMapping( namer, propName, indexType, length )
{
	if (pDefaultValue)
	{
		BlobMapping::decodeSection( defaultValue_, pDefaultValue );

		if (defaultValue_.length() > charLength_)
		{
			defaultValue_.resize( charLength_ );
			ERROR_MSG( "BlobMapping::BlobMapping: "
						"Default value for property %s has been truncated\n",
					propName.c_str() );
		}
	}
}


/**
 *	This method gets the section data as a base64 encoded string and decodes it,
 *	placing the result in output.
 */
void BlobMapping::decodeSection( BW::string & output, DataSectionPtr pSection )
{
	output = pSection->as<BW::string>();
	int len = output.length();
	if (len <= 256)
	{
		// Optimised for small strings.
		char decoded[256];
		int length = Base64::decode( output, decoded, 256 );
		output.assign(decoded, length);
	}
	else
	{
		char * decoded = new char[ len ];
		int length = Base64::decode( output, decoded, len );
		output.assign(decoded, length);
		delete [] decoded;
	}
}

BW_END_NAMESPACE

// blob_mapping.cpp
