#include "texture_converter.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "moo/base_texture.hpp"
#include "moo/texture_helpers.hpp"
#include "moo/convert_texture_tool.hpp"
#include "resmgr/bwresource.hpp"


BW_BEGIN_NAMESPACE

const uint64 TextureConverter::s_TypeId = Hash64::compute( TextureConverter::getTypeName() );
const char * TextureConverter::s_Version = "2.8.2";
Moo::ConvertTextureTool * TextureConverter::s_pTool = NULL;

/* construct a converter with parameters. */
TextureConverter::TextureConverter( const BW::string& params )
	: Converter( params )
{
	BW_GUARD;
}

TextureConverter::~TextureConverter()
{
}

/* builds the dependency list for a source file. */
bool TextureConverter::createDependencies( const BW::string & sourcefile, 
										   const Compiler & compiler,
										   DependencyList & dependencies )
{
	BW_GUARD;

	// add secondary dependencies to dependency list
	//
	//  primary are:
	//		the source texture file itself,
	//		the converter hash & version number
	//	secondary:
	//		the .texformat file
	//		the texture_detail_levels file.

	BW::string texformatFile = BWResource::changeExtension( sourcefile, ".texformat" );
	compiler.resolveRelativePath( texformatFile );
	dependencies.addSecondarySourceFileDependency( texformatFile, false );
	dependencies.addSecondaryOutputFileDependency( 
		textureDetailLevelManager_.getTextureDetailLevelsName(), true );

	return true;
}

/* convert a source file. */
/* work out the output filename & insert it into the converted files vector */
bool TextureConverter::convert( const BW::string & sourcefile,
								const Compiler & compiler,
								BW::vector< BW::string > & intermediateFiles,
								BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	BW::StringRef ext = BWResource::getExtension( sourcefile );
	MF_ASSERT( !ext.equals_lower( "dds" ) );

	BW::string relativeSourcefile = sourcefile;
	MF_VERIFY( compiler.resolveRelativePath( relativeSourcefile ) );
	BW::string baseName = Moo::removeTextureExtension( relativeSourcefile ).to_string();
	for (uint32 i = 1; TextureHelpers::textureExts[i] != NULL; i++)
	{
		if (ext.equals_lower( TextureHelpers::textureExts[i] ))
		{
			break;
		}

		BW::string filename = baseName + "." + TextureHelpers::textureExts[i];
		if ( BWResource::fileExists( filename ) )
		{
			ERROR_MSG( "Could not convert texture %s. A higher priorty texture was found %s.",
				sourcefile.c_str(), BWResource::resolveFilename( filename ).c_str() );
			return false;
		}
	}

	MF_ASSERT( s_pTool );

	BW::string textureDetailsName = textureDetailLevelManager_.getTextureDetailLevelsName();
	textureDetailLevelManager_.init( textureDetailsName );

	BW::string dissolvedName = BWResource::dissolveFilename( sourcefile );
	Moo::TextureDetailLevelPtr lod = 
		textureDetailLevelManager_.detailLevel( dissolvedName );

	// Convert to a non compressed dds
	BW::string outputfile = BWResource::changeExtension( sourcefile, ".dds" );
	compiler.resolveOutputPath( outputfile );
	// TODO ensureAbsolutePathExists is a very easy call to forget.
	// Maybe call this in resolve output path?
	BWResource::ensureAbsolutePathExists( outputfile );
	if (!s_pTool->convert( sourcefile, outputfile, lod, false ))
	{
		ERROR_MSG("Could not convert texture %s\n", sourcefile.c_str() );
		textureDetailLevelManager_.fini();
		return false;
	}
	outputFiles.push_back( outputfile );

	if (lod->isCompressed())
	{
		// Convert to a compressed dds
		outputfile = BWResource::changeExtension( sourcefile, ".c.dds" );
		compiler.resolveOutputPath( outputfile );
		if (!s_pTool->convert( sourcefile, outputfile, lod, true ))
		{
			ERROR_MSG("Could not convert compressed texture %s\n", sourcefile.c_str() );
			textureDetailLevelManager_.fini();
			return false;
		}
		outputFiles.push_back( outputfile );
	}

	textureDetailLevelManager_.fini();
	
	return true;
}
BW_END_NAMESPACE
