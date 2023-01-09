#ifndef SPACE_WRITER_HPP
#define SPACE_WRITER_HPP

#include "cstdmf/smartpointer.hpp"

namespace BW {

class CommandLine;
class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

namespace CompiledSpace {

class BinaryFormatWriter;
class AssetListWriter;
class StringTableWriter;
class ChunkConversionContext;

class ISpaceWriter
{
public:
	typedef ChunkConversionContext ConversionContext;

	ISpaceWriter();
	virtual ~ISpaceWriter();

	void configure( StringTableWriter* pStringTable, 
		AssetListWriter* pAssetList );

	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine ) = 0;
	virtual void postProcess() {}
	virtual bool write( BinaryFormatWriter& writer ) = 0;

protected:
	StringTableWriter& strings();
	AssetListWriter& assets();

private:

	StringTableWriter* pStringTable_; 
	AssetListWriter* pAssetList_;

};

} // namespace CompiledSpace
} // namespace BW

#endif // SPACE_COMPILER_HPP
