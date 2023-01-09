#include "pch.hpp"
#include "worldeditor/gui/pages/page_project.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/project/project_module.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "guimanager/gui_manager.hpp"

BW_BEGIN_NAMESPACE

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageProject::contentID = L"PageProject";

namespace
{
	enum OverlayModeIndex
	{
		OVERLAY_MODE_NONE = 0,
		OVERLAY_MODE_LOCKS,
		OVERLAY_MODE_LOD
	};
}


PageProject::PageProject()
	: CFormView(PageProject::IDD),
	pageReady_( false )
{
}

PageProject::~PageProject()
{
}

void PageProject::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROJECT_MAP_ALPHA_SLIDER, blendSlider_);
	DDX_Control(pDX, IDC_PROJECT_SELECTION_LOCK, selectionLock_);
	DDX_Control(pDX, IDC_PROJECT_COMMIT_MESSAGE, commitMessage_);
	DDX_Control(pDX, IDC_PROJECT_COMMIT_KEEPLOCKS, commitKeepLocks_);
	DDX_Control(pDX, IDC_PROJECT_COMMIT_ALL, commitAll_);
	DDX_Control(pDX, IDC_PROJECT_DISCARD_KEEPLOCKS, discardKeepLocks_);
	DDX_Control(pDX, IDC_PROJECT_DISCARD_ALL, discardAll_);
	DDX_Control(pDX, IDC_CALCULATEDMAP, mCalculatedMap);
	DDX_Control(pDX, IDC_PROJECT_UPDATE, update_);
	DDX_Control(pDX, IDC_PROJECT_OVERLAY_MODE, overlayMode_);
}

void PageProject::InitPage()
{
	BW_GUARD;

	INIT_AUTO_TOOLTIP();
	blendSlider_.SetRangeMin(1);
	blendSlider_.SetRangeMax(100);
	blendSlider_.SetPageSize(0);

	commitMessage_.SetLimitText(1000);
	commitKeepLocks_.SetCheck(BST_CHECKED);
	discardKeepLocks_.SetCheck(BST_UNCHECKED);

	overlayMode_.Clear();
	overlayMode_.AddString(CString((LPCSTR)IDC_PROJECT_NULL_OVERLAY));
	overlayMode_.AddString(CString((LPCSTR)IDC_PROJECT_LOCKS_OVERLAY));
	overlayMode_.AddString(CString((LPCSTR)IDC_PROJECT_LOD_OVERLAY));

	if (WorldManager::instance().connection().enabled())
	{
		overlayMode_.SetCurSel(OVERLAY_MODE_LOCKS);
	}
	else
	{
		overlayMode_.SetCurSel(OVERLAY_MODE_NONE);
	}
	OnOverlayModeSel();

	OnEnChangeProjectCommitMessage();

	commitMessage_.SetLimitText( 512 );

	pageReady_ = true;
}


BEGIN_MESSAGE_MAP(PageProject, CFormView)
	ON_MESSAGE (WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_PROJECT_SELECTION_LOCK, OnBnClickedProjectSelectionLock)
	ON_BN_CLICKED(IDC_PROJECT_COMMIT_ALL, OnBnClickedProjectCommitAll)
	ON_BN_CLICKED(IDC_PROJECT_DISCARD_ALL, OnBnClickedProjectDiscardAll)
	ON_EN_CHANGE(IDC_PROJECT_COMMIT_MESSAGE, OnEnChangeProjectCommitMessage)
	ON_BN_CLICKED(IDC_PROJECT_UPDATE, &PageProject::OnBnClickedProjectUpdate)
	ON_CBN_SELCHANGE(IDC_PROJECT_OVERLAY_MODE, OnOverlayModeSel)
END_MESSAGE_MAP()


LRESULT PageProject::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	const wchar_t *activePageId = (const wchar_t *)(wParam);
	if (getContentID() == activePageId)
	{
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", "Project");
		}
	}
	return 0;
}

void PageProject::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	// this captures all the scroll events for the page
	if ( !pageReady_ )
		InitPage();

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->onSliderAdjust("slrProjectMapBlend", 
												blendSlider_.GetPos(), 
												blendSlider_.GetRangeMin(), 
												blendSlider_.GetRangeMax());		
	}

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}


afx_msg LRESULT PageProject::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if ( !pageReady_ )
		InitPage();

	if ( !IsWindowVisible() || !ProjectModule::currentInstance() )
		return 0;

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageProject::OnUpdateControls: PythonAdapter is NULL" );
	WorldEditorApp::instance().pythonAdapter()->sliderUpdate(&blendSlider_, "slrProjectMapBlend");

	if( !WorldManager::instance().connection().connected() )
	{
		if( commitMessage_.IsWindowEnabled() )
		{
			commitMessage_.SetWindowText( Localise(L"WORLDEDITOR/GUI/PAGE_PROJECT/FAILED_TO_CONNECT") );
			commitMessage_.EnableWindow( FALSE );
			selectionLock_.EnableWindow( FALSE );
			commitAll_.EnableWindow( FALSE );
			discardAll_.EnableWindow( FALSE );
		}
	}
	else
	{
		selectionLock_.EnableWindow( ProjectModule::currentInstance()->isReadyToLock() );
		commitAll_.EnableWindow( ProjectModule::currentInstance()->isReadyToCommitOrDiscard());
		discardAll_.EnableWindow( ProjectModule::currentInstance()->isReadyToCommitOrDiscard());
	}
	return 0;
}

void PageProject::OnBnClickedProjectSelectionLock()
{
	BW_GUARD;

	CString commitMessage;
	commitMessage_.GetWindowText(commitMessage);
	if( commitMessage.GetLength() == 0 )
	{
		AfxMessageBox( Localise(L"WORLDEDITOR/GUI/PAGE_PROJECT/COMMIT_MESSAGE"), MB_OK );
		commitMessage_.SetFocus();
	}
	else
	{
		MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
			"PageProject::OnBnClickedProjectSelectionLock: PythonAdapter is NULL" );
		WorldEditorApp::instance().pythonAdapter()->projectLock( bw_wtoutf8( commitMessage.GetBuffer() ) );
	}
}

void PageProject::OnBnClickedProjectCommitAll()
{
	BW_GUARD;

	const bool keepLocks = commitKeepLocks_.GetCheck() == BST_CHECKED;

	CString commitMessage;
	commitMessage_.GetWindowText(commitMessage);
	if( commitMessage.GetLength() == 0 )
	{
		AfxMessageBox( Localise(L"WORLDEDITOR/GUI/PAGE_PROJECT/COMMIT_MESSAGE"), MB_OK );
		commitMessage_.SetFocus();
	}
	else
	{
		MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
			"PageProject::OnBnClickedProjectCommitAll: PythonAdapter is NULL" );
		WorldEditorApp::instance().pythonAdapter()->commitChanges( bw_wtoutf8( commitMessage.GetBuffer() ), keepLocks);
		commitMessage_.SetWindowText(L"");
	}
}

void PageProject::OnBnClickedProjectDiscardAll()
{
	BW_GUARD;

	const bool keepLocks = discardKeepLocks_.GetCheck() == BST_CHECKED;

	CString commitMessage;
	commitMessage_.GetWindowText(commitMessage);
	if( commitMessage.GetLength() == 0 )
		commitMessage = "(Discard)";

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageProject::OnBnClickedProjectDiscardAll: PythonAdapter is NULL" );
	WorldEditorApp::instance().pythonAdapter()->discardChanges( bw_wtoutf8( commitMessage.GetBuffer() ), keepLocks );

	commitMessage_.SetWindowText(L"");
}

void PageProject::OnEnChangeProjectCommitMessage()
{
	BW_GUARD;

	CString commitMessage;
	commitMessage_.GetWindowText( commitMessage );
	selectionLock_.EnableWindow( commitMessage.GetLength() && ProjectModule::currentInstance()->isReadyToLock() );
	commitAll_.EnableWindow( commitMessage.GetLength() && ProjectModule::currentInstance()->isReadyToCommitOrDiscard());
	discardAll_.EnableWindow( commitMessage.GetLength() && ProjectModule::currentInstance()->isReadyToCommitOrDiscard());
}

void PageProject::OnBnClickedProjectUpdate()
{
	BW_GUARD;

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageProject::OnBnClickedProjectUpdate: PythonAdapter is NULL" );
	WorldEditorApp::instance().pythonAdapter()->updateSpace();
}

void PageProject::OnOverlayModeSel()
{
	BW_GUARD;

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageProject::OnBnClickedLODOverlay: PythonAdapter is NULL" );

	switch (overlayMode_.GetCurSel())
	{
	case OVERLAY_MODE_NONE:
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actProjectOverlayNone");
		break;

	case OVERLAY_MODE_LOCKS:
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actProjectOverlayLocks");
		break;

	case OVERLAY_MODE_LOD:
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actProjectOverlayLOD");
		break;
	}
}

BW_END_NAMESPACE

