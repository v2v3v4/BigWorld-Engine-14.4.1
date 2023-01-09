// take_chunk_photo_dlg.cpp : implementation file
//

#include "pch.hpp"
#include "worldeditor/gui/dialogs/take_chunk_photo_dlg.hpp"

#define MIN_PIXELS_PER_METRE	0.1f
#define MAX_PIXELS_PER_METRE	10.0f
#define PIXELS_PER_METRE_SCROLL_STEP				0.1f
#define MAP_SIZE_COUNT			20
#define MIN_SINGLEMAP_SIZE		256
#define MAX_SINGLEMAP_SIZE		8192
#define MAX_TILE_SIZE			2048

BW_BEGIN_NAMESPACE

// take_chunk_photo_dlg dialog

IMPLEMENT_DYNAMIC(TakePhotoDlg, CDialog)

TakePhotoDlg::TakePhotoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(TakePhotoDlg::IDD, pParent),
		pixelsPerMetre_(MIN_PIXELS_PER_METRE),
		minPixelsperMetre_(MIN_PIXELS_PER_METRE),
		maxPixelsperMetre_(MAX_PIXELS_PER_METRE),
		createSingleMap_(false),
		createLevelMap_(true),
		blockSize_(0),
		gridCountX_(0),
		gridCountY_(0),
		mapMinWidth_(0),
		mapMaxWidth_(0),
		spaceName_("")
{

}

TakePhotoDlg::~TakePhotoDlg()
{
}

void TakePhotoDlg::setSpaceInfo( const BW::string& spaceName, float minPixelsperMetre, float maxPixelsperMetre, int blockSize, int gridCountX, int gridCountY )
{
	BW_GUARD;
	if ( minPixelsperMetre >= maxPixelsperMetre ||
		minPixelsperMetre <= 0 ||
		maxPixelsperMetre <= 0 )
	{
		minPixelsperMetre_ = MIN_PIXELS_PER_METRE;
		maxPixelsperMetre_ = MAX_PIXELS_PER_METRE;
	}
	else
	{
		minPixelsperMetre_ = minPixelsperMetre;
		maxPixelsperMetre_ = maxPixelsperMetre;
	}

	if ( maxPixelsperMetre_ * blockSize > MAX_TILE_SIZE )
	{
		maxPixelsperMetre_ = MAX_TILE_SIZE * 1.0f / blockSize;
	}
	
	blockSize_ = blockSize;
	gridCountX_ = gridCountX;
	gridCountY_ = gridCountY;
	spaceName_ = spaceName;
}

void TakePhotoDlg::calcMapSizeRange()
{
	int minPos = 0;
	int maxPos = 0;

	int minSize = MIN_SINGLEMAP_SIZE;
	if ( gridCountY_ < gridCountX_ )
	{
		//If the chunks count is less than minSize, make the minSize bigger,
		//otherwise there would be less than 1 pixel per chunk
		if ( minSize < gridCountX_ * 5 )
		{
			minSize = gridCountX_ * 5;
		}
		mapMinWidth_ = (float) (minSize * gridCountY_ / gridCountX_);
		mapMaxWidth_ = (float) (MAX_SINGLEMAP_SIZE * gridCountY_ / gridCountX_);
	}
	else
	{
		if ( minSize < gridCountY_ * 5 )
		{
			minSize = gridCountY_ * 5;
		}
		mapMinWidth_ = (float) minSize;
		mapMaxWidth_ = (float) MAX_SINGLEMAP_SIZE;
	}

	editorMapWidth_.SetMinimum( (double)mapMinWidth_ );
	editorMapWidth_.SetMaximum( (double)mapMaxWidth_ );
	editorMapWidth_.SetValue( mapMinWidth_ );
	editorMapHeight_.SetValue( mapMinWidth_ * gridCountX_ / gridCountY_ );
}

void TakePhotoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PIXELS_PER_METRE, editorPixelsperMetre_);
	DDX_Control(pDX, IDC_SLIDER_PIXELS_PER_METRE, sliderPixelsperMetre_);
	DDX_Control(pDX, IDC_EDIT_MAP_WIDTH, editorMapWidth_);
	DDX_Control(pDX, IDC_EDIT_MAP_HEIGHT, editorMapHeight_);
	DDX_Control(pDX, IDC_RADIO_CREATE_LEVEL_MAP, radioCreateLevelMap_);
	DDX_Control(pDX, IDC_RADIO_CREATE_SINGLE_MAP, radioCreateSingleMap_);
	DDX_Control(pDX, IDC_TAKE_PHOTO_DESC, lableDesc);
}


BEGIN_MESSAGE_MAP(TakePhotoDlg, CDialog)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_RADIO_CREATE_LEVEL_MAP, &TakePhotoDlg::OnBnClickedRadioCreateLevelMap)
	ON_BN_CLICKED(IDC_RADIO_CREATE_SINGLE_MAP, &TakePhotoDlg::OnBnClickedRadioCreateSingleMap)
	ON_EN_CHANGE(IDC_EDIT_MAP_WIDTH, &TakePhotoDlg::OnEnChangeEditMapWidth)
END_MESSAGE_MAP()


BOOL TakePhotoDlg::OnInitDialog()
{
	BW_GUARD;
	CDialog::OnInitDialog();
	int minScrollRange = 0;
	int maxScrollRange = (int)(maxPixelsperMetre_  / PIXELS_PER_METRE_SCROLL_STEP);
	sliderPixelsperMetre_.SetRange( minScrollRange, maxScrollRange );
	editorPixelsperMetre_.SetNumDecimals( 1 );
	editorPixelsperMetre_.SetValue( (float)pixelsPerMetre_ );
	editorMapWidth_.SetNumDecimals( 0 );
	editorMapHeight_.SetNumDecimals( 0 );
	radioCreateLevelMap_.SetCheck( TRUE );
	radioCreateSingleMap_.SetCheck( FALSE );
	this->OnPhotoOption();
	this->calcMapSizeRange();

	CString desc;
	BW::wstring temp(spaceName_.length(),L' ');
	std::copy(spaceName_.begin(), spaceName_.end(), temp.begin());

	lableDesc.GetWindowText( desc );
	desc.Replace( L"SPACENAME", temp.c_str() );
	lableDesc.SetWindowText( desc );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void TakePhotoDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_PIXELS_PER_METRE)
	{
		int pos = sliderPixelsperMetre_.GetPos();
		pixelsPerMetre_ = minPixelsperMetre_ + (float) pos * PIXELS_PER_METRE_SCROLL_STEP;
		if ( pixelsPerMetre_ > maxPixelsperMetre_)
		{
			pixelsPerMetre_ = maxPixelsperMetre_;
		}
		editorPixelsperMetre_.SetValue( pixelsPerMetre_ );
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

TakePhotoDlg::PhotoInfo TakePhotoDlg::getPhotoInfo()
{
	BW_GUARD;

	PhotoInfo photoInfo;
	
	if ( createSingleMap_ )
	{
		photoInfo.mode = SINGLE_MAP;
		photoInfo.spaceMapWidth = (int) editorMapWidth_.GetValue();
		photoInfo.spaceMapHeight = (int) editorMapHeight_.GetValue();
		photoInfo.pixelsPerMetre = photoInfo.spaceMapWidth * 1.0f/(blockSize_ * gridCountY_);
	}
	else if( createLevelMap_ )
	{
		photoInfo.mode = LEVEL_MAP;
		photoInfo.pixelsPerMetre = pixelsPerMetre_;
	}
	return photoInfo;
}

void TakePhotoDlg::OnPhotoOption()
{
	BW_GUARD;

	int createLevelMap = radioCreateLevelMap_.GetCheck();
	int createSingleMap = radioCreateSingleMap_.GetCheck();
	
	editorMapWidth_.EnableWindow( createSingleMap?TRUE:FALSE );
	editorMapHeight_.EnableWindow( createSingleMap?TRUE:FALSE );
	createLevelMap_ = createLevelMap?true:false;

	editorPixelsperMetre_.EnableWindow( createLevelMap?TRUE:FALSE );
	sliderPixelsperMetre_.EnableWindow( createLevelMap?TRUE:FALSE );
	createSingleMap_ = createSingleMap?true:false;
	
}

void TakePhotoDlg::OnBnClickedRadioCreateLevelMap()
{
	BW_GUARD;
	this->OnPhotoOption();
}


void TakePhotoDlg::OnBnClickedRadioCreateSingleMap()
{
	BW_GUARD;
	this->OnPhotoOption();
}

void TakePhotoDlg::OnEnChangeEditMapWidth()
{
	//calculate height
	float width = editorMapWidth_.GetValue();

	if ( width > mapMaxWidth_ )
	{
		width = mapMaxWidth_;
		editorMapWidth_.SetValue( width );
	}

	float height = width * gridCountX_ / gridCountY_;
	editorMapHeight_.SetValue( height );
}
BW_END_NAMESPACE

