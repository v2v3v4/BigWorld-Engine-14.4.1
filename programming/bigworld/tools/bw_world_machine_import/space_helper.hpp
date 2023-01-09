#ifndef SPACE_HELPER_HPP
#define SPACE_HELPER_HPP

#include "resmgr/datasection.hpp"
#include "resmgr/file_system.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a simple space helper for the World Machine plugin
 *	it is used to retrieve information and files from the space
 */
class SpaceHelper
{
public:
	SpaceHelper();
	bool init( const BW::string& spaceSettingsPath );

	int xMin() { return xMin_; }
	int xMax() { return xMax_; }
	int zMin() { return zMin_; }
	int zMax() { return zMax_; }

	float chunkSize() const { return chunkSize_; }
	bool singleDir() const { return singleDir_; }

	const BW::string& spaceRoot() const { return spaceRoot_; }

	BW::DataSectionPtr getCDataForChunk( int x, int z );

	bool valid() const { return !spaceRoot_.empty(); }
private:
	int xMin_;
	int xMax_;
	int zMin_;
	int zMax_;

	float chunkSize_;
	
	bool singleDir_;

	BW::string spaceRoot_;
	BW::FileSystemPtr pFileSystem_;
};

BW_END_NAMESPACE

#endif // SPACE_HELPER_HPP
