#ifndef ASSET_PIPELINE_SPACE_CONVERTER
#define ASSET_PIPELINE_SPACE_CONVERTER

#include "space_converter_dll.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/stringmap.hpp"
#include "resmgr/datasection.hpp"

#include "compiled_space/binary_writers/chunk_converter.hpp"
#include "compiled_space/binary_writers/settings_writer.hpp"
#include "compiled_space/binary_writers/string_table_writer.hpp"
#include "compiled_space/binary_writers/asset_list_writer.hpp"
#include "compiled_space/binary_writers/terrain2_writer.hpp"
#include "compiled_space/binary_writers/static_geometry_writer.hpp"
#include "compiled_space/binary_writers/static_scene_decal_writer.hpp"

#include "compiled_space/binary_writers/static_scene_writer.hpp"
#include "compiled_space/binary_writers/static_scene_model_writer.hpp"
#include "compiled_space/binary_writers/static_scene_speed_tree_writer.hpp"
#include "compiled_space/binary_writers/static_scene_water_writer.hpp"
#include "compiled_space/binary_writers/static_scene_flare_writer.hpp"
#include "compiled_space/binary_writers/particle_system_writer.hpp"
#include "compiled_space/binary_writers/entity_writer.hpp"

#include "compiled_space/binary_writers/light_scene_writer.hpp"
#include "compiled_space/binary_writers/static_texture_streaming_writer.hpp"

#include <asset_pipeline/conversion/converter.hpp>


BW_BEGIN_NAMESPACE

class Chunk;

namespace CompiledSpace
{
	class BinaryFormatWriter;
}

class SpaceConverter : 
	public Converter
{
protected:
	SPACECONVERTER_DLL SpaceConverter( const BW::string& params );
	SPACECONVERTER_DLL virtual ~SpaceConverter();

public:
	SPACECONVERTER_DLL virtual bool createDependencies( const BW::string & sourcefile,
									 const Compiler & compiler,
									 DependencyList & dependencies );

	SPACECONVERTER_DLL virtual bool convert( const BW::string & sourcefile,
						  const Compiler & compiler,
						  BW::vector< BW::string > & intermediateFiles,
						  BW::vector< BW::string > & outputFiles );

protected:

	typedef CompiledSpace::ChunkConverter::ConversionContext ConversionContext;

	SPACECONVERTER_DLL void addWriter( CompiledSpace::ISpaceWriter* pWriter );

	SPACECONVERTER_DLL CompiledSpace::SettingsWriter& settings();
	SPACECONVERTER_DLL CompiledSpace::StringTableWriter& stringTable();
	SPACECONVERTER_DLL CompiledSpace::AssetListWriter& assetList();
	SPACECONVERTER_DLL CompiledSpace::StaticGeometryWriter& staticGeometry();
	SPACECONVERTER_DLL CompiledSpace::ChunkConverter& converter();

private:

	bool initializeWriters( 
		const BW::string& spaceDir,
		const DataSectionPtr& pSpaceSettings,
		CommandLine& cmdLine );
	void processChunks();
	void postProcessWriters();
	bool outputWriterData( CompiledSpace::BinaryFormatWriter& writer );

	CompiledSpace::SettingsWriter settings_;
	CompiledSpace::StringTableWriter stringTable_;
	CompiledSpace::AssetListWriter assetList_;
	CompiledSpace::StaticGeometryWriter staticGeometry_;

	typedef BW::vector< CompiledSpace::ISpaceWriter* > WriterList;
	WriterList writers_;
	CompiledSpace::ChunkConverter processor_;
};


class DefaultSpaceConverter :
	public SpaceConverter
{
public:
	SPACECONVERTER_DLL DefaultSpaceConverter( const BW::string& params );
	SPACECONVERTER_DLL virtual ~DefaultSpaceConverter();

public:
	static uint64 getTypeId() { return s_TypeId; }
	static const char * getVersion() { return s_Version; }
	static const char * getTypeName() { return "SpaceConverter"; }
	static Converter * createConverter( const BW::string& params ) { return new DefaultSpaceConverter( params ); }

	static const uint64 s_TypeId;
	static const char * s_Version;

private:

	CompiledSpace::Terrain2Writer terrain2_;
	CompiledSpace::StaticSceneWriter staticScene_;
	CompiledSpace::StaticSceneWriter vloScene_;
	CompiledSpace::StaticSceneModelWriter staticModels_;
#if SPEEDTREE_SUPPORT
	CompiledSpace::StaticSceneSpeedTreeWriter speedTrees_;
#endif
	CompiledSpace::StaticSceneFlareWriter staticFlares_;
	CompiledSpace::StaticSceneWaterWriter water_;
	CompiledSpace::StaticSceneDecalWriter decals_;
	CompiledSpace::ParticleSystemWriter particles_;
	CompiledSpace::EntityWriter entities_;
	CompiledSpace::LightSceneWriter lightWriter_;
	CompiledSpace::StaticTextureStreamingWriter textureStreamingWriter_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_SPACE_CONVERTER
