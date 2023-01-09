
#include "pch.hpp"
#include "worldeditor/gui/dialogs/expand_space_dlg.hpp"
#include "chunk/geometry_mapping.hpp"

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNAMIC( ExpandSpaceControl, controls::EmbeddableCDialog )
ExpandSpaceControl::ExpandSpaceControl(
	IValidationChangeHandler * validationChangeHandler )
	: controls::EmbeddableCDialog( ExpandSpaceControl::IDD )
	, validationChangeHandler_( validationChangeHandler )
	, maxToExpandX_( 0 )
	, maxToExpandY_( 0 )
{
}


void ExpandSpaceControl::DoDataExchange( CDataExchange * pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_EDIT_WEST, editWest_);
	DDX_Control(pDX, IDC_EDIT_EAST, editEast_);
	DDX_Control(pDX, IDC_EDIT_NORTH, editNorth_);
	DDX_Control(pDX, IDC_EDIT_SOUTH, editSouth_);
	DDX_Control(pDX, IDC_INCLUDE_TERRAIN, includeTerrain_);
}


BOOL ExpandSpaceControl::OnInitDialog()
{
	controls::EmbeddableCDialog::OnInitDialog();
	this->init();

	return TRUE;
}

void ExpandSpaceControl::init()
{
	GeometryMapping *srcMapping = WorldManager::instance().geometryMapping();
	maxToExpandX_ = MAX_CHUNK_SIZE - ( srcMapping->maxGridX() - srcMapping->minGridX() + 1);
	maxToExpandY_ = MAX_CHUNK_SIZE - ( srcMapping->maxGridY() - srcMapping->minGridY() + 1);

	includeTerrain_.SetCheck( BST_CHECKED );
	if (maxToExpandX_ <= 0 || maxToExpandY_ <= 0)
	{
		editWest_.EnableWindow( FALSE );
		editEast_.EnableWindow( FALSE );
		editNorth_.EnableWindow( FALSE );
		editSouth_.EnableWindow( FALSE );
		return;
	}

	editWest_.initInt( 0, maxToExpandX_, 0 );
	editEast_.initInt( 0, maxToExpandX_, 0 );
	editNorth_.initInt( 0, maxToExpandY_, 0 );
	editSouth_.initInt( 0, maxToExpandY_, 0 );

}

BEGIN_MESSAGE_MAP( ExpandSpaceControl, controls::EmbeddableCDialog )
	ON_EN_CHANGE(IDC_EDIT_EAST, &ExpandSpaceControl::OnEnChangeEdit)
	ON_EN_CHANGE(IDC_EDIT_WEST, &ExpandSpaceControl::OnEnChangeEdit)
	ON_EN_CHANGE(IDC_EDIT_NORTH, &ExpandSpaceControl::OnEnChangeEdit)
	ON_EN_CHANGE(IDC_EDIT_SOUTH, &ExpandSpaceControl::OnEnChangeEdit)
	ON_BN_CLICKED(IDC_INCLUDE_TERRAIN, &ExpandSpaceControl::OnBnClickedCheckEmptyChunk)
END_MESSAGE_MAP()


const ExpandSpaceControl::ExpandInfo& ExpandSpaceControl::expandInfo()
{
	return expandInfo_;
}


void ExpandSpaceControl::OnBnClickedExpandSpaceBtnOk()
{
	int westCnt = editWest_.GetIntegerValue();
	int eastCnt = editEast_.GetIntegerValue();
	int northCnt = editNorth_.GetIntegerValue();
	int southCnt = editSouth_.GetIntegerValue();

	if ( eastCnt + westCnt > maxToExpandX_ ||
		northCnt + southCnt > maxToExpandY_ )
	{
		//todo message box
		MessageBox( Localise(L"WORLDEDITOR/GUI/EXPAND_SPACE_DLG/EXCEEDED_MAX_CHUNK_SIZE"), NULL, MB_ICONERROR );
		return;
	}

	expandInfo_.west_ = westCnt;
	expandInfo_.east_ = eastCnt;
	expandInfo_.north_ = northCnt;
	expandInfo_.south_ = southCnt;
}


void ExpandSpaceControl::OnEnChangeEdit()
{
	int westCnt = editWest_.GetIntegerValue();
	int eastCnt = editEast_.GetIntegerValue();
	int northCnt = editNorth_.GetIntegerValue();
	int southCnt = editSouth_.GetIntegerValue();

	if( validationChangeHandler_ )
	{
		bool validates = !(
			westCnt <= 0 &&
			eastCnt <= 0 &&
			northCnt <= 0 &&
			southCnt <= 0 );
		validationChangeHandler_->validationChange( validates );
	}
}



void ExpandSpaceControl::OnBnClickedCheckEmptyChunk()
{
	expandInfo_.includeTerrainInNewChunks_ = ( includeTerrain_.GetCheck() == BST_CHECKED );
}


IMPLEMENT_DYNAMIC( ExpandSpaceDlg, CDialog )

BEGIN_MESSAGE_MAP( ExpandSpaceDlg, CDialog )
	ON_BN_CLICKED(IDC_EXPAND_SPACE_BTN_OK, &ExpandSpaceDlg::OnBnClickedExpandSpaceBtnOk)
	ON_BN_CLICKED(IDC_EXPAND_SPACE_BTN_CANCEL, &ExpandSpaceDlg::OnBnClickedExpandSpaceBtnCancel)
END_MESSAGE_MAP()

ExpandSpaceDlg::ExpandSpaceDlg( CWnd * pParent )
	: CDialog( ExpandSpaceDlg::IDD, pParent )
	, expandSpaceControl_( this )
{
}


BOOL ExpandSpaceDlg::OnInitDialog()
{
	UpdateData( FALSE );
	expandSpaceControl_.SubclassDlgItem( IDC_EXPAND_SPACE_CONTROL, this );
	return TRUE;
}


void ExpandSpaceDlg::DoDataExchange( CDataExchange * pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_EXPAND_SPACE_BTN_OK, btnExpand_);
}


void ExpandSpaceDlg::OnBnClickedExpandSpaceBtnOk()
{
	BW_GUARD;
	expandSpaceControl_.OnBnClickedExpandSpaceBtnOk();
	CDialog::OnOK();
}


void ExpandSpaceDlg::OnBnClickedExpandSpaceBtnCancel()
{
	CDialog::OnCancel();
}


const ExpandSpaceControl::ExpandInfo & ExpandSpaceDlg::expandInfo()
{
	return expandSpaceControl_.expandInfo();
}


/*virtual */void ExpandSpaceDlg::validationChange( bool validationResult )
{
	if ( validationResult )
	{
		btnExpand_.EnableWindow( TRUE );
	}
	else
	{
		btnExpand_.EnableWindow( FALSE );
	}
}

BW_END_NAMESPACE

