#include "pch.hpp"

#include "particle_system_writer.hpp"
#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "chunk_converter.hpp"

#include "resmgr/quick_file_writer.hpp"

#include "cstdmf/lookup_table.hpp"
#include "../asset_list_types.hpp"

namespace BW {
namespace CompiledSpace {

class StringTableWriter;

// ----------------------------------------------------------------------------
ParticleSystemWriter::ParticleSystemWriter()
{

}


// ----------------------------------------------------------------------------
ParticleSystemWriter::~ParticleSystemWriter()
{
	
}


// ----------------------------------------------------------------------------
bool ParticleSystemWriter::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	return true;
}


// ----------------------------------------------------------------------------
void ParticleSystemWriter::postProcess()
{
	TRACE_MSG( "%d particle systems.\n", systemData_.size() );
}


// ----------------------------------------------------------------------------
bool ParticleSystemWriter::write( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream * stream = writer.appendSection(
		ParticleSystemTypes::FORMAT_MAGIC,
		ParticleSystemTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	stream->write( systemData_ );
	return true;
}


// ----------------------------------------------------------------------------
void ParticleSystemWriter::convertParticles( const ConversionContext& ctx, 
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addFromChunkParticles( pItemDS, ctx.chunkTransform,
		strings(), assets() );
}


// ----------------------------------------------------------------------------
void ParticleSystemWriter::addFromChunkParticles(
	const DataSectionPtr& pDS,
	const Matrix& chunkTransform,
	StringTableWriter& stringTable,
	AssetListWriter& assetList )
{
	BW::string resourceID = pDS->readString( "resource" );
	if (resourceID.empty())
	{
		return;
	}

	if (resourceID.find( '/' ) == BW::string::npos)
	{
		resourceID = "particles/" + resourceID;
	}

	DataSectionPtr resourceDS = BWResource::openSection( resourceID );
	if (!resourceDS)
	{
		return;
	}

	// ok, we're committed to loading now.
	ParticleSystemTypes::ParticleSystem data;
	memset( &data, 0, sizeof(data) );

	data.resourceID_ = assetList.addAsset(
		CompiledSpace::AssetListTypes::ASSET_TYPE_DATASECTION,
		resourceID, stringTable );
	data.seedTime_ = resourceDS->readFloat( "seedTime", 0.1f );
	bool isReflectionVisible = pDS->readBool( "reflectionVisible",
		false );

	if (isReflectionVisible)
	{
		data.flags_ |= ParticleSystemTypes::FLAG_REFLECTION_VISIBLE;
	}

	// Transform
	data.worldTransform_ = chunkTransform;
	data.worldTransform_.preMultiply(
		pDS->readMatrix34( "transform", Matrix::identity ) );

	systemData_.push_back( data );
}


// ----------------------------------------------------------------------------
AABB ParticleSystemWriter::boundBox() const
{
	// Generate bounds
	AABB bounds;
	typedef BW::vector<ParticleSystemTypes::ParticleSystem> SystemVector;
	for (SystemVector::const_iterator iter = systemData_.begin();
		iter != systemData_.end(); ++iter)
	{
		const ParticleSystemTypes::ParticleSystem& system = *iter;
		bounds.addBounds( system.worldTransform_.applyToOrigin() );
	}
	return bounds;
}


// ----------------------------------------------------------------------------

} // namespace CompiledSpace
} // namespace BW
