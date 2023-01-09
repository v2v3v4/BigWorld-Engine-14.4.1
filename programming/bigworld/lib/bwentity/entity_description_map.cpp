#include "pch.hpp"

#include "bwentity_types.hpp"
#include "entity_description_map.hpp"

#include "entitydef/entity_description.hpp"

BWENTITY_BEGIN_NAMESPACE

EntityDescriptionMap::EntityDescriptionMap( 
		const BW::EntityDescriptionMap * ptr ) :
	pimpl_( ptr ),
	pConstants_( NULL )
{
}

EntityDescriptionMap::~EntityDescriptionMap()
{
	if (pConstants_)
	{
		delete pConstants_;
	}
}
	
bool EntityDescriptionMap::nameToIndex( const BW::string & name,
	EntityTypeID& index ) const
{
	MF_ASSERT( pimpl_ );
	
	return pimpl_->nameToIndex( name, index );
}

int	EntityDescriptionMap::size() const
{
	MF_ASSERT( pimpl_ );
	
	return pimpl_->size();
}

const BW::EntityDescription &
EntityDescriptionMap::entityDescription( EntityTypeID index ) const
{
	MF_ASSERT( pimpl_ );

	return pimpl_->entityDescription( index );
}

const BW::EntityDefConstants& EntityDescriptionMap::connectionConstants()
{
	MF_ASSERT( pimpl_ );
	
	if (pConstants_ == NULL) 
	{
		pConstants_ = new BW::EntityDefConstants ( pimpl_->digest(),
			pimpl_->maxExposedClientMethodCount(),
			pimpl_->maxExposedBaseMethodCount(),
			pimpl_->maxExposedCellMethodCount() );
	}
	
	MF_ASSERT( pConstants_ );

	return *pConstants_;
}

BWENTITY_END_NAMESPACE
