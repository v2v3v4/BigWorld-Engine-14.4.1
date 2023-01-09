#ifndef ASSET_PIPELINE_INTERMEDIATE_FILE_DEPENDENCY
#define ASSET_PIPELINE_INTERMEDIATE_FILE_DEPENDENCY

#include "dependency.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class IntermediateFileDependency : public Dependency
{
public:
	IntermediateFileDependency();
	IntermediateFileDependency( const BW::string & fileName );
	virtual ~IntermediateFileDependency();

	const BW::string & getFileName() const { return fileName_; }

	virtual DependencyType getType() const { return IntermediateFileDependencyType; }

	virtual bool serialiseIn( DataSectionPtr pSection );
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

private:
	BW::string fileName_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_INTERMEDIATE_FILE_DEPENDENCY
