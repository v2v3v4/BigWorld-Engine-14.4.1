#ifndef GUI_TAB_CONTENT_HPP
#define GUI_TAB_CONTENT_HPP

#include "guitabs/guitabs_content.hpp"

BW_BEGIN_NAMESPACE

class IMainFrame;

class GuiTabContent
	: public GUITABS::Content
{
public:
	GuiTabContent()
		: mainFrame_( NULL )
	{
	}

	void setMainFrame( IMainFrame * mainFrame )
	{
		mainFrame_ = mainFrame;
	}

protected:
	IMainFrame * mainFrame_;
};

BW_END_NAMESPACE

#endif //GUI_TAB_CONTENT_HPP