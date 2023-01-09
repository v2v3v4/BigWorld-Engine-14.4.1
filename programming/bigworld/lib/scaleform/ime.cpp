#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "ime.hpp"

#if SCALEFORM_IME

#include "manager.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/bw_util.hpp"
#include <GFx_IMEManagerWin32.h>

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	int IME_token = 0;
	PyMovieView * IME::s_pFocussed = NULL;

	bool IME::init( const BW::string& imeMovie )
	{
		BW_GUARD;
		GFx::IME::GFxIMEManagerWin32 *ime = new GFx::IME::GFxIMEManagerWin32(Moo::rc().windowHandle());
		ime->Init(Manager::instance().pLogger());
		ime->SetIMEMoviePath( imeMovie.c_str() );
		Manager::instance().pLoader()->SetIMEManager(ime);
		DEBUG_MSG( "ime manager ref count %d\n", ime->GetRefCount() );
		//ime->Release();
		//lifetime of ime object now controlled by loader
		return true;
	}


	bool IME::handleIMMMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		BW_GUARD;

		GFx::IMEWin32Event ev( GFx::IMEWin32Event::IME_PreProcessKeyboard, (UPInt)hWnd, msg, wParam, lParam);
		if ( s_pFocussed )
		{
			int32 ret = s_pFocussed->pMovieView()->HandleEvent(ev);
			return !!(ret & GFx::Movie::HE_NoDefaultAction);
		}
		return false;
	}


	bool IME::handleIMEMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		BW_GUARD;

		GFx::IMEWin32Event ev(GFx::IMEWin32Event::IME_Default, (UPInt)hWnd, msg, wParam, lParam);
		// Do not pass IME events handled by GFx to DefWindowProc.
		if ( s_pFocussed )
		{
			int32 ret = s_pFocussed->pMovieView()->HandleEvent(ev);
			return !!(ret & GFx::Movie::HE_NoDefaultAction);
		}
		return false;
	}

	void IME::setFocussedMovie(class PyMovieView *m)
	{
		if (m != s_pFocussed)
		{
			if (s_pFocussed != NULL)
				s_pFocussed->pMovieView()->HandleEvent(GFx::Event::KillFocus);

			s_pFocussed = m;

			if (s_pFocussed != NULL)
				s_pFocussed->pMovieView()->HandleEvent(GFx::Event::SetFocus);
		}
	}

	void IME::onDeleteMovieView(class PyMovieView *m)
	{
		if (s_pFocussed == m)
		{
			s_pFocussed->pMovieView()->HandleEvent(GFx::Event::KillFocus);
			s_pFocussed = NULL;
		}
	}

}	//namespace ScaleformBW

BW_END_NAMESPACE

#endif // SCALEFORM_IME
#endif //#if SCALEFORM_SUPPORT