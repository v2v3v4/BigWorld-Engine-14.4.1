#include "pch.hpp"
#include "worldeditor/gui/dialogs/resize_maps_dlg.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "appmgr/options.hpp"
#include "terrain/terrain_settings.hpp"

BW_BEGIN_NAMESPACE

// ResizeMaps dialog

IMPLEMENT_DYNAMIC(ResizeMapsDlg, CDialog)


ResizeMapsDlg::ResizeMapsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ResizeMapsDlg::IDD, pParent)
{
}


ResizeMapsDlg::~ResizeMapsDlg()
{
}

uint32 ResizeMapsDlg::blendsMapSize() const
{
	BW_GUARD;

	return Options::getOptionInt( "terrain2/defaults/blendMapSize",
		Terrain::TerrainSettings::defaults()->blendMapSize());
}

void ResizeMapsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RESIZEMAP_CURSIZE, curBlendMapSize_);
	DDX_Control(pDX, IDC_RESIZE_BLENDMAP_SIZE, blendMapSize_);
	DDX_Control(pDX, IDC_ICON_WARNING, iconWarning_);
	DDX_Control(pDX, IDCANCEL, buttonCancel_);
	DDX_Control(pDX, IDOK, buttonCreate_);
}


BEGIN_MESSAGE_MAP(ResizeMapsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// ResizeMaps message handlers

BOOL ResizeMapsDlg::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	INIT_AUTO_TOOLTIP();
	
	blendMapSize_.initInt( MIN_HOLE_MAP_SIZE, MAX_MAP_SIZE,
		Options::getOptionInt( "terrain2/defaults/blendMapSize", 
		Terrain::TerrainSettings::defaults()->blendMapSize() ) );
	blendMapSize_.setSilent( false );

	BW::stringstream sizeStr;
	sizeStr << WorldManager::instance().pTerrainSettings()->blendMapSize();
	curBlendMapSize_.SetWindowText( bw_utf8tow( sizeStr.str() ).c_str() );

	iconWarning_.SetIcon( LoadIcon( NULL, IDI_WARNING ) );

	UpdateData( FALSE );

	return TRUE;  // return TRUE unless you set the focus to a control
}


void ResizeMapsDlg::OnBnClickedOk()
{
	BW_GUARD;

	// store the defaults
	if (GetFocus() != &buttonCreate_)
	{
		if (!blendMapSize_.isValidValue())
		{
			blendMapSize_.setSilent( true );
			blendMapSize_.SetFocus();
			blendMapSize_.setSilent( false );
			return;
		}
	}
	Options::setOptionInt( "terrain2/defaults/blendMapSize", blendMapSize_.GetIntegerValue() );

	OnOK();
}

BW_END_NAMESPACE

