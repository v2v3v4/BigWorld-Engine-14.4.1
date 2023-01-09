#ifndef DEV_MENU_HPP
#define DEV_MENU_HPP

#include <iostream>
#include "module.hpp"
#include "romp/font.hpp"

BW_BEGIN_NAMESPACE

class SimpleGUIComponent;
class TextGUIComponent;
class Button;

class DevMenu : public FrameworkModule
{
public:
	DevMenu();
	~DevMenu();

	virtual void onStart();
	virtual int	 onStop();

	virtual void onPause();
	virtual void onResume( int exitCode );

	virtual void updateAnimations();
	virtual void render( float dTime );

	// Input handlers
	virtual bool handleKeyEvent( const KeyEvent & /*event*/ );
	virtual bool handleMouseEvent( const MouseEvent & /*event*/ );
	virtual void setApp( App * app ) {};
	virtual void setMainFrame( IMainFrame * mainFrame ) {};

private:
	DevMenu(const DevMenu&);
	DevMenu& operator=(const DevMenu&);

	typedef BW::vector< BW::string > Modules;
	Modules	modules_;

	SimpleGUIComponent* watermark_;

	typedef BW::vector< Button* > Buttons;
	Buttons	buttons_;

	typedef BW::vector< TextGUIComponent* > MenuItems;
	MenuItems menuItems_;

	friend std::ostream& operator<<(std::ostream&, const DevMenu&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "dev_menu.ipp"
#endif

#endif
/*dev_menu.hpp*/
