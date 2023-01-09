#ifndef ASSET_PIPELINE_CONVERSION_RULE
#define ASSET_PIPELINE_CONVERSION_RULE

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"

BW_BEGIN_NAMESPACE

class Compiler;
struct ConversionTask;

class ConversionRule
{
public:
	ConversionRule() {}
	virtual ~ConversionRule() {}

	/* returns true and populates a root conversion task if the rule can match the input filename. */
	virtual bool createRootTask( const BW::StringRef& sourceFile,
							     ConversionTask& task ) { 
		return false; 
	}

	/* returns true and populates a conversion task if the rule can match the input filename. */
	virtual bool createTask( const BW::StringRef& sourceFile,
							 ConversionTask& task ) {
		return createRootTask( sourceFile, task );
	}
	
	/* returns true if the rule can match the output filename. */
	virtual bool getSourceFile( const BW::StringRef& file,
								BW::string& sourcefile ) const {
		return false;
	}

};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERSION_RULE
