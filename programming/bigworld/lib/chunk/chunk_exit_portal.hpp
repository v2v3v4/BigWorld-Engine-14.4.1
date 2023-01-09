#ifndef CHUNK_EXIT_PORTAL_HPP
#define CHUNK_EXIT_PORTAL_HPP

#include "chunk_item.hpp"
#include "chunk_manager.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a chunk item that represents an
 *	exit portal.  If we are drawn, you can be sure
 *	that there has been some 'inside/outside' action
 *	going on.
 */
class ChunkExitPortal : public ChunkItem
{
public:
	ChunkExitPortal( struct ChunkBoundary::Portal& p );
	bool load( DataSectionPtr pSection );

	///Use this method from external classes to check
	///which exit portals have been traversed this frame.
	typedef BW::vector<ChunkExitPortal*> Vector;
	static ChunkExitPortal::Vector& seenExitPortals();
	struct ChunkBoundary::Portal& portal() const { return portal_; }

#ifdef EDITOR_ENABLED
	DECLARE_EDITOR_CHUNK_ITEM_CLASS_NAME( "ChunkExitPortal" );
	virtual void edBounds( BoundingBox & bbRet ) const;
	virtual void edPostClone( EditorChunkItem* srcItem ) {} 
	virtual void edPostCreate()  {} 
	virtual void edPostModify() {} 

	virtual DataSectionPtr pOwnSect();
	virtual const DataSectionPtr pOwnSect()	const;
#endif

	void draw( Moo::DrawContext& drawContext );

private:
	struct ChunkBoundary::Portal&	portal_;
	static ChunkExitPortal::Vector seenExitPortals_;

#ifdef EDITOR_ENABLED
	DataSectionPtr pOwnSection_;
#endif
};

BW_END_NAMESPACE

#endif // CHUNK_EXIT_PORTAL_HPP
