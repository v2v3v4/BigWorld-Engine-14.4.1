#ifndef ASSET_PIPELINE_CONVERTER_DEPENDENCY
#define ASSET_PIPELINE_CONVERTER_DEPENDENCY

#include "dependency.hpp"

BW_BEGIN_NAMESPACE

class ConverterDependency : public Dependency
{
public:
	ConverterDependency();
	ConverterDependency( uint64 converterId, const BW::string & converterVersion );
	virtual ~ConverterDependency();

	uint64 getConverterId() const { return converterId_; }
	const BW::string & getConverterVersion() const { return converterVersion_; }

	virtual DependencyType getType() const { return ConverterDependencyType; }

	virtual bool serialiseIn( DataSectionPtr pSection );
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

private:
	uint64 converterId_;
	BW::string converterVersion_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERTER_DEPENDENCY