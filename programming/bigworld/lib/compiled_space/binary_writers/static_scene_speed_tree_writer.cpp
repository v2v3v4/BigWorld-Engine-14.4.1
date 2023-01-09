#include "pch.hpp"

#include "resmgr/bwresource.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "static_scene_speed_tree_writer.hpp"
#include "chunk_converter.hpp"

#include "../asset_list_types.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneSpeedTreeWriter::StaticSceneSpeedTreeWriter()
{

}


// ----------------------------------------------------------------------------
StaticSceneSpeedTreeWriter::~StaticSceneSpeedTreeWriter()
{

}


// ----------------------------------------------------------------------------
 SceneTypeSystem::RuntimeTypeID StaticSceneSpeedTreeWriter::typeID() const
 {
 	return StaticSceneTypes::StaticObjectType::SPEED_TREE;
 }
	

// ----------------------------------------------------------------------------
size_t StaticSceneSpeedTreeWriter::numObjects() const
{
	return trees_.size();
}


// ----------------------------------------------------------------------------
bool StaticSceneSpeedTreeWriter::writeData( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
		StaticSceneSpeedTreeTypes::FORMAT_MAGIC, 
		StaticSceneSpeedTreeTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );
	
	stream->write( trees_ );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneSpeedTreeWriter::convertSpeedTree( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addFromChunkTree( pItemDS, ctx.chunkTransform, 
		strings(), assets() );
}


// ----------------------------------------------------------------------------
bool StaticSceneSpeedTreeWriter::addFromChunkTree( const DataSectionPtr& pItemDS,
	const Matrix& chunkTransform,
	StringTableWriter& stringTable,
	AssetListWriter& assetList )
{
	MF_ASSERT( pItemDS != NULL );

	StaticSceneSpeedTreeTypes::SpeedTree result;
	memset( &result, 0, sizeof(result) );

	// SpeedTree
	BW::string resource = pItemDS->readString( "spt" );
	if (resource.empty())
	{
		ERROR_MSG( "SpeedTreeWriter: ChunkTree has no speedtree specified.\n" );
		return false;
	}

	result.resourceID_ = stringTable.addString( resource );

	// Seed
	result.seed_ = pItemDS->readInt( "seed", 1 );

	// Transform
	result.worldTransform_ = chunkTransform;
	result.worldTransform_.preMultiply(
		pItemDS->readMatrix34( "transform", Matrix::identity ) );

	// Bounding box
	BW::speedtree::SpeedTreeRenderer speedtree;
	speedtree.load( resource.c_str(), result.seed_, result.worldTransform_ );
	AABB bounds = speedtree.boundingBox();
	if (bounds.insideOut())
	{
		ERROR_MSG( "SpeedTreeWriter: Invalid bounding box.\n" );
		return false;
	}

	bounds.transformBy( result.worldTransform_ );

	// Various flags
	bool castsShadow = true;
	castsShadow = pItemDS->readBool( "castsShadow", castsShadow );
	castsShadow = pItemDS->readBool( "editorOnly/castsShadow", castsShadow ); // check legacy
	if (castsShadow)
	{
		result.flags_ |= StaticSceneSpeedTreeTypes::FLAG_CASTS_SHADOW;
	}

	if (pItemDS->readBool( "reflectionVisible" ))
	{
		result.flags_ |= StaticSceneSpeedTreeTypes::FLAG_REFLECTION_VISIBLE;
	}

	trees_.push_back( result );
	worldBounds_.push_back( bounds );

	return true;
}


// ----------------------------------------------------------------------------
const AABB& StaticSceneSpeedTreeWriter::worldBounds( size_t idx ) const
{
	MF_ASSERT( idx < worldBounds_.size() );
	return worldBounds_[idx];
}


// ----------------------------------------------------------------------------
const Matrix& StaticSceneSpeedTreeWriter::worldTransform( size_t idx ) const
{
	MF_ASSERT( idx < trees_.size() );
	return trees_[idx].worldTransform_;
}

} // namespace CompiledSpace
} // namespace BW
