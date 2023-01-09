#include "pch.hpp"
#include "worldeditor/gui/pages/page_terrain_ruler.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/terrain/terrain_ruler_tool_view.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "guimanager/gui_manager.hpp"

BW_BEGIN_NAMESPACE

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageTerrainRuler::contentID = L"PageTerrainRuler";

#define PERIODIC_TIMER_ID 101234

PageTerrainRuler::PageTerrainRuler()
	: PageTerrainBase(PageTerrainRuler::IDD),
	pageReady_(false),
	worldScale_(1.f)
{
}

PageTerrainRuler::~PageTerrainRuler()
{
}

void PageTerrainRuler::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TERRAIN_RULER_DISTANCE, ctlEditDistance);
	DDX_Control(pDX, IDC_TERRAIN_RULER_HORZ_DISTANCE, ctlEditHorzDistance);
	DDX_Control(pDX, IDC_TERRAIN_RULER_VERT_DISTANCE, ctlEditVertDistance);
	DDX_Control(pDX, IDC_TERRAIN_RULER_WIDTH_SLIDER, ctlRulerWidthSlider);
	DDX_Control(pDX, IDC_TERRAIN_RULER_VERTICAL_MODE, ctlCheckBoxVerticalMode);
}

BEGIN_MESSAGE_MAP(PageTerrainRuler, PageTerrainBase)
	ON_WM_TIMER()
	ON_MESSAGE(WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_BN_CLICKED(IDC_TERRAIN_RULER_VERTICAL_MODE, &PageTerrainRuler::OnBnClickedCheckBoxVerticalMode)
END_MESSAGE_MAP()


LRESULT PageTerrainRuler::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	const wchar_t *activePageId = (const wchar_t *)(wParam);
	if (activePageId == getContentID())
	{
		if (WorldEditorApp::instance().pythonAdapter())
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgcTerrain", "TerrainRuler");
	}
	return 0;
}

BOOL PageTerrainRuler::OnInitDialog()
{
	BW_GUARD;

	PageTerrainBase::OnInitDialog();

	if (!pageReady_)
	{
		pageReady_ = true;
		SetTimer(PERIODIC_TIMER_ID, 100, NULL);
	}

	ctlRulerWidthSlider.SetRangeMin(10); // in cm
	ctlRulerWidthSlider.SetRangeMax(2000);

	TerrainRulerToolView *toolView = TerrainRulerToolView::instance();
	if (toolView == NULL)
		ctlRulerWidthSlider.SetPos(200);
	else
	{
		ctlRulerWidthSlider.SetPos((int)(toolView->width() * 100));
		toolView->verticalMode(false);
	}

	worldScale_ = Options::getOptionFloat( "terrain/ruler/worldScale", 1.0 );
	INIT_AUTO_TOOLTIP();
	return TRUE;
}

void PageTerrainRuler::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != PERIODIC_TIMER_ID)
		return;

	TerrainRulerToolView *toolView = TerrainRulerToolView::instance();
	if (toolView == NULL)
		return;

	Vector3 startPos = toolView->startPos();
	Vector3 endPos = toolView->endPos();
	float dist = (endPos - startPos).length();
	float distVert = endPos.y - startPos.y;
	startPos.y = endPos.y;
	float distHorz = (endPos - startPos).length();

	CString str;
	str.Format(L"%.2f", dist * worldScale_);
	ctlEditDistance.SetWindowText(str.GetString());

	if (toolView->verticalMode())
	{
		ctlEditHorzDistance.SetWindowText( L"0" );
		ctlEditVertDistance.SetWindowText( str.GetString() );
	}
	else
	{
		str.Format(L"%.2f", distHorz * worldScale_);
		ctlEditHorzDistance.SetWindowText(str.GetString());
		str.Format(L"%.2f", distVert * worldScale_);
		ctlEditVertDistance.SetWindowText(str.GetString());
	}
}

afx_msg LRESULT PageTerrainRuler::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!IsWindowVisible())
		return 0;

	TerrainRulerToolView *toolView = TerrainRulerToolView::instance();
	if (toolView == NULL)
		return 0;

	int widthInCm = ctlRulerWidthSlider.GetPos();
	toolView->width((float)widthInCm * 0.01f);

	return 0;
}

void PageTerrainRuler::OnBnClickedCheckBoxVerticalMode()
{
	BW_GUARD;

	if (!IsWindowVisible())
		return;

	TerrainRulerToolView *toolView = TerrainRulerToolView::instance();
	if (toolView != NULL)
		toolView->verticalMode(ctlCheckBoxVerticalMode.GetCheck() == BST_CHECKED);
}
BW_END_NAMESPACE

