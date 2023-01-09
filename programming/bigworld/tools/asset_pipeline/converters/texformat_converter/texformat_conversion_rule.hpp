#ifndef ASSET_PIPELINE_TEX_FORMAT_CONVERSION_RULE
#define ASSET_PIPELINE_TEX_FORMAT_CONVERSION_RULE

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"

BW_BEGIN_NAMESPACE


class TexFormatConversionRule : public ConversionRule
{
public:	
	/* returns true and populates a conversion task if the rule can match the input filename. */
	virtual bool createRootTask( const BW::StringRef& sourceFile,
							     ConversionTask& task );

	/* returns true if the rule can match the output filename. */
	virtual bool getSourceFile( const BW::StringRef& file,
								BW::string& sourcefile ) const;

private:
	bool isOutOfDateTexFormat( const BW::StringRef & resourceID ) const;

};


BW_END_NAMESPACE

#endif //ASSET_PIPELINE_TEX_FORMAT_CONVERSION_RULE
