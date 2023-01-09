#include "pch.hpp"
#include "gui_menu.hpp"
#include "cstdmf/string_utils.hpp"
#include <stack>
#include "i_menu_helper.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

//==============================================================================
Menu::Menu( const BW::string& root, IMenuHelper * menuHelper )
	: Subscriber( root )
	, helper_( menuHelper )
{
	BW_GUARD;

	hmenu_ = helper_->getMenu();
	if (hmenu_)
	{
		while( helper_->getMenuItemCount( hmenu_ ) != 0 )
		{
			helper_->deleteMenu( hmenu_, 0 );
		}
	}
	MENUINFO info = { sizeof( info ), MIM_BACKGROUND };
	info.hbrBack = GetSysColorBrush( COLOR_MENUBAR );
	helper_->setMenuInfo( hmenu_, &info );
}


//==============================================================================
Menu::~Menu()
{
	BW_GUARD;

	helper_->destroyMenu( hmenu_ );
}


//==============================================================================
void Menu::changed( ItemPtr item )
{
	BW_GUARD;

	std::stack< void * > menus;
	menus.push( hmenu_ );
	bool found = false;
	while( !menus.empty() )
	{
		void * hmenu = menus.top();
		menus.pop();
		for( int i = 0; i < helper_->getMenuItemCount( hmenu ); ++i )
		{
			Item * storedItem = helper_->getMenuItemInfo( hmenu, i, NULL );
			if( storedItem &&
				storedItem == item )
			{
				changed( hmenu, item );
			}
		}
	}
	if( !found )// we'll do a full refresh
		changed( hmenu_, rootItem() );
}


//==============================================================================
void Menu::updateItem(
	void * parentHMenu, int & index, ItemPtr item )
{
	if( item->type() == "SEPARATOR" )
	{
		helper_->setSeperator( parentHMenu, index );
		return;
	}

	if( item->type() == "GROUP" )
	{
		updateGroup( parentHMenu, index, item );
		return;
	}

	if( item->type() == "ACTION" )
	{
		updateAction( parentHMenu, index, item );
		return;
	}

	if( item->type() == "TOGGLE" )
	{
		updateToggle( parentHMenu, index, item );
		return;
	}

	if( item->type() == "CHOICE" )
	{
		updateChoice( parentHMenu, index, item );
		return;
	}

	if( item->type() == "EXPANDED_CHOICE" )
	{
		updateExpandedChoice( parentHMenu, index, item );
		return;
	}
}


//==============================================================================
void Menu::changed( void * hmenu, ItemPtr item )
{
	BW_GUARD;

	int i = 0;
	unsigned int j = 0;
	while( i < helper_->getMenuItemCount( hmenu ) )
	{
		Item * currentItem = helper_->getMenuItemInfo( hmenu, i, NULL );

		if( j < item->num() )
		{
			ItemPtr sub = ( *item )[ j ];
			if( currentItem != sub )
			{
				helper_->insertMenuItem( hmenu, i, sub );
				helper_->getMenuItemInfo( hmenu, i, NULL );
			}
			updateItem( hmenu, i, sub );
			++i;
			++j;
		}
		else
		{
			helper_->deleteMenu( hmenu, i );
		}
	}
	for(; j< item->num(); ++j )
	{
		ItemPtr sub = ( *item )[ j ];
		helper_->insertMenuItem( hmenu, i, sub );
		updateItem( hmenu, i, sub );
		++i;
	}
}


//==============================================================================
void Menu::updateAction( void * hmenu, int& index, ItemPtr item )
{
	BW_GUARD;
	helper_->updateText( hmenu, index, buildMenuText( item ) );
	helper_->enableMenuItem(
		hmenu, index, ( item->update() ?
			IMenuHelper::MS_ENABLED :
			IMenuHelper::MS_GRAYED ) );
}


//==============================================================================
void Menu::updateToggle( void * hmenu, int& index, ItemPtr item )
{
	BW_GUARD;

	helper_->updateText( hmenu, index, buildMenuText( item ) );
	if( item->num() != 2 )// exactly 2
		throw 1;
	helper_->enableMenuItem(
		hmenu, index, ( item->update() ?
			IMenuHelper::MS_ENABLED :
			IMenuHelper::MS_GRAYED ) );

	helper_->checkMenuItem(
		hmenu, index, ( ( *item )[ 0 ]->update() ?
			IMenuHelper::MCS_UNCHECKED :
			IMenuHelper::MCS_CHECKED ) );
}


//==============================================================================
void Menu::updateExpandedChoice( void * hmenu, int& index, ItemPtr item )
{
	BW_GUARD;

	helper_->updateText( hmenu, index, buildMenuText( item ) );
	void * subMenu = helper_->setSubMenu( hmenu, index );
	helper_->enableMenuItem( hmenu, index, 
		( item->num() && item->update() ?
			IMenuHelper::MS_ENABLED :
			IMenuHelper::MS_GRAYED ) );
	int subIndex = 0;
	updateChoice( subMenu, subIndex, item );
}


//==============================================================================
void Menu::updateChoice( void * hmenu, int& index, ItemPtr item )
{
	BW_GUARD;
	
	wchar_t txtBuf[ IMenuHelper::MAX_MENU_TEXT + 1 ];
	unsigned int subItemIndex = 0;
	while( index < helper_->getMenuItemCount( hmenu ) )
	{
		Item * storedItem = helper_->getMenuItemInfo( hmenu, index, &txtBuf );
		if (storedItem != item.getObject())
		{
			break;
		}

		if( subItemIndex < item->num() )
		{
			ItemPtr subItem = ( *item )[ subItemIndex ];
			if( !wcslen( txtBuf )|| buildMenuText( subItem ) != txtBuf )
			{
				helper_->insertMenuItem( hmenu, index, item );
			}

			helper_->modifyMenu( hmenu, index, subItem,
				buildMenuText( subItem ).c_str() );
			helper_->enableMenuItem(
				hmenu, index, ( item->update() ?
					IMenuHelper::MS_ENABLED :
					IMenuHelper::MS_GRAYED ) );
			helper_->checkMenuItem(
				hmenu, index, ( subItem->update() ?
					IMenuHelper::MCS_CHECKED :
					IMenuHelper::MCS_UNCHECKED ) );
			++index;
			++subItemIndex;
		}
		else
		{
			helper_->deleteMenu( hmenu, index );
		}
	}
	while( subItemIndex < item->num() )
	{
		ItemPtr subItem = ( *item )[ subItemIndex ];
		helper_->insertMenuItem( hmenu, index, item );
		helper_->modifyMenu( hmenu, index, subItem,
			buildMenuText( subItem ).c_str() );
		helper_->enableMenuItem(
			hmenu, index, ( item->update() ?
				IMenuHelper::MS_ENABLED :
				IMenuHelper::MS_GRAYED ) );
		helper_->checkMenuItem(
			hmenu, index, ( subItem->update() ?
				IMenuHelper::MCS_CHECKED :
				IMenuHelper::MCS_UNCHECKED ) );
		++subItemIndex;
	}
	--index;
}


//==============================================================================
void Menu::updateGroup( void * hmenu, int& index, ItemPtr item )
{
	BW_GUARD;

	helper_->updateText( hmenu, index, buildMenuText( item ) );
	void * subMenu = helper_->setSubMenu( hmenu, index );
	helper_->enableMenuItem(
		hmenu, index, ( item->update() ?
			IMenuHelper::MS_ENABLED :
			IMenuHelper::MS_GRAYED ) );
	changed( subMenu, item );
}


//==============================================================================
BW::wstring Menu::buildMenuText( ItemPtr item )
{
	BW_GUARD;

	BW::wstring text;
	bw_utf8tow( item->displayName(), text );
	if ( !item->shortcutKey().empty() )
	{
		BW::wstring wshortcutkey;
		bw_utf8tow( item->shortcutKey(), wshortcutkey );
		text += L"\t" + wshortcutkey;
	}
	return text;
}

END_GUI_NAMESPACE
BW_END_NAMESPACE

