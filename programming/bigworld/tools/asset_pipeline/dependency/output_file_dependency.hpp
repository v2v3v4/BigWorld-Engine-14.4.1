#ifndef ASSET_PIPELINE_OUTPUT_FILE_DEPENDENCY
#define ASSET_PIPELINE_OUTPUT_FILE_DEPENDENCY

#include "dependency.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class OutputFileDependency : public Dependency
{
public:
	OutputFileDependency();
	OutputFileDependency( const BW::string & fileName );
	virtual ~OutputFileDependency();

	const BW::string & getFileName() const { return fileName_; }

	virtual DependencyType getType() const { return OutputFileDependencyType; }

	virtual bool serialiseIn( DataSectionPtr pSection );
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

private:
	BW::string fileName_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_OUTPUT_FILE_DEPENDENCY
