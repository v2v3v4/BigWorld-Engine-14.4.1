#ifndef ASSET_PIPELINE_TEXTURE_CONVERTER
#define ASSET_PIPELINE_TEXTURE_CONVERTER

#include "asset_pipeline/conversion/converter.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "moo/texture_detail_level_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class ConvertTextureTool;
}

class TextureConverter : public Converter
{
protected:
	TextureConverter( const BW::string& params );
	~TextureConverter();

public:
	virtual bool createDependencies( const BW::string & sourcefile,
									 const Compiler & compiler,
									 DependencyList & dependencies );

	/* convert a source file. */
	virtual bool convert( const BW::string & sourcefile,
						  const Compiler & compiler,
						  BW::vector< BW::string > & intermediateFiles,
						  BW::vector< BW::string > & outputFiles );

public:
	static uint64 getTypeId() { return s_TypeId; }
	static const char * getVersion() { return s_Version; }
	static const char * getTypeName() { return "TextureConverter"; }
	static Converter * createConverter( const BW::string& params ) { return new TextureConverter( params ); }

	static const uint64 s_TypeId;
	static const char * s_Version;

	Moo::TextureDetailLevelManager textureDetailLevelManager_;
	static Moo::ConvertTextureTool * s_pTool;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_TEXTURE_CONVERTER
