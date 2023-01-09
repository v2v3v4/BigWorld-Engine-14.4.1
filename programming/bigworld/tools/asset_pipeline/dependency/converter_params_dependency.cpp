#include "converter_params_dependency.hpp"

BW_BEGIN_NAMESPACE

namespace ConverterParamsDependency_Locals
{
	// XML tag names
	const char * PARAMS_TAG = "Params";
}

ConverterParamsDependency::ConverterParamsDependency()
{
}

ConverterParamsDependency::ConverterParamsDependency( const BW::string & converterParams )
	: converterParams_( converterParams )
{
}

ConverterParamsDependency::~ConverterParamsDependency()
{
}

bool ConverterParamsDependency::serialiseIn( DataSectionPtr pSection )
{
	// Search for the params tag
	DataSectionPtr paramsPtr = pSection->findChild( ConverterParamsDependency_Locals::PARAMS_TAG );
	if (!paramsPtr || !paramsPtr->isAttribute())
	{
		// Could not find the params tag,
		// there is an error with this data section
		return false;
	}

	// Parse the params section
	converterParams_ = paramsPtr->asString();
	return Dependency::serialiseIn( pSection );
}

bool ConverterParamsDependency::serialiseOut( DataSectionPtr pSection ) const
{
	// Create a params sub section in the provided section 
	// and save out our file name.
	DataSectionPtr paramsPtr = pSection->newSection( ConverterParamsDependency_Locals::PARAMS_TAG );
	MF_ASSERT( paramsPtr.hasObject() );
	MF_VERIFY( paramsPtr->isAttribute( true ) );
	MF_VERIFY( paramsPtr->setString( converterParams_ ) );
	return Dependency::serialiseOut( pSection );
}

BW_END_NAMESPACE