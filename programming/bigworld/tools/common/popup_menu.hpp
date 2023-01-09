/*
 *  Generic popup menu helper class
 */

#ifndef POPUP_MENU_HPP
#define POPUP_MENU_HPP

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class PopupMenu
{
public:
	typedef std::pair<BW::wstring,int> Item;
	typedef BW::vector<Item> Items;
	
	static const int Separator = 0;
	static const int StartSubmenu = -1;
	static const int EndSubmenu = -2;

	PopupMenu();
	PopupMenu( Items& items );

	// Menu Item display name and ID:
	// name: empty for separators and exit menu commands, string for items, start with ## to set checked
	// ID values:
	//     0 = separator
	//    -1 = Create a submenu and add subsequent items to it
	//    -2 = Finish submenu, so add subsequent items to the menu existing before -1
	//	  >0 = your ID, which will be returned by doModal if the item is clicked
	void addItem( const BW::wstring& name, int id ) { addItem( items_, name, id ); };

	// adds all items in the Items vector
	void addItems( Items& items ) { addItems( items_, items ); };

	// start/end submenues (can be nested)
	void startSubmenu( const BW::wstring& name ) { addItem( name, StartSubmenu ); };
	void endSubmenu() { addItem( L"", EndSubmenu ); };
	// adds a separator
	void addSeparator() { addItem( L"", Separator ); };

	// show and execute the menu.
	int doModal( HWND parent );

	// show and execute the menu, specifying the origin point.
	int doModal( HWND parent, const POINT & pt );

	bool empty() const { return items_.empty(); }

// STATICS versions that work directly on an Items vector
	// add an item
	static void addItem( Items& in, const BW::wstring& name, int id );

	// adds all items in the Items vector
	static void addItems( Items& in, Items& items );

	// start/end submenues (can be nested)
	static void startSubmenu( Items& in, const BW::wstring& name ) { addItem( in, name, StartSubmenu ); };
	static void endSubmenu( Items& in ) { addItem( in, L"", EndSubmenu ); };
	// adds a separator
	static void addSeparator( Items& in ) { addItem( in, L"", Separator ); };

private:
	Items items_;
};

BW_END_NAMESPACE
#endif // POPUP_MENU_HPP
