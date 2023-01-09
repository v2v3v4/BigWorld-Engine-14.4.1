#ifndef COMPILED_SPACE_ASSET_LIST_HPP
#define COMPILED_SPACE_ASSET_LIST_HPP

#include "compiled_space/forward_declarations.hpp"
#include "compiled_space/binary_format.hpp"

#include "asset_list_types.hpp"
#include "string_table_types.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

	class ReferenceCount;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class StringTable;

class COMPILED_SPACE_API AssetList
{
public:
	AssetList();
	~AssetList();

	bool read( BinaryFormat& reader );
	void close();

	bool isValid() const;
	size_t size() const;
	bool empty() const;

	AssetListTypes::AssetType assetType( size_t idx ) const;
	size_t stringTableIndex( size_t idx ) const;

	void preloadAssets( const StringTable& stringTable );
	void releasePreloadedAssets();

	float percentLoaded() const;

private:
	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<AssetListTypes::Entry> entries_;

private:
	class Detail;
	class IAssetReference;
	IAssetReference* preloadAsset( AssetListTypes::AssetType type,
		const StringRef& resourceID );
	
private:
	BW::vector<SmartPointer<IAssetReference> > preloadedAssets_;
	size_t numPreloadsProcessed_;
};

} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_ASSET_LIST_HPP
