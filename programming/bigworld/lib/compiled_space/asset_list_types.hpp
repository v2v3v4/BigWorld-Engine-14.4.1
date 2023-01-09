#ifndef ASSET_LIST_TYPES_HPP
#define ASSET_LIST_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"


namespace BW {
namespace CompiledSpace {

	namespace AssetListTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWAL" );
		const uint32 FORMAT_VERSION = 0x00000001;

		typedef uint8 AssetType;

		const AssetType	ASSET_TYPE_UNKNOWN_TYPE	= 0;
		const AssetType ASSET_TYPE_DATASECTION	= 1;
		const AssetType	ASSET_TYPE_TEXTURE		= 2;
		const AssetType	ASSET_TYPE_EFFECT		= 3;
		const AssetType	ASSET_TYPE_PRIMITIVE	= 4;
		const AssetType	ASSET_TYPE_VISUAL		= 5;
		const AssetType	ASSET_TYPE_MODEL		= 6;

		typedef uint32 Index;

		struct Entry
		{
			AssetType type_;
			Index stringTableIndex_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // SRTING_TABLE_TYPES_HPP
