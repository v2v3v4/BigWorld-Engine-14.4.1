#include "pch.hpp"

#include "umbra_config.hpp"

#if UMBRA_ENABLE

#include "umbra_draw_item.hpp"

#include <ob/umbracell.hpp>
#include <ob/umbraobject.hpp>

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );


BW_BEGIN_NAMESPACE

/*
 * Destructor
 */
UmbraDrawItem::~UmbraDrawItem()
{
	// Make sure we remove our draw item from the cell
	// As the cell contains a reference to the draw item
	if (pObject_.exists() && pObject_->object())
		updateCell( NULL );
}


/**
 *	This helper method updates the umbra objects cell
 *	@param pNewCell the cell to move the object to
 */
void UmbraDrawItem::updateCell( Umbra::OB::Cell* pNewCell )
{
	MF_ASSERT( pObject_.exists() && pObject_->object() );
	pObject_->object()->setCell( pNewCell );
}

/**
 *	This helper method updates the umbra objects transform
 *	@param newTransform the new transform of the object
 */
void UmbraDrawItem::updateTransform( const Matrix& newTransform )
{
	MF_ASSERT( pObject_.exists() && pObject_->object() );
	pObject_->object()->setObjectToCellMatrix( (Umbra::Matrix4x4&)newTransform );
}

BW_END_NAMESPACE

#endif // UMBRA_ENABLE

// umbra_draw_item.cpp
