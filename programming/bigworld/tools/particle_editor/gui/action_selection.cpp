#include "pch.hpp"
#include "action_selection.hpp"
#include "ps_node.hpp"
#include "main_frame.hpp"
#include "particle_editor.hpp"
#include "particle_editor_view.hpp"
#include "controls/utils.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "particle/actions/collide_psa.hpp"
#include "particle/actions/jitter_psa.hpp"
#include "particle/actions/source_psa.hpp"
#include "particle/actions/tint_shader_psa.hpp"
#include "particle/renderers/particle_system_renderer.hpp"
#include "particle/renderers/mesh_particle_renderer.hpp"
#include "particle/renderers/sprite_particle_renderer.hpp"
#include "common/user_messages.hpp"
#include "common/string_utils.hpp"
#include "appmgr/options.hpp"
#include "moo/texture_manager.hpp"
#include "ual/ual_manager.hpp"
#include <afxpriv.h>

DECLARE_DEBUG_COMPONENT2("GUI", 0)

BW_BEGIN_NAMESPACE

namespace
{
    std::pair<UINT, UINT> actionDescriptions[] =
    {
        std::make_pair<UINT, UINT>(IDS_BARRIER_DESC    , PSA_BARRIER_TYPE_ID     ),
        std::make_pair<UINT, UINT>(IDS_COLLIDE_DESC    , PSA_COLLIDE_TYPE_ID     ),
        std::make_pair<UINT, UINT>(IDS_FLARE_DESC      , PSA_FLARE_TYPE_ID       ),
        std::make_pair<UINT, UINT>(IDS_FORCE_DESC      , PSA_FORCE_TYPE_ID       ),
        std::make_pair<UINT, UINT>(IDS_JITTER_DESC     , PSA_JITTER_TYPE_ID      ),
        std::make_pair<UINT, UINT>(IDS_MAGNET_DESC     , PSA_MAGNET_TYPE_ID      ),
        std::make_pair<UINT, UINT>(IDS_MATRIXSWARM_DESC, PSA_MATRIX_SWARM_TYPE_ID),        
        std::make_pair<UINT, UINT>(IDS_NODECLAMP_DESC  , PSA_NODE_CLAMP_TYPE_ID  ),
        std::make_pair<UINT, UINT>(IDS_ORBIT_DESC      , PSA_ORBITOR_TYPE_ID     ),
        std::make_pair<UINT, UINT>(IDS_SCALER_DESC     , PSA_SCALAR_TYPE_ID      ),
        std::make_pair<UINT, UINT>(IDS_SINK_DESC       , PSA_SINK_TYPE_ID        ),
        std::make_pair<UINT, UINT>(IDS_SRC_DESC        , PSA_SOURCE_TYPE_ID      ),
        std::make_pair<UINT, UINT>(IDS_SPLAT_DESC      , PSA_SPLAT_TYPE_ID       ),
        std::make_pair<UINT, UINT>(IDS_STREAM_DESC     , PSA_STREAM_TYPE_ID      ),
        std::make_pair<UINT, UINT>(IDS_TINT_DESC       , PSA_TINT_SHADER_TYPE_ID )
    };

    void actionDescriptionTTCB
    (      
        unsigned int    item, 
        BW::string     &tooltip,
        void            * /*data*/
    )
    {
		BW_GUARD;

        if (item < sizeof(actionDescriptions)/sizeof(std::pair<UINT, UINT>))
        {
			const UINT_PTR stringID = actionDescriptions[item].first;
			// Requirement for CString::CString to treat this as a string ID
			MF_ASSERT( HIWORD( stringID ) == 0 );
			CString result( reinterpret_cast< LPCSTR >( stringID ) );
            bw_wtoutf8( result.GetString(), tooltip );
        }
    }

    class PauseApp
    {
    public:
        PauseApp() :
            pauseState_(ParticleEditorApp::instance().getState())
        {
			BW_GUARD;

            ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PAUSED);
        }

        ~PauseApp()
        {
			BW_GUARD;

            ParticleEditorApp::instance().setState(pauseState_);
        }

    private:
        PauseApp(PauseApp const &);             // not allowed
        PauseApp &operator=(PauseApp const &);  // not allowed

    private:
        ParticleEditorApp::State        pauseState_;
    };
}


BEGIN_MESSAGE_MAP(ActionSelection, CDialog)
    ON_NOTIFY(TVN_SELCHANGED  , IDC_PARTICLETREE, OnPartTreeSel        )
    ON_NOTIFY(TVN_ENDLABELEDIT, IDC_PARTICLETREE, OnPartTreeEditLabel  )
	ON_NOTIFY(NM_CLICK		  ,	IDC_PARTICLETREE, OnNMClickParticletree)
    ON_COMMAND(IDC_ADD                  , OnAdd           )
    ON_COMMAND(IDC_COPY                 , OnCopy          )
    ON_COMMAND(IDC_DELETE               , OnDelete        )
    ON_COMMAND(IDC_ADD_COMP             , OnAddAction     )
    ON_COMMAND(ID_FILE_OPEN             , OnOpen          )
    ON_COMMAND(ID_FILE_SVEPARTICLESYSTEM, OnSave          )
    ON_COMMAND(ID_OPENEXPLORER          , OnOpenInExplorer)
    ON_COMMAND(ID_COPYPATHCLIPBOARD     , OnCopyPath      )
    ON_UPDATE_COMMAND_UI(IDC_ADD     , OnAddEnabled      )
    ON_UPDATE_COMMAND_UI(IDC_COPY    , OnCopyEnabled     )
    ON_UPDATE_COMMAND_UI(IDC_DELETE  , OnDeleteEnabled   )
    ON_UPDATE_COMMAND_UI(IDC_ADD_COMP, OnAddActionEnabled)
    ON_EN_CHANGE(SEARCH_FILTER_ID, OnEditFilter)
    ON_WM_SIZE()
    ON_WM_CLOSE()
    ON_MESSAGE(WM_DELETE_NODE    , OnNodeDelete  )
    ON_MESSAGE(WM_DRAG_START     , OnDragStart   )
    ON_MESSAGE(WM_DRAG_DONE      , OnDragDone    )
    ON_MESSAGE(WM_PSTREE_CMENU   , OnPartTreeMenu)
	ON_MESSAGE(WM_CLOSING_PANELS , OnClosing     )
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE(ActionSelection, CDialog)


const BW::wstring ActionSelection::contentID = L"PageActions1ID";
/*static*/ ActionSelection *ActionSelection::s_instance_ = NULL;


/**
 *  This is the constructor.
 */
ActionSelection::ActionSelection():
	CDialog(IDD),
	subDlg_(NULL),
	sysOffsGizmo_(NULL),
	sysPosGizmo_(NULL),
	selection_(NULL),
	filterPromptSave_(false),
	pauseState_(0),
	filterFocusSelection_(0)
{
    s_instance_ = this;
}


/**
 *  This is the destructor.
 */
/*virtual*/ ActionSelection::~ActionSelection()
{
    s_instance_ = NULL;
}


/**
 *  This function returns the instance of the dialog.
 * 
 *  @returns                The instance of the dialog.
 */
/*static*/ ActionSelection *ActionSelection::instance()
{
    return s_instance_;
}


/**
 *  This function is called during the first update.
 */
/*virtual*/ BOOL ActionSelection::OnInitDialog()
{
	BW_GUARD;

    BOOL result = CDialog::OnInitDialog();

    INIT_AUTO_TOOLTIP();

    // Initialise the filter control:
    CWnd *filterProxy = GetDlgItem(IDC_FILTER);
    CRect extents;
    filterProxy->GetWindowRect(extents);
    ScreenToClient(&extents);
    searchFilter_.Create
    (
        WS_CHILD | WS_VISIBLE, 
        extents, 
        this, 
        SEARCH_FILTER_ID,
        IDB_SEARCHFILTER,
        IDB_CLEARFILTER
    );
    CString filterText((LPCSTR)IDS_AS_FILTERTEXT);
    searchFilter_.setEmptyText(filterText.GetBuffer());
    filterProxy->ShowWindow(SW_HIDE);

    // Initialise the action toolbar: 
    actionBar_.CreateEx
    (
        this, 
        TBSTYLE_FLAT, 
        WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
    );
    actionBar_.LoadToolBar(IDR_ACTSELTB);
    actionBar_.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);
    actionBar_.ShowWindow(SW_SHOW);
    actionBar_.Subclass(IDC_TOOLBAR);    

    // Initialise some controls:
    initComponentList();

    // Set the bitmaps for the buttons:
    addActBtn_.setBitmapID(IDB_ADDACT  , IDB_ADDACTD  );
    saveBtn_  .setBitmapID(IDB_SAVE    , IDB_SAVE     );
    openBtn_  .setBitmapID(IDB_OPEN    , IDB_OPEND    );

    // Register interest in drag and drop operations on the particle tree.
    dropTarget_.Register(this, &particleTree_);

    UalManager::instance().dropManager().add
    (
        new UalDropFunctor<ActionSelection>
        (
            ParticleEditorView::instance(), 
            "xml", 
            this,
            &ActionSelection::dropParticleSystem
        ),
		false /*can't highlight the DX window*/
    );

    return result;
}


/**
 *  This function returns whether there is a selected MetaParticleSystem?
 * 
 *  @returns            true if a MetaParticleSystem is selected.
 */
bool ActionSelection::isMetaParticleSystemSelected() const
{
    return particleTree_.GetSelectedMetaParticleSystem() != NULL;
}


/**
 *  This function gets the selected meta particle system.
 * 
 *  @returns            The active meta particle system. 
 */
MetaParticleSystemPtr ActionSelection::getMetaParticleSystem()
{
	BW_GUARD;

    return particleTree_.GetSelectedMetaParticleSystem();
}


/**
 *  This function gets the selected particle system.
 * 
 *  @returns            The selected particle system.  This can be NULL.
 */
ParticleSystemPtr ActionSelection::getCurrentParticleSystem()
{
	BW_GUARD;

    return particleTree_.GetSelectedParticleSystem();
}


/**
 *  This function gets the selected action.
 * 
 *  @returns            The selected action.  This can be NULL.
 */
ParticleSystemActionPtr ActionSelection::getSelectedAction()
{
	BW_GUARD;

    return particleTree_.GetSelectedAction();
}


/**
 *  This function selects a meta particle system.  It collapses the current
 *  system (if there is one), opens another and expands it out.
 *  
 *  @param name         The name of the system to select.
 *  @returns            True if the system was selected, false otherwise.
 */
bool ActionSelection::selectMetaParticleSystem(BW::string const &name)
{
	BW_GUARD;

    for (size_t i = 0; i < particleTree_.NumberChildren(NULL); ++i)
    {
        TreeNode *node = particleTree_.GetChild(NULL, i);
        if (strcmpi(node->GetLabel().c_str(), name.c_str()) == 0)
        {
            MetaNode *selMNode = getMetaNode(selection_);
            if (selMNode != NULL)
                particleTree_.ExpandNode(selMNode, TVE_COLLAPSE);
            MetaNode *mnode = dynamic_cast<MetaNode *>(node);
            if (mnode != NULL)
                mnode->EnsureLoaded();
            particleTree_.SetSelectedNode(node);
            particleTree_.ExpandNode(node, TVE_EXPAND);
			setSelectedMetaNode( mnode );
            return true;
        }
    }
    return false;
}

bool ActionSelection::isSelectionReadOnly() const
{
	BW_GUARD;

	MetaNode *mn = getMetaNode(selection_);
	if ( mn != NULL )
		return mn->IsReadOnly();
	else
		return false;
}

/**
 *  This function saves everything.
 * 
 *  @returns            True if the saving was ok.
 */
bool ActionSelection::save()
{
	BW_GUARD;

    PauseApp pauseApp;

	if ( isSelectionReadOnly() )
		return false;

    MetaNode *mn = getMetaNode(selection_);
    if (mn != NULL)
    {
        mn->onSave();
        MetaParticleSystemPtr mps = mn->GetMetaParticleSystem();
        if (mps != NULL) // may not yet have been loaded
        {
            BW::string dir = directory();
            BW::string filename = mn->GetFilename();
            BW::string dirFile = dir + filename;
		    BW::string fullpath = BWResource::resolveFilename(dirFile);
            mps->save(filename, fullpath.substr( 0, fullpath.find(filename) ) );
			BWResource::instance().purge( dirFile );
	        if (!dirFile.empty())
	        {
		        HANDLE mailSlot = 
                    CreateFile
                    (
                        L"\\\\.\\mailslot\\WorldEditorUpdate",
			            GENERIC_WRITE, 
                        FILE_SHARE_READ, 
                        NULL,
			            OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL, 
                        0 
                    );
		        if (mailSlot != INVALID_HANDLE_VALUE)
		        {
			        DWORD bytesWritten;
			        WriteFile
                    (
                        mailSlot, 
                        fullpath.c_str(), 
                        static_cast<DWORD>(fullpath.size() + 1),
                        &bytesWritten, 
                        0
                    );
			        CloseHandle(mailSlot);
		        }
	        }
        }
    }

    return true;
}


/**
 *  This is called when the user is prompted to save and the user says no.
 */
void ActionSelection::onNotSave()
{
	BW_GUARD;

    MetaNode *mnode = getMetaNode(selection_);
    if (mnode != NULL)
        mnode->onNotSave();
}


/**
 *  This function reload everything.
 * 
 *  @returns            True if the reloading was ok.
 */
bool ActionSelection::reload()
{
	BW_GUARD;

    selection_ = NULL;
    BW::string directory = bw_wtoutf8( MainFrame::instance()->ParticlesDirectory().GetString() );
    SetDlgItemText(IDC_DIR, MainFrame::instance()->ParticlesDirectory().GetString() );
    particleTree_.Load(directory);
    clearAppendedPS();
    return true;
}


/**
 *  This function is called to reserialise the selected metaparticle system 
 *  plus information necessary to do an undo/redo.
 * 
 *  @param data         The data section to save/load to or from.
 *  @param load         True if loading, false if saving.
 *  @param transient    True if only doing a transient save, full saves can
 *                      take longer.
 */
void ActionSelection::reserialise(DataSectionPtr data, bool load, 
    bool transient, bool undoing/*=true*/)
{
	BW_GUARD;

    MetaNode *mnode = getMetaNode(selection_);
    if (mnode == NULL)
        return;  

    // Do the saving/loading:
    mnode->EnsureLoaded();
    MetaParticleSystemPtr mps = mnode->GetMetaParticleSystem();    
    
    // Add as a new particle system with the same name and delete the old
    // meta-particle system:
    if (load)
    {
        TreeNode *sel = selection_;
        DataSectionPtr mpsds = data->findChild("mps");
        mps->reSerialise(mpsds, load, transient);
        filterPromptSave_ = true;
        sel = NULL;            
        MetaNode *newNode =
            addMetaParticleSystem
            (
                mps,
                mnode,
                false,
                false,
                mnode->GetLabel(),
                true,
				mnode->IsReadOnly()
            );                  
        DataSectionPtr ds2 = data->findChild(TreeNode::DefaultSerializeName());
        if (ds2 != NULL)
            sel = newNode->Serialise(ds2, load);
        TreeNode *sel2 = newNode->CopyExpandState(mnode);
		if (undoing)
			newNode->CopyCheckBoxState(mnode);
		particleTree_.updateEnabledState(newNode);
        if (sel2 != NULL)
            sel = sel2;
        removeTreeNode(mnode, false);
        MainFrame::instance()->SetDocumentTitle(newNode->GetFilename());    
		particleTree_.SetSelectedNode(sel);
		if (sel)
			particleTree_.SelectItem( *sel );
        OnSelectComponent(sel);
        filterPromptSave_ = false;        
    }
    else
    {
        DataSectionPtr mpsds = data->newSection("mps");
        mps->reSerialise(mpsds, load, transient);

        DataSectionPtr ds2 = data->newSection(TreeNode::DefaultSerializeName());
        mnode->Serialise(ds2, load);
    }
}


/** 
 *  This function saves the state of this object so that if were to be
 *  destroyed it could be restored.
 *
 *  @param data         The DataSection to save the state to.
 */
void ActionSelection::saveState(DataSectionPtr data)
{
	BW_GUARD;

    if (selection_ != NULL)
        data->writeString("selection", getMetaNode(selection_)->GetLabel());
    reserialise(data, false, true); // save, transient
}


/** 
 *  This function restores the state of this object.
 *
 *  @param data         The DataSection to load the state from.
 */
void ActionSelection::restoreState(DataSectionPtr data)
{
	BW_GUARD;

    reload();
    BW::string selection = data->readString("selection");
    if (!selection.empty())
        selectMetaParticleSystem(selection);
    reserialise(data, true, true, false); // load, transient, not undoing
}


/**
 *  This function set the subdialog to the given dialog.
 * 
 *  @param subdlg       The new subdialog.
 */
void ActionSelection::setSubDlg(CView *dlg)
{
	BW_GUARD;

	CView* oldDlg = subDlg_;

	// if we have no new dialog, destroy old one and return
    if (dlg == NULL)
	{
		// remove the last subdialog
		if (oldDlg)
		{
			MainFrame::instance()->SetActiveView( NULL );
			oldDlg->DestroyWindow();
		}
		return;
	}

    static BW::wstring classname;
    static bool registered = false;
    if (!registered)
    {
        classname = 
            AfxRegisterWndClass
            ( 
                CS_OWNDC, 
                ::LoadCursor(NULL, IDC_ARROW), 
                (HBRUSH)::GetSysColorBrush(COLOR_BTNFACE) 
            );
        registered = true;
    }

    // Create the child if not done so:
    if (!IsWindow(dlg->GetSafeHwnd()))
    {
        CRect dlgRect;
        getChildDlgRect(dlgRect);
        BOOL ok = 
            dlg->Create
            (
                classname.c_str(),
                L"properties",
                WS_CHILD,
                dlgRect,
                this,
                0,
                0
            );
    }
    dlg->ShowWindow(SW_SHOW);
    CString title;
    dlg->GetWindowText(title);
    CWnd *grpWnd = GetDlgItem(IDC_PROPERTIES_GRP);
    grpWnd->SetWindowText(title);
    subDlg_ = dlg;

	// Tell the window to finish creating itself
	// alexanderr: No idea why we have to call this explicitly
	// MFC crashes without it
	// better leave it as is
	// this articles explains some details about explicit WM_INITIALUPDATE
	// http://msdn.microsoft.com/en-us/library/s199bks0(v=vs.80).aspx
	subDlg_->SendMessage(WM_INITIALUPDATE, 0, 0);

	// set new active view, recalc layout and invalidate it
	MainFrame::instance()->SetActiveView( subDlg_ );
	MainFrame::instance()->RecalcLayout();

	subDlg_->Invalidate();
	// HACK
	// Remove the last subdialog
	// we have to do this after SendMessage(WM_INITIALUPDATE...
	// otherwise Particle Editor crashes if Wacom drivers are installed
	// it has something to do with OLE 
	// My guess wacom inject itself into the process and if we call
	// DestroyWindow on the old dialog too early wacom code sends
	// a notification message on destroyed window
	// sending this message triggers infinite loop
	// One way to fix it is to call oldDlg->SetParent(NULL) before
	// calling DestroyWindow
	// However this results in crash if Parent window is floating
	// current scenario works for both Wacom and floating window cases
	if (oldDlg)
	{
		oldDlg->DestroyWindow();
	}
}


/**
 *  This function adds a system offset Gizmo.
 */
void ActionSelection::addSystemOffsetGizmo()
{
	BW_GUARD;

	if (sysOffsGizmo_ != NULL)
		removeSystemOffsetGizmo();

	ParticleSystemPtr system = getCurrentParticleSystem();
	if (!system)
	{
		return;
	}

	GeneralEditorPtr generalEditor = 
		GeneralEditorPtr(new GeneralEditor(), true);
	VectorGeneratorMatrixProxy<ParticleSystem> *matrixProxy 
        =   new VectorGeneratorMatrixProxy<ParticleSystem>
            (
                &*system, 
		        &ParticleSystem::localOffset, 
		        &ParticleSystem::localOffset
            );
	generalEditor->addProperty
    (
        new GenPositionProperty
        (
            "system origin", 
            matrixProxy
        )
    );
	sysOffsGizmo_ = 
        new PositionGizmo
        (
            MODIFIER_SHIFT | MODIFIER_ALT, 
            matrixProxy, 
            NULL, 
            0.1f
        );
	GizmoManager::instance().addGizmo(sysOffsGizmo_);
	GeneralEditor::Editors newEditors;
	newEditors.push_back(generalEditor);
	GeneralEditor::currentEditors(newEditors);
}


/**
 *  This function removes the system offset Gizmo (if it exists).
 */
void ActionSelection::removeSystemOffsetGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(sysOffsGizmo_);
	sysOffsGizmo_ = NULL;
}


/**
 *  This function gets the position of the sub dialog.
 */
void ActionSelection::getChildDlgRect(CRect &rect) const
{
	BW_GUARD;

    CWnd *grpWnd = GetDlgItem(IDC_PROPERTIES_GRP);
    grpWnd->GetWindowRect(&rect);
    ScreenToClient(&rect);
    rect.InflateRect(-4, -16);
}


/**
 *  Append the current particle system if it is a one-shot particle system
 *  to an internally maintained list for display.
 */
void ActionSelection::appendOneShotPS()
{
	BW_GUARD;

	if (!isMetaParticleSystemSelected())
	{
		return;
	}

	// Go through the list of sub-systems and see if they are all one-shot.
	bool isOneShot = true;
	MetaParticleSystemPtr mps = getMetaParticleSystem();
	MetaParticleSystem::ParticleSystems &systems = mps->systemSet();
	MetaParticleSystem::ParticleSystems::iterator it = systems.begin();

	for (; (it != systems.end()) && isOneShot; ++it)
	{
		ParticleSystemPtr system = *it;
		ParticleSystem::Actions &actions = system->actionSet();
		ParticleSystem::Actions::iterator it2 = actions.begin(); 
	 
		for (; (it2 != actions.end()) && isOneShot; ++it2)
		{
			ParticleSystemActionPtr action = *it2;
	        
			if (action->typeID() == PSA_SOURCE_TYPE_ID)
			{
				SourcePSA *sourcePSA = static_cast<SourcePSA *>(&*action);
				if (sourcePSA->timeTriggered())
				{
					isOneShot = false;
        		}
    		}
		}
	}

	if (isOneShot)
	{	
		MetaParticleSystemPtr copyMPS = mps->clone();
		appendedPS_.push_back(copyMPS);
		copyMPS->clear();
		copyMPS->spawn();
	}
}


/**
 *  Clear the appended particle systems.
 */
void ActionSelection::clearAppendedPS()
{
	BW_GUARD;

    appendedPS_.clear();
}


size_t ActionSelection::numberAppendPS() const
{
    return appendedPS_.size();
}


MetaParticleSystem &ActionSelection::getAppendedPS(size_t idx)
{
	BW_GUARD;

    return *appendedPS_[idx];
}


void ActionSelection::cleanupAppendPS()
{
	BW_GUARD;

    for (int i = (int)appendedPS_.size() - 1; i >= 0; --i)
    {
        int numParticles = 0;
        MetaParticleSystemPtr mps = appendedPS_[i];
        MetaParticleSystem::ParticleSystems &systems = mps->systemSet();
        for 
        (
            MetaParticleSystem::ParticleSystems::iterator it = systems.begin();
            it != systems.end() && numParticles == 0;
            ++it
        )
        {
            ParticleSystemPtr system = *it;
            numParticles += system->size();
        }
        if (numParticles == 0)
            appendedPS_.erase(appendedPS_.begin() + i);
    }
}

/**
 *  This reselects the current particle system and expands it
 *  as undo/redo was collapsing and setting an invalid checkbox state.
 */
void ActionSelection::refreshSelection() 
{
	BW_GUARD;

	setSelectedMetaNode(selection_);
	if (selection_)
	{
		particleTree_.Expand(*selection_, TVE_EXPAND);
	}
}

/**
 *  This function initialises the component list.
 */
void ActionSelection::initComponentList()
{
	BW_GUARD;

    // Clear the list:
    componentList_.ResetContent();
    // Add the components:
    for (int i = 1; i < PSA_TYPE_ID_MAX; i++)
    {
        BW::string compName = ParticleSystemAction::typeToName(i);
        componentList_.AddString(bw_utf8tow( compName ).c_str());
    }
    // Select the first item:
    componentList_.SetCurSel(0);
    // Enable the tooltip callback:
    componentList_.setTooltipCallback(actionDescriptionTTCB);
}


/**
 *  This function is for DDX/DDV support.
 * 
 *  @param dx           The data to exchange.
 */
/*virtual*/ void ActionSelection::DoDataExchange(CDataExchange *dx)
{
	BW_GUARD;

    CDialog::DoDataExchange(dx);

    DDX_Control(dx, IDC_PARTICLETREE         , particleTree_ );
    DDX_Control(dx, IDC_COMPONENTLIST        , componentList_);
    DDX_Control(dx, IDC_ADD_COMP             , addActBtn_    );
    DDX_Control(dx, ID_FILE_SVEPARTICLESYSTEM, saveBtn_      );
    DDX_Control(dx, ID_FILE_OPEN             , openBtn_      );
}


/**
 *  This function fixes defect MSDN::q167960
 *  
 *  @param msg          The message.
 *  @returns            FALSE if the message was used and no further 
 *                      processing is necessary.
 */
/*virtual*/ BOOL ActionSelection::PreTranslateMessage(MSG *msg)
{
	BW_GUARD;

    // RETURN and ESCAPE are not handled correctly by CTreeCtrl.  We hack
    // around this by screening out these keypresses and passing it to the
    // tree control as appropriate.
    if 
    (
        msg->message == WM_KEYDOWN 
        &&
        (
            msg->wParam == VK_RETURN
            ||
            msg->wParam == VK_ESCAPE
        )
    )
    {
        CEdit *edit = particleTree_.GetEditControl();
        if (edit != NULL)
        {
            TreeNode *tn = particleTree_.GetSelectedNode();
            edit->SendMessage(msg->message, msg->wParam, msg->lParam);
            if (tn != NULL)
                particleTree_.SetSelectedNode(tn);
            return TRUE;
        }
    }
    UpdateDialogControls(this, FALSE);
    CALL_TOOLTIPS(msg);
    return CDialog::PreTranslateMessage(msg);
}


/**
 *  This function is called when a selection in the particle tree is made.
 * 
 *  @param nmhdr        The notification data.
 *  @param result       The return code.
 */
void ActionSelection::OnPartTreeSel(NMHDR *nmhdr, LRESULT *lresult)
{
	BW_GUARD;

    // Ignore fake selections due to large operations done on the tree
    if (!particleTree_.InLargeOperation())
    {
        TreeNode   *node       = particleTree_.GetSelectedNode();
        MetaNode   *mnode      = dynamic_cast<MetaNode   *>(node);
        PSNode     *psNode     = dynamic_cast<PSNode     *>(node);
        ActionNode *actionNode = dynamic_cast<ActionNode *>(node);

        // Prompt for changes etc
        if (updateSelection(node))
        {
            // Update the action/component dialog:
            OnSelectComponent(node);
        }

        if (filterFocusSelection_ == 0)
        {
            // Make the particle tree the focused window:
            particleTree_.SetFocus();
        }

        // We processed this notification:
        if (lresult != NULL)
            *lresult = 1;
    }
    else
    {
        // This was ignored.
        if (lresult != NULL)
            *lresult = 0;
    }
}


BOOL ActionSelection::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result)
{
	BW_GUARD;

    // Allow top level routing frame to handle the message
    if (GetRoutingFrame() != NULL)
        return FALSE;

    // Need to handle UNICODE versions of the message
    TOOLTIPTEXTW *pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    CString cstTipText;
    CString cstStatusText;

    UINT_PTR nID = pNMHDR->idFrom;
    if 
    (
        pNMHDR->code == TTN_NEEDTEXTW 
        && 
        (pTTTW->uFlags & TTF_IDISHWND)
    )
    {
        // idFrom is actually the HWND of the tool
        nID = ((UINT)(WORD)::GetDlgCtrlID((HWND)nID));
    }

    if (nID != 0) // will be zero on a separator
    {
		MF_ASSERT( nID <= UINT_MAX );
        cstTipText.LoadString( (uint) nID);
        cstStatusText.LoadString( (uint) nID);
    }

	wcsncpy( pTTTW->szText, cstTipText.GetString(), ARRAY_SIZE( pTTTW->szText ) );
    *result = 0;

    // bring the tooltip window above other popup windows
    ::SetWindowPos
    (
        pNMHDR->hwndFrom, 
        HWND_TOP, 
        0, 
        0, 
        0, 
        0,
        SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE
    );

    return TRUE;    // message was handled
}


void ActionSelection::setSelectedMetaNode(TreeNode *newSel)
{
	BW_GUARD;

	if ( selection_ != NULL )
	{
		MetaNode *metaNode = getMetaNode(selection_);
		if ( metaNode != NULL )
		{
			particleTree_.SetCheck( *metaNode, BST_UNCHECKED );
		}
	}
    selection_ = newSel;
	if ( selection_ != NULL )
	{
		MetaNode *metaNode = getMetaNode(selection_);
		if ( metaNode != NULL )
		{
			particleTree_.SetCheck( *metaNode, BST_CHECKED );
			particleTree_.updateCheckBoxes( metaNode );
		}
	}
}


/**
 *  This function tries to set a new meta-particle system as being the selected 
 *  one.   The user is prompted to save the current system if needs be.
 *  
 *  @param newsel       The newly selected node.
 *  @returns            True if the user did not need to save or the user
 *                      saved or decided not to save, false if the user
 *                      needed to save and he pressed cancel.
 */
bool ActionSelection::updateSelection(TreeNode *newsel)
{
	BW_GUARD;

    // See if there was a change in selected metanode:
    MetaNode *oldMN = getMetaNode(selection_);
    MetaNode *newMN = getMetaNode(newsel);
    int ok = IDYES;
    if (oldMN != newMN)
    {
        if (!filterPromptSave_ && oldMN != NULL)
            ok = MainFrame::instance()->PromptSave(MB_YESNOCANCEL, true);
        if (ok == IDYES)
        {
            clearAppendedPS();
			particleTree_.ExpandNode(oldMN, TVE_COLLAPSE);
			if (newsel)
			{
				particleTree_.Expand(*newsel, TVE_EXPAND);
			}
			setSelectedMetaNode(newsel);
            ParticleEditorApp::instance().setState
            (
                ParticleEditorApp::PE_STOPPED
            );
            ParticleEditorApp::instance().setState
            (
                ParticleEditorApp::PE_PLAYING
            );
            GUI::Manager::instance().update();
			GizmoManager::instance().removeAllGizmo();
        }
        else if (ok == IDNO)
        {
            // Add a new node corresponding to the original file and destroy
            // the old one:
            MetaParticleSystemPtr mps = new MetaParticleSystem();
            mps->load(oldMN->GetFilename(), oldMN->GetDirectory());
            MetaNode *newNode = 
                addMetaParticleSystem
                (
                    mps,
                    oldMN,
                    false,
                    false,
                    oldMN->GetLabel(),
                    true,
					oldMN->IsReadOnly()
                );
            filterPromptSave_ = true;
            TreeNode *sel = newNode->CopyExpandState(oldMN);
            removeTreeNode(oldMN, false);
			particleTree_.ExpandNode(oldMN, TVE_COLLAPSE);
			if (newsel)
			{
				particleTree_.Expand(*newsel, TVE_EXPAND);
			}
            particleTree_.SetSelectedNode(newsel);
            filterPromptSave_ = false;
			setSelectedMetaNode(newsel);
            clearAppendedPS();
        }
        else // ok = IDCANCEL
        {
            particleTree_.SetSelectedNode(selection_);
        }
    }
    else
	{
		setSelectedMetaNode(newsel);
    }
    if (newMN != NULL)
        MainFrame::instance()->SetDocumentTitle(newMN->GetFilename());
    else
        MainFrame::instance()->SetDocumentTitle("");
    return ok != IDCANCEL;
}


/**
 *  This function is called when a label is edited in the particle tree.
 * 
 *  @param nmhdr        The notification data.
 *  @param result       The return code.
 */
void ActionSelection::OnPartTreeEditLabel(NMHDR *nmhdr, LRESULT *result)
{
	BW_GUARD;

    NMTVDISPINFO *dispInfo = (NMTVDISPINFO*)nmhdr;
    if (dispInfo->item.pszText != NULL)
    {
        TreeNode *node  = particleTree_.Find(dispInfo->item.hItem);
        if (node == NULL)
            return;

        BW::string newName = bw_wtoutf8( dispInfo->item.pszText );
        RenameResult renameResult = checkUniqueName(node, newName);
        switch (renameResult)
        {
        case NAME_OK:
            {
            MainFrame::instance()->PotentiallyDirty
            (
                true,
                UndoRedoOp::AK_NPARAMETER,
                LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/RENAME")
            );
            MetaNode *mnode = dynamic_cast<MetaNode *>(node);
            if (mnode != NULL)
            {
                mnode->onRename();
                MainFrame::instance()->SetDocumentTitle(mnode->GetFilename(newName));
            }
            node->SetLabel(newName);
            }
            *result = TRUE;
            break;
        case NAME_INVALID_CHAR_INC_SPACE:
			{
			BW::wstring msg = Localise(controls::loadStringW(IDS_INVALIDFILENAME).c_str());
            AfxMessageBox(msg.c_str(), MB_OK);
            particleTree_.EditLabel(dispInfo->item.hItem);
            *result = FALSE;
			}
            break;
        case NAME_INVALID_CHAR:
			{
			BW::wstring msg = Localise(controls::loadStringW(IDS_INVALIDNAME).c_str());
            AfxMessageBox(msg.c_str(), MB_OK);
            particleTree_.EditLabel(dispInfo->item.hItem);
            *result = FALSE;
			}
            break;
        case NAME_NOT_UNIQUE:
			{
			BW::wstring text = Localise(controls::loadStringW(IDS_NOTUNIQUENAME).c_str());
			AfxMessageBox(text.c_str(), MB_OK);
            particleTree_.EditLabel(dispInfo->item.hItem);
            *result = FALSE;
			}
            break;
        }
    }
}

/**
 *	This function is called when a checkbox is used.
 *
 *  @param nmhdr        The notification data.
 *  @param result       The return code.
 */

void ActionSelection::OnNMClickParticletree(NMHDR *nmhdr, LRESULT *result)
{
	BW_GUARD;

	POINT point;
	::GetCursorPos( &point );
	particleTree_.ScreenToClient(&point);

	UINT htFlags = 0;

	TreeNode* node = particleTree_.HitTest(point, &htFlags);	
	
	if (node && ( htFlags & TVHT_ONITEMSTATEICON ) )
	{
		//check to see if we have changed particle systems
		MetaNode *oldMN = getMetaNode(particleTree_.GetSelectedNode());
		MetaNode *newMN = getMetaNode(node);
		bool changedParticle = (oldMN != newMN);

		particleTree_.SetSelectedNode( node );
		if (changedParticle)
		{
			particleTree_.SetCheck(*newMN, false);
			return;
		}

		
		bool boxState = particleTree_.GetCheck( *node ) == BST_UNCHECKED;

		// MetaNode's state is reflected in the children
        MetaNode *metaNode = dynamic_cast<MetaNode*>(node);
		if ( metaNode != NULL )
		{
			// Make sure the children have been created so we can pass the message down.
			metaNode->OnExpand();
			for ( size_t i = 0; i < particleTree_.NumberChildren( metaNode ); i++ )
			{
				PSNode* child = dynamic_cast<PSNode*>( particleTree_.GetChild( metaNode, i ) );
				MF_ASSERT( child != NULL );
				child->getParticleSystem()->enabled( boxState );
				particleTree_.SetCheck( *child, boxState ? BST_CHECKED : BST_UNCHECKED );
			}
		}

		// Disable Particle Systems from being shown
        PSNode *psNode = dynamic_cast<PSNode*>(node);
		if ( psNode != NULL )
		{
			ParticleSystemPtr ps = psNode->getParticleSystem();
			TreeNode *parent = particleTree_.GetParent( psNode );

			// If the control key is down then disable everyone but psNode 
			if ( ( GetAsyncKeyState( VK_LCONTROL ) < 0 ) || ( GetAsyncKeyState( VK_RCONTROL ) < 0 ) ) 
			{
				// first check to see if any other nodes are enabled. We do this to see if we should
				// be enabling this node and disabling others or vice versa
				bool anyEnabled = false;
				for ( size_t i = 0; i < particleTree_.NumberChildren( parent ); i++ )
				{
					PSNode* child = dynamic_cast<PSNode*>( particleTree_.GetChild( parent, i ) );
					if ( psNode != child )
					{
						if ( child->getParticleSystem()->enabled() )
						{
							anyEnabled = true;
							break;
						}
					}
				}
				
				// Special Case: if everything is off
				if ( !(anyEnabled || psNode->getParticleSystem()->enabled()) )
					anyEnabled = true;

				for ( size_t i = 0; i < particleTree_.NumberChildren( parent ); i++ )
				{
					PSNode* child = dynamic_cast<PSNode*>( particleTree_.GetChild( parent, i ) );
					if ( psNode == child )
					{
						psNode->getParticleSystem()->enabled( anyEnabled );
					}
					else
					{
						child->getParticleSystem()->enabled( !anyEnabled );
					}
					// Windows toggles the button state after this function so 
					// set both the psNode and children to the same state
					// as psNode will be toggled straight after.
					particleTree_.SetCheck( *child, anyEnabled ? BST_UNCHECKED : BST_CHECKED );

				}
			}
			else			
				ps->enabled( boxState );
			
			// If we have all particle systems disabled then we might as well disable the metaPS box
			// and if all of them are enabled then we enable the metaPS box
			bool allDisabled = true;
			for ( size_t i = 0; i < particleTree_.NumberChildren( parent ); i++ )
			{
				PSNode* child = dynamic_cast<PSNode*>( particleTree_.GetChild( parent, i ) );
				if ( child->getParticleSystem()->enabled() )
				{
					allDisabled = false;
					break;
				}
			}
			if ( allDisabled )
				particleTree_.SetCheck( *parent, BST_UNCHECKED );
			else 
				particleTree_.SetCheck( *parent, BST_CHECKED );
			// TODO: get the INDETERMINATE state working
			//else
			//	particleTree_.SetCheck( *parent, BST_INDETERMINATE );
		}

		// Disable the effects of particle system actions on the system
        ActionNode *actionNode = dynamic_cast<ActionNode*>(node);
		if ( actionNode != NULL )
		{
			// Special Case: Make sure you cannot turn off System Prop or Render Prop checkbox
			if ( ( actionNode->getActionType() != ActionNode::AT_SYS_PROP  ) &&
				 ( actionNode->getActionType() != ActionNode::AT_REND_PROP ) )
			{
				ParticleSystemActionPtr psa = actionNode->getAction();
				psa->enabled( boxState );
			}
		}
	}

	*result = 0;
}

/**
 *  This function is called when an action is selected.
 * 
 *  @param node         The node selected.
 */
void ActionSelection::OnSelectComponent(TreeNode *node)
{
	BW_GUARD;

    // Remove the gizmo:
    removeSystemOffsetGizmo();

    // Update the action dialog:
     ActionNode *actionNode = dynamic_cast<ActionNode *>(node);
    if (actionNode == NULL)
    {
		MetaNode* metaNode = dynamic_cast<MetaNode *>(node);

        MainFrame::instance()->ChangeToActionPropertyWindow
        (
			metaNode ? ActionNode::AT_META_PS : ActionNode::AT_PS,
            NULL
        );
    }
    else
    {
        // System or renderer properties case:
        if (actionNode->getActionType() != ActionNode::AT_ACTION)
        {
            MainFrame::instance()->ChangeToActionPropertyWindow
            (
                (int)actionNode->getActionType(), 
                NULL
            );
        }
        // An actual component:
        else
        {
            ParticleSystemActionPtr action = actionNode->getAction();
            MainFrame::instance()->ChangeToActionPropertyWindow
            (
                action->typeID(), 
                action
            );
        }
    }
}


/**
 *  This function is called when a particle system should be added.
 */
void ActionSelection::OnAdd()
{
	BW_GUARD;

    TreeNode   *sel   = particleTree_.GetSelectedNode();
    MetaNode   *mnode = dynamic_cast<MetaNode   *>(sel);
    PSNode     *pnode = dynamic_cast<PSNode     *>(sel);
    ActionNode *anode = dynamic_cast<ActionNode *>(sel);

    // Add a new meta-particle system?
    if (mnode != NULL || sel == NULL)
    {
        int id = MainFrame::instance()->PromptSave(MB_YESNOCANCEL, true);
        if (id != IDCANCEL)
        {
            // Create a new meta-particle system:
            MetaParticleSystemPtr newsystem = new MetaParticleSystem();
			newsystem->populateMetaData( NULL );
			MetaData::updateCreationInfo( newsystem->metaData() );
            addMetaParticleSystem(newsystem);
        }
        return;
    }

    // Add a new particle system?
    if (pnode != NULL)
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_NPARAMETER,
            LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/ADD_ACTION")
        );
        // Find the selected meta-particle system
        mnode = dynamic_cast<MetaNode *>(pnode->GetParent());
        if (mnode == NULL)
            return;

        // Add a new particle system.
        ParticleSystemPtr particleSys = generateNewParticleSystem();
        addParticleSystem(particleSys, mnode);
        return;
    }

    // Is it an action node?
    if (anode != NULL)
    {
        OnAddAction();
    }
}


/**
 *  This function is called when the user presses copy.
 */
void ActionSelection::OnCopy()
{
	BW_GUARD;

    TreeNode   *sel      = particleTree_.GetSelectedNode();
    MetaNode   *metanode = dynamic_cast<MetaNode  *>(sel);
    PSNode     *psnode   = dynamic_cast<PSNode    *>(sel);
    ActionNode *anode    = dynamic_cast<ActionNode*>(sel);
    bool       done      = false;

    if (sel == NULL)
        return;

    // Is it a meta-particle system that we need to copy?    
    if (metanode != NULL)
    {
        metanode->EnsureLoaded();
		BW::string label = metanode->GetLabel();
        int id = MainFrame::instance()->PromptSave(MB_YESNOCANCEL, true);
        if (id != IDCANCEL)
        {
            MetaParticleSystemPtr oldsystem = metanode->GetMetaParticleSystem();
            if (oldsystem != NULL)
            {
                MetaParticleSystemPtr newsystem = oldsystem->clone();
				MetaData::updateCreationInfo( newsystem->metaData() );
                addMetaParticleSystem
                (
                    newsystem, 
                    NULL, 
                    true, 
                    false, 
                    label
                );
                done = true;
            }
        }
		if (id == IDNO)
        {
            // Add a new node corresponding to the original file and destroy
            // the old one:
            MetaParticleSystemPtr mps = new MetaParticleSystem();
            mps->load(metanode->GetFilename(), metanode->GetDirectory());
			MetaData::updateCreationInfo( mps->metaData() );
            MetaNode *newNode = 
                addMetaParticleSystem
                (
                    mps,
                    metanode,
                    false,
                    false,
                    metanode->GetLabel(),
                    true,
					metanode->IsReadOnly()
                );
            newNode->CopyExpandState(metanode);
            removeTreeNode(metanode, false);
        }
    }

    // Is it a particle system that we should copy?
    if (psnode != NULL)
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_NPARAMETER,
            LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/DUPLICATE")
        );
        metanode = dynamic_cast<MetaNode *>(psnode->GetParent());
        if (metanode == NULL)
            return;
        ParticleSystemPtr oldps = psnode->getParticleSystem();
        ParticleSystemPtr newps = oldps->clone();
        addParticleSystem(newps, metanode, NULL, true, false, oldps->name()); 
        done = true;
    }

    // is it an action node that we should copy?
    if (anode != NULL)
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_NPARAMETER,
            LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/DUPLICATE")
        );
        psnode = dynamic_cast<PSNode *>(anode->GetParent());
        if (psnode == NULL)
            return;
        ParticleSystemActionPtr oldpsa = anode->getAction();
        if (oldpsa == NULL)
            return;
        ParticleSystemActionPtr newpsa = oldpsa->clone();
        addAction(newpsa, psnode);
        done = true;
    }
}


/**
 *  This function is called when something should be deleted.
 */
void ActionSelection::OnDelete()
{
	BW_GUARD;

    TreeNode *sel  = particleTree_.GetSelectedNode();
    if (sel == NULL)
        return;
    bool canDelete = true;
    bool isMNode   = false;
    if (sel->DeleteNeedsConfirm())
    {
        UINT id = IDS_CONFIRMDELETE_COMP;
        if (dynamic_cast<MetaNode *>(sel) != 0)
        {
            id = IDS_CONFIRMDELETE_PS;
            isMNode = true;
        }
		BW::wstring text = Localise(controls::loadStringW( id ).c_str() );
		int result = AfxMessageBox(text.c_str(), MB_YESNO|MB_ICONSTOP);
        if (result != IDYES)
            canDelete = false;
    }
    if (canDelete)
    {
        if (isMNode)
        {
            MainFrame::instance()->PotentiallyDirty(false);
        }
        else
        {
            MainFrame::instance()->PotentiallyDirty
            (
                true,
                UndoRedoOp::AK_NPARAMETER,
                LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/DELETE")
            );
        }
        selection_ = NULL; // This is no longer valid
        removeTreeNode(sel, true);
        clearAppendedPS();
    }
    if (filterFocusSelection_ == 0)
    {
        particleTree_.SetFocus();
    }
}


/**
 *  This function is called when an action should be added.
 */
void ActionSelection::OnAddAction()
{
	BW_GUARD;

    // Find the selected particle system and particle system node:
    TreeNode   *sel     = particleTree_.GetSelectedNode();
    ActionNode *actnode = dynamic_cast<ActionNode *>(sel);
    PSNode     *psnode  = dynamic_cast<PSNode *>(sel);
    if (actnode != NULL)
    {
        psnode = dynamic_cast<PSNode *>(sel->GetParent());
    }

    // Just in case there was no appropriate selection:
    if (psnode == NULL)
        return;

    // The id of the action:
    int selIdx = componentList_.GetCurSel();
    if (selIdx == CB_ERR)
        return;
    int id = actionDescriptions[selIdx].second;

    // Update undo/redo:
    MainFrame::instance()->PotentiallyDirty
    (
        true,
        UndoRedoOp::AK_NPARAMETER,
        LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/ADD_ACTION")
    );

    // Create the new action:
    ParticleSystemActionPtr action = ParticleSystemAction::createActionOfType(id);
    fixupAction(action, psnode->getParticleSystem());
    addAction(action, psnode);
}


/**
 *  This function is called when the open button is pressed.
 */
void ActionSelection::OnOpen()
{
	BW_GUARD;

    ParticleEditorApp *app = (ParticleEditorApp *)AfxGetApp();
    if (app != NULL)
	{
        app->OnDirectoryOpen();
		searchFilter_.clearFilter();
	}
}


/**
 *  This function is called when the save button is pressed.
 */
void ActionSelection::OnSave()
{
	BW_GUARD;

    ParticleEditorApp *app = (ParticleEditorApp *)AfxGetApp();
    if (app != NULL)
        app->OnFileSaveParticleSystem();
}


/**
 *  This function is called to open the folder containg the selected particle 
 *  system.
 */
void ActionSelection::OnOpenInExplorer()
{
	BW_GUARD;

    MetaNode *node = getMetaNode(selection_);
    if (node == NULL)
        return;
    BW::wstring fullpath = bw_utf8tow( node->GetFullpath() );
    StringUtils::replace(fullpath, BW::wstring(L"/"), BW::wstring(L"\\"));
    BW::wstring cmd      = L"explorer /select,\"" + fullpath + L"\"";
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
	wchar_t cmdline [ 32768 ];
	wcsncpy( cmdline, cmd.c_str(), ARRAY_SIZE( cmdline ) );
    GetStartupInfo(&si);
    if 
    (
        CreateProcess
        (
            NULL,                   // app. name
            cmdline,				// command line
            NULL,                   // process attributes
            NULL,                   // thread attributes
            FALSE,                  // handle inheritance option
            0,                      // creation flags
            NULL,                   // environment block
            0,                      // current directory
            &si,                    // startup info
            &pi                     // process info
        )
    )
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}


/**
 *  This function is called to copy the path of the selected particle system to 
 *  the clipboard.
 */
void ActionSelection::OnCopyPath()
{
	BW_GUARD;

    MetaNode *node = getMetaNode(selection_);
    if (node == NULL)
        return;
    BW::wstring fullpath = bw_utf8tow( node->GetFullpath() );
    StringUtils::replace(fullpath, BW::wstring(L"/"), BW::wstring(L"\\"));
    StringUtils::copyToClipboardT(fullpath);
}


/**
 *  This function is called to determine whether the add toolbar button is 
 *  enabled.
 * 
 *  @param cmdui         The update information.
 */
void ActionSelection::OnAddEnabled(CCmdUI *cmdui)
{
	BW_GUARD;

    // To be enabled there should be a PSNode in the selections parent
    // chain.
    bool enabled = true;
   
	ActionNode *anode = dynamic_cast<ActionNode *>(selection_);
	if (anode != NULL)
    {
		ActionNode::ActionType actionType = anode->getActionType();
		if (actionType == ActionNode::AT_SYS_PROP ||
			actionType == ActionNode::AT_REND_PROP ||
			(actionType == ActionNode::AT_ACTION && anode->getAction() != NULL))
		{
			PSNode * psnode = dynamic_cast< PSNode * >( anode->GetParent() );

			int selIdx = componentList_.GetCurSel();
			if (selIdx != CB_ERR)
			{
				int id = actionDescriptions[ selIdx ].second;
				enabled = canInsertAction( psnode, id );
			}
		}
    }
    cmdui->Enable(enabled ? TRUE : FALSE);
}


/**
 *  This function is called to determine whether the copy toolbar button is 
 *  enabled.
 * 
 *  @param cmdui         The update information.
 */
void ActionSelection::OnCopyEnabled(CCmdUI *cmdui)
{
	BW_GUARD;

    // To be enabled we need to have something selected that isn't a
    // renderer or system properties.
    bool enabled = true;
    if (selection_ == NULL)
    {
        enabled = false;
    }
    else
    {
        ActionNode *anode = dynamic_cast<ActionNode *>(selection_);
        if (anode != NULL && anode->getAction() != NULL)
        {
            PSNode *psnode = dynamic_cast<PSNode *>(anode->GetParent());
            if (!canInsertAction(psnode, anode->getAction()->typeID()))
                enabled = false;
        }
        if (anode != NULL && anode->getActionType() != ActionNode::AT_ACTION)
        {
            enabled = false;
        }
        MetaNode *mnode = dynamic_cast<MetaNode *>(selection_);
        if (mnode != NULL)
        {
            mnode->EnsureLoaded();
            if (mnode->GetMetaParticleSystem() == NULL)
                enabled = false;
        }
    }
    cmdui->Enable(enabled ? TRUE : FALSE);
}


/**
 *  This function is called to determine whether the delete toolbar button is 
 *  enabled.
 * 
 *  @param cmdui         The update information.
 */
void ActionSelection::OnDeleteEnabled(CCmdUI *cmdui)
{
	BW_GUARD;

    // To be enabled we need to have something selected that isn't a
    // renderer or system properties.
    bool enabled = true;
    if (selection_ == NULL)
    {
        enabled = false;
    }
    else
    {
        ActionNode *anode = dynamic_cast<ActionNode *>(selection_);
        if (anode != NULL && anode->getActionType() != ActionNode::AT_ACTION)
            enabled = false;
        MetaNode *mnode = dynamic_cast<MetaNode *>(selection_);
        if (mnode != NULL)
        {
            mnode->EnsureLoaded();
            if (mnode->GetMetaParticleSystem() == NULL)
                enabled = false;
        }
    }
    cmdui->Enable(enabled ? TRUE : FALSE);
}


/**
 *  This function is called to determine whether the add action button is 
 *  enabled.
 * 
 *  @param cmdui         The update information.
 */
void ActionSelection::OnAddActionEnabled(CCmdUI *cmdui)
{
	BW_GUARD;

    // To be enabled there should be a PSNode in the selections parent
    // chain.
    bool enabled = false;
    TreeNode *node = selection_;
    while (node != NULL)
    {
        PSNode *psnode = dynamic_cast<PSNode *>(node);
        if (psnode !=  NULL)
        {
            int selIdx = componentList_.GetCurSel();
            if (selIdx != CB_ERR)
            {
                int id = actionDescriptions[selIdx].second;
                enabled = canInsertAction(psnode, id);
            }
            break;
        }
        node = node->GetParent();
    }
    cmdui->Enable(enabled ? TRUE : FALSE);
}


/**
 *  This function is called when the filter changes.
 */
void ActionSelection::OnEditFilter()
{
	BW_GUARD;

    ++filterFocusSelection_;
    CString ftxt;
    searchFilter_.GetWindowText(ftxt);
    particleTree_.SetFilter(bw_wtoutf8( ftxt.GetString() ));
    updateSelection(particleTree_.GetSelectedNode());
    --filterFocusSelection_;
}


/**
 *  This function is called when the window is resized.
 * 
 *  @param type          The type of the resize.
 *  @param cx            The new width.
 *  @param cy            The new height.
 */
void ActionSelection::OnSize(UINT type, int cx, int cy)
{
	BW_GUARD;

    CDialog::OnSize(type, cx, cy);
    
    // Fit the properties group:
    CWnd * propGrp = GetDlgItem(IDC_PROPERTIES_GRP);
    if (::IsWindow(propGrp->GetSafeHwnd()))
    {
        CRect actRect;
        actionBar_.GetWindowRect(&actRect);
        ScreenToClient(&actRect);
        CRect grpRect(4, actRect.bottom + 4, cx - 4, cy - 4);
        propGrp->SetWindowPos
        (
            NULL,
            grpRect.left,
            grpRect.top,
            grpRect.Width(),
            grpRect.Height(),
            SWP_NOZORDER
        );
    }
    // Fit the child dialog:
    if (subDlg_ != NULL)
    {
        CRect rect;
        getChildDlgRect(rect);
        subDlg_->SetWindowPos
        (
            0,
            rect.left,
            rect.top,
            rect.Width(),
            rect.Height(),
            SWP_NOZORDER
        );
    }
}


/**
 *  This function is called during window closure.
 */
void ActionSelection::OnClose()
{
	BW_GUARD;

	MainFrame::instance()->SetActiveView( NULL );
	subDlg_->DestroyWindow();
}


/**
 *  This function is called when the delete key was pressed on the tree 
 *  control.
 * 
 *  @param wparam       The WPARAM of the message (not used).
 *  @param lparam       The LPARAM of the message (not used).    
 *	@returns			0.
 */
LRESULT ActionSelection::OnNodeDelete(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	BW_GUARD;

    OnDelete();
    return 0;
}


/**
 *	This is called when the panels start to close down.
 *
 *  @param wparam       The WPARAM of the message (not used).
 *  @param lparam       The LPARAM of the message (not used).    
 *	@returns			0.
 */
LRESULT ActionSelection::OnClosing(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	BW_GUARD;

	appendedPS_.clear();
	return 0;
}


/**
 *  This function generates a unique name for the new particle system.
 * 
 *  @param ms            The meta-particle system.
 *  @param name          The prototype name.
 *  @param nameIsNew     The name is new, as opposed to being based on an
 *                       existing particle system.
 *  @returns             A unique name for a new particle system.
 */
BW::string 
ActionSelection::generateUniquePSName
(
    MetaParticleSystemPtr   ms,
    BW::string                  const &name_,
    bool                    nameIsNew
) const
{
	BW_GUARD;

    if (ms == NULL)
        return BW::string();

    BW::string name = name_;
    while (true)
    {
        // See if this name is already used:
        size_t i = 0;
        for (; i < ms->systemSet().size(); ++i)
        {
            ParticleSystemPtr ps =ms->systemSet()[i];
            if (strcmpi(ps->name().c_str(), name.c_str()) == 0)
                break;
        }
        if (i == ms->systemSet().size())
            break;
        // Generate a potentially new name:
        StringUtils::incrementT
        (
            name, 
            nameIsNew 
                ? StringUtils::IS_END 
                : StringUtils::IS_EXPLORER
        );
    }

    return name;
}


/**
 *  This function generates a unique name for an action of type id for the 
 *  particle system.
 * 
 *  @param ps            The particle system's node.
 *  @param id            The id of the new action.
 *  @returns             A unique name for the action.
 */
BW::string ActionSelection::generateUniqueActionName(PSNode *ps, int id) const
{
	BW_GUARD;

    if (ps == NULL)
        return BW::string();

    BW::string name = ParticleSystemAction::typeToName(id) + " 1";
    while (true)
    {
        // See if the name is used by the particle system:
        size_t i = 0;
        for (; i < ps->NumberChildren(); ++i)
        {
            ActionNode *an = dynamic_cast<ActionNode *>(ps->GetChild(i));
            if (an != NULL && name == an->GetLabel())
                break;
        }
        if (i == ps->NumberChildren())
            break;
        // Generate another potential name:
        StringUtils::incrementT(name, StringUtils::IS_END);
    }
    return name;
}


/**
 *  This function generates a unique name for a metaparticle system.
 * 
 *  @param name          The prototype name.
 *  @param nameIsNew     The name is new, as opposed to being based on an
 *                       existing meta particle system.
 *  @returns             The new, unique name.
 */
BW::string 
ActionSelection::generateUniqueMetaSystemName
(
    BW::string      const &name,
    bool        nameisNew,
    MetaNode    const *ignoreMN   /*= NULL*/
) const
{
	BW_GUARD;

    BW::string result = name;
    while (true)
    {
        // Test this name against existing ones:
        size_t i = 0;
        for (; i < particleTree_.NumberChildren(NULL); ++i)
        {
            MetaNode const *mn = 
                dynamic_cast<MetaNode const *>(particleTree_.GetChild(NULL, i));
            if (mn != NULL && mn != ignoreMN)
            {
                if (strcmpi(mn->GetLabel().c_str(), result.c_str()) == 0)
                {
                    break;
                }
            }
        }
        if (i == particleTree_.NumberChildren(NULL))
            break;
        // Generate another potential name:
        StringUtils::incrementT
        (
            result, 
            nameisNew 
                ? StringUtils::IS_END 
                : StringUtils::IS_EXPLORER
        );
        StringUtils::makeValidFilenameT(result, '_', false);
    }
    return result;
}


/**
 *  This function generates a blank particle system.
 */
ParticleSystemPtr ActionSelection::generateNewParticleSystem() const
{
	BW_GUARD;

    ParticleSystemPtr particleSys = new ParticleSystem();
	BW::string defaultTexture = Options::getOptionString( "defaults/renderer/spriteTexture", Moo::TextureManager::notFoundBmp() );
    SpriteParticleRendererPtr renderer = 
        new SpriteParticleRenderer(defaultTexture);
    renderer->viewDependent(false);
    renderer->materialFX(SpriteParticleRenderer::FX_ADDITIVE);
    particleSys->pRenderer(renderer);
    return particleSys;
}


/**
 *  This function is called when the user drags into a window.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param keyState      The state of the keys.
 *  @param point         The mouse cursor location.
 *  @returns             DROPEFFECT_NONE etc.
 */
/*virtual*/ 
DROPEFFECT
ActionSelection::OnDragEnter
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DWORD               keyState,
    CPoint              point
)
{
	BW_GUARD;

    if (window = &particleTree_)
        particleTree_.BeginDrag(1);
    return OnDragTest(window, dataObject, keyState, point);
}


/**
 *  This function is called when the user drags and leaves the window.
 *  @param window		The window dragged out of.
 */
/*virtual*/
void ActionSelection::OnDragLeave(CWnd *window)
{
	BW_GUARD;

	particleTree_.EndDrag();
}


/**
 *  This function is called when the user drags over a window.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param keyState      The state of the keys.
 *  @param point         The mouse cursor location.
 *  @returns             DROPEFFECT_NONE etc.
 */
/*virtual*/ 
DROPEFFECT
ActionSelection::OnDragOver
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DWORD               keyState,
    CPoint              point
)
{
	BW_GUARD;

    return OnDragTest(window, dataObject, keyState, point);
}
 

/**
 *  This is called during drag and drop to see what the drop effect should be.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param keyState      The state of the keys.
 *  @param point         The mouse cursor location.
 *  @returns             DROPEFFECT_NONE etc.
 */
DROPEFFECT
ActionSelection::OnDragTest
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DWORD               keyState,
    CPoint              point
)
{
	BW_GUARD;

    if (dataObject->IsDataAvailable(CF_UNICODETEXT) != FALSE)
        return OnDragTestAction(window, dataObject, keyState, point);
    else if (dataObject->IsDataAvailable(TreeControl::GetDragDropID()) != FALSE)
        return OnDragTestTree(window, dataObject, keyState, point);
    else
        return DROPEFFECT_NONE;
}


/**
 *  This does the drag-drop test to see what the effect should be when an action
 *  is dragged onto the tree control.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param keyState      The state of the keys.
 *  @param point         The mouse cursor location.
 *  @returns             DROPEFFECT_NONE etc.
 */
DROPEFFECT 
ActionSelection::OnDragTestAction
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DWORD               keyState,
    CPoint              point
)
{
	BW_GUARD;

    // Drop must be on particle tree:
    if (window != &particleTree_)
        return DROPEFFECT_NONE;

    // Drop must be on a ActionNode or a PSNode:
    TreeNode   *tn = particleTree_.HitTest(point);
    ActionNode *an = dynamic_cast<ActionNode *>(tn);
    PSNode     *pn = dynamic_cast<PSNode     *>(tn);
    if (an != NULL || pn != NULL)
    {
        if (pn == NULL)
            pn = dynamic_cast<PSNode *>(an->GetParent());
        if (canInsertAction(pn, actionDescriptions[componentList_.GetCurSel()].second))
            return DROPEFFECT_COPY;
    }
    return DROPEFFECT_NONE;
}


/**
 *  This does the drag-drop test to see what the effect should be when part of 
 *  the tree control is dragged onto the tree control.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param keyState      The state of the keys.
 *  @param point         The mouse cursor location.
 *  @returns             DROPEFFECT_NONE etc.
 */
DROPEFFECT 
ActionSelection::OnDragTestTree
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DWORD               keyState,
    CPoint              point
)
{
	BW_GUARD;

    // Drop must be on particle tree:
    if (window != &particleTree_)
        return DROPEFFECT_NONE;

    TreeNode    *dragNode   = particleTree_.GetDraggedNode();
    ActionNode  *dragANode  = dynamic_cast<ActionNode *>(dragNode);
    PSNode      *dragPSNode = dynamic_cast<PSNode     *>(dragNode);
    MetaNode    *dragMNode  = dynamic_cast<MetaNode   *>(dragNode);
    DROPEFFECT  possibleFx  = dragNode->CanDragDrop();

    UINT        flags       = 0;
    TreeNode    *hitNode    = particleTree_.HitTest(point, &flags);
    PSNode      *hitPSNode  = dynamic_cast<PSNode     *>(hitNode);
    ActionNode  *hitANode   = dynamic_cast<ActionNode *>(hitNode);
    MetaNode    *hitMNode   = dynamic_cast<MetaNode   *>(hitNode);

    // Dragged an ActionNode?
    if (dragANode != NULL)
    {
        // Was this really an action?
        if (dragANode->getAction() != NULL)
        {
            // Dropped onto another ActionNode?
            if (hitANode != NULL || hitPSNode != NULL)
            {
                if (hitPSNode == NULL)
                    hitPSNode = dynamic_cast<PSNode *>(hitANode->GetParent());
                if (canInsertAction(hitPSNode, dragANode->getAction()->typeID()))
                    return possibleFx;
            }
        }
        // Was it system or render properties?
        else
        {
            if 
            (
                hitANode != dragANode
                &&
                (
                    (
                        hitANode != NULL 
                        && 
                        dragANode->getActionType() == hitANode->getActionType()
                    )
                    ||
                    hitPSNode != NULL
                )
            )
            {
                return DROPEFFECT_COPY;
            }
        }
    }
    // Dragged a PSNode?
    else if (dragPSNode != NULL)
    {       
		// Dropped onto a MSMode?
        if (hitMNode != NULL)
        {
			hitMNode->EnsureLoaded();
			MetaParticleSystemPtr dropMPS = hitMNode->GetMetaParticleSystem();
			if (!dropMPS)
				return DROPEFFECT_NONE;
			else
				return possibleFx;
		}
        // Dropped onto another PSNode
        else if (hitPSNode != NULL)
        {
            return possibleFx;
        }
    }
    // Dragged a MetaNode?
    else if (dragMNode != NULL)
    {
        if (hitMNode != NULL)
        {
            return possibleFx;
        }
        else if ((flags & TVHT_NOWHERE) == TVHT_NOWHERE)
        {
            return possibleFx;
        }
    }
    // Must not be allowed:
    return DROPEFFECT_NONE;
}


/**
 *  This function is called when a drop occurs.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param dropEffect    DROPEFFECT_COPY etc.
 *  @param point         The mouse cursor location.
 *  @returns             TRUE if successful, FALSE if failed.
 */
BOOL
ActionSelection::OnDrop
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DROPEFFECT          dropEffect,
    CPoint              point
)
{
	BW_GUARD;

    particleTree_.EndDrag();

    if (dataObject->IsDataAvailable(CF_UNICODETEXT) != FALSE)
        return OnDropAction(window, dataObject, dropEffect, point);
    else if (dataObject->IsDataAvailable(TreeControl::GetDragDropID()) != FALSE)
        return OnDropTree(window, dataObject, dropEffect, point);
    else
        return FALSE;
}


/**
 *  This function is called when a drop occurs where the dropped item is an 
 *  action.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param dropEffect    DROPEFFECT_COPY etc.
 *  @param point         The mouse cursor location.
 *  @returns             TRUE if successful, FALSE if failed.
 */
/*virtual*/
BOOL
ActionSelection::OnDropAction
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DROPEFFECT          dropEffect,
    CPoint              point
)
{
	BW_GUARD;

    TreeNode   *tn     = particleTree_.HitTest(point);
    PSNode     *psnode = dynamic_cast<PSNode *>(tn);
    ActionNode *anode  = dynamic_cast<ActionNode *>(tn);

    if (updateSelection(tn))
    {
        // Create the action:
        BW::wstring actionWStr = DropTarget::GetText(dataObject);
		BW::string actionStr;
		bw_wtoutf8( actionWStr.c_str(), actionStr );
        int id = ParticleSystemAction::nameToType(actionStr);
        if (id == PSA_TYPE_ID_MAX)
            return FALSE;
        ParticleSystemActionPtr psa = ParticleSystemAction::createActionOfType(id);    
        if (psa == NULL)
            return FALSE;
       
        // Add to the end of a particle system?
        if (psnode && canInsertAction(psnode, id))
        {
            MainFrame::instance()->PotentiallyDirty
            (
                true,
                UndoRedoOp::AK_NPARAMETER,
                LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/ADD_ACTION")
            );
            fixupAction(psa, psnode->getParticleSystem());
            addAction(psa, psnode);
            return TRUE;
        }

        // Add after another action?
        if (anode != NULL)
        {
            psnode = dynamic_cast<PSNode *>(anode->GetParent());
            if (canInsertAction(psnode, id))
            {
                MainFrame::instance()->PotentiallyDirty
                (
                    true,
                    UndoRedoOp::AK_NPARAMETER,
                    LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/ADD_ACTION")
                );                
                fixupAction(psa, psnode->getParticleSystem());
                ActionNode *prevSibling = getActionInsert(anode);
                addAction(psa, psnode, prevSibling);
                return TRUE;
            }
        }

        // Nothing was done, cleanup and return failure:
        psa = NULL;
    }
    return FALSE;
}


/**
 *  This function is called when a drop occurs where the dropped item is part of 
 *  the tree.
 * 
 *  @param window        The window dragged into.
 *  @param dataObject    The dragged data.
 *  @param dropEffect    DROPEFFECT_COPY etc.
 *  @param point         The mouse cursor location.
 *  @returns             TRUE if successful, FALSE if failed.
 */
/*virtual*/
BOOL
ActionSelection::OnDropTree
(
    CWnd                *window,
    COleDataObject      *dataObject,
    DROPEFFECT          dropEffect,
    CPoint              point
)
{
	BW_GUARD;

    TreeNode    *dragNode   = particleTree_.GetDraggedNode();
    ActionNode  *dragANode  = dynamic_cast<ActionNode *>(dragNode);
    PSNode      *dragPSNode = dynamic_cast<PSNode     *>(dragNode);
    MetaNode    *dragMNode  = dynamic_cast<MetaNode   *>(dragNode);

    UINT        flags       = 0;
    TreeNode    *hitNode    = particleTree_.HitTest(point, &flags);
    PSNode      *hitPSNode  = dynamic_cast<PSNode     *>(hitNode);
    ActionNode  *hitANode   = dynamic_cast<ActionNode *>(hitNode);
    MetaNode    *hitMNode   = dynamic_cast<MetaNode   *>(hitNode);

    TreeNode    *newNode    = NULL;

    bool doneSomething      = false;
    bool sameParent         = false;

    // Make sure everthing is loaded:
    if (dragMNode != NULL)
        dragMNode->EnsureLoaded();
    if (hitMNode != NULL)
        hitMNode->EnsureLoaded();

    // Dragged an ActionNode?
    if (dragANode != NULL)
    {
        ParticleSystemActionPtr dragAction = dragANode->getAction();
        // Was this really an action?
        if (dragAction != NULL)
        {
            // Clone the action:
            ParticleSystemActionPtr dropAction = dragAction->clone();

            // Dropped onto another ActionNode?
            if (hitANode != NULL)
            {
                hitPSNode = dynamic_cast<PSNode *>(hitANode->GetParent());
                if (canInsertAction(hitPSNode, dropAction->typeID()))
                {
                    if (!updateSelection(hitNode))
                        return FALSE;
                    MainFrame::instance()->PotentiallyDirty
                    (
                        true,
                        UndoRedoOp::AK_NPARAMETER,
                        LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/COPY")
                    );                
                    ActionNode *prevSibling = getActionInsert(hitANode);
                    newNode = addAction(dropAction, hitPSNode, prevSibling); 
                    doneSomething = true;
                    sameParent = dragANode->GetParent() == hitANode->GetParent();
                }
            }
            // Dropped onto a PSNode?
            else if (hitPSNode != NULL)
            {
                if (canInsertAction(hitPSNode, dropAction->typeID()))
                {
                    if (!updateSelection(hitNode))
                        return FALSE;
                    MainFrame::instance()->PotentiallyDirty
                    (
                        true,
                        UndoRedoOp::AK_NPARAMETER,
                        LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/COPY")
                    );
                    newNode = addAction(dropAction, hitPSNode);
                    doneSomething = true;
                    sameParent = dragANode->GetParent() == hitNode;
                }
            }
        }
        else
        {
            PSNode *sourcePNode = 
                dynamic_cast<PSNode *>(dragANode->GetParent());
            PSNode *destPNode = NULL;
            if (hitPSNode != NULL)
                destPNode = hitPSNode;
            if (hitANode != NULL)
            {
                destPNode = dynamic_cast<PSNode *>(hitANode->GetParent());
            }
            if (sourcePNode != NULL && destPNode != NULL && sourcePNode != destPNode)
            {
                ParticleSystemPtr      sourcePS   = sourcePNode->getParticleSystem()->clone();
                ParticleSystemPtr      destPS     = destPNode  ->getParticleSystem();
                ActionNode::ActionType actionType = dragANode->getActionType();
                if (!updateSelection(hitNode))
                    return FALSE;
                MainFrame::instance()->PotentiallyDirty
                (
                    true,
                    UndoRedoOp::AK_NPARAMETER,
                    LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/COPY")
                );
                if (actionType == ActionNode::AT_SYS_PROP)
                {
                    copySystemProperties(sourcePS, destPS);
                    newNode = destPNode->GetChild(0);
                    doneSomething = true;

                }
                else if (actionType == ActionNode::AT_REND_PROP)
                {
                    copyRendererProperties(sourcePS, destPS);
                    newNode = destPNode->GetChild(1);
                    doneSomething = true;
                }
                if (doneSomething)
                {
                    OnSelectComponent(newNode);
                }
            }
        }
    }
    // Dragged a PSNode?
    else if (dragPSNode != NULL)
    {       
        ParticleSystemPtr dragPS = dragPSNode->getParticleSystem();
        ParticleSystemPtr dropPS = dragPS->clone();
        // Dropped onto another PSNode?
        if (hitPSNode != NULL)
        {
            BW::string label = dragPSNode->GetLabel();
            if (!updateSelection(hitNode))
                return FALSE;
            MainFrame::instance()->PotentiallyDirty
            (
                true,
                UndoRedoOp::AK_NPARAMETER,
                LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/COPY")
            );
            hitMNode = dynamic_cast<MetaNode *>(hitPSNode->GetParent());
            newNode =
                addParticleSystem
                (
                    dropPS, 
                    hitMNode, 
                    hitPSNode, 
                    false, 
                    false,
                    label
                );
            doneSomething = true;
            sameParent = dragPSNode->GetParent() == hitPSNode->GetParent();
        }
        // Dropped onto a MetaNode?
        else if (hitMNode != NULL)
        {
			MetaParticleSystemPtr dropMPS = hitMNode->GetMetaParticleSystem();
			if (dropMPS)
			{
				BW::string label = dragPSNode->GetLabel();
				if (!updateSelection(hitNode))
					return FALSE;
				MainFrame::instance()->PotentiallyDirty
				(
					true,
					UndoRedoOp::AK_NPARAMETER,
					LocaliseUTF8(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/COPY")
				);
				hitMNode->OnExpand();
				newNode =
					addParticleSystem
					(
						dropPS, 
						hitMNode, 
						NULL, 
						false, 
						false,
						label
					);
				doneSomething = true;
				sameParent = dragPSNode->GetParent() == hitNode;
			}
        }
    }
    // Dragged a MetaNode?
    else if (dragMNode != NULL)
    {
        MetaParticleSystemPtr dragMPS = dragMNode->GetMetaParticleSystem();
        MetaParticleSystemPtr dropMPS = dragMPS->clone();
        BW::string label = dragMNode->GetLabel();
        if (!updateSelection(hitNode))
            return FALSE;
        if (hitMNode != NULL || (flags & TVHT_NOWHERE) == TVHT_NOWHERE)
        {
            newNode =
                addMetaParticleSystem
                (
                    dropMPS, 
                    hitMNode, 
                    false,
                    false,
                    label
                ); 
            doneSomething = true;
            sameParent = true;
        }
    }

    // Select the new node.
    if (newNode != NULL)
        particleTree_.SetSelectedNode(newNode);

    return doneSomething ? TRUE : FALSE;
}


/**
 *  This function copies the system properties from one ParticleSystem to 
 *  another.
 * 
 *  @param source        The source ParticleSystem.
 *  @param dest          The destination ParticleSystem.
 */
void
ActionSelection::copySystemProperties
(
    ParticleSystemPtr   source,
    ParticleSystemPtr   dest
)
{
	BW_GUARD;

    if (source.getObject() == dest.getObject())
        return;

    dest->capacity    (source->capacity  ());
    dest->windFactor  (source->windFactor());
    dest->maxLod      (source->maxLod    ());
}


/**
 *  This function copies the renderer properties from one ParticleSystem to 
 *  another.
 * 
 *  @param source        The source ParticleSystem.
 *  @param dest          The destination ParticleSystem.
 */
void
ActionSelection::copyRendererProperties( ParticleSystemPtr source,
    ParticleSystemPtr dest )
{
	BW_GUARD;

    if (source.getObject() == dest.getObject())
	{
		return;
	}

    ParticleSystemRendererPtr sourceRenderer = source->pRenderer();
    ParticleSystemRenderer * destRenderer = sourceRenderer->clone();
	dest->pRenderer(destRenderer);
}


/**
 *  This function is called when a drag is about to start.
 * 
 *  @param wparam        The WPARAM of the message.
 *  @param lparam        The LPARAM of the message.    
 */
LRESULT ActionSelection::OnDragStart(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    pauseState_ = (int)ParticleEditorApp::instance().getState();
    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PAUSED);
    return TRUE;
}


/**
 *  This function is called when a drag is done (regardless of whether it was 
 *  successful).
 * 
 *  @param wparam        The WPARAM of the message.
 *  @param lparam        The LPARAM of the message.    
 */
LRESULT ActionSelection::OnDragDone(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    particleTree_.EndDrag();
    ParticleEditorApp::instance().setState
    (
        (ParticleEditorApp::State)pauseState_
    );
    return TRUE;
}


/**
 *  This function is called when a right-click occurs on the particle tree 
 *  control.
 * 
 *  @param wparam        The WPARAM of the message.
 *  @param lparam        The LPARAM of the message.    
 */
LRESULT ActionSelection::OnPartTreeMenu(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    if (!isMetaParticleSystemSelected())
        return TRUE;

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu
    (
        MF_ENABLED, 
        ID_OPENEXPLORER, 
		CString(Localise(L"`RCS_IDS_OPENINEXPLORER"))
    );
    menu.AppendMenu
    (
        MF_ENABLED, 
        ID_COPYPATHCLIPBOARD, 
        CString(Localise(L"`RCS_IDS_COPYPATHCLIPBOARD"))
    );
    CPoint pt;
    GetCursorPos(&pt);
    menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this, NULL);
    return TRUE;
}


/**
 *  This function removes the TreeNode from the particle tree and remove it 
 *  from the underlying model.
 * 
 *  @param tn            The TreeNode to remove.
 *  @param delFile       Delete any underlying files if the node is a 
 *                       meta-particle system.
 */
void ActionSelection::removeTreeNode(TreeNode *tn, bool delFile)
{
	BW_GUARD;

    // Delete a meta-particle system?
    MetaNode *mnode = dynamic_cast<MetaNode *>(tn);
    if (mnode != NULL)
    {
        if (delFile)
        {
            mnode->DeleteFile();
        }
        particleTree_.RemoveNode(mnode);
        return;
    }

    // Delete a particle system?
    PSNode *psnode = dynamic_cast<PSNode *>(tn);
    if (psnode != NULL)
    {
        MetaNode *mnode = dynamic_cast<MetaNode *>(psnode->GetParent());
        if (mnode == NULL)
            return;
        MetaParticleSystemPtr metasys = mnode->GetMetaParticleSystem();
        if (metasys == NULL)
            return;
        ParticleSystemPtr system = psnode->getParticleSystem();
        if (system == NULL)
            return;
        metasys->removeSystem(system);
        particleTree_.RemoveNode(psnode);
        return;
    }

    // Delete a component/action?
    ActionNode *actnode = dynamic_cast<ActionNode *>(tn);
    if (actnode != NULL)
    {
        PSNode *psnode = dynamic_cast<PSNode *>(actnode->GetParent());
        if (psnode == NULL)
            return;
        ParticleSystemPtr system = psnode->getParticleSystem();
        if (system == NULL)
            return;
        ParticleSystemActionPtr action = actnode->getAction();
        if (action == NULL)
            return;
        system->removeAction(action);
        particleTree_.RemoveNode(actnode);
        return;
    }
}


/**
 *  This function adds a MetaParticleSystem.
 *  
 *  @param mps           The new meta-particle system.
 *  @param prevSibiling  The meta-particle system to insert after.
 *  @param edit          Start editing the label?
 *  @param isNew         Is the meta-particle system new (true) or based on
 *                       an existing one (false).
 *  @param name          Suggested name.  We check for duplicates and
 *                       increment the string intelligently.
 *  @param keepName      Keep the name, or autoincrement.
 *  @returns             The new node.
 */
MetaNode * 
ActionSelection::addMetaParticleSystem
(
    MetaParticleSystemPtr   mps,
    MetaNode                *prevSibling    /*= NULL*/,
    bool                    edit            /*= true*/,
    bool                    isNew           /*= true*/,
    BW::string                  const &name     /*= "system"*/,
    bool                    keepName        /*= false*/,
	bool					readOnly		/*= false*/
)
{
	BW_GUARD;

    // Generate a new MetaNode for the system:
    BW::string newname;
    if (keepName)
        newname = name;
    else
        newname = generateUniqueMetaSystemName(name, isNew);
    MetaNode *mn =
        (MetaNode *)particleTree_.AddNode
        (
            new MetaNode(directory(), newname), 
            NULL, 
            prevSibling
        );
    mn->SetMetaParticleSystem(mps);
	mn->SetReadOnly(readOnly);

    // Add to the meta-particle system a dummy particle system if there aren't
    // any:
    if (mps->systemSet().size() == 0)
    {
        ParticleSystemPtr particleSys = generateNewParticleSystem();
        addParticleSystem(particleSys, mn, NULL, false);
        mn->FlagChildrenReady();
    }

    // Save the new node:    
	BW::string dir = directory();
    BW::string filename = mn->GetFilename();
	BW::string dirFile = dir + name + "." + BWResource::getExtension( filename );
    BW::string fullpath = BWResource::resolveFilename(dirFile);
	if ( isNew || !keepName )
		mps->save(filename, fullpath.substr( 0, fullpath.find(name) ), true ); // new node, so need only transient
    mn->onSave();

    // Select the new node:
    particleTree_.SetSelectedNode(mn);

    // Optionally edit the meta-particle system's label:
    if (edit)
        particleTree_.EditNodeLabel(mn);

    return mn;
}


/**
 *  This function adds a ParticleSystem.
 * 
 *  @param ps            The new particle system.
 *  @param parent        The parent MetaNode.
 *  @param prevSibling   The particle system to insert after.
 *  @param edit          Start editing the label?
 *  @param isNew         Is the particle system new (true) or based on an
 *                       existing one (false).
 *  @param name          Suggested name.  We check for duplicates and
 *                       increment the string intelligently.
 *  @returns             The new node.
 */
PSNode *
ActionSelection::addParticleSystem
(
    ParticleSystemPtr       ps,
    MetaNode                *parent,
    PSNode                  *prevSibling    /*= NULL*/,
    bool                    edit            /*= true*/,
    bool                    isNew           /*= true*/,
    BW::string                  const &name     /*= "component"*/
)
{
	BW_GUARD;

    // Update the model:
    MetaParticleSystemPtr metaSys = parent->GetMetaParticleSystem();
    BW::string particleSysName = generateUniquePSName(metaSys, name, isNew);
    ps->name(particleSysName);
    if (prevSibling == NULL)
    {
        metaSys->addSystem(ps);
    }
    else
    {
        ParticleSystemPtr prevSys = prevSibling->getParticleSystem();
        size_t idx;
        for (idx = 0; idx < metaSys->systemSet().size(); ++idx)
        {
            if (prevSys.getObject() == metaSys->systemSet()[idx].getObject())
                break;
        }
        if (idx != metaSys->systemSet().size())
            metaSys->insertSystem(idx + 1, ps);
        else
            metaSys->addSystem(ps);
    }

    // Update the tree control:
    PSNode *psnode = 
        (PSNode *)particleTree_.AddNode
        (
            new PSNode(ps), 
            parent, 
            prevSibling
        );
	particleTree_.SetCheck ( *psnode, ps->enabled() );
    psnode->addChildren();

    // Edit the label?
    if (edit)
    {
        particleTree_.SetSelectedNode(psnode);
        particleTree_.EditNodeLabel(psnode);
    }

    return psnode;
}


/**
 *  This function adds an Action.
 * 
 *  @param psa           The new particle system action.
 *  @param parent        The parent PSNode.
 *  @param prevSibling   The action to insert after.
 *  @returns             The new node.
 */
ActionNode *
ActionSelection::addAction
(
    ParticleSystemActionPtr  psa,
    PSNode                  *parent,
    ActionNode              *prevSibling    /*= NULL*/
)
{
	BW_GUARD;

    // Update the model:
    ParticleSystemPtr system = parent->getParticleSystem();
    if (prevSibling != NULL)
    {
        // Add after renderer properties?
        if (prevSibling->getAction() == NULL)
        {
            system->insertAction(0, psa);
        }
        else
        {
            ParticleSystemActionPtr prevAction = prevSibling->getAction();
            size_t idx;
            for (idx = 0; idx < system->actionSet().size(); ++idx)
            {
                if (system->actionSet()[idx] == prevAction)
                    break;
            }
            if (idx == system->actionSet().size())
                system->addAction(psa);
            else
                system->insertAction(idx + 1, psa);
        }
    }
    else
    {
        system->addAction(psa);
    }
    BW::string actionname = generateUniqueActionName(parent, psa->typeID());
    psa->name(actionname);

    // Update the tree control:    
    ActionNode *an = 
        (ActionNode *)particleTree_.AddNode
        (
            new ActionNode(psa, actionname), 
            parent,
            prevSibling
        );
    particleTree_.SetSelectedNode(an);

    return an;
}


/**
 *  This function returns the directory that is being edited.
 * 
 *  @returns             The directory being edited.
 */
BW::string ActionSelection::directory() const
{
	BW_GUARD;

    return bw_wtoutf8( MainFrame::instance()->ParticlesDirectory().GetString() );
}


/**
 *  This function returns the action place of insertion for an ActionNode.  
 *  This adjusts for system and render properties.
 * 
 *  @param anode         The best guess of where the node should be inserted
 *                       after.
 *  @returns             The actual location of insertion.
 */
ActionNode *ActionSelection::getActionInsert(ActionNode *anode) const
{
	BW_GUARD;

    if (anode == NULL)
        return NULL;

    PSNode *parent = dynamic_cast<PSNode *>(anode->GetParent());

    if (parent->NumberChildren() <= 2)
        return NULL; // add at end

    // Treat the system and render properties as a special case:
    if (anode->getAction() == NULL)
    {
        return dynamic_cast<ActionNode *>(parent->GetChild(1));
    }

    // We can insert after anode after all:
    return anode;
}


/**
 *  This function returns whether an action of the given type id be inserted 
 *  into the particle system in the PSNode?
 * 
 *  @param psnode        The PSNode where the action will be placed.
 *  @param actionID      The id of the new action.
 *  @returns             True if an action of the given type id can be added
 *                       to the particle system of psnode.
 */
bool ActionSelection::canInsertAction(PSNode const *node, int actionID) const
{
	BW_GUARD;

    if (node == NULL)
        return false;
    ParticleSystemPtr ps = node->getParticleSystem();
    // There is only one collider allowed:
    if (actionID == PSA_COLLIDE_TYPE_ID)
    {
        ParticleSystem::Actions &actions = ps->actionSet();
        for
        (
            ParticleSystem::Actions::iterator it = actions.begin();
            it != actions.end();
            ++it
        )
        {
            if ((*it)->typeID() == PSA_COLLIDE_TYPE_ID)
                return false;
        }
    }
    // There is only one matrix swarm allowed:
    if (actionID == PSA_MATRIX_SWARM_TYPE_ID)
    {
        ParticleSystem::Actions &actions = ps->actionSet();
        for
        (
            ParticleSystem::Actions::iterator it = actions.begin();
            it != actions.end();
            ++it
        )
        {
            if ((*it)->typeID() == PSA_MATRIX_SWARM_TYPE_ID)
                return false;
        }
    }
    return true;
}


/**
 *  This function is called when the user drops a particle system onto the
 *  main window.
 */
bool ActionSelection::dropParticleSystem(UalItemInfo *ii)
{
	BW_GUARD;

    bool ok = MainFrame::instance()->PromptSave( MB_YESNOCANCEL, true ) != FALSE;
    if (ok)
	{
		BW::string fullname = bw_wtoutf8( ii->longText() );
		BWResource::dissolveFilename( fullname );
		BW::string dir  = BWResource::getFilePath( fullname );
		BW::StringRef file = BWResource::getFilename( fullname );

		ParticleEditorApp *app = (ParticleEditorApp *)AfxGetApp();
		app->openDirectory( dir );

		file = BWResource::removeExtension( file );
		this->selectMetaParticleSystem( file.to_string() );

		UalManager::instance().history().add( ii->assetInfo() );
	}
    return ok;
}


/**
 *  This function checks whether the new name of the node is allowable
 *  (i.e. unique and contains only valid chars).
 */
ActionSelection::RenameResult 
ActionSelection::checkUniqueName
(
    TreeNode        const *node, 
    BW::string     const &label
) const
{
	BW_GUARD;

    MetaNode const *metanode = dynamic_cast<MetaNode const *>(node);

    // Check that the new name doesn't have ":", "<", spaces (if a metanode) or
    // periods etc:
    BW::string validLabel = label;
    if (metanode != NULL && !StringUtils::makeValidFilenameT(validLabel, '_', false))
        return NAME_INVALID_CHAR_INC_SPACE;
    if (metanode == NULL && !StringUtils::makeValidFilenameT(validLabel, '_', true))
        return NAME_INVALID_CHAR;
	if (validLabel.size() > 0 && ::isdigit(validLabel[0]))
		return NAME_INVALID_CHAR;
	if (validLabel.find('.') != BW::string::npos)
		return NAME_INVALID_CHAR;

    // If the node is a MetaNode check that it is uniquely named:    
    if (metanode != NULL)
    {        
        for (size_t i = 0; i < particleTree_.NumberChildren(NULL); ++i)
        {
            MetaNode const *thismn = 
                dynamic_cast<MetaNode const *>(particleTree_.GetChild(NULL, i));
            if (thismn != NULL && thismn != metanode)
            {
                if (strcmpi(thismn->GetLabel().c_str(), label.c_str()) == 0)
                {
                    return NAME_NOT_UNIQUE;
                }
            }
        }
    }

    // If the node is a particle system then make sure that it is unique:
    PSNode const *psnode = dynamic_cast<PSNode const *>(node);
    if (psnode != NULL)
    {
        MetaNode const *parent = 
            dynamic_cast<MetaNode const *>(psnode->GetParent());
        if (parent != NULL)
        {
            for (size_t i = 0; i < parent->NumberChildren(); ++i)
            {
                PSNode const *thispsnode = 
                    dynamic_cast<PSNode const *>(parent->GetChild(i));
                if (thispsnode != NULL && thispsnode != psnode)
                {
                    if (strcmpi(thispsnode->GetLabel().c_str(), label.c_str()) == 0)
                    {
                        return NAME_NOT_UNIQUE;
                    }
                }
            }
        }
    }

    return NAME_OK;
}


/**
 *  This function sets extra properties to some actions since they are not, in 
 *  their default state, setup entirely correctly.
 * 
 *  @param newAction     The new action to fix.
 *  @param parent        The parent particle system.
 */
/*static*/ 
void 
ActionSelection::fixupAction
(
    ParticleSystemActionPtr newAction,
    ParticleSystemPtr       parent
)
{
	BW_GUARD;

    if (newAction->typeID() == PSA_SOURCE_TYPE_ID)
    {
        // TODO: take this out when we have gizmos for the vector generators
        PointVectorGenerator *initPos = new PointVectorGenerator();
        initPos->position(Vector3(0.f, 0.f, 0.f));
        PointVectorGenerator *initVel = new PointVectorGenerator();
        initVel->position(Vector3(0.f, 1.f, 0.f));

        SourcePSA *source = static_cast<SourcePSA *>(&*newAction);
        source->setPositionSource(initPos);
        source->setVelocitySource(initVel);
        source->maximumSize(0.1f);
        source->minimumSize(0.1f);
    }
    else if (newAction->typeID() == PSA_JITTER_TYPE_ID)
    {
        // TODO: take this out when we have gizmos for the vector generators
        PointVectorGenerator *initPos = new PointVectorGenerator();
        initPos->position(Vector3(0.f, 0.f, 0.f));
        PointVectorGenerator *initVel = new PointVectorGenerator();
        initVel->position(Vector3(0.f, 1.f, 0.f));

        JitterPSA *jitter = static_cast<JitterPSA *>(&*newAction);
        jitter->setPositionSource(initPos);
        jitter->setVelocitySource(initVel);
    }
    else if (newAction->typeID() == PSA_TINT_SHADER_TYPE_ID)
    {
        // Create some tints please:
        TintShaderPSA *tintShader = static_cast<TintShaderPSA *>(&*newAction);
        tintShader->addTintAt(0.f, Vector4(0.f, 0.f, 0.f, 1.f));
        tintShader->addTintAt(1.f, Vector4(1.f, 1.f, 1.f, 1.f));
    }
    else if (newAction->typeID() == PSA_COLLIDE_TYPE_ID)
    {
        CollidePSA *collideAction = static_cast<CollidePSA *>(&*newAction);
        collideAction->elasticity(1.f);
        collideAction->spriteBased( !parent->pRenderer()->isMeshStyle() );
    }
}


/**
 *  This function finds the parent MetaNode of a node.
 * 
 *  @param node          The node to search from.
 *  @returns             The parent MetaNode.
 */
/*static*/ MetaNode *ActionSelection::getMetaNode(TreeNode *node)
{
	BW_GUARD;

    MetaNode *mn = NULL;
    while (node != NULL)
    {
        mn = dynamic_cast<MetaNode *>(node);
        if (mn != NULL)
            break;
        node = node->GetParent();
    }
    return mn;
}
BW_END_NAMESPACE

