#include "pch.hpp"

#include "shlwapi.h"
#include "atlimage.h"

#include "tools/common/python_adapter.hpp"
#include "tools/common/string_utils.hpp"
#include "tools/common/utilities.hpp"

#include "main_frm.h"
#include "me_shell.hpp"
#include "me_app.hpp"
#include "mru.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"

#include "ual/ual_manager.hpp"

#include "common/material_utility.hpp"
#include "common/material_properties.hpp"
#include "common/material_editor.hpp"
#include "common/editor_views.hpp"
#include "common/user_messages.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"

#include "physics2/material_kinds.hpp"

#include "cstdmf/string_utils.hpp"

#include "me_material_proxies.hpp"

#include "page_object.hpp"

#include "modeleditor_core/Models/mutant.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

struct PageObjectImpl: public SafeReferenceCount
{
	static PageObject* s_currPage;

	DataSectionPtr materialKinds;
	BW::vector< int* > kindData;
	BW::map< int, int > kindItem;

	bool inited;
	bool ready;
	bool updating;

	BW::string lastDefaultText;

	CImage* thumbnail;

	BW::wstring modelName;

	CStatic name;

	CStatic thumbnailRect;
	
	CButton batch;

	BW::wstring editorProxyName;
	
	CEdit editorProxy;

	CButton editorProxySel;
	
	CComboBox kind;

	GeneralEditorPtr editor;

	int pageWidth;
};

PageObject* PageObjectImpl::s_currPage = NULL;

// PageObject

//ID string required for the tearoff tab manager
const BW::wstring PageObject::contentID = L"PageObjectID";

IMPLEMENT_DYNCREATE(PageObject, CFormView)

PageObject::PageObject():
	PropertyTable( PageObject::IDD )
{
	BW_GUARD;

	pImpl_ = new PageObjectImpl;

	pImpl_->inited = false;
	pImpl_->ready = false;
	pImpl_->updating = false;

	pImpl_->modelName = L"";
	pImpl_->editorProxyName = L"";

	pImpl_->thumbnail = new CImage();
	
	DataSectionPtr flagsFile = BWResource::openSection( "resources/flags.xml" );
	pImpl_->materialKinds = flagsFile->newSection( "materialKinds" );
	pImpl_->lastDefaultText = "None";
	pImpl_->materialKinds->writeInt( pImpl_->lastDefaultText, 0 );
	MaterialKinds::instance().populateDataSection( pImpl_->materialKinds );

	pImpl_->pageWidth = 0;
	pImpl_->s_currPage = this;
}

PageObject::~PageObject()
{
	BW_GUARD;

	delete pImpl_->thumbnail;
	for (unsigned i=0; i<pImpl_->kindData.size(); i++)
		delete pImpl_->kindData[i];

	if (pImpl_->editor)
	{
		pImpl_->editor->expel();

		GeneralEditor::currentEditors( GeneralEditor::Editors() );
	}
}

/*static*/ PageObject* PageObject::currPage()
{
	BW_GUARD;

	return PageObjectImpl::s_currPage;
}

void PageObject::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	PropertyTable::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_OBJECT_NAME, pImpl_->name);

	DDX_Control(pDX, IDC_OBJECT_THUMBNAIL, pImpl_->thumbnailRect);
	
	DDX_Control(pDX, IDC_OBJECT_PROXY, pImpl_->editorProxy);
	DDX_Control(pDX, IDC_OBJECT_PROXY_SEL, pImpl_->editorProxySel);
	
	DDX_Control(pDX, IDC_OBJECT_KIND, pImpl_->kind);

	CRect setupsRect;
	pImpl_->kind.GetWindowRect(setupsRect);
    ScreenToClient (&setupsRect);
	setupsRect.bottom += 256;	// Extend the dropdown box to show a reasonable selection
	pImpl_->kind.MoveWindow(setupsRect);
		
	int item = 0;

	DataSectionIterator it = pImpl_->materialKinds->begin();
	for (; it != pImpl_->materialKinds->end(); ++it)
	{
		int* materialID = new int( (*it)->asInt() );
		pImpl_->kindData.push_back( materialID );
		pImpl_->kindItem[ *materialID ] = 
			pImpl_->kind.InsertString( item,
				bw_utf8tow( (*it)->sectionName() ).c_str() );
		pImpl_->kind.SetItemData( item, (DWORD_PTR)materialID );
		item++;
	}

	pImpl_->inited = true;
}

BOOL PageObject::OnInitDialog()
{
	BW_GUARD;

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageObject >( &(pImpl_->editorProxy), "model", this, &PageObject::changeEditorProxyDrop ) );
	
	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(PageObject, CFormView)

	ON_WM_CREATE()

	ON_WM_SIZE()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)

	ON_WM_HSCROLL()

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_CBN_SELCHANGE(IDC_OBJECT_KIND, OnCbnSelchangeObjectKind)

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

	ON_BN_CLICKED(IDC_OBJECT_PROXY_SEL, OnBnClickedObjectProxySel)
	ON_BN_CLICKED(IDC_OBJECT_PROXY_REMOVE, OnBnClickedObjectProxyRemove)
END_MESSAGE_MAP()

void PageObject::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageObject::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )                                                   
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageObject::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	LPTSTR* msg = (LPTSTR*)wParam;
	mainFrame_->setMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageObject::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	mainFrame_->setMessageText( L"" );
	return 0;
}

// PageObject message handlers

int PageObject::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

void PageObject::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!pImpl_->inited) return;
	
	Utilities::stretchToRight( this, pImpl_->name, cx, 12 );
	PathSetDlgItemPath( this->GetSafeHwnd(), IDC_OBJECT_NAME, pImpl_->modelName.c_str() );

	Utilities::centre( this, pImpl_->thumbnailRect, cx );

	Utilities::stretchToRight( this, pImpl_->editorProxy, cx, 40 );
	PathSetDlgItemPath( this->GetSafeHwnd(), IDC_OBJECT_PROXY, pImpl_->editorProxyName.c_str() );

	Utilities::moveToRight( this, pImpl_->editorProxySel, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->kind, cx, 12 );

	PropertyTable::OnSize( nType, cx, cy );

	pImpl_->pageWidth = cx;
}

afx_msg LRESULT PageObject::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	static int s_updateCount = 0;
	pImpl_->updating = s_updateCount != MeApp::instance().mutant()->updateCount("Object");
	s_updateCount = MeApp::instance().mutant()->updateCount("Object");
	
	if (!pImpl_->ready)
	{
		OnInitDialog();

		pImpl_->updating = true; // Do an update first time through...

		pImpl_->ready = true;
	}

	// Check whether the LOD selection has changed
	if (pImpl_->updating)
	{
		pImpl_->s_currPage = this;

		bw_utf8tow( MeApp::instance().mutant()->modelName(), pImpl_->modelName );
		PathSetDlgItemPath( this->GetSafeHwnd(), IDC_OBJECT_NAME, pImpl_->modelName.c_str() );

		OnUpdateThumbnail();

		bw_utf8tow( MeApp::instance().mutant()->editorProxyName(), pImpl_->editorProxyName );
		PathSetDlgItemPath( this->GetSafeHwnd(), IDC_OBJECT_PROXY, pImpl_->editorProxyName.c_str() );

		int item = pImpl_->kindItem[ MeApp::instance().mutant()->modelMaterial() ];
		
		pImpl_->kind.SetCurSel( item );

		wchar_t text[256];
		pImpl_->kind.GetLBText( item, text );
		bw_snwprintf( text, ARRAY_SIZE(text), L"%s (Default)", text );
		BW::string ntext = bw_wtoutf8( text );
			
		pImpl_->materialKinds->delChild( pImpl_->lastDefaultText );
		pImpl_->lastDefaultText = ntext;
		pImpl_->materialKinds->writeInt( pImpl_->lastDefaultText, 0 );

		OnUpdateList();
	}

	PropertyTable::update();

	PropTable::table( this );

	return 0;
}

void PageObject::OnUpdateThumbnail()
{
	BW_GUARD;

	BW::wstring::size_type last = pImpl_->modelName.rfind(L".");
	BW::string thumbName = bw_wtoutf8( pImpl_->modelName.substr( 0, last ) );
	thumbName += ".thumbnail.jpg";
	thumbName = BWResource::resolveFilename( thumbName );

	pImpl_->thumbnail->Destroy();
	pImpl_->thumbnail->Load( bw_utf8tow( thumbName ).c_str() );
	UalManager::instance().thumbnailManager().stretchImage( *(pImpl_->thumbnail), 128, 128, true );
	pImpl_->thumbnailRect.SetBitmap( (HBITMAP)(*(pImpl_->thumbnail)) );

	//Do the centering here to make sure that it is done with the correct size
	Utilities::centre( this, pImpl_->thumbnailRect, pImpl_->pageWidth );
}

void PageObject::OnUpdateList()
{
	BW_GUARD;

	TreeRoot* treeRoot = MeApp::instance().mutant()->materialTree();

	PropTable::table( this );
	
	if (pImpl_->editor)
	{
		pImpl_->editor->expel();
	}

	pImpl_->editor = GeneralEditorPtr(new GeneralEditor(), true);

	GeneralEditor::Editors newEds;
	newEds.push_back( pImpl_->editor );
	GeneralEditor::currentEditors( newEds );

	MeApp::instance().mutant()->edit( *pImpl_->editor );

	pImpl_->editor->elect();
}

afx_msg LRESULT PageObject::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		bool transient = !!wParam;
		relevantView->onChange( transient );
	}

	return 0;
}

void PageObject::OnCbnSelchangeObjectKind()
{
	BW_GUARD;

	int id = *(int*)(pImpl_->kind.GetItemData( pImpl_->kind.GetCurSel() ));

	MeApp::instance().mutant()->modelMaterial( id );
}

bool PageObject::changeEditorProxy( const BW::wstring& editorProxyFile )
{
	BW_GUARD;

	if (editorProxyFile != pImpl_->modelName)
	{
		MeApp::instance().mutant()->editorProxyName( bw_wtoutf8( editorProxyFile ) );
		PathSetDlgItemPath( this->GetSafeHwnd(), IDC_OBJECT_PROXY, editorProxyFile.c_str() );
		pImpl_->editorProxyName = editorProxyFile;
		return true;
	}

	::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
		Localise(L"MODELEDITOR/PAGES/PAGE_OBJECT/BAD_DIR_EP"),
		Localise(L"MODELEDITOR/PAGES/PAGE_OBJECT/UNABLE_RESOLVE_EP"),
		MB_OK | MB_ICONWARNING );
	return false;
}

bool PageObject::changeEditorProxyDrop( UalItemInfo* ii ) 
{
	BW_GUARD;

	return changeEditorProxy( bw_utf8tow( BWResource::dissolveFilename( bw_wtoutf8( ii->longText() ) ) ) );
}

void PageObject::OnBnClickedObjectProxySel()
{
	BW_GUARD;

	static wchar_t szFilter[] =	L"Model (*.model)|*.model||";
	BWFileDialog fileDlg (
		true, L"", L"", BWFileDialog::FD_WRITABLE_AND_EXISTS, szFilter);

	BW::wstring modelDir = pImpl_->editorProxyName;
	if (modelDir == L"")
	{
		BW::string nmodelDir;
		MRU::instance().getDir("models", nmodelDir );
		bw_utf8tow( nmodelDir, modelDir );
	}
	fileDlg.initialDir( modelDir.c_str() );

	if (fileDlg.showDialog())
	{
		BW::string editorProxyFile = BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ));

		if (BWResource::validPath( editorProxyFile ))
		{
			changeEditorProxy( bw_utf8tow( editorProxyFile ) );
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/APP/ME_APP/BAD_DIR"),
				Localise(L"MODELEDITOR/APP/ME_APP/UNABLE_RESOLVE"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

void PageObject::OnBnClickedObjectProxyRemove()
{
	BW_GUARD;

	MeApp::instance().mutant()->removeEditorProxy();
	pImpl_->editorProxyName = L"";
	pImpl_->editorProxy.SetWindowText( L"" );
}
BW_END_NAMESPACE

