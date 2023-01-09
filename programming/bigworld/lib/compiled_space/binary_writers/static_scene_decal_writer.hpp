#ifndef STATIC_SCENE_DECAL_WRITER_HPP
#define STATIC_SCENE_DECAL_WRITER_HPP


#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "math/boundbox.hpp"

#include "static_scene_writer.hpp"

#include "../static_scene_types.hpp"
#include "../static_scene_decal_types.hpp"

namespace BW {
namespace CompiledSpace {

class StringTableWriter;
class AssetListWriter;

class StaticSceneDecalWriter : public IStaticSceneWriterHandler
{
public:
    StaticSceneDecalWriter();
    ~StaticSceneDecalWriter();
    
    void convertStaticDecal( const ConversionContext& ctx, 
        const DataSectionPtr& pItemDS, const BW::string& uid );

    bool addFromChunkDecal( const DataSectionPtr& pObjectDS,
        const Matrix& chunkTransform,
        StringTableWriter& stringTable,
        AssetListWriter& assetList );

private:

    // IStaticSceneWriterHandler interface
    virtual SceneTypeSystem::RuntimeTypeID typeID() const;

    virtual size_t numObjects() const;
    virtual const BoundingBox& worldBounds( size_t idx ) const;
    virtual const Matrix& worldTransform( size_t idx ) const;

    virtual bool writeData( BinaryFormatWriter& writer );

private:
    BW::vector<StaticSceneDecalTypes::Decal> decals_;
    BW::vector<BoundingBox> worldBounds_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_DECAL_WRITER_HPP