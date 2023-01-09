#include "pch.hpp"
#include "worldeditor/gui/pages/page_properties.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/gui/scene_browser/scene_browser.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "common/editor_views.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "common/user_messages.hpp"
#include "common/utilities.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// PageProperties
PageProperties * PageProperties::instance_ = NULL;


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageProperties::contentID = L"PageProperties";



PageProperties::PageProperties()
	: PropertyTable(PageProperties::IDD)
	, rclickItem_( NULL )
	, inited_( false )
{
	BW_GUARD;

	ASSERT(!instance_);
	instance_ = this;
	PropTable::table( this );
}

PageProperties::~PageProperties()
{
	BW_GUARD;

	ASSERT(instance_);
	instance_ = NULL;
}

PageProperties& PageProperties::instance()
{
	ASSERT(instance_);
	return *instance_;
}


void PageProperties::DoDataExchange(CDataExchange* pDX)
{
	PropertyTable::DoDataExchange(pDX);
}

void PageProperties::addItems()
{
	BW_GUARD;

	propertyList()->SetRedraw(FALSE);

	propertyList()->clear();

	for (BW::list<BaseView*>::iterator vi = viewList().begin();
		vi != viewList().end();
		vi++)
	{
		addItemsForView(*vi);
	}

	propertyList()->SetRedraw(TRUE);
}


BEGIN_MESSAGE_MAP(PageProperties, CFormView)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_MESSAGE(WM_DEFAULT_PANELS , OnDefaultPanels ) 
	ON_MESSAGE(WM_LAST_PANELS    , OnDefaultPanels ) 
	ON_MESSAGE(WM_SELECT_PROPERTYITEM, OnSelectPropertyItem)
	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_MESSAGE(WM_DBLCLK_PROPERTYITEM, OnDblClkPropertyItem)
	ON_MESSAGE(WM_RCLK_PROPERTYITEM, OnRClkPropertyItem)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// PageProperties message handlers
void PageProperties::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	if (inited_)
	{
		Utilities::stretchToBottomRight( this, *propertyList(), cx, 12, cy, 12 );

		PropertyTable::OnSize( nType, cx, cy );
	}
}

afx_msg void PageProperties::OnClose()
{
	BW_GUARD;

	clear();
	CFormView::OnClose();
}

afx_msg LRESULT PageProperties::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!inited_)
	{	
		// pre allocate 1000 strings of about 16 char per string
		propertyList()->InitStorage(1000, 16);
		propertyList()->isSorted( true );

		CString name;
		GetWindowText(name);
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", bw_wtoutf8( name.GetBuffer() ) );
		}

		// add any current properties to the list
		addItems();

		RECT rect;

		GetClientRect( &rect );
		Utilities::stretchToBottomRight( this, *propertyList(), rect.right, 12, rect.bottom, 12 );

		inited_ = true;
	}
	
	if ( !IsWindowVisible() )
		return 0;

	{
		static DogWatch dw( "PropertiesUpdate" );
		ScopedDogWatch sdw( dw );
		WorldManager::ScopedDeferSelectionReplacement deferSelectionReplacement;
		update( 4 /* interleave by 4 */, 5 /* maximum of 5 millisecods */ );
	}

	return 0;
}


afx_msg LRESULT PageProperties::OnDefaultPanels(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (!inited_)
		return 0;

	// Make sure the property list gets cleared.
	GeneralEditor::Editors newEds;
	GeneralEditor::currentEditors(newEds);

	return 0;
}


afx_msg LRESULT PageProperties::OnSelectPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	GizmoManager::instance().forceGizmoSet( NULL );

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onSelect();
	}

	return 0;
}

afx_msg LRESULT PageProperties::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onChange(wParam != 0);
	}

	return 0;
}

afx_msg LRESULT PageProperties::OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		PropertyItem* relevantView = (PropertyItem*)lParam;
		relevantView->onBrowse();
	}
	
	return 0;
}

afx_msg LRESULT PageProperties::OnRClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	PropertyItem * item = (PropertyItem*)(lParam);
	if (!item)
		return 0;

	rclickItem_ = item;

	BaseView * view = (BaseView *)rclickItem_->getChangeBuddy();
	if (!view)
		return 0;

	PropertyManagerPtr propManager = view->getPropertyManager();
	if (!propManager)
		return 0;

	if (propManager->canAddItem())
	{
		CMenu menu;
		menu.LoadMenu( IDR_PROPERTIES_LIST_POPUP );
		CMenu* pPopup = menu.GetSubMenu(0);

		POINT pt;
		::GetCursorPos( &pt );
		pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y,
			AfxGetMainWnd() );
	}
	else if (propManager->canRemoveItem())
	{
		CMenu menu;
		menu.LoadMenu( IDR_PROPERTIES_LISTITEM_POPUP );
		CMenu* pPopup = menu.GetSubMenu(0);

		POINT pt;
		::GetCursorPos( &pt );
		pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y,
			AfxGetMainWnd() );
	}

	return 0;
}

void PageProperties::OnListAddItem()
{
	BW_GUARD;

	// add a new item to the selected list
	BaseView * view = (BaseView *)rclickItem_->getChangeBuddy();
	PropertyManagerPtr propManager = view->getPropertyManager();
	if (propManager)
	{
		propManager->addItem();
	}
}

void PageProperties::OnListItemRemoveItem()
{
	BW_GUARD;

	// remove the item from the list
	BaseView * view = (BaseView *)rclickItem_->getChangeBuddy();
	PropertyManagerPtr propManager = view->getPropertyManager();
	if (propManager)
	{
		propManager->removeItem();
	}
}

BOOL PageProperties::PreTranslateMessage(MSG* pMsg)
{
	BW_GUARD;

    if(pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_TAB)
		{
			if( GetAsyncKeyState( VK_SHIFT ) )
				propertyList()->needsSelectPrevItem();
			else
				propertyList()->needsSelectNextItem();
			return true;
		}
        if (pMsg->wParam == VK_RETURN)
		{
			propertyList()->needsDeselectCurrentItem();
		}
    }

	return CFormView::PreTranslateMessage(pMsg);
}


void PageProperties::adviseSelectedId( BW::string id )
{
	BW_GUARD;

	if (!GetSafeHwnd())     // see if this page has been created
		return;

/*	CPropertySheet* parentSheet = (CPropertySheet *)GetParent();

	if (parentSheet->GetActivePage() != this)
		return; */

	PropertyItem * hItem = propertyList()->getHighlightedItem();
	if ((hItem == NULL) || (hItem->getType() != PropertyItem::Type_ID))
		return;

	StaticTextView * view = (StaticTextView*)(hItem->getChangeBuddy());
	view->setCurrentValue( bw_utf8tow( id ) );
}

BW_END_NAMESPACE

