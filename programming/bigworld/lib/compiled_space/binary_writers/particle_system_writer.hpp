#ifndef PARTICLE_SYSTEM_BINARY_WRITER_HPP
#define PARTICLE_SYSTEM_BINARY_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "resmgr/datasection.hpp"

#include "../particle_system_types.hpp"
#include "space_writer.hpp"

#include "cstdmf/lookup_table.hpp"

#include "math/boundbox.hpp"
#include "math/loose_octree.hpp"

namespace BW {

namespace CompiledSpace {

class BinaryFormatWriter;
class StringTableWriter;
class AssetListWriter;

class ParticleSystemWriter :
	public ISpaceWriter
{
public:
	ParticleSystemWriter();
	~ParticleSystemWriter();
	
	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual void postProcess();
	virtual bool write( BinaryFormatWriter& writer );

	void convertParticles( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );

	void addFromChunkParticles( const DataSectionPtr& pObjectDS,
		const Matrix& chunkTransform,
		StringTableWriter& stringTable,
		AssetListWriter& assetList );

	AABB boundBox() const;

private:

	BW::vector<ParticleSystemTypes::ParticleSystem> systemData_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // PARTICLE_SYSTEM_BINARY_WRITER_HPP
