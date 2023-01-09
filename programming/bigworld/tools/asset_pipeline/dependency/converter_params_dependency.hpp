#ifndef ASSET_PIPELINE_CONVERTER_PARAMS_DEPENDENCY
#define ASSET_PIPELINE_CONVERTER_PARAMS_DEPENDENCY

#include "dependency.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class ConverterParamsDependency : public Dependency
{
public:
	ConverterParamsDependency();
	ConverterParamsDependency( const BW::string & converterParams );
	virtual ~ConverterParamsDependency();

	const BW::string & getConverterParams() const { return converterParams_; }

	virtual DependencyType getType() const { return ConverterParamsDependencyType; }

	virtual bool serialiseIn( DataSectionPtr pSection );
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

private:
	BW::string converterParams_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERTER_PARAMS_DEPENDENCY
