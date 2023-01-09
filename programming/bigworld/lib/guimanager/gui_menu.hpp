#ifndef GUI_MENU_HPP__
#define GUI_MENU_HPP__

#include "gui_manager.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE
class IMenuHelper;

class Menu : public Subscriber
{
protected:
	void * hmenu_;
	virtual void changed( void * hmenu, ItemPtr item );
	virtual void updateAction( void * hmenu, int& index, ItemPtr item );
	virtual void updateToggle( void * hmenu, int& index, ItemPtr item );
	virtual void updateExpandedChoice( void * hmenu, int& index, ItemPtr item );
	virtual void updateChoice( void * hmenu, int& index, ItemPtr item );
	virtual void updateGroup( void * hmenu, int& index, ItemPtr item );
	BW::wstring buildMenuText( ItemPtr item );
public:
	Menu( const BW::string& root, IMenuHelper * menuHelper );
	virtual ~Menu();
	virtual void changed( ItemPtr item );

private:
	IMenuHelper * helper_;

	void updateItem(
		void * hMenu, int & index, ItemPtr item );
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_MENU_HPP__
