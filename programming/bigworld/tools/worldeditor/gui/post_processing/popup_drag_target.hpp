#ifndef POPUP_DRAG_TARGET_HPP
#define POPUP_DRAG_TARGET_HPP

BW_BEGIN_NAMESPACE

/**
 *	This class draws a translucent popup menu that is suitable for use when
 *	performing a drag & drop operation.
 */
class PopupDragTarget : public CWnd
{
public:
	typedef BW::vector< BW::string > ItemList;

	PopupDragTarget();
	virtual ~PopupDragTarget();

	CSize calcSize( const ItemList & itemList ) const;

	bool open( const CPoint & pt, const ItemList & itemList, bool arrowUp );

	BW::string update( int alpha );

	void close();

private:
	CFont font_;
	ItemList items_;
	bool arrowUp_;
};

BW_END_NAMESPACE

#endif // POPUP_DRAG_TARGET_HPP
