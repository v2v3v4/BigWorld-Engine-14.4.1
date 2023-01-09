
#include "pch.hpp"
#include "moo_test_window.hpp"
#include "moo/init.hpp"
#include "moo/render_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


namespace
{
	///Window function for test windows.
	LRESULT CALLBACK testWindowWndProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		return DefWindowProc( hWnd, msg, wParam, lParam );
	}

	///Window class for test windows.
	WNDCLASS TEST_WINDOW_CLASS = {
		0, & testWindowWndProc,
		0, 0,
		NULL, NULL, NULL, NULL,
		NULL, L"Moo::TestWindow" };
}


BW_BEGIN_NAMESPACE

namespace Moo
{
	bool TestWindow::isAnInstanceCurrentlyInitialised_ = false;


	/**
	 *	Creates the window object but it's not initialised.
	 */
	TestWindow::TestWindow()
		:
		hwnd_( NULL )
	{
	}


	/**
	 *	Destroys the window and cleans up.
	 */
	TestWindow::~TestWindow()
	{
		destroy();
	}


	/**
	 *	@pre This window is not currently initialised and neither is any
	 *		other instance of this class.
	 *	@post Returned true if the window was successfully initialised,
	 *		false if an error occurred. Moo::init() was called.
	 *		On false, this object's state is unchanged and Moo::fini() was
	 *		called.
	 */
	bool TestWindow::init()
	{
		MF_ASSERT( !isAnInstanceCurrentlyInitialised_ );
		MF_ASSERT( hwnd_ == NULL );

		if (!Moo::init())
		{
			ERROR_MSG( "Failed Moo::init in Moo::TestWindow.\n" );
			return false;
		}

		OSVERSIONINFO ose = { sizeof(ose) };
		if (GetVersionEx( &ose ) == 0)
		{
			ERROR_MSG( "Failed GetVersionEx in Moo::TestWindow.\n" );
			Moo::fini();
			return false;
		}

		if (!RegisterClass( &TEST_WINDOW_CLASS ))
		{
			ERROR_MSG( "Couldn't register class for Moo::TestWindow.\n" );
			Moo::fini();
			return false;
		}

		HWND newHwnd = CreateWindow(
			L"Moo::TestWindow", L"Moo::TestWindow",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			100, 100,
			NULL, NULL, NULL, 0 );

		if (newHwnd == NULL)
		{
			ERROR_MSG( "Couldn't create window for Moo::TestWindow.\n" );
			Moo::fini();
			return false;
		}
		else
		{
			const bool isAtLeastVista = (ose.dwMajorVersion > 5);
			const bool forceRef = !isAtLeastVista;

			if ( !Moo::rc().createDevice( newHwnd, 0, 0,
				true, false, Vector2::ZERO, true, forceRef ) )	
			{
				ERROR_MSG( "Couldn't create render device for "
					"Moo::TestWindow.\n" );

				if (!DestroyWindow( newHwnd ))
				{
					ERROR_MSG(
						"Couldn't DestroyWindow for Moo::TestWindow.\n" );
				}
				
				Moo::fini();
				return false;
			}
		}

		hwnd_ = newHwnd;
		isAnInstanceCurrentlyInitialised_ = true;
		return true;
	}

	
	/**
	 *	@pre True.
	 *	@post If the window was initialised, it is now destroyed with any
	 *		associated resources released and Moo::fini() called.
	 */
	void TestWindow::destroy()
	{
		if ( hwnd_ == NULL )
		{
			return;
		}

		MF_ASSERT( isAnInstanceCurrentlyInitialised_ );

		Moo::rc().releaseDevice();
		if (!DestroyWindow( hwnd_ ))
		{
			ERROR_MSG( "Couldn't DestroyWindow for Moo::TestWindow.\n" );
		}
		hwnd_ = NULL;

		if ( !UnregisterClass( TEST_WINDOW_CLASS.lpszClassName, NULL ) )
		{
			ERROR_MSG( "Couldn't UnregisterClass for Moo::TestWindow.\n" );
		}

		Moo::fini();

		isAnInstanceCurrentlyInitialised_ = false;
	};


	/**
	 *	@return The handle for the window (or NULL if not initialised).
	 */
	HWND TestWindow::hwnd() const
	{
		return hwnd_;
	}
} // namespace Moo

BW_END_NAMESPACE
