#ifndef SCENE_BROWSER_LIST_HPP
#define SCENE_BROWSER_LIST_HPP


#include "world/item_info_db.hpp"
#include "checkbox_helper.hpp"
#include "list_selection.hpp"
#include "list_column.hpp"
#include "list_column_states.hpp"
#include "list_group_states.hpp"
#include "list_search_filters.hpp"

BW_BEGIN_NAMESPACE

class SetupItemsBackgroundTask;

/**
 *	This class extends CListCtrl to implement SceneBrowser required
 *	functionality such as grouping, column sorting, item info update, etc.
 */
class SceneBrowserList : public CListCtrl
{
public:

	// SceneBrowserList class declarations

	typedef BW::list< ItemInfoDB::ItemPtr > ItemList;


	SceneBrowserList();
	virtual ~SceneBrowserList();

	bool init( DataSectionPtr pDS );

	bool tick( bool forceTick = false );

	void load( DataSectionPtr pDS );
	void save( DataSectionPtr pDS );

	void groupBy( ListGroup * group );
	const ListGroup * groupBy() const { return groupStates_.groupBy(); }
	void groups( ListGroups & retGroups ) const
										{ columnStates_.groups( retGroups ); }

	int numItems() const { return numItems_; }
	int numTris() const { return numTris_; }
	int numPrimitives() const { return numPrimitives_; }

	int numSelectedItems() const { return selHelper_.numItems(); }
	int numSelectedTris() const { return selHelper_.numSelectedTris(); }
	int numSelectedPrimitives() const
								{ return selHelper_.numSelectedPrimitives(); }

	bool hasSelectionChanged() const { return selHelper_.changed(); }
	void clearSelectionChanged() { selHelper_.resetChanged(); }
	const ItemList & selection( bool validate = true )
								{ return selHelper_.selection( validate ); }
	void selection( const BW::vector< ChunkItemPtr > & selection );

	void search( const BW::string & newSearch );

	bool needsTick() const;

	void scrollTo( ChunkItemPtr pItem );

	ListSearchFilters & searchFilters() { return searchFilters_; }

	void currentItems( BW::vector< ChunkItemPtr > & chunkItems ) const;

	// MFC overrides
	virtual void PreSubclassWindow();

	virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

protected:
	// MFC message handling
	afx_msg void OnGetDispInfo( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnOdStateChanged( LPNMHDR pNMHDR, LRESULT * pResult );
	afx_msg void OnItemChanged( LPNMHDR pNMHDR, LRESULT * pResult );
	afx_msg void OnHdrColumnReorderEnd( NMHDR *pNMHDR, LRESULT * pResult );
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
	DECLARE_MESSAGE_MAP()

	BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT * pResult );
	BOOL PreTranslateMessage( MSG * pMsg );
	INT_PTR OnToolHitTest( CPoint point, TOOLINFO * pTI ) const;

private:
	// Typedefs

	typedef BW::vector< ItemInfoDB::ItemPtr > ItemIndex;
	typedef BW::map< uint64, bool > ItemTextOverflow;


	// Member variables

	bool firstTick_;	// used to ensure it's updated when first created.
	ListGroupStates groupStates_;
	ItemIndex itemIndex_;
	int numItems_;
	int numTris_;
	int numPrimitives_;

	ListColumnStates columnStates_;

	ListSelection selHelper_;

	bool ignoreSelMessages_;

	ListColumns columns_;				// columns existing in the ItemInfoDB

	bool sortColumnAdded_;

	BW::string search_;
	bool searchChanged_;

	bool needsTick_;

	int lastMouseOverIdx_;

	SmartPointer<SetupItemsBackgroundTask> pSetupItemsTask_;

	CheckboxHelper checkboxHelper_;

	CImageList headerImgList_;

	CToolTipCtrl headerToolTips_;

	ItemTextOverflow itemTextOverflow_;

	ListSearchFilters searchFilters_;


	// Methods
	bool onLeftMouseDown( const CPoint & mouseDownPoint );

	void updateColumns( bool reSort );
	void updateColumnTooltips();
	void updateSorting( int colIdx, int sorting, bool reSort );
	
	int ctrlToCol( int ctrlIdx, bool assertOnOutOfBounds = true );
	int colToCtrl( int colIdx, bool assertOnOutOfBounds = true );
	
	void setupItems();

	ItemInfoDB::ItemPtr getItem( int index ) const;

	void drawInfoNumItems( const ItemInfoDB::ItemPtr & pItem,
		COLORREF textCol, COLORREF bgCol, CDC & dc, BW::string & retText );

	void drawNumItemsPostfix( CDC & dc, const BW::wstring & mainText,
			int ctrlCol, const BW::vector<BW::wstring> & colText,
			const BW::string & numItemsText, const RECT & itemRect );

	void drawExpandCollapseBtn( bool expanded, CDC & dc,
						COLORREF textCol, COLORREF bgCol, const CRect & rect );

	void drawItemColours( bool selected, int idx, COLORREF & textCol,
				COLORREF & grayTextCol, COLORREF & bgCol, COLORREF & bgHiCol );

	void drawItemColumn( CDC & dc, int ctrlCol, CRect & rect,
						const BW::wstring & text, bool selected, int idx );

	void checkboxRect( int idx, int ctrlIdx, CRect & retRect );

	void restoreSelection();

	bool handleItemClick( ItemInfoDB::ItemPtr pItem, int idx, int ctrlCol,
														const CPoint & pt );

	void finishSetupItems();

	void finishSetupItemsThread();
	
	ItemInfoDB::ComparerPtr currentComparer() const;

	void redraw();

	void updateHeaderColumnWidth( NMHDR *pNMHDR );
};

BW_END_NAMESPACE

#endif // SCENE_BROWSER_LIST_HPP
