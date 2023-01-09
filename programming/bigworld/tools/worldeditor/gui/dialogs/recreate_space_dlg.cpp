#include "pch.hpp"
#include "worldeditor/gui/dialogs/recreate_space_dlg.hpp"
#include "worldeditor/import/space_settings.hpp"
#include "worldeditor/terrain/space_recreator.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/project/chunk_walker.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/items/editor_chunk_light.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "appmgr/options.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/zip_section.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "chunk/chunk_format.hpp"
#include "controls/dir_dialog.hpp"
#include "controls/utils.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"
#include "moo/effect_visual_context.hpp"
#include "cstdmf/debug.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "math/math_extra.hpp"
#include "cstdmf/bw_set.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

static const int RECREATESPACEDLG_MAX_CHUNKS = 1000;
static wchar_t* RECREATESPACEDLG_KM_FORMAT = L"(%.1f km)";
static wchar_t* RECREATESPACEDLG_M_FORMAT = L"(%d m)";

// RecreateSpace dialog

IMPLEMENT_DYNAMIC(RecreateSpaceDlg, CDialog)
RecreateSpaceDlg::RecreateSpaceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(RecreateSpaceDlg::IDD, pParent)
	, defaultSpacePath_( "spaces" )
	, spaceSettings_( NULL )
{
}

RecreateSpaceDlg::~RecreateSpaceDlg()
{
}


void RecreateSpaceDlg::setSpaceSettings(
	SpaceSettings *spaceSettings )
{
	spaceSettings_ = spaceSettings;
}


void RecreateSpaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPACE, space_);
	DDX_Control(pDX, IDC_SPACE_PATH, spacePath_);
	DDX_Control(pDX, IDC_CHUNKSIZE, chunkSize_);
	DDX_Control(pDX, IDC_WIDTH, width_);
	DDX_Control(pDX, IDC_HEIGHT, height_);
	DDX_Control(pDX, IDC_HEIGHTMAP_SIZE, heightMapSize_);
	DDX_Control(pDX, IDC_HEIGHTMAP_EDITOR_SIZE, heightMapEditorSize_);
	DDX_Control(pDX, IDC_NORMALMAP_SIZE, normalMapSize_);
	DDX_Control(pDX, IDC_HOLEMAP_SIZE, holeMapSize_);
	DDX_Control(pDX, IDC_SHADOWMAP_SIZE, shadowMapSize_);
	DDX_Control(pDX, IDC_BLENDMAP_SIZE, blendMapSize_);
	DDX_Control(pDX, IDOK, buttonCreate_);
	DDX_Control(pDX, IDCANCEL, buttonCancel_);

	DDX_Control(pDX, IDC_RECREATESPACE_WIDTH_KM, widthKms_);
	DDX_Control(pDX, IDC_RECREATESPACE_HEIGHT_KM, heightKms_);
	DDX_Control(pDX, IDC_HEIGHTMAP_RES, heightMapRes_);
	DDX_Control(pDX, IDC_HEIGHTMAP_EDITOR_RES, heightMapEditorRes_);
	DDX_Control(pDX, IDC_NORMALMAP_RES, normalMapRes_);
	DDX_Control(pDX, IDC_HOLEMAP_RES, holeMapRes_);
	DDX_Control(pDX, IDC_SHADOWMAP_RES,shadowMapRes_);
	DDX_Control(pDX, IDC_BLENDMAP_RES, blendMapRes_);
	DDX_Control(pDX, IDC_RECREATESPACE_WARNINGS, warnings_ );
}


BEGIN_MESSAGE_MAP(RecreateSpaceDlg, CDialog)
	ON_EN_CHANGE(IDC_SPACE, OnEnChangeSpace)
	ON_EN_CHANGE(IDC_SPACE_PATH, OnEnChangeSpacePath)
	ON_BN_CLICKED(IDC_RECREATESPACE_BROWSE, OnBnClickedRecreatespaceBrowse)
	ON_EN_CHANGE(IDC_CHUNKSIZE, OnEnChangeChunkSize)
	ON_EN_KILLFOCUS(IDC_CHUNKSIZE, OnEnKillFocusChunkSize)
	ON_EN_CHANGE(IDC_WIDTH, OnEnChangeWidth)
	ON_EN_CHANGE(IDC_HEIGHT, OnEnChangeHeight)
	ON_CBN_SELCHANGE(IDC_HEIGHTMAP_EDITOR_SIZE, OnCbnSelChangeEditorHeightmapSize)
	ON_CBN_SELCHANGE(IDC_HEIGHTMAP_SIZE, OnCbnSelChangeHeightmapSize)
	ON_CBN_SELCHANGE(IDC_NORMALMAP_SIZE, OnCbnSelChangeNormalmapSize)
	ON_CBN_SELCHANGE(IDC_HOLEMAP_SIZE, OnCbnSelChangeHolemapSize)
	ON_CBN_SELCHANGE(IDC_BLENDMAP_SIZE, OnCbnSelChangeBlendmapSize)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_SHADOWMAP_SIZE, &RecreateSpaceDlg::OnCbnSelchangeShadowmapSize)
END_MESSAGE_MAP()


void RecreateSpaceDlg::initIntEdit(
	controls::EditNumeric& edit, int minVal, int maxVal, int val )
{
	BW_GUARD;

	edit.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	edit.SetAllowNegative( minVal < 0 );
	edit.SetMinimum( float( minVal ) );
	edit.SetMaximum( float( maxVal ) );
	edit.SetIntegerValue( val );
}


// RecreateSpace message handlers

BOOL RecreateSpaceDlg::OnInitDialog()
{
	BW_GUARD;

	if (!CDialog::OnInitDialog())
		return FALSE;

	INIT_AUTO_TOOLTIP();
	
	float srcChunkSize;
	int srcMinX, srcMinZ, srcMaxX, srcMaxZ;
	TerrainUtils::terrainSize( "", srcChunkSize, srcMinX, srcMinZ, srcMaxX, srcMaxZ );

	bool hasSettings = spaceSettings_ != NULL;

	int desiredWidth = 10;
	int desiredHeight = 10;
	float gridSize = DEFAULT_GRID_RESOLUTION;

	Terrain::TerrainSettingsPtr terrainSettings = hasSettings
		? spaceSettings_->terrainSettings()
		: Terrain::TerrainSettings::defaults();

	if( hasSettings )
	{
		int gridMinX;
		int gridMinY;
		int gridMaxX;
		int gridMaxY;
		TerrainUtils::terrainSize( "", gridSize, gridMinX, gridMinY, gridMaxX, gridMaxY );
		desiredWidth = gridMaxX - gridMinX;
		desiredHeight = gridMaxY - gridMinY;
	}

	initIntEdit( chunkSize_, 10, (int)MAX_SPACE_WIDTH, (int)srcChunkSize );
	initIntEdit( width_, srcMaxX - srcMinX + 1, RECREATESPACEDLG_MAX_CHUNKS, srcMaxX - srcMinX + 1 );
	initIntEdit( height_, srcMaxZ - srcMinZ + 1, RECREATESPACEDLG_MAX_CHUNKS, srcMaxZ - srcMinZ + 1 );

	wchar_t buffer[64];
	int size = MIN_MAP_SIZE;
	while ( size <= MAX_MAP_SIZE )
	{
		bw_snwprintf(buffer, ARRAY_SIZE(buffer), L"%d", size );

		heightMapEditorSize_.AddString( buffer );
		heightMapSize_.AddString( buffer );
		normalMapSize_.AddString( buffer );
		holeMapSize_.AddString( buffer );
		shadowMapSize_.AddString( buffer );
		blendMapSize_.AddString( buffer );	
		size *= 2;
	}

	Terrain::TerrainSettingsPtr ts = WorldManager::instance().pTerrainSettings();

	uint32 heightMapSize = ts->heightMapSize();
	uint32 heightMapEditorSize = max( heightMapSize, ts->heightMapEditorSize() );

	_itow( heightMapSize, buffer, 10 );
	heightMapSize_.SelectString( -1, buffer );
	_itow( heightMapEditorSize, buffer, 10 );
	heightMapEditorSize_.SelectString( -1, buffer );
	_itow( ts->normalMapSize(), buffer, 10 );
	normalMapSize_.SelectString( -1, buffer );
	_itow( ts->holeMapSize(), buffer, 10 );
	holeMapSize_.SelectString( -1, buffer );
	_itow( ts->shadowMapSize(), buffer, 10 );
	shadowMapSize_.SelectString( -1, buffer );
	_itow( ts->blendMapSize(), buffer, 10 );
	blendMapSize_.SelectString( -1, buffer );

	const BW::string space = WorldManager::instance().getCurrentSpace();
	const BW::string fullSpace = BWResource::resolveFilename( space );
	space_.SetWindowText( bw_utf8tow( BWResource::getFilename( fullSpace ).to_string() ).c_str() );
	spacePath_.SetWindowText( bw_utf8tow( BWResource::getFilePath( fullSpace ) ).c_str() );

	UpdateData( FALSE );

	chunkSize_.setSilent( false );
	width_.setSilent( false );
	height_.setSilent( false );

	// Refresh all static data that uses ChunkSize
	OnEnChangeChunkSize();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void RecreateSpaceDlg::OnBnClickedOk()
{
	BW_GUARD;

	CWaitCursor wait;

	// Create the space!
	CString s, p, z, w, h;
	space_.GetWindowText( s );
	spacePath_.GetWindowText( p );
	chunkSize_.GetWindowText( z );
	width_.GetWindowText( w );
	height_.GetWindowText( h );

	if ( !p.IsEmpty() && p.Right( 1 ) != "/" && p.Right( 1 ) != "\\" )
		p = p + L"/";

	BW::wstring spaceName = ( p + s ).GetString();

	if ( s.IsEmpty() )
	{
		BW::wstring msg = L"There is no space name specified.\n\nFill in the space name, e.g. \"space001\".";
		MessageBox( msg.c_str(), L"No space name given", MB_ICONERROR );
		space_.SetSel( 0, -1 ); // Select the space name field
		space_.SetFocus();
		return;
	}
	
	if ( p.IsEmpty() || BWResource::dissolveFilename( bw_wtoutf8( p.GetString( ) ) ) == "" )
	{
		BW::wstring msg = L"The space cannot be located in the root directory."
			L"\n\nWould you like to create it in \""
			+ bw_utf8tow( BWResource::resolveFilename( defaultSpacePath_ ) ) + L"/" + s.GetString() + L"\"?";
		if ( MessageBox( msg.c_str(), L"No space folder given", MB_ICONINFORMATION | MB_OKCANCEL ) == IDCANCEL )
		{
			spacePath_.SetSel( 0, -1 ); // Select the space path field
			spacePath_.SetFocus();
			return;
		}
		spaceName = bw_utf8tow( BWResource::resolveFilename( defaultSpacePath_ ) ) + L"/" + s.GetString();
	}
	else if ( BWResource::fileExists( bw_wtoutf8( spaceName ) + "/" + SPACE_SETTING_FILE_NAME ) )
	{
		BW::wstring msg = BW::wstring( L"The folder \"" )
			+ spaceName + L"\" already contains a space."
			L"\nPlease use a different space name or select a different folder.";
		MessageBox( msg.c_str(), L"Folder already contains a space", MB_ICONERROR );
		spacePath_.SetSel( 0, -1 ); // Select the space path field
		spacePath_.SetFocus();
		return;
	}
	else if ( bw_MW_strcmp( BWResource::dissolveFilename( bw_wtoutf8( spaceName ) ).c_str(), spaceName.c_str() ) == 0 )
	{
		BW::wstring msg = BW::wstring( L"The folder \"" )
			+ p.GetString() +
			L"\" is not in a BigWorld path."
			L"\nPlease correct the path, or select the desired folder using the '...' button";
		MessageBox( msg.c_str(), L"Folder not inside a BigWorld path.", MB_ICONERROR );
		spacePath_.SetSel( 0, -1 ); // Select the space path field
		spacePath_.SetFocus();
		return;
	}

	float chunkSize = (float)_wtoi( z.GetString() );
	int width = _wtoi( w.GetString() );
	int height = _wtoi( h.GetString() );

	SpaceRecreator::TerrainInfo terrainInfo;
	CString buffer;
	terrainInfo.version_		= Terrain::TerrainSettings::TERRAIN2_VERSION;
	heightMapEditorSize_.GetLBText( heightMapEditorSize_.GetCurSel(), buffer );
	terrainInfo.heightMapEditorSize_	= _wtoi( buffer.GetString() );
	heightMapSize_.GetLBText( heightMapSize_.GetCurSel(), buffer );
	terrainInfo.heightMapSize_	= _wtoi( buffer.GetString() );
	normalMapSize_.GetLBText( normalMapSize_.GetCurSel(), buffer );
	terrainInfo.normalMapSize_	= _wtoi( buffer.GetString() );
	holeMapSize_.GetLBText( holeMapSize_.GetCurSel(), buffer );
	terrainInfo.holeMapSize_	= _wtoi( buffer.GetString() );
	shadowMapSize_.GetLBText( shadowMapSize_.GetCurSel(), buffer );
	terrainInfo.shadowMapSize_	= _wtoi( buffer.GetString() );
	blendMapSize_.GetLBText( blendMapSize_.GetCurSel(), buffer );
	terrainInfo.blendMapSize_	= _wtoi( buffer.GetString() );
	
	// store the defaults for next time
	// TODO: This changes defaults for NewSpaceDlg too! Is that what we want?
	Options::setOptionInt( "terrain2/defaults/heightMapEditorSize", terrainInfo.heightMapEditorSize_ );
	Options::setOptionInt( "terrain2/defaults/heightMapSize", terrainInfo.heightMapSize_ );
	Options::setOptionInt( "terrain2/defaults/normalMapSize", terrainInfo.normalMapSize_ );
	Options::setOptionInt( "terrain2/defaults/holeMapSize", terrainInfo.holeMapSize_ );
	Options::setOptionInt( "terrain2/defaults/shadowMapSize", terrainInfo.shadowMapSize_ );
	Options::setOptionInt( "terrain2/defaults/blendMapSize", terrainInfo.blendMapSize_ );

	SpaceRecreator recreator( bw_wtoutf8( spaceName ),
		chunkSize, width, height,
		terrainInfo );
	buttonCreate_.EnableWindow( FALSE );
	buttonCancel_.EnableWindow( FALSE );
	EnableWindow( FALSE );
	UpdateWindow();
	bool result = recreator.create();
	EnableWindow( TRUE );
	buttonCreate_.EnableWindow( TRUE );
	buttonCancel_.EnableWindow( TRUE );
	if ( !result )
	{
		MessageBox( bw_utf8tow( recreator.errorMessage() ).c_str(), L"Unable to create space", MB_ICONERROR );
	}
	else
	{
		createdSpace_ = bw_utf8tow( BWResource::dissolveFilename( bw_wtoutf8( spaceName ) ) ).c_str();
		OnOK();
	}
}

void RecreateSpaceDlg::validateFile( CEdit& ctrl, bool isPath )
{
	BW_GUARD;

	static CString invalidFileChar = L"\\/:*?\"<>| ";
	static CString invalidDirChar = L"*?\"<>|";
	CString s;
	ctrl.GetWindowText( s );

	bool changed = false;
	for (int i = 0; i < s.GetLength(); ++i)
	{
		if (isupper(s[i]))
		{
			changed = true;
			s.SetAt(i, tolower(s[i]));
		}
		else if ( !isPath && ( s[i] == ':' || s[i] == '/' ) )
		{
			changed = true;
			s.SetAt(i, '_');
		}
		else if ( isPath && s[i] == '\\' )
		{
			changed = true;
			s.SetAt(i, '/');
		}
		else if ( isPath && invalidDirChar.Find( s[i] ) != -1 )
		{
			changed = true;
			s.SetAt(i, '_');
		}
		else if ( !isPath && invalidFileChar.Find( s[i] ) != -1 )
		{
			changed = true;
			s.SetAt(i, '_');
		}
	}

	if ( changed || s.GetLength() > MAX_PATH )
	{
		int start, end;
		ctrl.GetSel( start, end );
		s = s.Left( MAX_PATH );
		ctrl.SetWindowText( s );
		ctrl.SetSel( start, end );
	}
}

float RecreateSpaceDlg::currentChunkSize() {
	BW_GUARD;

	CString chunkSizeStr;
	chunkSize_.GetWindowText( chunkSizeStr );
	return (float)_wtoi( chunkSizeStr.GetString() );
}

void RecreateSpaceDlg::formatChunksToKms( CString& str )
{
	BW_GUARD;

	int val = _wtoi( str.GetString() );
	float chunkSize = currentChunkSize();

	if ( val * chunkSize >= 1000 )
		str.Format( RECREATESPACEDLG_KM_FORMAT, val * chunkSize / 1000.0f );
	else
		str.Format( RECREATESPACEDLG_M_FORMAT, (int) ( val * chunkSize ) );
}

void RecreateSpaceDlg::formatPerChunkToPerMetre( CString& str )
{
	BW_GUARD;

	int val = _wtoi( str.GetString() );
	float chunkSize = currentChunkSize();

	if ( val / chunkSize > 1.f )
		str.Format( L"%.0f", val / chunkSize );
	else
		str.Format( L"%.2f", val / chunkSize );
}

void RecreateSpaceDlg::formatPerChunkToMetresPer( CString& str )
{
	BW_GUARD;

	int val = _wtoi( str.GetString() );
	float chunkSize = currentChunkSize();

	str.Format( L"%.2f", chunkSize / val );
}

void RecreateSpaceDlg::refreshHeightmapRes()
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
}

void RecreateSpaceDlg::refreshEditorHeightmapRes()
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
}

void RecreateSpaceDlg::refreshHolemapRes()
{
	BW_GUARD;

	CString str;

	holeMapSize_.GetWindowText( str );
	formatPerChunkToMetresPer( str );
	holeMapRes_.SetWindowText( str );
}

void RecreateSpaceDlg::refreshShadowmapRes()
{
	BW_GUARD;

	CString str;

	shadowMapSize_.GetWindowText( str );
	formatPerChunkToMetresPer( str );
	shadowMapRes_.SetWindowText( str );
}


void RecreateSpaceDlg::refreshBlendmapRes()
{
	BW_GUARD;

	CString str;

	blendMapSize_.GetWindowText( str );
	formatPerChunkToPerMetre( str );
	blendMapRes_.SetWindowText( str );
}

void RecreateSpaceDlg::refreshWarnings()
{
	BW_GUARD;
	warnings_.ResetContent();

	float srcChunkSize;
	int srcMinX, srcMinZ, srcMaxX, srcMaxZ;
	TerrainUtils::terrainSize( "", srcChunkSize, srcMinX, srcMinZ, srcMaxX, srcMaxZ );
	Terrain::TerrainSettingsPtr pSrcTerrain = WorldManager::instance().pTerrainSettings();
	float srcEditorHeightMapRes = pSrcTerrain->heightMapEditorSize() / srcChunkSize;
	float srcHeightMapRes = pSrcTerrain->heightMapSize() / srcChunkSize;
	float srcHoleMapRes = pSrcTerrain->holeMapSize() / srcChunkSize;
	float srcBlendMapRes = pSrcTerrain->blendMapSize() / srcChunkSize;
	float srcShadowMapRes = pSrcTerrain->shadowMapSize() / srcChunkSize; 

	float chunkSize = currentChunkSize();
	CString str;
	heightMapEditorSize_.GetWindowText( str );
	float editorHeightMapRes = _wtoi( str.GetString() ) / chunkSize;
	heightMapSize_.GetWindowText( str );
	float heightMapRes = _wtoi( str.GetString() ) / chunkSize;
	holeMapSize_.GetWindowText( str );
	float holeMapRes = _wtoi( str.GetString() ) / chunkSize;
	shadowMapSize_.GetWindowText( str );
	float shadowMapRes = _wtoi( str.GetString() ) / chunkSize;
	blendMapSize_.GetWindowText( str );
	float blendMapRes = _wtoi( str.GetString() ) / chunkSize;
	

	if ( editorHeightMapRes <= (srcEditorHeightMapRes / 2.f) )
	{
		warnings_.AddString( Localise( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/WARNING_HEIGHTMAP_RES_REDUCED" ) );
	}

	if ( heightMapRes <= (srcHeightMapRes / 2.f) )
	{
		warnings_.AddString( Localise( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/WARNING_HEIGHTMAP_RES_REDUCED" ) );
	}

	if ( holeMapRes <= (srcHoleMapRes / 2.f) )
	{
		warnings_.AddString( Localise( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/WARNING_HOLEMAP_RES_REDUCED" ) );
	}

	if ( blendMapRes <= (srcBlendMapRes / 2.f) )
	{
		warnings_.AddString( Localise( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/WARNING_BLENDMAP_RES_REDUCED" ) );
	}

	if ( shadowMapRes <= (srcShadowMapRes / 2.f) )
	{
		warnings_.AddString( Localise( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/WARNING_SHADOWMAP_RES_REDUCED" ) );
	}
}

void RecreateSpaceDlg::OnEnChangeSpace()
{
	BW_GUARD;

	validateFile( space_, false );
}

void RecreateSpaceDlg::OnEnChangeSpacePath()
{
	BW_GUARD;

	validateFile( spacePath_, true );
}

void RecreateSpaceDlg::OnBnClickedRecreatespaceBrowse()
{
	BW_GUARD;

	DirDialog dlg;

	dlg.windowTitle_ = _T("New Space Folder");
	dlg.promptText_  = _T("Choose a folder for the new space...");
	dlg.fakeRootDirectory_ = dlg.basePath();

	CString curpath;
	spacePath_.GetWindowText( curpath );

	// Set the start directory, check if exists:
	if ( BWResource::instance().fileSystem()->getFileType( bw_wtoutf8( curpath.GetString( ) ) ) == IFileSystem::FT_DIRECTORY )
		dlg.startDirectory_ = BWResource::resolveFilename( bw_wtoutf8( curpath.GetString( ) ) ).c_str();
	else
		dlg.startDirectory_ = dlg.basePath();

	if (dlg.doBrowse(AfxGetApp()->m_pMainWnd)) 
	{
		spacePath_.SetWindowText( dlg.userSelectedDirectory_ );
	}
}

void RecreateSpaceDlg::OnEnChangeChunkSize()
{
	BW_GUARD;

	OnEnChangeWidth();
	OnEnChangeHeight();
	refreshHeightmapRes();
	refreshEditorHeightmapRes();
	OnCbnSelChangeNormalmapSize();
	refreshHolemapRes();
	refreshShadowmapRes();
	refreshBlendmapRes();

	refreshWarnings();
}

void RecreateSpaceDlg::OnEnKillFocusChunkSize()
{
	BW_GUARD;

	float chunkSize = currentChunkSize();

	float srcChunkSize;
	int srcMinX, srcMinZ, srcMaxX, srcMaxZ;
	TerrainUtils::terrainSize( "", srcChunkSize, srcMinX, srcMinZ, srcMaxX, srcMaxZ );
	float chunkRatio = srcChunkSize / chunkSize;

	//To fix floating point rounding errors.
	const float epsilon = std::numeric_limits<float>::epsilon();

	float minWidth = ( srcMaxX - srcMinX + 1 ) * chunkRatio;
	minWidth = ceilf( minWidth - epsilon * floor( minWidth ) );
	width_.SetMinimum( minWidth );
	int curWidth = width_.GetIntegerValue();
	width_.SetIntegerValue( ::max( (int)minWidth, curWidth ) );

	float minHeight = ( srcMaxZ - srcMinZ + 1 ) * chunkRatio;
	minHeight = ceilf( minHeight - epsilon * floor( minHeight ) );
	height_.SetMinimum( minHeight );
	int curHeight = height_.GetIntegerValue();
	height_.SetIntegerValue( ::max( (int)minHeight, curHeight ) );
}

void RecreateSpaceDlg::OnEnChangeWidth()
{
	BW_GUARD;

	CString str;

	width_.GetWindowText( str );
	formatChunksToKms( str );
	widthKms_.SetWindowText( str );
}

void RecreateSpaceDlg::OnEnChangeHeight()
{
	BW_GUARD;

	CString str;

	height_.GetWindowText( str );
	formatChunksToKms( str );
	heightKms_.SetWindowText( str );
}

void RecreateSpaceDlg::OnCbnSelChangeHeightmapSize()
{
	BW_GUARD;
	refreshHeightmapRes();
	refreshWarnings();
}

void RecreateSpaceDlg::OnCbnSelChangeEditorHeightmapSize()
{
	BW_GUARD;
	refreshEditorHeightmapRes();
	refreshWarnings();
}

void RecreateSpaceDlg::OnCbnSelChangeNormalmapSize()
{
	BW_GUARD;

	CString str;

	normalMapSize_.GetWindowText( str );
	formatPerChunkToMetresPer( str );
	normalMapRes_.SetWindowText( str );
}

void RecreateSpaceDlg::OnCbnSelChangeHolemapSize()
{
	BW_GUARD;
	refreshHolemapRes();
	refreshWarnings();
}

void RecreateSpaceDlg::OnCbnSelChangeBlendmapSize()
{
	BW_GUARD;
	refreshBlendmapRes();
	refreshWarnings();
}


void RecreateSpaceDlg::OnCbnSelchangeShadowmapSize()
{
	BW_GUARD;
	refreshShadowmapRes();
	refreshWarnings();
}
BW_END_NAMESPACE

