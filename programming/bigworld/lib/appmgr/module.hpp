#ifndef MODULE_HPP
#define MODULE_HPP

#include "input/input.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class App;
class IMainFrame;

/**
 *	Module is the interface for all application modules.
 */
class Module : public InputHandler, public ReferenceCount
{
public:
	Module() {};

	virtual ~Module();

	// This is called when the module manager creates a module.
	virtual bool init( DataSectionPtr pSection );

	// These are called when a module enters or leaves the stack.
	virtual void onStart();
	virtual int  onStop();

	// These are called when a module stays, or stayed on the stack.
	virtual void onPause();
	virtual void onResume( int exitCode );

	/**
	 *	This pure virtual method is called each frame when this module is the
	 *	active one.
	 *
	 *	@dTime	The time taken for the last frame.
	 */
	virtual void updateFrame( float dTime ) = 0;
	virtual void updateAnimations() = 0;
	virtual void render( float dTime ) = 0;

	virtual void setApp( App * app ) = 0;
	virtual void setMainFrame( IMainFrame * mainFrame ) = 0;
};


/**
 *	This class extends module with a simple framework for rendering. It handles
 *	the beginScene and endScene calls and also displays the console manager.
 */
class FrameworkModule : public Module
{
protected:
	// Override from Module
	virtual void updateFrame( float dTime );

	// Extended methods
	virtual bool updateState( float dTime );

	virtual void setApp( App * app ) = 0;
	virtual void setMainFrame( IMainFrame * mainFrame ) = 0;

private:
	uint32	backgroundColour_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "module.ipp"
#endif

#endif // MODULE_HPP
