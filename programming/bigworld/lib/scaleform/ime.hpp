#ifndef SCALEFORM_IME_HPP
#define SCALEFORM_IME_HPP

#include "config.hpp"

#if SCALEFORM_IME

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
/**
 *	This class contains some utility methods enabling the use of Scaleform's
 *	in-game IME support.
 */
class IME
{
	static class PyMovieView *s_pFocussed;

public:
	static bool init( const BW::string& imeMovie );
	static bool handleIMMMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static bool handleIMEMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	static PyMovieView *pFocussedMovie()
	{
		return s_pFocussed;
	}

	static void setFocussedMovie(class PyMovieView *m);
	static void onDeleteMovieView(class PyMovieView *m);
};

}	// namespace ScaleformBW

BW_END_NAMESPACE

#endif

#endif