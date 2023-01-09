#include "pch.hpp"
#include "main_frame.hpp"
#include "particle_editor.hpp"
#include "particle_editor_doc.hpp"
#include "gui/panel_manager.hpp"
#include "gui/ps_node.hpp"
#include "gui/action_selection.hpp"
#include "gui/propdlgs/mps_properties.hpp"
#include "gui/propdlgs/ps_properties.hpp"
#include "gui/propdlgs/ps_renderer_properties.hpp"
#include "gui/propdlgs/psa_source_properties.hpp"
#include "gui/propdlgs/psa_sink_properties.hpp"
#include "gui/propdlgs/psa_barrier_properties.hpp"
#include "gui/propdlgs/psa_force_properties.hpp"
#include "gui/propdlgs/psa_stream_properties.hpp"
#include "gui/propdlgs/psa_scaler_properties.hpp"
#include "gui/propdlgs/psa_orbitor_properties.hpp"
#include "gui/propdlgs/psa_flare_properties.hpp"
#include "gui/propdlgs/psa_tint_shader_properties.hpp"
#include "gui/propdlgs/psa_empty_properties.hpp"
#include "gui/propdlgs/psa_magnet_properties.hpp"
#include "gui/propdlgs/psa_jitter_properties.hpp"
#include "gui/propdlgs/psa_collide_properties.hpp"
#include "gui/propdlgs/psa_nodeclamp_properties.hpp"
#include "gui/propdlgs/psa_matrixswarm_properties.hpp"
#include "gui/propdlgs/psa_splat_properties.hpp"
#include "gui/dialogs/splash_dialog.hpp"
#include "gui/controls/color_picker_dialog.hpp"
#include "shell/pe_shell.hpp"
#include "common/tools_camera.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "appmgr/Options.hpp"
#include "cstdmf/message_box.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "chunk/chunk_manager.hpp"
#include "gizmo/meta_data.hpp"
#include "gizmo/meta_data_helper.hpp"

DECLARE_DEBUG_COMPONENT2("ParticleEditor", 0)

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(MainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(MainFrame, CFrameWnd)
    ON_WM_CREATE()
	ON_WM_DESTROY()
    
    //For now the help has been removed, this will be reimplemented in 1.8
    //ON_COMMAND(ID_HELP_FINDER , CFrameWnd::OnHelpFinder )
    //ON_COMMAND(ID_HELP        , CFrameWnd::OnHelp       )
    //ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    //ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder )

    ON_UPDATE_COMMAND_UI(ID_PERFORMANCE_PANE, OnUpdatePerformancePane)
    ON_WM_SIZE()
	ON_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove)
	ON_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove)
    ON_WM_CLOSE()
    ON_COMMAND(ID_UNDO , OnUndo )
    ON_COMMAND(ID_REDO , OnRedo )
    ON_COMMAND(ID_PLAY , OnPlay )
    ON_COMMAND(ID_STOP , OnStop )
    ON_COMMAND(ID_PAUSE, OnPause)
    ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
    ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)    
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_PERFORMANCE_PANE,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

namespace
{
    const int c_menuPaneWidth   = 360;
    const int c_topPaneHight    = 225;
    const int c_middlePaneHight = 410;
    const int c_bottomPaneHight =  20;
}

/*static*/ MainFrame *MainFrame::s_instance = NULL;

MainFrame::MainFrame() : 
    performancePaneString_(_T("None")),
    initialised_(false),
    particleDirectory_("particles/"),
    colorDialogThread_(NULL),
    bgColour_(0.63f, 0.63f, 0.81f, 0.f),
    skipForceActionPorpertiesUpdate_( false ),
    pendingLBtnUpDesc_( "" ),
    pendingLbtnUpKind_( -1 ),
    undoing_( false ),
    potentiallyDirty_( false ),
    psaDlg_(NULL),
    resizing_( false )
{
    s_instance = this;
    m_bAutoMenuEnable = FALSE;
}

MainFrame::~MainFrame()
{   
    s_instance = NULL;
}

bool MainFrame::SelectParticleSystem(BW::string const &name)
{
	BW_GUARD;

    ActionSelection *actionSelection = GetActionSelection();
    return actionSelection->selectMetaParticleSystem(name);
}

int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	BW_GUARD;

	MsgBox::setDefaultParent( m_hWnd );
	SetWindowLong( m_hWnd, GWL_STYLE, GetStyle() & ~FWS_ADDTOTITLE );

    bool drawScene = true;
    
    // set initial particle directory from the options file
	particleDirectory_ = bw_utf8tow( Options::getOptionString( "defaults/startDirectory", bw_wtoutf8( particleDirectory_.GetString() ) ) ).c_str();

	Vector4 colour;
	colour.x = bgColour_.r;
    colour.y = bgColour_.g;
    colour.z = bgColour_.b;
    colour.w = bgColour_.a;	
	colour = Options::getOptionVector4( "defaults/backgroundColour", colour );
    bgColour_.r = colour.x;
    bgColour_.g = colour.y;
    bgColour_.b = colour.z;
    bgColour_.a = colour.w;

	drawScene = Options::getOptionBool( "defaults/drawScene", true );

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    //Added a splashscreen
    if( !IsDebuggerPresent() )
        CSplashDlg::ShowSplashScreen( this );

    if 
    (
        !wndStatusBar_.Create(this) 
        ||
        !wndStatusBar_.SetIndicators
        (
            indicators, 
            sizeof(indicators)/sizeof(UINT)
        )
    )
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // setup the performance pane
    SetPerformancePaneText(performancePaneString_);

    EnableDocking(CBRS_ALIGN_ANY);

    return 0;
}

void MainFrame::OnDestroy()
{
	MsgBox::setDefaultParent( NULL );
}

/*virtual*/ LRESULT MainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (message == WM_INPUT)
	{
		// In the main frame, only relay raw input messages.
		LRESULT inputResult;
		if (InputDevices::handleWindowsMessage(this->m_hWnd, message, wParam, lParam, inputResult))
		{
			return inputResult;
		}
	}

	return CFrameWnd::WindowProc(message, wParam, lParam);
}

BOOL MainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	BW_GUARD;

    cs.style |= WS_MAXIMIZE;

    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;

    return TRUE;
}

void MainFrame::OnUpdatePerformancePane(CCmdUI * pCmdUI)
{
	BW_GUARD;

    pCmdUI->Enable();
    pCmdUI->SetText(performancePaneString_);
}

void MainFrame::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

    if (::IsWindow(GetSafeHwnd()))
	    GUI::Manager::instance().act( nID );
}

void MainFrame::OnGUIManagerCommandUpdate(CCmdUI *cmdUI)
{
	BW_GUARD;

	GUI::Manager::instance().update(cmdUI->m_nID);
}

void MainFrame::SetPerformancePaneText(CString text)
{
	BW_GUARD;

    performancePaneString_ = text;

    CClientDC dc(this);
    SIZE size  = dc.GetTextExtent(performancePaneString_);
    int  index = wndStatusBar_.CommandToIndex(ID_PERFORMANCE_PANE);
    wndStatusBar_.SetPaneInfo
    (
        index, 
        ID_PERFORMANCE_PANE, 
        SBPS_NOBORDERS, 
        size.cx
    );
    wndStatusBar_.SetPaneText(index, performancePaneString_, TRUE);
}

ParticleSystemPtr MainFrame::GetCurrentParticleSystem()
{
	BW_GUARD;

    ActionSelection *actionSelection = GetActionSelection();
    return actionSelection->getCurrentParticleSystem();
}

bool MainFrame::IsCurrentParticleSystem()
{
	BW_GUARD;

    ActionSelection *actionSelection = GetActionSelection();
    return actionSelection->getCurrentParticleSystem() != NULL;
}

MetaParticleSystemPtr MainFrame::GetMetaParticleSystem()
{
	BW_GUARD;

    ActionSelection *actionSelection = GetActionSelection();
    return actionSelection->getMetaParticleSystem();
}

bool MainFrame::IsMetaParticleSystem()
{
	BW_GUARD;

    ActionSelection *actionSelection = GetActionSelection();
    return actionSelection->isMetaParticleSystemSelected();
}


void 
MainFrame::ChangeToActionPropertyWindow
(
    int                     index, 
    ParticleSystemActionPtr action
)
{
	BW_GUARD;

    ParticleEditorApp::State oldState 
        = ParticleEditorApp::instance().getState();
    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PAUSED);   

    ActionSelection *actionSelection = GetActionSelection();
    ASSERT(actionSelection != NULL);

	psaDlg_ = NULL;

    CWnd *newDlg = NULL;

    if (action == NULL)
    {
		if (index == ActionNode::AT_META_PS)
        {
            CFormView *dlg = new MpsProperties();
            actionSelection->setSubDlg(dlg);
            newDlg = dlg;
        }
        // System properties?
        else if (index == ActionNode::AT_SYS_PROP)
        {
            CFormView *dlg = new PsProperties();
            actionSelection->setSubDlg(dlg);
            newDlg = dlg;
            actionSelection->addSystemOffsetGizmo();
        }
        else if (index == ActionNode::AT_REND_PROP)
        {
            CFormView *dlg = new PsRendererProperties();
            actionSelection->setSubDlg(dlg);
            newDlg = dlg;
        }
    }
    else
    {
        // Gizmos are used in some of the action property windows, 
        // remove the system position one to avoid confusion
        actionSelection->removeSystemOffsetGizmo();

        PsaProperties *dlg = NULL;
        int           id   = 0;

        switch (index)
        {
            case PSA_SOURCE_TYPE_ID:
                dlg = new PsaSourceProperties();
                id  = PsaSourceProperties::IDD;
                break;
            case PSA_SINK_TYPE_ID:
                dlg = new PsaSinkProperties();
                id  = PsaSinkProperties::IDD;
                break;
            case PSA_BARRIER_TYPE_ID:
                dlg = new PsaBarrierProperties();
                id  = PsaBarrierProperties::IDD;
                break;
            case PSA_FORCE_TYPE_ID:
                dlg = new PsaForceProperties();
                id  = PsaForceProperties::IDD;
                break;
            case PSA_STREAM_TYPE_ID:
                dlg = new PsaStreamProperties();
                id  = PsaStreamProperties::IDD;
                break;
            case PSA_SCALAR_TYPE_ID:
                dlg = new PsaScalerProperties();
                id  = PsaScalerProperties::IDD;
                break;
            case PSA_ORBITOR_TYPE_ID:
                dlg = new PsaOrbitorProperties();
                id  = PsaOrbitorProperties::IDD;
                break;
            case PSA_FLARE_TYPE_ID:
                dlg = new PsaFlareProperties();
                id  = PsaFlareProperties::IDD;
                break;
            case PSA_NODE_CLAMP_TYPE_ID:
                dlg = new PsaNodeClampProperties();
                id  = PsaNodeClampProperties::IDD;
                break;
            case PSA_TINT_SHADER_TYPE_ID:
                dlg = new PsaTintShaderProperties();
                id  = PsaTintShaderProperties::IDD;
                break;    
            case PSA_MAGNET_TYPE_ID:
                dlg = new PsaMagnetProperties();
                id  = PsaMagnetProperties::IDD;
                break;    
            case PSA_JITTER_TYPE_ID:
                dlg = new PsaJitterProperties();
                id  = PsaJitterProperties::IDD;
                break;
            case PSA_COLLIDE_TYPE_ID:
                dlg = new PsaCollideProperties();
                id  = PsaCollideProperties::IDD;
                break;
            case PSA_MATRIX_SWARM_TYPE_ID:
                dlg = new PsaMatrixSwarmProperties();
                id  = PsaMatrixSwarmProperties::IDD;
                break;
            case PSA_SPLAT_TYPE_ID:
                dlg = new PsaSplatProperties();
                id  = PsaSplatProperties::IDD;
                break;
            default:
                dlg = new PsaEmptyProperties();
                id  = PsaEmptyProperties::IDD;
                break;
        }
        if (dlg != NULL)
        {
            psaDlg_ = dlg;
            dlg->SetPSA(action);
            actionSelection->setSubDlg(dlg);
            newDlg = dlg;
        }
    }

    ParticleEditorApp::instance().setState(oldState);  
}

void MainFrame::CopyFromDataSection(size_t kind, DataSectionPtr newState)
{
	BW_GUARD;

    // This method is used internally by the undo/redo system;
    // it restores the state of the particle system to the given
    // data section.
    //
    // In order for redo to work, it must first save the current
    // state and add a new UndoRedoOp before setting the new state.

    MF_ASSERT(IsMetaParticleSystem());  // make sure there is a particle system

    ActionSelection *actionSelection = GetActionSelection();

    //Save the old state, for undo ( or redo )    
    XMLSectionPtr currentState = new XMLSection( "undoState" );
    actionSelection->reserialise(currentState, false, true); // save, transient 
    UndoRedo::instance().add
    ( 
        new UndoRedoOp((UndoRedoOp::ActionKind)kind, currentState) 
    );

    //And set the new state
    actionSelection->reserialise(newState, true, true); // load, transient
}

void MainFrame::SaveUndoState(int actionKind, const BW::string& changeDesc, bool addBarrier)
{
	BW_GUARD;

    // This method is used internally to cache the current state of the particle
    // system + GUI.
    //
    // It checks whether there is actually any change in the particle system.

    if (undoing_)
        return;

    ActionSelection *actionSelection = GetActionSelection();
    XMLSectionPtr pDS = new XMLSection("undoState");
    actionSelection->reserialise(pDS, false, true); // save, transient  
   
    UndoRedo::instance().add
    (
        new UndoRedoOp((UndoRedoOp::ActionKind)actionKind, pDS)
    );

	if (addBarrier)
	{
	    UndoRedo::instance().barrier( changeDesc, true );
	}
}

void MainFrame::OnBatchedUndoOperationEnd()
{
	BW_GUARD;

    // This method is public and called at the end of a batched operation,
    // for example mouse-up when dragging a slider control.  It simply
    // adds a barrier to signify the end of the operation.
    
    if (pendingLbtnUpKind_ != -1)
    {
        pendingLBtnUpDesc_ = "";
        pendingLbtnUpKind_ = -1;
    }
}


/**
 *  This method is called whenever a change is made to a particle system.
 *
 *  @param  option - whether the particle system is now potentially dirty or not.
 *  @param  actionKind - integer specifying the type of operation, for gui refresh
 *  @param  changeDesc - string description of the operation, for undo/redo
 *  @param  wait       - signifies a batched interactive operation, for undo/redo
 *  @param  addBarrier - add an undo/redo barrier (last call of a batched operation)
 */
void 
MainFrame::PotentiallyDirty
(
    bool                    option,
    UndoRedoOp::ActionKind  actionKind,
    const BW::string &          changeDesc,
    bool                    waitForLButtonUp,
	bool					addBarrier
)
{
	BW_GUARD;

    potentiallyDirty_ = option; // Save it away for future reference

	if (potentiallyDirty_)
	{
		MetaData::Environment::instance().changed( NULL );
	}
    
    ActionSelection *actionSelection = GetActionSelection();
    if (actionSelection->GetSafeHwnd() == NULL)
        return;

    if (!IsMetaParticleSystem())
        return;

    // save the new state into the undo / redo list
    if (option)
    {
        if ( !waitForLButtonUp )
        {
            // All non-batched operations signify that any batched
            // operation is now complete.
            OnBatchedUndoOperationEnd();
            SaveUndoState( actionKind, changeDesc, addBarrier );
        }
        else
        {
            if 
            (
                (changeDesc != pendingLBtnUpDesc_) 
                || 
                (actionKind != pendingLbtnUpKind_)
            )
            {
                // Save the new type of batched undo
                pendingLBtnUpDesc_ = changeDesc;
                pendingLbtnUpKind_ = actionKind;
                // Save the undo state
                SaveUndoState( actionKind, changeDesc, addBarrier );
            }
            //else we are still performing a batched operation.
        }            
    }
    else
    {
        UndoRedo::instance().clear();  
    }

    UpdateTitle();
}

void MainFrame::OnClose()
{
	BW_GUARD;

    if (PromptSave(MB_YESNOCANCEL) == IDCANCEL)
        return;

    PeShell::instance().camera().save();

    Options::save();

    PanelManager::instance().onClose();

    CFrameWnd::OnClose();
}

void MainFrame::RefreshGUI(size_t /*kind*/)
{
	BW_GUARD;

    PsaProperties *actionFrame = GetPsaProperties();
    if (actionFrame)
        actionFrame->CopyDataToControls();
}

void MainFrame::UpdateGUI()
{
	BW_GUARD;

	//Update the undo/redo buttons if needed
	static bool firstTime = true;
	static bool canUndo, canRedo;
	if
    ( 
        firstTime 
        ||
		canUndo != UndoRedo::instance().canUndo() 
        ||
		canRedo != UndoRedo::instance().canRedo() 
    )
	{
		firstTime = false;
		canUndo = UndoRedo::instance().canUndo();
		canRedo = UndoRedo::instance().canRedo();
		GUI::Manager::instance().update();
	}

	if ( deferredGuiUpdate_ )
	{
		GUI::Manager::instance().update();
		deferredGuiUpdate_ = false;
	}

	//Update all the Panels
	PanelManager::instance().updateControls();
}

ActionSelection *MainFrame::GetActionSelection() const
{
	BW_GUARD;

    return 
        (ActionSelection*)PanelManager::instance().panels().getContent
        (
            ActionSelection::contentID
        );
}

PsaProperties *MainFrame::GetPsaProperties() const
{
    return psaDlg_;
}

void MainFrame::InitialiseMetaSystemRegister()
{
	BW_GUARD;

    ActionSelection *actionSelection = GetActionSelection();
    actionSelection->reload();
}

void MainFrame::SetDocumentTitle(BW::string const &title)
{
	BW_GUARD;

    title_ = title;
    UpdateTitle();
}

void MainFrame::UpdateTitle()
{
	BW_GUARD;

	BW::wstring title = bw_utf8tow( title_ );
	if( title.empty() )
		title = Localise(L"PARTICLEEDITOR/UNTITLED" );
    if (potentiallyDirty_)
        title += L" *";
    ParticleEditorDoc::instance().SetTitle(title.c_str());
	SetWindowText( Localise(L"PARTICLEEDITOR/DASH_PARTICLEEDITOR", title ) );
}

/*virtual*/ CDocument *MainFrame::GetActiveDocument()
{
	BW_GUARD;

    return &ParticleEditorDoc::instance();
}

void MainFrame::OnButtonViewFree()
{
	BW_GUARD;

    PeShell::instance().camera().mode(ToolsCamera::CameraMode_Free);
}

void MainFrame::OnButtonViewX()
{
	BW_GUARD;

    PeShell::instance().camera().mode(ToolsCamera::CameraMode_X);
}

void MainFrame::OnButtonViewY()
{
	BW_GUARD;

    PeShell::instance().camera().mode(ToolsCamera::CameraMode_Y);
}

void MainFrame::OnButtonViewZ()
{
	BW_GUARD;

    PeShell::instance().camera().mode(ToolsCamera::CameraMode_Z);
}

void MainFrame::OnButtonViewOrbit()
{
	BW_GUARD;

    PeShell::instance().camera().mode(ToolsCamera::CameraMode_Orbit);
}

void MainFrame::OnUndo()
{
	BW_GUARD;

    undoing_ = true;
    MF_ASSERT(IsMetaParticleSystem());
    OnBatchedUndoOperationEnd();
    UndoRedo::instance().undo();
    potentiallyDirty_ = true;
    UpdateTitle();
    undoing_ = false;
	ActionSelection::instance()->refreshSelection();
}

bool MainFrame::CanUndo() const
{
	BW_GUARD;

    return UndoRedo::instance().canUndo();
}

void MainFrame::OnRedo()
{
	BW_GUARD;

    undoing_ = true;
    MF_ASSERT(IsMetaParticleSystem());  
    UndoRedo::instance().redo();
    potentiallyDirty_ = true;
    UpdateTitle();
    undoing_ = false;
	ActionSelection::instance()->refreshSelection();
}

bool MainFrame::CanRedo() const
{
	BW_GUARD;

    return UndoRedo::instance().canRedo();
}

void MainFrame::ForceSave()
{
	BW_GUARD;

    ActionSelection *actionSelectionFrame = GetActionSelection();
    if ( actionSelectionFrame->save() )
		potentiallyDirty_ = false;
    UpdateTitle();
}

int MainFrame::PromptSave(UINT type, bool clearUndoStack /*= false*/)
{
	BW_GUARD;

    int result = IDYES;
    if (potentiallyDirty_)
    {        
        ActionSelection *actionSelection = GetActionSelection();
		if ( actionSelection->isSelectionReadOnly() )
		{
			result = IDNO;
		}
		else
		{
			result = 
				::MessageBox
				( 
					AfxGetMainWnd()->GetSafeHwnd(), 
					Localise(L"`RCS_IDS_PROMPTSAVE"),
					Localise(L"`RCS_IDS_PROMPTSAVETITLE"),
					type | MB_ICONWARNING 
				);
		}
        if (result == IDYES)
        {            
            actionSelection->save();
            potentiallyDirty_ = false;
        }
        else if (result == IDNO)
        {
            actionSelection->onNotSave();
        }
    }
    if (clearUndoStack && result != IDCANCEL)
    {
        UndoRedo::instance().clear();
        potentiallyDirty_ = false;
    }
    return result;
}

void MainFrame::ForceActionPropertiesUpdate()
{
	BW_GUARD;

    if (skipForceActionPorpertiesUpdate_)
    {
        skipForceActionPorpertiesUpdate_ = false;
        return;
    }
    PsaProperties *actionFrame = GetPsaProperties();
    if (actionFrame)
        actionFrame->CopyDataToControls();
}

void MainFrame::ParticlesDirectory(CString directory)
{
	BW_GUARD;

    particleDirectory_ = directory;
    // set initial particle directory in the options file
	Options::setOptionString( "defaults/startDirectory", bw_wtoutf8( particleDirectory_.GetString() ) );
}

void MainFrame::BgColour(Moo::Colour c)
{
	BW_GUARD;

    bgColour_ = c;
    Vector4 colour;
    colour.x = bgColour_.r;
    colour.y = bgColour_.g;
    colour.z = bgColour_.b;
    colour.w = bgColour_.a;
	Options::setOptionVector4( "defaults/backgroundColour", colour );
}

POINT MainFrame::CurrentCursorPosition() const
{
	BW_GUARD;

    POINT pt;
    pt.x = pt.y = 0;
    CWnd *view = GetActiveView();
    if (view != NULL)
    {
        ::GetCursorPos(&pt);
        view->ScreenToClient(&pt);
    }
    return pt;
}

Vector3 MainFrame::GetWorldRay(int x, int y) const
{
	BW_GUARD;

    Vector3 v = 
        Moo::rc().invView().applyVector
        (
            Moo::rc().camera().nearPlanePoint
            (
                (float(x)/Moo::rc().screenWidth()) * 2.f - 1.f,
                1.f - (float(y)/Moo::rc().screenHeight()) * 2.f
            ) 
        );
    v.normalise();
    return v;
}

BOOL MainFrame::CursorOverGraphicsWnd() const
{
	BW_GUARD;

	HWND fore = ::GetForegroundWindow();
	if 
    ( 
        fore != PeShell::instance().hWndApp() 
        && 
        ::GetParent(fore) != PeShell::instance().hWndApp()
    )
    {
		return false; // foreground window is not the main window nor a floating panel.
    }

	RECT rt;
	::GetWindowRect(PeShell::instance().hWndGraphics(), &rt);
	POINT pt;
	::GetCursorPos(&pt);

	if 
    (
        pt.x < rt.left 
        ||
		pt.x > rt.right 
        ||
		pt.y < rt.top 
        ||
		pt.y > rt.bottom 
    )
	{
		return false;
	}

	HWND hwnd = ::WindowFromPoint(pt);
	if (hwnd != PeShell::instance().hWndGraphics())
		return false; // it's a floating panel, return.
	HWND parent = hwnd;
	while(::GetParent(parent) != NULL)
		parent = ::GetParent(parent);
	::SendMessage
    (
        hwnd, 
        WM_MOUSEACTIVATE, 
        (WPARAM)parent, 
        MAKELPARAM(HTCLIENT, WM_LBUTTONDOWN)
    );

	return true;
}

void MainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
}

LRESULT MainFrame::OnEnterSizeMove (WPARAM, LPARAM)
{
	// Set the resizing_ flag to true, so the view knows that we are resizing
	// and that it shouldn't change the Moo mode.
	resizing_ = true;
	return 0;
}

LRESULT MainFrame::OnExitSizeMove (WPARAM, LPARAM)
{
	BW_GUARD;

	// Set the resizing_ flag back to false, so the view knows that it has to
	// change the Moo mode on the next repaint.
	resizing_ = false;
	// And send the repaint message to the view.
	::InvalidateRect( PeShell::instance().hWndGraphics(), NULL, TRUE );
	::UpdateWindow( PeShell::instance().hWndGraphics() );
	return 0;
}

BOOL 
MainFrame::Create
(
    LPCTSTR         lpszClassName, 
    LPCTSTR         lpszWindowName, 
    DWORD           dwStyle, 
    RECT            const &rect, 
    CWnd            *pParentWnd, 
    LPCTSTR         lpszMenuName, 
    DWORD           dwExStyle, 
    CCreateContext  *pContext
)
{
	BW_GUARD;

    dwStyle |= WS_MAXIMIZE;
    BOOL ans = 
        CFrameWnd::Create
        (
            lpszClassName, 
            lpszWindowName, 
            dwStyle, 
            rect, 
            pParentWnd, 
            lpszMenuName, 
            dwExStyle, 
            pContext
        );
    return ans;
}

void MainFrame::OnBackgroundColor()
{
	BW_GUARD;

	if ( !colorDialogThread_ )
	{
		colorDialogThread_ = 
			(ColorPickerDialogThread *)
				(AfxBeginThread(RUNTIME_CLASS(ColorPickerDialogThread)));
	}
}

void MainFrame::DereferenceColorDialogThread()
{
	colorDialogThread_ = NULL;
	deferredGuiUpdate_ = true;
}

void MainFrame::UpdateBackgroundColor()
{
	BW_GUARD;

    if (!colorDialogThread_)
        return;

    if (!colorDialogThread_->getDialog())
        return;

    Vector4 newColor = colorDialogThread_->getDialog()->colorSelected();
    MainFrame::instance()->BgColour
    (
        Moo::Colour
        (
            newColor.x, 
            newColor.y, 
            newColor.z, 
            newColor.w
        )
    );
}

void MainFrame::appendOneShotPS()
{
	BW_GUARD;

    ActionSelection *as = GetActionSelection();
    if (as != NULL)
        as->appendOneShotPS();
}

void MainFrame::clearAppendedPS()
{
	BW_GUARD;

    ActionSelection *as = GetActionSelection();
    if (as != NULL)
        as->clearAppendedPS();
}

size_t MainFrame::numberAppendPS() const
{
	BW_GUARD;

    ActionSelection *as = GetActionSelection();
    if (as != NULL)
        return as->numberAppendPS();
    else
        return 0;
}

MetaParticleSystem &MainFrame::getAppendedPS(size_t idx)
{
	BW_GUARD;

    ActionSelection *as = GetActionSelection();
    return as->getAppendedPS(idx);
}

void MainFrame::cleanupAppendPS()
{
	BW_GUARD;

    ActionSelection *as = GetActionSelection();
    if (as != NULL)
        as->cleanupAppendPS();
}

void MainFrame::OnPlay()
{
	BW_GUARD;

    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    GUI::Manager::instance().update();
}

void MainFrame::OnStop()
{
	BW_GUARD;

    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_STOPPED);
    GUI::Manager::instance().update();
}

void MainFrame::OnPause()
{
	BW_GUARD;

	if (InputDevices::isCtrlDown())
	{
		return;
	}

    if 
    ( 
        ParticleEditorApp::instance().getState()
        ==
        ParticleEditorApp::PE_PAUSED
        )
    {
        ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    }
    else
    {
        ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PAUSED);
    }
    GUI::Manager::instance().update();
}

void MainFrame::setMessageText( const wchar_t * pText )
{
	SetMessageText( pText );
}


void MainFrame::setStatusText( UINT id, const wchar_t * text )
{
	getStatusBar().SetPaneText(
		getStatusBar().CommandToIndex(id),
		text,
		TRUE);
}

class PEMetaDataEnvironment : public MetaData::Environment
{
	virtual void changed( void* param )
	{
		if (!UndoRedo::instance().isUndoing())
		{
			BW_GUARD;

			MetaData::updateModificationInfo(
				MainFrame::instance()->GetMetaParticleSystem()->metaData() );
		}
	}
};

static PEMetaDataEnvironment s_metaDataEnvironment;
BW_END_NAMESPACE

