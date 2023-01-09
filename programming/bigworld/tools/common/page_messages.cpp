#include "pch.hpp"

#include "shlwapi.h"
#include "atlimage.h"

#include "page_messages.hpp"

#include "delay_redraw.hpp"
#include "string_utils.hpp"
#include "user_messages.hpp"
#include "utilities.hpp"

#include "appmgr/commentary.hpp"
#include "appmgr/options.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_item.hpp"

#include "chunk/editor_chunk_item.hpp"

#include "controls/user_messages.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"

#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

namespace
{
	const char * DATE_OPTION_PATH = "messages/showDate";
	const char * TIME_OPTION_PATH = "messages/showTime";
	const char * PRIORITY_OPTION_PATH = "messages/showPriority";
	const char * ERRORS_OPTION_PATH = "messages/errorMsgs";
	const char * WARNING_OPTION_PATH = "messages/warningMsgs";
	const char * NOTICE_OPTION_PATH = "messages/noticeMsgs";
	const char * INFO_OPTION_PATH = "messages/infoMsgs";
	const char * ASSET_OPTION_PATH = "messages/assetMsgs";


	int getOrCreateOption( const BW::string & name, int defaultVal )
	{
		if (!Options::optionExists( name ))
		{
			Options::setOptionInt( name, defaultVal );
		}
		return Options::getOptionInt( name );
	}


	void toggleOption( const BW::string & name )
	{
		Options::setOptionInt( name,
				Options::getOptionInt( name ) == 0 ? 1 : 0 );
	}
} // anonymous


struct PageMessagesImpl: public SafeReferenceCount
{
	static class PageMessages* s_currPage;

	MsgsImpl* msgsImpl;

	bool inited;
	bool ready;

	CFrameWnd* mainFrame;

	CButton showDateChk;
	CButton showTimeChk;
	CButton showPriorityChk;
	CButton errorMsgsChk;
	CButton warningMsgsChk;
	CButton noticeMsgsChk;
	CButton infoMsgsChk;
	CButton assetMsgsChk;

	int showDate;
	int showTime;
	int showPriority;
	int errorMsgs;
	int warningMsgs;
	int noticeMsgs;
	int infoMsgs;
	int assetMsgs;

	size_t dateWidth_;
	size_t timeWidth_;
	size_t priorityWidth_;
	int msgWidth_;

	bool widthIncreased_;

	int lastMessage;

	CImageList imageList;

	BW::vector< BWMessageInfo * > tempData;
};

/*static*/ PageMessages* PageMessagesImpl::s_currPage = NULL;


// PageMessages

//ID string required for the tearoff tab manager
const BW::wstring PageMessages::contentID = L"PageMessagesID";

IMPLEMENT_DYNCREATE(PageMessages, CFormView)

PageMessages::PageMessages():
	CFormView( PageMessages::IDD )
{
	BW_GUARD;

	pImpl_ = new PageMessagesImpl;

	pImpl_->mainFrame = NULL;

	pImpl_->inited = false;
	pImpl_->ready = false;

	pImpl_->showDate = true;
	pImpl_->showTime = true;

	pImpl_->dateWidth_ = wcslen(Localise(L"COMMON/PAGE_MESSAGES/DATE"));
	pImpl_->timeWidth_ = wcslen(Localise(L"COMMON/PAGE_MESSAGES/TIME"));
	pImpl_->priorityWidth_ = wcslen(Localise(L"COMMON/PAGE_MESSAGES/PRIORITY"));
	pImpl_->msgWidth_ = 
		static_cast<int>(wcslen(Localise(L"COMMON/PAGE_MESSAGES/MESSAGE")));

	pImpl_->widthIncreased_ = true;

	pImpl_->lastMessage = 0;

	pImpl_->s_currPage = this;	

	pImpl_->msgsImpl = NULL;

	selected_ = "";
}

PageMessages::~PageMessages()
{
	BW_GUARD;

	bw_safe_delete(pImpl_->msgsImpl);
}

/*static*/ PageMessages* PageMessages::currPage()
{
	BW_GUARD;

	return PageMessagesImpl::s_currPage;
}

void PageMessages::mainFrame( CFrameWnd* mf )
{
	BW_GUARD;

	pImpl_->mainFrame = mf;
}


bool PageMessages::showFilter( int priority )
{
	BW_GUARD;

	if (msgFilter_.find( priority ) != msgFilter_.end())
	{
		return msgFilter_[ priority ];
	}
	else
	{
		return false;
	}
}

bool PageMessages::setPriority( int priority, bool enabled )
{
	BW_GUARD;

	msgFilter_[ priority ] = enabled;

	redrawList();

	return msgFilter_[ priority ];
}

int PageMessages::autoSizeColumn( int i )
{
	BW_GUARD;

	DelayRedraw temp( &msgList_ );
	
	msgList_.SetColumnWidth( i, LVSCW_AUTOSIZE );
	int columnWidth = msgList_.GetColumnWidth( i );

	msgList_.SetColumnWidth( i, LVSCW_AUTOSIZE_USEHEADER );
	int headerWidth = msgList_.GetColumnWidth( i ); 

	int maxWidth = max( columnWidth, headerWidth );

	msgList_.SetColumnWidth( i, maxWidth );

	return maxWidth;
}

bool PageMessages::addItem( BWMessageInfoPtr message )
{
	BW_GUARD;

	BW::string msg = message->msgStr();
		
	bool createIndex = false;
	bool willShow = true;
	bool subItem = true;
	int index = 0;
	int numInst = MsgHandler::instance().numAssetMsgs( msg );
	if ((numInst > 0) && (message->priority() == MESSAGE_PRIORITY_ASSET))
	{
		if (index_.find( msg ) == index_.end())
		{
			index_[ msg ] = 0;
			createIndex = true;
		}
		else
		{
			BWMessageInfo* itemData = (BWMessageInfo*)( msgList_.GetItemData( index_[ msg ] - 1 ));
			if (itemData)
			{
				const wchar_t* plural = numInst > 1 ? Localise(L"COMMON/PAGE_MESSAGES/PLURAL") : L"";
				msgList_.SetItemText( index_[ msg ] - 1, MSG_COL, 
					Localise(L"COMMON/PAGE_MESSAGES/INSTANCE", itemData->msgStr(), numInst, plural ) );
			}
		}

		if (!expanded_[ msg ])
		{
			willShow = false;
		}

		index = index_[ msg ];
		
	}
	else
	{
		subItem = false;
	}
	
	if (willShow)
	{
		// Advance all indexes after the new entry
		BW::map< BW::string, int >::iterator it = index_.begin();
		BW::map< BW::string, int >::iterator end = index_.end();
		for (; it != end; ++it)
		{
			if ((*it).second > index)
			{
				(*it).second++;
			}
		}
		
		// NOTE: I am using %S (capital letter) for formatting here...

		const wchar_t* indent = subItem ? L"    " : L"";
		wchar_t buf[1024];
		int nIndex = msgList_.InsertItem( index, L"" );
		bw_snwprintf( buf, ARRAY_SIZE( buf ), L"%s%S", indent, message->dateStr() );
		msgList_.SetItemText( nIndex, DATE_COL, buf );
		bw_snwprintf( buf, ARRAY_SIZE( buf ), L"%s%S", indent, message->timeStr() );
		msgList_.SetItemText( nIndex, TIME_COL, buf );
		bw_snwprintf( buf, ARRAY_SIZE( buf ), L"%s%S", indent, message->priorityStr() );
		msgList_.SetItemText( nIndex, PRIORITY_COL, buf );
		bw_snwprintf( buf, ARRAY_SIZE( buf ), L"%s%S", indent, msg.c_str() );
		msgList_.SetItemText( nIndex, MSG_COL, buf );

		msgList_.SetItemData( nIndex, (DWORD_PTR)&*message );
	}

	if (createIndex)
	{
		BWMessageInfo* entry = new BWMessageInfo( MESSAGE_PRIORITY_ASSET, "", "", "", msg.c_str() );

		const wchar_t* plural = numInst > 1 ? Localise(L"PLURAL") : L"";

		int nIndex = msgList_.InsertItem( index, L"", 1 + willShow );
		msgList_.SetItemText( nIndex, DATE_COL, bw_utf8tow( message->dateStr() ).c_str() );
		msgList_.SetItemText( nIndex, TIME_COL, bw_utf8tow( message->timeStr() ).c_str() );
		msgList_.SetItemText( nIndex, PRIORITY_COL, bw_utf8tow( message->priorityStr() ).c_str() );
		msgList_.SetItemText( nIndex, MSG_COL, 
			Localise(L"COMMON/PAGE_MESSAGES/INSTANCE", msg, numInst, plural ) );

		msgList_.SetItemData( nIndex, (DWORD_PTR)&*entry );
		if ( pImpl_->tempData.size() == pImpl_->tempData.capacity() )
			pImpl_->tempData.reserve( pImpl_->tempData.size() + 256 );
		pImpl_->tempData.push_back( entry );

		if (msg == selected_)
		{
			msgList_.SetItemState( nIndex, LVIS_SELECTED, LVIS_SELECTED );
		}

		// Advance all indexes after the new entry
		BW::map< BW::string, int >::iterator it = index_.begin();
		BW::map< BW::string, int >::iterator end = index_.end();
		for (; it != end; ++it)
		{
			if ((*it).second >= index)
			{
				(*it).second++;
			}
		}
	}

	if ( (int)strlen(message->dateStr()) > pImpl_->dateWidth_ )
	{
		pImpl_->dateWidth_ = strlen(message->dateStr());
		pImpl_->widthIncreased_ = true;
	}
	if ( (int)strlen(message->timeStr()) > pImpl_->timeWidth_ )
	{
		pImpl_->timeWidth_ = strlen(message->timeStr());
		pImpl_->widthIncreased_ = true;
	}
	if ( (int)strlen(message->priorityStr()) > pImpl_->priorityWidth_)
	{
		pImpl_->priorityWidth_ = strlen(message->priorityStr());
		pImpl_->widthIncreased_ = true;
	}
	if ( (int)strlen(message->msgStr()) > pImpl_->msgWidth_)
	{
		pImpl_->msgWidth_ = static_cast<int>(strlen(message->msgStr()));
		pImpl_->widthIncreased_ = true;
	}

	return willShow || createIndex;
}

void PageMessages::redrawList()
{
	BW_GUARD;

	int initPos = msgList_.GetScrollPos( SB_VERT );

	{
		DelayRedraw temp( &msgList_ );

		//Delete all the temp message data
		BW::vector< BWMessageInfo* >::iterator it =  pImpl_->tempData.begin();
		BW::vector< BWMessageInfo* >::iterator end =  pImpl_->tempData.end();
		for (; it != end; ++it)
		{
			bw_safe_delete(*it);
		}
		pImpl_->tempData.clear();
	
		//Clear the list
		msgList_.DeleteAllItems();

		index_.clear();
			
		BW::vector< BWMessageInfoPtr > messages = MsgHandler::instance().messages();

		pImpl_->lastMessage = static_cast<int>(messages.size());

		for (int i=0; i<pImpl_->lastMessage; i++)
		{
			if (showFilter( messages[i]->priority() ))
			{
				addItem( messages[i] );
			}
		}
	}

	DelayRedraw temp( &msgList_ );
	
	updateColumnWidths( true );
		
	for (int i=0; i<initPos; i++)
	{
		::SendMessage(msgList_.m_hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
	}

	msgList_.UpdateWindow();
}

void PageMessages::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	CFormView::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_MSG_LIST, msgList_ );

	msgList_.SetExtendedStyle( LVS_EX_FULLROWSELECT );

	msgList_.InsertColumn( TREE_COL, L"", LVCFMT_LEFT, 16 );
	msgList_.InsertColumn( DATE_COL, Localise(L"COMMON/PAGE_MESSAGES/DATE"), LVCFMT_LEFT, 64 );
	msgList_.InsertColumn( TIME_COL, Localise(L"COMMON/PAGE_MESSAGES/TIME"), LVCFMT_LEFT, 64 );
	msgList_.InsertColumn( PRIORITY_COL, Localise(L"COMMON/PAGE_MESSAGES/PRIORITY"), LVCFMT_LEFT, 128 );
	msgList_.InsertColumn( MSG_COL, Localise(L"COMMON/PAGE_MESSAGES/MESSAGE"), LVCFMT_LEFT, 128 );

	// Create 16 colour image lists
	HIMAGELIST hList = ImageList_Create( 9, 14, ILC_COLOR4 | ILC_MASK, 3, 1);
	pImpl_->imageList.Attach( hList );

	//Load the image from file
	HBITMAP hBitmap = 
        (HBITMAP)::LoadImage
        (
            NULL,
            bw_utf8tow( BWResource::resolveFilename( "resources/data/tree_ctrl.bmp" ) ).c_str(),
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE
        );

	//Create a CBitmap using the loaded resource
	CBitmap treeCtrlBmp;
	treeCtrlBmp.Attach( hBitmap );

	//Ad the bitmap to the image list
	pImpl_->imageList.Add( &treeCtrlBmp, RGB(255, 0, 255) );

	//Apply this image list to the list control
	msgList_.SetImageList( &pImpl_->imageList, LVSIL_SMALL );

	DDX_Control(pDX, IDC_MSG_SHOW_DATE, pImpl_->showDateChk);
	DDX_Control(pDX, IDC_MSG_SHOW_TIME, pImpl_->showTimeChk);
	DDX_Control(pDX, IDC_MSG_SHOW_PRIORITY, pImpl_->showPriorityChk);

	DDX_Control(pDX, IDC_MSG_ERROR, pImpl_->errorMsgsChk);
	DDX_Control(pDX, IDC_MSG_WARNING, pImpl_->warningMsgsChk);
	DDX_Control(pDX, IDC_MSG_NOTICE, pImpl_->noticeMsgsChk);
	DDX_Control(pDX, IDC_MSG_INFO, pImpl_->infoMsgsChk);
	DDX_Control(pDX, IDC_MSG_ASSETS, pImpl_->assetMsgsChk);

	pImpl_->inited = true;
}

BOOL PageMessages::OnInitDialog()
{
	BW_GUARD;

	INIT_AUTO_TOOLTIP();

	return TRUE;
}

BEGIN_MESSAGE_MAP(PageMessages, CFormView)

	ON_WM_CREATE()

	ON_WM_SIZE()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_BN_CLICKED(IDC_MSG_SHOW_DATE, OnBnClickedErrorsShowDate)
	ON_BN_CLICKED(IDC_MSG_SHOW_TIME, OnBnClickedErrorsShowTime)
	ON_BN_CLICKED(IDC_MSG_SHOW_PRIORITY, OnBnClickedErrorsShowPriority)

	ON_BN_CLICKED(IDC_MSG_ERROR, OnBnClickedErrorsError)
	ON_BN_CLICKED(IDC_MSG_WARNING, OnBnClickedErrorsWarning)
	ON_BN_CLICKED(IDC_MSG_NOTICE, OnBnClickedErrorsNotice)
	ON_BN_CLICKED(IDC_MSG_INFO, OnBnClickedErrorsInfo)

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

	ON_NOTIFY(NM_CLICK, IDC_MSG_LIST, &PageMessages::OnNMClickMsgList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_MSG_LIST, &PageMessages::OnNMCustomdrawMsgList)

	ON_BN_CLICKED(IDC_MSG_ASSETS, &PageMessages::OnBnClickedMsgAssets)
END_MESSAGE_MAP()

void PageMessages::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageMessages::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageMessages::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!pImpl_->mainFrame) return 0;
	LPWSTR* msg = (LPWSTR*)wParam;
	pImpl_->mainFrame->SetMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageMessages::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!pImpl_->mainFrame) return 0;
	pImpl_->mainFrame->SetMessageText( L"" );
	return 0;
}

// PageMessages message handlers

int PageMessages::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

void PageMessages::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!pImpl_->inited) return;
	
	if ( cx < 642 )
	{
		Utilities::stretchToBottomRight( this, msgList_, cx, 12, cy, 74 );

		Utilities::moveToBottomLeft( this, pImpl_->showDateChk, cy, 48, 12 );
		Utilities::moveToBottomLeft( this, pImpl_->showTimeChk, cy, 28, 12 );
		Utilities::moveToBottomLeft( this, pImpl_->showPriorityChk, cy, 8, 12 );

		Utilities::moveToBottomLeft( this, pImpl_->errorMsgsChk, cy, 48, 112 );
		Utilities::moveToBottomLeft( this, pImpl_->warningMsgsChk, cy, 28, 112 );
		Utilities::moveToBottomLeft( this, pImpl_->noticeMsgsChk, cy, 8, 112 );

		Utilities::moveToBottomLeft( this, pImpl_->assetMsgsChk, cy, 48, 180 );
		Utilities::moveToBottomLeft( this, pImpl_->infoMsgsChk, cy, 28, 180 );
	}
	else
	{
		Utilities::stretchToBottomRight( this, msgList_, cx, 12, cy, 34 );

		Utilities::moveToBottomLeft( this, pImpl_->showDateChk, cy, 8, 12 );
		Utilities::moveToBottomLeft( this, pImpl_->showTimeChk, cy, 8, 102 );
		Utilities::moveToBottomLeft( this, pImpl_->showPriorityChk, cy, 8, 192 );

		Utilities::moveToBottomLeft( this, pImpl_->errorMsgsChk, cy, 8, 292 );
		Utilities::moveToBottomLeft( this, pImpl_->warningMsgsChk, cy, 8, 362 );
		Utilities::moveToBottomLeft( this, pImpl_->noticeMsgsChk, cy, 8, 432 );

		Utilities::moveToBottomLeft( this, pImpl_->assetMsgsChk, cy, 8, 502 );
		Utilities::moveToBottomLeft( this, pImpl_->infoMsgsChk, cy, 8, 572 );
	}

	updateColumnWidths( true );
			
	CFormView::OnSize( nType, cx, cy );
	
	pImpl_->showDateChk.RedrawWindow();
	pImpl_->showTimeChk.RedrawWindow();
	pImpl_->showPriorityChk.RedrawWindow();
	pImpl_->errorMsgsChk.RedrawWindow();
	pImpl_->warningMsgsChk.RedrawWindow();
	pImpl_->noticeMsgsChk.RedrawWindow();
	pImpl_->infoMsgsChk.RedrawWindow();
	pImpl_->assetMsgsChk.RedrawWindow();
}

void PageMessages::updateColumnWidths( bool all /* = false */ )
{
	BW_GUARD;

	int checked = 0;
	
	checked = getOrCreateOption( DATE_OPTION_PATH, 1 );
	pImpl_->showDateChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if (all || ( checked != pImpl_->showDate ))
	{
		if (checked)
		{
			autoSizeColumn( DATE_COL );
		}
		else
		{
			msgList_.SetColumnWidth( DATE_COL, 0 );
		}
		pImpl_->showDate = !!checked;
	}

	checked = getOrCreateOption( TIME_OPTION_PATH, 1 );
	pImpl_->showTimeChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if (all || ( checked != pImpl_->showTime ))
	{
		if (checked)
		{
			autoSizeColumn( TIME_COL );
		}
		else
		{
			msgList_.SetColumnWidth( TIME_COL, 0 );
		}
		pImpl_->showTime = !!checked;
	}

	checked = getOrCreateOption( PRIORITY_OPTION_PATH, 1 );
	pImpl_->showPriorityChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if (all || ( checked != pImpl_->showPriority ))
	{
		if (checked)
		{
			autoSizeColumn( PRIORITY_COL );
		}
		else
		{
			msgList_.SetColumnWidth( PRIORITY_COL, 0 );
		}
		pImpl_->showPriority = !!checked;
	}

	if (all)
	{
		autoSizeColumn( TREE_COL );
		autoSizeColumn( MSG_COL );
	}
}

afx_msg LRESULT PageMessages::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!pImpl_->ready)
	{
		OnInitDialog();

		pImpl_->ready = true;
	}

	int checked = 0;

	checked = getOrCreateOption( ERRORS_OPTION_PATH, 1 );
	pImpl_->errorMsgsChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if ( checked != pImpl_->errorMsgs )
	{
		setPriority( MESSAGE_PRIORITY_ERROR, !!checked );
		pImpl_->errorMsgs = !!checked;
	}

	checked = getOrCreateOption( WARNING_OPTION_PATH, 1 );
	pImpl_->warningMsgsChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if ( checked != pImpl_->warningMsgs )
	{
		setPriority( MESSAGE_PRIORITY_WARNING, !!checked );
		pImpl_->warningMsgs = !!checked;
	}

	checked = getOrCreateOption( NOTICE_OPTION_PATH, 0 );
	pImpl_->noticeMsgsChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if ( checked != pImpl_->noticeMsgs )
	{
		setPriority( MESSAGE_PRIORITY_NOTICE, !!checked );
		pImpl_->noticeMsgs = !!checked;
	}

	checked = getOrCreateOption( INFO_OPTION_PATH, 0 );
	pImpl_->infoMsgsChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if ( checked != pImpl_->infoMsgs )
	{
		setPriority( MESSAGE_PRIORITY_INFO, !!checked );
		pImpl_->infoMsgs = !!checked;
	}	

	checked = getOrCreateOption( ASSET_OPTION_PATH, 1 );
	pImpl_->assetMsgsChk.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
	if ( checked != pImpl_->assetMsgs )
	{
		setPriority( MESSAGE_PRIORITY_ASSET, !!checked );
		pImpl_->assetMsgs = !!checked;
	}	

	if (MsgHandler::instance().updateMessages())
	{
		bool changed = false;
		
		BW::vector< BWMessageInfoPtr > messages = MsgHandler::instance().messages();
	
		int last = static_cast<int>(messages.size());

		//If for some reason there have been messages removed
		if (last < pImpl_->lastMessage || MsgHandler::instance().forceRedraw() )
		{
			redrawList();
			changed = true;
			pImpl_->lastMessage = last;
		}

		for (int i = pImpl_->lastMessage; i<last; i++)
		{
			if (showFilter( messages[i]->priority() ))
			{
				changed = addItem( messages[i] ) || changed;
			}
		}
	
		if (changed && pImpl_->widthIncreased_)
		{
			updateColumnWidths( true );
			pImpl_->widthIncreased_ = false;
		}

		pImpl_->lastMessage = last;
	}

	return 0;
}

void PageMessages::OnBnClickedErrorsShowDate()
{
	BW_GUARD;

	toggleOption( DATE_OPTION_PATH );
	updateColumnWidths( true );
}

void PageMessages::OnBnClickedErrorsShowTime()
{
	BW_GUARD;

	toggleOption( TIME_OPTION_PATH );
	updateColumnWidths( true );
}

void PageMessages::OnBnClickedErrorsShowPriority()
{
	BW_GUARD;

	toggleOption( PRIORITY_OPTION_PATH );
	updateColumnWidths( true );
}

void PageMessages::OnBnClickedErrorsError()
{
	BW_GUARD;

	toggleOption( ERRORS_OPTION_PATH );
}

void PageMessages::OnBnClickedErrorsWarning()
{
	BW_GUARD;

	toggleOption( WARNING_OPTION_PATH );
}

void PageMessages::OnBnClickedErrorsNotice()
{
	BW_GUARD;

	toggleOption( NOTICE_OPTION_PATH );
}

void PageMessages::OnBnClickedErrorsInfo()
{
	BW_GUARD;

	toggleOption( INFO_OPTION_PATH );
}
void PageMessages::OnBnClickedMsgAssets()
{
	BW_GUARD;

	toggleOption( ASSET_OPTION_PATH );
}

// The following methods are for tool specific implementations used by the Messages panel
// to implement click and redraw events.

void PageMessages::msgsImpl( MsgsImpl* msgsImpl )
{
	BW_GUARD;

	// Make sure we remove the old implementation if one exists
	bw_safe_delete(pImpl_->msgsImpl); 

	pImpl_->msgsImpl = msgsImpl;
}

MsgsImpl* PageMessages::msgsImpl()
{
	return pImpl_->msgsImpl;
}

void PageMessages::OnNMClickMsgList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	if (pImpl_->msgsImpl)
	{
		pImpl_->msgsImpl->OnNMClickMsgList( pNMHDR, pResult );
	}
}

void PageMessages::OnNMCustomdrawMsgList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	if (pImpl_->msgsImpl)
	{
		pImpl_->msgsImpl->OnNMCustomdrawMsgList( pNMHDR, pResult );
	}
}

BW_END_NAMESPACE

