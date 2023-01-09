#ifndef LIST_SELECTION_HPP
#define LIST_SELECTION_HPP

#include "cstdmf/bw_unordered_set.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class helps to keeps track of a ItemInfoDB item list selection.
 */
class ListSelection
{
public:
	typedef BW::list< ItemInfoDB::ItemPtr > ItemList;

	ListSelection();

	void init( CListCtrl * pList );

	void filter( bool eraseMissingItems = true );

	void restore( const BW::vector< ItemInfoDB::ItemPtr > & items );

	void update( const BW::vector< ItemInfoDB::ItemPtr > & items );

	int numItems() const { return (int)selection_.size(); }
	int numSelectedTris() const;
	int numSelectedPrimitives() const;

	const ItemList & selection( bool validate );
	void selection( const BW::vector< ChunkItemPtr > & selection,
					const BW::vector< ItemInfoDB::ItemPtr > & items  );

	void deleteSelectedItems() const;

	bool needsUpdate() const { return needsUpdate_; }
	void needsUpdate( bool val ) { needsUpdate_ = val; }

	bool changed() const { return changed_; }
	void resetChanged() { changed_ = false; }

private:
	typedef BW::unordered_set< ChunkItem * > ChunkItemHash;

	CListCtrl * pList_;

	ItemList selection_;
	ChunkItemHash selectionHash_; // for speed
	bool needsUpdate_;
	bool changed_;
};

BW_END_NAMESPACE

#endif // LIST_SELECTION_HPP
