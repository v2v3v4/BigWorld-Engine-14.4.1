#include "converter_dependency.hpp"

BW_BEGIN_NAMESPACE

namespace ConverterDependency_Locals
{
	// XML tag names
	const char * ID_TAG = "Id";
	const char * VERSION_TAG = "Version";
}

ConverterDependency::ConverterDependency()
{
}

ConverterDependency::ConverterDependency( uint64 converterId, const BW::string & converterVersion )
	: converterId_( converterId )
	, converterVersion_( converterVersion )
{
}

ConverterDependency::~ConverterDependency()
{
}

bool ConverterDependency::serialiseIn( DataSectionPtr pSection )
{
	// Search for the id tag
	DataSectionPtr idPtr = pSection->findChild( ConverterDependency_Locals::ID_TAG );
	if (!idPtr || !idPtr->isAttribute())
	{
		// Could not find the id tag,
		// there is an error with this data section
		return false;
	}

	// Search for the version tag
	DataSectionPtr versionPtr = pSection->findChild( ConverterDependency_Locals::VERSION_TAG );
	if (!versionPtr || !versionPtr->isAttribute())
	{
		// Could not find the version tag,
		// there is an error with this data section
		return false;
	}

	// Parse the id section
	converterId_ = idPtr->asUInt64();
	// Parse the version section
	converterVersion_ = versionPtr->asString();
	return Dependency::serialiseIn( pSection );
}

bool ConverterDependency::serialiseOut( DataSectionPtr pSection ) const
{
	// Create an id sub section in the provided section 
	// and save out our id.
	DataSectionPtr idPtr = pSection->newSection( ConverterDependency_Locals::ID_TAG );
	MF_ASSERT( idPtr.hasObject() );
	MF_VERIFY( idPtr->isAttribute( true ) );
	MF_VERIFY( idPtr->setUInt64( converterId_ ) );

	// Create a version sub section in the provided section 
	// and save out our version.
	DataSectionPtr versionPtr = pSection->newSection( ConverterDependency_Locals::VERSION_TAG );
	MF_ASSERT( versionPtr.hasObject() );
	MF_VERIFY( versionPtr->isAttribute( true ) );
	MF_VERIFY( versionPtr->setString( converterVersion_ ) );
	return Dependency::serialiseOut( pSection );
}

BW_END_NAMESPACE