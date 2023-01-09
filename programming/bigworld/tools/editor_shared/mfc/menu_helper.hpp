#ifndef MENU_HELPER_HPP
#define MENU_HELPER_HPP

#include <Windows.h>

#include "guimanager/i_menu_helper.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class MenuHelper
	: public IMenuHelper
{
public:
	MenuHelper( HWND hWnd );

	void * getMenu();

	int getMenuItemCount( void * menu );

	void deleteMenu( void * menu, int index );

	void setMenuInfo( void * menu, void * menuInfo );

	void destroyMenu( void * menu );

	GUI::Item * getMenuItemInfo( void * menu, unsigned int index, void * typeData );

	void setMenuItemInfo( void * menu, unsigned int index, void * info );

	void insertMenuItem( void * menu, unsigned int index, GUI::ItemPtr info );
	 
	void modifyMenu(
		void * menu,
		unsigned int index,
		GUI::ItemPtr info,
		const wchar_t * string );

	void enableMenuItem( void * menu, unsigned int index, MenuState state );

	void checkMenuItem(
		void * menu, unsigned int index, MenuCheckState state );

	void setSeperator( void * menu, unsigned int index );

	void updateText(
		void * hMenu, unsigned index, const BW::wstring & newName );

	void * setSubMenu( void * hMenu, unsigned index );

private:
	HWND hWnd_;
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif //MENU_HELPER_HPP