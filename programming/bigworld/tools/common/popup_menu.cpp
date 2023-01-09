/*
 *  Generic popup menu helper class
 */

#include "pch.hpp"
#include "cstdmf/guard.hpp"
#include <stack>
#include "popup_menu.hpp"

BW_BEGIN_NAMESPACE

PopupMenu::PopupMenu()
{
}

PopupMenu::PopupMenu( Items& items ) :
	items_( items )
{
}

int PopupMenu::doModal( HWND parent )
{
	BW_GUARD;

	POINT pt;
	GetCursorPos( &pt );

	return doModal( parent, pt );
}

int PopupMenu::doModal( HWND parent, const POINT & pt )
{
	BW_GUARD;

	if ( items_.empty() )
		return 0;

	HMENU menu = ::CreatePopupMenu();
	MENUITEMINFO info = { sizeof(info),
		MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE,
		MFT_STRING, MFS_DEFAULT, 0, 0, 0, 0, 0, L"", 0, 0 };
	int pos = 0;

	std::stack<HMENU> menuStack;
	menuStack.push( menu );
	for( PopupMenu::Items::iterator i = items_.begin();
		i != items_.end(); ++i )
	{
		HMENU curMenu = menuStack.top();
		const wchar_t * str = (*i).first.c_str();
		bool checked = false;
		if ( wcslen( str ) >= 2 && str[0] == L'#' && str[1] == L'#' )
		{
			str += 2;
			checked = true;
		}
		if ( (*i).second == PopupMenu::Separator )
		{
			info.fType = MFT_SEPARATOR;
			::InsertMenuItem( curMenu, pos++, TRUE, &info );
		}
		else if ( (*i).second == PopupMenu::StartSubmenu )
		{
			info.dwTypeData = const_cast< wchar_t * > ( str );
			info.fMask |= MIIM_SUBMENU;
			info.fType = MFT_STRING;
			info.fState = checked ? MFS_CHECKED : 0;
			info.wID = (*i).second;
			info.hSubMenu = ::CreateMenu();
			::InsertMenuItem( curMenu, pos++, TRUE, &info );
			info.fMask = info.fMask & ~MIIM_SUBMENU;
			menuStack.push( info.hSubMenu );
		}
		else if ( (*i).second == PopupMenu::EndSubmenu )
		{
			if ( menuStack.size() > 1 )
				menuStack.pop();
		}
		else
		{
			info.dwTypeData = const_cast< wchar_t * > ( str );
			info.fState = checked ? MFS_CHECKED : 0;
			info.fType = MFT_STRING;
			info.wID = (*i).second;
			::InsertMenuItem( curMenu, pos++, TRUE, &info );
		}
	}

	int result = 0;
	if ( ::GetMenuItemCount( menu ) )
	{
		result = TrackPopupMenu( menu, TPM_RETURNCMD | TPM_LEFTBUTTON,
			pt.x, pt.y, 0, parent, NULL );
	}

	DestroyMenu( menu );

	return result;
}

//statics
void PopupMenu::addItem( Items& in, const BW::wstring& name, int id )
{
	BW_GUARD;

	in.push_back( PopupMenu::Item( name, id ) );
}

// adds all items in the Items vector
void PopupMenu::addItems( Items& in, Items& items )
{
	BW_GUARD;

	for( PopupMenu::Items::iterator i = items.begin();
		i != items.end(); ++i )
		in.push_back( *i );
}
BW_END_NAMESPACE

