
#include "pch.hpp"
#include "gui/pages/post_processing_properties.hpp"
#include "gui/post_processing/base_post_processing_node.hpp"
#include "gui/post_processing/post_proc_property_editor.hpp"
#include "common/editor_views.hpp"
#include "common/user_messages.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
PostProcessingProperties::PostProcessingProperties() :
	CDialogPropertyTable( PostProcessingProperties::IDD ),
	listRightPadding_( 0 ),
	listBottomPadding_( 0 ),
	inited_( false )
{
}


/**
 *	Destructor.
 */
PostProcessingProperties::~PostProcessingProperties()
{
}


/**
 *	This method extracts all the properties from the node and puts them into
 *	the property list for editing.
 *
 *	@param node	Node to edit, or NULL to clear the properties list.
 */
void PostProcessingProperties::editNode( BasePostProcessingNodePtr node )
{
	BW_GUARD;

	PropTableSetter setter( this );

	if (editor_)
	{
		editor_->expel();
		editor_ = NULL;
	}

	if (node)
	{
		if (node->effectNode())
		{
			INFO_MSG( "EffectNode '%s' selected\n", node->name().c_str() );
		}
		else if (node->phaseNode())
		{
			INFO_MSG( "PhaseNode '%s' selected\n", node->name().c_str() );
		}

		editor_ = PostProcPropertyEditorPtr( new PostProcPropertyEditor( node ), true );
		
		editor_->elect();
	}
	else
	{
		INFO_MSG( "No node selected\n" );
	}
}


/**
 *	This MFC method syncs the Windows dialog controls with the corresponding
 *	member variables.
 *
 *	@param pDX	MFC data exchange struct.
 */
void PostProcessingProperties::DoDataExchange( CDataExchange * pDX )
{
	CDialogPropertyTable::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_PP_PROPERIES_LABEL, label_);
}


/**
 *	This method is called when this window is being initialised.
 *
 *	@return	TRUE to signal success.
 */
BOOL PostProcessingProperties::OnInitDialog()
{
	BW_GUARD;

	BOOL ok = CDialogPropertyTable::OnInitDialog();

	if (ok == FALSE)
	{
		return ok;
	}

	INIT_AUTO_TOOLTIP();

	CRect thisRect;
	GetClientRect( thisRect );

	CRect listRect;
	propertyList()->GetWindowRect( listRect );
	ScreenToClient( listRect );

	listRightPadding_ = thisRect.right - listRect.right;
	listBottomPadding_ = thisRect.bottom - listRect.bottom;

	inited_ = true;

	return TRUE;
}


// MFC message map.
BEGIN_MESSAGE_MAP( PostProcessingProperties, CDialogPropertyTable )
	ON_WM_SIZE()
	ON_MESSAGE( WM_SELECT_PROPERTYITEM, OnSelectPropertyItem )
	ON_MESSAGE( WM_CHANGE_PROPERTYITEM, OnChangePropertyItem )
	ON_MESSAGE( WM_DBLCLK_PROPERTYITEM, OnDblClkPropertyItem )
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden to adjust the size of the property list
 *	contained in this window.
 *
 *	@param nType	MFC resize type.
 *	@param cx	New width.
 *	@param cy	New height.
 */
void PostProcessingProperties::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	if (inited_)
	{
		CDialogPropertyTable::OnSize( nType, cx, cy );
	
		CRect listRect;
		propertyList()->GetWindowRect( listRect );
		ScreenToClient( listRect );

		int listW = cx - listRect.left - listRightPadding_;
		int listH = cy - listRect.top - listBottomPadding_;

		propertyList()->SetWindowPos( NULL, 0, 0, listW, listH, SWP_NOZORDER | SWP_NOMOVE | SWP_NOCOPYBITS );
		
		label_.RedrawWindow();
		RedrawWindow();
	}
}


/**
 *	This method is called when a list item has been selected.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, property view sending the notification.
 *	@return	MFC return value, 0.
 */
LRESULT PostProcessingProperties::OnSelectPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onSelect();
	}

	return 0;
}


/**
 *	This method is called when a list item's value has been changed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, property view sending the notification.
 *	@return	MFC return value, 0.
 */
LRESULT PostProcessingProperties::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onChange(wParam != 0);
	}

	return 0;
}


/**
 *	This method is called when a list item has been double-clicked.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, property sending the notification.
 *	@return	MFC return value, 0.
 */
LRESULT PostProcessingProperties::OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		PropertyItem* relevantView = (PropertyItem*)lParam;
		relevantView->onBrowse();
	}
	
	return 0;
}
BW_END_NAMESPACE

