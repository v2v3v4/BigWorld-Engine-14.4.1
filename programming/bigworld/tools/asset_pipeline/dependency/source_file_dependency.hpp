#ifndef ASSET_PIPELINE_SOURCE_FILE_DEPENDENCY
#define ASSET_PIPELINE_SOURCE_FILE_DEPENDENCY

#include "dependency.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class SourceFileDependency : public Dependency
{
public:
	SourceFileDependency();
	SourceFileDependency( const BW::string & fileName );
	virtual ~SourceFileDependency();

	const BW::string & getFileName() const { return fileName_; }

	virtual DependencyType getType() const { return SourceFileDependencyType; }

	virtual bool serialiseIn( DataSectionPtr pSection );
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

private:
	BW::string fileName_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_SOURCE_FILE_DEPENDENCY
