
#include "pch.hpp"
#include "post_proc_caption_bar.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// Constants used to reposition the buttons on resize.
	const int BTN_SPACING = 3;
	const int BTN_RIGHT_MARGIN = 16;
} // anonymous namespace


IMPLEMENT_DYNAMIC( PostProcCaptionBar, CDialog )


// XML options.xml tag names for the state of the toggle buttons.
/*static*/ const char * PostProcCaptionBar::OPTION_PREVIEW3D = "post_processing/preview3d";
/*static*/ const char * PostProcCaptionBar::OPTION_PROFILE = "post_processing/profile";


/**
 *	Constructor.
 *
 *	@param pParent	Parent for the caption bar.
 */
PostProcCaptionBar::PostProcCaptionBar( CWnd * pParent /*=NULL*/ ) :
	CDialog( PostProcCaptionBar::IDD, pParent ),
	handler_( NULL ),
	previewImg_( NULL ),
	previewOnImg_( NULL ),
	profileImg_( NULL ),
	profileOnImg_( NULL ),
	layoutImg_( NULL ),
	deleteImg_( NULL ),
	inited_( false )
{
}


/**
 *	Destructor.
 */
PostProcCaptionBar::~PostProcCaptionBar()
{
	BW_GUARD;

	::DestroyIcon( zoomInImg_ );
	zoomOutImg_ = NULL;

	::DestroyIcon( zoomOutImg_ );
	zoomOutImg_ = NULL;

	::DestroyIcon( noZoomImg_ );
	noZoomImg_ = NULL;

	::DestroyIcon( previewImg_ );
	previewImg_ = NULL;

	::DestroyIcon( previewOnImg_ );
	previewOnImg_ = NULL;

	::DestroyIcon( profileImg_ );
	profileImg_ = NULL;

	::DestroyIcon( profileOnImg_ );
	profileOnImg_ = NULL;

	::DestroyIcon( layoutImg_ );
	layoutImg_ = NULL;

	::DestroyIcon( deleteImg_ );
	deleteImg_ = NULL;
}


/**
 *	This method sets the caption bar text.
 *
 *	@param text	New caption bar text.
 */
void PostProcCaptionBar::captionText( const BW::wstring & text )
{
	BW_GUARD;

	graphCaption_.SetWindowText( text.c_str() );
}


/**
 *	This method activates or deactivates the preview state and button.
 *
 *	@param activate	True to activate, false to deactivate.
 */
void PostProcCaptionBar::setPreview3D( bool activate )
{
	BW_GUARD;

	if (activate != (preview3D_.GetCheck() == BST_CHECKED))
	{
		Options::setOptionInt( PostProcCaptionBar::OPTION_PREVIEW3D, activate ? 1 : 0 );
		preview3D_.SetCheck( activate ? BST_CHECKED : BST_UNCHECKED );
	}
}


/**
 *	This method activates or deactivates the profiling state and button.
 *
 *	@param activate	True to activate, false to deactivate.
 */
void PostProcCaptionBar::setProfile( bool activate )
{
	BW_GUARD;

	if (activate != (profile_.GetCheck() == BST_CHECKED))
	{
		Options::setOptionInt( PostProcCaptionBar::OPTION_PROFILE, activate ? 1 : 0 );
		profile_.SetCheck( activate ? BST_CHECKED : BST_UNCHECKED );
	}
}


/**
 *	This method is called when this window is being initialised, and it in turn
 *	initialises all the caption bar's buttons, tooltips, etc.
 *
 *	@return	TRUE to signal success.
 */
BOOL PostProcCaptionBar::OnInitDialog()
{
	BW_GUARD;

	BOOL ok = CDialog::OnInitDialog();

	if (ok == FALSE)
	{
		return ok;
	}

	INIT_AUTO_TOOLTIP();

	zoomInImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_ZOOM_IN ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
	zoomIn_.SetIcon( zoomInImg_ );

	zoomOutImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_ZOOM_OUT ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
	zoomOut_.SetIcon( zoomOutImg_ );

	noZoomImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_NOZOOM ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
	noZoom_.SetIcon( noZoomImg_ );

	previewImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_3DPREVIEW_SHOW ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );

	previewOnImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_3DPREVIEW_CHECKED ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );

	preview3D_.SetIcon( previewImg_ );
	preview3D_.SetCheckedIcon( previewOnImg_ );

	preview3D_.SetCheck( BST_UNCHECKED );
	Options::setOptionInt( OPTION_PREVIEW3D, 0 );

	profileImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_PROFILE_SHOW ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );

	profileOnImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_PROFILE_CHECKED ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );

	profile_.SetIcon( profileImg_ );
	profile_.SetCheckedIcon( profileOnImg_ );

	profile_.SetCheck( BST_UNCHECKED );
	Options::setOptionInt( OPTION_PROFILE, 0 );

	layoutImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_LAYOUT ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );

	layout_.SetIcon( layoutImg_ );

	deleteImg_ = (HICON)LoadImage( AfxGetInstanceHandle(),
			MAKEINTRESOURCE( IDI_PP_DELETE_ALL ),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );

	deleteAll_.SetIcon( deleteImg_ );

	inited_ = true;

	return TRUE;
}


/**
 *	This MFC method syncs our Windows dialog controls with our member
 *	variables.
 *
 *	@param pDC	MFC data exchange struct.
 */
void PostProcCaptionBar::DoDataExchange( CDataExchange * pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_PP_GRAPH_INFO, graphCaption_);
	DDX_Control(pDX, IDC_PP_ZOOM_IN, zoomIn_);
	DDX_Control(pDX, IDC_PP_ZOOM_OUT, zoomOut_);
	DDX_Control(pDX, IDC_PP_NOZOOM, noZoom_);
	DDX_Control(pDX, IDC_PP_3D_PREVIEW, preview3D_);
	DDX_Control(pDX, IDC_PP_PROFILE, profile_);
	DDX_Control(pDX, IDC_PP_LAYOUT, layout_);
	DDX_Control(pDX, IDC_PP_DELETE_ALL, deleteAll_);
}


// MFC message map.
BEGIN_MESSAGE_MAP( PostProcCaptionBar, CDialog )
	ON_WM_SIZE()
	ON_BN_CLICKED( IDC_PP_ZOOM_IN, &PostProcCaptionBar::OnZoomIn )
	ON_BN_CLICKED( IDC_PP_ZOOM_OUT, &PostProcCaptionBar::OnZoomOut )
	ON_BN_CLICKED( IDC_PP_NOZOOM, &PostProcCaptionBar::OnNoZoom )
	ON_BN_CLICKED( IDC_PP_3D_PREVIEW, &PostProcCaptionBar::On3dPreview )
	ON_BN_CLICKED( IDC_PP_PROFILE, &PostProcCaptionBar::OnProfile )
	ON_BN_CLICKED( IDC_PP_LAYOUT, &PostProcCaptionBar::OnLayout )
	ON_BN_CLICKED( IDC_PP_DELETE_ALL, &PostProcCaptionBar::OnDeleteAll )
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden reposition the buttons when this window gets
 *	resized.
 *
 *	@param nType	MFC resize type.
 *	@param cx	New width.
 *	@param cy	New height.
 */
void PostProcCaptionBar::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	if (!inited_)
		return;

	CDialog::OnSize( nType, cx, cy );

	int buttonXPos = cx - BTN_RIGHT_MARGIN;

	CRect deleteRect;
	deleteAll_.GetWindowRect( deleteRect );
	buttonXPos -= deleteRect.Width();

	deleteAll_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect layoutRect;
	layout_.GetWindowRect( layoutRect );
	buttonXPos -= layoutRect.Width() + BTN_SPACING;

	layout_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect profileRect;
	profile_.GetWindowRect( profileRect );
	buttonXPos -= profileRect.Width() + BTN_SPACING;

	profile_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect previewRect;
	preview3D_.GetWindowRect( previewRect );
	buttonXPos -= previewRect.Width() + BTN_SPACING;

	preview3D_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect noZoomRect;
	noZoom_.GetWindowRect( noZoomRect );
	buttonXPos -= noZoomRect.Width() + BTN_SPACING;

	noZoom_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect zoomOutRect;
	zoomOut_.GetWindowRect( zoomOutRect );
	buttonXPos -= zoomOutRect.Width() + BTN_SPACING;

	zoomOut_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect zoomInRect;
	zoomIn_.GetWindowRect( zoomInRect );
	buttonXPos -= zoomInRect.Width() + BTN_SPACING;

	zoomIn_.SetWindowPos( NULL, buttonXPos, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect captionRect;
	graphCaption_.GetWindowRect( captionRect );

	buttonXPos -= BTN_SPACING * 2;

	graphCaption_.SetWindowPos( NULL, 0, 0, buttonXPos, captionRect.Height(), SWP_NOZORDER | SWP_NOMOVE );

	graphCaption_.RedrawWindow();
	zoomIn_.RedrawWindow();
	zoomOut_.RedrawWindow();
	noZoom_.RedrawWindow();
	preview3D_.RedrawWindow();
	profile_.RedrawWindow();
	layout_.RedrawWindow();
	deleteAll_.RedrawWindow();

	RedrawWindow();
}


/**
 *	This MFC method is called when the zoom in button is pressed.
 */
void PostProcCaptionBar::OnZoomIn()
{
	BW_GUARD;

	if (handler_)
	{
		handler_->SendMessage( WM_PP_ZOOM_IN );
	}
}


/**
 *	This MFC method is called when the zoom out button is pressed.
 */
void PostProcCaptionBar::OnZoomOut()
{
	BW_GUARD;

	if (handler_)
	{
		handler_->SendMessage( WM_PP_ZOOM_OUT );
	}
}


/**
 *	This MFC method is called when the zoom reset button is pressed.
 */
void PostProcCaptionBar::OnNoZoom()
{
	BW_GUARD;

	if (handler_)
	{
		handler_->SendMessage( WM_PP_NO_ZOOM );
	}
}


/**
 *	This MFC method is called when the preview button is pressed.
 */
void PostProcCaptionBar::On3dPreview()
{
	BW_GUARD;

	int newState = (preview3D_.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
	preview3D_.SetCheck( newState );

	int newVal = (newState == BST_CHECKED ? 1 : 0);

	Options::setOptionInt( OPTION_PREVIEW3D, newVal );

	if (handler_)
	{
		handler_->SendMessage( WM_PP_3D_PREVIEW, newVal );
	}
}


/**
 *	This MFC method is called when the profile button is pressed.
 */
void PostProcCaptionBar::OnProfile()
{
	BW_GUARD;

	int newState = (profile_.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
	profile_.SetCheck( newState );

	int newVal = (newState == BST_CHECKED ? 1 : 0);

	Options::setOptionInt( OPTION_PROFILE, newVal );

	if (handler_)
	{
		handler_->SendMessage( WM_PP_PROFILE, newVal );
	}
}


/**
 *	This MFC method is called when the layout button is pressed.
 */
void PostProcCaptionBar::OnLayout()
{
	BW_GUARD;

	if (handler_)
	{
		handler_->SendMessage( WM_PP_LAYOUT );
	}
}


/**
 *	This MFC method is called when the delete all button is pressed.
 */
void PostProcCaptionBar::OnDeleteAll()
{
	BW_GUARD;

	if (handler_)
	{
		handler_->SendMessage( WM_PP_DELETE_ALL );
	}
}
BW_END_NAMESPACE

