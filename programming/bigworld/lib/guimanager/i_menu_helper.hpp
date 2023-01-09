#ifndef I_MENU_HELPER_H
#define I_MENU_HELPER_H

#include "cstdmf/bw_namespace.hpp"
#include "guimanager/gui_forward.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class IMenuHelper
{
public:
	static const unsigned int MAX_MENU_TEXT = 1024;

	enum MenuState
	{
		MS_ENABLED = 1,
		MS_GRAYED = 1 << 1
	};

	enum MenuCheckState
	{
		MCS_UNCHECKED = 1,
		MCS_CHECKED = 1 << 1
	};

	virtual void * getMenu() = 0;
	virtual int getMenuItemCount( void * menu ) = 0;
	virtual void deleteMenu( void * menu, int index ) = 0;
	virtual void destroyMenu( void * menu ) = 0;
	virtual void setMenuInfo( void * menu, void * menuInfo ) = 0;
	virtual void setMenuItemInfo(
		void * menu, unsigned int index, void * info ) = 0;
	virtual GUI::Item * getMenuItemInfo(
		void * menu, unsigned int index, void * typeData ) = 0;
	virtual void insertMenuItem(
		void * menu, unsigned int index, GUI::ItemPtr info ) = 0;
	virtual void modifyMenu(
		void * menu, unsigned int index, GUI::ItemPtr info,
		const wchar_t * string ) = 0;
	virtual void enableMenuItem(
		void * menu, unsigned int index, MenuState state ) = 0;
	virtual void checkMenuItem(
		void * menu, unsigned int index, MenuCheckState state ) = 0;

	virtual void setSeperator( void * menu, unsigned int index ) = 0;

	virtual void updateText(
		void * hMenu, unsigned index, const BW::wstring & newName ) = 0;

	virtual void * setSubMenu( void * hMenu, unsigned index ) = 0;

};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif //I_MENU_HELPER_H
