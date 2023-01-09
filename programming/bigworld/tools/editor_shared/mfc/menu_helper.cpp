#include "menu_helper.hpp"
#include "guimanager/gui_item.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

//==============================================================================
MenuHelper::MenuHelper( HWND hWnd )
	: hWnd_( hWnd )
{
}

//==============================================================================
void * MenuHelper::getMenu()
{
	if (hWnd_ != NULL)
	{
		return GetMenu( hWnd_ );
	}
	return CreatePopupMenu();
}

//==============================================================================
int MenuHelper::getMenuItemCount( void * menu )
{
	return GetMenuItemCount( ( HMENU ) menu );
}


//==============================================================================
void MenuHelper::deleteMenu( void * menu, int index )
{
	DeleteMenu( ( HMENU ) menu, index, MF_BYPOSITION );
}


//==============================================================================
void MenuHelper::setMenuInfo( void * menu, void * menuInfo )
{
	SetMenuInfo( ( HMENU ) menu, ( MENUINFO * ) menuInfo );
}


//==============================================================================
void MenuHelper::destroyMenu( void * menu )
{
	if (hWnd_ != NULL)
	{
		DestroyMenu( ( HMENU ) menu );
	}
}


//==============================================================================
GUI::Item * MenuHelper::getMenuItemInfo(
	void * menu,
	unsigned int index,
	void * typeData )
{
	MENUITEMINFO info = { sizeof( info ),
		MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA | MIIM_FTYPE | MIIM_ID |
		MIIM_STATE | MIIM_STRING | MIIM_SUBMENU };

	if(typeData != NULL)
	{
		info.cch = MAX_MENU_TEXT;
		info.dwTypeData = (TCHAR*)typeData;
	}
	if( GetMenuItemInfo(
		( HMENU ) menu, index, TRUE, &info ) == FALSE )
	{
		return NULL;
	}
	return ( GUI::Item * ) info.dwItemData;
}


//==============================================================================
void MenuHelper::setMenuItemInfo( void * menu, unsigned int index, void * info )
{
	SetMenuItemInfo( ( HMENU ) menu, index, TRUE, ( LPMENUITEMINFOW ) info );
}


//==============================================================================
void MenuHelper::insertMenuItem(
	void * menu, unsigned int index, GUI::ItemPtr item )
{
	MENUITEMINFO info = { sizeof( info ), MIIM_STRING | MIIM_ID | MIIM_DATA, 0, 0,
		item->commandID(), NULL, NULL, NULL, (DWORD_PTR)item.getObject(), L"menu" };
	InsertMenuItem(
		( HMENU ) menu, index, MF_BYPOSITION, ( LPMENUITEMINFOW ) &info );
}

//==============================================================================
void MenuHelper::modifyMenu(
	void * menu,
	unsigned int index,
	ItemPtr info,
	const wchar_t * string )
{
	ModifyMenu(
		(HMENU ) menu, index,
		MF_BYPOSITION | MF_STRING | MIIM_ID, ( UINT_PTR ) info->commandID(), string );
}


//==============================================================================
void MenuHelper::enableMenuItem(
	void * menu, unsigned int index, MenuState state )
{
	unsigned int enableState = MF_BYPOSITION;
	if( state & MS_ENABLED )
	{
		enableState |= MF_ENABLED;
	}
	if( state & MS_GRAYED )
	{
		enableState |= MF_GRAYED;
	}
	EnableMenuItem( ( HMENU ) menu, index, enableState );
}


//==============================================================================
void MenuHelper::checkMenuItem(
	void * menu, unsigned int index, MenuCheckState state )
{
	unsigned int checkState = MF_BYPOSITION;
	if( state & MCS_CHECKED )
	{
		checkState |= MF_CHECKED;
	}
	if( state & MCS_UNCHECKED )
	{
		checkState |= MF_UNCHECKED;
	}
	CheckMenuItem( ( HMENU ) menu, index, checkState );
}


//==============================================================================
void MenuHelper::setSeperator( void * menu, unsigned int index )
{
	MENUITEMINFO info = { sizeof( info ), MIIM_FTYPE, MFT_SEPARATOR };
	SetMenuItemInfo( ( HMENU ) menu, index, TRUE, &info );
}

//==============================================================================
void * MenuHelper::setSubMenu( void * hMenu, unsigned index )
{
	MENUITEMINFO info = { sizeof( info ), MIIM_SUBMENU };
	GetMenuItemInfo( (HMENU) hMenu, index, MF_BYPOSITION, &info );
	if (info.hSubMenu == NULL )
	{
		info.hSubMenu = CreateMenu();
		SetMenuItemInfo( ( HMENU )hMenu, index, MF_BYPOSITION, &info );
	}
	return info.hSubMenu;
}

//==============================================================================
void MenuHelper::updateText(
	void * hMenu, unsigned index, const BW::wstring & newName )
{
	MENUITEMINFO info = { sizeof( info ),
		MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA | MIIM_FTYPE | MIIM_ID |
		MIIM_STATE | MIIM_STRING | MIIM_SUBMENU };
	info.cch = MAX_MENU_TEXT;

	GetMenuItemInfo( ( HMENU ) hMenu, index, true, &info );
	if (info.dwTypeData == NULL ||
		newName != ( LPCTSTR ) info.dwTypeData )
	{
		GUI::ItemPtr itemPtr = ( GUI::Item * ) info.dwItemData;
		modifyMenu(
			hMenu, index, itemPtr,
			newName.c_str() );
	}
}


END_GUI_NAMESPACE
BW_END_NAMESPACE
