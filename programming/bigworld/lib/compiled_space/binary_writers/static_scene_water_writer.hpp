#ifndef STATIC_SCENE_WATER_BINARY_WRITER_HPP
#define STATIC_SCENE_WATER_BINARY_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "math/boundbox.hpp"

#include "static_scene_writer.hpp"

#include "../static_scene_types.hpp"
#include "../static_scene_water_types.hpp"

namespace BW {

namespace CompiledSpace {

class StringTableWriter;

class StaticSceneWaterWriter : public IStaticSceneWriterHandler
{
public:
	static BW::string outputName( const BW::string& spaceDir );

public:
	StaticSceneWaterWriter();
	~StaticSceneWaterWriter();
	
	void convertWater( const ConversionContext& ctx, 
		const DataSectionPtr& pObjectDS, const BW::string& uid );

	bool addFromChunkVLO( const DataSectionPtr& pObjectDS,
		const Vector3& chunkCentre,
		const BW::string& oDataFile,
		StringTableWriter& stringTable,
		AssetListWriter& assetList );

private:

	// IStaticSceneWriterHandler interface
	virtual SceneTypeSystem::RuntimeTypeID typeID() const;

	virtual size_t numObjects() const;
	virtual const AABB& worldBounds( size_t idx ) const;
	virtual const Matrix& worldTransform( size_t idx ) const;

	virtual bool writeData( BinaryFormatWriter& writer );


private:
	BW::vector<StaticSceneWaterTypes::Water> water_;
	BW::vector<Matrix> worldTransforms_;
	BW::vector<AABB> worldBounds_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_WATER_BINARY_WRITER_HPP
