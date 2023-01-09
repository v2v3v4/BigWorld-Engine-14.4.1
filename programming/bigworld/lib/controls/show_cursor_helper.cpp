#include "pch.hpp"
#include "show_cursor_helper.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

bool ShowCursorHelper::visible()
{
	BW_GUARD;

	ShowCursor( TRUE );
	return ShowCursor( FALSE ) >= 0;
}


ShowCursorHelper::ShowCursorHelper( bool show ) :
	show_( show ),
	showCursorCount_( 0 )
{
	BW_GUARD;

	if ( show_ == visible() )
		return;

	// Show cursor will be called at least once from now, so pre-incrementing.
	showCursorCount_++;

	if ( show_ )
		while ( ShowCursor( TRUE ) < 0 ) showCursorCount_++;
	else
		while ( ShowCursor( FALSE ) >= 0 ) showCursorCount_++;
}

void ShowCursorHelper::dontRestore()
{
	showCursorCount_ = 0;
}

void ShowCursorHelper::restoreNow()
{
	BW_GUARD;

	// restore ShowCursor count
	if ( show_ )
		while ( showCursorCount_-- > 0 ) ShowCursor( FALSE );
	else
		while ( showCursorCount_-- > 0 ) ShowCursor( TRUE );
	showCursorCount_ = 0;
}


ShowCursorHelper::~ShowCursorHelper()
{
	BW_GUARD;

	restoreNow();
}

BW_END_NAMESPACE
