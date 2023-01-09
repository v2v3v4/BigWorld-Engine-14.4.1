#include "pch.hpp"

#include "resmgr/bwresource.hpp"
#include "romp/water.hpp"

#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "static_scene_water_writer.hpp"
#include "chunk_converter.hpp"

#include "../asset_list_types.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneWaterWriter::StaticSceneWaterWriter()
{

}


// ----------------------------------------------------------------------------
StaticSceneWaterWriter::~StaticSceneWaterWriter()
{

}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneWaterWriter::typeID() const
{
	return StaticSceneTypes::StaticObjectType::WATER;
}


// ----------------------------------------------------------------------------
size_t StaticSceneWaterWriter::numObjects() const
{
	return water_.size();
}


// ----------------------------------------------------------------------------
bool StaticSceneWaterWriter::writeData( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
		StaticSceneWaterTypes::FORMAT_MAGIC, 
		StaticSceneWaterTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	stream->write( water_ );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneWaterWriter::convertWater( const ConversionContext& ctx, 
	const DataSectionPtr& pObjectDS, 
	const BW::string& uid )
{
	Vector3 chunkCentre = Vector3( ctx.gridSize * 0.5f, 0.0f, ctx.gridSize * 0.5f );
	chunkCentre = ctx.chunkTransform.applyPoint( chunkCentre );

	BW::string odata = ctx.spaceDir + "/_" + uid + ".odata";
	BW::string legacyOdata = ctx.spaceDir + "/" + uid + ".odata";
	if (!BWResource::fileExists( odata ) && BWResource::fileExists( legacyOdata ))
	{
		odata = legacyOdata;
	}

	addFromChunkVLO( pObjectDS, chunkCentre, odata, strings(), assets() );
}


// ----------------------------------------------------------------------------
bool StaticSceneWaterWriter::addFromChunkVLO( const DataSectionPtr& pObjectDS,
	const Vector3& chunkCentre,
	const BW::string& oDataFile,
	StringTableWriter& stringTable,
	AssetListWriter& assetList )
{
	MF_ASSERT( pObjectDS != NULL );

	StaticSceneWaterTypes::Water result;
	memset( &result, 0, sizeof(result) );

	if (!result.state_.load( pObjectDS, chunkCentre, Matrix::identity ))
	{
		return false;
	}

	BW::Water::WaterResources resources;
	if (!resources.load( pObjectDS, oDataFile ))
	{
		return false;
	}

	result.foamTexture_ = assetList.addAsset(
		CompiledSpace::AssetListTypes::ASSET_TYPE_TEXTURE,
		resources.foamTexture_, stringTable );

	result.waveTexture_ = stringTable.addString( resources.waveTexture_ );

	result.reflectionTexture_ = assetList.addAsset(
		CompiledSpace::AssetListTypes::ASSET_TYPE_TEXTURE,
		resources.reflectionTexture_, stringTable );

	result.transparencyTable_ = stringTable.addString( resources.transparencyTable_ );

	// calculate world transform
	Matrix transform;
	transform.setRotateY( result.state_.orientation_ );
	transform.postTranslateBy( result.state_.position_ );

	// calculate world bounds
	Vector3 size( result.state_.size_.x * 0.5f, 0, result.state_.size_.y * 0.5f );
	AABB bounds( -size, size );
	bounds.transformBy( transform );

	water_.push_back( result );
	worldTransforms_.push_back( transform );
	worldBounds_.push_back( bounds );

	return true;
}


// ----------------------------------------------------------------------------
const AABB& StaticSceneWaterWriter::worldBounds( size_t idx ) const
{
	MF_ASSERT( idx < worldBounds_.size() );
	return worldBounds_[idx];
}


// ----------------------------------------------------------------------------
const Matrix& StaticSceneWaterWriter::worldTransform( size_t idx ) const
{
	MF_ASSERT( idx < water_.size() );
	return worldTransforms_[idx];
}

} // namespace CompiledSpace
} // namespace BW
