#include "pch.hpp"
#include "delay_redraw.hpp"

BW_BEGIN_NAMESPACE

BW::map< CWnd*, int > DelayRedraw::s_counter_;

DelayRedraw::DelayRedraw( CWnd* wnd ):
	wnd_(wnd)
{
	BW_GUARD;

	if (s_counter_[wnd_]++ == 0)
	{
		wnd_->SetRedraw( FALSE );
	}
}

DelayRedraw::~DelayRedraw()
{
	BW_GUARD;

	if (--s_counter_[wnd_] <= 0)
	{
		wnd_->SetRedraw( TRUE );
		wnd_->Invalidate(); 
	}
}
BW_END_NAMESPACE

