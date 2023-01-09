#ifndef STATIC_GEOMETRY_WRITER_HPP
#define STATIC_GEOMETRY_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "../static_geometry_types.hpp"
#include "space_writer.hpp"

namespace BW {
namespace CompiledSpace {

class StringTableWriter;
class AssetListWriter;
class BinaryFormatWriter;

class StaticGeometryWriter :
	public ISpaceWriter
{
public:
	StaticGeometryWriter();
	~StaticGeometryWriter();

	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual void postProcess();
	virtual bool write( BinaryFormatWriter& writer );

	void extractCandidates( const StringTableWriter& stringTable,
		const AssetListWriter& assetList );

	size_t size() const;

private:
	BW::vector<BW::string> primitiveReferences_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_GEOMETRY_WRITER_HPP
