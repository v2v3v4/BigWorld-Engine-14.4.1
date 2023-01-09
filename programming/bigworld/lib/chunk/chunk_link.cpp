#include "pch.hpp"
#include "chunk_link.hpp"
#include "chunk_stationnode.hpp"

#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is the ChunkLink constructor.
 */
ChunkLink::ChunkLink() :
    direction_(DIR_NONE)
{
}


/**
 *  This is the ChunkLink destructor.
 */
ChunkLink::~ChunkLink()
{
}


/**
 *  This function returns the start item linked to.
 *
 *  @return         The start item.
 */
ChunkItemPtr ChunkLink::startItem() const
{
    return startItem_;
}



/**
 *  This function sets the start item linked to.
 *
 *  @param item     The new start item (this can be NULL).
 */
void ChunkLink::startItem(ChunkItemPtr item)
{
    startItem_ = item;
}


/**
 *  This function returns the end item linked to.
 *
 *  @return         The start item.
 */
ChunkItemPtr ChunkLink::endItem() const
{
    return endItem_;
}


/**
 *  This function sets the end item linked to.
 *
 *  @param item     The new end item (this can be NULL).
 */
void ChunkLink::endItem(ChunkItemPtr item)
{
    endItem_ = item;
}


/**
 *  This function gets the direction of the link.
 *
 *  @returns        The direction of the link.
 */
ChunkLink::Direction ChunkLink::direction() const
{
    return direction_;
}


/**
 *  This function sets the direction of the link.
 *
 *  @param dir      The new link direction.
 */
void ChunkLink::direction(Direction dir)
{
    direction_ = dir;
}


#ifdef EDITOR_ENABLED


/**
 *  This function validates the link.
 *
 *  @param failureMsg   This will be set to the failure message if invalid.
 *  
 *  @return             True if the link is valid, false otherwise.
 */
/*virtual*/ bool ChunkLink::isValid(BW::string &failureMsg) const
{
    BW_GUARD;
	if (startItem_ == NULL)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/FRAMEWORK/CHUNK_LINK/NO_START_ITEM");
        return false;
    }

    if (endItem_ == NULL)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/FRAMEWORK/CHUNK_LINK/NO_END_ITEM");
        return false;
    }

    if (direction_ == DIR_NONE)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/FRAMEWORK/CHUNK_LINK/DIRECTION_IS_NONE");
        return false;
    }

    if 
    (
        startItem_->isEditorChunkStationNode() 
        &&
        endItem_->isEditorChunkStationNode()
    )
    {
        ChunkStationNode *start = (ChunkStationNode *)startItem_.getObject();
        ChunkStationNode *end   = (ChunkStationNode *)endItem_  .getObject();
        if (start->graph() != end->graph())
        {
            failureMsg = LocaliseUTF8(L"CHUNK/FRAMEWORK/CHUNK_LINK/NODES_IN_DIFFERENT_GRAPH");
            return false;
        }
    }

    if (chunk() == NULL)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/FRAMEWORK/CHUNK_LINK/LINK_NOT_IN_CHUNK");
        return false;
    }

    return true;
}


/**
 *  This function is used to make the link 'dirty'.  Derived classes can
 *  use it to force the link and it's end points to a dirty state without
 *  having to do lots of casts on the link-type.
 */
/*virtual*/ void ChunkLink::makeDirty()
{
}

#endif // EDITOR_ENABLED

BW_END_NAMESPACE

// chunk_link.cpp

