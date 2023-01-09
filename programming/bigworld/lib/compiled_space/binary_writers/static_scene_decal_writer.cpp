#include "pch.hpp"

#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "static_scene_decal_writer.hpp"
#include "chunk_converter.hpp"

#include "moo/deferred_decals_manager.hpp"

namespace BW {
namespace CompiledSpace {

//-----------------------------------------------------------------------------
StaticSceneDecalWriter::StaticSceneDecalWriter()
{

}


//-----------------------------------------------------------------------------
StaticSceneDecalWriter::~StaticSceneDecalWriter()
{

}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneDecalWriter::typeID() const
{
    return StaticSceneTypes::StaticObjectType::DECAL;
}


// ----------------------------------------------------------------------------
size_t StaticSceneDecalWriter::numObjects() const
{
    return decals_.size();
}


// ----------------------------------------------------------------------------
bool StaticSceneDecalWriter::writeData( BinaryFormatWriter& writer )
{
    BinaryFormatWriter::Stream * stream = writer.appendSection(
        CompiledSpace::StaticSceneDecalTypes::FORMAT_MAGIC,
        CompiledSpace::StaticSceneDecalTypes::FORMAT_VERSION );
    MF_ASSERT( stream != NULL );

    stream->write( decals_ );
    return true;
}


// ----------------------------------------------------------------------------
void StaticSceneDecalWriter::convertStaticDecal( const ConversionContext& ctx,
    const DataSectionPtr& pItemDS, const BW::string& uid )
{
    addFromChunkDecal( pItemDS, ctx.chunkTransform,
        strings(), assets() );
}


//-----------------------------------------------------------------------------
bool StaticSceneDecalWriter::addFromChunkDecal(
    const DataSectionPtr& pObjectDS,
    const Matrix& chunkTransform,
    StringTableWriter& stringTable,
    AssetListWriter& assetList )
{
    MF_ASSERT( pObjectDS != NULL );

    CompiledSpace::StaticSceneDecalTypes::Decal result;
    memset( &result, 0, sizeof(result) );

    typedef DecalsManager::Decal Decals;

    result.diffuseTexture_ =
        stringTable.addString( pObjectDS->readString( "diffTex" ) );

    result.bumpTexture_ =
        stringTable.addString( pObjectDS->readString( "bumpTex" ) );

    result.influenceType_ =
        (uint8)pObjectDS->readUInt( "influence", Decals::APPLY_TO_TERRAIN );

    result.materialType_ = (uint8)pObjectDS->readUInt( "type", 1 );
    result.priority_ = (uint8)pObjectDS->readUInt( "priority", 0 );

    result.worldTransform_ =
        pObjectDS->readMatrix34( "transform", Matrix::identity );
    result.worldTransform_.postMultiply( chunkTransform );

    decals_.push_back( result );

    BoundingBox bb( Vector3(-0.5f, -0.5f, -0.5f), Vector3(+0.5f, +0.5f, +0.5f) );
    bb.transformBy( result.worldTransform_ );

    worldBounds_.push_back( bb );

    return true;
}


// ----------------------------------------------------------------------------
const BoundingBox& StaticSceneDecalWriter::worldBounds( size_t idx ) const
{
    MF_ASSERT( idx < worldBounds_.size() );
    return worldBounds_[idx];
}


// ----------------------------------------------------------------------------
const Matrix& StaticSceneDecalWriter::worldTransform( size_t idx ) const
{
    MF_ASSERT( idx < decals_.size() );
    return decals_[idx].worldTransform_;
}


} // namespace CompiledSpace
} // namespace BW