#include "pch.hpp"

#include "space_writer.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
ISpaceWriter::ISpaceWriter() :
	pAssetList_( NULL ),
	pStringTable_( NULL )
{

}


// ----------------------------------------------------------------------------
ISpaceWriter::~ISpaceWriter()
{

}


// ----------------------------------------------------------------------------
void ISpaceWriter::configure( StringTableWriter* pStringTable, 
	AssetListWriter* pAssetList )
{
	pStringTable_ = pStringTable;
	pAssetList_ = pAssetList;
}


// ----------------------------------------------------------------------------
StringTableWriter& ISpaceWriter::strings()
{
	MF_ASSERT( pStringTable_ );
	return *pStringTable_;
}


// ----------------------------------------------------------------------------
AssetListWriter& ISpaceWriter::assets()
{
	MF_ASSERT( pAssetList_ );
	return *pAssetList_;
}

} // namespace CompiledSpace
} // namespace BW
