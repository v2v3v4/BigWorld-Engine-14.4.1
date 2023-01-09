#include "pch.hpp"

#include "entity_list.hpp"
#include "entity_list_types.hpp"

#include "cstdmf/profiler.hpp"
#include "resmgr/packed_section.hpp"


namespace BW {
namespace CompiledSpace {

//-----------------------------------------------------------------------------
EntityList::EntityList() :
	pReader_( NULL ),
	pStream_( NULL )
{

}


//-----------------------------------------------------------------------------
EntityList::~EntityList()
{
	this->close();
}


//-----------------------------------------------------------------------------
bool EntityList::read( BinaryFormat& reader, const FourCC& sectionMagic )
{
	PROFILER_SCOPED( EntityList_readFromSpace );

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection( sectionMagic,
		EntityListTypes::FORMAT_VERSION, "StringTable" );
	if (!pStream_)
	{
		this->close();
		return false;
	}

	BinaryPtr pRawBin = new BinaryBlock(
		pStream_->readRaw( pStream_->length() ),
		pStream_->length(),
		"EntityList_read", true ); // true == we retain ownership on this data
	MF_ASSERT( pRawBin );
	MF_ASSERT( pRawBin->data() );

	pPackedSection_ = PackedSection::create( "EntityList", pRawBin );
	if (!pPackedSection_)
	{
		ASSET_MSG( "EntityList malformed in '%s'\n", pReader_->resourceID() );
		this->close();
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
void EntityList::close()
{
	pPackedSection_ = NULL;

	if (pReader_ && pStream_)
	{
		pReader_->closeSection( pStream_ );
	}

	pStream_ = NULL;
	pReader_ = NULL;
}


//-----------------------------------------------------------------------------
bool EntityList::isValid() const
{
	return pPackedSection_ != NULL;
}


//-----------------------------------------------------------------------------
const DataSectionPtr& EntityList::dataSection()
{
	return pPackedSection_;
}




} // namespace CompiledSpace
} // namespace BW