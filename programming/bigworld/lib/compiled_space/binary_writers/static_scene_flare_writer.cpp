#include "pch.hpp"

#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "static_scene_flare_writer.hpp"
#include "chunk_converter.hpp"

#include "resmgr/bwresource.hpp"
#include "romp/lens_effect.hpp"

#include "../asset_list_types.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneFlareWriter::StaticSceneFlareWriter()
{

}


// ----------------------------------------------------------------------------
StaticSceneFlareWriter::~StaticSceneFlareWriter()
{

}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneFlareWriter::typeID() const
{
	return StaticSceneTypes::StaticObjectType::FLARE;
}


// ----------------------------------------------------------------------------
size_t StaticSceneFlareWriter::numObjects() const
{
	return flares_.size();
}


// ----------------------------------------------------------------------------
bool StaticSceneFlareWriter::writeData( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
		StaticSceneFlareTypes::FORMAT_MAGIC, 
		StaticSceneFlareTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	stream->write( flares_ );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneFlareWriter::convertFlare( const ConversionContext& ctx, 
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addFromChunkFlare( pItemDS, ctx.chunkTransform,
		strings(), assets() );
}


// ----------------------------------------------------------------------------
bool StaticSceneFlareWriter::addFromChunkFlare( const DataSectionPtr& pObjectDS,
	const Matrix& chunkTransform,
	StringTableWriter& stringTable,
	AssetListWriter& assetList )
{
	MF_ASSERT( pObjectDS != NULL );

	StaticSceneFlareTypes::Flare result;
	memset( &result, 0, sizeof(result) );
	
	BW::string resourceID = pObjectDS->readString( "resource" );
	if (resourceID.empty()) 
	{
		return false;
	}
	result.resourceID_ = assetList.addAsset(
		CompiledSpace::AssetListTypes::ASSET_TYPE_DATASECTION,
		resourceID, stringTable );

	DataSectionPtr pFlareRoot = BWResource::openSection( resourceID );
	if (!pFlareRoot)
	{
		return false;
	}

	{
		// Use lens effect data
		result.maxDistance_ = pFlareRoot->readFloat( "maxDistance", 
			LensEffect::DEFAULT_MAXDISTANCE );
		result.area_ = pFlareRoot->readFloat( "area", 
			LensEffect::DEFAULT_AREA );
		result.fadeSpeed_ = pFlareRoot->readFloat( "fadeSpeed", 
			LensEffect::DEFAULT_FADE_SPEED );

		// Then use local data to override it
		result.maxDistance_ = pObjectDS->readFloat( "maxDistance", result.maxDistance_ );
		result.area_ = pObjectDS->readFloat( "area", result.area_ );
		result.fadeSpeed_ = pObjectDS->readFloat( "fadeSpeed", result.fadeSpeed_ );
	}

	result.position_ = pObjectDS->readVector3( "position" );
	DataSectionPtr pColourSect = pObjectDS->openSection( "colour" );
	if (pColourSect)
	{
		result.colour_ = pColourSect->asVector3();
		result.flags_ |= StaticSceneFlareTypes::FLAG_COLOUR_APPLIED;
	}
	else
	{
		result.colour_.setZero();
	}

	// calculate world transform
	Matrix transform = chunkTransform;
	transform.preTranslateBy( result.position_ );
	result.position_ = transform.applyToOrigin();

	// calculate world bounds
	Vector3 size( 0.5f, 0.5f, 0.5f );
	AABB bounds( -size, size );
	bounds.transformBy( transform );

	flares_.push_back( result );
	worldTransforms_.push_back( transform );
	worldBounds_.push_back( bounds );

	return true;
}


// ----------------------------------------------------------------------------
const AABB& StaticSceneFlareWriter::worldBounds( size_t idx ) const
{
	MF_ASSERT( idx < worldBounds_.size() );
	return worldBounds_[idx];
}


// ----------------------------------------------------------------------------
const Matrix& StaticSceneFlareWriter::worldTransform( size_t idx ) const
{
	MF_ASSERT( idx < worldTransforms_.size() );
	return worldTransforms_[idx];
}

} // namespace CompiledSpace
} // namespace BW