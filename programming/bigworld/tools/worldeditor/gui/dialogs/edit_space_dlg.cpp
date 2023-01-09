#include "pch.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/import/space_settings.hpp"
#include "worldeditor/terrain/space_recreator.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "appmgr/options.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "chunk/chunk_format.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "controls/dir_dialog.hpp"
#include "controls/utils.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"
#include "asset_pipeline/asset_lock.hpp"
#include "moo/effect_visual_context.hpp"
#include "cstdmf/debug.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "edit_space_dlg.hpp"
#include "cstdmf/bw_set.hpp"
#include <afxpriv.h>

BW_BEGIN_NAMESPACE

namespace
{
	/**
	/* RAII class to ensure handle en/disabling of UI 
	 * when doing background tasks
	 */
	class SpacePropertyChangeOperation
	{
	public:
		SpacePropertyChangeOperation(
			CButton & buttonCreate,
			CButton & buttonCancel,
			bool & isBusy )
			: buttonApply_( buttonCreate )
			, buttonCancel_( buttonCancel )
			, isBusy_( isBusy )
		{
			buttonApply_.EnableWindow( FALSE );
			buttonCancel_.EnableWindow( FALSE );
			isBusy = true;
		}

		~SpacePropertyChangeOperation()
		{
			buttonApply_.EnableWindow( TRUE );
			buttonCancel_.EnableWindow( TRUE );
			isBusy_ = false;
		}

	private:
		CButton & buttonCancel_;
		CButton & buttonApply_;
		bool & isBusy_;
	};
}

// EditSpace dialog

IMPLEMENT_DYNAMIC(EditSpaceDlg, CDialog)
EditSpaceDlg::EditSpaceDlg(
	SpaceSettings & spaceSettings, CWnd* pParent /*=NULL*/)
	: CDialog(EditSpaceDlg::IDD, pParent)
	, spaceSettings_( &spaceSettings )
	, busy_( false )
	, filterChanged_( 0 )
	, terrainTextureLod_( filterChanged_, this )
	, expandSpaceDlg_( this )
	, tickCount_( GetTickCount() )
	, expandSpaceValid_( false )
	, modifySizeValid_( false )
	, modifyTextureLodValid_( false )
	, initialized_( false )
{
}

EditSpaceDlg::~EditSpaceDlg()
{
	terrainTextureLod_.DestroyWindow();
	expandSpaceDlg_.DestroyWindow();
}


void EditSpaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHUNKSIZE, chunkSize_);
	DDX_Control(pDX, IDC_HEIGHTMAP_EDITOR_SIZE, heightMapEditorSize_);
	DDX_Control(pDX, IDC_HEIGHTMAP_SIZE, heightMapSize_);
	DDX_Control(pDX, IDC_NORMALMAP_SIZE, normalMapSize_);
	DDX_Control(pDX, IDC_HOLEMAP_SIZE, holeMapSize_);
	DDX_Control(pDX, IDC_SHADOWMAP_SIZE, shadowMapSize_);
	DDX_Control(pDX, IDC_BLENDMAP_SIZE, blendMapSize_);
	DDX_Control(pDX, IDCANCEL, buttonCancel_);
	DDX_Control(pDX, IDOK, buttonApply_);
	DDX_Control(pDX, IDC_HEIGHTMAP_RES, heightMapRes_);
	DDX_Control(pDX, IDC_HEIGHTMAP_EDITOR_RES, heightMapEditorRes_);
	DDX_Control(pDX, IDC_NORMALMAP_RES, normalMapRes_);
	DDX_Control(pDX, IDC_HOLEMAP_RES, holeMapRes_);
	DDX_Control(pDX, IDC_BLENDMAP_RES, blendMapRes_);
	DDX_Control(pDX, IDC_SHADOWMAP_RES, shadowMapRes_);
}


BEGIN_MESSAGE_MAP(EditSpaceDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_EN_CHANGE(IDC_CHUNKSIZE, OnEnChangeChunkSize)
	ON_CBN_SELCHANGE(IDC_HEIGHTMAP_EDITOR_SIZE, OnCbnSelChangeEditorHeightmapSize)
	ON_CBN_SELCHANGE(IDC_HEIGHTMAP_SIZE, OnCbnSelChangeHeightmapSize)
	ON_CBN_SELCHANGE(IDC_NORMALMAP_SIZE, OnCbnSelChangeNormalmapSize)
	ON_CBN_SELCHANGE(IDC_HOLEMAP_SIZE, OnCbnSelChangeHolemapSize)
	ON_CBN_SELCHANGE(IDC_SHADOWMAP_SIZE, OnCbnSelChangeShadowmapSize)
	ON_CBN_SELCHANGE(IDC_BLENDMAP_SIZE, OnCbnSelChangeBlendmapSize)
	ON_MESSAGE_VOID( WM_KICKIDLE, OnKickIdle )
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


void EditSpaceDlg::initIntEdit(
	controls::EditNumeric& edit, int minVal, int maxVal, int val )
{
	BW_GUARD;

	edit.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	edit.SetAllowNegative( minVal < 0 );
	edit.SetMinimum( float( minVal ) );
	edit.SetMaximum( float( maxVal ) );
	edit.SetIntegerValue( val );
}


// EditSpaceDlg message handlers

BOOL EditSpaceDlg::OnInitDialog()
{
	BW_GUARD;

	if (!CDialog::OnInitDialog())
	{
		return FALSE;
	}
	

	INIT_AUTO_TOOLTIP();

	int desiredWidth = 10;
	int desiredHeight = 10;
	float gridSize = DEFAULT_GRID_RESOLUTION;

	Terrain::TerrainSettingsPtr terrainSettings =
		spaceSettings_->terrainSettings();

	gridSize = spaceSettings_->gridSize();
	spaceName_ = WorldManager::instance().getCurrentSpace();
	desiredWidth = spaceSettings_->maxX() - spaceSettings_->minX();
	desiredHeight = spaceSettings_->maxY() - spaceSettings_->minY();

	initIntEdit( chunkSize_, 10, (int)MAX_SPACE_WIDTH, (int)gridSize );

	wchar_t buffer[64];
	int size = MIN_MAP_SIZE;

#define ADD_CURRENT_VALUE( VarName ) \
	VarName##_.AddString( buffer );\
	\
	if( size < ( int ) terrainSettings->VarName() &&\
		( int ) terrainSettings->VarName() < nextSize )\
	{\
		bw_snwprintf(buffer, ARRAY_SIZE(buffer), L"%d",\
			terrainSettings->VarName() );\
		VarName##_.AddString( buffer );\
	}\

	while ( size <= MAX_MAP_SIZE )
	{
		bw_snwprintf(buffer, ARRAY_SIZE(buffer), L"%d", size );

		int nextSize = size * 2;

		ADD_CURRENT_VALUE( heightMapEditorSize );
		ADD_CURRENT_VALUE( heightMapSize );
		ADD_CURRENT_VALUE( normalMapSize );
		ADD_CURRENT_VALUE( holeMapSize );
		ADD_CURRENT_VALUE( shadowMapSize );
		ADD_CURRENT_VALUE( blendMapSize );

		size = nextSize;
	}

#undef ADD_CURRENT_VALUE

	_itow( Options::getOptionInt( "terrain2/defaults/heightMapSize",
		terrainSettings->heightMapSize() ), buffer, 10 );

#define SELECT_CURRENT_VALUE( VarName, OptionName ) \
		bw_snwprintf( buffer, ARRAY_SIZE( buffer ), L"%d",\
			terrainSettings->VarName##() );\
	VarName##_.SelectString( -1, buffer );

	if (terrainSettings->heightMapEditorSize() < terrainSettings->heightMapSize())
	{
		bw_snwprintf( buffer, ARRAY_SIZE( buffer ), L"%d",
			terrainSettings->heightMapSize() );
		heightMapEditorSize_.SelectString( -1, buffer );
	}
	else
	{
		SELECT_CURRENT_VALUE( heightMapEditorSize, "terrain2/defaults/heightMapEditorSize" );
	}

	SELECT_CURRENT_VALUE( heightMapSize, "terrain2/defaults/heightMapSize" );
	SELECT_CURRENT_VALUE( normalMapSize, "terrain2/defaults/normalMapSize" );
	SELECT_CURRENT_VALUE( holeMapSize, "terrain2/defaults/holeMapSize" );
	SELECT_CURRENT_VALUE( shadowMapSize, "terrain2/defaults/shadowMapSize" );
	SELECT_CURRENT_VALUE( blendMapSize, "terrain2/defaults/blendMapSize" );

#undef SELECT_CURRENT_VALUE

	spacePath_ =
		BWResource::resolveFilename( BWResource::getFilePath( spaceName_ ) );
	space_ =
		BWResource::dissolveFilename( BWResource::getFilename( spaceName_ ) );

	filterChanged_++;
	terrainTextureLod_.SubclassDlgItem( IDC_EDIT_SPACE_TERRAIN_LOD, this );
	terrainTextureLod_.reinitialise();

	expandSpaceDlg_.SubclassDlgItem( IDC_EDIT_SPACE_EXPAND, this );

	UpdateData( FALSE );
	filterChanged_--;

	chunkSize_.setSilent( false );

	initialized_ = true;

	// Refresh all static data that uses ChunkSize
	OnEnChangeChunkSize();

	return TRUE;  // return TRUE unless you set the focus to a control
}


void EditSpaceDlg::OnBnClickedOk()
{
	BW_GUARD;

	CWaitCursor wait;

	BW::wstring spaceName = bw_utf8tow( spacePath_ + space_ );

	float chunkSize;
	uint32 heightMapEditorSize;
	uint32 heightMapSize, normalMapSize, holeMapSize;
	uint32 shadowMapSize, blendMapSize;

	getSizeValues(
		chunkSize,
		heightMapEditorSize,
		heightMapSize, normalMapSize,
		shadowMapSize, holeMapSize,
		blendMapSize );
	
	SpacePropertyChangeOperation spacePropertyChangeOperation(
		buttonApply_, buttonCancel_, busy_ );

	return editSpace(
		spaceName,
		chunkSize,
		heightMapEditorSize,
		heightMapSize, normalMapSize,
		shadowMapSize, holeMapSize,
		blendMapSize );
}


void EditSpaceDlg::validateSizeValues()
{
	BW_GUARD;

	if( initialized_ == false )
	{
		return;
	}

	float chunkSize;
	uint32 heightMapEditorSize;
	uint32 heightMapSize, normalMapSize, holeMapSize;
	uint32 shadowMapSize, blendMapSize;

	getSizeValues(
		chunkSize,
		heightMapEditorSize,
		heightMapSize, normalMapSize,
		shadowMapSize, holeMapSize,
		blendMapSize );

	Terrain::TerrainSettingsPtr terrainSettings
		= spaceSettings_->terrainSettings();

	modifySizeValid_ =
		terrainSettings->heightMapEditorSize() != heightMapEditorSize ||
		terrainSettings->heightMapSize() != heightMapSize ||
		terrainSettings->normalMapSize() != normalMapSize ||
		terrainSettings->shadowMapSize() != shadowMapSize ||
		terrainSettings->holeMapSize() != holeMapSize ||
		terrainSettings->blendMapSize() != blendMapSize ||
		spaceSettings_->gridSize() != chunkSize;

	updateValidation();
}


void EditSpaceDlg::getSizeValues(
	float & chunkSize,
	uint32 & heightMapEditorSize,
	uint32 & heightMapSize, uint32 & normalMapSize,
	uint32 & shadowMapSize, uint32 & holeMapSize,
	uint32 & blendMapSize )
{
	BW_GUARD;

	CString chunkSizeString;
	chunkSize_.GetWindowText( chunkSizeString );
	chunkSize = (float)_wtoi( chunkSizeString.GetString() );

	CString buffer;
	heightMapEditorSize_.GetLBText( heightMapEditorSize_.GetCurSel(), buffer );
	heightMapEditorSize	= _wtoi( buffer.GetString() );
	heightMapSize_.GetLBText( heightMapSize_.GetCurSel(), buffer );
	heightMapSize	= _wtoi( buffer.GetString() );
	normalMapSize_.GetLBText( normalMapSize_.GetCurSel(), buffer );
	normalMapSize = _wtoi( buffer.GetString() );
	holeMapSize_.GetLBText( holeMapSize_.GetCurSel(), buffer );
	holeMapSize	= _wtoi( buffer.GetString() );
	shadowMapSize_.GetLBText( shadowMapSize_.GetCurSel(), buffer );
	shadowMapSize	= _wtoi( buffer.GetString() );
	blendMapSize_.GetLBText( blendMapSize_.GetCurSel(), buffer );
	blendMapSize = _wtoi( buffer.GetString() );
}


void EditSpaceDlg::editSpace(
	const BW::wstring & spaceName,
	float chunkSize,
	uint32 heightMapEditorSize,
	uint32 heightMapSize, uint32 normalMapSize,
	uint32 shadowMapSize, uint32 holeMapSize,
	uint32 blendMapSize )
{
#if ENABLE_ASSET_PIPE
	// Lock the asset pipeline to prevent it trying to convert the space
	// we are editing
	AssetLock assetLock;
#endif // ENABLE_ASSET_PIPE

	int currentWidth = spaceSettings_->maxX() - spaceSettings_->minX() + 1;
	int currentHeight = spaceSettings_->maxY() - spaceSettings_->minY() + 1;

	if( expandSpaceValid_ )
	{
		expandSpaceDlg_.OnBnClickedExpandSpaceBtnOk();
		WorldManager::instance().expandSpace(
			expandSpaceDlg_.expandInfo().west_,
			expandSpaceDlg_.expandInfo().east_,
			expandSpaceDlg_.expandInfo().north_,
			expandSpaceDlg_.expandInfo().south_,
			expandSpaceDlg_.expandInfo().includeTerrainInNewChunks_ );
		currentWidth +=
			expandSpaceDlg_.expandInfo().west_ +
			expandSpaceDlg_.expandInfo().east_;
		currentHeight +=
			expandSpaceDlg_.expandInfo().north_ +
			expandSpaceDlg_.expandInfo().south_;
	}

	Terrain::TerrainSettingsPtr terrainSettings
		= spaceSettings_->terrainSettings();

	if( modifyTextureLodValid_ )
	{
		WorldManager::instance().quickSave();
	}

	if( terrainSettings->heightMapEditorSize() == heightMapEditorSize &&
		terrainSettings->heightMapSize() == heightMapSize &&
		terrainSettings->normalMapSize() == normalMapSize &&
		terrainSettings->shadowMapSize() == shadowMapSize &&
		terrainSettings->holeMapSize() == holeMapSize &&
		terrainSettings->blendMapSize() == blendMapSize &&
		spaceSettings_->gridSize() == chunkSize )
	{
		OnOK();
		return;
	}

	SpaceRecreator::TerrainInfo terrainInfo;
	terrainInfo.version_		= Terrain::TerrainSettings::TERRAIN2_VERSION;
	terrainInfo.heightMapEditorSize_ = heightMapEditorSize;
	terrainInfo.heightMapSize_	= heightMapSize;
	terrainInfo.normalMapSize_	= normalMapSize;
	terrainInfo.holeMapSize_	= holeMapSize;
	terrainInfo.shadowMapSize_	= shadowMapSize;
	terrainInfo.blendMapSize_	= blendMapSize;

	BW::string srcSpaceName = bw_wtoutf8( spaceName );
	BW::string targetSpaceName = srcSpaceName + ".temp";
	SpaceRecreator spaceRecreator(
		targetSpaceName, chunkSize, currentWidth, currentHeight, terrainInfo );
	bool result = spaceRecreator.create();
	if( result == false )
	{
		RemoveDirectory( bw_utf8tow( targetSpaceName ).c_str() );
		return;
	}

	BWResource & bwResource = BWResource::instance();
	IFileSystem::Directory dir;
	bwResource.fileSystem()->readDirectory( dir, targetSpaceName );

	srcSpaceName = BWUtil::formatPath( srcSpaceName );
	targetSpaceName = BWUtil::formatPath( targetSpaceName );

	BW::vector< BW::string > backupFiles;
	backupFiles.reserve( dir.size() );
	IFileSystem::Directory::iterator it = dir.begin();
	for ( ; it != dir.end(); ++it )
	{
		BW::string & fileName = *it;
		BW::string dstPath = srcSpaceName + fileName;
		BW::string srcPath = targetSpaceName + fileName;
		if( MoveFileEx( bw_utf8tow( dstPath ).c_str(),
				bw_utf8tow( dstPath + ".bak" ).c_str(),
				MOVEFILE_REPLACE_EXISTING ) == FALSE &&
			GetLastError() != ERROR_FILE_NOT_FOUND )
		{
			result = false;
			break;
		}
		backupFiles.push_back( dstPath );
		if( MoveFile( bw_utf8tow( srcPath ).c_str(),
			bw_utf8tow( dstPath ).c_str() ) == FALSE )
		{
			result = false;
			break;
		}
	}

	if(result)
	{
		//Successful, delete all backup files
		BW::vector< BW::string >::const_iterator it = backupFiles.begin();
		for ( ; it != backupFiles.end(); ++it )
		{
			const BW::string & fileName = *it;
			DeleteFile( bw_utf8tow( fileName + ".bak" ).c_str() );
		}
	}
	else
	{
		//Failed, revert all files
		BW::vector< BW::string >::const_iterator it = backupFiles.begin();
		for ( ; it != backupFiles.end(); ++it )
		{
			const BW::string & fileName = *it;
			MoveFile( bw_utf8tow( fileName + ".bak" ).c_str(),
				bw_utf8tow( fileName ).c_str() );
		}
	}

	RemoveDirectory( bw_utf8tow( targetSpaceName ).c_str() );

	WorldManager::instance().reloadAllChunks( false );
	OnOK();
}


void EditSpaceDlg::formatPerChunkToPerMetre( CString& str )
{
	BW_GUARD;

	int val = _wtoi( str.GetString() );
	float chunkSize = currentChunkSize();

	if ( val / chunkSize > 1.f )
		str.Format( L"%.0f", val / chunkSize );
	else
		str.Format( L"%.2f", val / chunkSize );
}


float EditSpaceDlg::currentChunkSize()
{
	BW_GUARD;

	CString chunkSizeStr;
	chunkSize_.GetWindowText( chunkSizeStr );
	return (float)_wtoi( chunkSizeStr.GetString() );
}


void EditSpaceDlg::formatPerChunkToMetresPer( CString& str )
{
	BW_GUARD;

	int val = _wtoi( str.GetString() );
	float chunkSize = currentChunkSize();

	str.Format( L"%.2f", chunkSize / val );
}


void EditSpaceDlg::OnEnChangeChunkSize()
{
	BW_GUARD;

	CString chunkSize;

	chunkSize_.GetWindowText( chunkSize );

	OnCbnSelChangeEditorHeightmapSize();
	OnCbnSelChangeHeightmapSize();
	OnCbnSelChangeNormalmapSize();
	OnCbnSelChangeHolemapSize();
	OnCbnSelChangeBlendmapSize();
	OnCbnSelChangeShadowmapSize();

	validateSizeValues();
}


BOOL EditSpaceDlg::OnSetCursor( CWnd* wnd, UINT nHitTest, UINT msg )
{
	if (busy_)
	{
		::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

		return TRUE;
	}

	return CDialog::OnSetCursor( wnd, nHitTest, msg );
}


void EditSpaceDlg::OnCbnSelChangeHeightmapSize()
{
	BW_GUARD;

	CString editorHeightMapString, heightMapString;
	int heightMapEditorSize, heightMapSize;

	heightMapEditorSize_.GetWindowText( editorHeightMapString );
	heightMapEditorSize = _wtoi( editorHeightMapString );
	heightMapSize_.GetWindowText( heightMapString );
	heightMapSize = _wtoi( heightMapString );

	if (heightMapEditorSize < heightMapSize)
	{
		heightMapEditorSize_.SelectString( -1, heightMapString );
		formatPerChunkToMetresPer( heightMapString );
		heightMapEditorRes_.SetWindowText( heightMapString );
		heightMapRes_.SetWindowText( heightMapString );
	}
	else
	{
		formatPerChunkToMetresPer( heightMapString );
		heightMapRes_.SetWindowText( heightMapString );
	}

	validateSizeValues();
}

void EditSpaceDlg::OnCbnSelChangeEditorHeightmapSize()
{
	BW_GUARD;

	CString editorHeightMapString, heightMapString;
	int heightMapEditorSize, heightMapSize;

	heightMapEditorSize_.GetWindowText( editorHeightMapString );
	heightMapEditorSize = _wtoi( editorHeightMapString );
	heightMapSize_.GetWindowText( heightMapString );
	heightMapSize = _wtoi( heightMapString );

	if (heightMapEditorSize < heightMapSize)
	{
		heightMapSize_.SelectString( -1, editorHeightMapString );
		formatPerChunkToMetresPer( editorHeightMapString );
		heightMapRes_.SetWindowText( editorHeightMapString );
		heightMapEditorRes_.SetWindowText( editorHeightMapString );
	}
	else
	{
		formatPerChunkToMetresPer( editorHeightMapString );
		heightMapEditorRes_.SetWindowText( editorHeightMapString );
	}

	validateSizeValues();
}


void EditSpaceDlg::OnCbnSelChangeNormalmapSize()
{
	BW_GUARD;

	CString str;

	normalMapSize_.GetWindowText( str );
	formatPerChunkToMetresPer( str );
	normalMapRes_.SetWindowText( str );

	validateSizeValues();
}


void EditSpaceDlg::OnCbnSelChangeHolemapSize()
{
	BW_GUARD;

	CString str;

	holeMapSize_.GetWindowText( str );
	formatPerChunkToMetresPer( str );
	holeMapRes_.SetWindowText( str );

	validateSizeValues();
}


void EditSpaceDlg::OnCbnSelChangeBlendmapSize()
{
	BW_GUARD;

	CString str;

	blendMapSize_.GetWindowText( str );
	formatPerChunkToPerMetre( str );
	blendMapRes_.SetWindowText( str );

	validateSizeValues();
}


void EditSpaceDlg::OnCbnSelChangeShadowmapSize()
{
	BW_GUARD;

	CString str;

	shadowMapSize_.GetWindowText( str );
	formatPerChunkToPerMetre( str );
	shadowMapRes_.SetWindowText( str );

	validateSizeValues();
}


/*afx_msg */void EditSpaceDlg::OnKickIdle()
{
	DWORD oldTickcount = tickCount_;
	tickCount_ = GetTickCount();
	DWORD ticks = tickCount_ - oldTickcount;
	WorldEditorApp::instance().OnIdle( ticks );
}


/*virtual */void EditSpaceDlg::OnCancel()
{
	terrainTextureLod_.restoreSettings( true );
	CDialog::OnCancel();
}


/*virtual */void EditSpaceDlg::validationChange( bool validationResult )
{
	expandSpaceValid_ = validationResult;
	updateValidation();
}


/*virtual */void EditSpaceDlg::valueChanged()
{
	modifyTextureLodValid_ = true;
	updateValidation();
}


void EditSpaceDlg::updateValidation()
{
	BOOL canApply = modifySizeValid_ ||
		expandSpaceValid_ ||
		modifyTextureLodValid_;
	buttonApply_.EnableWindow( canApply );
}

BW_END_NAMESPACE

