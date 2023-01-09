#ifndef ASSET_PIPELINE_GENERIC_CONVERSION_RULE
#define ASSET_PIPELINE_GENERIC_CONVERSION_RULE

#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/conversion/converter_map.hpp"
#include "resmgr/hierarchical_config.hpp"

BW_BEGIN_NAMESPACE 

class GenericConversionRule : public ConversionRule
{
public:
	GenericConversionRule( ConverterMap & converters );

	void load( const StringRef & rules );

	virtual bool createRootTask( const BW::StringRef & sourceFile,
							     ConversionTask & task );

	virtual bool createTask( const BW::StringRef & sourceFile,
							 ConversionTask & task );
	
	virtual bool getSourceFile( const BW::StringRef & file,
								BW::string & sourcefile ) const;

private:
	bool createTask( const BW::StringRef & sourceFile,
					 ConversionTask & task,
					 bool root );

private:
	ConverterMap & converters_;
	HierarchicalConfig rules_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_GENERIC_CONVERSION_RULE
