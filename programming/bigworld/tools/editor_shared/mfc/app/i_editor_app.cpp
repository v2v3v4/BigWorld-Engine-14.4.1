#include "../../app/i_editor_app.hpp"

#include <afxwin.h>

BW_BEGIN_NAMESPACE

//==============================================================================
bool IEditorApp::isMinimized()
{
	return AfxGetMainWnd()->IsIconic() == TRUE;
}

BW_END_NAMESPACE