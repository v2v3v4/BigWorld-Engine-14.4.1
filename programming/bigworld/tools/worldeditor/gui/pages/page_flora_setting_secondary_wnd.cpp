#include "pch.hpp"
#include "ual_manager.hpp"
#include "page_flora_setting_tree.hpp"
#include "page_flora_setting.hpp"
#include "page_flora_setting_secondary_wnd.hpp"
#include "moo/texture_manager.hpp"
#include "../../lib/ual/ual_resource.h"
#include "common/utilities.hpp"

BW_BEGIN_NAMESPACE

static const int GROUP_WND_INNER_SPACE = 6;
static const int GROUP_WND_OUTER_SPACE = 3;
static const int GROUP_WND_TEXT_HEIGHT = 10;
static const int GROUP_WND_HEIGHT_ERROR = 2;
static const int MINIMUM_CHECBOX_WIDTH = 90;
static const int MINIMUM_LIST_WIDTH = 90;
static const int MINIMUM_LIST_HEIGHT = 110;
static const int RESOURCE_IMAGE_SIZE = 140;
static const int MINIMUM_DESCRIPTION_WIDTH = 100;
static const int MINIMUM_ERROR_WIDTH = 100;
static const int MINIMUM_FLOATBAR_WIDTH = 100;
static const int SPLITTER_WND_BORDER_SPACE = 3;
static const int SPLITTER_SIZER_SPACE = 4;

/////////////////////FloraSettingFloatBarWnd begin///////////////////////
IMPLEMENT_DYNAMIC(FloraSettingFloatBarWnd, CDialog)


BEGIN_MESSAGE_MAP(FloraSettingFloatBarWnd, CDialog)
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE(IDC_FLORA_SETTING_FLOAT_VALUE, OnEnChangeValueEdit)
END_MESSAGE_MAP()

FloraSettingFloatBarWnd::FloraSettingFloatBarWnd( CWnd* pParent /*=NULL*/ ):
	CDialog( FloraSettingFloatBarWnd::IDD, pParent),
	changeFromScrollBarConfirmed_( false ),
	changeFromScrollBar_( false )
{
	BW_GUARD;
	valueEdit_.SetNumericType(controls::EditNumeric::ENT_FLOAT);
	valueEdit_.SetAllowNegative( false );
	valueEdit_.SetMinimum( 0.f );
	valueEdit_.SetMaximum( 1.f );
}


void FloraSettingFloatBarWnd::init( PageFloraSetting* pPageWnd )
{
	BW_GUARD;
	pPageWnd_ = pPageWnd;
}


void FloraSettingFloatBarWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FLORA_SETTING_FLOAT_SLIDER, sliderBar_);
	DDX_Control(pDX, IDC_FLORA_SETTING_FLOAT_VALUE, valueEdit_);
}


/**
 *	This function set up the properties.
 */
void FloraSettingFloatBarWnd::onActive( HTREEITEM hItem )
{
	BW_GUARD;
	wchar_t propertyName[256];
	FloatSection* pSection = dynamic_cast<FloatSection*>
								( pPageWnd_->tree().getItemSection( hItem ) );
	if (pSection != NULL)
	{
		pSection_ = pSection;
		hItem_ = hItem;

		pSection->getNameAndValue( hItem, propertyName,
			ARRAY_SIZE( propertyName ), value_ );

		// property name:
		this->SetDlgItemText( IDC_FLORA_SETTING_FLOAT_STATIC_TEXT, propertyName );

		// Slider bar:
		min_ = pSection->min();
		max_ = pSection->max();
		sliderBar_.SetRange( 0 , 100 );
		int nValue = ( int ) ( value_ * 100/(max_ - min_) );
		sliderBar_.SetPos( nValue );

		// Value:
		valueEdit_.SetMinimum( min_ );
		valueEdit_.SetMaximum( max_ );
		valueEdit_.SetValue( value_ );
	}
}


/**
 *	This MFC event handler when user clicks a window's horizontal scroll bar.
 */
void FloraSettingFloatBarWnd::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	BW_GUARD;

	if (pScrollBar->GetDlgCtrlID() == IDC_FLORA_SETTING_FLOAT_SLIDER)
	{
		FloatSection* pSection = dynamic_cast<FloatSection*>( pSection_ );
		if (pSection != NULL)
		{
			int nMin, nMax;
			sliderBar_.GetRange( nMin, nMax );
			int nPos = sliderBar_.GetPos();
			float fValue = (float)nPos * (max_ - min_)/ (float)(nMax - nMin);
			valueEdit_.SetValue( fValue );
			//OnEnChangeValueEdit will update the rest
			changeFromScrollBar_ = true;
			changeFromScrollBarConfirmed_ = (nSBCode == SB_ENDSCROLL);
		}
	}
	CDialog::OnHScroll( nSBCode, nPos, pScrollBar );
}



/*afx_msg*/ HBRUSH FloraSettingFloatBarWnd::OnCtlColor( CDC* pDC, CWnd* pWnd, 
																UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );
	valueEdit_.SetBoundsColour( pDC, pWnd,
		valueEdit_.GetMinimum(), valueEdit_.GetMaximum() );

	return brush;
}


/*afx_msg*/ void FloraSettingFloatBarWnd::OnEnChangeValueEdit()
{
	BW_GUARD;
	float fNewValue = valueEdit_.GetValue();
	if (fabs( fNewValue - value_ ) < 0.01)
	{
		// We ignore the small tiny change,otherwise too many
		// undo will be added, plus float tree item is %.2f formated
		return;
	}
	value_ = fNewValue;

	int nMin, nMax;
	sliderBar_.GetRange( nMin, nMax );
	int nPos =
		(int ) ( (float)(nMax - nMin) * value_/(max_ - min_) );
	sliderBar_.SetPos( nPos );

	FloatSection* pSection = dynamic_cast<FloatSection*>( pSection_ );
	if (pSection != NULL)
	{
		wchar_t name[ 256 ];
		float oldValue;
		pSection->getNameAndValue( hItem_, name, ARRAY_SIZE( name ), oldValue );

		pPageWnd_->preChange();
		pSection->setNameAndValue( hItem_, pSection->name(), value_ );
		pPageWnd_->postChange();

		if (!changeFromScrollBar_ ||
			changeFromScrollBarConfirmed_)
		{
			// Add undo
			FloatValueOperation* pOperation = new FloatValueOperation( 
				&(pPageWnd_->tree()), hItem_, oldValue, this );
			UndoRedo::instance().add( pOperation );
			UndoRedo::instance().barrier( LocaliseUTF8( 
				L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/CHANGE_ITEM_VALUE"), 
				false );

			changeFromScrollBar_ = false;
			changeFromScrollBarConfirmed_ = false;
		}
	}
}

/////////////////////FloraSettingAssetWnd begin///////////////////////
IMPLEMENT_DYNAMIC(FloraSettingAssetWnd, CDialog)


FloraSettingAssetWnd::FloraSettingAssetWnd( PageFloraSetting* pPageWnd,
														CWnd* pParent /*=NULL*/)
	: CDialog( FloraSettingAssetWnd::IDD, pParent),
	pPageWnd_( pPageWnd ),
	assetList_( &UalManager::instance().thumbnailManager() ),
	pSection_( NULL ),
	hItem_( NULL )
{
	BW_GUARD;
}


BEGIN_MESSAGE_MAP(FloraSettingAssetWnd, CDialog)
	ON_WM_SIZE()
	ON_MESSAGE( WM_SEARCHFIELD_CHANGE, OnSearchFieldChanged )
END_MESSAGE_MAP()



void FloraSettingAssetWnd::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FLORA_SETTING_ASSET_LIST, assetList_ );
	DDX_Control(pDX, IDC_FLORA_SEARCHBK, search_);
}


/**
 *	This MFC event handler when the dialog is intializing.
 */
BOOL FloraSettingAssetWnd::OnInitDialog()
{
	BW_GUARD;
	BOOL ret = CDialog::OnInitDialog();

	BW::string configFile;
	bw_wtoutf8( UalManager::instance().getConfigFile(), configFile );
	DataSectionPtr pConfigXMLDataSect = 
						BWResource::instance().openSection( configFile, false );
	DataSectionPtr pConfigDataSect = pConfigXMLDataSect->openSection( "Config" );
	int maxCache = pConfigDataSect->readInt( "maxCacheItems", 200 );
	if ( maxCache < 0 )
		ERROR_MSG( "PageFloraSetting::InitPage, "
				"invalid maxCacheItems. Should be greater or equal to zero." );
	else
		assetList_.setMaxCache( maxCache );
	assetList_.SetIconSpacing(
		pConfigDataSect->readInt( "iconSpacingX", 90 ),
		pConfigDataSect->readInt( "iconSpacingY", 100 )
		);


	search_.init( MAKEINTRESOURCE( IDB_UALMAGNIFIER ),
		MAKEINTRESOURCE( IDB_UALSEARCHCLOSE ), L"",
		Localise(L"UAL/UAL_DIALOG/TOOLTIP_SEARCH_FILTERS"),
		Localise(L"UAL/UAL_DIALOG/TOOLTIP_SEARCH") );

	search_.idleText( Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DEFAULT_SEARCH_IDLE_TEXT") );

	filterHolder_.addFilter( 
						new ValidTextureFilterSpec( 
							BW::wstring( FLORA_VISUAL_TEXTURE_FILTER_NAME ), false )
							);


	filtersCtrl_.Create(
		AfxRegisterWndClass( 0, 0, GetSysColorBrush( COLOR_BTNFACE ), 0 ),
		L"", WS_VISIBLE | WS_CHILD, CRect( 0, 0, 1, 1 ), this, 0 );
	filtersCtrl_.setEventHandler( this );

	this->buildAssetListFilters();

	return ret;
}


void FloraSettingAssetWnd::OnOK()
{
	BW_GUARD;
}


/**
 *	This method is called when a filter is clicked, which requires an update
 *	of the list.
 *
 *	@param name		Name of the filter.
 *	@param pushed	State of the filter, true if checked, false if not.
 *	@param data		Filter specification object corresponding to the filter
 *					being clicked.
 */
void FloraSettingAssetWnd::filterClicked( const wchar_t* name, bool pushed, void* data )
{
	BW_GUARD;

	FilterSpecPtr filter = (FilterSpec*)data;
	filter->setActive( pushed );
	assetList_.updateFilters();
}


/**
 *	This method initializes the filters.
 */
void FloraSettingAssetWnd::buildAssetListFilters()
{
	int i = 0;
	FilterSpecPtr filter = 0;
	filtersCtrl_.clear();
	while (filter = filterHolder_.getFilter( i++ ))
	{
		if (filter->getName().length())
		{
			filtersCtrl_.add( filter->getName().c_str(), filter->getActive(), 
													(void*)filter.getObject() );
		}
		else
		{
			filtersCtrl_.addSeparator();
		}
	} 
}

/**
*	This function is called when we are about to be active.
*	@param hItem	 the tree control item who triggered this event.
*/
void FloraSettingAssetWnd::onActive( HTREEITEM hItem )
{
	BW_GUARD;
	hItem_ = hItem;
	pSection_ = pPageWnd_->tree().getItemSection( hItem_ );
	assetListprovider_ = pSection_->prepareAssetListProvider( hItem_, filterHolder_);
	assetList_.init( assetListprovider_ );

	if (assetListprovider_)
	{
		search_.searchText( L"" );
		assetListprovider_->setFilterHolder( &filterHolder_ );
		this->buildAssetListFilters();
		assetList_.updateFilters();
	}

	this->layOut();
}


/**
 *	This method is called when the search text changes, which requires an
 *	update of the list.
 *
 *	@param wParam	MFC param, ignored.
 *	@param lParam	MFC param, ignored.
 *	@result		MFC return 0.
 */
LRESULT FloraSettingAssetWnd::OnSearchFieldChanged( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	filterHolder_.setSearchText( search_.searchText() );
	assetList_.updateFilters();
	return 0;
}


/**
*	This function is called when we are about to be deactive.
*	@param hItem	 the tree control item who triggered this event.
*/
void FloraSettingAssetWnd::onDeactive( HTREEITEM hItem )
{
	BW_GUARD;

	if (assetListprovider_)
	{
		filterHolder_.activateAll( false );
	}
	assetList_.init( NULL );
	assetList_.UpdateWindow();

}


/**
 *  This method returns the minimum usable width.
 */
int FloraSettingAssetWnd::minimumWidth()
{
	int width = 0;

	if (assetListprovider_)
	{
		if (search_.GetSafeHwnd())
		{
			CRect rect;
			search_.GetWindowRect(rect);
			width = rect.Width();
		}

		if (filterHolder_.hasActiveFilters() && filtersCtrl_.GetSafeHwnd())
		{
			width = max(width, MINIMUM_CHECBOX_WIDTH);
		}

		width = max(width, MINIMUM_LIST_WIDTH);
	}

	width += GROUP_WND_INNER_SPACE * 2 + GROUP_WND_OUTER_SPACE * 2;
	return width;
}


/**
 *  This method returns the minimum usable height.
 */
int FloraSettingAssetWnd::minimumHeight()
{
	int height = 0;

	if (assetListprovider_)
	{
		if (search_.GetSafeHwnd())
		{
			CRect rect;
			search_.GetWindowRect(rect);
			height += rect.Height() + GROUP_WND_INNER_SPACE;
		}

		if (filterHolder_.hasActiveFilters() && filtersCtrl_.GetSafeHwnd())
		{
			height += filtersCtrl_.getHeight() + GROUP_WND_INNER_SPACE;
		}

		height += MINIMUM_LIST_HEIGHT + GROUP_WND_INNER_SPACE;
	}

	height += GROUP_WND_TEXT_HEIGHT + GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE * 2;
	return height;
}


/**
 *	This method show the related windows, and hide unrelated windows.
 *	and lay out them.
 */
void FloraSettingAssetWnd::layOut()
{
	BW_GUARD;
	if (assetList_.GetSafeHwnd() == NULL)
	{		
		return;// not created yet.
	}

	CWnd* groupFrame = GetDlgItem( IDC_FLORA_SETTING_ASSET_GROUP );
	Utilities::stretch( this, *groupFrame, GROUP_WND_OUTER_SPACE );
	groupFrame->ShowWindow( SW_SHOW );

	int xPos = GROUP_WND_OUTER_SPACE + GROUP_WND_INNER_SPACE;
	int yPos = GROUP_WND_OUTER_SPACE + GROUP_WND_INNER_SPACE + GROUP_WND_TEXT_HEIGHT + GROUP_WND_HEIGHT_ERROR;

	//asset list:
	if (assetListprovider_)
	{
		// Search edit box 
		if (search_.GetSafeHwnd())
		{
			yPos = Utilities::moveAndStretchWidth( this, search_, xPos, yPos, GROUP_WND_OUTER_SPACE + GROUP_WND_INNER_SPACE );
			yPos += GROUP_WND_INNER_SPACE;
			search_.ShowWindow( SW_SHOW );
		}

		// Filter check boxes
		if (filterHolder_.hasActiveFilters() && filtersCtrl_.GetSafeHwnd() )
		{
			CRect rect;
			this->GetClientRect( rect );
			int totalWidth = rect.Width() - GROUP_WND_OUTER_SPACE * 2;

			filtersCtrl_.ShowWindow( SW_SHOW );
			filtersCtrl_.recalcWidth( totalWidth );
			Utilities::moveAndSize( filtersCtrl_, xPos, yPos, totalWidth, filtersCtrl_.getHeight() );
			yPos += filtersCtrl_.getHeight() + GROUP_WND_INNER_SPACE;
		}
		else
		{
			filtersCtrl_.ShowWindow( SW_HIDE );
		}

		// Asset list
		Utilities::moveAndStretch( this, assetList_, xPos, yPos,
				GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE,
				GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE );
		assetList_.ShowWindow( SW_SHOW );
	}
	else
	{
		search_.ShowWindow( SW_HIDE );
		filtersCtrl_.ShowWindow( SW_HIDE );
		assetList_.ShowWindow( SW_HIDE );
	}
}


/**
*	This MFC event handler changes the layout of the dialog's controls to best
*	fit the window shape and size.
*
*	@param nType	MFC resize type
*	@param cx		New dialog width.
*	@param cy		New dialog height.
*/
void FloraSettingAssetWnd::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;
	this->layOut();
}


/////////////////////FloraSettingPropertiesWnd begin///////////////////////
IMPLEMENT_DYNAMIC(FloraSettingPropertiesWnd, CDialog)


FloraSettingPropertiesWnd::FloraSettingPropertiesWnd(
	PageFloraSetting* pPageWnd, CWnd* pParent /*=NULL*/)
	: CDialog( FloraSettingPropertiesWnd::IDD, pParent),
	pPageWnd_( pPageWnd ),
	existingResourceList_( &UalManager::instance().thumbnailManager() ),
	pSection_( NULL ),
	hItem_( NULL ),
	usedFloatBarWnds_( 0 ),
	showCurDisplayResource_( false ),
	showError_( false )
{
	BW_GUARD;
}


BEGIN_MESSAGE_MAP(FloraSettingPropertiesWnd, CDialog)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


void FloraSettingPropertiesWnd::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FLORA_SETTING_EXIST_RESOURCE_LIST, existingResourceList_ );
	DDX_Control(pDX, IDC_FLORA_SETTING_CUR_DISPLAY_RESOURCE_WND, curDisplayImage_ );
}


/*afx_msg*/ HBRUSH FloraSettingPropertiesWnd::OnCtlColor( CDC* pDC, CWnd* pWnd, 
																UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );
	if (pWnd->GetDlgCtrlID() == IDC_FLORA_SETTING_ERROR_MESSAGE)
	{
		pDC->SetTextColor( RGB( 255, 0, 0 ) );
	}
	return brush;
}

/**
 *	This MFC event handler when the dialog is intializing.
 */
BOOL FloraSettingPropertiesWnd::OnInitDialog()
{
	BW_GUARD;
	BOOL ret = CDialog::OnInitDialog();

	BW::string configFile;
	bw_wtoutf8( UalManager::instance().getConfigFile(), configFile );
	DataSectionPtr pConfigXMLDataSect = 
						BWResource::instance().openSection( configFile, false );
	DataSectionPtr pConfigDataSect = pConfigXMLDataSect->openSection( "Config" );
	int maxCache = pConfigDataSect->readInt( "maxCacheItems", 200 );
	if ( maxCache < 0 )
	{
		ERROR_MSG( "PageFloraSetting::InitPage, invalid maxCacheItems. "
										"Should be greater or equal to zero." );
	}
	else
	{
		existingResourceList_.setMaxCache( maxCache );
	}
	existingResourceList_.SetIconSpacing(
		pConfigDataSect->readInt( "iconSpacingX", 90 ),
		pConfigDataSect->readInt( "iconSpacingY", 100 )
		);

	// Float bars
	for (int i = 0; i < MAX_FLOAT_BAR_WND; ++i)
	{
		floatBarWnds[ i ].Create( IDD_FLORA_SETTING_FLOAT_WND, this );
		floatBarWnds[ i ].init( pPageWnd_ );
	}
	return ret;
}


/**
 *	This MFC method relays double-click events to the event handler.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void FloraSettingPropertiesWnd::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	BW_GUARD;

	if (showCurDisplayResource_ && pSection_)
	{
		CRect rc;
		curDisplayImage_.GetWindowRect( &rc );
		this->ScreenToClient( rc );
		if (rc.PtInRect( point ))
		{
			MF_ASSERT( hItem_ );
			pSection_->onDoubleClicked( hItem_ );
		}
	}
}


void FloraSettingPropertiesWnd::updateCurDisplayImage()
{
	BW_GUARD;
	BW::string thumb;
	BW::wstring wError;
	wchar_t  wDiscription[256];
	showCurDisplayResource_ = pSection_->prepareCurDisplayImage( hItem_, 
													wDiscription, 256, thumb );
	if (showCurDisplayResource_)
	{
		Moo::BaseTexturePtr pBaseTexture = 
			Moo::TextureManager::instance()->get( thumb, true, false, true );
		if (pBaseTexture != NULL &&
			pBaseTexture->pTexture()->GetType() == D3DRTYPE_TEXTURE )
		{
			controls::DibSection32 newTexture;
			if (newTexture.copyFromTexture( pBaseTexture ))
			{
				curDisplayImage_.image( newTexture );
			}
		}

		this->SetDlgItemText( IDC_FLORA_SETTING_CUR_DISPLAY_IMAGE_DESCRIPTION,
																wDiscription );
	}
	showError_ = pSection_->prepareEorrorMessage( hItem_, wError );
	if (showError_)
	{
		this->SetDlgItemText( IDC_FLORA_SETTING_ERROR_MESSAGE, wError.c_str() );
		WARNING_MSG( "%S", wError.c_str() );
	}
}


/**
 *  This method returns the minimum usable width.
 */
int FloraSettingPropertiesWnd::minimumWidth()
{
	int width = 0;

	if (showCurDisplayResource_)
	{
		width = RESOURCE_IMAGE_SIZE + GROUP_WND_INNER_SPACE +
				MINIMUM_DESCRIPTION_WIDTH;
	}

	if (showError_)
	{
		width = max(width, MINIMUM_ERROR_WIDTH);
	}

	for (int i = 0; i < usedFloatBarWnds_; ++i)
	{		
		width = max(width, MINIMUM_FLOATBAR_WIDTH);
	}

	if (existingResourceListprovider_)
	{
		width = max(width, MINIMUM_LIST_WIDTH);
	}

	width += GROUP_WND_INNER_SPACE * 2 + GROUP_WND_OUTER_SPACE * 2;
	return width;
}


/**
 *  This method returns the minimum usable height.
 */
int FloraSettingPropertiesWnd::minimumHeight()
{
	int height = 0;
	CRect rect;

	if (showCurDisplayResource_)
	{
		height += RESOURCE_IMAGE_SIZE + GROUP_WND_INNER_SPACE;
	}

	if (showError_)
	{
		this->GetDlgItem(IDC_FLORA_SETTING_ERROR_MESSAGE)->GetClientRect(rect);
		height += rect.Height() + GROUP_WND_INNER_SPACE;
	}

	for (int i = 0; i < usedFloatBarWnds_; ++i)
	{		
		floatBarWnds[i].GetClientRect(rect);
		height += rect.Height() + GROUP_WND_INNER_SPACE;
	}

	if (existingResourceListprovider_)
	{
		height += MINIMUM_LIST_HEIGHT;
	}

	height += GROUP_WND_TEXT_HEIGHT + GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE * 2;
	return height;
}


/**
*	This function is called when we are about to be active.
*	@param hItem	 the tree control item who triggered this event.
*/
void FloraSettingPropertiesWnd::onActive( HTREEITEM hItem )
{
	BW_GUARD;
	hItem_ = hItem;
	pSection_ = pPageWnd_->tree().getItemSection( hItem_ );
	// Exist resource:
	existingResourceListprovider_ = 
							pSection_->prepareExistingResourceProvider( hItem );
	existingResourceList_.init( existingResourceListprovider_ );

	// Current display image:
	this->updateCurDisplayImage();

	// Float bars:
	usedFloatBarWnds_ = 0;
	HTREEITEM hChildItem = pPageWnd_->tree().GetChildItem( hItem );
	while (hChildItem != NULL)
	{
		bool lookForChild = true;
		FloraSettingTreeSection* pChildSection =
			pPageWnd_->tree().getItemSection( hChildItem );
		if (pChildSection && pChildSection->isFloatResource( hChildItem ))
		{
			floatBarWnds[ usedFloatBarWnds_++ ].onActive( hChildItem );
		}
		hChildItem = pPageWnd_->tree().GetNextItem( hChildItem, TVGN_NEXT );
	}
	this->layOut();
}


/**
*	This function is called when we are about to be deactive.
*	@param hItem	 the tree control item who triggered this event.
*/
void FloraSettingPropertiesWnd::onDeactive( HTREEITEM hItem )
{
	BW_GUARD;
	existingResourceList_.init( NULL );
	existingResourceList_.UpdateWindow();

	showCurDisplayResource_ = false;
	usedFloatBarWnds_ = 0;
	showError_ = false;
}


/**
 *	This method show the related windows, and hide unrelated windows.
 *	and lay out them.
 */
void FloraSettingPropertiesWnd::layOut()
{
	BW_GUARD;
	if (existingResourceList_.GetSafeHwnd() == NULL)
	{
		// not created yet.
		return;
	}

	CWnd* groupFrame = GetDlgItem( IDC_FLORA_SETTING_ASSET_PROPERTIES_GROUP );
	Utilities::stretch( this, *groupFrame, GROUP_WND_OUTER_SPACE );
	groupFrame->ShowWindow( SW_SHOW );

	int xPos = GROUP_WND_OUTER_SPACE + GROUP_WND_INNER_SPACE;
	int yPos = GROUP_WND_OUTER_SPACE + GROUP_WND_INNER_SPACE + GROUP_WND_TEXT_HEIGHT + GROUP_WND_HEIGHT_ERROR;

	// visual texture
	CWnd* imageDescription = GetDlgItem( IDC_FLORA_SETTING_CUR_DISPLAY_IMAGE_DESCRIPTION );

	if (showCurDisplayResource_)
	{
		Utilities::moveAndSize( curDisplayImage_, xPos, yPos, RESOURCE_IMAGE_SIZE, RESOURCE_IMAGE_SIZE );
		curDisplayImage_.ShowWindow( SW_SHOW );

		Utilities::moveAndStretchWidth( this, *imageDescription,
				xPos + RESOURCE_IMAGE_SIZE + GROUP_WND_INNER_SPACE, yPos,
				GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE );
		yPos += RESOURCE_IMAGE_SIZE + GROUP_WND_INNER_SPACE;
		imageDescription->ShowWindow( SW_SHOW );
	}
	else
	{
		curDisplayImage_.ShowWindow( SW_HIDE );
		imageDescription->ShowWindow( SW_HIDE );
	}

	// error messages
	CWnd* errorMessage = GetDlgItem( IDC_FLORA_SETTING_ERROR_MESSAGE );

	if (showError_)
	{
		yPos = Utilities::moveAndStretchWidth( this, *errorMessage, xPos, yPos,
				GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE );
		yPos += GROUP_WND_INNER_SPACE;
		errorMessage->ShowWindow( SW_SHOW );
	}
	else
	{
		errorMessage->ShowWindow( SW_HIDE );
	}

	// float bars
	for (int i = 0; i < usedFloatBarWnds_; ++i)
	{
		yPos = Utilities::move( floatBarWnds[i], xPos, yPos ).y;
		yPos += GROUP_WND_INNER_SPACE;
		floatBarWnds[i].ShowWindow( SW_SHOW );
	}

	// hide the rest float bars.
	for (int i = usedFloatBarWnds_; i < MAX_FLOAT_BAR_WND; ++i)
	{		
		floatBarWnds[i].ShowWindow( SW_HIDE );
	}

	//existResource list:
	if (existingResourceListprovider_)
	{
		Utilities::moveAndStretch( this, existingResourceList_, xPos, yPos,
			GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE,
			GROUP_WND_INNER_SPACE + GROUP_WND_OUTER_SPACE );
		existingResourceList_.ShowWindow( SW_SHOW );
	}
	else
	{
		existingResourceList_.ShowWindow( SW_HIDE );
	}
}


/**
*	This MFC event handler changes the layout of the dialog's controls to best
*	fit the window shape and size.
*
*	@param nType	MFC resize type
*	@param cx		New dialog width.
*	@param cy		New dialog height.
*/
void FloraSettingPropertiesWnd::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;
	this->layOut();
}

////////////////////FloraSettingSecondaryWnd begin///////////////////
BEGIN_MESSAGE_MAP( FloraSettingSecondaryWnd, CWnd )
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CHAR()
END_MESSAGE_MAP()



FloraSettingSecondaryWnd::FloraSettingSecondaryWnd(
	PageFloraSetting* pPageWnd ):
	pPageWnd_( pPageWnd ),
	assetListWnd_( pPageWnd, this ),
	propertiesWnd_( pPageWnd, this ),
	pSection_( NULL ),
	initialized_( false ),
	hItem_( NULL )
{
	BW_GUARD;

}


/**
 *  This method returns the minimum usable width.
 */
int FloraSettingSecondaryWnd::minimumWidth()
{
	return max(assetListWnd_.minimumWidth(), propertiesWnd_.minimumWidth()) + SPLITTER_SIZER_SPACE;
}


/**
 *  This method returns the minimum usable height.
 */
int FloraSettingSecondaryWnd::minimumHeight()
{
	CRect rect;
	assetListWnd_.GetWindowRect(&rect);
	return max(rect.Height(), assetListWnd_.minimumHeight()) + propertiesWnd_.minimumHeight() + SPLITTER_SIZER_SPACE;
}


// Minimum size of a splitter pane
static const int MIN_SPLITTER_PANE_SIZE = 16;
void FloraSettingSecondaryWnd::init()
{
	BW_GUARD;

	// create new splitter
	splitterBar_.setMinRowSize( MIN_SPLITTER_PANE_SIZE );
	splitterBar_.setMinColSize( MIN_SPLITTER_PANE_SIZE );

	CRect rect;
	this->GetClientRect( &rect );
	int id2;

	if ( true )
	{
		id2 = splitterBar_.IdFromRowCol( 1, 0 );

		int idealHeight = rect.Height()/2;
		splitterBar_.SetRowInfo( 0, idealHeight, 1 );
		splitterBar_.SetRowInfo( 1, idealHeight, 1 );
	}
	else
	{
		id2 = splitterBar_.IdFromRowCol( 0, 1 );

		int idealWidth = rect.Width()/2;
		splitterBar_.SetColumnInfo( 0, idealWidth, 1 );
		splitterBar_.SetColumnInfo( 1, idealWidth, 1 );
	}

	assetListWnd_.SetDlgCtrlID( splitterBar_.IdFromRowCol( 0, 0 ) );
	assetListWnd_.SetParent( &splitterBar_ );

	propertiesWnd_.SetDlgCtrlID( id2 );
	propertiesWnd_.SetParent( &splitterBar_ );

	propertiesWnd_.ShowWindow( SW_SHOW );
	assetListWnd_.ShowWindow( SW_SHOW );
	splitterBar_.ShowWindow( SW_SHOW );	

	initialized_ = true;
	this->layOut();
}

/**
 *	This MFC event handler when creating this window
 */
int FloraSettingSecondaryWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	BW_GUARD;
	int ret = CWnd::OnCreate( lpCreateStruct );

	splitterBar_.CreateStatic( this, 2, 1, WS_CHILD );

	assetListWnd_.Create( IDD_FLORA_SETTING_ASSET_WND, &splitterBar_ );
	propertiesWnd_.Create( IDD_FLORA_SETTING_PROPERTIES_WND, &splitterBar_ );

	return ret;
}


/**
 *	This method aids in adjusting the size of the splitter bar when the dialog
 *	is resized, in order to avoid completely collapsing a splitter pane.
 */
void FloraSettingSecondaryWnd::layOut()
{
	BW_GUARD;
	if (!initialized_)
	{
		return;
	}

	Utilities::stretch(this, splitterBar_, SPLITTER_WND_BORDER_SPACE);
	splitterBar_.RecalcLayout();
	assetListWnd_.RedrawWindow();
	propertiesWnd_.RedrawWindow();
}


void FloraSettingSecondaryWnd::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;
	CWnd::DoDataExchange(pDX);
	DDX_Control(pDX, IDD_FLORA_SETTING_ASSET_WND, assetListWnd_);
	DDX_Control(pDX, IDD_FLORA_SETTING_PROPERTIES_WND, propertiesWnd_);
}

/**
 *	This MFC event handler changes the layout of the dialog's controls to best
 *	fit the window shape and size.
 *
 *	@param nType	MFC resize type
 *	@param cx		New dialog width.
 *	@param cy		New dialog height.
 */
void FloraSettingSecondaryWnd::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;
	CWnd::OnSize( nType, cx, cy );
	this->layOut();
}


/**
 *	This method is called when the tooltip for a list item is needed.
 *
 *	@param index	Index of the clicked item on the list.
 *	@param info		Return string that will contain the tooltip for the item.
 */
void FloraSettingSecondaryWnd::listItemToolTip( int index, BW::wstring& info )
{
	BW_GUARD;
	if ( index < 0 )
	{
		return;
	}

	SmartListCtrl* pCurSmartList = NULL;
	POINT pt;
	GetCursorPos( &pt );
	HWND hwnd = ::WindowFromPoint( pt );
	if ( hwnd == assetList().GetSafeHwnd() )
	{
		pCurSmartList = &assetList();
	}
	else if ( hwnd == existResourceList().GetSafeHwnd() )
	{
		pCurSmartList = &existResourceList();
	}
	if (pCurSmartList)
	{
		AssetInfo assetInfo = pCurSmartList->getAssetInfo( index );

		ListProviderSection* pSection = dynamic_cast<ListProviderSection*>( pSection_ );
		MF_ASSERT( pSection );
		if (pSection)
		{
			wchar_t wInfo[256];
			pSection->generateItemText( assetInfo, wInfo, 256, true );
			info = wInfo;
		}
	}
}


/**
*	This function is called when we are about to be active.
*	@param hItem	 the tree control item who triggered this event.
*/
void FloraSettingSecondaryWnd::onActive( HTREEITEM hItem )
{
	BW_GUARD;
	if (!initialized_)
	{
		this->init();
	}

	hItem_ = hItem;
	pSection_ = pPageWnd_->tree().getItemSection( hItem_ );

	assetListWnd_.onActive( hItem );
	propertiesWnd_.onActive( hItem );

	if (pPageWnd_->isReady())
	{
		pPageWnd_->layOut();
	}
}


/**
*	This function is called when we are about to be deactive.
*	@param hItem	 the tree control item who triggered this event.
*/
void FloraSettingSecondaryWnd::onDeactive( HTREEITEM hItem )
{
	BW_GUARD;
	hItem_ = NULL;
	pSection_ = NULL;

	assetListWnd_.onDeactive( hItem );
	propertiesWnd_.onDeactive( hItem );

}

BW_END_NAMESPACE

