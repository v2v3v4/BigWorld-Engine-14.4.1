#ifndef TERRAIN2_BINARY_WRITER_HPP
#define TERRAIN2_BINARY_WRITER_HPP

#include "../terrain2_types.hpp"
#include "space_writer.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/lookup_table.hpp"
#include "math/boundbox.hpp"
#include "math/loose_octree.hpp"
#include "resmgr/datasection.hpp"

namespace BW {

namespace CompiledSpace {

class BinaryFormatWriter;
class StringTableWriter;

class Terrain2Writer :
	public ISpaceWriter
{
public:
	Terrain2Writer();
	~Terrain2Writer();
	
	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual void postProcess();
	virtual bool write( BinaryFormatWriter& writer );

	void convertTerrain( const ConversionContext& ctx,
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void addFromChunk( int gridX, int gridZ, const DataSectionPtr& pDS,
		StringTableWriter& stringTable, const DataSectionPtr & pCDataSection,
		const Matrix & chunkTransform );

	size_t numBlocks() const;
	AABB boundBox() const;

private:
	bool supportedVersion_;

	typedef vector<DynamicLooseOctree::NodeDataReference> NodeContents;
	typedef LookUpTable < NodeContents > NodeContentsTable;
	void generateOctree( DynamicLooseOctree & octree,
		NodeContentsTable & sceneOctreeContents );

	Terrain2Types::Header header_;
	BW::vector<Terrain2Types::BlockData> blocks_;
	BW::vector<uint32> blockGridLookup_;

	BW::vector<AABB> blockBounds_;
	AABB terrainBounds_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // SRTING_TABLE_WRITER_HPP
