#include "pch.hpp"

#include "chunk_item.hpp"
#include "chunk.hpp"

#if UMBRA_ENABLE
#include "umbra_chunk_item.hpp"
#include <ob/umbraobject.hpp>
#endif

BW_BEGIN_NAMESPACE

uint32	ChunkItemBase::s_instanceCount_		= 0;
uint32	ChunkItemBase::s_instanceCountPeak_	= 0;

/**
 *	Constructor
 */
ChunkItemBase::ChunkItemBase( WantFlags wantFlags ) :
	drawMark_(0),
	wantFlags_( wantFlags ), 
	pChunk_( NULL ),
#if UMBRA_ENABLE
	pUmbraDrawItem_( NULL ),
	pUmbraShadowCasterTestItem_( NULL ),
#endif
	sceneObject_( this, SceneObjectFlags() )
{
#ifndef _RELEASE
	s_instanceCount_++;
	if ( s_instanceCount_ > s_instanceCountPeak_ )
	{
		s_instanceCountPeak_ = s_instanceCount_;
	}
#endif
}

/**
 *	Copy constructor
 */
ChunkItemBase::ChunkItemBase( const ChunkItemBase & oth ) :
	BW::SafeReferenceCount( oth ),
	drawMark_(0),
	wantFlags_( oth.wantFlags_ ), 
	pChunk_( oth.pChunk_ ),
#if UMBRA_ENABLE
	pUmbraDrawItem_( NULL ),
	pUmbraShadowCasterTestItem_( NULL ),
#endif
	sceneObject_( oth.sceneObject_ )
{
#ifndef _RELEASE
	s_instanceCount_++;
	if ( s_instanceCount_ > s_instanceCountPeak_ )
		s_instanceCountPeak_ = s_instanceCount_;
#endif
}

/**
 *	Destructor
 */
ChunkItemBase::~ChunkItemBase()
{
	// Note, we explicitly dereference pointers so ensuing destruction can be
	// profiled.
	PROFILER_SCOPED( ChunkItemBase_destruct );

#if UMBRA_ENABLE
	// Delete any umbra draw item
	bw_safe_delete(pUmbraDrawItem_);
	bw_safe_delete(pUmbraShadowCasterTestItem_);
#endif

	s_instanceCount_--;
}


/**
 *	Utility method to implement 'lend' given a (world space) bounding box
 */
void ChunkItemBase::lendByBoundingBox( Chunk * pLender,
	const BoundingBox & worldbb )
{
	BW_GUARD;
	int allInOwnChunk = pChunk_->isOutsideChunk() ? 0 : -1;
	// assume it's not all within its own chunk if the item
	// is in an outside chunk (i.e. if bb test passes then that's
	// good enough to loan for us)

	// go through every bound portal
	Chunk::piterator pend = pLender->pend();
	for (Chunk::piterator pit = pLender->pbegin(); pit != pend; pit++)
	{
		if (!pit->hasChunk()) continue;

		Chunk * pConsider = pit->pChunk;

		// if it's not in that chunk's bounding box
		// then it definitely doesn't want it
		if (!worldbb.intersects( pConsider->boundingBox() )) continue;

		// if that's an outside chunk and the item is completely
		// within its own chunk then it also doesn't want it
		if (pConsider->isOutsideChunk())
		{
			// don't bother checking this for inside chunks since they're
			// not allowed to have interior chunks (i.e. bb is good enough)
			if (allInOwnChunk < 0)
			{
				// should really check if it's not completely within the union
				// of all interior chunks, but checking just its own is an OK
				// approximation ... if we had the hull tree at this stage then
				// we could do a different test using findChunkFromPoint, but we
				// don't, and it would miss some cases too, so this will do
				Vector3 bbpts[2] = { worldbb.minBounds(), worldbb.maxBounds() };
				Vector3 tpoint;	// this simple algorithm obviously only works
				int i;			// if our own chunk has no interior chunks
				for (i = 0; i < 8; i++)
				{
					tpoint.x = bbpts[(i>>0)&1].x;
					tpoint.y = bbpts[(i>>1)&1].y;
					tpoint.z = bbpts[(i>>2)&1].z;
					if (!pChunk_->contains( tpoint )) break;
				}
				allInOwnChunk = (i == 8);
				// if we are all in our own chunk (and we are in an inside
				// chunk, which is the only way we get here), then we can't
				// be in this chunk too... and furthermore we can't be any
				// any other chunks at all, so we can just stop here
				if (allInOwnChunk) break;
			}
			// if it's all within its own chunk then it can't be in this one
			//if (allInOwnChunk) continue;
			// ... but since we only calculate allInOwnChunk if our chunk is
			// an inside chunk, and if it were true we would have stopped
			// the loop already, then there's no point checking it again here,
			// in fact allInOwnChunk can only be zero here.
			MF_ASSERT_DEV( !allInOwnChunk );
			// could make the code a bit neater but this way is more logical
		}

		// ok so that chunk does really want this item then
		if (pConsider->addLoanItem( static_cast<ChunkItem*>(this) ))
		{
			pConsider->updateBoundingBoxes( static_cast<ChunkItem*>( this ) );
		}
	}
}


/**
 *	Constructor. Registers with chunk's static factory registry
 */
ChunkItemFactory::ChunkItemFactory(
		const BW::string & section,
		int priority,
		Creator creator ) :
	priority_( priority ),
	creator_( creator )
{
	BW_GUARD;
	Chunk::registerFactory( section, *this );
}


/**
 *	This virtual method calls the creator function that was passed in,
 *	as long as it's not NULL. It is called by a Chunk when it encounters
 *	the section name.
 *
 *	@return true if succeeded
 */
ChunkItemFactory::Result ChunkItemFactory::create( Chunk * pChunk,
	DataSectionPtr pSection ) const
{
	BW_GUARD;
	if (creator_ == NULL)
	{	
		BW::string errorStr = "No item factory found for section ";
		if ( pSection )
		{
			errorStr += "'" + pSection->sectionName() + "'";
		}
		else
		{
			errorStr += "<unknown>";
		}
		return ChunkItemFactory::Result( NULL, errorStr );
	}
	return (*creator_)( pChunk, pSection );
}

#ifndef MF_SERVER
/**
 *	This method is the specialised toss method for ChunkItem
 *	It makes sure the item is added to the spaces tick list
 *	if it wants to be ticked
 *	@param pChunk the chunk to toss the item into
 */
void ChunkItem::toss(Chunk* pChunk)	
{ 
	BW_GUARD;

	// Grab the current space
	ChunkSpace* pCurrentSpace = NULL;
	if (pChunk_)
	{
		pCurrentSpace = pChunk_->space();
	}

	ChunkSpace* pNewSpace = NULL;
	if (pChunk)
	{
		pNewSpace = pChunk->space();
	}

	if (pCurrentSpace != pNewSpace)
	{
		// If this item wants tick make sure it is in the appropriate
		// spaces tick list
		if (this->wantsTick())
		{
			// If the spaces are different remove the item from the
			// current space tick list and insert it into the new space
			if (pCurrentSpace)
			{
				pCurrentSpace->delTickItem( this );
			}
			if (pNewSpace)
			{
				pNewSpace->addTickItem( this );
			}
		}

		if (this->wantsUpdate())
		{
			if (pCurrentSpace)
			{
				pCurrentSpace->delUpdateItem( this );
			}
			if (pNewSpace)
			{
				pNewSpace->addUpdateItem( this );
			}
		}
	}

#if UMBRA_ENABLE
	if (pUmbraDrawItem_ != NULL)
	{
		Umbra::OB::Cell* pCell = NULL;
		if (pChunk)
		{
			pCell = pChunk->getUmbraCell();
		}
		pUmbraDrawItem_->pUmbraObject()->object()->setCell(pCell);
	}

	if ( pUmbraShadowCasterTestItem_ != NULL )
	{
		Umbra::OB::Cell* pCell = NULL;
		if ( pChunk )
		{
			pCell = pChunk->getUmbraCell();
		}

		pUmbraShadowCasterTestItem_->pUmbraObject()->object()->setCell( pCell );
	}
#endif

	// Call the base class toss
	this->SpecialChunkItem::toss(pChunk);
}
#endif // MF_SERVER

/**
 *	This method adds a chunk as a borrower of this item
 *	what this means is that the ChunkItem overlaps this chunk
 *	as well, which means we need to render as part of this chunk
 *	if it exists in a different umbra cell to our own cell
 *	@param pChunk the chunk that is borrowing us
 */
void ChunkItemBase::addBorrower( Chunk* pChunk )
{
	borrowers_.insert( pChunk );

#if UMBRA_ENABLE
	if (pUmbraDrawItem_)
	{
		if (this->chunk()->getUmbraCell() == NULL &&
			pChunk->getUmbraCell() != NULL)
		{
			pUmbraDrawItem_->updateCell( pChunk->getUmbraCell() );
		}
	}
#endif // UMBRA_ENABLE
}


/**
 *	This method removes a chunk as a borrower of this item
 *	@param pChunk the chunk that no longer wants to borrow us
 *	@see ChunkItemBase::addBorrower
 */
void ChunkItemBase::delBorrower( Chunk* pChunk )
{
	BW_GUARD;
	// If the chunk is one of our borrowers, continue
	Borrowers::iterator bit = borrowers_.find( pChunk );
	if (bit != borrowers_.end())
	{
		borrowers_.erase( bit );

#if UMBRA_ENABLE
		if (pUmbraDrawItem_ != NULL &&
			this->chunk()->getUmbraCell() == NULL &&
			pChunk->getUmbraCell() != NULL)
		{
			bool stillHasUmbraCell = false;

			for (bit = borrowers_.begin(); bit != borrowers_.end(); ++bit)
			{
				if ((*bit)->getUmbraCell() != NULL)
				{
					stillHasUmbraCell = true;
					break;
				}
			}
			
			if (!stillHasUmbraCell)
			{
				pUmbraDrawItem_->updateCell( NULL );
			}
		}
#endif // UMBRA_ENABLE
	}
}


/**
 *	This method removes a chunk as a borrower of this item
 *
 *	@see ChunkItemBase::addBorrower
 */
void ChunkItemBase::clearBorrowers()
{
	BW_GUARD;

	borrowers_.clear();

#if UMBRA_ENABLE
	if (this->chunk()->getUmbraCell() == NULL &&
		pUmbraDrawItem_ != NULL)
	{
		pUmbraDrawItem_->updateCell( NULL );
	}
#endif

}


#if UMBRA_ENABLE
/**
 *	This method updates all our umbra lender objects, this needs to be done
 *	if the Umbra draw item is recreated
 */
void ChunkItemBase::updateUmbraLenders()
{
	// Take a copy of all our borrowers
	Borrowers oldBorrowers = borrowers_;

	// Clear all the borrowers and lenders
	borrowers_.clear();

	// Re-add all the borrowers to recreate the lenders
	Borrowers::iterator bit = oldBorrowers.begin();
	for (; bit != oldBorrowers.end(); bit++)
	{
		this->addBorrower( *bit );
	}
}
#endif // UMBRA_ENABLE

BW_END_NAMESPACE

// chunk_item.cpp
