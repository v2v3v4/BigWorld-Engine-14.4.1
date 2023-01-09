#include "../../cursor/wait_cursor.hpp"

#include <afxwin.h>

//==============================================================================
WaitCursor::WaitCursor()
{
	AfxGetApp()->BeginWaitCursor();
}

//==============================================================================
WaitCursor::~WaitCursor()
{
	AfxGetApp()->EndWaitCursor();
}