#include "pch.hpp"

#include "tools/common/python_adapter.hpp"

#include "main_frm.h"
#include "me_shell.hpp"
#include "me_app.hpp"
#include "main_frm.h"

#include "tools/common/utilities.hpp"

#include "moo/graphics_settings.hpp"
#include "romp/flora.hpp"
#include "romp/time_of_day.hpp"

#include "appmgr/options.hpp"
#include "resmgr/bwresource.hpp"

#include "guimanager/gui_manager.hpp"

#include "common/user_messages.hpp"
#include "common/string_utils.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "ual/ual_manager.hpp"
#include "controls/user_messages.hpp"

#include "controls/slider.hpp"

#include "page_display.hpp"
#include "tools/modeleditor_core/App/me_python_adapter.hpp"
#include "tools/modeleditor_core/GUI/panel_manager.hpp"
#include "tools/modeleditor_core/Models/mutant.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// PageDisplay

struct PageDisplayImpl: public SafeReferenceCount
{
	bool inited;
	bool ready;
	
	int last_shadow;
	int last_bkg;
	int last_flora;
	BW::string last_time;

	Moo::GraphicsSetting::GraphicsSettingPtr floraSettings;
	
	CWnd generalBox;
	CButton showAxes;
	CButton checkForSparkles;
	CButton enableFog;
	CButton enableHDR;
	CButton enableSSAO;

	CWnd modelBox;
	CButton showModel;
	CButton showWireframe;
	CButton showSkeleton;
	CComboBox shadowing;
	CButton showBsp;
	CButton showBoundingBox;
	CButton showPortals;
	CButton showNormals;
	CButton showBinormals;
	CButton showCustomHull;
	CButton showHardPoints;

	CButton groundModel;
	CButton centreModel;

	CButton editorProxy;

	controls::Slider normalsLength;

	CWnd bkgBox;
	CComboBox bkg;
	CComboBox flora;
	CButton bkgColour;
	CButton floorTexture;

	CWnd timeOfDayBox;
	controls::Slider timeOfDaySlider;
	CEdit timeOfDayEdit;

	unsigned numFloraOptions;

};

//ID string required for the tearoff tab manager
const BW::wstring PageDisplay::contentID = L"PageDisplayID";

IMPLEMENT_DYNCREATE(PageDisplay, CFormView)

PageDisplay::PageDisplay()
	: CFormView(PageDisplay::IDD)
{
	BW_GUARD;

	pImpl_ = new PageDisplayImpl;

	pImpl_->inited = false;
	pImpl_->ready = false;

	pImpl_->last_shadow = -1;
	pImpl_->last_bkg = -1;
	pImpl_->last_flora = -1;
	pImpl_->last_time = "";

	pImpl_->floraSettings = Moo::GraphicsSetting::getFromLabel(
		FloraSettings::getFloraSettingId() );
	pImpl_->numFloraOptions = 0;
}

PageDisplay::~PageDisplay()
{}

void PageDisplay::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	CFormView::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DISPLAY_GENERAL_BOX, pImpl_->generalBox );
	DDX_Control(pDX, IDC_SHOW_AXES, pImpl_->showAxes );
	DDX_Control(pDX, IDC_CHECK_FOR_SPARKLES, pImpl_->checkForSparkles);
	DDX_Control(pDX, IDC_ENABLE_FOG, pImpl_->enableFog);
	DDX_Control(pDX, IDC_ENABLE_HDR, pImpl_->enableHDR);
	DDX_Control(pDX, IDC_ENABLE_SSAO, pImpl_->enableSSAO);

	DDX_Control(pDX, IDC_DISPLAY_MODEL_BOX, pImpl_->modelBox );
	DDX_Control(pDX, IDC_SHOW_MODEL, pImpl_->showModel);
	DDX_Control(pDX, IDC_SHOW_WIREFRAME, pImpl_->showWireframe);
	DDX_Control(pDX, IDC_SHOW_SKELETON, pImpl_->showSkeleton);
	DDX_Control(pDX, IDC_SHADOWING, pImpl_->shadowing);
	DDX_Control(pDX, IDC_SHOW_BSP, pImpl_->showBsp);
	DDX_Control(pDX, IDC_SHOW_BOUNDING_BOX, pImpl_->showBoundingBox);
	DDX_Control(pDX, IDC_SHOW_VERTEX_NORMALS, pImpl_->showNormals);
	DDX_Control(pDX, IDC_SHOW_VERTEX_BINORMALS, pImpl_->showBinormals);
	DDX_Control(pDX, IDC_SHOW_CUSTOM_HULL, pImpl_->showCustomHull);
	DDX_Control(pDX, IDC_SHOW_PORTALS, pImpl_->showPortals);
	DDX_Control(pDX, IDC_SHOW_HARD_POINTS, pImpl_->showHardPoints);
	DDX_Control(pDX, IDC_GROUND_MODEL, pImpl_->groundModel);
	DDX_Control(pDX, IDC_CENTRE_MODEL, pImpl_->centreModel);
	DDX_Control(pDX, IDC_SHOW_EDITOR_PROXY, pImpl_->editorProxy);

	DDX_Control(pDX, IDC_NORMALS_SIZE_SLIDER, pImpl_->normalsLength);

	DDX_Control(pDX, IDC_DISPLAY_BKG_BOX, pImpl_->bkgBox );
	DDX_Control(pDX, IDC_DISPLAY_BKG, pImpl_->bkg);
	DDX_Control(pDX, IDC_DISPLAY_FLORA, pImpl_->flora);
	DDX_Control(pDX, IDC_DISPLAY_CHOOSE_BKG_COLOUR, pImpl_->bkgColour);
	DDX_Control(pDX, IDC_DISPLAY_CHOOSE_FLOOR_TEXTURE, pImpl_->floorTexture);

	DDX_Control(pDX, IDC_DISPLAY_TIMEOFDAY_BOX, pImpl_->timeOfDayBox );
	DDX_Control(pDX, IDC_TIMEOFDAY_TEXT, pImpl_->timeOfDayEdit);
	DDX_Control(pDX, IDC_TIMEOFDAY_SLIDER, pImpl_->timeOfDaySlider);

	pImpl_->inited = true;
}

BOOL PageDisplay::OnInitDialog()
{
	BW_GUARD;

	if (pImpl_->floraSettings)
	{
		Moo::GraphicsSetting::Options opts = pImpl_->floraSettings->options();

		pImpl_->numFloraOptions = static_cast<uint>(opts.size());
			
		pImpl_->flora.ResetContent();
		for (unsigned i=0; i<pImpl_->numFloraOptions; i++)
		{
			BW::wstring label; 
			
			if (i != pImpl_->numFloraOptions - 1)
			{
				label = bw_utf8tow( opts[i].m_label );
				
				StringUtils::toMixedCaseT( label );

				label += Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/FLORA_DENSITY");
			}
			else
			{
				label = Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/DISABLE_FLORA");
			}

			pImpl_->flora.InsertString( 0, label.c_str() );
		}

		pImpl_->floraSettings->selectOption(
			Options::getOptionInt( "settings/floraDensity", pImpl_->numFloraOptions ));
	}
	else
	{
		//Disable the flora tab if we can't get settings
		pImpl_->flora.InsertString( 0, Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/DISABLE_FLORA") );
		pImpl_->flora.SetCurSel( 0 );
		pImpl_->flora.ModifyStyle(0, WS_DISABLED);
		pImpl_->flora.RedrawWindow();
	}
	
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageDisplay >( &(pImpl_->floorTexture), "bmp", this, &PageDisplay::floorTextureDrop ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageDisplay >( &(pImpl_->floorTexture), "tga", this, &PageDisplay::floorTextureDrop ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageDisplay >( &(pImpl_->floorTexture), "jpg", this, &PageDisplay::floorTextureDrop ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageDisplay >( &(pImpl_->floorTexture), "png", this, &PageDisplay::floorTextureDrop ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageDisplay >( &(pImpl_->floorTexture), "dds", this, &PageDisplay::floorTextureDrop ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageDisplay >( &(pImpl_->floorTexture), "texanim", this, &PageDisplay::floorTextureDrop ) );

	pImpl_->normalsLength.SetRangeMin(0);
	pImpl_->normalsLength.SetRangeMax(100);
	pImpl_->normalsLength.SetPos(50);

	pImpl_->timeOfDaySlider.SetRangeMin(0); 
	pImpl_->timeOfDaySlider.SetRangeMax(60*24 - 1);

	MeShell::instance().timeOfDay()->setTimeOfDayAsString(
		Options::getOptionString( "settings/gameTime",
			MeShell::instance().timeOfDay()->getTimeOfDayAsString() ));	

	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(PageDisplay, CFormView)

	ON_WM_SIZE()

	ON_WM_HSCROLL()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_BN_CLICKED(IDC_SHOW_AXES, OnBnClickedShowAxes)
	ON_BN_CLICKED(IDC_CHECK_FOR_SPARKLES, OnBnClickedCheckForSparkles)
	ON_BN_CLICKED(IDC_ENABLE_FOG, OnBnClickedEnableFog)
	ON_BN_CLICKED(IDC_ENABLE_HDR, OnBnClickedEnableHDR)
	ON_BN_CLICKED(IDC_ENABLE_SSAO, OnBnClickedEnableSSAO)

	ON_BN_CLICKED(IDC_SHOW_MODEL, OnBnClickedShowModel)
	ON_BN_CLICKED(IDC_SHOW_WIREFRAME, OnBnClickedShowWireframe)
	ON_BN_CLICKED(IDC_SHOW_SKELETON, OnBnClickedShowSkeleton)
	ON_CBN_SELCHANGE(IDC_SHADOWING, OnCbnChangeShadowing)
	ON_BN_CLICKED(IDC_SHOW_BSP, OnBnClickedShowBsp)
	ON_BN_CLICKED(IDC_SHOW_BOUNDING_BOX, OnBnClickedShowBoundingBox)
	ON_BN_CLICKED(IDC_SHOW_VERTEX_NORMALS, OnBnClickedShowVertexNormals)
	ON_BN_CLICKED(IDC_SHOW_VERTEX_BINORMALS, OnBnClickedShowVertexBinormals)
	ON_BN_CLICKED(IDC_SHOW_CUSTOM_HULL, OnBnClickedShowCustomHull)
	ON_BN_CLICKED(IDC_SHOW_PORTALS, OnBnClickedShowPortals)
	ON_BN_CLICKED(IDC_SHOW_HARD_POINTS, OnBnClickedShowHardPoints)
	ON_BN_CLICKED(IDC_GROUND_MODEL, OnBnClickedGroundModel)
	ON_BN_CLICKED(IDC_CENTRE_MODEL, OnBnClickedCentreModel)

	ON_BN_CLICKED(IDC_DISPLAY_CHOOSE_BKG_COLOUR, OnBnClickedDisplayChooseBkgColour)
	ON_CBN_SELCHANGE(IDC_DISPLAY_BKG, OnCbnSelchangeDisplayBkg)
	ON_BN_CLICKED(IDC_DISPLAY_CHOOSE_FLOOR_TEXTURE, OnBnClickedDisplayChooseFloorTexture)

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

	ON_CBN_SELCHANGE(IDC_DISPLAY_FLORA, OnCbnSelchangeDisplayFlora)
	ON_BN_CLICKED(IDC_SHOW_EDITOR_PROXY, OnBnClickedShowEditorProxy)
END_MESSAGE_MAP()

afx_msg LRESULT PageDisplay::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	LPTSTR* msg = (LPTSTR*)wParam;
	mainFrame_->setMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageDisplay::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	mainFrame_->setMessageText( L"" );
	return 0;
}

void PageDisplay::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!pImpl_->inited) return;
	
	Utilities::stretchToRight( this, pImpl_->generalBox, cx, 6 );
	Utilities::stretchToRight( this, pImpl_->modelBox, cx, 6 );
	Utilities::stretchToRight( this, pImpl_->bkgBox, cx, 6 );
	Utilities::stretchToRight( this, pImpl_->timeOfDayBox, cx, 6 );

	Utilities::stretchToRight( this, pImpl_->shadowing, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->bkg, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->flora, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->bkgColour, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->floorTexture, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->normalsLength, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->timeOfDaySlider, cx, 12 );
	
	pImpl_->shadowing.RedrawWindow();
	pImpl_->bkg.RedrawWindow();
	pImpl_->flora.RedrawWindow();
	pImpl_->normalsLength.RedrawWindow();
	pImpl_->timeOfDaySlider.RedrawWindow();
	
	CFormView::OnSize( nType, cx, cy );
}

void PageDisplay::updateCheck( CButton& button, const BW::string& actionName )
{
	BW_GUARD;

	int enabled = 0;
	int checked = 0;
	PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptUpdate( actionName, enabled, checked );
	button.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
}

afx_msg LRESULT PageDisplay::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!pImpl_->ready)
	{
		OnInitDialog();

		pImpl_->ready = true;
	}
	
	int enabled = 0;
	int index = 0;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		updateCheck( pImpl_->showAxes,              "actShowAxes" );
		updateCheck( pImpl_->checkForSparkles,      "actCheckForSparkles");
		updateCheck( pImpl_->enableFog,				"actEnableFog");
		updateCheck( pImpl_->enableHDR,				"actEnableHDR");
		updateCheck( pImpl_->enableSSAO,			"actEnableSSAO");

		updateCheck( pImpl_->showModel,             "actShowModel");
		updateCheck( pImpl_->showWireframe,         "actShowWireframe");
		updateCheck( pImpl_->showSkeleton,          "actShowSkeleton");
		updateCheck( pImpl_->showBsp,               "actShowBsp");
		updateCheck( pImpl_->showBoundingBox,       "actShowBoundingBox");
		updateCheck( pImpl_->showPortals,           "actShowPortals");
		updateCheck( pImpl_->showNormals,           "actShowNormals");
		updateCheck( pImpl_->showBinormals,         "actShowBinormals");
		updateCheck( pImpl_->showCustomHull,        "actShowCustomHull");
		updateCheck( pImpl_->showHardPoints,        "actShowHardPoints");
		updateCheck( pImpl_->groundModel,           "actGroundModel");
		updateCheck( pImpl_->centreModel,           "actCentreModel");
		updateCheck( pImpl_->editorProxy,           "actShowEditorProxy");

		bool success;
		success = PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptUpdate( 
			"actGetShadowIndex", enabled, index );
		if (success && (index != pImpl_->last_shadow))
		{
			pImpl_->shadowing.SetCurSel( index );
			OnCbnChangeShadowing();
			pImpl_->last_shadow = index;
		}

		success = PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptUpdate( 
			"actGetBkgIndex", enabled, index );
		if (success && (index != pImpl_->last_bkg))
		{
			pImpl_->bkg.SetCurSel( index );
			OnCbnSelchangeDisplayBkg();
			pImpl_->last_bkg = index;
		}
	}

	if (pImpl_->numFloraOptions)
	{
		index = pImpl_->numFloraOptions - pImpl_->floraSettings->activeOption() - 1;
		if (index != pImpl_->last_flora)
		{
			pImpl_->flora.SetCurSel( index );
			pImpl_->last_flora = index;
		}
	}

	BW::string time = MeShell::instance().timeOfDay()->getTimeOfDayAsString();
	if (time != pImpl_->last_time)
	{
		pImpl_->timeOfDayEdit.SetWindowText(bw_utf8tow( time ).c_str());
		pImpl_->timeOfDaySlider.SetPos( (int)(
			pImpl_->timeOfDaySlider.GetRangeMin() +
			(pImpl_->timeOfDaySlider.GetRangeMax() - pImpl_->timeOfDaySlider.GetRangeMin())
			* MeShell::instance().timeOfDay()->gameTime() / 24.f ));
		Options::setOptionString( "settings/gameTime", time );
		pImpl_->last_time = time;
	}


	return 0;
}

void PageDisplay::OnBnClickedShowAxes()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowAxes");
	}
}

void PageDisplay::OnBnClickedCheckForSparkles()
{
	BW_GUARD;

	if ( PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actCheckForSparkles");
	}
}

void PageDisplay::OnBnClickedEnableFog()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actEnableFog");
	}
}

void PageDisplay::OnBnClickedEnableHDR()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actEnableHDR");
	}
}

void PageDisplay::OnBnClickedEnableSSAO()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actEnableSSAO");
	}
}

void PageDisplay::OnBnClickedShowModel()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowModel");
	}
}

void PageDisplay::OnBnClickedShowWireframe()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowWireframe");
	}
}

void PageDisplay::OnBnClickedShowSkeleton()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowSkeleton");
	}
}

void PageDisplay::OnBnClickedShowBsp()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowBsp");
	}
}

void PageDisplay::OnBnClickedShowBoundingBox()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowBoundingBox");
		GUI::Manager::instance().update(); // Need this since it will cause a toolbar update
	}
}

void PageDisplay::OnBnClickedShowPortals()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowPortals");
	}
}

void PageDisplay::OnBnClickedShowVertexNormals()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowNormals");
	}
}

void PageDisplay::OnBnClickedShowVertexBinormals()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowBinormals");
	}
}

void PageDisplay::OnBnClickedShowCustomHull()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowCustomHull");
	}
}

void PageDisplay::OnBnClickedShowHardPoints()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowHardPoints");
	}
}

void PageDisplay::OnBnClickedGroundModel()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actGroundModel");
	}
}

void PageDisplay::OnBnClickedCentreModel()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actCentreModel");
	}
}

void PageDisplay::OnBnClickedShowEditorProxy()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShowEditorProxy");
	}
}

void PageDisplay::OnCbnChangeShadowing()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		switch (pImpl_->shadowing.GetCurSel())
		{
			case 0:
			default:
				PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShadowMaxQuality");
			break;
			case 1:
				PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShadowHighQuality");
			break;
			case 2:
				PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShadowMedQuality");
			break;
			case 3:
				PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShadowLowQuality");
			break;
			case 4:
				PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actShadowOff");
			break;
		};
	}
}

void PageDisplay::OnCbnSelchangeDisplayBkg()
{
	BW_GUARD;

	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		CWaitCursor wait; // This could take a little while...

		switch( pImpl_->bkg.GetCurSel() )
		{
		case 0:
			if (PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actBkgNone"))
			{
				pImpl_->flora.ModifyStyle( 0, WS_DISABLED );
				pImpl_->bkgColour.ModifyStyle( WS_DISABLED, 0 );
				pImpl_->floorTexture.ModifyStyle( 0, WS_DISABLED );
			}
		break;
		case 1:
			if (PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actBkgFloor"))
			{
				pImpl_->flora.ModifyStyle( 0, WS_DISABLED );
				pImpl_->bkgColour.ModifyStyle( WS_DISABLED, 0 );
				pImpl_->floorTexture.ModifyStyle( WS_DISABLED, 0 );
			}
		break;
		case 2:
		default:
			if (PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptExecute("actBkgTerrain"))
			{
				pImpl_->flora.ModifyStyle(
					pImpl_->floraSettings ? WS_DISABLED : 0,
					pImpl_->floraSettings ? 0 : WS_DISABLED );
				pImpl_->bkgColour.ModifyStyle( 0, WS_DISABLED );
				pImpl_->floorTexture.ModifyStyle( 0, WS_DISABLED );
			}
		break;
		};
		pImpl_->flora.RedrawWindow();
		pImpl_->bkgColour.RedrawWindow();
		pImpl_->floorTexture.RedrawWindow();
	}
}

void PageDisplay::OnCbnSelchangeDisplayFlora()
{
	BW_GUARD;

	if (pImpl_->floraSettings)
	{
		int index = pImpl_->numFloraOptions - pImpl_->flora.GetCurSel() - 1;
		pImpl_->floraSettings->selectOption( index );
		Options::setOptionInt( "settings/floraDensity", index );
	}
}

void PageDisplay::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	MeApp::instance().mutant()->normalsLength( pImpl_->normalsLength.GetPos() );
	if (PanelManager::instance().getEditorApp()->pythonAdapter())
	{
		PanelManager::instance().getEditorApp()->pythonAdapter()->onSliderAdjust("slrCurrentTime", 
												pImpl_->timeOfDaySlider.GetPos(), 
												pImpl_->timeOfDaySlider.GetRangeMin(), 
												pImpl_->timeOfDaySlider.GetRangeMax());

		BW::string currentTime = MeShell::instance().timeOfDay()->getTimeOfDayAsString();
		pImpl_->timeOfDayEdit.SetWindowText( bw_utf8tow( currentTime ).c_str() );
	}

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageDisplay::OnBnClickedDisplayChooseBkgColour()
{
	BW_GUARD;

	Vector3 bkgColour = Options::getOptionVector3("settings/bkgColour", Vector3( 255.f, 255.f, 255.f ) );
	CColorDialog colorDlg( RGB( bkgColour[0], bkgColour[1], bkgColour[2] ), CC_FULLOPEN );
	if( colorDlg.DoModal() == IDOK )
	{
		COLORREF col = colorDlg.GetColor();
		int r = (col) & 0xff;
		int g = (col / 256) & 0xff;
		int b = (col / 65536) & 0xff;
		Options::setOptionVector3("settings/bkgColour", Vector3((float) r, (float)g, (float)b ) );
	}
}

void PageDisplay::setFloorTexture( const BW::wstring& texture )
{
	BW_GUARD;

	Options::setOptionString( "settings/floorTexture", bw_wtoutf8( texture ) );
	MeApp::instance().floor()->setTextureName( bw_wtoutf8( texture ) );
}

bool PageDisplay::floorTextureDrop( UalItemInfo* ii )
{
	BW_GUARD;

	setFloorTexture( BWResource::dissolveFilenameW( ii->longText() ) );
	return true;
}

void PageDisplay::OnBnClickedDisplayChooseFloorTexture()
{
	BW_GUARD;

	static wchar_t szFilter[] =	L"Texture files(*.bmp;*.tga;*.jpg;*.png;*.dds;*.texanim)|*.bmp;*.tga;*.jpg;*.png;*.dds;*.texanim|"
								L"Bitmap files(*.bmp)|*.bmp|"
								L"Targa files(*.tga)|*.tga|"
								L"Jpeg files(*.jpg)|*.jpg|"
								L"Png files(*.png)|*.png|"
								L"DDS files(*.dds)|*.dds|"
								L"Animated Textures (*.texanim)|*.texanim||";
	BWFileDialog fileDlg (true, L"", L"", BWFileDialog::FD_WRITABLE_AND_EXISTS, szFilter);

	BW::string floorTextureDir = BWResource::resolveFilename( MeApp::instance().floor()->getTextureName() );
	std::replace( floorTextureDir.begin(), floorTextureDir.end(), '/', '\\' );
	BW::wstring wfloorTextureDir = bw_utf8tow( floorTextureDir );
	fileDlg.initialDir( wfloorTextureDir.c_str() );

	if (fileDlg.showDialog())
	{
		BW::wstring floorTexture = BWResource::dissolveFilenameW( fileDlg.getFileName());

		if (BWResource::validPathW( floorTexture ))
		{
			setFloorTexture( floorTexture );
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/BAD_DIR"),
				Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/UNABLE_RESOLVE"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

BW_END_NAMESPACE

