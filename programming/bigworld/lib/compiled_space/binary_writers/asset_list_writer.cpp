#include "pch.hpp"

#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "string_table_writer.hpp"

#include "resmgr/quick_file_writer.hpp"

#include <limits>

namespace BW {
namespace CompiledSpace {


// ----------------------------------------------------------------------------
AssetListWriter::AssetListWriter()
{

}


// ----------------------------------------------------------------------------
AssetListWriter::~AssetListWriter()
{

}


// ----------------------------------------------------------------------------
bool AssetListWriter::write( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
		AssetListTypes::FORMAT_MAGIC, 
		AssetListTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	// Entries
	stream->write( assets_ );
	return true;
}


// ----------------------------------------------------------------------------
StringTableTypes::Index AssetListWriter::addAsset( 
		AssetListTypes::AssetType type,
		const BW::string& resourceID,
		StringTableWriter& stringTable )
{
	class Finder
	{
	public:
		Finder( StringTableTypes::Index idx ) : idx_( idx )
		{}

		bool operator ()( const AssetListTypes::Entry& entry ) 
		{
			return entry.stringTableIndex_ == idx_;
		}
	private:
		StringTableTypes::Index idx_;
	};

	StringTableTypes::Index tableIdx = stringTable.addString( resourceID );

	auto it = std::find_if( assets_.begin(), assets_.end(),
		Finder( tableIdx ) );
	if (it == assets_.end())
	{
		AssetListTypes::Entry entry = {
			type, tableIdx
		};
		assets_.push_back( entry );
	}

	return tableIdx;
}


// ----------------------------------------------------------------------------
size_t AssetListWriter::size() const
{
	return assets_.size();
}


// ----------------------------------------------------------------------------
size_t AssetListWriter::assetNameIndex( size_t idx ) const
{
	return assets_[idx].stringTableIndex_;
}


// ----------------------------------------------------------------------------
AssetListTypes::AssetType AssetListWriter::assetType( size_t idx ) const
{
	return assets_[idx].type_;
}

} // namespace CompiledSpace
} // namespace BW
