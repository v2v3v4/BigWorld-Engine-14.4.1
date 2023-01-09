
#include "pch.hpp"
#include "gui/pages/post_processing_chains.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

namespace
{
	// Constants used for adjusting the button layout on resize, etc.
	const int BUTTON_XMARGIN = 4;
	const int BUTTON_YMARGIN = 4;
	const int LIST_XMARGIN = 4;
	const int LIST_YMARGIN = 4;


	/**
	 *	This function decrements the refcount of pArgs, similar to Script::call.
	 */
	void callPython( PyObject * pModule, const BW::string funcName, PyObject * pArgs )
	{
		BW_GUARD;

		bool argsNeedsDecref = true;
		if (pModule)
		{
			PyObject * pFunction = PyObject_GetAttrString( pModule, funcName.c_str() );

			if (pFunction != NULL)
			{
				Script::call( pFunction, pArgs, ("PostProcessingChains::" + funcName + ": ").c_str() );
				argsNeedsDecref = false;
			}
			else
			{
				ERROR_MSG( "PostProcessingChains::%s - script function not found.\n", funcName.c_str() );
				PyErr_Print();
			}
		}
		if (argsNeedsDecref)
		{
			Py_XDECREF( pArgs );
		}
	}


	/**
	 *	This function decrements the refcount of pArgs, similar to Script::ask.
	 */
	PyObject * askPython( PyObject * pModule, const BW::string funcName, PyObject * pArgs )
	{
		BW_GUARD;

		PyObject * ret = NULL;

		bool argsNeedsDecref = true;
		if (pModule)
		{
			PyObject * pFunction = PyObject_GetAttrString( pModule, funcName.c_str() );

			if (pFunction != NULL)
			{
				ret = Script::ask( pFunction, pArgs, ("PostProcessingChains::" + funcName + ": ").c_str() );
				argsNeedsDecref = false;
			}
			else
			{
				ERROR_MSG( "PostProcessingChains::%s - script function not found.\n", funcName.c_str() );
				PyErr_Print();
			}
		}
		if (argsNeedsDecref)
		{
			Py_XDECREF( pArgs );
		}

		return ret;
	}

} // anonymous namespace


/**
 *	Constructor.
 */
PostProcessingChains::PostProcessingChains() :
	CDialog( PostProcessingChains::IDD ),
	handler_( NULL ),
	pScriptObject_( NULL ),
	listNeedsUpdate_( false ),
	ignoreSelection_( false ),
	defaultItem_( NULL )
{
}


/**
 *	Destructor.
 */
PostProcessingChains::~PostProcessingChains()
{
	BW_GUARD;

	Py_XDECREF( pScriptObject_ );
}


/**
 *	This method is called regularly and allows for updating the state of
 *	buttons and, if necessary, reloading the list of chains.
 */
void PostProcessingChains::update()
{
	BW_GUARD;

	HTREEITEM hItem = list_.GetSelectedItem();

	BOOL enableListItemOps = (hItem != NULL && hItem != defaultItem_);
	GetDlgItem( IDC_PP_RENAME_CHAIN )->EnableWindow( enableListItemOps );
	GetDlgItem( IDC_PP_DELETE_CHAIN )->EnableWindow( enableListItemOps );

	GetDlgItem( IDC_PP_DUPLICATE_CHAIN )->EnableWindow( TRUE );

	BOOL enableSave = (WorldManager::instance().userEditingPostProcessing() ? TRUE : FALSE);
	GetDlgItem( IDC_PP_SAVE_CHAIN )->EnableWindow( enableSave );

	BOOL enableDiscard = ((WorldManager::instance().userEditingPostProcessing() || enableListItemOps)
							? TRUE : FALSE);
	GetDlgItem( IDC_PP_DISCARD_CHANGES )->EnableWindow( enableDiscard );

	if (listNeedsUpdate_)
	{
		listNeedsUpdate_ = false;

		BW::string pyChainName;
		getPythonChain( pyChainName );

		fillChainsList();
		select( pyChainName );
	}
}


/**
 *	This method is called when the post processing chain has changed.
 */
void PostProcessingChains::chainChanged()
{
	BW_GUARD;

	list_.SelectItem( defaultItem_ );
	changeSelectedChain();
}


/**
 *	This method is called on exit, and informs the Python script object so it
 *	can verify and save the chain if necessary.
 */
void PostProcessingChains::onExit()
{
	BW_GUARD;

	callPython( pScriptObject_, "onExit", PyTuple_New(0) );
}


/**
 *	This MFC method syncs our Windows dialog controls with our member
 *	variables.
 *
 *	@param pDC	MFC data exchange struct.
 */
void PostProcessingChains::DoDataExchange( CDataExchange * pDX )
{
	BW_GUARD;

	CDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_PP_CHAIN_LIST, list_ );
}


/**
 *	This method is called when this window is being initialised.
 *
 *	@return	TRUE to signal success.
 */
BOOL PostProcessingChains::OnInitDialog()
{
	BW_GUARD;

	BOOL ok = CDialog::OnInitDialog();

	if (ok == FALSE)
	{
		return ok;
	}

	INIT_AUTO_TOOLTIP();

	// Initialise member variables
	chainsFolder_ = Options::getOptionString( "post_processing/chainsFolder", "system/post_processing/chains" );
	if (!chainsFolder_.empty() && (*chainsFolder_.rbegin()) != '/')
	{
		chainsFolder_ += "/";
	}

	// Init python module
	BW::string scriptFile = "PostProcessingUIAdapter";

    PyObject * pModule = PyImport_ImportModule(
					    		const_cast< char * >( scriptFile.c_str() ) );

    if (PyErr_Occurred())
    {
		ERROR_MSG( "PostProcessingUIAdapter - Failed to init python module %s\n", scriptFile.c_str() );
        PyErr_Print();
	}

    if (pModule != NULL)
    {
        pScriptObject_ = pModule;
    }
    else
    {
        ERROR_MSG( "PostProcessingUIAdapter - Could not get module %s\n", scriptFile.c_str() );
        PyErr_Print();
    }

	callPython( pScriptObject_, "init", Py_BuildValue( "(s)", chainsFolder_.c_str() ) );

	// Read the available chain files.
	listNeedsUpdate_ = true;

	// First update
	update();

	return TRUE;
}


/**
 *	This MFC method is overriden to handle special key presses such as Esc.
 *
 *	@param pMsg		Windows message to handle.
 *	@return TRUE to stop further handling, FALSE to continue handling it.
 */
BOOL PostProcessingChains::PreTranslateMessage( MSG * pMsg )
{
	BW_GUARD;

	//Handle tooltips first...
	CALL_TOOLTIPS( pMsg );
	
	// If edit control is visible in tree view control, when you send a
	// WM_KEYDOWN message to the edit control it will dismiss the edit
	// control. When the ENTER key was sent to the edit control, the
	// parent window of the tree view control is responsible for updating
	// the item's label in TVN_ENDLABELEDIT notification code.
	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
	{
		CEdit* edit = list_.GetEditControl();
		if (edit)
		{
			edit->SendMessage( WM_KEYDOWN, pMsg->wParam, pMsg->lParam );
			return TRUE; // Handled
		}
	}
	return CDialog::PreTranslateMessage( pMsg );
}


// MFC message map.
BEGIN_MESSAGE_MAP( PostProcessingChains, CDialog )
	ON_WM_SIZE()
	ON_NOTIFY( TVN_SELCHANGED, IDC_PP_CHAIN_LIST, OnChangeSelectedChain )
	ON_BN_CLICKED( IDC_PP_NEW_CHAIN, OnBtnNewChain )
	ON_BN_CLICKED( IDC_PP_RENAME_CHAIN, OnBtnRenameChain )
	ON_BN_CLICKED( IDC_PP_DUPLICATE_CHAIN, OnBtnDuplicateChain )
	ON_BN_CLICKED( IDC_PP_DELETE_CHAIN, OnBtnDeleteChain )
	ON_BN_CLICKED( IDC_PP_SAVE_CHAIN, OnBtnSaveChanges )
	ON_BN_CLICKED( IDC_PP_DISCARD_CHANGES, OnBtnDiscardChanges )
	ON_NOTIFY( NM_CUSTOMDRAW, IDC_PP_CHAIN_LIST, OnCustomDrawList )
	ON_NOTIFY( TVN_BEGINLABELEDIT, IDC_PP_CHAIN_LIST, OnListBeginEditName )
	ON_NOTIFY( TVN_ENDLABELEDIT, IDC_PP_CHAIN_LIST, OnListEndEditName )
	ON_NOTIFY( TVN_KEYDOWN, IDC_PP_CHAIN_LIST, OnListKeyDown )
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden reposition the buttons and resize the list
 *	when this window gets resized.
 *
 *	@param nType	MFC resize type.
 *	@param cx	New width.
 *	@param cy	New height.
 */
void PostProcessingChains::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CDialog::OnSize( nType, cx, cy );

	if (list_.GetSafeHwnd())
	{
		CRect btnRect;
		GetDlgItem( IDC_PP_NEW_CHAIN )->GetWindowRect( btnRect );
		ScreenToClient( btnRect );

		int btnX = cx - btnRect.Width() - BUTTON_XMARGIN;
		int btnY = btnRect.top;
		int btnYStep = btnRect.Height() + BUTTON_YMARGIN;

		CRect listRect;
		list_.GetWindowRect( listRect );
		ScreenToClient( listRect );
		list_.SetWindowPos( NULL, 0, 0, cx - btnRect.Width() - BUTTON_XMARGIN - LIST_XMARGIN, cy - LIST_YMARGIN - listRect.top, SWP_NOZORDER | SWP_NOMOVE );

		GetDlgItem( IDC_PP_NEW_CHAIN )->SetWindowPos( NULL, btnX, btnY, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
		btnY += btnYStep;
		GetDlgItem( IDC_PP_RENAME_CHAIN )->SetWindowPos( NULL, btnX, btnY, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
		btnY += btnYStep;
		GetDlgItem( IDC_PP_DUPLICATE_CHAIN )->SetWindowPos( NULL, btnX, btnY, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
		btnY += btnYStep;
		GetDlgItem( IDC_PP_DELETE_CHAIN )->SetWindowPos( NULL, btnX, btnY, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
		btnY += btnYStep;
		GetDlgItem( IDC_PP_SAVE_CHAIN )->SetWindowPos( NULL, btnX, btnY, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
		btnY += btnYStep;
		GetDlgItem( IDC_PP_DISCARD_CHANGES )->SetWindowPos( NULL, btnX, btnY, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

		RedrawWindow();
	}
}


/**
 *	This MFC method is called when a chain is selected in the chains list.
 *
 *	@param pNotifyStruct	MFC notify strict, ignored.
 *	@param result	MFC return result, ignored.
 */
void PostProcessingChains::OnChangeSelectedChain( NMHDR * pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	if (!ignoreSelection_)
	{
		changeSelectedChain();
	}
}


/**
 *	This MFC method is called when the New Chain button is pressed.
 */
void PostProcessingChains::OnBtnNewChain()
{
	BW_GUARD;

	callPython( pScriptObject_, "newChain", PyTuple_New(0) );

	// Refresh the list in case a new chain was created, but keep the selection.
	listNeedsUpdate_ = true;
}


/**
 *	This MFC method is called when the Rename button is pressed.
 */
void PostProcessingChains::OnBtnRenameChain()
{
	BW_GUARD;

	if (list_.GetSelectedItem() != defaultItem_)
	{
		list_.SetFocus();
		list_.EditLabel( list_.GetSelectedItem() );
	}
}


/**
 *	This MFC method is called when the Duplicate button is pressed.
 */
void PostProcessingChains::OnBtnDuplicateChain()
{
	BW_GUARD;

	callPython( pScriptObject_, "duplicateChain", PyTuple_New(0) );

	// Refresh the list in case the chain was renamed, and re-select the chain.
	listNeedsUpdate_ = true;
}


/**
 *	This MFC method is called when the Delete button is pressed.
 */
void PostProcessingChains::OnBtnDeleteChain()
{
	BW_GUARD;

	callPython( pScriptObject_, "deleteChain", PyTuple_New(0) );

	listNeedsUpdate_ = true;
}


/**
 *	This MFC method is called when the Save button is pressed.
 */
void PostProcessingChains::OnBtnSaveChanges()
{
	BW_GUARD;

	callPython( pScriptObject_, "saveChanges", PyTuple_New(0) );

	// Refresh the list in case a new chain was created, but keep the selection.
	listNeedsUpdate_ = true;
}


/**
 *	This MFC method is called when the Discard button is pressed.
 */
void PostProcessingChains::OnBtnDiscardChanges()
{
	BW_GUARD;

	callPython( pScriptObject_, "discardChanges", PyTuple_New(0) );
}


/**
 *	This MFC method is overriden to paint the default system chain differntly.
 *
 *	@param pNMHDR	MFC notify header struct.
 *	@param pResult	MFC result, indicating if we need to be notified when an
 *					item will be painted.
 */
void PostProcessingChains::OnCustomDrawList( NMHDR * pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	NMTVCUSTOMDRAW * pTVCD = reinterpret_cast< NMTVCUSTOMDRAW * >( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pTVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pTVCD->nmcd.dwDrawStage)
	{
		if ((HTREEITEM)pTVCD->nmcd.dwItemSpec == defaultItem_)
		{
			pTVCD->clrText = ::GetSysColor( COLOR_GRAYTEXT );
		}
	}
}


/**
 *	This MFC method is overriden to prevent the user from editing the name of
 *	the default system chain.
 *
 *	@param pNMHDR	MFC notify header struct.
 *	@param pResult	MFC result, 0 to allow editing, 1 to cancel.
 */
void PostProcessingChains::OnListBeginEditName( NMHDR * pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	*pResult =
		(list_.GetSelectedItem() != defaultItem_) ? 0 /*allow edit*/ : 1 /*cancel edit*/;
}


/**
 *	This MFC method is overriden to finish the user's renaming of a chain.
 *
 *	@param pNMHDR	MFC notify header struct.
 *	@param pResult	MFC result, 0.
 */
void PostProcessingChains::OnListEndEditName( NMHDR * pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	if (pTVDispInfo->item.pszText)
	{
		BW::string newName = bw_wtoutf8( pTVDispInfo->item.pszText );
		callPython( pScriptObject_, "renameChain", Py_BuildValue( "(s)", newName.c_str() ) );

		// Refresh the list in case a new chain was created, but keep the selection.
		listNeedsUpdate_ = true;
	}

	*pResult = 0;
}


/**
 *	This MFC method is overriden to ensure F2 allows for renaming the currently
 *	selected chain (if it's not the default system chain).
 *
 *	@param pNMHDR	MFC notify header struct.
 *	@param pResult	MFC result, 0.
 */
void PostProcessingChains::OnListKeyDown( NMHDR * pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	if (list_.GetSelectedItem() != defaultItem_)
	{
		LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast< LPNMTVKEYDOWN >( pNMHDR );
		
		if (pTVKeyDown->wVKey == VK_F2)
		{
			list_.SetFocus();
			list_.EditLabel( list_.GetSelectedItem() );
		}
	}
	*pResult = 0;
}


/**
 *	This method retrieves the current chain from Python.
 *
 *	@param ret	Return param, name of the current chain in Python.
 */
void PostProcessingChains::getPythonChain( BW::string & ret )
{
	BW_GUARD;

	PyObject * pChainName = askPython( pScriptObject_, "getCurrentChain", PyTuple_New(0) );
	if (pChainName && PyString_Check( pChainName ))
	{
		Script::setData( pChainName, ret );
		ret = BWResource::removeExtension( BWResource::getFilename( ret ) ).to_string();
	}
	Py_XDECREF( pChainName );
}


/**
 *	This method returns the name of the chain currently selected on the list.
 *
 *	@param ret	Return param, name of the currently selected chain.
 */
void PostProcessingChains::getListChain( BW::string & ret )
{
	BW_GUARD;

	CString curListChainNameW;
	HTREEITEM hItem = list_.GetSelectedItem();
	if (hItem && hItem != defaultItem_)
	{
		curListChainNameW = list_.GetItemText( hItem );
	}
	ret = bw_wtoutf8( (LPCWSTR)curListChainNameW );
}


/**
 *	This method sets the currently selected chain as the post-processing chain.
 */
void PostProcessingChains::changeSelectedChain()
{
	BW_GUARD;

	BW::string chainName;
	getListChain( chainName );

	BW::string chainPath;
	if (!chainName.empty())
	{
		chainPath = chainsFolder_ + chainName;
	}

	callPython( pScriptObject_, "setChain", Py_BuildValue( "(s)", chainPath.c_str() ) );

	BW::string pyChainName;
	getPythonChain( pyChainName );

	if (chainName == pyChainName)
	{
		// change successful
		if (WorldManager::instance().changedPostProcessing())
		{
			// The chain was set. Reset this flag because it should be set when the
			// chain is changed outside the post-processing panel, not now.
			WorldManager::instance().changedPostProcessing( false );

			// We still need to tell the panel that it needs to update the graph.
			if (handler_)
			{
				handler_->SendMessage( WM_PP_CHAIN_SELECTED );
			}

			// In some cases, it's possible that a new file was saved, so reload
			// the list
			listNeedsUpdate_ = true;
		}
	}
	else
	{
		// change failed (probably there were unsaved changes that the user didn't want to lose).
		select( pyChainName );
	}
}


/**
 *	This method scans the disk for chains and fills in the list.
 */
void PostProcessingChains::fillChainsList()
{
	BW_GUARD;

	ignoreSelection_ = true;

	list_.DeleteAllItems();
	listMap_.clear();

	BW::string chainFolder( chainsFolder_ );
	if (!chainFolder.empty() && (*chainFolder.rbegin()) == '/')
	{
		chainFolder = chainFolder.substr( 0, chainFolder.length() - 1 );
	}

	BWResource::instance().purge( chainFolder );
	DataSectionPtr pChainsDir = BWResource::openSection( chainFolder );
	if (pChainsDir)
	{
		for (int i = 0; i < pChainsDir->countChildren(); ++i)
		{
			BW::string chainFile = pChainsDir->childSectionName( i );
			if (BWResource::getExtension( chainFile ) == "ppchain")
			{
				BW::string chainName = BWResource::removeExtension( chainFile ).to_string();
				
				HTREEITEM hItem = list_.InsertItem( bw_utf8tow( chainName ).c_str() );
				listMap_.insert( std::make_pair( chainName, hItem ) );
			}
		}
	}
	else
	{
        ERROR_MSG( "PostProcessingChains - Could not open the chains directory %s\n", chainFolder.c_str() );
	}

	list_.SortChildren( TVI_ROOT );

	defaultItem_ = list_.InsertItem( Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DEFAULT_CHAIN_NAME" ), TVI_ROOT, TVI_FIRST );  // Default Chain

	ignoreSelection_ = false;
}


/**
 *	This method selects the specified chain on the list.
 *
 *	@param str	Name of the chain to be selected.
 */
void PostProcessingChains::select( const BW::string & str )
{
	BW_GUARD;

	if (str.empty())
	{
		list_.SelectItem( defaultItem_ );
	}
	else
	{
		list_.SelectItem( listMap_[ str ] );
	}
}
BW_END_NAMESPACE

