#ifndef ASSET_PIPELINE_DIRECTORY_DEPENDENCY
#define ASSET_PIPELINE_DIRECTORY_DEPENDENCY

#include "dependency.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class DirectoryDependency : public Dependency
{
public:
	DirectoryDependency();
	DirectoryDependency( const BW::string & directory, 
		const BW::string & pattern, bool regex, bool recursive );
	virtual ~DirectoryDependency();

	const BW::string & getDirectory() const { return directory_; }
	const BW::string & getPattern() const { return pattern_; }
	bool isRegex() const { return regex_; }
	bool isRecursive() const { return recursive_; }

	virtual DependencyType getType() const { return DirectoryDependencyType; }

	virtual bool serialiseIn( DataSectionPtr pSection );
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

private:
	BW::string directory_;
	BW::string pattern_;
	bool regex_;
	bool recursive_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_DIRECTORY_DEPENDENCY
