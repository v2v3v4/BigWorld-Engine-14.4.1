#include "pch.hpp"
#include "dev_menu.hpp"
#include "module_manager.hpp"
#include "application_input.hpp"

//library includes
#include "ashes/simple_gui_component.hpp"
#include "ashes/simple_gui.hpp"
#include "cstdmf/debug.hpp"
#include "moo/render_context.hpp"
#include "resmgr/bwresource.hpp"
#include "math/colour.hpp"
#include "romp/xconsole.hpp"

DECLARE_DEBUG_COMPONENT2( "Module", 0 );

#ifndef CODE_INLINE
#include "dev_menu.ipp"
#endif

BW_BEGIN_NAMESPACE

typedef ModuleManager ModuleFactory;
IMPLEMENT_CREATOR(DevMenu, Module)
int DevMenu_token;

/**
 *	Constructor.
 */
DevMenu::DevMenu()
:watermark_( NULL )
{
	DataSectionPtr spSection =
		BWResource::instance().openSection( "resources/data/modules.xml/modules");

	if ( spSection )
	{
		BW::vector< DataSectionPtr > spModules;
		spSection->openSections( "module", spModules );

		BW::vector< DataSectionPtr >::iterator it = spModules.begin();
		BW::vector< DataSectionPtr >::iterator end = spModules.end();

		while ( it != end )
		{
			DataSectionPtr pModuleSection = *it++;

			modules_.push_back( pModuleSection->readString( "name", "Error - no module name" ) );
		}
	}
	else
	{
		modules_.push_back( "AI Prototype" );
		modules_.push_back( "Display Prototype" );
	}
}


/**
 *	Destructor.
 */
DevMenu::~DevMenu()
{
	if ( watermark_ )
	{
		SimpleGUI::instance().removeSimpleComponent( *watermark_ );
		watermark_->decRef();
		watermark_ = NULL;
	}
}


/**
 * This method is called when the DevMenu module
 * is pushed onto the module stack.
 */
void DevMenu::onStart()
{
	this->onResume(0);
}


/**
 * This method is called when the DevMenu module leaves
 * the module stack.
 *
 *	@see Module::onStop
 */
int DevMenu::onStop()
{
	this->onPause();

	return 0;
}


/**
 * This method is called when another module usurps our position as
 * the currently active module.  Our duty is to restore global
 * structures to their previous state.
 */
void DevMenu::onPause()
{
	SimpleGUI::instance().removeSimpleComponent( *watermark_ );

	watermark_->decRef();
	watermark_ = NULL;
}


/**
 * This method is called after an onStart, and when this module
 * is about to run again.  We ensure the global application state
 * is ready for our use.
 *
 *	@param exitCode		The exitCode of the module that has just left the stack.
 */
void DevMenu::onResume( int exitCode )
{
	watermark_ = new SimpleGUIComponent( "maps/gui/watermark.tga" );
	watermark_->position( Vector3( 0.f, 0.f, 1.f ) );
	watermark_->anchor( SimpleGUIComponent::ANCHOR_H_CENTER,
		SimpleGUIComponent::ANCHOR_V_CENTER );
	watermark_->width( 2.f );
	watermark_->height( 2.f );
	watermark_->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
	watermark_->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
	watermark_->colour( 0xffffffff );
	watermark_->materialFX( SimpleGUIComponent::FX_BLEND );

	SimpleGUI::instance().addSimpleComponent( *watermark_ );
}


/**
 *	Updates animated objects in this module.
 */
void DevMenu::updateAnimations()
{
	BW_GUARD;
}


/**
 *	This method draws the development menu.
 *
 *	@param dtime	The change in time since the last rendering frame.
 */
void DevMenu::render( float dTime )
{
	BW_GUARD;
	Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		Colour::getUint32( 0, 105, 64 ), 1, 0 );

	SimpleGUI::instance().update( dTime );
	SimpleGUI::instance().draw();

	static XConsole console;

	console.setCursor( 0, 0 );
	console.print( "Welcome to MF Rugby os v1.0" );
	console.setCursor( 0, 2 );

	Modules::iterator it = modules_.begin();
	Modules::iterator end = modules_.end();

	char buf[256];
	int keyNumber = 1;
	while ( it != end )
	{
		const BW::string & name = *it++;

		bw_snprintf( buf, sizeof(buf), "%d) %s\n\0", keyNumber, name.c_str() );
		console.print( buf );

		keyNumber++;
	}

	bw_snprintf( buf, sizeof(buf), "%d) Quit\n\0", keyNumber );
	console.print( buf );
	console.draw( 0.1f );
}


/**
 *	This method overrides the InputHandler method to handle key events. These
 *	events include when mouse and joystick buttons are pressed, as well as when
 *	the keyboard keys are pressed or released.
 *
 *	@param event	The current key event.
 *
 *	@return			True if the event was handled, false otherwise.
 */
bool DevMenu::handleKeyEvent( const KeyEvent & event )
{
	bool handled = false;

	handled = ApplicationInput::handleKeyEvent( event );
	if ( !handled )
	{
		if ( event.isKeyDown() )
		{
			int keyCode = (int)event.key();

			if ( ( keyCode >= KeyCode::KEY_1 ) && ( keyCode <= KeyCode::KEY_9 ) )
			{
				int idx = keyCode - KeyCode::KEY_1;

				if ( InputDevices::isShiftDown() )
					idx += 10;

				if ( InputDevices::isCtrlDown() )
					idx += 20;

				handled = true;

				if ( idx < (int) modules_.size() )
				{
					ModuleManager::instance().push( modules_[idx] );
				}
				else if ( idx == modules_.size() )
				{
					ModuleManager::instance().pop();
				}
				else
				{
					handled = false;
				}
			}

			if ( event.key() == KeyCode::KEY_ESCAPE )
			{
				ModuleManager::instance().pop();
				handled = true;
			}
		}
	}

	return handled;
}


/**
 *	This method handles mouse events for the development menu. It always returns
 *	false, to indicate that we do not care about mouse events.
 */
bool DevMenu::handleMouseEvent( const MouseEvent & /*event*/ )
{
	return false;
}


std::ostream& operator<<(std::ostream& o, const DevMenu& t)
{
	o << "DevMenu\n";
	return o;
}

BW_END_NAMESPACE

/*dev_menu.cpp*/
