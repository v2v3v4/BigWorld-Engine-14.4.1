#ifndef CHUNK_ITEM_PLACER_HPP
#define CHUNK_ITEM_PLACER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_item.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

class ChunkItemExistenceOperation : public UndoRedo::Operation
{
public:
	ChunkItemExistenceOperation( ChunkItemPtr pItem, Chunk * pOldChunk ) :
		UndoRedo::Operation( 0 ),
		pItem_( pItem ),
		pOldChunk_( pOldChunk )
	{
		BW_GUARD;

		addChunk( pOldChunk_ );
		if( pItem_ )
			addChunk( pItem_->chunk() );
	}

protected:

	virtual void undo();

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		// these operations never replace each other
		return false;
	}

	ChunkItemPtr	pItem_;
	Chunk			* pOldChunk_;
};


class ChunkItemHideOperation : public UndoRedo::Operation
{
public:
	ChunkItemHideOperation( ChunkItemPtr pItem ) :
		UndoRedo::Operation( 0 ),
		pItem_( pItem )
	{
		BW_GUARD;

		if( pItem_ )
			addChunk( pItem_->chunk() );
	}

protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		// these operations never replace each other
		return false;
	}
	ChunkItemPtr	pItem_;
};


class ChunkItemFreezeOperation : public UndoRedo::Operation
{
public:
	ChunkItemFreezeOperation( ChunkItemPtr pItem ) :
		UndoRedo::Operation( 0 ),
		pItem_( pItem )
	{
		BW_GUARD;

		if( pItem_ )
			addChunk( pItem_->chunk() );
	}

protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		// these operations never replace each other
		return false;
	}
	UndoRedo::Operation* pOperation_;
	ChunkItemPtr	pItem_;
};


class LinkerExistenceOperation : public ChunkItemExistenceOperation
{
public:
	LinkerExistenceOperation( ChunkItemPtr pItem, Chunk * pOldChunk ) :
		ChunkItemExistenceOperation( pItem, pOldChunk )	{}

protected:
	/*virtual*/ void undo();
};


class CloneNotifier
{
	static BW::set<CloneNotifier*>* notifiers_;
public:
	CloneNotifier()
	{
		BW_GUARD;

		if( !notifiers_ )
			notifiers_ = new BW::set<CloneNotifier*>;
		notifiers_->insert( this );
	}
	~CloneNotifier()
	{
		BW_GUARD;

		notifiers_->erase( this );
		if( notifiers_->empty() )
		{
			bw_safe_delete(notifiers_);
		}
	}
	virtual void begin() = 0;
	virtual void end() = 0;

	static void beginClone()
	{
		BW_GUARD;

		if( notifiers_ )
		{
			for( BW::set<CloneNotifier*>::iterator iter = notifiers_->begin();
				iter != notifiers_->end(); ++iter )
			{
				(*iter)->begin();
			}
		}
	}
	static void endClone()
	{
		BW_GUARD;

		if( notifiers_ )
		{
			for( BW::set<CloneNotifier*>::iterator iter = notifiers_->begin();
				iter != notifiers_->end(); ++iter )
			{
				(*iter)->end();
			}
		}
	}
	class Guard
	{
	public:
		Guard()
		{
			BW_GUARD;

			CloneNotifier::beginClone();
		}
		~Guard()
		{
			BW_GUARD;

			CloneNotifier::endClone();
		}
	};
};




/**
 * The creation and deletion of chunk items is handled with a pair of functions
 * in the WorldEditor module, rather than with a functor. It didn't really make
 * sense to have a functor that immediately popped itself. The controlling
 * script could jump through some hoops to create and delete items, or we
 * could just let them do what they want.
 *
 *
 */
class ChunkItemPlacer
{
	typedef ChunkItemPlacer This;

public:
	PY_MODULE_STATIC_METHOD_DECLARE( py_createChunkItem )
	PY_MODULE_STATIC_METHOD_DECLARE( py_deleteChunkItems )
	PY_MODULE_STATIC_METHOD_DECLARE( py_cloneChunkItems )
	
	PY_MODULE_STATIC_METHOD_DECLARE( py_hideChunkItems )
	PY_MODULE_STATIC_METHOD_DECLARE( py_freezeChunkItems )

	static bool hideChunkItems( const BW::vector<ChunkItemPtr> & items, bool hide, bool keepSelection );
	static bool freezeChunkItems( const BW::vector<ChunkItemPtr> & items, bool freeze, bool keepSelection );

	static int s_currFenceCloneOpId;

private:
	static PyObject * createChunkItem(
		Chunk * pChunk, DataSectionPtr pSection,
		ToolLocator * pLoc, const Matrix & pose,
		int useLocPos, const Vector3 & snaps );
	static void removeFromSelected( ChunkItemPtr pItem );
};

BW_END_NAMESPACE


#endif // CHUNK_ITEM_PLACER_HPP
