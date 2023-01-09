#include "pch.hpp"
#include "page_fences.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/misc/fences_tool_view.hpp"
#include "common/user_messages.hpp"
#include "guimanager/gui_manager.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "ual/ual_manager.hpp"
#include "ual/ual_dialog.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "tools/common/string_utils.hpp"

BW_BEGIN_NAMESPACE

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageFences::contentID = L"PageFences";

#define PERIODIC_TIMER_ID 101234

PageFences::PageFences()
	: CFormView(PageFences::IDD)
	, pageReady_(false)
{
}

PageFences::~PageFences()
{
}

void PageFences::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_FENCE_MODELNAME, ctlEditFenceModelName);
	DDX_Control(pDX, IDC_CHECKBOX_FENCE_SELECTWHOLE, ctlCbSelectWholeFences);
	DDX_Control(pDX, IDC_CHECKBOX_FENCE_ALIGNNORMAL, ctlCbAlignToTerrainNormal);
	DDX_Control(pDX, IDC_EDIT_FENCE_STEP, ctlEditFenceStep);
}

BEGIN_MESSAGE_MAP(PageFences, CFormView)
	ON_MESSAGE(WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_BN_CLICKED(IDC_BUTTON_FENCE_FILLMODELNAME, &PageFences::OnBnClickedSelectionToFenceModelName)
	ON_BN_CLICKED(IDC_BUTTON_FENCE_STARTNEW, &PageFences::OnBnClickedStartNewSequence)
	ON_BN_CLICKED(IDC_CHECKBOX_FENCE_SELECTWHOLE, &PageFences::OnCbClickedSelectWholeFences)
	ON_BN_CLICKED(IDC_CHECKBOX_FENCE_ALIGNNORMAL, &PageFences::OnCbClickedAlignToTerrainNormal)
	ON_EN_CHANGE(IDC_EDIT_FENCE_STEP, &PageFences::OnEditChangedFenceStep)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

/**
* Initializes the FencesToolView to start/continue fence from selected model.
*/
LRESULT PageFences::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	const wchar_t *activePageId = (const wchar_t *)(wParam);
	if (activePageId != getContentID())
		return 0;

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView == NULL)
		return 0;

	toolView->breakSequence();

	const BW::vector<ChunkItemPtr> &selection = WorldManager::instance().selectedItems();
	if (selection.size() > 0)
	{
		ChunkItem *pFirstItem = selection[0].get();
		if (pFirstItem != NULL && pFirstItem->edIsSelected())
		{
			const type_info& ChunkModelTypeId = typeid(EditorChunkModel);
			const type_info& itemTypeId = typeid(*pFirstItem);
			if (itemTypeId == ChunkModelTypeId)
			{
				BW::string modelName = pFirstItem->edFilePath();
				if (modelName.size() > 0)
				{
					BW::wstring modelNameW = bw_utf8tow(modelName).c_str();
					ctlEditFenceModelName.SetWindowText(modelNameW.c_str());

					toolView->setModelName(modelName);
					toolView->continueSequenceFrom((EditorChunkModel*)pFirstItem);
				}
			}
		}
	}

	if (ctlEditFenceModelName.GetWindowTextLength() == 0)
		OnBnClickedSelectionToFenceModelName();

	if (WorldEditorApp::instance().pythonAdapter())
		WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgcTerrain", "Fences");

	ctlCbSelectWholeFences.SetCheck(EditorChunkModel::s_autoSelectFenceSections ? BST_CHECKED : BST_UNCHECKED);

	return 0;
}

afx_msg LRESULT PageFences::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!IsWindowVisible())
		return 0;

	// this captures all the scroll events for the page
	if ( !pageReady_ )
		InitPage();

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView == NULL)
		return 0;

	return 0;
}

void PageFences::OnBnClickedSelectionToFenceModelName()
{
	BW_GUARD;

	UalDialog *dlg = UalManager::instance().getActiveDialog();
	if (dlg != NULL)
	{
		BW::vector<BW::wstring> selAssetPaths = dlg->getSelectedAssetPaths();
		if (selAssetPaths.size() > 0)
		{
			BW::wstring filename = selAssetPaths[0];
			BW::wstring extension = BWResource::getExtensionW( filename );
			StringUtils::toLowerCaseT( extension );

			if (extension != L"model")
			{
				return;
			}

			ctlEditFenceModelName.SetWindowText( filename.c_str() );

			FencesToolView *toolView = WorldManager::instance().getFenceToolView();
			if (toolView != NULL)
			{
				toolView->setModelName( bw_wtoutf8( filename ) );
			}
		}
	}
}

void PageFences::OnBnClickedStartNewSequence()
{
	BW_GUARD;

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView != NULL)
	{
		OnBnClickedSelectionToFenceModelName();
		toolView->breakSequence();

		BW::string modelName = toolView->modelName();
		if (modelName.size() == 0)
		{
			// szFilters is a text string that includes two file name filters:
			// "*.my" for "MyType Files" and "*.*' for "All Files."
			wchar_t szFilters[]=
				L"Model Files (*.model)|*.model";

			// Create an Open dialog; the default file name extension is ".prefab".
			BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
				(	BWFileDialog::FD_OVERWRITEPROMPT |
					BWFileDialog::FD_HIDEREADONLY |
					BWFileDialog::FD_PATHMUSTEXIST );
			BWFileDialog fileDlg(true, L"model", L"*.model",
				flags, szFilters, this);

			// Display the file dialog. When user clicks OK, fileDlg.DoModal() 
			// returns IDOK.
			if(fileDlg.showDialog())
			{
				BW::wstring pathName = fileDlg.getFileName();
				if (pathName.length() != 0)
				{
					//BW::wstring modelNameW = bw_utf8tow( pathName.c_str() ).c_str();
					ctlEditFenceModelName.SetWindowText( pathName.c_str() );

					FencesToolView *toolView = WorldManager::instance().getFenceToolView();
					if (toolView != NULL)
						toolView->setModelName(bw_wtoutf8(pathName));
				}		
			}
		}
	}
}

void PageFences::OnCbClickedSelectWholeFences()
{
	BW_GUARD;

	EditorChunkModel::s_autoSelectFenceSections 
		= (ctlCbSelectWholeFences.GetCheck() == BST_CHECKED);
}

void PageFences::OnCbClickedAlignToTerrainNormal()
{
	BW_GUARD;

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView != NULL)
		toolView->setAlignToTerrainNormal(ctlCbAlignToTerrainNormal.GetCheck() == BST_CHECKED);
}

void PageFences::OnEditChangedFenceStep()
{
	BW_GUARD;

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView != NULL && ctlEditFenceStep.GetWindowTextLength() > 0)
	{
		CString str;
		ctlEditFenceStep.GetWindowText(str);
		float step = 0;
		if (sscanf_s(bw_wtoutf8(str.GetString()).c_str(), "%f", &step) == 1)
			toolView->setFenceStep(step);
	}
}

void PageFences::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	// this captures all the scroll events for the page
	if ( !pageReady_ )
		InitPage();

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageFences::InitPage()
{
	BW_GUARD;

	INIT_AUTO_TOOLTIP();

	ctlEditFenceStep.SetWindowText(L"0");

	pageReady_ = true;
}
BW_END_NAMESPACE
