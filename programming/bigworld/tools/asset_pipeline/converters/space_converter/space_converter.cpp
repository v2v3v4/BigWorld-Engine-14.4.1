// VS2019: warning C4996: 'std::tr1': warning STL4002: The non-Standard std::tr1 namespace and
// TR1-only machinery are deprecated and will be REMOVED.
// You can define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING to acknowledge that you have
// received this warning.
#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING 1

#include "space_converter.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/command_line.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "moo/texture_detail_level_manager.hpp"

#include "compiled_space/binary_writers/binary_format_writer.hpp"

BW_BEGIN_NAMESPACE

const uint64 DefaultSpaceConverter::s_TypeId = Hash64::compute( DefaultSpaceConverter::getTypeName() );
const char * DefaultSpaceConverter::s_Version = "2.9.0";

namespace {

	const StringRef SPACE_SETTINGS_IDENTIFIER = "/space.settings";

	BW::string spaceDirFromSettings( const BW::string& settingsFile )
	{
		return settingsFile.substr( 0, 
			settingsFile.length() - SPACE_SETTINGS_IDENTIFIER.length() );
	}

}

// ----------------------------------------------------------------------------
/* construct a converter with parameters. */
SpaceConverter::SpaceConverter( const BW::string& params )
	:	Converter( params )
{
	BW_GUARD;

	addWriter( &settings_ );
	addWriter( &staticGeometry_ );
}


// ----------------------------------------------------------------------------
SpaceConverter::~SpaceConverter()
{

}


// ----------------------------------------------------------------------------
void SpaceConverter::addWriter( CompiledSpace::ISpaceWriter* pWriter )
{
	writers_.push_back( pWriter );
}


// ----------------------------------------------------------------------------
CompiledSpace::SettingsWriter& SpaceConverter::settings()
{
	return settings_;
}


// ----------------------------------------------------------------------------
CompiledSpace::StringTableWriter& SpaceConverter::stringTable()
{
	return stringTable_;
}


// ----------------------------------------------------------------------------
CompiledSpace::AssetListWriter& SpaceConverter::assetList()
{
	return assetList_;
}


// ----------------------------------------------------------------------------
CompiledSpace::StaticGeometryWriter& SpaceConverter::staticGeometry()
{
	return staticGeometry_;
}


// ----------------------------------------------------------------------------
CompiledSpace::ChunkConverter& SpaceConverter::converter()
{
	return processor_;
}


// ----------------------------------------------------------------------------
/* builds the dependency list for a source file. */
bool SpaceConverter::createDependencies( const BW::string & sourcefile, 
										  const Compiler & compiler,
										  DependencyList & dependencies )
{
	BW_GUARD;

	BW::string spaceDir = spaceDirFromSettings( sourcefile );
	MF_VERIFY( compiler.resolveRelativePath( spaceDir ) );
	dependencies.addSecondaryDirectoryDependency( spaceDir, ".*\\.(chunk|cdata)$", true, true, true );

	return true;
}


// ----------------------------------------------------------------------------
/* convert a source file. */
/* work out the output filename & insert it into the converted files vector */
bool SpaceConverter::convert( const BW::string & sourcefile,
							   const Compiler & compiler,
							   BW::vector< BW::string > & intermediateFiles,
							   BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;
	BW::string spaceDir = spaceDirFromSettings( sourcefile );
	compiler.resolveRelativePath( spaceDir );

	DataSectionPtr pSpaceSettings =
		BWResource::instance().openSection( sourcefile );
	if (!pSpaceSettings)
	{
		ERROR_MSG( "%s is not valid.\n", sourcefile.c_str() );
		return false;
	}
	CommandLine commandLine( params_.c_str() );

	if (!initializeWriters( spaceDir, pSpaceSettings, commandLine ))
	{
		ERROR_MSG( "Failed to initialize space writers. \n" );
		return false;
	}

	processChunks();
	postProcessWriters();

	CompiledSpace::BinaryFormatWriter writer;
	outputWriterData( writer );

	BW::string binaryFilename = spaceDir + "/space.bin";
	if (writer.write( binaryFilename.c_str() ))
	{
		compiler.resolveOutputPath( binaryFilename );
		outputFiles.push_back( binaryFilename );
	}
	else
	{
		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
bool SpaceConverter::initializeWriters( const BW::string& spaceDir,
	const DataSectionPtr& pSpaceSettings,
	CommandLine& commandLine )
{
	// Iterate over all the writers and configure them
	for (WriterList::iterator iter = writers_.begin(); 
		iter != writers_.end(); ++iter)
	{
		CompiledSpace::ISpaceWriter* pWriter = *iter;

		pWriter->configure( &stringTable_, &assetList_ );
	}

	// Iterate over all writers, and execute initialize
	for (WriterList::iterator iter = writers_.begin(); 
		iter != writers_.end(); ++iter)
	{
		CompiledSpace::ISpaceWriter* pWriter = *iter;

		pWriter->initialize( pSpaceSettings, commandLine );
	}

	processor_.initialize( spaceDir, pSpaceSettings, commandLine );

	return true;
}


// ----------------------------------------------------------------------------
void SpaceConverter::processChunks()
{
	processor_.process();
}


// ----------------------------------------------------------------------------
void SpaceConverter::postProcessWriters()
{
	TRACE_MSG( "%d assets queued for preload.\n", assetList_.size() );

	// Iterate over all writers, and execute initialize
	for (WriterList::iterator iter = writers_.begin(); 
		iter != writers_.end(); ++iter)
	{
		CompiledSpace::ISpaceWriter* pWriter = *iter;

		pWriter->postProcess();
	}
}


// ----------------------------------------------------------------------------
bool SpaceConverter::outputWriterData( CompiledSpace::BinaryFormatWriter& writer )
{
	if (!stringTable_.write( writer ))	
	{
		ERROR_MSG( "Failed to write string table.\n" );
		return false;		
	}

	if (!assetList_.write( writer ))
	{
		ERROR_MSG( "Failed to write asset list.\n" );
		return false;		
	}

	// Iterate over all writers, and execute initialize
	for (WriterList::iterator iter = writers_.begin(); 
		iter != writers_.end(); ++iter)
	{
		CompiledSpace::ISpaceWriter* pWriter = *iter;

		if (!pWriter->write( writer ))
		{
			ERROR_MSG( "Failed to write compiled space data.\n" );
		}
	}

	return true;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
DefaultSpaceConverter::DefaultSpaceConverter( const BW::string& params ) :
	SpaceConverter( params ),
	staticScene_( CompiledSpace::StaticSceneTypes::FORMAT_MAGIC[
		CompiledSpace::StaticSceneTypes::eDEFAULT] ),
	vloScene_( CompiledSpace::StaticSceneTypes::FORMAT_MAGIC[
		CompiledSpace::StaticSceneTypes::eVLO] )
{
	staticScene_.addType( &staticModels_ );
#if SPEEDTREE_SUPPORT
	staticScene_.addType( &speedTrees_ );
#endif
	staticScene_.addType( &staticFlares_ );
	staticScene_.addType( &decals_ );
	vloScene_.addType( &water_ );

	settings().addBoundGenerator( 
		std::tr1::bind( &CompiledSpace::Terrain2Writer::boundBox, &terrain2_ ) );
	settings().addBoundGenerator( 
		std::tr1::bind( &CompiledSpace::ParticleSystemWriter::boundBox, &particles_ ) );
	settings().addBoundGenerator( 
		std::tr1::bind( &CompiledSpace::StaticSceneWriter::boundBox, &staticScene_ ) );
	settings().addBoundGenerator( 
		std::tr1::bind( &CompiledSpace::StaticSceneWriter::boundBox, &vloScene_ ) );
	settings().addBoundGenerator( 
		std::tr1::bind( &CompiledSpace::LightSceneWriter::boundBox, &lightWriter_ ) );

	// Configure handlers
	converter().addIgnoreHandler( "transform" );
	converter().addIgnoreHandler( "boundingBox" );
	converter().addIgnoreHandler( "worldNavmesh" );
	converter().addIgnoreHandler( "waypointGenerationTime" );
	converter().addItemHandler( "terrain", &terrain2_, &CompiledSpace::Terrain2Writer::convertTerrain );
	converter().addItemHandler( "model", &staticModels_, &CompiledSpace::StaticSceneModelWriter::convertModel );
	converter().addItemHandler( "entity", &entities_, &CompiledSpace::EntityWriter::convertEntity );
	converter().addItemHandler( "UserDataObject", &entities_, &CompiledSpace::EntityWriter::convertUDO );
#if SPEEDTREE_SUPPORT
	converter().addItemHandler( "speedtree", &speedTrees_, &CompiledSpace::StaticSceneSpeedTreeWriter::convertSpeedTree );
#endif
	converter().addItemHandler( "flare", &staticFlares_, &CompiledSpace::StaticSceneFlareWriter::convertFlare );
	converter().addItemHandler( "shell", &staticModels_, &CompiledSpace::StaticSceneModelWriter::convertShell );
	converter().addItemHandler( "particles", &particles_, &CompiledSpace::ParticleSystemWriter::convertParticles );
	converter().addItemHandler( "staticDecal", &decals_, &CompiledSpace::StaticSceneDecalWriter::convertStaticDecal );
	converter().addItemHandler( "water", &water_, &CompiledSpace::StaticSceneWaterWriter::convertWater );

	// NOTE: We're ignoring directionalLight and ambientLight definitions
	converter().addIgnoreHandler( "ambientLight" );
	converter().addIgnoreHandler( "directionLight" );
	converter().addItemHandler( "omniLight", &lightWriter_, &CompiledSpace::LightSceneWriter::convertOmniLight );
	converter().addItemHandler( "spotLight", &lightWriter_, &CompiledSpace::LightSceneWriter::convertSpotLight );
	converter().addItemHandler( "pulseLight", &lightWriter_, &CompiledSpace::LightSceneWriter::convertPulseLight );

	// Texture streaming handlers
	converter().addItemHandler( "speedtree", &textureStreamingWriter_, 
		&CompiledSpace::StaticTextureStreamingWriter::convertSpeedTree );
	converter().addItemHandler( "model", &textureStreamingWriter_, 
		&CompiledSpace::StaticTextureStreamingWriter::convertModel );
	converter().addItemHandler( "shell", &textureStreamingWriter_, 
		&CompiledSpace::StaticTextureStreamingWriter::convertShell );
		
	addWriter( &terrain2_ );
	addWriter( &staticScene_ );
	addWriter( &vloScene_ );
	addWriter( &particles_ );
	addWriter( &entities_ );
	addWriter( &lightWriter_ );
	addWriter( &textureStreamingWriter_ );
}


// ----------------------------------------------------------------------------
DefaultSpaceConverter::~DefaultSpaceConverter()
{

}


// ----------------------------------------------------------------------------


BW_END_NAMESPACE
