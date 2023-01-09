#include "pch.hpp"
#include "worldeditor/gui/dialogs/new_space_dlg.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "appmgr/options.hpp"
#include "asset_pipeline/asset_lock.hpp"
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
#include "moo/effect_visual_context.hpp"
#include "cstdmf/debug.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "moo/deferred_decals_manager.hpp"
#include "cstdmf/bw_set.hpp"
#include "romp/flora_constants.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_skyXML( "environment/skyXML" );
static AutoConfigString s_spaceDlgBlankCDataFname( "dummy/cData" );

static const BW::string LAST_DEFAULT_TEXTURE_TAG = "space/lastDefaultTexture";


static const int NEWSPACEDLG_MAX_CHUNKS = 1000;
static wchar_t* NEWSPACEDLG_KM_FORMAT = L"(%.1f km)";
static wchar_t* NEWSPACEDLG_M_FORMAT = L"(%d m)";


// NewSpace dialog

IMPLEMENT_DYNAMIC(NewSpaceDlg, CDialog)
NewSpaceDlg::NewSpaceDlg(CWnd* pParent /*=NULL*/)
    : CDialog(NewSpaceDlg::IDD, pParent)
    , defaultSpacePath_( "spaces" )
    , busy_( false )
    , lastChunkSize_( 0 )
{
}

NewSpaceDlg::~NewSpaceDlg()
{
}

void NewSpaceDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SPACE, space_);
    DDX_Control(pDX, IDC_CHUNKSIZE, chunkSize_);
    DDX_Control(pDX, IDC_WIDTH, width_);
    DDX_Control(pDX, IDC_HEIGHT, height_);
    DDX_Control(pDX, IDC_FLORA_VB, floraVB_);
    DDX_Control(pDX, IDC_FLORA_WIDTH, floraTextureWidth_);
    DDX_Control(pDX, IDC_FLORA_HEIGHT, floraTextureHeight_);
    DDX_Control(pDX, IDC_HEIGHTMAP_SIZE, heightMapSize_);
    DDX_Control(pDX, IDC_HEIGHTMAP_EDITOR_SIZE, heightMapEditorSize_);
    DDX_Control(pDX, IDC_NORMALMAP_SIZE, normalMapSize_);
    DDX_Control(pDX, IDC_HOLEMAP_SIZE, holeMapSize_);
    DDX_Control(pDX, IDC_SHADOWMAP_SIZE, shadowMapSize_);
    DDX_Control(pDX, IDC_BLENDMAP_SIZE, blendMapSize_);
    DDX_Control(pDX, IDC_DEFTEX_PATH, defaultTexture_);
    DDX_Control(pDX, IDC_DEFTEX_IMAGE, textureImage_);
    DDX_Control(pDX, IDC_PROGRESS1, progress_ );
    DDX_Control(pDX, IDC_NEWSPACE_WIDTH_KM, widthKms_);
    DDX_Control(pDX, IDC_NEWSPACE_HEIGHT_KM, heightKms_);
    DDX_Control(pDX, IDC_SPACE_PATH, spacePath_);
    DDX_Control(pDX, IDCANCEL, buttonCancel_);
    DDX_Control(pDX, IDOK, buttonCreate_);
    DDX_Control(pDX, IDC_HEIGHTMAP_RES, heightMapRes_);
    DDX_Control(pDX, IDC_NORMALMAP_RES, normalMapRes_);
    DDX_Control(pDX, IDC_HOLEMAP_RES, holeMapRes_);
    DDX_Control(pDX, IDC_BLENDMAP_RES, blendMapRes_);
    DDX_Control(pDX, IDC_SHADOWMAP_RES, shadowMapRes_);
}


BEGIN_MESSAGE_MAP(NewSpaceDlg, CDialog)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_EN_CHANGE(IDC_SPACE, OnEnChangeSpace)
    ON_BN_CLICKED(IDC_NEWSPACE_BROWSE, OnBnClickedNewspaceBrowse)
    ON_EN_CHANGE(IDC_DEFTEX_PATH, OnEnTextureChange)
    ON_BN_CLICKED(IDC_DEFTEX_BROWSE, OnBnTextureBrowse)
    ON_EN_CHANGE(IDC_CHUNKSIZE, OnEnChangeChunkSize)
    ON_EN_CHANGE(IDC_WIDTH, OnEnChangeWidth)
    ON_EN_CHANGE(IDC_HEIGHT, OnEnChangeHeight)
    ON_EN_CHANGE(IDC_SPACE_PATH, OnEnChangeSpacePath)
    ON_CBN_SELCHANGE(IDC_HEIGHTMAP_EDITOR_SIZE, &NewSpaceDlg::OnCbnSelchangeHeightmapEditorSize)
    ON_CBN_SELCHANGE(IDC_HEIGHTMAP_SIZE, OnCbnSelChangeHeightmapSize)
    ON_CBN_SELCHANGE(IDC_NORMALMAP_SIZE, OnCbnSelChangeNormalmapSize)
    ON_CBN_SELCHANGE(IDC_HOLEMAP_SIZE, OnCbnSelChangeHolemapSize)
    ON_CBN_SELCHANGE(IDC_SHADOWMAP_SIZE, OnCbnSelChangeShadowmapSize)
    ON_CBN_SELCHANGE(IDC_BLENDMAP_SIZE, OnCbnSelChangeBlendmapSize)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()


void NewSpaceDlg::initIntEdit(
    controls::EditNumeric& edit, int minVal, int maxVal, int val )
{
    BW_GUARD;

    edit.SetNumericType( controls::EditNumeric::ENT_INTEGER );
    edit.SetAllowNegative( minVal < 0 );
    edit.SetMinimum( float( minVal ) );
    edit.SetMaximum( float( maxVal ) );
    edit.SetIntegerValue( val );
}


// NewSpace message handlers

BOOL NewSpaceDlg::OnInitDialog()
{
    BW_GUARD;

    if (!CDialog::OnInitDialog())
    {
        return FALSE;
    }
    

    INIT_AUTO_TOOLTIP();
    
    initIntEdit( chunkSize_, 10, (int)MAX_SPACE_WIDTH, (int)DEFAULT_GRID_RESOLUTION );
    initIntEdit( width_, 1, NEWSPACEDLG_MAX_CHUNKS, 10 );
    initIntEdit( height_, 1, NEWSPACEDLG_MAX_CHUNKS, 10 );

    initIntEdit( floraVB_, DEFAULT_FLORA_VB_SIZE / 2, DEFAULT_FLORA_VB_SIZE * 2, DEFAULT_FLORA_VB_SIZE );
    initIntEdit( floraTextureWidth_, DEFAULT_FLORA_TEXTURE_WIDTH / 4,
        DEFAULT_FLORA_TEXTURE_WIDTH * 2, DEFAULT_FLORA_TEXTURE_WIDTH );
    initIntEdit( floraTextureHeight_, DEFAULT_FLORA_TEXTURE_HEIGHT / 4,
        DEFAULT_FLORA_TEXTURE_HEIGHT * 2, DEFAULT_FLORA_TEXTURE_HEIGHT );


    wchar_t buffer[64];
    int size = MIN_MAP_SIZE;
    while ( size <= MAX_MAP_SIZE )
    {
        bw_snwprintf(buffer, ARRAY_SIZE(buffer), L"%d", size );

        heightMapSize_.AddString( buffer );
        heightMapEditorSize_.AddString( buffer );
        normalMapSize_.AddString( buffer );
        holeMapSize_.AddString( buffer );
        shadowMapSize_.AddString( buffer );
        blendMapSize_.AddString( buffer );

        size *= 2;
    }

    int heightMapSize =
        Options::getOptionInt( "terrain2/defaults/heightMapSize", 
            Terrain::TerrainSettings::defaults()->heightMapSize() );
    int heightMapEditorSize = max( heightMapSize,
        Options::getOptionInt( "terrain2/defaults/heightMapEditorSize", 
        Terrain::TerrainSettings::defaults()->heightMapSize() ) );

    _itow( heightMapSize, buffer, 10 );
    heightMapSize_.SelectString( -1, buffer );
    _itow( heightMapEditorSize, buffer, 10 );
    heightMapEditorSize_.SelectString( -1, buffer );
    _itow( Options::getOptionInt( "terrain2/defaults/normalMapSize", 
        Terrain::TerrainSettings::defaults()->normalMapSize() ), buffer, 10 );
    normalMapSize_.SelectString( -1, buffer );

    _itow( Options::getOptionInt( "terrain2/defaults/holeMapSize", 
        Terrain::TerrainSettings::defaults()->holeMapSize() ), buffer, 10 );
    holeMapSize_.SelectString( -1, buffer );
    _itow( Options::getOptionInt( "terrain2/defaults/shadowMapSize", 
        Terrain::TerrainSettings::defaults()->shadowMapSize() ), buffer, 10 );
    shadowMapSize_.SelectString( -1, buffer );
    _itow( Options::getOptionInt( "terrain2/defaults/blendMapSize", 
        Terrain::TerrainSettings::defaults()->blendMapSize() ), buffer, 10 );
    blendMapSize_.SelectString( -1, buffer );

    space_.SetWindowText( defaultSpace_ );
    spacePath_.SetWindowText( bw_utf8tow( BWResource::resolveFilename( defaultSpacePath_ ) ).c_str() );

    defaultTexture_.SetWindowText( bw_utf8tow( Options::getOptionString( LAST_DEFAULT_TEXTURE_TAG ) ).c_str() );
        
    UpdateData( FALSE );

    chunkSize_.setSilent( false );
    width_.setSilent( false );
    height_.setSilent( false );
    floraVB_.setSilent( false );
    floraTextureWidth_.setSilent( false );
    floraTextureHeight_.setSilent( false );

    // Refresh all static data that uses ChunkSize
    OnEnChangeChunkSize();

    return TRUE;  // return TRUE unless you set the focus to a control
}

class SpaceMaker
{
public:

    // Holder structure for user configurable terrain 2 information
    struct TerrainInfo
    {
        uint32  version_;
        uint32  heightMapSize_;
        uint32  heightMapEditorSize_;
        uint32  normalMapSize_;
        uint32  holeMapSize_;
        uint32  shadowMapSize_;
        uint32  blendMapSize_;
    };

    SpaceMaker(
        const BW::string& spaceName, float chunkSize,
        int width, int height, int minHeight, int maxHeight,
        int floraVBSize, int floraTextureWidth, int floraTextureHeight,
        const TerrainInfo& terrainInfo,
        const BW::string& defaultTexture )
        : spaceName_( spaceName ),
          chunkSize_( chunkSize ),
          minY_( minHeight ),
          maxY_( maxHeight ),
          floraVBSize_( floraVBSize ),
          floraTextureWidth_( floraTextureWidth ),
          floraTextureHeight_( floraTextureHeight ),
          terrainInfo_( terrainInfo ),
          defaultTexture_( defaultTexture ),
          progress_( NULL ),
          terrainSize_( 0 ),
          terrainCache_( NULL ),
          memoryCache_( NULL ),
          memoryCacheSize_( 128*1024 )
    {
        BW_GUARD;

        calcBounds( width, height );
    }

    ~SpaceMaker()
    {
        BW_GUARD;

        if (memoryCache_ != NULL)
        {
            VirtualFree(memoryCache_, 0, MEM_RELEASE);
            memoryCache_ = NULL;
        }
        if (terrainCache_ != NULL)
        {
            VirtualFree(terrainCache_, 0, MEM_RELEASE);
            terrainCache_ = NULL;
        }
    }

    bool create()
    {
        BW_GUARD;

        progress_ = new ProgressBarTask (
            LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CREATE_SPACE/STAGE1"),
            (float)(maxX_-minX_+1) * (maxZ_-minZ_+1), true
            );
        bool result = createInternal();
        bw_safe_delete(progress_);
        return result;
    }

    bool createInternal()
    {
        BW_GUARD;
        MF_ASSERT( progress_ != NULL );

        if( WorldManager::instance().connection().enabled() )
        {
            if( !WorldManager::instance().connection().changeSpace( BWResource::dissolveFilename( spaceName_ ) )
                || !WorldManager::instance().connection().lock( GridRect( GridCoord( minX_ - 1, minZ_ - 1 ),
                    GridCoord( maxX_ + 1, maxZ_ + 1 ) ), "create space" ) )
            {
                errorMessage_ = "Unable to lock space";
                return false;
            }
        }

        if ( progress_->isCancelled() )
            return false;

        if (!createSpaceDirectory())
        {
            errorMessage_ = "Unable to create space directory";
            return false;
        }

        if ( progress_->isCancelled() )
            return false;

#if ENABLE_ASSET_PIPE
        AssetLock assetLock;
#endif // ENABLE_ASSET_PIPE

        if (!createSpaceSettings())
        {
            errorMessage_ = "Unable to create " + SPACE_SETTING_FILE_NAME;
            return false;
        }

        if (!createFloraSettings())
        {
            errorMessage_ = "Unable to create " + SPACE_FLORA_SETTING_FILE_NAME;
            return false;
        }

        if ( progress_->isCancelled() )
            return false;

        if (!createTemplateTerrainBlock())
        {
            errorMessage_ = "Unable to create a terrain block for the space";
            return false;
        }

        if ( progress_->isCancelled() )
            return false;

        // True means purge every 'n' chunks to avoid wasting memory
        if (!createChunks( true ))
        {
            if(errorMessage_.empty())
                errorMessage_ = "Unable to create chunks";
            return false;
        }

        return true;
    }

    BW::string errorMessage() { return errorMessage_; }

private:

    bool createSpaceDirectory()
    {
        BW_GUARD;

        BWResource::ensurePathExists( spaceName_ + "/" );

        return BWResource::openSection( BWResource::dissolveFilename( spaceName_ ) );
    }

    void calcBounds( int width, int height )
    {
        BW_GUARD;

        /*
        // Standard bounds calc
        */
        int w = width / 2;
        int h = height / 2;

        minX_ = -w;
        maxX_ = w - 1;
        if (w * 2 != width)
            ++maxX_;

        minZ_ = -h;
        maxZ_ = h - 1;
        if (h * 2 != height)
            ++maxZ_;
        /*
        // Corner bounds calc
        minX_ = 0;
        maxX_ = width_;
        minZ_ = -height_;
        maxZ_ = -1;
        */
    }


    bool createSpaceSettings()
    {
        BW_GUARD;

        DataSectionPtr settings = BWResource::openSection(
            BWResource::dissolveFilename( spaceName_ ) )->newSection( SPACE_SETTING_FILE_NAME );

        if (!settings)
            return false;       
        
        BW::string skyXML = s_skyXML.value();
        if (skyXML.empty())
            skyXML = "system/data/sky.xml";
        settings->writeString( "timeOfDay", skyXML);
        settings->writeString( "skyGradientDome", skyXML );
        settings->writeFloat( "chunkSize", chunkSize_ );
        settings->writeString( "flora", 
            BWResource::dissolveFilename( spaceName_ ) + 
            "/" + SPACE_FLORA_SETTING_FILE_NAME );

        Vector2 fadeRange = DecalsManager::instance().getFadeRange();
        settings->writeFloat( "decals/fadeStart", fadeRange.x);
        settings->writeFloat( "decals/fadeEnd", fadeRange.y);

        settings->writeInt( "bounds/minX", minX_ );
        settings->writeInt( "bounds/maxX", maxX_ );
        settings->writeInt( "bounds/minY", minZ_ );
        settings->writeInt( "bounds/maxY", maxZ_ );
        // use Navgen for all newly created spaces
        settings->writeString( "navmeshGenerator", "navgen" );
        // write terrain settings
        DataSectionPtr terrainSettings = settings->openSection( "terrain", true );

        // Create a terrainsettings object with default settings
        pTerrainSettings_ = new Terrain::TerrainSettings;
        pTerrainSettings_->initDefaults( chunkSize_,
            Terrain::DEFAULT_BLOCK_SIZE );

        pTerrainSettings_->version( terrainInfo_.version_ );
        pTerrainSettings_->heightMapEditorSize( terrainInfo_.heightMapEditorSize_ );
        pTerrainSettings_->heightMapSize( terrainInfo_.heightMapSize_ );
        pTerrainSettings_->normalMapSize( terrainInfo_.normalMapSize_ );
        pTerrainSettings_->holeMapSize( terrainInfo_.holeMapSize_ );
        pTerrainSettings_->shadowMapSize( terrainInfo_.shadowMapSize_ );
        pTerrainSettings_->blendMapSize( terrainInfo_.blendMapSize_ );
        pTerrainSettings_->save( terrainSettings );

        settings->save();

        return true;
    }


    bool createFloraSettings()
    {
        BW_GUARD;

        DataSectionPtr settings = BWResource::openSection( "system/data/flora.xml" );

        if (!settings)
            return false;       

        settings->writeInt( "vb_size", floraVBSize_ );
        settings->writeInt( "texture_width", floraTextureWidth_ );
        settings->writeInt( "texture_height", floraTextureHeight_ );

        settings->save( BWResource::dissolveFilename( spaceName_ )+ "/" + SPACE_FLORA_SETTING_FILE_NAME );

        return true;
    }


    bool createTemplateTerrainBlock()
    {
        BW_GUARD;

        // get relevant space settings
        DataSectionPtr settings = BWResource::openSection(
            BWResource::dissolveFilename( spaceName_ ) + "/" + SPACE_SETTING_FILE_NAME );
        DataSectionPtr terrainSettingsDS = NULL;
        if ( settings != NULL )
            terrainSettingsDS = settings->openSection( "terrain" );

        // init the terrain settings so the terrain is set to the appropriate
        // version when creating/loading the block.
        Terrain::TerrainSettingsPtr pTerrainSettings = new Terrain::TerrainSettings();
        pTerrainSettings->init( chunkSize_, terrainSettingsDS,
            Terrain::DEFAULT_BLOCK_SIZE );

        uint32 blendSize = pTerrainSettings->blendMapSize();

        // setup the template cdata file
        BW::string templateTname;
        bool deleteTemplate = false;
        {
            // create a new terrain2 block
            templateTname = BWResource::resolveFilename( s_spaceDlgBlankCDataFname );

            Terrain::EditorBaseTerrainBlockPtr block = 
                TerrainUtils::createDefaultTerrain2Block( pTerrainSettings,
                terrainSettingsDS );
            if (!block.hasObject())
            {
                return false;
            }

            // save it
            std::remove( templateTname.c_str() );
            
            DataSectionPtr cdata = new BinSection( s_spaceDlgBlankCDataFname, NULL );
            cdata=cdata->convertToZip( s_spaceDlgBlankCDataFname );
            if ( cdata == NULL )
                return false;

            DataSectionPtr pTerrain = cdata->openSection( block->dataSectionName(), true );
            pTerrain->setParent(cdata);
            pTerrain = pTerrain->convertToZip();
            bool savedOK = (pTerrain && block->save( pTerrain ));
            pTerrain->setParent( NULL );
            if (!savedOK)
            {
                return false;
            }

            if ( !cdata->save( s_spaceDlgBlankCDataFname ) )
            {
                return false;
            }

            deleteTemplate = true;
        }

        if ( !defaultTexture_.empty() )
        {
            // Create temporary blank terrain file with the default texture
            BW::string fname = templateTname;
            BW::string terrainName = fname + "/terrain2";
            
            // load the blank block, insert a texture layer with the default,
            // texture, and save it.
            Terrain::EditorBaseTerrainBlockPtr block =
                static_cast<Terrain::EditorBaseTerrainBlock*>(
                    Terrain::EditorBaseTerrainBlock::loadBlock(
                        terrainName,
                        Matrix::identity,
                        Vector3( 0, 0, 0 ),
                        pTerrainSettings,
                        NULL ).getObject() );
            if ( block )
            {
                // remove all layers
                size_t numLayers = block->numberTextureLayers();
                for (size_t i = 0; i < numLayers; ++i)
                    block->removeTextureLayer( 0 );

                // insert a clean layer with the default texture
                int32 idx = static_cast<int32>(block->insertTextureLayer( blendSize, blendSize ));
                if ( idx == -1 )
                {
                    ERROR_MSG( "Couldn't create default texture for new space.\n" );
                }
                else
                {
                    Terrain::TerrainTextureLayer& layer = block->textureLayer( idx );
                    layer.textureName( defaultTexture_ );
                    {
                        // set the blends to full white
                        Terrain::TerrainTextureLayerHolder holder( &layer, false );
                        layer.image().fill(
                            std::numeric_limits<Terrain::TerrainTextureLayer::PixelType>::max() );
                    }
                    // update and save
                    block->rebuildCombinedLayers();
                    if (!block->rebuildLodTexture( Matrix::identity ))
                    {
                        return false;
                    }
                    DataSectionPtr cDataDS = BWResource::openSection( fname );
                    if ( cDataDS )
                    {
                        DataSectionPtr blockDS = cDataDS->openSection( block->dataSectionName() );
                        if ( blockDS )
                        {
                            if ( block->save( blockDS ) )
                            {
                                cDataDS->save();
                                // set 'templateTname' to the temp file instead of the blank
                                // 's_ndBlankCDataFname' file.
                                templateTname = fname;
                                deleteTemplate = true;
                            }
                        }
                    }
                }
            }
        }

        //Copy the cData
        auto wtemplateTname = bw_utf8tow( templateTname );
        HANDLE file = CreateFile( wtemplateTname.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL );
        if( file != INVALID_HANDLE_VALUE )
        {
            DWORD size = GetFileSize( file, NULL );
            terrainSize_ = size;
            wchar_t diskRoot[] = { wtemplateTname[0], L':', L'\0' };
            DWORD dummy, bytesPerSector;
            GetDiskFreeSpace( diskRoot, &dummy, &bytesPerSector, &dummy, &dummy );
            size = ( size + bytesPerSector - 1 ) / bytesPerSector * bytesPerSector;
            if (terrainCache_ != NULL)
            {
                VirtualFree(terrainCache_, 0, MEM_RELEASE);
                terrainCache_ = NULL;
            }
            terrainCache_ = VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_READWRITE );
            ReadFile( file, terrainCache_, size, &dummy, NULL );
            CloseHandle( file );
        }
        if (memoryCache_ != NULL)
        {
            VirtualFree(memoryCache_, 0, MEM_RELEASE);
            memoryCache_ = NULL;
        }
        memoryCache_ = (char*)VirtualAlloc( NULL, memoryCacheSize_ /*big enough*/, MEM_COMMIT, PAGE_READWRITE );
        
        // delete temp file
        if ( deleteTemplate )
            std::remove( templateTname.c_str() );
        return true;
    }

    BW::string chunkName( int x, int z ) const
    {
        BW_GUARD;

        return ChunkFormat::outsideChunkIdentifier( x,z );
    }

    void createTerrain( int x, int z, char*& buffer )
    {
        BW_GUARD;

        BW::string name = chunkName( x, z );
        BW::string cDataName = name.substr( 0, name.size() - 1 )
            + "o.cdata";
        BW::string terrainName = cDataName + "/terrain2";

        BW::string chunkFile;
        chunkFile =  "<root>\r\n";
        chunkFile +=    "\t<terrain>\r\n";
        chunkFile +=        "\t\t<resource>\t"+terrainName+"\t</resource>\r\n";
        chunkFile +=    "\t</terrain>\r\n";
        chunkFile += "</root>\r\n";

        buffer += bw_snprintf( buffer, memoryCacheSize_, chunkFile.c_str() );

        {
            MF_ASSERT(!s_spaceDlgBlankCDataFname.value().empty());
        }

        //Copy the cData
        BW::string fname = spaceName_ + "/" + cDataName;
        auto wfname = bw_utf8tow( fname );
        HANDLE file = CreateFile( wfname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL );
        if( file != INVALID_HANDLE_VALUE )
        {
            DWORD dummy;
            static DWORD bytesPerSector = 0;
            if( bytesPerSector == 0 )
            {
                wchar_t diskRoot[] = { wfname[0], L':', L'\0' };
                GetDiskFreeSpace( diskRoot, &dummy, &bytesPerSector, &dummy, &dummy );
            }
            DWORD size = ( terrainSize_ + bytesPerSector - 1 ) / bytesPerSector * bytesPerSector;
            WriteFile( file, terrainCache_, size, &dummy, NULL );
            CloseHandle( file );
            file = CreateFile( bw_utf8tow( fname ).c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, 0, NULL );
            SetFilePointer( file, terrainSize_, NULL, FILE_BEGIN );
            SetEndOfFile( file );
            CloseHandle( file );
        }

        {
            // terrain2 needs the texture lod to be calculated here.
            Moo::rc().effectVisualContext().initConstants();
            BW::string terrainName = fname + "/terrain2";
            SmartPointer<Terrain::EditorTerrainBlock2> block =
                static_cast<Terrain::EditorTerrainBlock2*>(
                    Terrain::EditorBaseTerrainBlock::loadBlock(
                        terrainName,
                        Matrix::identity,
                        Vector3( 0, 0, 0 ),
                        pTerrainSettings_,
                        NULL ).getObject() );
            if ( block )
            {
                Matrix m( Matrix::identity );
                m.translation( Vector3( chunkSize_ * x, 0.0f, chunkSize_ * z ) );
                if ( block->rebuildLodTexture( m ) )
                {
                    DataSectionPtr cdataDS = BWResource::openSection( fname );
                    if ( cdataDS )
                    {
                        DataSectionPtr terrainDS = cdataDS->openSection( block->dataSectionName() );
                        if ( terrainDS && block->saveLodTexture( terrainDS ) )
                        {
                            ChunkCleanFlags flags( cdataDS );

                            flags.terrainLOD_ = 1;
                            flags.save();
                            cdataDS->save();
                        }
                        else
                        {
                            ERROR_MSG( "Couldn't save terrain for %s\n", fname.c_str() );
                        }
                    }
                    else
                    {
                        ERROR_MSG( "Couldn't open terrain block %s .cdata file\n", fname.c_str() );
                    }
                }
                else
                {
                    ERROR_MSG( "Couldn't create terrain lod texture for %s\n", fname.c_str() );
                }
            }
            else
            {
                ERROR_MSG( "Couldn't load terrain block %s to create its terrain lod texture\n", fname.c_str() );
            }
        }
    }

    bool createChunk( int x, int z )
    {
        BW_GUARD;

        if( !terrainCache_ )
        {
            errorMessage_ = "Cannot find template cdata file.";
            return false;
        }

        BW::string name = chunkName( x, z ) + ".chunk";

        if( name.rfind( '/' ) != name.npos  )
        {
            BW::string path = name.substr( 0, name.rfind( '/' ) + 1 );
            if( subDir_.find( path ) == subDir_.end() )
            {
                BWResource::ensurePathExists( spaceName_ + "/" + path );
                subDir_.insert( path );
            }
        }

        char* buffer = memoryCache_;
        createTerrain( x, z, buffer );

        BW::wstring fname = bw_utf8tow( spaceName_ + "/" + name );
        HANDLE file = CreateFile( fname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL );
        if( file != INVALID_HANDLE_VALUE )
        {
            DWORD dummy;
            static DWORD bytesPerSector = 0;
            if( bytesPerSector == 0 )
            {
                wchar_t diskRoot[] = { fname[0], L':', L'\0' };
                GetDiskFreeSpace( diskRoot, &dummy, &bytesPerSector, &dummy, &dummy );
            }
            size_t size = ( buffer - memoryCache_ + bytesPerSector - 1 ) / bytesPerSector * bytesPerSector;
            WriteFile( file, memoryCache_, static_cast<DWORD>(size), &dummy, NULL );
            CloseHandle( file );
            file = CreateFile( fname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, 0, NULL );
            size_t distanceToMove = buffer - memoryCache_;
            MF_ASSERT( distanceToMove <= LONG_MAX );
            SetFilePointer( file, ( LONG ) distanceToMove, NULL, FILE_BEGIN );
            SetEndOfFile( file );
            CloseHandle( file );
        }

        return true;
    }

    bool createChunks( bool withPurge )
    {
        BW_GUARD;

        // Create a copy of the default texture if possible.  This keeps the
        // texture in the TextureManager's cache throughout the chunk
        // creation. 
        Moo::BaseTexturePtr pDefaultTexture = 
            Moo::TextureManager::instance()->get( defaultTexture_ );

        subDir_.clear();
        bool good = true;

        int i = 0;
        
        int start = GetTickCount();

        bool keepRunning = true;
        
        for (int x = minX_; x <= maxX_ && good; ++x)
        {
            int loop = GetTickCount();
            
            for (int z = minZ_; z <= maxZ_ && good; ++z)
            {
                good &= createChunk( x, z );
                progress_->step();
                if ((withPurge) && (++i > 150))
                {
                    BWResource::instance().purgeAll();
                    i = 0;
                }
                if ( progress_->isCancelled() )
                {
                    good = false;
                }
            }
            
            int last = GetTickCount();

            float average = float( (last - start) / (x - minX_ + 1) );
        }

        return good;
    }

    BW::string spaceName_;
    float chunkSize_;
    BW::string errorMessage_;
    int minX_, maxX_, minZ_, maxZ_, minY_, maxY_;
    TerrainInfo terrainInfo_;
    Terrain::TerrainSettingsPtr pTerrainSettings_;
    BW::string defaultTexture_;
    LPVOID terrainCache_;
    DWORD terrainSize_;
    ProgressBarTask* progress_;

    char* memoryCache_;
    const uint32 memoryCacheSize_;
    BW::set<BW::string> subDir_;

    int floraVBSize_;
    int floraTextureWidth_;
    int floraTextureHeight_;
};

void NewSpaceDlg::OnBnClickedOk()
{
    BW_GUARD;

    CWaitCursor wait;

    if ( !validateDefaultTexture() )
    {
        return;
    }

    // Create the space!
    CString s, p, z, w, h, ny, my, defaultTexture;
    space_.GetWindowText( s );
    spacePath_.GetWindowText( p );
    chunkSize_.GetWindowText( z );
    width_.GetWindowText( w );
    height_.GetWindowText( h );
    defaultTexture_.GetWindowText( defaultTexture );
    ny.Format( L"%d", MIN_CHUNK_HEIGHT );
    my.Format( L"%d", MAX_CHUNK_HEIGHT );

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
    int minHeight = _wtoi( ny.GetString() );
    int maxHeight = _wtoi( my.GetString() );

    int floraVBSize = floraVB_.GetIntegerValue();
    int floraTextureWidth = floraTextureWidth_.GetIntegerValue();
    int floraTextureHeight = floraTextureHeight_.GetIntegerValue();

    if ( minHeight > maxHeight )
    {
        int h = maxHeight;
        maxHeight = minHeight;
        minHeight = h;
    }

    SpaceMaker::TerrainInfo terrainInfo;
    {
        CString buffer;
        terrainInfo.version_ = Terrain::TerrainSettings::TERRAIN2_VERSION;
        heightMapSize_.GetLBText( heightMapSize_.GetCurSel(), buffer );
        terrainInfo.heightMapSize_  = _wtoi( buffer.GetString() );
        heightMapEditorSize_.GetLBText( heightMapEditorSize_.GetCurSel(), buffer );
        terrainInfo.heightMapEditorSize_    = _wtoi( buffer.GetString() );
        normalMapSize_.GetLBText( normalMapSize_.GetCurSel(), buffer );
        terrainInfo.normalMapSize_  = _wtoi( buffer.GetString() );
        holeMapSize_.GetLBText( holeMapSize_.GetCurSel(), buffer );
        terrainInfo.holeMapSize_    = _wtoi( buffer.GetString() );
        shadowMapSize_.GetLBText( shadowMapSize_.GetCurSel(), buffer );
        terrainInfo.shadowMapSize_  = _wtoi( buffer.GetString() );
        blendMapSize_.GetLBText( blendMapSize_.GetCurSel(), buffer );
        terrainInfo.blendMapSize_   = _wtoi( buffer.GetString() );

        // store the defaults for next time
        Options::setOptionInt( "terrain2/defaults/heightMapSize", terrainInfo.heightMapSize_ );
        Options::setOptionInt( "terrain2/defaults/heightMapEditorSize", terrainInfo.heightMapEditorSize_ );
        Options::setOptionInt( "terrain2/defaults/normalMapSize", terrainInfo.normalMapSize_ );
        Options::setOptionInt( "terrain2/defaults/holeMapSize", terrainInfo.holeMapSize_ );
        Options::setOptionInt( "terrain2/defaults/shadowMapSize", terrainInfo.shadowMapSize_ );
        Options::setOptionInt( "terrain2/defaults/blendMapSize", terrainInfo.blendMapSize_ );
    }

    SpaceMaker maker( bw_wtoutf8( spaceName ),
        chunkSize, width, height, minHeight, maxHeight,
        floraVBSize, floraTextureWidth, floraTextureHeight,
        terrainInfo,
        bw_wtoutf8( defaultTexture.GetString() ) );
    buttonCreate_.EnableWindow( FALSE );
    buttonCancel_.EnableWindow( FALSE );
    busy_ = true;
    bool result = maker.create();
    busy_ = false;
    buttonCreate_.EnableWindow( TRUE );
    buttonCancel_.EnableWindow( TRUE );
    if ( !result )
    {
        MessageBox( bw_utf8tow( maker.errorMessage()).c_str(), L"Unable to create space", MB_ICONERROR );
    }
    else
    {
        createdSpace_ = bw_utf8tow( BWResource::dissolveFilename( bw_wtoutf8( spaceName ) ) ).c_str();

        if ( !textureImage_.image().isEmpty() )
        {
            // save last used default texture
            CString dt;
            defaultTexture_.GetWindowText( dt );
            BW::string dissolvedFileName = BWResource::dissolveFilename( bw_wtoutf8( dt.GetString( )) );
            Options::setOptionString( LAST_DEFAULT_TEXTURE_TAG, dissolvedFileName );
        }
        OnOK();
    }
}

void NewSpaceDlg::validateFile( CEdit& ctrl, bool isPath )
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

bool NewSpaceDlg::validateDefaultTexture()
{
    BW_GUARD;

    if ( defaultTexture_.GetWindowTextLength() > 0 && textureImage_.image().isEmpty() )
    {
        CString dt;
        defaultTexture_.GetWindowText( dt );
        BW::wstring resolvedFName = bw_utf8tow( BWResource::resolveFilename( bw_wtoutf8( dt.GetString( ) ) ) );
        BW::wstring msg = L"The Default Terrain Texture '" + resolvedFName +
            L"' cannot be used because it's missing or it's not in a valid resource path.\n" +
            L"To create the space, please enter a valid texture file name, or clear the Default Terrain Texture field.";
        MessageBox( msg.c_str(), L"Invalid Default Terrain Texture", MB_ICONERROR );
        return false;
    }

    return true;
}

void NewSpaceDlg::formatChunkToKms( CString& str, const CString& chunkSizeStr )
{
    BW_GUARD;

    int val = _wtoi( str.GetString() );
    float chunkSize = (float)_wtoi( chunkSizeStr.GetString() );

    if ( val * chunkSize >= 1000 )
        str.Format( NEWSPACEDLG_KM_FORMAT, val * chunkSize / 1000.0f );
    else
        str.Format( NEWSPACEDLG_M_FORMAT, (int) ( val * chunkSize ) );
}

void NewSpaceDlg::formatPerChunkToPerMetre( CString& str )
{
    BW_GUARD;

    int val = _wtoi( str.GetString() );
    float chunkSize = currentChunkSize();

    if ( val / chunkSize > 1.f )
        str.Format( L"%.0f", val / chunkSize );
    else
        str.Format( L"%.2f", val / chunkSize );
}

float NewSpaceDlg::currentChunkSize() {
    BW_GUARD;

    CString chunkSizeStr;
    chunkSize_.GetWindowText( chunkSizeStr );
    return (float)_wtoi( chunkSizeStr.GetString() );
}

void NewSpaceDlg::formatPerChunkToMetresPer( CString& str )
{
    BW_GUARD;

    int val = _wtoi( str.GetString() );
    float chunkSize = currentChunkSize();

    str.Format( L"%.2f", chunkSize / val );
}

void NewSpaceDlg::OnEnChangeSpace()
{
    BW_GUARD;

    validateFile( space_, false );
}

void NewSpaceDlg::OnEnChangeSpacePath()
{
    BW_GUARD;

    validateFile( spacePath_, true );
}

void NewSpaceDlg::OnBnClickedNewspaceBrowse()
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

void NewSpaceDlg::OnEnTextureChange()
{
    BW_GUARD;

    validateFile( defaultTexture_, true );
    CString dt;
    defaultTexture_.GetWindowText( dt );

    textureImage_.image().load( BWResource::resolveFilename( bw_wtoutf8( dt.GetString() ) ) );
    textureImage_.Invalidate();
    textureImage_.UpdateWindow();
}

void NewSpaceDlg::OnBnTextureBrowse()
{
    BW_GUARD;

    static wchar_t szFilter[] = 
        L"Texture Files (*.tga;*.bmp;*.dds)|*.tga;*.bmp;*.dds|"
        L"All Files (*.*)|*.*||";

    BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
        ( BWFileDialog::FD_HIDEREADONLY | BWFileDialog::FD_OVERWRITEPROMPT );
    BWFileDialog fileDialog( true, NULL, NULL, flags, szFilter, GetParent() );

    CString dt;
    defaultTexture_.GetWindowText( dt );
    // lastDefTex must be declared here to keep the string alive during DoModal
    BW::wstring lastDefTex;
    if ( !dt.IsEmpty() )
    {
        lastDefTex = bw_utf8tow( 
            BWResource::getFilePath(
                BWResource::resolveFilename( bw_wtoutf8( dt.GetString() ) ) ) );
        std::replace( lastDefTex.begin(), lastDefTex.end(), L'/', L'\\' );
        fileDialog.initialDir( lastDefTex.c_str() );
    }

    if (fileDialog.showDialog())
    {
        bool failed = true;
        BW::string dissolvedFileName = BWResource::dissolveFilename( bw_wtoutf8( fileDialog.getFileName() ) );
        if ( dissolvedFileName[1] != ':' && dissolvedFileName.size() )
        {
            defaultTexture_.SetWindowText( bw_utf8tow( dissolvedFileName ).c_str() );
            OnEnTextureChange();
            failed = false;
        }
        
        if ( textureImage_.image().isEmpty() || failed )
        {
            BW::string msg = "The file '" + dissolvedFileName +
                "' cannot be used as a texture because it's missing or is not in the resource paths.";
            MessageBox( bw_utf8tow( msg ).c_str(), L"Browse for Default Terrain Texture", MB_ICONERROR );
        }
    }
}

void NewSpaceDlg::OnEnChangeChunkSize()
{
    BW_GUARD;

    CString width;
    CString height;
    CString chunkSize;

    width_.GetWindowText( width );
    height_.GetWindowText( height );
    chunkSize_.GetWindowText( chunkSize );

    formatChunkToKms( width, chunkSize );
    formatChunkToKms( height, chunkSize );

    widthKms_.SetWindowText( width );
    heightKms_.SetWindowText( height );

    OnCbnSelChangeHeightmapSize();
    OnCbnSelChangeNormalmapSize();
    OnCbnSelChangeHolemapSize();
    OnCbnSelChangeBlendmapSize();
    OnCbnSelChangeShadowmapSize();
}

void NewSpaceDlg::OnEnChangeWidth()
{
    BW_GUARD;

    CString str;
    CString chunkSize;

    width_.GetWindowText( str );
    chunkSize_.GetWindowText( chunkSize );

    formatChunkToKms( str, chunkSize );

    widthKms_.SetWindowText( str );
}

void NewSpaceDlg::OnEnChangeHeight()
{
    BW_GUARD;

    CString str;
    CString chunkSize;

    height_.GetWindowText( str );
    chunkSize_.GetWindowText( chunkSize );

    formatChunkToKms( str, chunkSize );

    heightKms_.SetWindowText( str );
}

BOOL NewSpaceDlg::OnSetCursor( CWnd* wnd, UINT nHitTest, UINT msg )
{
    if (busy_)
    {
        ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

        return TRUE;
    }

    return CDialog::OnSetCursor( wnd, nHitTest, msg );
}

void NewSpaceDlg::OnCbnSelchangeHeightmapEditorSize()
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
    }
}

void NewSpaceDlg::OnCbnSelChangeHeightmapSize()
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
    }

    formatPerChunkToMetresPer( heightMapString );
    heightMapRes_.SetWindowText( heightMapString );
}

void NewSpaceDlg::OnCbnSelChangeNormalmapSize()
{
    BW_GUARD;

    CString str;

    normalMapSize_.GetWindowText( str );
    formatPerChunkToMetresPer( str );
    normalMapRes_.SetWindowText( str );
}

void NewSpaceDlg::OnCbnSelChangeHolemapSize()
{
    BW_GUARD;

    CString str;

    holeMapSize_.GetWindowText( str );
    formatPerChunkToMetresPer( str );
    holeMapRes_.SetWindowText( str );
}

void NewSpaceDlg::OnCbnSelChangeBlendmapSize()
{
    BW_GUARD;

    CString str;

    blendMapSize_.GetWindowText( str );
    formatPerChunkToPerMetre( str );
    blendMapRes_.SetWindowText( str );
}

void NewSpaceDlg::OnCbnSelChangeShadowmapSize()
{
    BW_GUARD;

    CString str;

    shadowMapSize_.GetWindowText( str );
    formatPerChunkToPerMetre( str );
    shadowMapRes_.SetWindowText( str );
}
BW_END_NAMESPACE
