#include "../../cursor/cursor.hpp"

#include "cstdmf/cstdmf_windows.hpp"

BW_BEGIN_NAMESPACE

//==============================================================================
/*static */void Cursor::setPosition( int x, int y )
{
	::SetCursorPos( x, y );
	emitCursorPosChanged( x, y );
}

BW_END_NAMESPACE

