#ifndef ASSET_LIST_WRITER_HPP
#define ASSET_LIST_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "../asset_list_types.hpp"
#include "../string_table_types.hpp"

namespace BW {
namespace CompiledSpace {

class BinaryFormatWriter;
class StringTableWriter;

class AssetListWriter
{
public:
	AssetListWriter();
	~AssetListWriter();
	
	bool write( BinaryFormatWriter& writer );

	StringTableTypes::Index addAsset( AssetListTypes::AssetType type,
		const BW::string& resourceID,
		StringTableWriter& stringTable );

	size_t size() const;

	size_t assetNameIndex( size_t idx ) const;
	AssetListTypes::AssetType assetType( size_t idx ) const;

private:
	BW::vector< AssetListTypes::Entry > assets_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // SRTING_TABLE_WRITER_HPP
