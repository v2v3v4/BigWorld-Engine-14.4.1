#ifndef STATIC_TEXTURE_STREAMING_BINARY_WRITER_HPP
#define STATIC_TEXTURE_STREAMING_BINARY_WRITER_HPP

#include "../static_texture_streaming_types.hpp"
#include "space_writer.hpp"

#include "math/forward_declarations.hpp"

#include <memory>

namespace BW {

namespace Moo{
	class ModelTextureUsage;
}

typedef SmartPointer<class DataSection> DataSectionPtr;

namespace CompiledSpace {

class BinaryFormatWriter;
class StringTableWriter;

class StaticTextureStreamingWriter :
	public ISpaceWriter
{
public:
	StaticTextureStreamingWriter();
	~StaticTextureStreamingWriter();
	
	virtual bool initialize( const DataSectionPtr & pSpaceSettings,
		const CommandLine & commandLine );
	virtual void postProcess();
	virtual bool write( BinaryFormatWriter & writer );

	void convertModel( const ConversionContext & ctx,
		const DataSectionPtr & pItemDS, const BW::string & uid );
	void convertShell( const ConversionContext& ctx,
		const DataSectionPtr & pItemDS, const BW::string & uid );
	void convertSpeedTree( const ConversionContext& ctx,
		const DataSectionPtr & pItemDS, const BW::string & uid );

	bool addFromChunkModel( 
		const DataSectionPtr & pItemDS,
		const Matrix & chunkTransform,
		bool isShell,
		StringTableWriter & stringTable,
		AssetListWriter & assetList );

	bool addFromChunkTree( 
		const DataSectionPtr & pItemDS,
		const Matrix & chunkTransform,
		StringTableWriter & stringTable,
		AssetListWriter & assetList );

private:
	class Detail;
	std::auto_ptr<Detail> detail_;

	BW::vector<StaticTextureStreamingTypes::Usage> usages_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_TEXTURE_STREAMING_BINARY_WRITER_HPP
