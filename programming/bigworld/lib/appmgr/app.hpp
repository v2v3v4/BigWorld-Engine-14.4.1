#ifndef APP_HPP
#define APP_HPP

#include "cstdmf/cstdmf_windows.hpp"

#include "input_manager.hpp"
#include "scripted_module.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}
/**
 *	This is a class that is used to represent the entire application.
 */
class App
{
public:
	App();
	~App();

	bool init( HINSTANCE hInstance, HWND hWndApp, HWND hwndGraphics,
		 IMainFrame * mainFrame,
		 bool ( *userInit )( HINSTANCE, HWND, HWND ) );
	void fini( void (*userFini)() = NULL );
	void updateFrame( bool tick = true );
	void consumeInput();

	HWND hwndApp();

	// Windows messages
	void resizeWindow( );
	void handleSetFocus( bool focusState );

	// Application Tasks
	float	calculateFrameTime();
    void    pause(bool paused);
    bool    isPaused() const;

	// State changes for subclasses to override
	virtual void	presenting( bool isPresenting ) {};

private:
	// Properties
	HWND				hWndApp_;

	// Timing
	uint64				lastTime_;
    bool                paused_;

	// Input
	InputManager		inputHandler_;
	bool				bInputFocusLastFrame_;

	// General
	float				maxTimeDelta_;
	float				timeScale_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
	#include "app.ipp"
#endif

#endif // APP_HPP
