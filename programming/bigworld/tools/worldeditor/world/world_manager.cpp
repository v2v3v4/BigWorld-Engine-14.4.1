/*~ module WorldEditor
 *  @components{ worldeditor }
 *
 *  The WorldEditor Module is a Python module that provides an interface to
 *  the various information about the world items in WorldEditor.
 *  It also provides functionality to configure the WorldEditor GUI, replicate 
 *  menu item actions, capture user interaction and provides an interface
 *  to the 'bwlockd' (lock server).
 */

#include "pch.hpp"

#include "world_manager.hpp"
#include "world_editor_romp_harness.hpp"

#include "worldeditor/import/space_settings.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/editor_chunk_link_manager.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/items/editor_chunk_tree.hpp"
#include "worldeditor/world/items/editor_chunk_portal.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "worldeditor/world/items/editor_chunk_water.hpp"
#include "worldeditor/world/items/editor_chunk_flare.hpp"
#include "worldeditor/world/items/editor_chunk_meta_data.hpp"
#include "worldeditor/world/items/editor_chunk_marker_cluster.hpp"
#include "worldeditor/world/items/editor_chunk_particle_system.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_binding.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"
#include "worldeditor/world/we_chunk_saver.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/world_editor_doc.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/gui/pages/chunk_watcher.hpp"
#include "worldeditor/gui/pages/page_chunk_texture.hpp"
#include "worldeditor/gui/pages/page_properties.hpp"
#include "worldeditor/gui/pages/page_terrain_import.hpp"
#include "worldeditor/gui/pages/page_terrain_texture.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/dialogs/low_memory_dlg.hpp"
#include "worldeditor/gui/dialogs/wait_dialog.hpp"
#include "worldeditor/gui/dialogs/new_space_dlg.hpp"
#include "worldeditor/gui/dialogs/edit_space_dlg.hpp"
#include "worldeditor/gui/dialogs/recreate_space_dlg.hpp"
#include "worldeditor/gui/dialogs/splash_dialog.hpp"
#include "worldeditor/gui/dialogs/undo_warn_dlg.hpp"
#include "worldeditor/gui/dialogs/expand_space_dlg.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/collisions/collision_callbacks.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/popup_menu_helper.hpp"
#include "worldeditor/misc/chunk_row_cache.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "worldeditor/misc/light_container_debugger.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/misc/fences_tool_view.hpp"
#include "worldeditor/height/height_map.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/editor/item_frustum_locator.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/project/chunk_photographer.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/project/space_map.hpp"
#include "worldeditor/project/nearby_chunk_loader.hpp"
#include "world/editor_chunk_thumbnail_cache.hpp"

#include "asset_pipeline/asset_lock.hpp"

#include "appmgr/app.hpp"
#include "appmgr/application_input.hpp"
#include "appmgr/commentary.hpp"
#include "appmgr/options.hpp"

#include "chunk/chunk_bsp_holder.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_item_tree_node.hpp"
#include "chunk/chunk_loader.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_vlo.hpp"
#if UMBRA_ENABLE
#include "chunk/chunk_umbra.hpp"
#endif
#include "chunk/clean_chunk_list.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"
#include "cstdmf/cstdmf_init.hpp"

#include "space/global_embodiments.hpp"

#include "common/compile_time.hpp"
#include "common/dxenum.hpp"
#include "common/material_properties.hpp"
#include "common/mouse_look_camera.hpp"
#include "common/page_messages.hpp"
#include "common/resource_loader.hpp"
#include "common/space_mgr.hpp"
#include "common/lighting_influence.hpp"
#include "common/utilities.hpp"

#include "cstdmf/memory_load.hpp"

#include "duplo/chunk_embodiment.hpp"

#if FMOD_SUPPORT
#include "fmodsound/sound_manager.hpp"
#endif // FMOD_SUPPORT

#include "gizmo/gizmo_manager.hpp"
#include "gizmo/item_view.hpp"
#include "gizmo/tool_manager.hpp"

#include "guimanager/gui_functor_option.hpp"
#include "guimanager/gui_input_handler.hpp"
#include "guimanager/gui_manager.hpp"

#include "moo/draw_context.hpp"

#include "physics2/material_kinds.hpp"

#include "particle/actions/particle_system_action.hpp"

#include "pyscript/py_data_section.hpp"
#include "pyscript/py_callback.hpp"
#include "pyscript/personality.hpp"

#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/data_section_cache.hpp"
#include "resmgr/data_section_census.hpp"

#include "romp/console_manager.hpp"
#include "moo/debug_geometry.hpp"
#include "romp/engine_statistics.hpp"
#include "romp/flora.hpp"
#include "romp/flora.hpp"
#include "romp/fog_controller.hpp"
#include "moo/geometrics.hpp"
#include "romp/chunk_omni_light_embodiment.hpp"
#include "romp/chunk_spot_light_embodiment.hpp"
#include "moo/texture_renderer.hpp"
#include "romp/time_of_day.hpp"
#include "romp/water.hpp"
#include "romp/water_scene_renderer.hpp"

#include "scene/scene.hpp"
#include "scene/change_scene_view.hpp"
#include "space/client_space.hpp"
#include "space/space_manager.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/global_embodiments.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "terrain/manager.hpp"
#include "terrain/base_terrain_block.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "terrain/editor_chunk_terrain_projector.hpp"
#include "terrain/horizon_shadow_map.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_texture_layer.hpp"

#include "common/editor_chunk_terrain_cache.hpp"
#include "common/editor_chunk_terrain_lod_cache.hpp"
#include "editor_chunk_navmesh_cache.hpp"

#include "math/sma.hpp"

#include "cstdmf/message_box.hpp" 
#include "cstdmf/restart.hpp"
#include "cstdmf/bwversion.hpp"
#include "cstdmf/log_msg.hpp"
#include "cstdmf/log_meta_data.hpp"

#include <algorithm>
#include <fstream>
#include <time.h>
#include <sstream>
#include <psapi.h>

#include "moo/effect_material.hpp"
#include "common/material_editor.hpp"
#include "cstdmf/bw_stack_container.hpp"

#include "moo/renderer.hpp"
#include "moo/deferred_decals_manager.hpp"


#pragma comment( lib, "psapi.lib" )

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

static DogWatch s_AmortiseChunkItemDelete( "chnk_item_del" );
static DogWatch s_linkManager( "link_manager" );
static DogWatch s_linkerManager( "linker_manager" );
static DogWatch s_chunkTick( "chunk_tick" );
static DogWatch s_chunkDraw( "chunk_draw" );
static DogWatch s_umbraDraw( "umbra_draw" );
static DogWatch s_terrainDraw( "terrain_draw" );
static DogWatch s_rompDraw( "romp_draw" );
static DogWatch s_drawSorted( "draw_sorted" );
static DogWatch s_render( "render" );
static DogWatch s_update( "update" );
static DogWatch s_detailTick( "detail_tick" );
static DogWatch s_detailDraw( "detail_draw" );

//for ChunkManager
BW::string g_specialConsoleString;

static AutoConfigString s_terrainSelectionFx( "selectionfx/terrain" );
static AutoConfigString s_blankCDataFname( "dummy/cData" );
static AutoConfigString s_blankTerrainTexture(
    "system/emptyTerrainTextureBmp", "helpers/maps/aid_builder.tga" );
static AutoConfigString s_dxEnumPath( "system/dxenum" );

// Preload models

static AutoConfigString s_notFoundModel( "system/notFoundModel" );
static AutoConfigString s_entityModel( "editor/entityModel", "resources/models/entity.model" );
static AutoConfigString s_legacyEntityModel( "editor/entityModelLegacy", "helpers/props/standin.model" );


BW_INIT_SINGLETON_STORAGE(WorldManager);

/**
 *  This class is a workaround to the fact that the VisualManager and the
 *  PrimitiveManager are not entirely thread safe. We simply preload the models
 *  that can cause problems (the ones that can potentially get loaded in both
 *  the main thread and the loading thread in the editor).
 */
class WEPreloader : public Singleton< WEPreloader >
{
public:
    WEPreloader()
    {
        notFoundModel_ = Model::get( s_notFoundModel.value() );
        entityModel_ = Model::get( s_entityModel.value() );
        legacyEntityModel_ = Model::get( s_legacyEntityModel.value() );
        udoModel_ = Model::get( "resources/models/user_data_object.model" );
        waterModel_ = Model::get( "resources/models/water.model" );
    }

private:
    ModelPtr notFoundModel_;
    ModelPtr entityModel_;
    ModelPtr legacyEntityModel_;
    ModelPtr udoModel_;
    ModelPtr waterModel_;
};
BW_SINGLETON_STORAGE( WEPreloader );


class WEChunkLoader : public UnsavedChunks::IChunkLoader 
{ 
    GeometryMapping* mapping_; 

public: 
    WEChunkLoader( GeometryMapping* mapping ) : mapping_( mapping ) 
    {} 

    virtual void load( const BW::string& chunk ) 
    { 
        ChunkManager::instance().loadChunkNow( chunk, mapping_ ); 

        while (ChunkManager::instance().checkLoadingChunks()) 
        { 
            ChunkManager::instance().processPendingChunks(); 
        } 
    } 

    virtual Chunk* find( const BW::string& chunk ) 
    { 
        return ChunkManager::instance().findChunkByName( 
            chunk, mapping_, false ); 
    } 
};

//--------------------------------
//CHUNKIN EXTERNS
// reference the ChunkInhabitants that we want to be able to load.
extern int ChunkModel_token;
extern int ChunkLight_token;
extern int ChunkTerrain_token;
extern int ChunkFlare_token;
extern int ChunkWater_token;
extern int ChunkAttachment_token;
extern int EditorChunkOverlapper_token;
extern int ChunkParticles_token;
extern int ChunkTree_token;
extern int PyResourceRefs_token;
extern int Servo_token;
extern int InvViewMatrixProvider_token;
extern int EditorChunkNavmeshCache_token;
extern int ChunkModelVLO_token;
static int s_chunkTokenSet = ChunkModel_token | ChunkLight_token |
    ChunkTerrain_token | ChunkFlare_token | ChunkWater_token |
    EditorChunkOverlapper_token | ChunkParticles_token | ChunkAttachment_token |
    ChunkTree_token | PyResourceRefs_token | Servo_token |
    InvViewMatrixProvider_token | EditorChunkNavmeshCache_token |
    ChunkModelVLO_token;

extern int ScriptedModule_token;
static int s_moduleTokenSet = ScriptedModule_token;

extern int genprop_gizmoviews_token;
static int giz = genprop_gizmoviews_token;

extern int Math_token;
extern int PyScript_token;
extern int GUI_token;
extern int ResMgr_token;
static int s_moduleTokens =
    Math_token |
    PyScript_token |
    GUI_token |
    ResMgr_token;

extern int PyGraphicsSetting_token;
static int pyTokenSet = PyGraphicsSetting_token;

// WE specific tool functor tokens
extern int PipeFunctor_token;
extern int ScriptedToolFunctor_token;
extern int TeeFunctor_token;
extern int TerrainHeightFilterFunctor_token;
extern int TerrainHoleFunctor_token;
extern int TerrainSetHeightFunctor_token;
extern int TerrainTextureFunctor_token;

static int s_toolFunctorTokenSet = PipeFunctor_token |
ScriptedToolFunctor_token |
TeeFunctor_token |
TerrainHeightFilterFunctor_token |
TerrainHoleFunctor_token |
TerrainSetHeightFunctor_token |
TerrainTextureFunctor_token;

extern int TextureToolView_token;
static int s_toolViewTokenSet = TextureToolView_token;

namespace PostProcessing
{
    extern int tokenSet;
    static int ppTokenSet = tokenSet;
}

namespace Terrain
{
    class Manager;
}


static Moo::EffectMaterialPtr s_selectionMaterial = NULL;
static bool s_selectionMaterialOk = false;

bool ScopedReentryAssert::entered_ = false;

// static members
WorldEditorDebugMessageCallback WorldManager::s_debugMessageCallback_;
WorldEditorCriticalMessageCallback WorldManager::s_criticalMessageCallback_;
bool WorldManager::s_criticalMessageOccurred_ = false;
double WorldManager::s_totalTime_ = 0.0;
ReadWriteLock WorldManager::s_chunkProcessorThreadsTimeSliceLock_;
volatile bool WorldManager::s_allowChunkProcessorThreads_ = true;
uint64 WorldManager::s_presentTimeSliceTimeStamp_ = 0;
uint32 WorldManager::s_minimumAllowTimeSliceMS_ = 4;
uint32 WorldManager::s_timeSlicePauseWarnThresholdMS_ = 0;

ScriptObject WorldManager::readyForScreenshotCallback_;

/**
 *  Number of background chunk processing threads to use.
 *  This is used for background processing while the user is editing, not while
 *  in processing mode.
 *  This number has been found through testing to work well with time slicing.
 */
const int WorldManager::NUM_BACKGROUND_CHUNK_PROCESSING_THREADS = 1;

class WorldManager::InitFailureCleanup
{
public:
    InitFailureCleanup( bool & rInited, WorldManager & worldManager ) :
        rInited_( rInited ),
        worldManager_( worldManager )
    {
    }

    ~InitFailureCleanup()
    {
        if (!rInited_)
        {
            delete AmortiseChunkItemDelete::pInstance();
            delete ChunkPhotographer::pInstance();
            delete DXEnum::pInstance();
            EditorUserDataObjectType::shutdown();
            EditorEntityType::shutdown();
            MaterialKinds::fini();
            worldManager_.pTerrainManager_.reset();
            worldManager_.pHeightMap_.reset();
            ChunkManager::instance().fini();
            SpaceManager::instance().fini();
            Moo::rc().releaseDevice();
        }
    }
private:
    bool & rInited_;
    WorldManager & worldManager_;
};

namespace 
{
    static const BW::string TERRAIN_LOD = "TerrainLod";
    static const int s_delayedFrameCallbackDelay = 5;

    /**
     *  This helper function returns the list of all chunk items currently
     *  loaded.
     */
    void allChunkItems( ChunkItemRevealer::ChunkItems & items )
    {
        BW_GUARD;

        for (ChunkMap::iterator i = ChunkManager::instance().cameraSpace()->chunks().begin();
            i != ChunkManager::instance().cameraSpace()->chunks().end(); i++)
        {
            BW::vector<Chunk*> & chunks = i->second;
            for (BW::vector<Chunk*>::iterator j = chunks.begin();
                j != chunks.end(); ++j)
            {
                Chunk* pChunk = *j;

                if (!pChunk->isBound() || !EditorChunkCache::instance( *pChunk ).edIsWriteable() )
                    continue;

                struct Visitor
                {
                    BW::vector< ChunkItemPtr > & items_;
                    Visitor( BW::vector< ChunkItemPtr > & items ) :
                        items_( items ) {}
                    bool operator()( const ChunkItemPtr & item )
                    {
                        items_.push_back( item );
                        return false;
                    }
                };

                Visitor visitor( items );
                EditorChunkCache::visitStaticItems( *pChunk, visitor );

            }
        }
    }


    /**
     *  This helper function filters a list of chunk items and returns only
     *  the ones that are selectable.
     */
    void filterSelectableItems( ChunkItemRevealer::ChunkItems & unfilteredItems, bool ignoreVis, bool ignoreFrozen,
                               ChunkItemRevealer::ChunkItems & retItems )
    {
        BW_GUARD;

        BW::set< Chunk * > selectableShells;
        BW::set< VeryLargeObjectPtr > selectedVLO;
        for (ChunkItemRevealer::ChunkItems::iterator it = unfilteredItems.begin();
            it != unfilteredItems.end(); ++it)
        {
            ChunkItemPtr item = *it;

            if (item->isShellModel())
            {
                if (SelectionFilter::canSelect( item.get(), true, false, ignoreVis, ignoreFrozen ))
                {
                    selectableShells.insert( item->chunk() );
                }
            }
        }

        for (ChunkItemRevealer::ChunkItems::iterator it = unfilteredItems.begin();
            it != unfilteredItems.end(); ++it)
        {
            ChunkItemPtr item = *it;

            if (!SelectionFilter::canSelect( item.get(), true, false, ignoreVis, ignoreFrozen ))
            {
                continue;
            }

            DataSectionPtr ds = item->pOwnSect();

            if (ds && ds->sectionName() == "overlapper")
            {
                continue;
            }

            if (ds && ds->sectionName() == "vlo" &&
                !item->edCheckMark(EditorChunkItem::selectionMark()))
            {
                continue;
            }

            if (!item->isShellModel() && !item->chunk()->isOutsideChunk() &&
                selectableShells.find( item->chunk() ) != selectableShells.end())
            {
                // If the object is inside a shell, and the shell is already selected,
                // then we shouldn't select the shell's contents (this item)
                continue;
            }

            if(item->edIsVLO())
            {
                ChunkVLO *chunkVLO = dynamic_cast< ChunkVLO* >( item.getObject() );
                VeryLargeObjectPtr vlo = chunkVLO->object();
                
                std::pair< BW::set< VeryLargeObjectPtr >::iterator, bool > insertIt = selectedVLO.insert( vlo );
                if( insertIt.second == false )
                {
                    continue;
                }
            }
            
            retItems.push_back( item );
        }
    }


    /**
     *  This class helps when changing the Selection Filters temporarily,
     *  restoring the previous Selection Filters on destruction.
     */
    class ScopedSelectionFilter
    {
    public:
        /**
         *  Constructor.  Default, does nothing.
         */
        ScopedSelectionFilter() :
            started_( false )
        {
        }


        /**
         *  Constructor.  Sets the selection filters straight away.
         */
        ScopedSelectionFilter( SelectionFilter::SelectMode selectMode,
                                const BW::string & typeFilters,
                                const BW::string & noSelTypeFilters ) :
            started_( false )
        {
            start( selectMode, typeFilters, noSelTypeFilters );
        }


        /**
         *  This method starts changing the selection filters on an instance
         *  created with the default constructor.
         */
        void start( SelectionFilter::SelectMode selectMode,
                                const BW::string & typeFilters,
                                const BW::string & noSelTypeFilters )
        {
            if (!started_)
            {
                oldSelectMode_ = SelectionFilter::getSelectMode();
                oldTypeFilters_ = SelectionFilter::typeFilters();
                oldNoSelTypeFilters_ = SelectionFilter::noSelectTypeFilters();
                SelectionFilter::setSelectMode( selectMode );
                SelectionFilter::typeFilters( typeFilters );
                SelectionFilter::noSelectTypeFilters( noSelTypeFilters );
                started_ = true;
            }
            else
            {
                ERROR_MSG( "Selection filters already started.\n" );
            }
        }


        /**
         *  Destructor.  Restore the filters, if set.
         */
        ~ScopedSelectionFilter()
        {
            if (started_)
            {
                SelectionFilter::setSelectMode( oldSelectMode_ );
                SelectionFilter::typeFilters( oldTypeFilters_ );
                SelectionFilter::noSelectTypeFilters( oldNoSelTypeFilters_ );
            }
        }

    private:
        bool started_;
        SelectionFilter::SelectMode oldSelectMode_;
        BW::string oldTypeFilters_;
        BW::string oldNoSelTypeFilters_;
    };
} // anonymous namespace


class SelectionOperation : public UndoRedo::Operation
{
public:
    SelectionOperation( const BW::vector<ChunkItemPtr>& before, const BW::vector<ChunkItemPtr>& after );

private:
    virtual void undo();
    virtual bool iseq( const UndoRedo::Operation & oth ) const;

    BW::vector<ChunkItemPtr> before_;
    BW::vector<ChunkItemPtr> after_;
};

static AutoConfigString s_selectionFxPrefix( "selectionfx/prefix" );

// Used for showing the triangles swept through the collision scene
class SelectionOverrideBlock : public Moo::DrawContext::OverrideBlock
{
public:
    SelectionOverrideBlock():
        Moo::DrawContext::OverrideBlock( s_selectionFxPrefix, true )
    {   }
};

WorldManager::WorldManager(): 
    spaceLock_( INVALID_HANDLE_VALUE )
    , inited_( false )
    , updating_( false )
    , chunkManagerInited_( false )
    , dTime_( 0.1f )
    , romp_( NULL )
    , globalWeather_( false )
    , hwndApp_( NULL )
    , hwndGraphics_( NULL )
    , changedEnvironment_( false )
    , secsPerHour_( 0.f )
    , changedPostProcessing_( false )
    , userEditingPostProcessing_( false )
    , currentMonitoredChunk_( 0 )
    , currentPrimGroupCount_( 0 )
    , angleSnaps_( 0.f )
    , movementSnaps_( 0.f, 0.f, 0.f )
    , canSeeTerrain_( false )
    , mapping_( NULL )
    , spaceManager_( NULL )
    , drawSelection_( false )
    , terrainFormatStorage_()
    , renderDisabled_( false )
    , GUI::ActionMaker<WorldManager>(
        "changeSpace|newSpace|editSpace|recreateSpace|recentSpace|clearUndoRedoHistory|doExternalEditor|"
        "doReloadAllTextures|doReloadAllChunks|doExit|setLanguage|recalcCurrentChunk",
        &WorldManager::handleGUIAction )
    , GUI::UpdaterMaker<WorldManager>( "updateRecreateSpace|updateUndo|updateRedo|updateExternalEditor|updateLanguage", &WorldManager::handleGUIUpdate )
    , lastModifyTime_( 0 )
    , cursor_( NULL )
    , waitCursor_( true )
    , warningOnLowMemory_( true )
    , insideQuickSave_(false)
    , chunkWatcher_ (new ChunkWatcher() )
    , timeLastUpdateTexLod_( 0.0f )
    , settingSelection_( false )
    , revealingSelection_( false )
    , disableClose_( false )
    , uiBlocked_( false )
    , slowTaskCount_( 0 )
    , savedCursor_( NULL )
    , enableEditorRenderables_( true )
    , ChunkProcessorManager()
    , colourDrawContext_( NULL )
    , shadowDrawContext_( NULL )
    , exitRequested_( false )
    , selectionOverride_( NULL )
    , framesToDelayBeforeCallback_( 0 )
    , addUndoBarrier_( false )
    , frameDelayCallback_( NULL )

{
    BW_GUARD;

    MF_WATCH( "Chunk Processor/Time Slice",
        s_minimumAllowTimeSliceMS_, Watcher::WT_READ_WRITE,
        "Minimum amount of time per frame to dedicate to the chunk processors. "
        "A value of 0 means no dedicated time.");

    MF_WATCH( "Chunk Processor/Wait Warn Threshold",
        s_timeSlicePauseWarnThresholdMS_, Watcher::WT_READ_WRITE,
        "Threshold for warning about chunk processors taking too long to yield. "
        "A value of 0 disables the warning." );

    SpaceEditor::instance( this );

    SlowTaskHandler::handler( this );
    MaterialProperties::runtimeInitMaterialProperties();
    setPlayerPreviewMode( false );
    resetCursor();
    ChunkProcessorManager::setThreadBlockCallback( &checkThreadNeedsToBeBlocked );
    WorldManager::allowChunkProcessorThreadsToRun( false );
}


WorldManager::~WorldManager()
{
    BW_GUARD;

    if (SlowTaskHandler::handler() == this)
    {
        SlowTaskHandler::handler( NULL );
    }

    if (inited_) this->fini();
    if( spaceLock_ != INVALID_HANDLE_VALUE )
        CloseHandle( spaceLock_ );

    bw_safe_delete(spaceManager_);
}


void WorldManager::fini()
{
    BW_GUARD;

    if (!inited_)
    {
        return;
    }

    WorldManager::allowChunkProcessorThreadsToRun( true );
    BgTaskManager::instance().stopAll();
    ChunkProcessorManager::stopAll();
    FileIOTaskManager::instance().stopAll();
    ChunkProcessorManager::tick();

    // neilr: Clear unsaved data before starting the shutdown.
    //        The world manager hangs on to unsaved terrain blocks, and needs Moo to free their
    //        renderer resources. Do this prior to performing shut down.
    //ZhongxiZ: This should be called after all background threads are stopped.
    //Some threads may add more unsaved data into the list.
    //Bug: BWT-22072
    this->clearUnsavedData();

    if( inited_ )
    {
        bw_safe_delete( colourDrawContext_ );
        bw_safe_delete( shadowDrawContext_ );
        bw_safe_delete( selectionOverride_ );

        #if UMBRA_ENABLE    
        ChunkManager::instance().umbra()->terrainOverride( NULL );
        #endif

        Script::clearTimers();
        GlobalEmbodiments::fini();
        stopBackgroundCalculation();

        // Clear objects held on to by the selection and the undo/redo 
        // barriers.
        BW::vector<ChunkItemPtr> emptySelection;
        setSelection(emptySelection);
        UndoRedo::instance().clear();

        readOnlyTerrainBlocks_.clear();

        // Release terrain block
        terrainFormatStorage_.clear();

        LightContainerDebugger::fini();
        ChunkItemFrustumLocator::fini();
        CoordModeProvider::fini();
        SnapProvider::fini();
        ObstacleLockCollisionCallback::s_default.clear();

        SpaceMap::deleteInstance();

        forcedLodMap_.clearLODData();

        ResourceCache::instance().fini();
        worldEditorCamera_ = NULL;

        if ( romp_ )
            romp_->enviroMinder().deactivate();

        DEBUG_MSG( "Calling WorldEditor Destructor\n" );
        
        DeprecatedSpaceHelpers::cameraSpace( NULL );
        pHeightMap_.reset();

        HeightModule::fini();

        EditorChunkLinkManager::instance().setValid(false);
        mapping_ = NULL;

        // Need to clear all Tools here because it holds references to
        // objects that need to be destroyed before ChunkManager fini.
        // These tools are related to selections and gizmos. The tools
        // themselves are created via python and are not destroyed until
        // the scripting side has been shutdown in
        // Initialisation::finaliseScripts()
        ToolList::clearAll();

        ChunkManager::instance().fini();
        SpaceManager::instance().fini();

        EditorChunkTerrainProjector::instance().fini();
        MaterialKinds::fini();
        pTerrainManager_.reset();
        ResourceLoader::fini();

        if ( romp_ )
        {
            PyObject * pMod = PyImport_AddModule( "WorldEditor" );
            PyObject_DelAttrString( pMod, "romp" );

            Py_DECREF( romp_ );
            romp_ = NULL;
        }

        while( ToolManager::instance().tool() )
        {
            WARNING_MSG( "WorldManager::fini : There is still a tool on the stack that should have been cleaned up\n" );
            ToolManager::instance().popTool();
        }

        EditorUserDataObjectType::shutdown();
        EditorEntityType::shutdown();

        ChunkItemTreeNode::nodeCache().fini();

        if (s_selectionMaterial)
        {
            s_selectionMaterial = NULL;
        }

        delete AmortiseChunkItemDelete::pInstance();
        delete ChunkPhotographer::pInstance();
        delete DXEnum::pInstance();

        GUI::Win32InputDevice::fini();
        PropManager::fini();
        BWResource::instance().purgeAll();

        MetaDataType::fini();

        delete SceneBrowser::pInstance();

        delete WEPreloader::pInstance();

        inited_ = false;
    }
}


WorldManager& WorldManager::instance()
{
    MF_ASSERT(s_pInstance);


    return *s_pInstance;
}

namespace
{

struct isChunkFileExists
{
    bool operator()( const BW::string& filename, GeometryMapping* dirMapping )
    {
        BW_GUARD;

        return BWResource::fileExists( dirMapping->path() + filename + ".chunk" );
    }
}
isChunkFileExists;

};

void WorldManager::update( float dTime )
{
    BW_GUARD;

    // This could be called by UI script events at any time
    // Block it while doing other things
    // Block it when the progress bar is displayed (UI blocked)
    if (!inited_ || updating_ || criticalMessageOccurred() || uiBlocked())
    {
        return;
    }

    if (framesToDelayBeforeCallback_ > 0)
    {
        if (--framesToDelayBeforeCallback_ == 0 && frameDelayCallback_)
        {
            frameDelayCallback_();
        }
    }

    this->flushDelayedChanges();
    ChunkProcessorManager::setNumberOfThreads(
        NUM_BACKGROUND_CHUNK_PROCESSING_THREADS );
    ChunkProcessorManager::tick();

    FileIOTaskManager::instance().tick();

    Moo::TextureManager::instance()->streamingManager()->tick();

    OptionsHelper::tick();

    updating_ = true;

    Renderer::instance().tick(dTime);

    s_update.start();

    dTime_ = dTime;
    s_totalTime_ += dTime;

    g_specialConsoleString = "";

    postPendingErrorMessages();

    // set input focus as appropriate
    bool acceptInput = cursorOverGraphicsWnd();
    WorldEditorApp::instance().mfApp()->handleSetFocus( acceptInput );

    //GIZMOS
    if ( InputDevices::isShiftDown() ||
        InputDevices::isCtrlDown() ||
        InputDevices::isAltDown() )
    {
        // if pressing modifier keys, remove the forced gizmo set to enable
        // normal gizmo behaviour with the modifier keys.
        GizmoManager::instance().forceGizmoSet( NULL );
    }

    //TOOLS
    // calculate the current world ray from the mouse position
    // (don't do this if moving the camera around (for more response)
    bool castRay = !InputDevices::isKeyDown(KeyCode::KEY_RIGHTMOUSE);
    if ( acceptInput && castRay )
    {
        worldRay_ = getWorldRay( currentCursorPosition() );

        ToolPtr spTool = ToolManager::instance().tool();
        if ( spTool )
        {
            spTool->calculatePosition( worldRay_ );
            spTool->update( dTime );
        }
        
        // increment the mark counter as it can be used by some of the tools.
        Chunk::nextMark();
    }

    // Tick editor objects that want to be ticked.
    this->tickEditorTickables();

    // Chunks:
    if ( chunkManagerInited_ )
    {
        // Linker manager tick method
        s_linkerManager.start();
        WorldManager::instance().linkerManager().tick(); 
        s_linkerManager.stop();

        // Link manager tick method
        s_linkManager.start();
        EditorChunkLinkManager::instance().update( dTime_ ); 
        s_linkManager.stop();

        s_chunkTick.start();
        markChunks();
        ChunkManager::instance().tick( dTime_ );
        s_chunkTick.stop();

        // Amortise chunk item delete tick method
        s_AmortiseChunkItemDelete.start();
        AmortiseChunkItemDelete::instance().tick();
        s_AmortiseChunkItemDelete.stop();
    }

    // Background tasks:
    BgTaskManager::instance().tick();
    ProviderStore::tick( dTime );

    // Scripts
    Script::tick( getTotalTime() );
    LightEmbodimentManager::instance().tick();
    
    // Entity models:
    EditorChunkEntity::calculateDirtyModels();

    // UserDataObject models:
    EditorChunkUserDataObject::calculateDirtyModels();

    if ( romp_ )
        romp_->update( dTime_, globalWeather_ );

    // update the flora redraw state
    bool drawFlora = !!Options::getOptionInt( "render/environment/drawDetailObjects", 1 );
    drawFlora &= !!Options::getOptionInt( "render/environment", 0 );
    drawFlora &= !Options::getOptionInt( "render/hideOutsideObjects", 0 );
    Flora::enabled(drawFlora);

    static bool firstTime = true;
    static bool canUndo, canRedo, canEE;
    static bool playerPreviewMode;
    static BW::string cameraMode;
    static int terrainWireFrame;
    static int drawNavMesh;
    if( firstTime || canUndo != UndoRedo::instance().canUndo() ||
        canRedo != UndoRedo::instance().canRedo() ||
        canEE != !!updateExternalEditor( NULL ) ||
        playerPreviewMode != isInPlayerPreviewMode() ||
        cameraMode != Options::getOptionString( "camera/speed" ) ||
        terrainWireFrame != Options::getOptionInt( "render/terrain/wireFrame", 0 ) ||
        drawNavMesh != Options::getOptionInt( "render/misc/drawNavMesh", 0 ) )
    {
        firstTime = false;
        canUndo = UndoRedo::instance().canUndo();
        canRedo = UndoRedo::instance().canRedo();
        canEE = !!updateExternalEditor( NULL );
        cameraMode = Options::getOptionString( "camera/speed" );
        playerPreviewMode = isInPlayerPreviewMode();
        terrainWireFrame = Options::getOptionInt( "render/terrain/wireFrame", 0 );
        drawNavMesh = Options::getOptionInt( "render/misc/drawNavMesh", 0 );
        GUI::Manager::instance().update();
    }

#if FMOD_SUPPORT
    //Tick FMod by setting the camera position
    Matrix view = WorldEditorCamera::instance().currentCamera().view();
    view.invert();
    Vector3 cameraPosition = view.applyToOrigin();
    Vector3 cameraDirection = view.applyToUnitAxisVector( 2 );
    Vector3 cameraUp = view.applyToUnitAxisVector( 1 );
    SoundManager::instance().setListenerPosition(
        cameraPosition, cameraDirection, cameraUp, dTime_ );
#endif // FMOD_SUPPORT

    s_update.stop();

    // Update missing LOD textures at the specified rate.
    timeLastUpdateTexLod_ += dTime_;

    if (timeLastUpdateTexLod_ > Options::getOptionFloat("terrain/texture/lodregentime", 1.0f))
    {
        timeLastUpdateTexLod_ = 0.0f;

        processMainthreadTask();
    }

    ToolPtr spTool = ToolManager::instance().tool();
    if (!spTool || !spTool->applying())
    {
        // Only check for memory load when not applying a tool. Interrupting a
        // tool might result in crashes, etc.
        checkMemoryLoad();
    }

    GlobalEmbodiments::tick( dTime );

    doBackgroundUpdating();

    updating_ = false;
}


/**
 *  Check if the memory is low.
 *  @param testNow test now or wait.
 *  TODO unify getMemoryLoad with the one in OfflineProcessorManager.
 */
bool WorldManager::isMemoryLow( bool testNow ) const
{
    BW_GUARD;

    static uint64 s_lastMemoryAllocTest = 0;
    static const uint64 s_memoryAllocTestInterval = 1 * stampsPerSecond();
    static bool s_memoryIsFragmented = false;

    int memoryLoad = (int)Memory::memoryLoad();
    if (memoryLoad > Options::getOptionInt( "warningMemoryLoadLevel", 90 ))
    {
        // We are passed the memory usage threshold, return 'true' now.
        return true;
    }

    // Try to alloc some memory, and raise a flag if it fails. We need to test
    // allocation like this to detect memory fragmentation: if we can't alloc a
    // fairly big block, memory is probably fragmented, so we fail.
    uint64 curTime = timestamp();
    if (testNow || curTime > (s_lastMemoryAllocTest + s_memoryAllocTestInterval))
    {
        s_lastMemoryAllocTest = curTime;
        s_memoryIsFragmented = false;
        char * memoryTest = NULL;

        // To detect fragmentation, we test for different size allocations
        // depending on the texture quality. That way, WE can still keep
        // going when using medium or low quality textures, evem if memory
        // is fragmented enough not to fit a high quality textures.

        // If using high texture quality, try to alloc 22M, which is the
        // approximate size of the biggest single alloc that we normally do
        // (a 2k x 2k texture + mips).
        uint32 testSize = 22;

        Moo::GraphicsSetting::GraphicsSettingPtr pTextureQuality =
                    Moo::GraphicsSetting::getFromLabel( "TEXTURE_QUALITY" );
        if (pTextureQuality && pTextureQuality->activeOption() != 0)
        {
            // Since we're not using high texture quality, the size of a
            // common big memory block is much smaller, for example, a
            // 1K x 1K medium quality texture with mips is about 5.6MB.
            testSize = 8;
        }

        try
        {
            memoryTest = new char[ testSize * 1024 * 1024 ]; 
        }
        catch(...)
        {
        }

        delete [] memoryTest;
        s_memoryIsFragmented = (memoryTest == NULL);
        if (s_memoryIsFragmented)
        {
            WARNING_MSG( "Memory is fragmented, failed to alloc %d MB (memory load is %d %%).\n",
                testSize, memoryLoad );
        }
    }

    return s_memoryIsFragmented;
}


bool WorldManager::isMemoryLoadWarningNeeded() const
{
    BW_GUARD;

    return warningOnLowMemory_ &&
            (isMemoryLow() ||
            Moo::rc().memoryCritical() ||
            BinaryBlock::memoryCritical());
}


void WorldManager::checkMemoryLoad()
{
    BW_GUARD;

    static uint64 s_lastCanLoadChunksWarning = 0;
    static const uint64 s_canLoadChunksWarningInterval = 5 * stampsPerSecond();

    // Don't allow more chunk loading if memory is low and/or fragmented.
    bool canLoadChunks = !isMemoryLow();
    if (!canLoadChunks)
    {
        Moo::GraphicsSetting::GraphicsSettingPtr pTextureQuality = Moo::GraphicsSetting::getFromLabel( "TEXTURE_QUALITY" );
        if (pTextureQuality && pTextureQuality->activeOption() != 2)
        {
            ShowCursor( TRUE );
            CWaitCursor wait;
            ERROR_MSG( "Memory is Low, setting texture quality to 'Low' to save memory.\n" );
            WorldEditorApp::instance().mainWnd()->RedrawWindow();
            pTextureQuality->selectOption( 2, true );
        }
        if (s_lastCanLoadChunksWarning == 0 || timestamp() > (s_lastCanLoadChunksWarning + s_canLoadChunksWarningInterval))
        {
            s_lastCanLoadChunksWarning = timestamp();
            ERROR_MSG( "Memory is Low, please reduce the far plane, save and/or clear the undo/redo history.\n" );
        }
    }
    else
    {
        s_lastCanLoadChunksWarning = 0;
    }
    ChunkManager::instance().canLoadChunks( canLoadChunks );

    // Warn the user
    if (warningOnLowMemory_)
    {
        // Variable to avoid re-entry
        static bool s_showingDialog = false;

        if (!s_showingDialog && isMemoryLoadWarningNeeded())
        {
            s_showingDialog = true;

            if( LowMemoryDlg().DoModal() == IDC_SAVE )
            {
                CWaitCursor wait;
                stopBackgroundCalculation();
                UndoRedo::instance().clear();
                if (!insideQuickSave_)
                {
                    // We might have just done a quick save. Possible when
                    // UpdateWindow has been called from within the quickSave
                    // function.
                    quickSave();
                }
                unloadChunks();

                Moo::rc().memoryCritical( false );
                BinaryBlock::memoryCritical( false );
            }
            else
            {
                warningOnLowMemory_ = false;
            }

            s_showingDialog = false;
        }
    }
}

/**
 *  This method describes currently dirty chunk counts
 */
BW::string WorldManager::describeDirtyChunkStatus() const
{
    RecursiveMutexHolder lock( dirtyChunkListsMutex_ );
    EditorChunkCache::lock();

    BW::stringstream ss;

    const DirtyChunkLists& dirtyChunkLists = 
        this->dirtyChunkLists();

    for (size_t i = 0; i < dirtyChunkLists.size(); ++i)
    {
        if (dirtyChunkLists[ i ].num())
        {
            ss << dirtyChunkLists.name( i ) << ": "
                << dirtyChunkLists[ i ].num() << ' ';
        }
    }

    EditorChunkCache::unlock();

    BW::string dirtyChunks = ss.str();

    if (!dirtyChunks.empty())
    {
        dirtyChunks.resize( dirtyChunks.size() - 1 );
    }
    return dirtyChunks;
}


/**
 *  This method writes out some status panel sections that are done every frame.
 *  i.e. FPS and cursor location.
 */
void WorldManager::writeStatus()
{
    BW_GUARD;

    BW::StackString< 2048 > stackStringContainer;

    //Panel 0 - memory load
    BW::string & statusMessage = stackStringContainer.container();
    LocaliseUTF8( statusMessage, L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/MEMORY_LOAD", (int)Memory::memoryLoad() );
    this->setStatusMessage( 0, statusMessage );

    //Panel 1 - num polys
    statusMessage.clear();
    LocaliseUTF8( statusMessage, L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/TRIS", Moo::rc().lastFrameProfilingData().nPrimitives_ );
    this->setStatusMessage( 1, statusMessage );

    //Panel 2 - snaps
    statusMessage.clear();
    if ( this->snapsEnabled() )
    {
        Vector3 snaps = this->movementSnaps();
        if( this->terrainSnapsEnabled() )
        {
            LocaliseUTF8( statusMessage,
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SNAP",
                snaps.x, "T", snaps.z );
        }
        else
        {
            LocaliseUTF8( statusMessage,
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SNAP",
                snaps.x, snaps.y, snaps.z );
        }
    }
    else
    {
        if ( this->terrainSnapsEnabled() )
        {
            LocaliseUTF8( statusMessage,
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SNAP_TERRAIN");
        }
        else
        {
            LocaliseUTF8( statusMessage,
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SNAP_FREE");
        }
    }
    this->setStatusMessage( 2, statusMessage );

    statusMessage.clear();

    //Panel 3 - locator position
    if ( ToolManager::instance().tool() && ToolManager::instance().tool()->locator() )
    {
        Vector3 pos = ToolManager::instance().tool()->locator()->transform().applyToOrigin();
        Chunk* chunk = ChunkManager::instance().cameraSpace()->findChunkFromPoint( pos );

        if (chunk && EditorChunkCache::instance( *chunk ).pChunkSection())
        {
            BW::vector<DataSectionPtr>  modelSects;
            EditorChunkCache::instance( *chunk ).pChunkSection()->openSections( "model", modelSects );

            LocaliseUTF8( statusMessage, L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHUNK_LOCATOR_POSITION",
                Formatter( pos.x, L"%0.2f" ),
                Formatter( pos.y, L"%0.2f" ),
                Formatter( pos.z, L"%0.2f" ),
                chunk->identifier(),
                (int) modelSects.size(),
                currentPrimGroupCount_ );
        }
        else
        {
            LocaliseUTF8( statusMessage,
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHUNK_LOCATOR_POSITION",
                Formatter( pos.x, L"%0.2f" ),
                Formatter( pos.y, L"%0.2f" ),
                Formatter( pos.z, L"%0.2f" ) );
        }
    }
    this->setStatusMessage( 3, statusMessage );


    //Panel5 - fps

    //7 period simple moving average of the frames per second
    static SMA<float>   s_averageFPS(7);
    static float        s_countDown = 1.f;

    float fps = dTime_ == 0.f ? 0.f : 1.f / dTime_;

    s_averageFPS.append( fps );

    if ( s_countDown < 0.f )
    {
        statusMessage.clear();
        LocaliseUTF8( statusMessage, L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/FPS",
            Formatter( s_averageFPS.average(), L"%0.1f" ) );
        this->setStatusMessage( 4, statusMessage );
        s_countDown = 1.f;
    }
    else
    {
        s_countDown -= dTime_;
    }

    // Panel 6 - number of chunks loaded
    BW::string dirtyChunks = this->describeDirtyChunkStatus();

    statusMessage.clear();
    if (!dirtyChunks.empty())
    {
        LocaliseUTF8( statusMessage, L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHUNK_LOADED_WITH_DIRTY",
            EditorChunkCache::chunks_.size(), dirtyChunks );
    }
    else
    {
        LocaliseUTF8( statusMessage, L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHUNK_LOADED",
            EditorChunkCache::chunks_.size() );
    }
    this->setStatusMessage( 5, statusMessage );
}


/**
 * This method returns the total time the editor has been running
 */
/*static */double WorldManager::getTotalTime()
{
    return s_totalTime_;
}


/**
 *  Re-calculate LOD and apply animations to objects in the world for this
 *  frame.
 *  @param chunkManager the chunk manager.
 */
void WorldManager::updateAnimations( ChunkManager& chunkManager )
{
    BW_GUARD_PROFILER( WorldManager_updateAnimations );

    static DogWatch updateAnimations( "Scene update" );
    ScopedDogWatch sdw( updateAnimations );

    chunkManager.updateAnimations();

    ChunkSpacePtr pSpace = chunkManager.cameraSpace();
    if (pSpace.exists())
    {
        EnviroMinder & enviro = pSpace->enviro();
        enviro.updateAnimations();
    }

    ToolPtr spTool = ToolManager::instance().tool();
    if (spTool)
    {
        spTool->updateAnimations();
    }

    ParticleSystemAction::flushLateUpdates();
}


/**
 *  This method renders the scene in a standard way.
 *  Call this method, or call each other method individually,
 *  interspersed with your own custom routines.
 */
void WorldManager::render( float dTime )
{
    BW_GUARD;

    if (renderDisabled_ || !inited_  || criticalMessageOccurred())
    {
        return;
    }
    this->updateAnimations( ChunkManager::instance() );

    ScopedDogWatch sdw( s_render );

    if( farPlane() != Options::getOptionInt( "graphics/farclip", (int)farPlane() ) )
        farPlane( (float)Options::getOptionInt( "graphics/farclip" ) );

    EditorChunkItem::hideAllOutside( !!Options::getOptionInt( "render/hideOutsideObjects", 0 ) );

    // Setup the data for counting the amount of primitive groups in the chunk
    // the locator is in, used for the status bar
    currentMonitoredChunk_ = 0;
    currentPrimGroupCount_ = 0;
    if ( ToolManager::instance().tool() && ToolManager::instance().tool()->locator() )
    {
        Vector3 pos = ToolManager::instance().tool()->locator()->transform().applyToOrigin();
        currentMonitoredChunk_ = ChunkManager::instance().cameraSpace()->findChunkFromPoint( pos );
    }

    // update any dynamic textures
    TextureRenderer::updateDynamics( dTime_ );
    //or just the water??

    //TODO: under water effect..
    Waters::instance().checkVolumes();

    // This is used to limit the number of rebuildCombinedLayer calls per frame
    // because they are very expensive.
    Terrain::EditorTerrainBlock2::nextBlendBuildMark();

    //  Make sure lodding occurs        
    Terrain::BasicTerrainLodController::instance().setCameraPosition( 
        Moo::rc().invView().applyToOrigin(), Moo::rc().zoomFactor() );

    IRendererPipeline* rp = Renderer::instance().pipeline();

    this->beginRender();

    //-- set up sun lighting.
    const ChunkSpacePtr& pSpace = ChunkManager::instance().cameraSpace();
    if (pSpace)
    {
        Moo::SunLight sun;
        sun.m_dir     = pSpace->sunLight()->worldDirection();
        sun.m_color   = pSpace->sunLight()->colour();
        sun.m_ambient = pSpace->ambientLight();

        Moo::rc().effectVisualContext().sunLight(sun);
    }

    this->renderRompPreScene();

    colourDrawContext_->resetStatistics();
    colourDrawContext_->begin( Moo::DrawContext::ALL_CHANNELS_MASK );

    rp->beginOpaqueDraw();

    // Ensure that the view projection matrix is set before rendering
    // any of the opaque geometry. It used to be that this would only be
    // set as a side effect of rendering a visual. If no visuals were drawn
    // then the terrain would be rendered with random view projection matrix
    // (PER_VIEW constants).
    // We'll also need to re-set the Fog constant (in PER_FRAME constants) since
    // it appears to be screwed up after the renderRompPreScene call above.
    Moo::EffectVisualContext& vc = Moo::rc().effectVisualContext();
    vc.updateSharedConstants( Moo::CONSTANTS_PER_VIEW
                            | Moo::CONSTANTS_PER_FRAME );

    if ( chunkManagerInited_ )
    {
        vc.initConstants();

        this->renderChunks();

        //Moo::LightContainerPtr lc = new Moo::LightContainer;
        //
        //lc->addDirectional(
        //  ChunkManager::instance().cameraSpace()->sunLight() );
        //lc->ambientColour(
        //  ChunkManager::instance().cameraSpace()->ambientLight() );
        //
        //Moo::rc().lightContainer( lc );

        EditorChunkLink::flush( dTime );
    }

    #if UMBRA_ENABLE    
    if (!ChunkManager::instance().umbra()->umbraEnabled())
    {
        // With umbra, terrain gets rendered inside "renderChunks" via the
        // ChunkUmbra terrainOverride.
    #endif
        this->renderTerrain();
    #if UMBRA_ENABLE    
    }
    #endif
    this->renderRompScene();

    rp->endOpaqueDraw();

    //Don't draw when paused
    if (dTime != 0.f)
    {
        chunkLockVisualizer_.draw();
    }

    //-- do lighting pass.
    rp->applyLighting();

    this->renderRompDelayedScene();
    rp->beginSemitransparentDraw();
    this->renderRompPostScene();
    rp->endSemitransparentDraw();

    this->renderRompPostProcess();

#if ENABLE_BSP_MODEL_RENDERING
    ChunkBspHolder::drawBsps( Moo::rc() );
#endif // ENABLE_BSP_MODEL_RENDERING

    Geometrics::flushDrawItems();

    if (enableEditorRenderables_)
    {
        colourDrawContext_->pushImmediateMode();
        this->renderEditorGizmos( *colourDrawContext_ );
        this->renderEditorRenderables();
        this->renderDebugGizmos();
        GeometryDebugMarker::instance().draw();
        GizmoManager::instance().draw( *colourDrawContext_ );
        colourDrawContext_->popImmediateMode();
    }

    colourDrawContext_->end( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK | Moo::DrawContext::SHIMMER_CHANNEL_MASK );
    colourDrawContext_->flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK | Moo::DrawContext::SHIMMER_CHANNEL_MASK );

    if (Options::getOptionBool( "drawSpecialConsole", false ) &&
        !g_specialConsoleString.empty())
    {
        static XConsole * pSpecCon = new XConsole();
        pSpecCon->clear();
        pSpecCon->setCursor( 0, 0 );
        pSpecCon->print( g_specialConsoleString );
        pSpecCon->draw( 0.1f );
    }

    Chunks_drawCullingHUD();
    this->endRender();

    // write status sections.
    // we write them here, because it is only here
    // that we can retrieve the poly count.
    writeStatus();

    // if no chunks are loaded then show the arrow + 
    showBusyCursor();
}


/**
 *  Note : this method assumes Moo::rc().view() has been set accordingly.
 *         it is up to the caller to set up this matrix.
 */
void WorldManager::beginRender()
{
    BW_GUARD;

    //--
    Renderer::instance().pipeline()->begin();

    Renderer::instance().pipeline()->beginCastShadows( *shadowDrawContext_);
    Renderer::instance().pipeline()->endCastShadows();

    //-- create the static view projection matrix holder.
    static Matrix s_lastVPMatrix = Moo::rc().viewProjection();

    //-- update the last view projection matrix.
    Moo::rc().lastViewProjection(s_lastVPMatrix);

    //-- update current frame matrices.
    Moo::rc().reset();
    Moo::rc().updateProjectionMatrix();
    Moo::rc().updateViewTransforms();
    Moo::rc().updateLodViewMatrix();

    //-- save the current view projection matrix.
    s_lastVPMatrix = Moo::rc().viewProjection();    

    //-- update constants.
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);
}


void WorldManager::renderRompPreScene()
{
    BW_GUARD;

    // draw romp pre scene
    s_rompDraw.start();
    romp_->drawPreSceneStuff();
    s_rompDraw.stop();

    FogController::instance().updateFogParams();
}

void WorldManager::renderChunksInUpdate()
{
    colourDrawContext_->pushImmediateMode();
    colourDrawContext_->begin( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
    renderChunks();
    colourDrawContext_->popImmediateMode();
}

void WorldManager::renderChunks()
{
    BW_GUARD;

    // draw chunks
    if (chunkManagerInited_)
    {
        if (drawSelection_)
        {
            WorldManager::instance().colourDrawContext().pushOverrideBlock( selectionOverride_ );
        }

        s_chunkDraw.start();
        uint32 scenaryWireFrameStatus_ = Options::getOptionInt( "render/scenery/wireFrame", 0 );         
        uint32 terrainWireFrameStatus_ = Options::getOptionInt( "render/terrain/wireFrame", 0 );
        ChunkManager::instance().camera( Moo::rc().invView(), ChunkManager::instance().cameraSpace() );

        Chunk::hideIndoorChunks_ = !OptionsScenery::shellsVisible();

        bool forceDrawShells = false;
        Chunk * cc = ChunkManager::instance().cameraChunk();

    #if UMBRA_ENABLE    
        if (ChunkManager::instance().umbra()->umbraEnabled())
        {
            // Umbra can't handle gameVisibility off or selected shells, so to
            // ensure shells under these circumnstances are drawn, we set the
            // forceDrawShells to true, but this is only needed if the camera
            // is not inside a shell.
            if (cc && cc->isOutsideChunk())
            {
                forceDrawShells = true;
            }

            s_umbraDraw.start();
            Moo::rc().setRenderState( D3DRS_FILLMODE, scenaryWireFrameStatus_ ?
                D3DFILL_WIREFRAME : D3DFILL_SOLID );

            ChunkManager::instance().umbraDraw( *colourDrawContext_ );          

            if (scenaryWireFrameStatus_ || terrainWireFrameStatus_)
            {
                Moo::rc().device()->EndScene();

                Vector3 bgColour = Vector3( 0, 0, 0 );
                Moo::rc().device()->Clear( 0, NULL,
                        D3DCLEAR_ZBUFFER,
                        Colour::getUint32( bgColour ), 1, 0 );

                Moo::rc().device()->BeginScene();           

                if (Terrain::BaseTerrainRenderer::instance()->version() ==
                    Terrain::TerrainSettings::TERRAIN2_VERSION)
                {
                    Terrain::TerrainRenderer2::instance()->zBufferIsClear( true );
                }

                ChunkManager::instance().umbra()->wireFrameTerrain( terrainWireFrameStatus_ & 1 );
                ChunkManager::instance().umbraRepeat( *colourDrawContext_ );
                ChunkManager::instance().umbra()->wireFrameTerrain( 0 );

                if (Terrain::BaseTerrainRenderer::instance()->version() ==
                    Terrain::TerrainSettings::TERRAIN2_VERSION)
                {
                    Terrain::TerrainRenderer2::instance()->zBufferIsClear( false );
                }
            }           
            s_umbraDraw.stop();
        }
        else
        {
            Moo::rc().setRenderState( D3DRS_FILLMODE, scenaryWireFrameStatus_ ?
            D3DFILL_WIREFRAME : D3DFILL_SOLID );
            ChunkManager::instance().draw( *colourDrawContext_ );
        }       
    #else
            Moo::rc().setRenderState( D3DRS_FILLMODE, scenaryWireFrameStatus_ ?
            D3DFILL_WIREFRAME : D3DFILL_SOLID );        
            ChunkManager::instance().draw( *colourDrawContext_ );
    #endif
        // render overlapping chunks
#if SPEEDTREE_SUPPORT
        speedtree::SpeedTreeRenderer::beginFrame( &romp_->enviroMinder(), 
            Moo::RENDERING_PASS_COLOR, Moo::rc().invView() );
#endif

        // This set makes sure that we draw shells only once when some shells
        // are selected or game visibility is off, no matter if the flag
        // 'forceDrawShells' is on or not.
        BW::set< Chunk * > shellsToDraw;

        if (cc)
        {
            // Umbra won't populate the overlapper drawList, this is used by
            // the non-umbra rendering path.
            for (BW::vector< Chunk * >::iterator ci = EditorChunkOverlapper::drawList.begin();
                 ci != EditorChunkOverlapper::drawList.end(); ++ci)
            {
                Chunk* c = *ci;
                if ( !c->isBound() )
                {
                    //TODO : this shoudln't happen, chunks should get out
                    // of the drawList when they are offline.
                    DEBUG_MSG( "WorldManager::renderChunks: Trying to draw chunk %s while it's offline!\n", c->resourceID().c_str() );
                    continue;
                }
                if (c->drawMark() != cc->drawMark() || forceDrawShells)
                {
                    shellsToDraw.insert( c );
                }
            }                   
        }
        EditorChunkOverlapper::drawList.clear();

        // Force rendering selected shells, ensuring the user can manipulate
        // them even if the should be culled normally.
        if (cc)
        {
            for (BW::vector< ChunkItemPtr >::iterator iter = selectedItems_.begin();
                iter != selectedItems_.end(); ++iter)
            {
                Chunk* c = (*iter)->chunk();
                if (c && !c->isOutsideChunk() &&
                    (c->drawMark() != cc->drawMark() || forceDrawShells))
                {
                    // Draw the shell if the draw mark requires it or if Umbra
                    // is enabled, because Umbra messes up with a shell's draw
                    // mark but doesn't draw it.
                    shellsToDraw.insert( c );
                }
            }
        }

        // inside chunks will not render if they are not reacheable through portals. 
        // If game visibility is off, the overlappers are used to render not-connected 
        // chunks. But, with the visibility bounding box culling, the overlapper may 
        // not be rendered, causing the stray shell to be invisible, even if it is 
        // itself inside the camera frustum. To fix this situation, when game visibility 
        // is turned off, after rendering the chunks, it goes through all loaded chunks, 
        // trying to render those that are inside and haven't been rendered for this frame. 
        // Visibility bounding box culling still applies.
        bool drawAllShells = Options::getOptionInt(
            "render/scenery/shells/gameVisibility" ) == 0;
        bool hideAllOutside = Options::getOptionInt(
            "render/hideOutsideObjects" ) == 1;

        if (cc != NULL && ( drawAllShells || hideAllOutside ))
        {
            ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
            if (space.exists())
            {
                for (ChunkMap::iterator it = space->chunks().begin();
                    it != space->chunks().end(); ++it)
                {
                    for (uint i = 0; i < it->second.size(); i++)
                    {
                        if (it->second[i] != NULL                       &&
                            !it->second[i]->isOutsideChunk()            &&
                            (it->second[i]->drawMark() != cc->drawMark() || forceDrawShells) &&
                            it->second[i]->isBound())
                        {
                            // If Umbra is enabled, simply ignore the draw mark
                            // because Umbra messes up with a shell's draw mark
                            // but doesn't draw it.
                            shellsToDraw.insert( it->second[i] );
                        }
                    }
                }
            }
        }

        // Add locked chunks to ChunkLockVisualiser to show them as read only
        ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
        if (space.exists())
        {
            ChunkMap::iterator begin = space->chunks().begin();
            ChunkMap::iterator end = space->chunks().end();
            for (ChunkMap::iterator it = begin; it != end; ++it)
            {
                for (uint i = 0; i < it->second.size(); i++)
                {
                    if (it->second[i] != NULL && 
                        it->second[i]->isBound() && 
                        OptionsMisc::readOnlyVisible() && 
                        !EditorChunkCache::instance( *it->second[i] ).edIsWriteable())
                    {
                        showReadOnlyRegion( it->second[i]->visibilityBox(), Matrix::identity );
                    }
                }
            }
        }

        // We set the next mark to be the same as the camera chunks mark
        // so that objects do not get rendered multiple times
        uint32 storedMark = Chunk::s_nextMark_;
        if (cc != NULL)
        {
            Chunk::s_nextMark_ = cc->drawMark();
        }

        // Draw all shells that need to be drawn explicitly.
        for (BW::set< Chunk * >::iterator it = shellsToDraw.begin();
            it != shellsToDraw.end(); ++it)
        {
            BoundingBox bb = (*it)->visibilityBox();
            bb.calculateOutcode( Moo::rc().viewProjection() );
            if (!bb.combinedOutcode())
            {
                (*it)->drawSelf( *colourDrawContext_ );
            }
            (*it)->drawMark( cc->drawMark() );
        }

        // Set the next mark back to the stored value
        Chunk::s_nextMark_ = storedMark;

        if (drawSelection_)
        {
            WorldManager::instance().colourDrawContext().popOverrideBlock( selectionOverride_ );
        }


        colourDrawContext_->end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
        colourDrawContext_->flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );

#if SPEEDTREE_SUPPORT
        speedtree::SpeedTreeRenderer::endFrame();
#endif

        Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

        s_chunkDraw.stop();
    }
    else
    {
        colourDrawContext_->end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
    }

}


void WorldManager::renderTerrain()
{   
    BW_GUARD;

    if (Options::getOptionInt( "render/terrain", 1 ))
    {
        // draw terrain
        ScopedDogWatch sdw( s_terrainDraw );

        canSeeTerrain_ = Terrain::BaseTerrainRenderer::instance()->canSeeTerrain();

        Moo::rc().pushRenderState( D3DRS_FILLMODE );

        bool wireFrameTerrain = Options::getOptionInt( "render/terrain/wireFrame", 0 ) != 0;

        #if UMBRA_ENABLE
        if (ChunkManager::instance().umbra()->umbraEnabled())
        {
            wireFrameTerrain &= ChunkManager::instance().umbra()->wireFrameTerrain();
        }
        #endif

        Moo::rc().setRenderState( D3DRS_FILLMODE, wireFrameTerrain ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

        if (drawSelection())
        {
            Moo::EffectMaterialPtr selectionMaterial = NULL;
            // TODO: We should get the FX name automatically with the version number,
            // instead of having to 'if' the version numbers.

            uint32 curTerrainVer = pTerrainSettings()->version();
            if (curTerrainVer == Terrain::TerrainSettings::TERRAIN2_VERSION)
            {
                if (s_selectionMaterial == NULL)
                {
                    s_selectionMaterial = new Moo::EffectMaterial();
                    s_selectionMaterialOk =
                        s_selectionMaterial->initFromEffect( s_terrainSelectionFx );
                }
                if ( s_selectionMaterialOk )
                    selectionMaterial = s_selectionMaterial;
            }

            if ( selectionMaterial != NULL )
            {
                Terrain::EditorBaseTerrainBlock::drawSelection( true );
                Terrain::BaseTerrainRenderer::instance()->drawAll( 
                    Moo::RENDERING_PASS_COLOR, selectionMaterial );
                Terrain::EditorBaseTerrainBlock::drawSelection( false );
            }
            else
            {
                // TODO: This is printing the error every frame. Should improve.
                ERROR_MSG( "WorldManager::renderTerrain: There is no valid selection shader for the current terrain\n" );
            }
        }
        else
        {
            Terrain::BaseTerrainRenderer::instance()->drawAll( Moo::RENDERING_PASS_COLOR );
        }

        if (!readOnlyTerrainBlocks_.empty())
        {
            AVectorNoDestructor< BlockInPlace >::iterator i = readOnlyTerrainBlocks_.begin();            
            for (; i != readOnlyTerrainBlocks_.end(); ++i)
            {
                Terrain::BaseTerrainRenderer::instance()->addBlock( i->second.getObject(),
                                                                i->first );
            }            

            if( !drawSelection() )
                Terrain::BaseTerrainRenderer::instance()->drawAll( 
                Moo::RENDERING_PASS_COLOR );

            readOnlyTerrainBlocks_.clear();

            FogController::instance().updateFogParams();
        }

        Moo::rc().popRenderState();
    }
    else
    {
        canSeeTerrain_ = false;
        Terrain::BaseTerrainRenderer::instance()->clearBlocks();
    }
}


void WorldManager::renderEditorGizmos( Moo::DrawContext& drawContext )
{
    BW_GUARD;

    // draw tools
    ToolPtr spTool = ToolManager::instance().tool();
    if ( spTool )
    {
        spTool->render( drawContext );
    }
}


void WorldManager::tickEditorTickables()
{
    BW_GUARD;

    // This allows tickable to add/remove tickables to the list
    // while being ticked, for example, removing itself after tick.
    BW::list<EditorTickablePtr> tempCopy = editorTickables_;

    for( BW::list<EditorTickablePtr>::iterator i = tempCopy.begin();
        i != tempCopy.end(); ++i )
    {
        (*i)->tick();
    }
}


void WorldManager::renderEditorRenderables()
{
    BW_GUARD;

    // This allows renderables to add/remove renderables to the list
    // while rendering, for example, removing itself after render.
    BW::set<EditorRenderablePtr> tempCopy = editorRenderables_;

    for( BW::set<EditorRenderablePtr>::iterator i = tempCopy.begin();
        i != tempCopy.end(); ++i )
    {
        (*i)->render();
    }
}


void WorldManager::renderDebugGizmos()
{
}


void WorldManager::renderRompScene()
{
    BW_GUARD;

    // draw romp post scene
    s_rompDraw.start();
    romp_->drawSceneStuff();
    s_rompDraw.stop();
}


void WorldManager::renderRompDelayedScene()
{
    BW_GUARD;

    // draw romp delayed scene
    s_rompDraw.start();
    romp_->drawDelayedSceneStuff();
    s_rompDraw.stop();
}


void WorldManager::renderRompPostScene()
{
    BW_GUARD;

    // draw romp post scene
    s_rompDraw.start();
    romp_->drawPostSceneStuff( *colourDrawContext_ );
    s_rompDraw.stop();
}

void WorldManager::renderRompPostProcess()
{
    BW_GUARD;

    // draw romp post Process
    s_rompDraw.start();
    romp_->drawPostProcessStuff();
    s_rompDraw.stop();
}


void WorldManager::addTickable( EditorTickablePtr tickable )
{
    BW_GUARD;

    editorTickables_.push_back( tickable );
}


void WorldManager::removeTickable( EditorTickablePtr tickable )
{
    BW_GUARD;

    editorTickables_.remove( tickable );
}


void WorldManager::addRenderable( EditorRenderablePtr renderable )
{
    BW_GUARD;

    editorRenderables_.insert( renderable );
}


void WorldManager::removeRenderable( EditorRenderablePtr renderable )
{
    BW_GUARD;

    editorRenderables_.erase( renderable );
}


void WorldManager::endRender()
{
    BW_GUARD;

    //--
    Renderer::instance().pipeline()->drawDebugStuff();

    //--
    Renderer::instance().pipeline()->end();
}


bool WorldManager::init( HINSTANCE hInst, HWND hwndApp, HWND hwndGraphics )
{
    BW_GUARD;

    if ( !inited_ )
    {
        MatrixProxy::setMatrixProxyCreator( new WEMatrixProxyCreator() );
        Tool::setChunkFinder( new WEChunkFinder() );

        new WEPreloader();

        new SceneBrowser;
        SceneBrowser::instance().callbackSelChange(
            new BWFunctor1<
                        WorldManager, const SceneBrowser::ItemList & >(
                                this, &WorldManager::sceneBrowserSelChange ) );
        SceneBrowser::instance().callbackCurrentSel(
            new BWFunctor0RC< WorldManager, const BW::vector<ChunkItemPtr>& >(
                                this, &WorldManager::selectedItems ) );

        if( CVSWrapper::init() == CVSWrapper::FAILURE )
        {
            return false;
        }

        hwndApp_ = hwndApp;
        hwndGraphics_ = hwndGraphics;

        ::ShowCursor( true );

        //init python data sections
        PyObject * pMod = PyImport_AddModule( "WorldEditor" );  // borrowed
        PyObject * pBW = PyImport_AddModule( "BigWorld" );  // borrowed

        Personality::import( Options::getOptionString( "personality",
                "Personality" ) );

        new DXEnum( s_dxEnumPath );

        new ChunkPhotographer();

        new AmortiseChunkItemDelete();

        // Set callback for PyScript so it can know total game time
        Script::setTotalGameTimeFn( getTotalTime );

        // create the editor entities descriptions
        // this cannot be called from within the load thread
        // as python and load load thread hate each other
        EditorEntityType::startup();
        EditorUserDataObjectType::startup();

        // init BWLockD
        if ( Options::getOptionBool( "bwlockd/use", true ))
        {
            BW::string host = Options::getOptionString( "bwlockd/host" );
            BW::string username = Options::getOptionString( "bwlockd/username" );
            if( username.empty() )
            {
                wchar_t name[1024];
                DWORD size = ARRAY_SIZE( name );
                GetUserName( name, &size );
                bw_wtoutf8( name, username );
            }
            BW::string hostname = host.substr( 0, host.find( ':' ) );           
            conn_.init( host, username, 0, 0 );
        }
        // Init GUI Manager
        // Backwards compatibility for options.xml without this option.
        // Otherwise all buttons light up
        Options::setOptionInt( "render/chunk/vizMode", 
            Options::getOptionInt( "render/chunk/vizMode", 0 ));
        GUI::Manager::instance().optionFunctor().setOption( this );
        updateLanguageList();

        // Init terrain:
        pTerrainManager_ = TerrainManagerPtr( new Terrain::Manager() );

        // Init Material Types:
        MF_VERIFY( MaterialKinds::init() );

        EditorChunkTerrainProjector::instance().init();

        // Init chunk manager
        DataSectionPtr configSection;
        SpaceManager::instance().init();
        ClientChunkSpaceAdapter::init();
        ChunkManager::instance().init();

        // Init HeightMap
        pHeightMap_ = HeightMapPtr( new HeightMap );

        // Precompile effects?
        if ( Options::getOptionInt( "precompileEffects", 1 ) )
        {
            BW::vector<ISplashVisibilityControl*> SVCs;
            if ( CSplashDlg::getSVC() )
                SVCs.push_back(CSplashDlg::getSVC());
            if (WaitDlg::getSVC())
                SVCs.push_back(WaitDlg::getSVC());
            ResourceLoader::instance().precompileEffects( SVCs );
        }

        class WorldEditorMRUProvider : public MRUProvider
        {
            virtual void set( const BW::string& name, const BW::string& value )
            {
                Options::setOptionString( name, value );
            }
            virtual const BW::string get( const BW::string& name ) const
            {
                return Options::getOptionString( name );
            }
        };
        static WorldEditorMRUProvider WorldEditorMRUProvider;
        spaceManager_ = new SpaceNameManager( WorldEditorMRUProvider );


        // This scoped class will clean up in case of an early return.
        InitFailureCleanup failureCleaner( inited_, *this );

        if( !spaceManager_->num() || !changeSpace( spaceManager_->entry( 0 ), false ) )
        {
            CSplashDlg::HideSplashScreen();
            if (WaitDlg::isValid())
                WaitDlg::getSVC()->setSplashVisible( false );
            for(;;)
            {
                MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
                MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_TITLE"),
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_TEXT"),
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_OPEN"),
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_CREATE"),
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_EXIT") );
                int result = mb.doModal( mainFrame->m_hWnd );
                if( result == 0 )
                {
                    if( changeSpace( GUI::ItemPtr() ) )
                        break;
                }
                else if( result == 1 )
                {
                    if( newSpace( GUI::ItemPtr() ) )
                        break;
                }
                else
                {
                    exitRequested_ = true;
                    // failureCleaner above takes care of cleaning up
                    return false; //sorry
                }
            }
        }

        chunkManagerInited_ = true;

        if ( !initRomp() )
        {
            // failureCleaner above takes care of cleaning up
            return false;
        }

        // start up the camera
        // Inform the camera of the window handle
        worldEditorCamera_ = new WorldEditorCamera();

        //watchers
        MF_WATCH( "Client Settings/Far Plane",
        *this,
        MF_ACCESSORS( float, WorldManager, farPlane) );

        MF_WATCH( "Render/Draw Portals", ChunkBoundary::Portal::drawPortals_ );

        BgTaskManager::instance().initWatchers( "WorldEditor" );
        FileIOTaskManager::instance().initWatchers( "FileIO" );

        // set the saved far plane
        float fp = Options::getOptionFloat( "graphics/farclip", 500 );
        farPlane( fp );

        // Use us to provide the snap settings for moving objects etc
        SnapProvider::instance( this );
        CoordModeProvider::ins( this );

        ApplicationInput::disableModeSwitch();

        ResourceCache::instance().init();

        Terrain::EditorTerrainBlock2::blendBuildInterval(
            Options::getOptionInt( "terrain2/blendsBuildInterval",
                Terrain::EditorTerrainBlock2::blendBuildInterval() ) );

        #if UMBRA_ENABLE
            ChunkManager::instance().umbra()->terrainOverride(
                new BWFunctor0< WorldManager >( this, &WorldManager::renderTerrain ) );
        #endif

        #if ENABLE_MULTITHREADED_COLLISION
        ChunkSpace::usingMultiThreadedCollision( true );
        #endif //ENABLE_MULTITHREADED_COLLISION

        // Start background processing
        startNumThreads( NUM_BACKGROUND_CHUNK_PROCESSING_THREADS );

        EditorChunkNavmeshCacheBase::s_doWaitBeforeCalculate = true;

        fencesToolView_ = new FencesToolView();

        colourDrawContext_ = new Moo::DrawContext( Moo::RENDERING_PASS_COLOR );
        colourDrawContext_->attachWatchers( "Colour", false );

        shadowDrawContext_ = new Moo::DrawContext( Moo::RENDERING_PASS_SHADOWS );
        shadowDrawContext_->attachWatchers( "Shadows", false );

        selectionOverride_ = new SelectionOverrideBlock;

        inited_ = true;
    }

    return true;
}

#ifndef BIGWORLD_CLIENT_ONLY
BWLock::BWLockDConnection& WorldManager::connection()
{
    return conn_;
}
#endif // BIGWORLD_CLIENT_ONLY


GeometryMapping* WorldManager::geometryMapping()
{
    return mapping_;
}


const GeometryMapping* WorldManager::geometryMapping() const
{
    return mapping_;
}


/*~ attribute WorldEditor.romp
 *  @components{ worldeditor }
 *
 *  The romp attribute gives access to the RompHarness object in the editor. For
 *  more information, see the RompHarness class.
 *
 *  @type   RompHarness
 */

bool WorldManager::initRomp()
{
    BW_GUARD;

    if ( !romp_ )
    {
        romp_ = new WorldEditorRompHarness;

        // set it into the WorldEditor module
        PyObject * pMod = PyImport_AddModule( "WorldEditor" );  // borrowed
        PyObject_SetAttrString( pMod, "romp", romp_ );

        if ( !romp_->init() )
        {
            return false;
        }

        romp_->enviroMinder().activate();
        this->setTimeFromOptions();
    }
    return true;
}

void WorldManager::focus( bool state )
{
    BW_GUARD;

    WorldEditorApp::instance().mfApp()->handleSetFocus( state );
}


void WorldManager::timeOfDay( float t )
{
    BW_GUARD;

    if ( romp_ )
        romp_->setTime( t );
}

bool WorldManager::cursorOverGraphicsWnd() const
{
    BW_GUARD;

    MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();

    HWND fore = ::GetForegroundWindow();
    if ( fore != mainFrame->GetSafeHwnd() && ::GetParent( fore ) != mainFrame->GetSafeHwnd() )
        return false; // foreground window is not the main window nor a floating panel.

    RECT rt;
    ::GetWindowRect( hwndGraphics_, &rt );
    POINT pt;
    ::GetCursorPos( &pt );

    if ( pt.x < rt.left ||
        pt.x > rt.right ||
        pt.y < rt.top ||
        pt.y > rt.bottom )
    {
        return false;
    }

    HWND hwnd = ::WindowFromPoint( pt );
    if ( hwnd != hwndGraphics_ )
        return false; // it's a floating panel, return.
    HWND parent = hwnd;
    while( ::GetParent( parent ) != NULL )
        parent = ::GetParent( parent );
    ::SendMessage( hwnd, WM_MOUSEACTIVATE, (WPARAM)parent, WM_LBUTTONDOWN * 65536 + HTCLIENT );
    return true;
}

POINT WorldManager::currentCursorPosition() const
{
    BW_GUARD;

    POINT pt;
    ::GetCursorPos( &pt );
    ::ScreenToClient( hwndGraphics_, &pt );

    return pt;
}

Vector3 WorldManager::getWorldRay(POINT& pt) const
{
    BW_GUARD;

    return getWorldRay( pt.x, pt.y );
}

Vector3 WorldManager::getWorldRay(int x, int y) const
{
    BW_GUARD;

    Vector3 v = Moo::rc().invView().applyVector(
        Moo::rc().camera().nearPlanePoint(
            (float(x) / Moo::rc().screenWidth()) * 2.f - 1.f,
            1.f - (float(y) / Moo::rc().screenHeight()) * 2.f ) );
    v.normalise();

    return v;
}

/*
void WorldManager::pushTool( ToolPtr tool )
{
    tools_.push( tool );
}


void WorldManager::popTool()
{
    if ( !tools_.empty() )
        tools_.pop();
}


ToolPtr WorldManager::tool()
{
    if ( !tools_.empty() )
        return tools_.top();

    return NULL;
}
*/


void WorldManager::addCommentaryMsg( const BW::string& msg, int id )
{
    BW_GUARD;

    Commentary::instance().addMsg( msg, id );
}


void WorldManager::addError(Chunk* chunk, ChunkItem* item, const char * format, ...)
{
    BW_GUARD;

    va_list argPtr;
    va_start( argPtr, format );

    char buffer[1024];
    bw_vsnprintf(buffer, sizeof(buffer), format, argPtr);

    // add to the gui errors pane
    if( &MsgHandler::instance() )
        MsgHandler::instance().addAssetErrorMessage( buffer, chunk, item );

    ASSET_MSG( buffer );
    // add to the view comments
    addCommentaryMsg( buffer, Commentary::CRITICAL );
}


void WorldManager::onDeleteVLO( const BW::string& id )
{
    VLOManager::instance()->markAsDeleted( id );
}


bool WorldManager::isChunkWritable( Chunk* pChunk ) const
{
    return EditorChunk::chunkWriteable( *pChunk );
}


bool WorldManager::isChunkEditable( Chunk* chunk ) const
{
    if (chunk->loaded() && EditorChunkCache::instance( *chunk ).edIsDeleted())
        return false;
    return EditorChunk::chunkWriteable( *chunk, false );
}


/**
*   This function append the chunks that the bb occupies
*   into the retChunks, but if the item is resident in
*   inside chunk, then we just return the resident chunk
*   @param bb               the bounding box of the item
*   @param pResidentChunk   the chunk that the item lives in
*   @param retChunks        out, returned chunks, it's not cleared but appended.
*/
void WorldManager::collectOccupiedChunks( const BoundingBox& bb,
                        Chunk* pResidentChunk,
                        BW::set<Chunk*>& retChunks )
{
    BW_GUARD;

    float gridSize = ChunkManager::instance().cameraSpace()->gridSize();
    if (!pResidentChunk || pResidentChunk->isOutsideChunk())
    {
        // Find all the chunks bb is in;
        // Sampling on every gridSize distance since the bounding box of a particle
        // can be large than chunk size
        for (float x = bb.minBounds().x; x < bb.maxBounds().x; x += gridSize)
        {
            for (float z = bb.minBounds().z; z < bb.maxBounds().z; z += gridSize)
            {
                retChunks.insert( 
                    EditorChunk::findOutsideChunk( Vector3( x, 0, z ) ) 
                    );
            }
        }

        // max z
        for (float x = bb.minBounds().x; x < bb.maxBounds().x; x += gridSize)
        {           
            retChunks.insert( 
                EditorChunk::findOutsideChunk( Vector3( x, 0, bb.maxBounds().z ) ) 
                );
        }

        // max x
        for (float z = bb.minBounds().z; z < bb.maxBounds().z; z += gridSize)
        {           
            retChunks.insert( 
                EditorChunk::findOutsideChunk( Vector3( bb.maxBounds().x, 0, z ) )
                );
        }

        // max x, max z
        retChunks.insert( 
            EditorChunk::findOutsideChunk( Vector3( bb.maxBounds().x, 0, bb.maxBounds().z ) )
            );

        // Remove the null chunk, if that got added
        retChunks.erase((Chunk*) 0);
    }
    else
    {
        // if the item intersect with neighbor chunks.
        for (ChunkBoundaries::iterator bit = pResidentChunk->joints().begin();
            bit != pResidentChunk->joints().end(); ++bit)
        {
            for (ChunkBoundary::Portals::iterator ppit = (*bit)->boundPortals_.begin();
                ppit != (*bit)->boundPortals_.end(); ++ppit)
            {
                ChunkBoundary::Portal* pit = *ppit;

                if (pit->hasChunk())
                {
                    if ( pit->pChunk->boundingBox().intersects( bb ) )
                    {
                        retChunks.insert( pit->pChunk );
                    }
                }
            }
        }

        // plus resident chunk
        retChunks.insert( pResidentChunk );
    }
}


/**
*   Update a list of chunks, this will be called when you did a change 
*   that's not caused by a chunk item,ie.: change terrain
*
*   @param primaryChunks    the chunks which have been changed.
*                               note: from the primary chunks, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param flags            the flags to notify which ChunkCaches to be updated
*                               only the ChunkCaches whose flag are included in flags 
*                               will be updated,if not provided, we only update thumbnail.
*/
void WorldManager::changedChunks( BW::set<Chunk*>& primaryChunks, 
                                 InvalidateFlags flags /*= InvalidateFlags::FLAG_THUMBNAIL*/ )
{
    BW_GUARD;

    for (BW::set<Chunk*>::iterator i = primaryChunks.begin(); i != primaryChunks.end(); ++i)
    {
        this->internalChangedChunk( *i, flags, NULL );
    }
}


/**
*   Update a list of chunks, this will be called when you changed a chunk item.
*   ie.: moved a model, deleted a tree.
*   @param primaryChunks    the chunks which have been changed.
*                               note: from the primary chunks, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param changedItem      the changed item that caused the chunk changed.
*/
void WorldManager::changedChunks( BW::set<Chunk*>& primaryChunks, 
                                 EditorChunkItem& changedItem )
{
    BW_GUARD;

    for (BW::set<Chunk*>::iterator i = primaryChunks.begin(); i != primaryChunks.end(); ++i)
    {
        this->internalChangedChunk( *i, 
            changedItem.edInvalidateFlags(), &changedItem );
    }
}



/**
*   Update a list of chunks, this will be called when you changed a chunk item,
*   but the invalidateFlags that you wish to update is different from the item's 
*   default invalidate flags, if flags == changedItem.edInvalidateFlags(), then use
*   changedChunks( BW::set<Chunk*>& chunks, EditorChunkItem& changedItem) instead.
*
*   @param primaryChunks    the chunks which have been changed.
*                               note: from the primary chunks, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param flags            the flags to notify which ChunkCaches to be updated
*                               only the ChunkCaches whose flag are included
*                               in flags will be updated
*   @param changedItem      the changed item that caused the chunk changed
*/
void WorldManager::changedChunks( BW::set<Chunk*>& primaryChunks, 
                                 InvalidateFlags flags,
                                 EditorChunkItem& changedItem )
{
    BW_GUARD;

    for (BW::set<Chunk*>::iterator i = primaryChunks.begin(); i != primaryChunks.end(); ++i)
    {
        this->internalChangedChunk( *i, flags, &changedItem );
    }
}






/**
*   This will be called when you did a change that's not caused by a chunk item,
*   ie.: change terrain
*   @param pPrimaryChunk    the chunk which has been changed.
*                               note: from the primary chunk, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param flags            the flags to notify which ChunkCaches to be updated
*                               only the ChunkCaches whose flag are included in flags
*                               will be updated, if not provided, we only update thumbnail.
*/
void WorldManager::changedChunk( Chunk * pPrimaryChunk, 
                                InvalidateFlags flags /*= InvalidateFlags::FLAG_THUMBNAIL*/ )
{
    BW_GUARD;

    this->internalChangedChunk( pPrimaryChunk, flags, NULL );
}



/**
*   This will be called when you changed a chunk item.
*   ie.: moved a model, deleted a tree.
*   @param pPrimaryChunk    the chunk which has been changed.
*                               note: from the primary chunk, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param changedItem  the changed item that caused the chunk changed.
*/
void WorldManager::changedChunk( Chunk * pPrimaryChunk, 
                                 EditorChunkItem& changedItem )
{
    BW_GUARD;

    this->internalChangedChunk( pPrimaryChunk, 
        changedItem.edInvalidateFlags(), &changedItem );
}



/**
*   This will be called when you changed a chunk item,
*   but the invalidateFlags that you wish to update is different from the item's 
*   default invalidate flags, if flags == changedItem.edInvalidateFlags(), then use
*   changedChunks( Chunk * pPrimaryChunk, EditorChunkItem& changedItem) instead.
*
*   @param pPrimaryChunk    the chunk which has been changed.
*                               note: from the primary chunk, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param flags            the flags to notify which ChunkCaches to be updated
*                               only the ChunkCaches whose flag are included in pFlags
*                               will be updated, if NULL, we only update thumbnail by default.
*   @param changedItem      the changed item that caused the chunk changed.
*/
void WorldManager::changedChunk( Chunk * pPrimaryChunk, 
                                InvalidateFlags flags,
                                EditorChunkItem& changedItem )
{
    BW_GUARD;

    this->internalChangedChunk( pPrimaryChunk, flags, &changedItem );
}

/**
*   internal function that doesn't do nextMark
*   @param pPrimaryChunk    the chunk which has been changed.
*                               note: from the primary chunk, the ChunkCache
*                               might spread out and find more chunks to be updated.
*   @param flags            the flags to notify which ChunkCaches to be updated
*                               only the ChunkCaches whose flag are included in pFlags
*                               will be updated, if NULL, we only update thumbnail by default.
*   @param pChangedItem the changed item that caused the chunk changed,
*                               NULL means it's other reason caused the chunk changed
*/
void WorldManager::internalChangedChunk( Chunk * pPrimaryChunk, 
                                InvalidateFlags flags,
                                EditorChunkItem* pChangedItem )
{
    BW_GUARD;

    MF_ASSERT( pPrimaryChunk );
    MF_ASSERT( pPrimaryChunk->loading() || pPrimaryChunk->loaded() );

    if (!EditorChunk::chunkWriteable( *pPrimaryChunk, false ))
    {
        ERROR_MSG( "Tried to mark non locked chuck %s as dirty\n", pPrimaryChunk->identifier().c_str() );
        return;
    }

    if (!Moo::isRenderThread() || !pPrimaryChunk->loaded())
    {
        SimpleMutexHolder smh( changeMutex_ );

        PendingChangedChunk chunk;
        chunk.pChunk_ = pPrimaryChunk;
        chunk.flags_ = flags;
        chunk.pChangedItem_ = pChangedItem;
        pendingChangedChunks_.push_back( chunk );

        return;
    }

    unsavedChunks_.add( pPrimaryChunk );

    for (int i = 0; i < ChunkCache::cacheNum(); ++i)
    {
        if (ChunkCache* cc = pPrimaryChunk->cache( i ))
        {
            cc->onChunkChanged( flags, this, pChangedItem );
        }
    }

    ClientSpacePtr cs = SpaceManager::instance().space( 
        pPrimaryChunk->space()->id() );
    if (cs)
    {
        ChangeSceneView* pView = cs->scene().getView<ChangeSceneView>();

        BoundingBox bb;
        if (pChangedItem)
        {
            pChangedItem->edWorldBounds( bb );
        }
        else
        {
            bb = pPrimaryChunk->boundingBox();
        }
        pView->notifyAreaChanged( bb );
    }
}


void WorldManager::lockChunkForEditing( Chunk * pChunk, bool editing )
{
    BW_GUARD;

    // We only care about outside chunks at the moment
    if ( !pChunk || !pChunk->isOutsideChunk() )
        return;

    const float gridSize = ChunkManager::instance().cameraSpace()->gridSize();
    for (float xpos = -MAX_TERRAIN_SHADOW_RANGE;
        xpos <= MAX_TERRAIN_SHADOW_RANGE;
        xpos += gridSize )
    {
        Vector3 pos = pChunk->centre() + Vector3( xpos, 0.f, 0.f );
        ChunkSpace::Column* col = ChunkManager::instance().cameraSpace()->column( pos, false );
        if (!col)
            continue;

        Chunk* c = col->pOutsideChunk();
        if (!c)
            continue;

        if ( editing )
        {
            // marking as editing, so insert and interrupt background
            // calculation if it's the current working chunk.
            chunksBeingEdited_.insert( c );
        }
        else
        {
            // erase it from the set so it can enter the background calculation
            // loop.
            BW::set<Chunk*>::iterator it =
                chunksBeingEdited_.find( c );
            if ( it != chunksBeingEdited_.end() )
                chunksBeingEdited_.erase( it );
        }
    }
}

void WorldManager::dirtyThumbnail( Chunk * pChunk, bool justLoaded /*= false*/ )
{
    BW_GUARD;

    if (!EditorChunk::chunkWriteable( *pChunk, false ))
    {
        return;
    }

    EditorChunkThumbnailCache::instance( *pChunk ).invalidate( this, false );


    lastModifyTime_ = GetTickCount();
}

void WorldManager::resetChangedLists()
{
    BW_GUARD;

    dirtyChunkLists_.clear();
    unsavedChunks_.clear();
    unsavedTerrainBlocks_.clear();
    unsavedCdatas_.clear();
    chunksBeingEdited_.clear();

    {
        SimpleMutexHolder smh( changeMutex_ );
        pendingChangedChunks_.clear();
    }

    VLOManager::instance()->clearLists();
}


void WorldManager::stopBackgroundCalculation()
{
    BW_GUARD;

    int numRunningThreads = ChunkProcessorManager::numRunningThreads();

    WorldManager::allowChunkProcessorThreadsToRun( true );
    ChunkProcessorManager::stopAll();
    if (!uiBlocked_)
    {
        WorldManager::allowChunkProcessorThreadsToRun( false );
    }
    ChunkProcessorManager::startThreads(
        "WE BG Process Thread", numRunningThreads );
}


void WorldManager::doBackgroundUpdating()
{
    BW_GUARD;

    if (this->numBgTasksLeft() <
        (unsigned int)ChunkProcessorManager::numRunningThreads() * 2)
    {
        processBackgroundTask( Moo::rc().invView().applyToOrigin(),
            chunksBeingEdited_ );
    }
}


/** Write a file to disk and (optionally) add it to cvs */
bool WorldManager::saveAndAddChunkBase( const BW::string & resourceID,
    const SaveableObjectBase & saver, bool add, bool addAsBinary )
{
    BW_GUARD;

    // add it before saving for if it has been cvs removed but not committed
    if (add)
    {
        CVSWrapper( currentSpace_ ).addFile( resourceID + ".cdata", addAsBinary, false );
        CVSWrapper( currentSpace_ ).addFile( resourceID + ".chunk", addAsBinary, false );
    }

    // save it out
    bool result = saver.save( resourceID );

    // add it again for if it has been ordinarily (re-)created
    if (add)
    {
        CVSWrapper( currentSpace_ ).addFile( resourceID + ".cdata", addAsBinary, false );
        CVSWrapper( currentSpace_ ).addFile( resourceID + ".chunk", addAsBinary, false );
    }

    return result;
}

/** Delete a file from disk and (eventually) remove it from cvs */
void WorldManager::eraseAndRemoveFile( const BW::string & resourceID )
{
    BW_GUARD;

    BW::string fileName = BWResource::resolveFilename( resourceID );
    BW::string backupFileName;
    if( fileName.size() > strlen( "i.chunk" ) &&
        strcmp( fileName.c_str() + fileName.size() - strlen( "i.chunk" ), "i.chunk" ) == 0 )
        backupFileName = fileName.substr( 0, fileName.size() - strlen( "i.chunk" ) ) + "i.~chunk~";
    if( !backupFileName.empty() && !BWResource::fileExists( backupFileName ) )
    {
        BW::wstring wfileName, wbackupFilename;
        bw_utf8tow( fileName, wfileName );
        bw_utf8tow( backupFileName, wbackupFilename );
        ::MoveFile( wfileName.c_str(), wbackupFilename.c_str() );
    }
    else
    {
        BW::wstring wfileName;
        bw_utf8tow( fileName, wfileName );
        ::DeleteFile( wfileName.c_str() );
    }

    CVSWrapper( currentSpace_ ).removeFile( fileName );
}


void WorldManager::changedTerrainBlock( Chunk* pChunk, bool collisionSceneChanged /*= true*/ )
{
    BW_GUARD;

    EditorChunkTerrain* pEct = static_cast<EditorChunkTerrain*>(
        ChunkTerrainCache::instance( *pChunk ).pTerrain());

    if (pEct)
    {
        unsavedTerrainBlocks_.add( &pEct->block() );

        //TODO : since we call changedChunk at the end of this method, we don't
        //need to call dirty thumbnail (changedChunk calls it).  remove
        dirtyThumbnail( pChunk );
    }

    InvalidateFlags flags( InvalidateFlags::FLAG_THUMBNAIL );
    if (collisionSceneChanged)
    {
        flags |= InvalidateFlags::FLAG_NAV_MESH;
        flags |= InvalidateFlags::FLAG_SHADOW_MAP;
    }
    this->changedChunk( pChunk, flags );
}


/*
 *  This method resaves all the terrain blocks in the space, handy for when the
 *  file format changes and the client does not support the same format.
 */
void WorldManager::resaveAllTerrainBlocks()
{
    BW_GUARD;

    // stop background chunk loading
    SyncMode chunkStopper;

    float steps = float((mapping_->maxLGridY() - mapping_->minLGridY() + 1) *
        (mapping_->maxLGridX() - mapping_->minLGridX() + 1));

    ProgressBarTask terrainTask( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RESAVE_TERRAIN"), 
        float(steps), false );

    const float gridSize = mapping_->pSpace()->gridSize();

    // Iterate over all chunks in the current space
    for (int32 z = mapping_->minLGridY(); z <= mapping_->maxLGridY(); z++)
    {
        for (int32 x = mapping_->minLGridX(); x <= mapping_->maxLGridX(); x++)
        {
            terrainTask.step();

            // Get the centre of the chunk so that we can get the chunk 
            // identifier
            float posX = float(x) * gridSize + gridSize * 0.5f;
            float posZ = float(z) * gridSize + gridSize * 0.5f;

            BW::string chunkIdentifier = 
                mapping_->outsideChunkIdentifier( Vector3(posX, 0.f, posZ ) );

            // See if the current chunk is in the chunk cache
            Chunk* pChunk = NULL;

            BW::set<Chunk*>::iterator it = EditorChunkCache::chunks_.begin();

            while (it != EditorChunkCache::chunks_.end())
            {
                if ((*it)->identifier() == chunkIdentifier)
                {
                    pChunk = *it;
                    break;
                }
                ++it;
            }

            // If the current chunk is in the chunk cache, set it as changed
            if (pChunk)
            {
                EditorChunkTerrainBase* pEct = EditorChunkTerrainCache::instance( *pChunk ).pTerrain();

                if (pEct)
                {
                    pEct->block().rebuildNormalMap(Terrain::NMQ_NICE);
                }

                this->changedTerrainBlock( pChunk );
            }
            else
            {
                Matrix xform = Matrix::identity;
                xform.setTranslate(mapping_->pSpace()->gridToPoint(x), 0.0f, mapping_->pSpace()->gridToPoint(z));

                // The current chunk is not in the chunk cache, load up the
                // terrain block and save it out again
                BW::string resourceName = mapping_->path() + 
                    chunkIdentifier + ".cdata/terrain";
                Terrain::EditorBaseTerrainBlockPtr pEtb = 
                    EditorChunkTerrain::loadBlock( resourceName, NULL,
                    xform.applyToOrigin() );

                if( pEtb )
                {
                    resourceName = mapping_->path() + chunkIdentifier + 
                        ".cdata";

                    DataSectionPtr pCDataSection = 
                        BWResource::openSection( resourceName );
                    if ( pCDataSection )
                    {
                        // We must save to a terrain2 section
                        DataSectionPtr dataSection = 
                            pCDataSection->openSection( pEtb->dataSectionName(), 
                                                        true );
                        if ( dataSection )
                        {
                            ChunkCleanFlags cf( pCDataSection );

                            cf.terrainLOD_ = pEtb->rebuildLodTexture( xform );
                            cf.save();

                            pEtb->rebuildNormalMap(Terrain::NMQ_NICE);
                            pEtb->save( dataSection );

                            pCDataSection->save();
                        }
                    }
                }
            }
        }
    }
    // Do a quick save to save out all the terrain blocks that are in memory
    quickSave();
}


void WorldManager::restitchAllTerrainBlocks()
{
    BW_GUARD;

    ChunkRowCache rowCache(1); // we do processing in 3x3 blocks

    float steps = float((mapping_->maxLGridY() - mapping_->minLGridY() + 1) *
        (mapping_->maxLGridX() - mapping_->minLGridX() + 1));

    ProgressBarTask terrainTask( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RE_STITCHING_ALL_TERRAIN_BLOCKS"), 
        float(steps), false );

    for (int32 z = mapping_->minLGridY(); z <= mapping_->maxLGridY(); z++)
    {
        rowCache.row(z);

        for (int32 x = mapping_->minLGridX(); x <= mapping_->maxLGridX(); x++)
        {
            // find chunk and neighbors
            BW::string chunkIdentifier[9];
            Chunk*      pChunk[9];
            uint32      ci = 0;

            for ( int32 i = -1; i <= 1; i++ )
            {
                for ( int32 j = -1; j <= 1; j++ )
                {
                    int16 wx = (int16)(x + j);
                    int16 wz = (int16)(z + i);
                    chunkID(chunkIdentifier[ci], wx, wz);
                    pChunk[ci] =
                        ChunkManager::instance().findChunkByName
                        (
                            chunkIdentifier[ci],
                            mapping_
                        );
                    ++ci;
                }
            }

            // we were trying to do a chunk that isn't in space
            if ( pChunk[4] == NULL )
            {
                continue;
            }

            EditorChunkTerrain* pEct = 
                (EditorChunkTerrain*)EditorChunkTerrainCache::instance( *pChunk[4] ).pTerrain();
            MF_ASSERT( pEct );

            pEct->onEditHeights();

            // Save:
            Terrain::EditorBaseTerrainBlock & pTerrain = pEct->block();
            BW::string resourceName = mapping_->path() 
                + chunkIdentifier[4] + ".cdata";
            pTerrain.save(resourceName);

            // update progress bar
            terrainTask.step();
        }
    }
}

void WorldManager::createSpaceImage()
{
    BW_GUARD;
    TakePhotoDlg dlg;

    dlg.setSpaceInfo( currentSpace_, 0, 0,
        (int)this->getTerrainInfo().widthM,
        mapping_->maxLGridY() - mapping_->minLGridY() + 1,
        mapping_->maxLGridX() - mapping_->minLGridX() + 1 );

    bool result = ( dlg.DoModal() == IDOK );
    if( result )
    {
        if (!canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/TAKEPHOTOS") ))
        {
            return;
        }
        BW::string targetPath    = BWResource::resolveFilename( mapping_->path() ) + "space_viewer/";
        TakePhotoDlg::PhotoInfo photoInfo = dlg.getPhotoInfo();
        
        this->createSpaceImageInternal( targetPath, photoInfo );
    }
}

void WorldManager::createSpaceImageInternal( const BW::string& targetPath, 
                    const TakePhotoDlg::PhotoInfo& photoInfo )
{
    BW_GUARD;

    uint64 startTime = timestamp();
    // Stop background chunk loading:
    SyncMode chunkStopper;
    bool result = false;

    this->unloadChunks();
    float pixelsPerMetre = photoInfo.pixelsPerMetre;
    bool prevLodTextureSetting = this->pTerrainSettings()->useLodTexture();
    this->pTerrainSettings()->useLodTexture( false );

    int imgSize = 0;
    bool cancelled = false;
    float steps = 1.0f * 
                ( mapping_->maxLGridY() - mapping_->minLGridY() + 1 ) * 
                ( mapping_->maxLGridX() - mapping_->minLGridX() + 1 );

    ScopedProgressBar helper(2);
    {
        ProgressBarTask createSpaceImageProgress(
            LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CREATING_SPACE_IMAGE" ),
            steps, true );
        NearbyChunkLoader ncManager( mapping_->minLGridX(), mapping_->maxLGridX(), mapping_->minLGridY(), mapping_->maxLGridY() );

        for (int32 z = mapping_->minLGridY(); z <= mapping_->maxLGridY() && !createSpaceImageProgress.isCancelled(); ++z)
        {
            if (Moo::rc().device()->TestCooperativeLevel() != D3D_OK)
            {
                ERROR_MSG( "Device is lost, regenerating thumbnails has been stopped\n" );
                return;
            }

            for (int32 x = mapping_->minLGridX(); 
                x <= mapping_->maxLGridX() && !createSpaceImageProgress.isCancelled(); ++x)
            {
                BW::string chunkName;
                chunkID( chunkName, (int16)x, (int16)z );
                int photoPosY = x - mapping_->minLGridX();
                int photoPosX = mapping_->maxLGridY() - z;

                BW::set<Chunk*> locallyModifiedChunks;
                ncManager.loadChunks( x, z );
                if (!this->takePhotoOfChunk( chunkName, targetPath, photoPosX, photoPosY, pixelsPerMetre, imgSize ))
                {
                    ERROR_MSG( "Failed to take photo for chunk '%s'.\n",
                        chunkName.c_str() );
                }
                ncManager.releaseUnusedChunks( locallyModifiedChunks );
                unsavedChunks_.filterWithoutSaving( locallyModifiedChunks );
                unsavedCdatas_.filterWithoutSaving( locallyModifiedChunks );
                createSpaceImageProgress.step();
            }
        }

        cancelled = createSpaceImageProgress.isCancelled();
    }
    this->pTerrainSettings()->useLodTexture( prevLodTextureSetting );
    if ( !cancelled )
    {
        if ( photoInfo.mode == TakePhotoDlg::LEVEL_MAP )
        {
            result = WorldEditorApp::instance().pythonAdapter()->createSpaceLevelImage( targetPath, imgSize,
                mapping_->maxLGridY() - mapping_->minLGridY() + 1,
                mapping_->maxLGridX() - mapping_->minLGridX() + 1,
                pixelsPerMetre, mapping_->minLGridX(), mapping_->maxLGridY(), (int)this->getTerrainInfo().widthM );
        }
        else if ( photoInfo.mode == TakePhotoDlg::SINGLE_MAP )
        {
            result = WorldEditorApp::instance().pythonAdapter()->createSpaceSingleMap( targetPath, imgSize,
                mapping_->maxLGridX() - mapping_->minLGridX() + 1,
                mapping_->maxLGridY() - mapping_->minLGridY() + 1, photoInfo.spaceMapWidth, photoInfo.spaceMapHeight );
        }
    }
    
    if ( result )
    {
        this->addCommentaryMsg( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SPACE_IMAGE_CREATED", targetPath ) );
    }
    else
    {
        this->addCommentaryMsg( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SPACE_IMAGE_CREAT_FAILED" ) );
    }

    //DEBUG_MSG("create space image finished, time spent: %d\n", (timestamp() - startTime) * 1000 / stampsPerSecond());
}

/**
 *  This function goes through all chunks, both loaded and unloaded, and
 *  recalculates the thumbnails and saves them directly to disk.  Chunks
 *  which were unloaded are ejected when it finishes with them, so large
 *  spaces can be regenerated.  The downside is that there is no undo/redo and
 *  the .cdata files are modified directly.  It also assumes that the shadow
 *  data is up to date.
 *
 *  This function also deletes the time stamps and dds files.
 */
void WorldManager::regenerateThumbnailsOffline()
{
    BW_GUARD;

    // Stop background chunk loading:
    SyncMode chunkStopper;

    // Remove the time stamps and space*thumbnail dds files:
    BW::string spacePath    = mapping_->path();
    BW::string timeStamps   = spacePath + "space.thumbnail.timestamps";
    BW::string spaceDDS     = spacePath + "space.thumbnail.dds";
    BW::string spaceTempDDS = spacePath + "space.temp_thumbnail.dds";
    timeStamps      = BWResource::resolveFilename(timeStamps  );
    spaceDDS        = BWResource::resolveFilename(spaceDDS    );
    spaceTempDDS    = BWResource::resolveFilename(spaceTempDDS);
    ::unlink(timeStamps  .c_str());
    ::unlink(spaceDDS    .c_str());
    ::unlink(spaceTempDDS.c_str());


    float steps =
        (mapping_->maxLGridY() - mapping_->minLGridY() + 1.0f) *
        (mapping_->maxLGridX() - mapping_->minLGridX() + 1.0f);

    ScopedProgressBar progressTaskHelper( 5 );
    ProgressBarTask thumbProgress(
        LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/REGENERATING_THUMBNAILS" ),
        steps, true );

    for (int32 z = mapping_->minLGridY(); 
        z <= mapping_->maxLGridY() && !thumbProgress.isCancelled(); ++z)
    {
        if (Moo::rc().device()->TestCooperativeLevel() != D3D_OK)
        {
            ERROR_MSG( "Device is lost, regenerating thumbnails has been stopped\n" );
            return;
        }

        for (int32 x = mapping_->minLGridX(); 
            x <= mapping_->maxLGridX() && !thumbProgress.isCancelled(); ++x)
        {
            BW::string chunkName;
            chunkID( chunkName, (int16)x, (int16)z );

            if (!this->recalcThumbnail( chunkName, true ))
            {
                ERROR_MSG( "Failed to regenerate thumbnail for chunk '%s'.\n",
                    chunkName.c_str() );
            }

            thumbProgress.step();
        }
    }


    ProjectModule::regenerateAllDirty();

    this->quickSave();
}


/**
 *  This function converts the current space to use zip sections.
 */
void WorldManager::convertSpaceToZip()
{
    BW_GUARD;

    // Prompt if the world has been modified:
    if( !canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CONVERT") ) )
        return;

    // Stop background chunk loading, clear the DataSection cache:
    SyncMode chunkStopper;
    BWResource::instance().purgeAll(); 

    // Find all .cdata files:
    BW::vector< BW::wstring > cdataFiles;
    BW::wstring spacePath;
    bw_utf8tow( BWResource::resolveFilename( mapping_->path() ), spacePath );   
    BW::wstring cdataFilesRE = spacePath + L"*.cdata";  
    WIN32_FIND_DATA fileInfo;
    HANDLE findResult = ::FindFirstFile( cdataFilesRE.c_str(), &fileInfo );
    if (findResult != INVALID_HANDLE_VALUE)
    {
        do
        {
            BW::wstring cdataFullFile = spacePath + fileInfo.cFileName;
            cdataFiles.push_back( cdataFullFile );
        }
        while (::FindNextFile( findResult, &fileInfo ) != FALSE);
        ::FindClose( findResult );
    }

    // Convert the .cdata files to ZipSections:
    ProgressBarTask 
        zipProgress
        ( 
            LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CONVERTING_TO_ZIP"), 
            (float)cdataFiles.size(), false
        );
    for
    (
        BW::vector< BW::wstring >::const_iterator it = cdataFiles.begin();
        it != cdataFiles.end();
        ++it
    )
    {
        BW::string nfilename;
        bw_wtoutf8( (*it), nfilename );
        BW::string    cdataFile   = BWResource::dissolveFilename( nfilename );
        DataSectionPtr dataSection = BWResource::openSection( cdataFile );
        if (dataSection)
        {
            dataSection = dataSection->convertToZip();
            dataSection->save();
        }
        zipProgress.step();
    }

    // Force the space to reload:
    reloadAllChunks( false );
}


/**
 *  This function goes through all chunks, both loaded and unloaded, and
 *  recalculates the terrain LODs and saves them directly to disk.  Chunks
 *  which were unloaded are ejected when it finishes with them, so large
 *  spaces can be regenerated.  The downside is that there is no undo/redo and
 *  the .cdata files are modified directly.
 */
void WorldManager::regenerateLODsOffline()
{
    BW_GUARD;

    // Stop background chunk loading:
    SyncMode chunkStopper;
    ScopedProgressBar progressTaskHelper( 4 );

    // stop background processing to prevent simultaneous read/write
    // access to collision data
    int numRunningThreads = ChunkProcessorManager::numRunningThreads();
    WorldManager::allowChunkProcessorThreadsToRun( true );
    ChunkProcessorManager::stopAll();
    WorldManager::allowChunkProcessorThreadsToRun( false );

    float steps = 
        (mapping_->maxLGridY() - mapping_->minLGridY() + 1.0f)
        *
        (mapping_->maxLGridX() - mapping_->minLGridX() + 1.0f);
    ProgressBarTask* thumbProgress = new ProgressBarTask( 
        LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/REGENERATING_LODS"), steps, true );

    BW::string space = this->getCurrentSpace();

    bool success = true;

    for (int32 z = mapping_->minLGridY(); 
        z <= mapping_->maxLGridY() && !thumbProgress->isCancelled(); ++z)
    {
        if (Moo::rc().device()->TestCooperativeLevel() != D3D_OK)
        {
            ERROR_MSG("Device is lost, regenerating LODs has been stopped\n");
            success = false;
            break;
        }

        for (int32 x = mapping_->minLGridX(); 
            x <= mapping_->maxLGridX() && !thumbProgress->isCancelled(); ++x)
        {
            CdataSaver cdataSaver;
            UnsavedChunks locallyModifiedCdatas;
            UnsavedTerrainBlocks locallyModifiedTerrainBlocks;

            // Get the chunk's name:
            BW::string chunkName;
            chunkID(chunkName, (int16)x, (int16)z);

            // Get the chunk:
            Chunk *chunk = ChunkManager::instance().findChunkByName(
                                                    chunkName,
                                                    this->geometryMapping() );

            if (chunk != NULL)
            {
                // Is the chunk in memory yet?  We use this below to unload
                // chunks which weren't in memory:
                bool inMemory = chunk->loaded();

                // Force to memory:
                if (!inMemory)
                {
                    ChunkManager::instance().loadChunkNow
                    (
                        chunkName,
                        geometryMapping()
                    );
                    ChunkManager::instance().checkLoadingChunks();
                }

                // Re-LOD the chunk and generate the dominant texture layers:
                bool ok = false;
                EditorChunkTerrainLodCache& eclt = 
                    EditorChunkTerrainLodCache::instance( *chunk );

                EditorChunkTerrainBase* ect = eclt.pTerrain();
                if (ect == NULL)
                {
                    continue;
                }
                MF_ASSERT( ect->chunk() == chunk );

                if (eclt.dirty())
                {
                    Terrain::EditorBaseTerrainBlock& block = ect->block();
                    block.rebuildCombinedLayers();
                    ok = eclt.recalc( this, *this );
                }

                // If we forced the chunk to memory then remove it from memory:
                if (!inMemory)
                {
                    if (ok)
                    {
                        locallyModifiedTerrainBlocks.add( &ect->block() );
                        locallyModifiedCdatas.add( chunk );

                        locallyModifiedTerrainBlocks.save();
                        // UnsavedChunks::save attempts to unload saved chunks
                        locallyModifiedCdatas.save( cdataSaver );
                    }
                    else if (chunk->removable())
                    {
                        chunk->unbind( false );
                        chunk->unload();
                    }
                }
                else
                {
                    changedTerrainBlock( chunk, false );
                    eclt.invalidate( this, false );
                }
            }

            // This needs to be done before processMessagesForTask()
            // as WorldManager::update() will try to mark
            // the chunks we saved and unloaded earlier.
            unsavedTerrainBlocks_.filter( locallyModifiedTerrainBlocks );
            unsavedCdatas_.filter( locallyModifiedCdatas );

            // Update the progress indicator:
            thumbProgress->step();
        }
    }

    bool wasEscaped = thumbProgress->isCancelled();

    bw_safe_delete( thumbProgress );
    // Save:
    if (success && !wasEscaped)
    {
        this->quickSave();
    }

    ChunkProcessorManager::startThreads( "WE BG Process Thread",
        numRunningThreads );
}


void WorldManager::chunkShadowUpdated( Chunk * pChunk )
{
    BW_GUARD;

    dirtyThumbnail( pChunk );

    if (pChunk->isOutsideChunk())
    {
        int16 x, z;
        geometryMapping()->gridFromChunkName( pChunk->identifier(), x, z);
        chunkWatcher_->onCalcShadow(x, z);
        chunkWatcher_->canUnload(x, z, pChunk->removable());
        if (PanelManager::pInstance())
        {
            PanelManager::instance().onChangedChunkState( x, z );
        }
    }

    changedChunk( pChunk, InvalidateFlags::FLAG_NONE );
}


/**
 *  Call this method when a thumbnail has now been generated for a chunk.
 */
void WorldManager::chunkThumbnailUpdated( Chunk* pChunk )
{
    BW_GUARD;

    MF_ASSERT( pChunk );

    ChunkNotWritableReason reason;
    if (!EditorChunk::chunkWriteable( *pChunk, false, &reason ))
    {
        if (reason != REASON_NOTLOCKEDBYME)
        {
            ERROR_MSG( "Tried to mark read-only chunk %s as dirty\n", pChunk->identifier().c_str() );
        }
        return;
    }

    unsavedCdatas_.add( pChunk );

    SpaceMap::instance().chunkThumbnailUpdated( pChunk );

    changedChunk( pChunk, InvalidateFlags::FLAG_NONE );
}


/**
 * Save changed terrain and chunk files, without recalculating anything
 */
bool WorldManager::saveChangedFiles()
{
    BW_GUARD;

    DataSectionCache::instance()->clear();
    DataSectionCensus::clear();
    bool errors = false;

    SyncMode chunkStopper;

    ScopedProgressBar progressTaskHelper( 4 );
    PanelManager::instance().onBeginSave();

    // Rebuild missing terrain texture LODs.  Complain if any cannot be
    // rebuilt, do all of them, add to the list of changed chunks and show the
    // progress bar when doing this.
    drawMissingTextureLODs(true, true, true, true);

    // Save terrain chunks
    {
        ProgressBarTask terrainTask(
            LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SAVE_TERRAIN"), 0, false );

        unsavedTerrainBlocks_.save( &terrainTask );
    }
    

    // Save all dirty chunks touched by VLOD editing.
    VLOManager::instance()->saveDirtyChunks();

    {
        ProgressBarTask chunkTask(
        LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SAVE_SCENE_DATA"), 0, false );


    WEChunkSaver  chunkSaver;
    WEChunkLoader chunkLoader( geometryMapping() );

    markChunks();

    unsavedChunks_.save( chunkSaver, &chunkTask, &chunkLoader );
    }
    

    // Update the "original VLO bounds" list with the new transform, and mark
    // VLOs that were deleted for later cleanup.
    VLOManager::instance()->postSave();

    VeryLargeObject::saveAll();

    BW::string space = this->getCurrentSpace();
    BW::string spaceSettingsFile = space + "/" + SPACE_SETTING_FILE_NAME;
    DataSectionPtr  pDS = BWResource::openSection( spaceSettingsFile  );    
    if (pDS)
    {       
        // check if navmeshGenerator tag isn't set
        if (pDS->readString( "navmeshGenerator", "" ) == "")
        {
            // default to navgen
            pDS->writeString( "navmeshGenerator", "navgen" );
        }

        if (romp_ != NULL)
            romp_->enviroMinder().save(pDS);            

        if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
        {
            Vector2 fadeRange = DecalsManager::instance().getFadeRange();
            pDS->writeFloat( "decals/fadeStart", fadeRange.x);
            pDS->writeFloat( "decals/fadeEnd", fadeRange.y);
        }

        DataSectionPtr terrainSection = pDS->openSection("terrain");
        if (terrainSection != NULL)
            pTerrainSettings()->save(terrainSection);

        // Personality script.  Allow scripts to save any files they've modified,
        // and allow them to save the space.settings file too.
        ScriptObject ds = ScriptObject( new PyDataSection( pDS ),
            ScriptObject::FROM_NEW_REFERENCE );
        
        Personality::instance().callMethod( "onSave",
            ScriptArgs::create( space, ds ),
            ScriptErrorPrint( "Personality onSave: " ),
            /* allowNullMethod */ true );

        pDS->save( spaceSettingsFile );
        changedEnvironment_ = false;
    }

    unsavedCdatas_.filter( unsavedChunks_ );

    {
        ProgressBarTask thumbnailTask(  LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SAVE_THUMBNAIL_DATA"), 100, false );
        if ( !unsavedCdatas_.empty() )
        {
            CdataSaver cdataSaver;
            unsavedCdatas_.save( cdataSaver, &thumbnailTask );
    
    
    
    
        }
    
        StationGraph::saveAll();
        SpaceMap::instance().save();
    }

    if ( !errors )
    {
        // clear dirty lists only if no errors were generated
        unsavedTerrainBlocks_.clear();
        unsavedCdatas_.clear();
        unsavedChunks_.clear();

        VLOManager::instance()->clearDirtyList();
    }

    PanelManager::instance().onEndSave();

    writeCleanList();

    return !errors;
}


/**
 *  This method is called whenever a chunk is bound
 *  Called from Chunk::bind right before setting Chunk::bound()
 *  Also called from internal Chunk::bind( Chunk* ) on the other
 *  end of a binding, Chunk::bindPortals, and Chunk::formPortal
 */
void WorldManager::onLoadChunk( Chunk * pChunk )
{
    BW_GUARD;

    MF_ASSERT( pChunk );
    MF_ASSERT( pChunk->loaded() );
    if (pChunk->isBound())
    {
        // Call from somewhere other than Chunk::bind
//      DEBUG_MSG( "WorldManager::onLoadChunk( %s ): Already bound\n",
//          pChunk->identifier().c_str() );
        return;
    }

    VLOManager::instance()->updateChunkReferences( pChunk );

    // Inform the link manager that the chunk has been loaded so that it can
    // update any relevant links
    EditorChunkLinkManager::instance().chunkLoaded( pChunk );

    // broadcast that the chunk is loaded:
    if (pChunk->isOutsideChunk())
    {
        int16 x, z;
        geometryMapping()->gridFromChunkName( pChunk->identifier(), x, z);
        chunkWatcher_->onLoadChunk(x, z);
        chunkWatcher_->canUnload(x, z, pChunk->removable());
        if (PanelManager::pInstance())
        {
            PanelManager::instance().onChangedChunkState( x, z );
        }
    }

    this->updateChunkDirtyStatus( pChunk );


    EditorChunkTerrainBase* pEct = EditorChunkTerrainCache::instance( *pChunk ).pTerrain();
    if ( pEct )
    {
        Terrain::EditorBaseTerrainBlock const &b = pEct->block();
        for (size_t i = 0; i < b.numberTextureLayers(); ++i)
        {
            Terrain::TerrainTextureLayer const &ttl = b.textureLayer(i);
            if (!TerrainUtils::sourceTextureHasAlpha( ttl.textureName() ))
            {
                BW::string warningMsg = LocaliseUTF8( L"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/TEXTURE_WITHOUT_ALPHA", ttl.textureName() );
                WorldManager::instance().addCommentaryMsg( warningMsg, Commentary::WARNING );
                WARNING_MSG( "%s\n", warningMsg.c_str() );
            }
            
        }

        if (pChunk->isOutsideChunk() && !terrainFormatStorage_.hasTerrainInfo())
        {
            terrainFormatStorage_.setTerrainInfo( pEct->ChunkTerrain::block() );
        }
    }
}


/**
 *  This method is called whenever a chunk is unbound
 *  Called from Chunk::unbind right before clearing Chunk::bound()
 *  Also called from internal Chunk::unbind( Chunk*, bool ) on
 *  the other end of an unbinding.
 */
void WorldManager::onUnloadChunk(Chunk *pChunk)
{
    BW_GUARD;

    MF_ASSERT( pChunk );
    MF_ASSERT( pChunk->isBound() );

    // Stop unless we're now completely adrift.
    for (ChunkBoundaries::const_iterator bit = pChunk->joints().begin();
        bit != pChunk->joints().end();
        bit++)
    {
        for (uint i=0; i < (*bit)->boundPortals_.size(); i++)
        {
            ChunkBoundary::Portal *& pPortal = (*bit)->boundPortals_[i];
            if (pPortal->hasChunk())
            {
//              DEBUG_MSG( "WorldManager::onUnloadChunk( %s ): Still bound\n",
//                  pChunk->identifier().c_str() );
                return;
            }
        }
    }

    // Inform the link manager that the chunk is being tossed so that it can
    // update any relevant links
    EditorChunkLinkManager::instance().chunkTossed( pChunk );

    // broadcast that the chunk is unloaded:
    if (pChunk->isOutsideChunk())
    {
        int16 x, z;
        geometryMapping()->gridFromChunkName( pChunk->identifier(), x, z);
        chunkWatcher_->onUnloadChunk(x, z);
        if (PanelManager::pInstance())
        {
            PanelManager::instance().onChangedChunkState( x, z );
        }
    }

    this->cleanChunkDirtyStatus( pChunk );
}

namespace
{
    BW::string getPythonStackTrace()
    {
        BW_GUARD;

        BW::string stack = "";

        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch( &ptype, &pvalue, &ptraceback);

        if (ptraceback != NULL)
        {
            // use traceback.format_exception 
            // to get stacktrace as a string
            PyObject* pModule = PyImport_ImportModule( "traceback" );
            if (pModule != NULL)
            {
                PyObject * formatFunction = PyObject_GetAttrString( pModule, "format_exception" );

                if (formatFunction != NULL)
                {
                    PyObject * list = Script::ask( 
                            formatFunction, Py_BuildValue( "(OOO)", ptype, pvalue, ptraceback ), 
                            "WorldEditor", false, false );

                    if (list != NULL)
                    {
                        for (int i=0; i < PyList_Size( list ); ++i)
                        {
                            stack += PyString_AS_STRING( PyList_GetItem( list, i ) );
                        }
                        Py_DECREF( list );
                    }
                }
                Py_DECREF( pModule );
            }
        }

        // restore error so that PyErr_Print still sends
        // traceback to console (PyErr_Fetch clears it)
        PyErr_Restore( ptype, pvalue, ptraceback);

        return stack;
    }



}


bool WorldManager::checkForReadOnly() const
{
    BW_GUARD;

    bool readOnly = Options::getOptionInt("objects/readOnlyMode", 0) != 0;
    if( readOnly )
    {
        MessageBox( *WorldEditorApp::instance().mainWnd(),
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/READ_ONLY_WARN_TEXT"),
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/READ_ONLY_WARN_TITLE"),
            MB_OK );
    }
    return readOnly;
}
/**
 * Only save changed chunk and terrain data, don't recalculate anything.
 *
 * Dirty lists are persisted to disk.
 */
void WorldManager::quickSave()
{
    BW_GUARD;

    // Prevent re-entry
    ScopedReentryAssert scopedReentryAssert;

#if ENABLE_ASSET_PIPE
    AssetLock assetLock;
#endif // ENABLE_ASSET_PIPE

    saveFailed_ = false;

    if (checkForReadOnly())
    {
        return;
    }

    this->cleanupTools();

    bool errors = false;

    // Announce that we are inside the quickSave function. This was introduced
    // because the call to UpdateWindow later could cause another call to 
    // quickSave. This happens if it is detected we are running out
    // of memory and need to quickSave so as not to lose data.
    insideQuickSave_ = true;

    Commentary::instance().addMsg( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/QUICK_SAVING"), 1 );

    if (!saveChangedFiles())
    {
        errors = true;
    }

    if ( errors )
    {
        MessageBox( *WorldEditorApp::instance().mainWnd(),
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/QUICK_SAVE_ERROR_TEXT"),
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/QUICK_SAVE_ERROR_TITLE"),
            MB_ICONERROR );
        addError( NULL, NULL, LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/QUICK_SAVE_ERROR_TEXT").c_str() );
    }
    else
    {
        Commentary::instance().addMsg( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/QUICK_SAVE_ERROR_COMPLETE"), 1 );
    }
    
    MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
    mainFrame->InvalidateRect( NULL );
    mainFrame->UpdateWindow();

    saveFailed_ = errors;
    insideQuickSave_ = false;
}


void updateDirtyChunks( WorldManager::ChunkSet& chunks, const DirtyChunkList& dcl )
{
    for (DirtyChunkList::const_iterator iter = dcl.begin();
        iter != dcl.end(); ++iter)
    {
        chunks.insert( iter->second->identifier() );
    }
}


void updateDirtyChunks( WorldManager::ChunkSet& chunks, const DirtyChunkLists& dcl )
{
    for (size_t i = 0; i < dcl.size(); ++i)
    {
        updateDirtyChunks( chunks, dcl[ i ] );
    }
}


void WorldManager::tickSavingChunks()
{
    BW_GUARD;

    BgTaskManager::instance().tick();
    FileIOTaskManager::instance().tick();
    AmortiseChunkItemDelete::instance().tick();

    this->postPendingErrorMessages();

    if (this->isMemoryLow( true ))
    {
        UndoRedo::instance().clear();
        INFO_MSG( "Process Data: Memory is running low, clearing the Undo/Redo stack.\n" );
    }

    this->writeStatus();

    if (MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd())
    {
        mainFrame->updateStatusBar();
    }
}


void WorldManager::save()
{
    BW_GUARD;

    // Prevent re-entry
    ScopedReentryAssert scopedReentryAssert;

#if ENABLE_ASSET_PIPE
    AssetLock assetLock;
#endif // ENABLE_ASSET_PIPE

    this->cleanupTools();

    MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();

    saveChangedFiles();

    saveFailed_ = false;

    if( checkForReadOnly() )
    {
        return;
    }

    if (Options::getOptionBool( "fullSave/warnUndoOnProcessData", true ))
    {
        // Warn the user that Process Data might clear the undo redo history.
        UndoWarnDlg undoWarn;
        INT_PTR result = undoWarn.DoModal();
        if (result == IDOK)
        {
            // Continue even if the Undo/Redo history is lost.
            Options::setOptionBool( "fullSave/warnUndoOnProcessData", !undoWarn.dontRepeat() );
        }
        else if (result == IDCANCEL)
        {
            // User doesn't want to lose the undo/redo stack.
            endModalOperation();
            return;
        }
        else
        {
            // Something else happened, this is probably a coding error.
            ERROR_MSG( "Failed to display the UndoWarnDlg on save.\n" );
        }
    }

    ScopedProgressBar progressTaskHelper( 6 );
    bool errors = false;

    stopBackgroundCalculation();
    allowChunkProcessorThreadsToRun( true );

    // We're about to load every chunk in the space, so kick any existing
    // loaded chunks out to avoid memory fragmentation
#ifndef _WIN64
    unloadChunks();
#endif

    Commentary::instance().addMsg( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SAVING"), 1 );

    ChunkSet dirtyChunks;

    GeometryMapping* dirMap = geometryMapping();

    {
        BW::string syncCleanList = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SYNC_CLEAN_LIST");
        ProgressBarTask syncTask(syncCleanList, 1.f, false );

        cleanChunkList_->sync( &syncTask );
    }

    ChunkSet cs = dirMap->gatherChunks();

    for (ChunkSet::iterator iter = cs.begin(); iter != cs.end(); ++iter)
    {
        //if (!cleanChunkList_->isClean( *iter ))
        {
            dirtyChunks.insert( *iter );
        }
    }

    {
        RecursiveMutexHolder lock( dirtyChunkListsMutex_ );
        updateDirtyChunks( dirtyChunks, this->dirtyChunkLists() );
    }

    BW::string recalcShadow = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECALC_SHADOW");
    float taskTotal = float( dirtyChunks.size() );
    float taskProgress = 0.f;
    ProgressBarTask* savingTask = new ProgressBarTask( recalcShadow, taskTotal, true );

    savingTask->set( taskProgress );

    // Use lowest texture quality to save memory during process data
    Moo::GraphicsSetting::GraphicsSettingPtr pTextureQuality = Moo::GraphicsSetting::getFromLabel( "TEXTURE_QUALITY" );
    int oldTextureQuality = 0;
    if (pTextureQuality)
    {
        oldTextureQuality = pTextureQuality->activeOption();
        pTextureQuality->selectOption( 2, true );
    }

    CpuInfo cpuInfo;
    this->startThreads(
        "WE Saving Thread",
        cpuInfo.numberOfSystemCores() );
    disableClose();

    ChunkSaver chunkSaver;
    CdataSaver cdataSaver;

    saveChunks( dirtyChunks, chunkSaver, cdataSaver, savingTask, true );
    bw_safe_delete( savingTask );

    // clear the pendingChangedChunks_ list as chunks may have been added to it during processing
    {
        SimpleMutexHolder smh( changeMutex_ );
        pendingChangedChunks_.clear();
    }

    enableClose();

    stopThreads( cpuInfo.numberOfSystemCores() );

    // Get the project module to update the dirty chunks.
    ProjectModule::regenerateAllDirty();

    // Get the terrain height import/export module to save its height map.
    HeightModule::ensureHeightMapCalculated();

    if (!saveChangedFiles())
    {
        errors = true;
    }

    // Regenerate the cached Forced LOD values from the file.
    forcedLodMap_.clearLODData();
    forcedLodMap_.loadLODDataFromFiles();

    if (errors)
    {
        if (mainFrame)
        {
            MessageBox( *mainFrame,
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/FULL_SAVE_ERROR_TEXT"),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/FULL_SAVE_ERROR_TITLE"),
                MB_ICONERROR );
        }

        addError( NULL, NULL, LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/FULL_SAVE_ERROR_TEXT").c_str() );
    }
    else
    {
        Commentary::instance().addMsg( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SAVE_COMPLETE"), 1 );
    }

    // restore texture quality
    if (pTextureQuality)
    {
        pTextureQuality->selectOption( oldTextureQuality, true );
    }

    // We've probably got a ton of chunks loaded we don't want anymore, so
    // unload all loaded chunks to avoid memory fragmentation
#ifndef _WIN64
    unloadChunks();
#endif

    if (mainFrame)
    {
        mainFrame->InvalidateRect( NULL );
        mainFrame->UpdateWindow();
    }

    allowChunkProcessorThreadsToRun( false );
    endModalOperation();
    saveFailed_ = errors;
}


float WorldManager::farPlane() const
{
    BW_GUARD;

    Moo::Camera camera = Moo::rc().camera();
    return camera.farPlane();
}


void WorldManager::farPlane( float f )
{
    BW_GUARD;

    float farPlaneDistance = Math::clamp( 0.0f, f, getMaxFarPlane() );

    Moo::Camera camera = Moo::rc().camera();
    camera.farPlane( farPlaneDistance );
    Moo::rc().camera( camera );

    // mark only things within the far plane as candidates for loading
    ChunkManager::instance().autoSetPathConstraints( farPlaneDistance );
}

bool WorldManager::isItemSelected( ChunkItemPtr item ) const
{
    BW_GUARD;

    for (BW::vector<ChunkItemPtr>::const_iterator it = selectedItems_.begin();
        it != selectedItems_.end();
        it++)
    {
        if ((*it) == item)
            return true;
    }

    return false;
}

bool WorldManager::isChunkSelected( Chunk * pChunk ) const
{
    BW_GUARD;

    for (BW::vector<ChunkItemPtr>::const_iterator it = selectedItems_.begin();
        it != selectedItems_.end();
        it++)
    {
        if ((*it)->chunk() == pChunk && (*it)->isShellModel())
            return true;
    }

    return false;
}

bool WorldManager::isChunkSelected( bool forceCalculateNow /*= false*/ ) const
{
    BW_GUARD;

    static uint32 s_frameMark = 0;
    static bool s_ret = false;
    if (forceCalculateNow || s_frameMark != Moo::rc().frameTimestamp())
    {
        s_ret = false;
        s_frameMark = Moo::rc().frameTimestamp();
        for (BW::vector<ChunkItemPtr>::const_iterator it = selectedItems_.begin();
            it != selectedItems_.end(); ++it)
        {
            if ((*it)->isShellModel())
            {
                s_ret = true;
                break;
            }
        }
    }

    return s_ret;
}

bool WorldManager::isChunkSelectable( Chunk * pChunk ) const
{
    BW_GUARD;

    for (BW::vector<ChunkItemPtr>::const_iterator it = selectedItems_.begin();
        it != selectedItems_.end();
        it++)
    {
        if ((*it)->edIsVLO())
        {
            const EditorChunkVLO* vlo = (EditorChunkVLO*)it->getObject();

            if (vlo->object()->chunkItems().size() != 1)
            {
                continue;
            }
        }
        if ((*it)->chunk() == pChunk && !(*it)->isShellModel())
            return true;
    }

    return false;
}

bool WorldManager::isInPlayerPreviewMode() const
{
    return isInPlayerPreviewMode_;
}

void WorldManager::setPlayerPreviewMode( bool enable )
{
    BW_GUARD;

    if( enable )
    {
        GUI::ItemPtr hideOrtho = GUI::Manager::instance()( "/MainToolbar/Edit/ViewOrtho/HideOrthoMode" );
        if( hideOrtho && hideOrtho->update() )
            hideOrtho->act();
    }
    isInPlayerPreviewMode_ = enable;
}


bool WorldManager::touchAllChunks()
{
    BW_GUARD;


    if (!canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/INVALIDATE") ))
    {
        return false;
    }

    // Stop background chunk loading:
    SyncMode chunkStopper;

    this->unloadChunks();

    this->cleanupTools();

    ProgressBarTask progressTask(
        LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/TOUCH_ALL_CHUNKS" ), 100, true );

    invalidateAllChunks( WorldManager::instance().geometryMapping(), &progressTask );

    // let dirty chunk list rebuild next time when the space gets loaded
    WorldManager::instance().dirtyChunkLists_.clear();
    WorldManager::instance().writeCleanList();
    WorldManager::instance().chunksBeingEdited_.clear();
    WorldManager::instance().reloadAllChunks( false );

    return true;
}

bool WorldManager::performingModalOperation()
{
    BW_GUARD;
    return this->uiBlocked();
}


/**
 *  This method sets or resets the "removable" flag for all
 *  chunks in memory, on the basis of whether or not they
 *  possess modified data.
 */
void WorldManager::markChunks()
{
    BW_GUARD;

    if( !EditorChunkCache::chunks_.empty() )
        getSelection();
    else
        selectedItems_.clear();

    for( BW::set<Chunk*>::iterator iter = EditorChunkCache::chunks_.begin();
        iter != EditorChunkCache::chunks_.end(); ++iter )
        (*iter)->removable( true );

    ChunkProcessorManager::mark();

    UndoRedo::instance().markChunk();

    for( BW::vector<ChunkItemPtr>::iterator iter = selectedItems_.begin();
        iter != selectedItems_.end(); ++iter )
        if ( (*iter)->chunk() )
            (*iter)->chunk()->removable( false );
}


/**
 *  Unload chunks. Does not cancel currently pending/loading chunks.
 */
void WorldManager::unloadChunks()
{
    BW_GUARD;

    ChunkManager::instance().switchToSyncMode( true );

    AmortiseChunkItemDelete::instance().purge();

    EditorChunkOverlapper::drawList.clear();

    DataSectionCache::instance()->clear();
    DataSectionCensus::clear();

    markChunks();

    // Copy the set as chunk caches can be deleted during unload
    BW::set< Chunk * > chunksCopy = EditorChunkCache::chunks_;
    ChunkManager::instance().cameraSpace()->unloadChunks( chunksCopy );

    // Make sure all the chunk's items are removed.
    AmortiseChunkItemDelete::instance().purge();

    ChunkManager::instance().switchToSyncMode( false );
}


int WorldManager::ScopedDeferSelectionReplacement::s_count_ = 0;
SimpleMutex WorldManager::ScopedDeferSelectionReplacement::s_mutex_;

WorldManager::ScopedDeferSelectionReplacement::ScopedDeferSelectionReplacement()
{
    SimpleMutexHolder smh( s_mutex_ );
    ++s_count_;
}

WorldManager::ScopedDeferSelectionReplacement::~ScopedDeferSelectionReplacement()
{
    SimpleMutexHolder smh( s_mutex_ );
    if (--s_count_ == 0)
    {
        WorldManager::instance().flushSelectionReplacement();
    }
}


void WorldManager::replaceSelection( const ChunkItemPtr & itemToRemove,
                                    const ChunkItemPtr & itemToAdd )
{
    SimpleMutexHolder smh( ScopedDeferSelectionReplacement::s_mutex_ );
    deferReplaceSelection( itemToRemove, itemToAdd );
    if (ScopedDeferSelectionReplacement::s_count_ == 0)
    {
        flushSelectionReplacement();
    }
}

bool WorldManager::hasPendingSelections()
{
    return ( !itemsToAdd_.empty() || !itemsToRemove_.empty() );
}

void WorldManager::deferReplaceSelection( const ChunkItemPtr & itemToRemove,
                                         const ChunkItemPtr & itemToAdd )
{
    // Check that the item to remove is in the current selection or in the items
    // we wish to add.
    MF_ASSERT( std::find( selectedItems_.begin(), selectedItems_.end(),
        itemToRemove ) != selectedItems_.end() || std::find( itemsToAdd_.begin(),
        itemsToAdd_.end(), itemToRemove ) != itemsToAdd_.end() );
    // Check we haven't already tried to remove the item to remove
    MF_ASSERT( std::find( itemsToRemove_.begin(), itemsToRemove_.end(), 
        itemToRemove ) == itemsToRemove_.end() );
    // Check that the item to add isn't in the current selection or is in the
    // the items we wish to remove.
    MF_ASSERT( std::find( selectedItems_.begin(), selectedItems_.end(),
        itemToAdd ) == selectedItems_.end() ||  std::find( itemsToRemove_.begin(),
        itemsToRemove_.end(), itemToAdd ) != itemsToRemove_.end() );
    // Check we haven't already tried to add the item to add
    MF_ASSERT( std::find( itemsToAdd_.begin(), itemsToAdd_.end(), 
        itemToAdd ) == itemsToAdd_.end() );

    BW::vector<ChunkItemPtr>::iterator it =
        std::find( itemsToAdd_.begin(), itemsToAdd_.end(), itemToRemove );
    if (it != itemsToAdd_.end())
    {
        itemsToAdd_.erase( it );
    }
    else
    {
        itemsToRemove_.push_back( itemToRemove );
    }

    it = std::find( itemsToRemove_.begin(), itemsToRemove_.end(), itemToAdd );
    if (it != itemsToRemove_.end())
    {
        itemsToRemove_.erase( it );
    }
    else
    {
        itemsToAdd_.push_back( itemToAdd );
    }
}


void WorldManager::flushSelectionReplacement()
{
    if (itemsToRemove_.empty() && itemsToAdd_.empty())
    {
        return;
    }

    BW::vector<ChunkItemPtr> items;
    for ( BW::vector<ChunkItemPtr>::const_iterator it = 
        selectedItems_.begin(); it != selectedItems_.end(); ++it )
    {
        if (std::find( itemsToRemove_.begin(), itemsToRemove_.end(), *it ) ==
            itemsToRemove_.end())
        {
            items.push_back( *it );
        }
    }
    for ( BW::vector<ChunkItemPtr>::const_iterator it = 
        itemsToAdd_.begin(); it != itemsToAdd_.end(); ++it )
    {
        items.push_back( *it );
    }

    // Verify all the items were successfully replaced.
    MF_ASSERT( items.size() == selectedItems_.size() - itemsToRemove_.size() +
        itemsToAdd_.size() );
    itemsToRemove_.clear();
    itemsToAdd_.clear();

    setSelection( items, true );
    // check if barrier needs to be added
    if (addUndoBarrier_)
    {
        // set the barrier with a meaningful name
        UndoRedo::instance().barrier( 
            LocaliseUTF8( L"GIZMO/UNDO/MOVE_PROPERTY" ) + 
            " " + items.front()->edDescription().str(), false );
        // TODO: Don't always say 'Move ' ...
        //  figure it out from change in matrix
    }
}


void WorldManager::setSelection( const BW::vector<ChunkItemPtr>& items, bool updateSelection )
{
    BW_GUARD;

    MF_ASSERT( itemsToRemove_.empty() && itemsToAdd_.empty() );

    PyObject* pModule = PyImport_ImportModule( "WorldEditorDirector" );
    if (pModule != NULL)
    {
        PyObject* pScriptObject = PyObject_GetAttrString( pModule, "bd" );

        if (pScriptObject != NULL)
        {
            ChunkItemGroupPtr group = 
                ChunkItemGroupPtr(new ChunkItemGroup( items ), PyObjectPtr::STEAL_REFERENCE);

            settingSelection_ = true;

            Script::call(
                PyObject_GetAttrString( pScriptObject, "setSelection" ),
                Py_BuildValue( "(Oi)",
                    static_cast<PyObject *>(group.getObject()),
                    (int) updateSelection ),
                "WorldEditor");

            settingSelection_ = false;

            if (!updateSelection)
            {
                // Note that this doesn't update snaps etc etc - it is assumed
                // that revealSelection will be called some time in the near
                // future, and thus this will get updated properly. This only
                // happens here so that a call to selectedItems() following
                // this will return what's expected.
                this->updateSelection( items );
            }
            Py_DECREF( pScriptObject );
        }
        Py_DECREF( pModule );
    }
}

void WorldManager::getSelection()
{
    BW_GUARD;

    PyObject* pModule = PyImport_ImportModule( "WorldEditorDirector" );
    if( pModule != NULL )
    {
        PyObject* pScriptObject = PyObject_GetAttrString( pModule, "bd" );

        if( pScriptObject != NULL )
        {
            ChunkItemGroup* cig = new ChunkItemGroup();
            Script::call(
                PyObject_GetAttrString( pScriptObject, "getSelection" ),
                Py_BuildValue( "(O)", static_cast<PyObject*>( cig ) ),
                "WorldEditor");

            updateSelection( cig->get() );

            Py_DecRef( cig );
            Py_DECREF( pScriptObject );
        }
        Py_DECREF( pModule );
    }
}

bool WorldManager::drawSelection() const
{
    return drawSelection_;
}


/**
 *  This method sets the current rendering state of WorldEditor. It also resets
 *  the list of registered selectable items.
 *
 *  @param drawingSelection Set to 'true' means that everything should render
 *                          in marquee selection mode, 'false' means normal
 *                          3D rendering.
 */
void WorldManager::drawSelection( bool drawingSelection )
{
    BW_GUARD;

    if (drawSelection_ != drawingSelection)
    {
        drawSelection_ = drawingSelection;
        if (drawSelection_)
        {
            // About to start draw selection, so clear the selection items
            drawSelectionItems_.clear();
        }
    }

    EditorChunkItem::drawSelection( drawingSelection );
}


/**
 *  This method is called by chunk items that wish to be selectable using the
 *  marquee selection, allowing WorldEditor to prepare the render states for
 *  the item and registering the item as selectable.
 *
 *  @param item     Chunk item to add to the list of selectable items.
 */
void WorldManager::registerDrawSelectionItem( ChunkItem * item )
{
    BW_GUARD;

    drawSelectionItems_.push_back( item );

    // This render state change works for most chunk items, but in some cases,
    // like terrain, the actual rendering is delayed, so these objects might 
    // need to set this render state again before issuing the draw calls.
    const DWORD index = static_cast< DWORD>( drawSelectionItems_.size() );
    Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, index );
}


/**
 *  This method returns a chunk item pointer by its index.
 *
 *  @param index    Chunk item index in the list of selectable items.
 *  @return         Chunk item or NULL if the index is invalid.
 */
ChunkItem * WorldManager::getDrawSelectionItem( DWORD index ) const
{
    BW_GUARD;
    return index > 0 && index <= drawSelectionItems_.size() ? 
        drawSelectionItems_[index - 1] : NULL;
}


/**
 *  return a loaded chunk, if wasForcedIntoMemory is specified, it represents whether this chunk is loaded in this function or not.
 */
Chunk* WorldManager::getChunk( const BW::string& chunkID, bool forceIntoMemory, bool* wasForcedIntoMemory )
{
    BW_GUARD;

    Chunk *pChunk = 
        ChunkManager::instance().findChunkByName(
                                        chunkID,
                                        this->geometryMapping() );
    if (!pChunk)
    {
        return NULL;
    }

    // If we are in the middle of loading, then we should wait
    // until the loading has finished. Note we don't need to care
    // if the chunk is in the pending list because pending chunks
    // won't be moved into the loading state until next time
    // ChunkManager::tick is called from the main thread. 
    while (pChunk->loading())
    {
        ChunkManager::instance().checkLoadingChunks();
    }

    bool inMemory = pChunk->loaded();
    if (!forceIntoMemory && !inMemory)
    {
        return NULL;
    }

    if (!inMemory)
    {
        ChunkManager::instance().loadChunkNow(
                chunkID,
                this->geometryMapping() );
        ChunkManager::instance().checkLoadingChunks();
        if (wasForcedIntoMemory != NULL)
        {
            *wasForcedIntoMemory = true;
        }
    }

    MF_ASSERT( pChunk->loaded() );

    return pChunk;
}

/**
 *  This method recalculates the thumbnail for the given chunk ID. Optionally,
 *  it is possible to temporarily load the chunk into memory to perform the
 *  operation if neccessary.
 *
 *  @param chunkID  Identifier string for the chunk to recalculate
 *  @param forceIntoMemory Load the chunk into memory temporarily if neccesary.
 *  @param unsavedChunks UnsavedChunks instance to add the chunk to.
 *  @return 'true' if the operation was done successfully, 'false' otherwise.
 */
bool WorldManager::recalcThumbnail( const BW::string& chunkID, 
                                   bool forceIntoMemory )
{
    bool wasForcedIntoMemory = false;
    Chunk *pChunk = this->getChunk( chunkID, forceIntoMemory, &wasForcedIntoMemory );
    if ( pChunk == NULL )
    {
        return false;
    }

    this->recalcThumbnail( pChunk );

    if (wasForcedIntoMemory)
    {
        MF_ASSERT( pChunk->removable() );

        WEChunkSaver chunkSaver;
        UnsavedChunks unsavedChunks;
        
        unsavedChunks.add( pChunk );
        unsavedChunks.save( chunkSaver );

        // Save will cause an implicit unload if pChunk is 
        // removable (which according to the assert above it is).
        MF_ASSERT( !pChunk->loaded() );

        // We just saved this chunk so make sure it is removed
        // from the global unsaved chunks list.
        unsavedChunks_.filter( unsavedChunks );
        unsavedCdatas_.filter( unsavedChunks );
    }

    return true;
}

/**
 *  Recalculates the thumbnail for the given Chunk. Assumes the chunk
 *  is fully loaded and ready to calculate.
 *
 *  @param item     Chunk item pointer to calculate.
 *  @return         'true' on success, false otherwise.
 */
bool WorldManager::recalcThumbnail( Chunk* pChunk )
{
    BW_GUARD;

    MF_ASSERT( pChunk != NULL );
    MF_ASSERT( pChunk->loaded() );

    return EditorChunkThumbnailCache::instance( *pChunk ).recalc(
        this, *this );
}

/**
 *  Generate image of each chunk.
 */
bool WorldManager::takePhotoOfChunk( const BW::string& chunkID, const BW::string& folderPath, int photoPosX, int photoPosY, float pixelsPerMetre, int& imgSize )
{
    bool ret = false;
    bool loadedByCaller = false;
    Chunk *pChunk = this->getChunk( chunkID, true, &loadedByCaller );
    if ( pChunk == NULL )
    {
        return false;
    }

    int width = 128;
    int height = 128;
    EditorChunkTerrainBase* pEct = EditorChunkTerrainCache::instance( *pChunk ).pTerrain();
    if( pEct )
    {
        width = (int32)( (int)this->getTerrainInfo().widthM * pixelsPerMetre );
        height = width;
        pEct->block().rebuildLodTexture(pChunk->transform());
    }
    else
    {
        width = (int32)( pTerrainSettings()->blockSize() * pixelsPerMetre );
        height = width;
    }
    imgSize = width;

    //std::ostringstream oss;
    //oss << folderPath << width <<
    char filePath[MAX_PATH];
    bw_snprintf( filePath, MAX_PATH, "%s/temp/%d/%d.jpg", folderPath.c_str(), photoPosY, photoPosX );
    BW::string newPath( filePath );
    ret = ChunkPhotographer::photograph( (*pChunk), newPath, width, height );

    //clear pendingChangedChunks_ before we unload the chunk
    if (pendingChangedChunks_.size())
    {
        this->flushDelayedChanges();
    }

    /*
    if (loadedByCaller && 
        this->isChunkProcessed( pChunk )
        )
    {
        if (pChunk->isBound())
        {
            pChunk->unbind( false );
        }
        pChunk->unload();
    }
    */

    return ret;
}


bool WorldManager::snapsEnabled() const
{
    BW_GUARD;

    return !!OptionsSnaps::snapsEnabled();
}

bool WorldManager::freeSnapsEnabled() const
{
    BW_GUARD;

    if (isChunkSelected())
        return false;

    return OptionsSnaps::placementMode() == 0;
}

bool WorldManager::terrainSnapsEnabled() const
{
    BW_GUARD;

    if (isChunkSelected())
        return false;

    return OptionsSnaps::placementMode() == 1;
}

bool WorldManager::obstacleSnapsEnabled() const
{
    BW_GUARD;

    if (isChunkSelected())
        return false;

    return OptionsSnaps::placementMode() == 2;
}

WorldManager::CoordMode WorldManager::getCoordMode() const
{
    BW_GUARD;

    BW::string mode( OptionsSnaps::coordMode() );
    if(mode == "Local")
        return COORDMODE_OBJECT;
    if(mode == "View")
        return COORDMODE_VIEW;
    return COORDMODE_WORLD;
}

Vector3 WorldManager::movementSnaps() const
{
    BW_GUARD;

    Vector3 movementSnap( OptionsSnaps::movementSnaps() );

    // Don't snap in the y-direction if snaps and terrain locking are both 
    // enabled.
    if (snapsEnabled() && terrainSnapsEnabled())
    {
       movementSnap.y = 0.f;
    }

    return movementSnap;
}

float WorldManager::angleSnaps() const
{
    BW_GUARD;

    if (snapsEnabled())
        return Snap::satisfy( angleSnaps_, OptionsSnaps::angleSnaps() );
    else
        return angleSnaps_;
}

void WorldManager::calculateSnaps()
{
    BW_GUARD;

    angleSnaps_ = 0.f;
    movementDeltaSnaps_ = Vector3( 0.f, 0.f, 0.f );

    for (BW::vector<ChunkItemPtr>::iterator it = selectedItems_.begin();
        it != selectedItems_.end();
        it++)
    {
        angleSnaps_ = Snap::satisfy( angleSnaps_, (*it)->edAngleSnaps() );
        Vector3 m = (*it)->edMovementDeltaSnaps();

        movementDeltaSnaps_.x = Snap::satisfy( movementDeltaSnaps_.x, m.x );
        movementDeltaSnaps_.y = Snap::satisfy( movementDeltaSnaps_.y, m.y );
        movementDeltaSnaps_.z = Snap::satisfy( movementDeltaSnaps_.z, m.z );
    }
}


int WorldManager::drawBSP() const
{
    BW_GUARD;

    static uint32 s_settingsMark_ = -16;
    static int drawBSP = 0;
    if (Moo::rc().frameTimestamp() != s_settingsMark_)
    {
        drawBSP = Options::getOptionInt( "render/drawBSP", 0 );
        s_settingsMark_ = Moo::rc().frameTimestamp();
    }
    return drawBSP;
}

int WorldManager::drawBSPSpeedTrees() const
{
    BW_GUARD;
    static uint32 s_settingsMark_ = -16;
    static int drawBSP = 0;

    if (Moo::rc().frameTimestamp() != s_settingsMark_)
    {
        drawBSP = Options::getOptionInt( "render/drawBSP/SpeedTrees", 0 );
        s_settingsMark_ = Moo::rc().frameTimestamp();
    }

    return drawBSP;
}

int WorldManager::drawBSPOtherModels() const
{
    BW_GUARD;
    static uint32 s_settingsMark_ = -16;
    static int drawBSP = 0;

    if (Moo::rc().frameTimestamp() != s_settingsMark_)
    {
        drawBSP = Options::getOptionInt( "render/drawBSP/OtherModels", 0 );
        s_settingsMark_ = Moo::rc().frameTimestamp();
    }

    return drawBSP;
}

void WorldManager::snapAngles( Matrix& v )
{
    BW_GUARD;

    if ( snapsEnabled() )
        Snap::angles( v, angleSnaps() );
}

SnapProvider::SnapMode WorldManager::snapMode( ) const
{
    BW_GUARD;

    return  terrainSnapsEnabled()   ?   SNAPMODE_TERRAIN :
            obstacleSnapsEnabled()  ?   SNAPMODE_OBSTACLE :
                                        SNAPMODE_XYZ ;
}

bool WorldManager::snapPosition( Vector3& v )
{
    BW_GUARD;

    Vector3 origPosition = v;
    if ( snapsEnabled() )
        v = Snap::vector3( v, movementSnaps() );
    if ( terrainSnapsEnabled() )
        v = Snap::toGround( v );
    else if ( obstacleSnapsEnabled() )
    {
        Vector3 startPosition = Moo::rc().invView().applyToOrigin();
        if (selectedItems_.size() > 1)
            startPosition = v - (Moo::rc().invView().applyToOrigin().length() * worldRay());
        bool hitObstacle = false;
        Vector3 newV = Snap::toObstacle( startPosition, worldRay(), false,
             getMaxFarPlane(), &hitObstacle );
        if (!hitObstacle)
        {
            v = origPosition;
            return false;
        }
        else
        {
            v = newV;
        }
    }
    return true;
}

Vector3 WorldManager::snapNormal( const Vector3& v )
{
    BW_GUARD;

    Vector3 result( 0, 1, 0 );// default for y axis, should never used
    if ( obstacleSnapsEnabled() )
    {
        Vector3 startPosition = Moo::rc().invView().applyToOrigin();
        if (selectedItems_.size() > 1)
            startPosition = v - (Moo::rc().invView().applyToOrigin().length() * worldRay());
        result = Snap::toObstacleNormal( startPosition, worldRay() );
    }
    return result;
}

void WorldManager::snapPositionDelta( Vector3& v )
{
    BW_GUARD;

    v = Snap::vector3( v, movementDeltaSnaps_ );
}

float WorldManager::angleSnapAmount()
{
    BW_GUARD;

    return angleSnaps();
}

void WorldManager::startSlowTask()
{
    BW_GUARD;

    SimpleMutexHolder smh( savedCursorMutex_ );
    ++slowTaskCount_;

    if (slowTaskCount_ == 1)
    {
        savedCursor_ = cursor_;
    }

    if (MainThreadTracker::isCurrentThreadMain())
    {
        cursor_ = ::LoadCursor( NULL, IDC_WAIT );
    }
    else
    {
        cursor_ = ::LoadCursor( NULL, IDC_APPSTARTING );
    }

    setCursor();
}

void WorldManager::stopSlowTask()
{
    BW_GUARD;

    SimpleMutexHolder smh( savedCursorMutex_ );

    --slowTaskCount_;
    if (slowTaskCount_ == 0)
    {
        cursor_ = savedCursor_;
        savedCursor_ = NULL;
        setCursor();
    }
}

void WorldManager::addReadOnlyBlock( const Matrix& transform, Terrain::BaseTerrainBlockPtr pBlock )
{
    BW_GUARD;

    readOnlyTerrainBlocks_.push_back( BlockInPlace( transform, pBlock ) );
}


void WorldManager::setColourFog( DWORD colour )
{
    BW_GUARD;

    Moo::FogParams params = Moo::FogHelper::pInstance()->fogParams();
    params.m_enabled = true;
    params.m_color = colour;
    params.m_start = -10000.0f;
    params.m_end = 10000.0f;
    Moo::FogHelper::pInstance()->fogParams(params);
}


void WorldManager::showReadOnlyRegion( const BoundingBox& bbox, const Matrix & boxTransform )
{
    BW_GUARD;
    chunkLockVisualizer_.addBox(bbox, boxTransform, 0x3FFF0000 );
}


void WorldManager::showFrozenRegion( const BoundingBox & bbox, const Matrix & boxTransform )
{
    BW_GUARD;
    chunkLockVisualizer_.addBox(bbox, boxTransform, 0x4FAAAAFF );
}


bool WorldManager::isPointInWriteableChunk( const Vector3& pt ) const
{
    BW_GUARD;

    return EditorChunk::outsideChunkWriteable( pt );
}

bool WorldManager::isBoundingBoxInWriteableChunk( const BoundingBox& box, const Vector3& offset ) const
{
    BW_GUARD;

    return EditorChunk::outsideChunksWriteableInSpace( BoundingBox( box.minBounds() + offset, box.maxBounds() + offset ) );
}

/**
 *  Checks to see if the space is fully locked and editable, and if it's not
 *  it'll popup a warning and return false.
 *
 *  @return     true if the space is fully editable (locked), false, otherwise.
 */
bool WorldManager::warnSpaceNotLocked()
{
    BW_GUARD;

    if( connection().isAllLocked() )
        return true;

    // Some parts of the space are not locked, so show the warning.
    ::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/WARN_NOT_LOCKED"),
        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/WARN_NOT_LOCKED_CAPTION"),
        MB_OK | MB_ICONWARNING );

    return false;
}


void WorldManager::reloadAllChunks( bool askBeforeProceed )
{
    BW_GUARD;

    if (askBeforeProceed)
    {
        if (!canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RELOAD") ))
        {
            return;
        }
    }

    this->cleanupTools();

    BW::string space = currentSpace_;
    currentSpace_ = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/EMPTY");
    MsgHandler::instance().removeAssetErrorMessages();
    changeSpace( space, true );
}

// -----------------------------------------------------------------------------
// Section: Error message handling
// -----------------------------------------------------------------------------

class LogFileIniter
{
public:
    LogFileIniter()
    {
        BW_GUARD;

        //Instanciate the Message handler to catch BigWorld messages
        MsgHandler::instance();
    }

    ~LogFileIniter()
    {
        BW_GUARD;
        MsgHandler::fini();
    }
};
// creating statically to allow output from very early in the app's lifetime.
LogFileIniter logFileIniter;

SimpleMutex WorldManager::pendingMessagesMutex_;
WorldManager::StringVector WorldManager::pendingMessages_;

/**
 * Post all messages we've recorded since the last time this was called
 */
void WorldManager::postPendingErrorMessages()
{
    BW_GUARD;

    pendingMessagesMutex_.grab();
    StringVector::iterator i = pendingMessages_.begin();
    for (; i != pendingMessages_.end(); ++i)
    {
        Commentary::instance().addMsg( *i, Commentary::WARNING );
    }
    pendingMessages_.clear();
    pendingMessagesMutex_.give();
}

/**
 *  This static function implements the callback that will be called for each
 *  *_MSG.
 *
 *  This is thread safe. We only want to add the error as a commentary message
 *  from the main thread, thus the adding them to a vector. The actual posting
 *  is done in postPendingErrorMessages.
 */
bool WorldEditorDebugMessageCallback::handleMessage(
        DebugMessagePriority messagePriority, const char * /*pCategory*/,
        DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
        const char * pFormat, va_list argPtr )
{
    return WorldManager::messageHandler( messagePriority, pFormat, argPtr );
}


/**
 *  This static function implements the callback that will be called when a
 *  critical msg occurs.
 */
void WorldEditorCriticalMessageCallback::handleCritical( const char * msg )
{
    WorldManager::criticalMessageOccurred( true );
}


bool WorldManager::messageHandler( DebugMessagePriority messagePriority,
        const char * pFormat, va_list argPtr )
{
    BW_GUARD;

    static const int MAX_MESSAGE_BUFFER = 8192;
    static char buf[ MAX_MESSAGE_BUFFER ];
    LogMsg::formatMessage( buf, MAX_MESSAGE_BUFFER, pFormat, argPtr );

    if (DebugFilter::shouldAccept( messagePriority ) &&
        messagePriority == MESSAGE_PRIORITY_ERROR)                          
    {
        bool isNewError    = false;
        bool isPythonError = false;

        // make sure Python is initialised before a check
        if (Py_IsInitialized())
        {
            isPythonError = PyErr_Occurred() != NULL;
        }

        if (isPythonError)
        {
            BW::string stacktrace = getPythonStackTrace();
            if( &MsgHandler::instance() )
                isNewError = MsgHandler::instance().addAssetErrorMessage( 
                    buf, NULL, NULL, stacktrace.c_str());
            else
                isNewError = true;

            strncat( buf, LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SEE_MSG_PANEL").c_str(), ARRAY_SIZE(buf) - strlen( buf ) );
        }

        if (isNewError) 
        {
            pendingMessagesMutex_.grab();
            pendingMessages_.push_back( buf );
            pendingMessagesMutex_.give();
        }
    }

    return false;
}


void WorldManager::addPrimGroupCount( Chunk* chunk, uint n )
{
    if (chunk == currentMonitoredChunk_)
        currentPrimGroupCount_ += n;
}


TimeOfDay* WorldManager::timeOfDay()
{
    return romp_->timeOfDay();
}


EnviroMinder& WorldManager::enviroMinder()
{
    return romp_->enviroMinder();
}


void WorldManager::refreshWeather()
{
    BW_GUARD;

    if( romp_ )
        romp_->update( 1.0, globalWeather_ );
}

void WorldManager::setTimeFromOptions()
{
    this->timeOfDay()->gameTime(
        float(
            Options::getOptionInt(
                "graphics/timeofday",
                12 * TIME_OF_DAY_MULTIPLIER /*noon*/ )
        ) / float( TIME_OF_DAY_MULTIPLIER )
    );
}

void WorldManager::setStatusMessage( unsigned int index, const BW::string& value )
{
    BW_GUARD;

    if( index >= statusMessages_.size() )
        statusMessages_.resize( index + 1 );
    statusMessages_[ index ] = value;
}

const BW::string& WorldManager::getStatusMessage( unsigned int index ) const
{
    BW_GUARD;

    static BW::string empty;
    if( index >= statusMessages_.size() )
        return empty;
    return statusMessages_[ index ];
}

void WorldManager::setCursor( HCURSOR cursor )
{
    BW_GUARD;

    SimpleMutexHolder smh( savedCursorMutex_ );

    if (savedCursor_ != NULL)
    {
        savedCursor_ = cursor;
    }
    else if( cursor_ != cursor )
    {
        cursor_ = cursor;
        setCursor();
    }
}

void WorldManager::resetCursor()
{
    BW_GUARD;

    static HCURSOR cursor = ::LoadCursor( NULL, IDC_ARROW );
    setCursor( cursor );
}

void WorldManager::setCursor()
{
    BW_GUARD;

    POINT mouse;
    GetCursorPos( &mouse );
    SetCursorPos( mouse.x, mouse.y + 1 );
    SetCursorPos( mouse.x, mouse.y );
}


void WorldManager::updateSelection( const BW::vector< ChunkItemPtr > & items )
{
    BW_GUARD;
    MF_ASSERT( itemsToRemove_.empty() && itemsToAdd_.empty() );
    BW::vector<ChunkItemPtr> itemsAdded, itemsRemoved;

    for (uint i = 0; i < selectedItems_.size(); i++)
    {
        ChunkItemPtr item = selectedItems_[i];
        if (std::find(items.begin(), items.end(), item) == items.end()
            || !item->edIsSelected())
        {
            selectedItems_.erase(selectedItems_.begin() + i);
            itemsRemoved.push_back(item);
            i--;
        }
    }

    for (uint i = 0; i < items.size(); i++)
    {
        ChunkItemPtr item = items[i];
        if (std::find(selectedItems_.begin(), selectedItems_.end(), item) == selectedItems_.end())
        {
            selectedItems_.push_back(item);
            itemsAdded.push_back(item);
        }
    }

    size_t itemsSelected = selectedItems_.size();

    for (uint i = 0; i < itemsRemoved.size(); i++)
        itemsRemoved[i]->edDeselected();

    for (uint i = 0; i < itemsAdded.size(); i++)
        itemsAdded[i]->edSelected( selectedItems_ );
    
    if (itemsSelected == selectedItems_.size())
    {
        return;
    }

    PyObject* module = PyImport_ImportModule( "WorldEditorDirector" );

    if (module == nullptr)
    {
        return;
    }

    PyObject* scriptObject = PyObject_GetAttrString( module, "bd" );

    if (scriptObject == nullptr)
    {
        return;
    }

    ChunkItemGroupPtr group = ChunkItemGroupPtr(
        new ChunkItemGroup( selectedItems_ ), PyObjectPtr::STEAL_REFERENCE );

    PyObject* update = static_cast<PyObject *>( group.getObject() );

    Script::call(
        PyObject_GetAttrString( scriptObject, "setSelection" ),
        Py_BuildValue( "(Oi)", update, int( 0 ) ), "WorldEditor" );
}

/**
 *  Tell the texture layers page to refresh (if visible) 
 *
 *  @param chunk        chunk to refresh
 */
void WorldManager::chunkTexturesPainted( Chunk *chunk, bool rebuiltLodTexture )
{
    BW_GUARD;

    if (chunk == NULL)
    {
        WARNING_MSG( "WorldManager::chunkTexturesPainted: chunk is NULL\n" );
    }

    BW::string chunkID;
    if ( chunk != NULL )
        chunkID = chunk->identifier();

    PageChunkTexture::refresh( chunkID );

    if (!rebuiltLodTexture && chunk != NULL)
    {
        for (int i = 0; i < ChunkCache::cacheNum(); ++i)
        {
            if (ChunkCache* cc = chunk->cache( i ))
            {
                cc->onChunkChanged( 
                    InvalidateFlags::FLAG_TERRAIN_LOD |
                    InvalidateFlags::FLAG_THUMBNAIL,
                    this );
            }
        }
    }
}


/**
 *  Show the context menu about textures in a chunk
 *
 *  @param chunk        chunk where the click was done
 */
void WorldManager::chunkTexturesContextMenu( Chunk* chunk )
{
    BW_GUARD;

    this->resetCursor();
    // force setting the cursor now
    SetCursor( this->cursor() );
    ShowCursor( TRUE );

    // If the button was released near where it was pressed, assume
    // its a right-click instead of a camera movement.
    PopupMenu menu;

    // Get the textures under the cursor and sort by strength:
    Vector3 cursorPos;
    TerrainTextureUtils::LayerVec layers;
    bool canEditProjections = false;
    if (PageTerrainTexture::instance() != NULL)
    {
        cursorPos = PageTerrainTexture::instance()->toolPos();
        PageTerrainTexture::instance()->layersAtPoint(cursorPos, layers);
        canEditProjections = PageTerrainTexture::instance()->canEditProjections();
    }
    std::sort(layers.begin(), layers.end());

    // build menu items
    const int toggleTrackCursorCmd = 1000;
    BW::string trackCursor;
    if ( PageChunkTexture::trackCursor() )
    {   
        trackCursor = "##";
    }
    menu.addItem( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/TRACK_CURSOR", trackCursor ), toggleTrackCursorCmd );
    menu.addSeparator();

    const int chunkTexturesCmd = 2000;
    if ( chunk != NULL )
    {
        menu.addItem( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/MANAGE_TEXTURE"), chunkTexturesCmd );
    }

    const int clearSelectedCmd = 3000;
    if ( !PageChunkTexture::trackCursor() && !PageChunkTexture::chunk().empty() )
    {
        menu.addItem( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/DESELECT_CHUNK"), clearSelectedCmd );
    }
    menu.addSeparator();

    const int selectTextureCmd = 4000;
    if (chunk != NULL)
    {
        menu.startSubmenu
        ( 
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SELECT_TEXTURE")
        );
        size_t maxLayers = std::min((size_t)1000, layers.size()); // clip to 1000 texture layers
        MF_ASSERT( maxLayers <= INT_MAX );
        for ( int i = 0; i < ( int ) maxLayers; ++i)
        {
            // Do not include zero-strength layers.  Note we break instead of
            // continue because the layers are sorted by strength and so there
            // are no layers with any strength after the first one with a 
            // strength of zero.
            if (layers[i].strength_ == 0)
                break;
            BW::wstring texName;
            bw_utf8tow( BWResource::getFilename(layers[i].textureName_).to_string(), texName );
            menu.addItem(texName, selectTextureCmd + i);
        }
        menu.endSubmenu();
    }

    const int opacityCmd = 5000;
    if (chunk != NULL)
    {
        menu.startSubmenu
        ( 
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/MATCH_OPACITY")
        );
        size_t maxLayers = std::min((size_t)1000, layers.size()); // clip to 1000 texture layers
        for (size_t i = 0; i < maxLayers; ++i)
        {
            // Do not include zero-strength layers.  Note we break instead of
            // continue because the layers are sorted by strength and so there
            // are no layers with any strength after the first one with a 
            // strength of zero.
            if (layers[i].strength_ == 0)
                break;
            BW::wstring texName = 
                Localise
                (
                    L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/MATCH_OPACITY_ENTRY", 
                    BWResource::getFilename(layers[i].textureName_).to_string(), 
                    (int)(100.0f*layers[i].strength_/255.0f + 0.5f)
                );
            menu.addItem(texName, static_cast<int>(opacityCmd + i));
        }
        menu.endSubmenu();
    }

    const int selTexForMaskCmd = 6000;
    if (chunk != NULL)
    {
        menu.startSubmenu
        ( 
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SELECT_TEXTURE_FOR_MASK")
        );
        size_t maxLayers = std::min((size_t)1000, layers.size()); // clip to 1000 texture layers
        BW::set<BW::string> addedTextures;
        for (size_t i = 0; i < maxLayers; ++i)
        {
            // Do not include zero-strength layers.  Note we break instead of
            // continue because the layers are sorted by strength and so there
            // are no layers with any strength after the first one with a 
            // strength of zero.
            if (layers[i].strength_ == 0)
                break;
            BW::string texName = BWResource::getFilename(layers[i].textureName_).to_string();
            if (addedTextures.find(texName) == addedTextures.end())
            {
                addedTextures.insert(texName);
                BW::wstring wtexName;
                bw_utf8tow( texName, wtexName );
                menu.addItem(wtexName, static_cast<int>(selTexForMaskCmd + i));
            }
        }
        menu.endSubmenu();
    }

    const int selTexAndProjForMaskCmd = 7000;
    if (chunk != NULL && canEditProjections)
    {
        menu.startSubmenu
        ( 
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SELECT_TEXTURE_AND_PROJ_FOR_MASK")
        );
        size_t maxLayers = std::min((size_t)1000, layers.size()); // clip to 1000 texture layers
        for (size_t i = 0; i < maxLayers; ++i)
        {
            // Do not include zero-strength layers.  Note we break instead of
            // continue because the layers are sorted by strength and so there
            // are no layers with any strength after the first one with a 
            // strength of zero.
            if (layers[i].strength_ == 0)
                break;
            BW::wstring texName;
            bw_utf8tow( BWResource::getFilename(layers[i].textureName_).to_string(), texName );
            menu.addItem(texName, static_cast<int>(selTexAndProjForMaskCmd + i));
        }
        menu.endSubmenu();
    }

    const int editProjectionCmd = 8000;
    if (chunk != NULL && canEditProjections)
    {
        menu.startSubmenu
        ( 
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/EDIT_PROJECTION_AND_SCALE")
        );
        size_t maxLayers = std::min((size_t)1000, layers.size()); // clip to 1000 texture layers
        for (size_t i = 0; i < maxLayers; ++i)
        {
            // Do not include zero-strength layers.  Note we break instead of
            // continue because the layers are sorted by strength and so there
            // are no layers with any strength after the first one with a 
            // strength of zero.
            if (layers[i].strength_ == 0)
                break;
            BW::wstring texName;
            bw_utf8tow( BWResource::getFilename(layers[i].textureName_).to_string(), texName );
            menu.addItem(texName, static_cast<int>(editProjectionCmd + i));
        }
        menu.endSubmenu();
    }


    // run the menu
    int result = menu.doModal( this->hwndGraphics() );

    // handle the results
    if (result == toggleTrackCursorCmd)
    {
        PageChunkTexture::trackCursor( !PageChunkTexture::trackCursor() );
    }
    else if (result == chunkTexturesCmd)
    {
        if ( chunk != NULL )
        {
            PageChunkTexture::trackCursor( false );
            PageChunkTexture::chunk( chunk->identifier(), true );
        }
    }
    else if (selectTextureCmd <= result && result < selectTextureCmd + 1000)
    {
        if ( chunk != NULL && PageTerrainTexture::instance() != NULL)
        {
            size_t idx      = result - selectTextureCmd;
            size_t layerIdx = layers[idx].layerIdx_;
            PageTerrainTexture::instance()->selectTextureAtPoint(cursorPos, layerIdx);
        }
    }
    else if (opacityCmd <= result && result < opacityCmd + 1000)
    {
        if ( chunk != NULL && PageTerrainTexture::instance() != NULL)
        {
            size_t idx      = result - opacityCmd;
            size_t layerIdx = layers[idx].layerIdx_;
            float opacity   = layers[idx].strength_/2.55f; // convert to %
            PageTerrainTexture::instance()->selectTextureAtPoint(cursorPos, layerIdx);
            PageTerrainTexture::instance()->setOpacity(opacity);
        }
    }
    else if (selTexForMaskCmd <= result && result < selTexForMaskCmd + 1000)
    {
            size_t idx      = result - selTexForMaskCmd;
            size_t layerIdx = layers[idx].layerIdx_;
            PageTerrainTexture::instance()->selectTextureMaskAtPoint(cursorPos, layerIdx, false, false);
    }
    else if (selTexAndProjForMaskCmd <= result && result < selTexAndProjForMaskCmd + 1000)
    {
            size_t idx      = result - selTexAndProjForMaskCmd;
            size_t layerIdx = layers[idx].layerIdx_;
            PageTerrainTexture::instance()->selectTextureMaskAtPoint(cursorPos, layerIdx, true, false);
    }
    else if (editProjectionCmd <= result && result < editProjectionCmd + 1000)
    {
        if ( chunk != NULL && PageTerrainTexture::instance() != NULL)
        {
            size_t idx      = result - editProjectionCmd;
            size_t layerIdx = layers[idx].layerIdx_;
            PageTerrainTexture::instance()->editProjectionAtPoint(cursorPos, layerIdx);
        }
    }
    else if (result == clearSelectedCmd)
    {
        PageChunkTexture::chunk( "" );
    }

    // restore previous cursor visibility state to whatever it was
    ShowCursor( FALSE );
}


/**
 *  Returns the terrain version used in the space by looking into the actual
 *  terrain blocks.
 */
uint32 WorldManager::getTerrainVersion() const
{
    BW_GUARD;
    return this->pTerrainSettings()->version();
}


/**
 *  Returns terrain parameters from a block with terrain in the space.
 */
const TerrainUtils::TerrainFormat& WorldManager::getTerrainInfo()
{
    BW_GUARD;
    const GeometryMapping* pMapping = this->geometryMapping();
    Terrain::TerrainSettingsPtr terrainSettings = this->pTerrainSettings();
    MF_ASSERT( pMapping != NULL );
    MF_ASSERT( terrainSettings.hasObject() );
    return terrainFormatStorage_.getTerrainInfo( pMapping, terrainSettings );
}


Terrain::TerrainSettingsPtr WorldManager::pTerrainSettings()
{
    if (this->geometryMapping() && this->geometryMapping()->pSpace())
    {
        return geometryMapping()->pSpace()->terrainSettings();
    }
    return NULL;
}


const Terrain::TerrainSettingsPtr WorldManager::pTerrainSettings() const
{
    if (this->geometryMapping() && this->geometryMapping()->pSpace())
    {
        return geometryMapping()->pSpace()->terrainSettings();
    }
    return NULL;
}


/**
 *  This method is called from the Scene Browser whenever the selection in it
 *  changes. The items in the "selection" list are guaranteed to be loaded only
 *  during the scope of this call.
 *
 *  @param selection    New selection from the SceneBrowser.
 */
void WorldManager::sceneBrowserSelChange(
                                    const SceneBrowser::ItemList & selection )
{
    BW_GUARD;

    BW::vector< ChunkItemPtr > items;
    for (SceneBrowser::ItemList::const_iterator it = selection.begin();
        it != selection.end(); ++it)
    {
        items.push_back( (*it)->chunkItem() );
    }

    UndoRedo::instance().add(
                            new SelectionOperation( selectedItems_, items ) );

    this->setSelection( items, true );

    UndoRedo::instance().barrier(
            LocaliseUTF8(
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SELECTION_CHANGE"),
            false );
}


/**
 *  This method controls whether ChunkProcessor threads are allowed to run
 *  or not.
 *  When called with 'false' will block until ChunkProcessor threads have
 *  blocked themselves.
 *
 *  @param  allow bool value indicating whether ChunkProcessor threads may run
 *
 */
/* static */ void WorldManager::allowChunkProcessorThreadsToRun( bool allow )
{
    if (allow == WorldManager::s_allowChunkProcessorThreads_)
    {
        return;
    }
    if (allow)
    {
        s_presentTimeSliceTimeStamp_ = timestamp();
        WorldManager::s_allowChunkProcessorThreads_ = true;
        WorldManager::s_chunkProcessorThreadsTimeSliceLock_.endWrite();
    }
    else
    {
        // Make sure we dedicate enough time per tick for the background task
        // to do a meaningful amount of work per time slice.
        if (s_minimumAllowTimeSliceMS_ > 0)
        {
            const uint64 minimumAllowedStamps = 
                (s_minimumAllowTimeSliceMS_ * stampsPerSecond()) / 1000;

            uint64 delta = timestamp() - s_presentTimeSliceTimeStamp_;
            if (delta < minimumAllowedStamps)
            {
                uint64 targetStamps = timestamp() + (minimumAllowedStamps - delta);
                while (timestamp() < targetStamps && WorldManager::instance().numBgTasksLeft())
                {
                    Sleep( 0 );
                }
            }
        }

        // Try to grab the slice lock. This will block until the BG task threads 
        // have relinquished their read locks in checkThreadNeedsToBeBlocked.
        uint64 timeStart = timestamp();
        WorldManager::s_allowChunkProcessorThreads_ = false;
        WorldManager::s_chunkProcessorThreadsTimeSliceLock_.beginWrite();
        uint64 timeWaited = timestamp() - timeStart;

        // Warn if we're spending too much time waiting for the
        // background thread to yield.
        if (s_timeSlicePauseWarnThresholdMS_ > 0)
        {
            const uint64 warningThreshold = 
                (s_timeSlicePauseWarnThresholdMS_ * stampsPerSecond()) / 1000;

            if (timeWaited > warningThreshold)
            {
                WARNING_MSG( "WorldManager::allowChunkProcessorThreadsToRun: Waited "
                    "%fms for ChunkProcessors to pause.\n",
                    (float)timeWaited * 1000.f / (float)stampsPerSecondD() );
            }
        }
        
        //uint64 timeRan = timestamp() - s_presentTimeSliceTimeStamp_;
        //float dts = (float)timeRan * 1000.f / (float)stampsPerSecondD();
        //TRACE_MSG( "--- background thread ran for %fms\n", dts );
    }
}


/**
 *  Callback indicating that the current thread has just called
 *  BgTaskManager::shouldAbortTask() so is in a safe place to be
 *  blocked or aborted.
 */
/* static */ void WorldManager::checkThreadNeedsToBeBlocked()
{
    if (WorldManager::s_allowChunkProcessorThreads_)
        return;
    // Block on the write lock taken by
    // WorldManager::allowChunkProcessorThreadsToRun
    WorldManager::s_chunkProcessorThreadsTimeSliceLock_.endRead();
    WorldManager::s_chunkProcessorThreadsTimeSliceLock_.beginRead();
}


/**
 *  Callback indicating that the current thread is just about
 *  to start the provided task, or has just finished a task.
 *
 *  @param  pTask BackgroundTaskPtr for the task about to be
 *          started, or NULL if we just finished a task.
 *
 */
/* virtual */ void WorldManager::setStaticThreadData( const BackgroundTaskPtr pTask )
{
    ChunkProcessorManager::setStaticThreadData( pTask );
    if (pTask != NULL)
    {
        // Pairs with the endRead() in checkThreadNeedsToBeBlocked()
        WorldManager::s_chunkProcessorThreadsTimeSliceLock_.beginRead();
    }
    else
    {
        // Pairs with the beginRead() in checkThreadNeedsToBeBlocked()
        WorldManager::s_chunkProcessorThreadsTimeSliceLock_.endRead();
    }
}


/**
 * This method can clear any records of changes
 */
void WorldManager::forceClean()
{
    BW_GUARD;

    unsavedTerrainBlocks_.clear();
    unsavedChunks_.clear();
    unsavedCdatas_.clear();
    Personality::instance().callMethod( "forceClean",
        ScriptErrorPrint( "WorldManager::forceClean: " ),
        /* allowNullMethod */ true );

    changedEnvironment_ = false;
}


/**
 * This method returns whether or not the scripts have anything
 * they need saving.
 *
 * @return true if there are any changes, false otherwise.
 */
bool WorldManager::isScriptDirty() const
{
    BW_GUARD;

    ScriptObject ret = Personality::instance().callMethod( "isDirty",
        ScriptErrorPrint( "WorldManager::isDirty: " ),
        /* allowNullMethod */ true );

    return ret && ret.isTrue( ScriptErrorPrint(
            "WorldManager isScriptDirty retval" ) );
};


/**
 * This method can return if there are any changes need to be saved
 *
 * @return true if there are any changes, false otherwise.
 */
bool WorldManager::isDirty() const
{
    BW_GUARD;

    bool changedTerrain = !unsavedTerrainBlocks_.empty();
    bool changedScenery = !unsavedChunks_.empty();
    bool changedThumbnail = !unsavedCdatas_.empty();

    return (changedTerrain || 
            changedScenery || 
            changedThumbnail || 
            changedEnvironment_ || 
            isScriptDirty());
}


/**
 * This method can return if the chunk has finished processing data
 *
 * @return true if processing data is finished, false otherwise.
 */
bool WorldManager::isChunkProcessed( Chunk* chunk ) const
{
    BW_GUARD;

    MF_ASSERT( chunk->loaded() );
    RecursiveMutexHolder lock( dirtyChunkListsMutex_ );
    return !this->dirtyChunkLists().isDirty( chunk->identifier() );
}


/**
 *  Ask the user if they want to close without saving.
 *  @param action name of the action we want to perform.
 */
bool WorldManager::canClose( const BW::string& action )
{
    BW_GUARD;

    if (disableClose_)
    {
        return false;
    }

    this->cleanupTools();
    PropertyList::deselectAllItems();

    bool exitWithoutSave = false;
    if (isDirty())
    {
        if (!CStdMf::checkUnattended())
        {
            BW::string actionNoShortcut( action );
            BW::string::iterator lastChar = std::remove( actionNoShortcut.begin(), actionNoShortcut.end(), '&' );
            actionNoShortcut.erase( lastChar, actionNoShortcut.end() );

            MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
            MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHANGED_FILES_TITLE", actionNoShortcut ),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHANGED_FILES_TEXT"),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SAVE"),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/PROCESS_AND_SAVE"),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/WITHOUT_SAVE", action ),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CANCEL") );

            // Remove input focus so that keyup events get forced through now.
            WorldEditorApp::instance().mfApp()->handleSetFocus( false );

            int result = mb.doModal( mainFrame->m_hWnd );
            if( result == 3 )
                return false;
            else if( result == 0 )
            {
                GUI::ItemPtr quickSave = GUI::Manager::instance()( "/MainToolbar/File/QuickSave" );
                quickSave->act();
                if ( saveFailed_ )
                    return false;
            }
            else if( result == 1 )
            {
                GUI::ItemPtr save = GUI::Manager::instance()( "/MainToolbar/File/Save" );
                save->act();
                if ( saveFailed_ )
                    return false;
            }
            else if (result == 2 )
            {
                exitWithoutSave = true;
            }
            WorldEditorApp::instance().mfApp()->consumeInput();
        }
        else
        {
            // exit without saving when running unattended
            exitWithoutSave = true;
        }
    }

    if (exitWithoutSave)
    {
        HeightModule::doNotSaveHeightMap();
    }

    // Delete VLOs no longer used in the space. Must do here to ensure it
    // happens both when changing space and quiting. Also, when a VLO is
    // deleted, it stays alive because there's a reference in the UndoRedo
    // history hanging to it, so must do this before clearing UndoRedo.
    VeryLargeObject::deleteUnused();

    UndoRedo::instance().clear();
    CSplashDlg::HideSplashScreen();
    return true;
}


/**
 *  Do any tool cleanup between changing modes.
 */
void WorldManager::cleanupTools() const
{
    BW_GUARD;

    // If a tool is applying, tell it to stop
    if (ToolManager::instance().isToolApplying())
    {
        ToolManager::instance().tool()->stopApplying( false );
    }
}


void WorldManager::updateUIToolMode( const BW::wstring& pyID )
{
    BW_GUARD;

    PanelManager::instance().updateUIToolMode( pyID );
}


//---------------------------------------------------------------------------

PY_MODULE_STATIC_METHOD( WorldManager, worldRay, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, repairTerrain, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, farPlane, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, save, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, quickSave, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, update, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, render, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, pause, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, showEditorRenderables, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, showUDOLinks, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, revealSelection, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, isChunkSelected, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, selectAll, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, isSceneBrowserFocused, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, cursorOverGraphicsWnd, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, importDataGUI, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, exportDataGUI, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, rightClick, WorldEditor )
PY_MODULE_STATIC_METHOD( WorldManager, setReadyForScreenshotCallback, WorldEditor )

/*~ function WorldEditor.worldRay
 *  @components{ worldeditor }
 *
 *  This function returns the world ray.
 *
 *  @return Returns the world ray from the view frustrum to the mouse position.
 */
PyObject * WorldManager::py_worldRay( PyObject * args )
{
    BW_GUARD;

    return Script::getData( s_pInstance->worldRay_ );
}


/*~ function WorldEditor.repairTerrain
 *  @components{ worldeditor }
 *
 *  This function puts back "terrain" resources inside a .chunk file if its
 *  corresponding .cdata file contains terrain.
 */
PyObject * WorldManager::py_repairTerrain( PyObject * args )
{
    BW_GUARD;

    GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
    for( int i = dirMap->minGridX(); i <= dirMap->maxGridX(); ++i )
    {
        for( int j = dirMap->minGridY(); j <= dirMap->maxGridY(); ++j )
        {
            BW::string prefix = dirMap->path() + dirMap->outsideChunkIdentifier( i, j );
            DataSectionPtr cs = BWResource::openSection( prefix + ".chunk" );
            DataSectionPtr ds = BWResource::openSection( prefix + ".cdata" );
            if( cs && ds && !cs->openSection( "terrain" ) && ds->openSection( "terrain" ) )
            {
                cs->newSection( "terrain" )->writeString( "resource",
                    dirMap->outsideChunkIdentifier( i, j ) + ".cdata/terrain" );
                cs->save();
            }
        }
    }

    Py_RETURN_NONE;
}


/*~ function WorldEditor.farPlane
 *  @components{ worldeditor }
 *
 *  This function queries and sets the far plane distance.
 *
 *  @param farPlane Optional float value. The distance to set the far plane to.
 *
 *  @return If the farPlane parameter was not supplied, then this function returns the 
 *          currently set far plane distance, otherwise returns the new far plane distance.
 */
PyObject * WorldManager::py_farPlane( PyObject * args )
{
    BW_GUARD;

    float nfp = -1.f;
    if (!PyArg_ParseTuple( args, "|f", &nfp ))
    {
        //There was not a single float argument,
        //therefore return the far plane
        return PyFloat_FromDouble( s_pInstance->farPlane() );
    }

    if (nfp != -1) s_pInstance->farPlane( nfp );

    return Script::getData( s_pInstance->farPlane() );
}

/*~ function WorldEditor.update
 *  @components{ worldeditor }
 *
 *  This function forces an update to be called in WorldEditor. 
 *  Usually called every frame, but it still receives a dTime 
 *  value which informs the update function how much time has passed 
 *  since the last update call.
 *
 *  @param dTime    The amount of time to be passed since the last update. 
 *                  Default value is one frame (1/30s).
 */
PyObject * WorldManager::py_update( PyObject * args )
{
    BW_GUARD;

    float dTime = 0.033f;

    if (!PyArg_ParseTuple( args, "|f", &dTime ))
    {
        PyErr_SetString( PyExc_TypeError, "WorldEditor.update() "
            "takes only an optional float argument for dtime" );
        return NULL;
    }

    s_pInstance->update( dTime );

    Py_RETURN_NONE;
}


/*~ function WorldEditor.render
 *  @components{ worldeditor }
 *  
 *  This function forces WorldEditor to render everything on the scene. 
 *  Usually called every frame, but it still receives a dTime value 
 *  which informs the renderer how much time has passed since the last 
 *  render call.
 *
 *  @param dTime    The amount of time that has passed since the last 
 *                  render call. Default value is one frame (1/30s).
 */
PyObject * WorldManager::py_render( PyObject * args )
{
    BW_GUARD;

    float dTime = 0.033f;

    if (!PyArg_ParseTuple( args, "|f", &dTime ))
    {
        PyErr_SetString( PyExc_TypeError, "WorldEditor.render() "
            "takes only an optional float argument for dtime" );
        return NULL;
    }

    s_pInstance->render( dTime );

    Py_RETURN_NONE;
}


/*~ function WorldEditor.pause
 *  @components{ worldeditor }
 *  
 *  This function forces WorldEditor to render everything on the scene. 
 *  Usually called every frame, but it still receives a dTime value 
 *  which informs the renderer how much time has passed since the last 
 *  render call.
 *
 *  @param dTime    The amount of time that has passed since the last 
 *                  render call. Default value is one frame (1/30s).
 */
PyObject * WorldManager::py_pause( PyObject * args )
{
    BW_GUARD;

    bool pause = true;
    if (!PyArg_ParseTuple(args, "b", &pause))
    {
        PyErr_SetString( PyExc_TypeError, "WorldEditor.pause: "
            "Argument parsing error: Expected a boolean" );
        return NULL;
    }

    WorldEditorApp::instance().mfApp()->pause( pause );

    Py_RETURN_NONE;
}


/*~ function WorldEditor.showEditorRenderables
 *  @components{ worldeditor }
 *  
 *  This function enables or disables the rendering of
 *  editor specific items like gizmos and selection boxes.
 *
 *  @param show true = show, false = don't show
 */
PyObject * WorldManager::py_showEditorRenderables( PyObject* args )
{
    BW_GUARD;

    bool showEditorRenderables = true;
    if (!PyArg_ParseTuple( args, "b", &showEditorRenderables ))
    {
        PyErr_SetString( PyExc_TypeError, "WorldEditor.showEditorRenderables: "
            "Argument parsing error: Expected a boolean" );
        return NULL;
    }

    WorldManager::instance().enableEditorRenderables( showEditorRenderables );
    Py_RETURN_NONE;
}


/*~ function WorldEditor.showUDOLinks
 *  @components{ worldeditor }
 *  
 *  This function enables or disables the rendering of
 *  links between UDOs.
 *
 *  @param show true = show, false = don't show
 */
PyObject * WorldManager::py_showUDOLinks( PyObject* args )
{
    BW_GUARD;

    bool showUDOLinks = true;
    if (!PyArg_ParseTuple( args, "b", &showUDOLinks ))
    {
        PyErr_SetString( PyExc_TypeError, "WorldEditor.showUDOLinks: "
            "Argument parsing error: Expected a boolean" );
        return NULL;
    }

    EditorChunkLink::enableDraw( showUDOLinks );
    Py_RETURN_NONE;
}


/*~ function WorldEditor.save
 *  @components{ worldeditor }
 *
 *  This function forces a full save and process all operation.
 */
PyObject * WorldManager::py_save( PyObject * args )
{
    BW_GUARD;

    s_pInstance->save();

    Py_RETURN_NONE;
}

/*~ function WorldEditor.quickSave
 *  @components{ worldeditor }
 *
 *  This function forces a quick save operation.
 */
PyObject * WorldManager::py_quickSave( PyObject * args )
{
    BW_GUARD;

    s_pInstance->quickSave();

    Py_RETURN_NONE;
}

SelectionOperation::SelectionOperation( const BW::vector<ChunkItemPtr>& before, const BW::vector<ChunkItemPtr>& after ) :
    UndoRedo::Operation( 0 ), before_( before ), after_( after )
{
    BW_GUARD;

    for( BW::vector<ChunkItemPtr>::iterator iter = before_.begin(); iter != before_.end(); ++iter )
        addChunk( (*iter)->chunk() );
    for( BW::vector<ChunkItemPtr>::iterator iter = after_.begin(); iter != after_.end(); ++iter )
        addChunk( (*iter)->chunk() );
}

void SelectionOperation::undo()
{
    BW_GUARD;

    //ChunkItemPtrs to ChunkVLO objects are only temporary
    //and they are re-generated by the VeryLargeObject when it is recreated.
    //As such, depending on when the references are released, it is possible
    //for the saved selection to hold ChunkItemPtrs to ChunkVLO's that are 
    //not registered to a chunk.
    //However, since they still hold references to the parent VeryLargeObject,
    //we can replace them in the saved selection with the actual children
    //of the VeryLargeObject
    BW::set< ChunkItemPtr > uniqueItems;
    for ( BW::vector<ChunkItemPtr>::const_iterator it = before_.begin();
        it != before_.end(); it++ )
    {
        ChunkItemPtr itemPtr = *it;
        if (itemPtr->chunk()== NULL &&
            itemPtr->edIsVLO())
        {
            ChunkVLO * chunkVLO =
                dynamic_cast< ChunkVLO * >( itemPtr.get() );
            if (chunkVLO != NULL &&
                chunkVLO->object())
            {
                VeryLargeObject::ChunkItemList & itemList
                    = chunkVLO->object()->chunkItems();
                uniqueItems.insert( itemList.begin(), itemList.end() );
                continue;
            }
        }
        uniqueItems.insert( itemPtr );
    }
    BW::vector< ChunkItemPtr > before;
    before.insert( before.begin(), uniqueItems.begin(), uniqueItems.end() );
    WorldManager::instance().setSelection( before, false );
    UndoRedo::instance().add( new SelectionOperation( after_, before ) );
}

bool SelectionOperation::iseq( const UndoRedo::Operation & oth ) const
{
    BW_GUARD;

    // these operations never replace each other
    return false;
}

/*~ function WorldEditor.revealSelection
 *  @components{ worldeditor }
 *  
 *  This function informs WorldEditor what is currently selected,
 *  i.e., what object/s such as shells, models, particle systems, etc. 
 *  are currently selected.
 *
 *  @param selection    A ChunkItemRevealer object to the selected object(s).
 */
PyObject * WorldManager::py_revealSelection( PyObject * args )
{
    BW_GUARD;

    if (s_pInstance->revealingSelection_)
    {
        Py_RETURN_NONE;
    }

    s_pInstance->revealingSelection_ = true;

    PyObject * p;
    if (PyArg_ParseTuple( args, "O", &p ))
    {
        if (ChunkItemRevealer::Check(p))
        {
            ChunkItemRevealer * revealer = static_cast<ChunkItemRevealer *>( p );

            BW::vector<ChunkItemPtr> selectedItems = s_pInstance->selectedItems_;

            BW::vector<ChunkItemPtr> newSelection;
            revealer->reveal( newSelection );

            s_pInstance->updateSelection( newSelection );

            s_pInstance->calculateSnaps();

            bool different = selectedItems.size() != s_pInstance->selectedItems_.size();
            if( !different )
            {
                std::sort( selectedItems.begin(), selectedItems.end() );
                std::sort( s_pInstance->selectedItems_.begin(), s_pInstance->selectedItems_.end() );
                different = ( selectedItems != s_pInstance->selectedItems_ );
            }
            if( different )
            {
                UndoRedo::instance().add( new SelectionOperation( selectedItems, s_pInstance->selectedItems_ ) );

                if( !s_pInstance->settingSelection_ )
                    UndoRedo::instance().barrier( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SELECTION_CHANGE"), false );
            }


            //TODO : put back in when page scene is working correctly
            //PageScene::instance().updateSelection( s_instance->selectedItems_ );
        }
    }

    s_pInstance->revealingSelection_ = false;

    Py_RETURN_NONE;
}

/*~ function WorldEditor.isChunkSelected
 *  @components{ worldeditor }
 *
 *  This function queries whether a shell is currently selected.
 *
 *  @return Returns True if a shell is selected, False otherwise.
 */
PyObject * WorldManager::py_isChunkSelected( PyObject * args )
{
    BW_GUARD;

    return Script::getData( WorldManager::instance().isChunkSelected( true /* force now */) );
}

/*~ function WorldEditor.selectAll
 *  @components{ worldeditor }
 *
 *  This function selects all editable items in all loaded chunks.
 *
 *  @param ignoreVis    (Optional) If True, it allows selecting hidden items.
 *  @param ignoreFrozen (Optional) If True, it allows selecting frozen items.
 *  @param ignoreFilter (Optional) If True, it ignores the Selection Filters.
 *  @return Returns a group of the selected chunk items in the form of a ChunkItemGroup object.
 */
PyObject * WorldManager::py_selectAll( PyObject * args )
{
    BW_GUARD;

    ChunkItemRevealer::ChunkItems allItems;
    EditorChunkItem::updateSelectionMark();

    bool ignoreVis = false;
    bool ignoreFrozen = false;
    bool ignoreFilters = false;

    // grab a optional arguments for including hidden/frozen and/or ignore the
    // Selection Filter
    PyArg_ParseTuple( args, "|bbb", &ignoreVis, &ignoreFrozen, &ignoreFilters );

    ScopedSelectionFilter selectionFilter;
    if (ignoreFilters)
    {
        selectionFilter.start( SelectionFilter::SELECT_ANY, "", "" );
    }

    ChunkItemRevealer::ChunkItems intermediateItems;
    if (!SceneBrowser::instance().currentItems( intermediateItems ))
    {
        // Not set from the scene browser, so get all the items from the chunks.
        allChunkItems( intermediateItems );
    }

    // Filter items.
    filterSelectableItems( intermediateItems, ignoreVis, ignoreFrozen, allItems );

    return new ChunkItemGroup( allItems );
}


/*~ function WorldEditor.isSceneBrowserFocused
 *  @components{ worldeditor }
 *
 *  This function returns whether or not the Scene Browser is in focus.
 *
 *  @return Returns True (1) if the SceneBrowser is in focus.
 */
PyObject * WorldManager::py_isSceneBrowserFocused( PyObject * args )
{
    BW_GUARD;

    // Select hidden objects if we are in the scene 
    bool ret = SceneBrowser::instance().isFocused();

    return PyInt_FromLong( (long)ret );
}


/*~ function WorldEditor.cursorOverGraphicsWnd
 *  @components{ worldeditor }
 *
 *  This function queries whether the mouse cursor is currently over 
 *  graphics window. This is usually used to see whether WorldEditor
 *  should be expecting input from the user.
 *
 *  @return Returns True (1) if the cursor is over the graphics window, False (0) otherwise.
 *
 *  Code Example:
 *  @{
 *  def onKeyEvent( ... ):
 *      if not WorldEditor.cursorOverGraphicsWnd():
 *          return 0
 *      ...
 *  @}
 */
PyObject * WorldManager::py_cursorOverGraphicsWnd( PyObject * args )
{
    BW_GUARD;

    return PyInt_FromLong((long)s_pInstance->cursorOverGraphicsWnd());
}


/*~ function WorldEditor.importDataGUI
 *  @components{ worldeditor }
 *
 *  This function enables the TerrainImport Tool Mode.
 */
PyObject * WorldManager::py_importDataGUI( PyObject * args )
{
    BW_GUARD;

    PanelManager::instance().setToolMode( L"TerrainImport" );

    Py_RETURN_NONE;
}

/*~ function WorldEditor.rightClick
 *  @components{ worldeditor }
 *
 *  This function opens an item's context menu. It is usually called
 *  when right clicking an item in the world.
 *
 *  @param selection    A ChunkItemRevealer object to the selected object. 
 */
PyObject * WorldManager::py_rightClick( PyObject * args )
{
    BW_GUARD;

    PyObject * p;
    if (PyArg_ParseTuple( args, "O", &p ))
    {
        if (ChunkItemRevealer::Check(p))
        {
            ChunkItemRevealer * revealer = static_cast<ChunkItemRevealer *>( p );
            BW::vector<ChunkItemPtr> items;
            revealer->reveal( items );

            Vector3 cursorPos;

            if (ToolManager::instance().tool() && ToolManager::instance().tool()->locator())
            {
                cursorPos = ToolManager::instance().tool()->locator()->transform().applyToOrigin();
            }

            PopupMenu menu;

            static const int commandBaseId = 1;
            static const int idProperties =         0xFF00;
            static const int idSeeInSceneBrowser =  0xFF01;
            static const int idHide =               0xFF02;
            static const int idUnhideAllFilter =    0xFF03;
            static const int idUnhideAll =          0xFF04;
            static const int idFreeze =             0xFF05;
            static const int idUnfreezeAllFilter =  0xFF06;
            static const int idUnfreezeAll =        0xFF07;
            static const int idVisLights =          0xFF08;
            static const int idClearVisLights =     0xFF09;

            ChunkItemPtr item;
            BW::vector< ChunkItemPtr > curSel = instance().selectedItems_;
            BW::vector< ChunkItemPtr > * multiSelectItems = NULL;
            if (items.size() == 1)
            {
                item = items[0];
                // must copy, selection changes underneath
                if (std::find( curSel.begin(), curSel.end(), item ) != curSel.end())
                {
                    multiSelectItems = &curSel;
                }
                else
                {
                    multiSelectItems = &items;
                }

                int numCmds = PopupMenuHelper::buildCommandMenu(
                                                item, commandBaseId, menu );

                if (item->edCanAddSelection())
                {
                    if (numCmds > 0)
                    {
                        menu.addSeparator();
                    }

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_HIDE"),
                        idHide );

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNHIDE_ALL_FILTER"),
                        idUnhideAllFilter );

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNHIDE_ALL"),
                        idUnhideAll );

                    menu.addSeparator();

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_FREEZE"),
                        idFreeze );

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNFREEZE_ALL_FILTER"),
                        idUnfreezeAllFilter );

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNFREEZE_ALL"),
                        idUnfreezeAll );

                    menu.addSeparator();

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/SEE_IN_SCENE_BROWSER"),
                        idSeeInSceneBrowser );

                    menu.addItem(
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/PROPERTIES"),
                        idProperties );
                }
            }
            else
            {
                multiSelectItems = &curSel;

                menu.addItem(
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNHIDE_ALL_FILTER"),
                    idUnhideAllFilter );

                menu.addItem(
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNHIDE_ALL"),
                    idUnhideAll );

                menu.addSeparator();

                menu.addItem(
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNFREEZE_ALL_FILTER"),
                    idUnfreezeAllFilter );

                menu.addItem(
                    Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RCLK_UNFREEZE_ALL"),
                    idUnfreezeAll );
            }

            size_t numLightVisItems = LightContainerDebugger::instance()->numItems();
            if ( !items.empty() || numLightVisItems > 0)
            {
                menu.addSeparator();
                if (!items.empty())
                {
                    menu.addItem( 
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/VIS_LIGHT_CONTRIBUTION"), 
                        idVisLights );
                }

                if (numLightVisItems > 0)
                {
                    menu.addItem( 
                        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CLEAR_LIGHT_CONTRIBUTION_VIS"), 
                        idClearVisLights );
                }
            }


            if (!menu.empty())
            {
                ShowCursor( TRUE );

                // Remove input focus so that keyup events get forced through now.
                WorldEditorApp::instance().mfApp()->handleSetFocus( false );

                int result = menu.doModal( WorldManager::instance().hwndGraphics() );

                ShowCursor( FALSE );

                // Consume input, otherwise input given while the popup menu was visible
                // would be sent to the main window
                WorldEditorApp::instance().mfApp()->consumeInput();

                if (result)
                {
                    if (result == idProperties)
                    {
                        WorldManager::instance().setSelection( *multiSelectItems );

                        PanelManager::instance().showPanel( PageProperties::contentID, true );
                    }
                    else if (result == idSeeInSceneBrowser)
                    {
                        WorldManager::instance().setSelection( items );

                        SceneBrowser::instance().scrollTo( items[0] );
                    }
                    else if (result == idHide)
                    {
                        ChunkItemPlacer::hideChunkItems( *multiSelectItems, true, false );
                    }
                    else if (result == idUnhideAllFilter || result == idUnhideAll)
                    {
                        EditorChunkItem::updateSelectionMark();
                        ChunkItemRevealer::ChunkItems allItems;
                        ChunkItemRevealer::ChunkItems intermediateItems;
                        ScopedSelectionFilter filterSetter;
                        if (result == idUnhideAll)
                        {
                            // select all, regardless of the selection filter
                            filterSetter.start( SelectionFilter::SELECT_ANY, "", "" );
                        }
                        allChunkItems( intermediateItems );
                        filterSelectableItems( intermediateItems, true, true, allItems );
                        ChunkItemPlacer::hideChunkItems( allItems, false, true );
                    }
                    else if (result == idFreeze)
                    {
                        ChunkItemPlacer::freezeChunkItems( *multiSelectItems, true, false );
                    }
                    else if (result == idUnfreezeAllFilter || result == idUnfreezeAll)
                    {
                        EditorChunkItem::updateSelectionMark();
                        ChunkItemRevealer::ChunkItems allItems;
                        ChunkItemRevealer::ChunkItems intermediateItems;
                        ScopedSelectionFilter filterSetter;
                        if (result == idUnfreezeAll)
                        {
                            // select all, regardless of the selection filter
                            filterSetter.start( SelectionFilter::SELECT_ANY, "", "" );
                        }
                        allChunkItems( intermediateItems );
                        filterSelectableItems( intermediateItems, true, true, allItems );
                        ChunkItemPlacer::freezeChunkItems( allItems, false, true );
                    }
                    else if (result == idVisLights)
                    {
                        WorldManager::instance().setSelection( *multiSelectItems );

                        LightContainerDebugger::instance()->setItems( 
                                WorldManager::instance().selectedItems() 
                            );
                    }
                    else if (result == idClearVisLights)
                    {
                        LightContainerDebugger::instance()->clearItems();
                    }
                    else if (item)
                    {
                        item->edExecuteCommand( "", result - commandBaseId );
                    }
                }
            }
        }
    }

    Py_RETURN_NONE;
}

/**
 * called when a specified number of frames were rendered.
 */
void WorldManager::readyForScreenshotCallback()
{
    if (readyForScreenshotCallback_)
    {
        readyForScreenshotCallback_.callFunction( ScriptErrorPrint() );
        readyForScreenshotCallback_ = ScriptObject();
    }
}

/**
 * called when semi dynamic shadows rendering has completed.
 */
void WorldManager::shadowsRenderedCallback()
{
    WorldManager::instance().framesToDelayBeforeCallback_ =
        s_delayedFrameCallbackDelay;
    WorldManager::instance().frameDelayCallback_ =
        &WorldManager::readyForScreenshotCallback;
}

/**
 * called when a few frames elapsed after shadow processing started.
 */
void WorldManager::shadowsStartedCallback()
{
    Moo::ShadowManager* shadows =
        Renderer::instance().pipeline()->dynamicShadow();

    shadows->setGenerationFinishedCallback(
        &WorldManager::shadowsRenderedCallback );
}

/**
 * called when flora rendering has completed.
 */
void WorldManager::floraRenderedCallback()
{
    Moo::ShadowManager* shadows =
        Renderer::instance().pipeline()->dynamicShadow();

    if (shadows && shadows->isSemiDynamicShadowsEnabled())
    {
        shadows->resetAll();
        WorldManager::instance().framesToDelayBeforeCallback_ =
            s_delayedFrameCallbackDelay;
        WorldManager::instance().frameDelayCallback_ =
            &WorldManager::shadowsStartedCallback;
    }
    else
    {
        shadowsRenderedCallback();
    }
}

/**
 * called when loading has completed. Receives time.
 *
 * @param outputString string containing a message with the time to load.
 *
 */
void WorldManager::chunksLoadedCallback( const char* outputString )
{
    Flora::setBlocksCompletedCallback( &WorldManager::floraRenderedCallback );
}
    
/*~ function WorldEditor.setReadyForScreenshotCallback
 *  @components{ worldeditor }
 *
 *  This function sets a python callback for when all the chunks have loaded.
 *
 *  @param callback A function object to the selected object. 
 */
bool WorldManager::setReadyForScreenshotCallback( const ScriptObject& callback )
{
    BW_GUARD;

    if (!callback.isCallable())
    {
        PyErr_SetString( PyExc_TypeError, "Argument is not callable" );
        return false;
    }

    readyForScreenshotCallback_ = callback;

    SimpleGUI::instance().setUpdateEnabled( false );
    ChunkManager::instance().processPendingChunks();
    ChunkManager::instance().startTimeLoading(
        &WorldManager::chunksLoadedCallback, true );

    return true;
}

/*~ function WorldEditor.outsideChunkIdentifier
 *  @components{ worldeditor }
 *
 *  This function gets the chunk identifier that the camera currently resides in.
 *
 *  @return String  The identifier of the camera chunk, empty string if not valid.
 */
static BW::string cameraChunkIdentifier()
{
    Chunk * pChunk = ChunkManager::instance().cameraChunk();
    if (pChunk)
    {
        return pChunk->identifier();        
    }
    else
    {
        return "";
    }
}

PY_AUTO_MODULE_FUNCTION( RETDATA, cameraChunkIdentifier, END, WorldEditor )

/*~ function WorldEditor.outsideChunkIdentifier
 *  @components{ worldeditor }
 *
 *  This function converts the given x, z position into an outdoor chunk
 *  identifier string (as used by the chunk filenames). If the checkBounds parameter
 *  is set to True, then this will return an empty string if the given coordinates
 *  fall outside the space boundaries.
 *
 *  @param  Vector3     Position in the space to test against.
 *  @param  Boolean     Whether or not to check if the point falls within space bounds.
 *
 *  @return String      The identifier of the outside chunk containing the given point.
 */
static BW::string outsideChunkIdentifier( const Vector3& pos, bool checkBounds )
{
    GeometryMapping* mapping = WorldManager::instance().geometryMapping();
    if (mapping)
    {
        return mapping->outsideChunkIdentifier( pos, true );
    }
    else
    {
        return "";
    }
}

PY_AUTO_MODULE_FUNCTION( RETDATA, outsideChunkIdentifier, ARG( Vector3, OPTARG( bool, true, END ) ), WorldEditor )


/*~ function WorldEditor.py_exportDataGUI
 *  @components{ worldeditor }
 *
 *  This function enables the TerrainImport Tool Mode.
 */
PyObject * WorldManager::py_exportDataGUI( PyObject *args )
{
    BW_GUARD;

    PanelManager::instance().setToolMode( L"TerrainImport" );

    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------

void WEChunkFinder::operator()( ToolPtr tool, float buffer )
{
    BW_GUARD;

    if ( tool->locator() )
    {
        float halfSize = buffer + tool->size() / 2.f;
        Vector3 start( tool->locator()->transform().applyToOrigin() -
                        Vector3( halfSize, 0.f, halfSize ) );
        Vector3 end( tool->locator()->transform().applyToOrigin() +
                        Vector3( halfSize, 0.f, halfSize ) );

        EditorChunk::findOutsideChunks(
            BoundingBox( start, end ), tool->relevantChunks() );

        tool->currentChunk() = EditorChunk::findOutsideChunk(
            tool->locator()->transform().applyToOrigin() );
    }
}
//---------------------------------------------------------------------------
bool WorldManager::changeSpace( const BW::string& space, bool reload )
{
    BW_GUARD;

    static int id = 1;

    if( currentSpace_ == space )
        return true;

    if( !BWResource::fileExists( space + '/' + SPACE_SETTING_FILE_NAME ) )
        return false;

    BWResource::instance().purge( space + "/" + SPACE_SETTING_FILE_NAME, true );
    DataSectionPtr spaceSettings = BWResource::openSection( space + "/" + SPACE_SETTING_FILE_NAME );
    if (!spaceSettings)
        return false;

    // It's possible that the space settings file exists but was totally
    // corrupted due to a version control conflict.  It may in fact be read as
    // a BinSection with the file's contents as the data as a result.  To
    // prevent this case we check for the existance of the "bounds" section.
    DataSectionPtr boundsSection = spaceSettings->openSection("bounds");
    if (!boundsSection)
        return false;

    EditorChunkCache::forwardReadOnlyMark();

    if( !reload )
    {
        if( spaceLock_ != INVALID_HANDLE_VALUE )
            CloseHandle( spaceLock_ );
        BW::wstring wspace;
        bw_utf8tow( BWResolver::resolveFilename( space + "/space.lck" ), wspace );
        spaceLock_ = CreateFile( wspace.c_str(), GENERIC_READ | GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL );
        if( spaceLock_ == INVALID_HANDLE_VALUE )
        {
            MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
            MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_TITLE"),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/UNABLE_TO_OPEN_SPACE", space ),
                Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OK") );
            mb.doModal( mainFrame->m_hWnd );
            return false;
        }
    }

    readOnlyTerrainBlocks_.clear();

    renderDisabled_ = true;

    stopBackgroundCalculation();
    ChunkProcessorManager::tick();

    if ( romp_ )
        romp_->enviroMinder().deactivate();

    if( !currentSpace_.empty() )
    {
        setSelection( BW::vector<ChunkItemPtr>(), false );
        setSelection( BW::vector<ChunkItemPtr>(), true );
    }

    ChunkManager::instance().switchToSyncMode( true );

    if( !reload )
    {
        if (inited_)
        {
            // Clear the message list before changing space, but not if it's
            // the first time.
            MsgHandler::instance().clear();
        }

        if( conn_.enabled() )
        {
            CWaitCursor wait;
            if( conn_.changeSpace( space ) )
                WaitDlg::overwriteTemp( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CONNECT_TO_BWLOCKD_DONE", conn_.host() ), 500 );
            else
                WaitDlg::overwriteTemp( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CONNECT_TO_BWLOCKD_FAILED", conn_.host() ), 500 );
        }
        else
            conn_.changeSpace( space );
    }

    LightContainerDebugger::instance()->clearItems();
    EditorChunkOverlapper::drawList.clear();
    AmortiseChunkItemDelete::instance().purge();
    EditorChunkLink::flush( 0, true );

    BWResource::instance().purgeAll();

    terrainFormatStorage_.clear();
    SpaceManager::instance().clearSpaces();
    ChunkManager::instance().camera( Matrix::identity, NULL );

    // Clear the linker manager lists
    WorldManager::instance().linkerManager().reset();

    ClientSpacePtr pSpace = SpaceManager::instance().findOrCreateSpace( id );
    ChunkSpacePtr chunkSpace = ChunkManager::instance().space( id );
    ++id;
    // TODO: find out why addMapping is passing a matrix as a float and fix it.
    Matrix* nonConstIdentity = const_cast<Matrix*>(&Matrix::identity);
    mapping_ = chunkSpace->addMapping( SpaceEntryID(), (float*)nonConstIdentity, space );
    if( !mapping_ )
    {
        ChunkManager::instance().switchToSyncMode( false );
        renderDisabled_ = false;
        return false;
    }

    currentSpace_ = space;

    if( !reload && PanelManager::pInstance() != NULL )
        PanelManager::instance().setToolMode( L"Objects" );

    resetChangedLists();

    ChunkManager::instance().switchToSyncMode( false );
    ChunkManager::instance().camera( Matrix::identity, chunkSpace );
    DeprecatedSpaceHelpers::cameraSpace( pSpace );

    ChunkProcessorManager::onChangeSpace();

    ChunkManager::instance().tick( 0.f );

    ToolManager::instance().changeSpace( getWorldRay( currentCursorPosition() ) );

    DataSectionPtr localSpaceSettings = BWResource::openSection( space + "/" + SPACE_LOCAL_SETTING_FILE_NAME );
    if( !reload )
    {
        Vector3 dir( 0.f, 0.f, 0.f ), pos( 0.f, 2.f, 0.f );
        if( localSpaceSettings )
        {
            pos = localSpaceSettings->readVector3( "startPosition", Vector3( 0.f, 2.f, 0.f ) );
            dir = localSpaceSettings->readVector3( "startDirection" );
        }
        Matrix m;
        m.setIdentity();
        m.setRotate( dir[2], dir[1], dir[0] );
        m.translation( pos );
        m.invert();
        Moo::rc().view( m );
    }

    // set the window title to the current space name
    BW::wstring wspace;
    bw_utf8tow( space + " - ", wspace );
    AfxGetMainWnd()->SetWindowText( ( wspace + Localise(L"WORLDEDITOR/APPLICATION_NAME" ) ).c_str() );

    spaceManager_->addSpaceIntoRecent( space );

    if (WorldEditorCamera::pInstance())
    {
        WorldEditorCamera::instance().respace( Moo::rc().view() );
    }

    DataSectionPtr terrainSettings = spaceSettings->openSection( "terrain" );
    if ( terrainSettings == NULL ||
        ( terrainSettings != NULL && terrainSettings->readInt( "version", 0 ) == 2 ) )
    {
        // If it doesn't have a terrain section in the space.settings, or
        // if the terrain version in the terrain section is '2', then
        // generate a new terrain section using the appropriate values
        // because the old space.settings value is wrong.
        if ( terrainSettings != NULL ) 
        {
            // discard the old space.settings section
            spaceSettings->deleteSection( "terrain" );
        }
        terrainSettings = spaceSettings->openSection( "terrain", true );

        uint32 terrainVersion = getTerrainVersion();
        Terrain::TerrainSettingsPtr pTempSettings = new Terrain::TerrainSettings;
        pTempSettings->initDefaults(spaceSettings->readFloat( "chunkSize", DEFAULT_GRID_RESOLUTION ));
        pTempSettings->version(terrainVersion);
        if (terrainVersion == Terrain::TerrainSettings::TERRAIN2_VERSION)
        {
            // set to old defaults for terrain 2
            pTempSettings->heightMapSize( 128 );
            pTempSettings->normalMapSize( 128 );
            pTempSettings->holeMapSize( 25 );
            pTempSettings->shadowMapSize( 32 );
            pTempSettings->blendMapSize( 256 );
            pTempSettings->save( terrainSettings );
            spaceSettings->save();
        }
        else if (terrainVersion == Terrain::TerrainSettings::TERRAIN1_VERSION)
        {
            ERROR_MSG( "Couldn't create space.settings/terrain section:"
                "terrain version %d is no longer supported.\n",
                terrainVersion );
        }
        else
        {
            ERROR_MSG( "Couldn't create space.settings/terrain section: unknown terrain version.\n" );
        }
    }

    if ( romp_ )
        romp_->enviroMinder().activate();
    this->setTimeFromOptions();
    Flora::floraReset();
    UndoRedo::instance().clear();

    updateRecentFile();

    if( !reload && PanelManager::pInstance() != NULL )
        PanelManager::instance().setDefaultToolMode();

    secsPerHour_ = romp_->timeOfDay()->secondsPerGameHour();

    romp_->changeSpace();

    update( 0.f );

    unsigned int spaceWidth = 0;
    unsigned int spaceHeight = 0;

    if (spaceSettings)
    {
        int minX = spaceSettings->readInt( "bounds/minX", 1 );
        int minY = spaceSettings->readInt( "bounds/minY", 1 );
        int maxX = spaceSettings->readInt( "bounds/maxX", -1 );
        int maxY = spaceSettings->readInt( "bounds/maxY", -1 );

        spaceWidth  = maxX - minX + 1;
        spaceHeight = maxY - minY + 1;
        SpaceInformation spaceInfo( space, GridCoord( minX, minY ), spaceWidth, spaceHeight);

        SpaceMap::instance().spaceInformation(spaceInfo);

        forcedLodMap_.setSpaceInformation(spaceInfo);

        chunkWatcher_->onNewSpace(minX, minY, maxX, maxY);
    }

    Personality::instance().callMethod( "onCameraSpaceChange",
        ScriptArgs::create( chunkSpace->id(), new PyDataSection( spaceSettings ) ),
        ScriptErrorPrint( "Personality onCameraSpaceChange: " ),
        /* allowNullMethod */ true );

    if (PanelManager::pInstance())
    {
        PanelManager::instance().onNewSpace( spaceWidth, spaceHeight ); 
    }

    localSpaceSettings = BWResource::openSection( Options::getOptionString( "space/mru0" ) + '/' +
        SPACE_LOCAL_SETTING_FILE_NAME );

    renderDisabled_ = false;
    return true;
}

bool WorldManager::handleGUIAction( GUI::ItemPtr item )
{
    BW_GUARD;

    const BW::string& action = item->action();

    if (action == "changeSpace" )               return changeSpace( item );
    else if(action == "newSpace")               return newSpace( item );
    else if(action == "editSpace")              return editSpace( item );
    else if(action == "recreateSpace")          return recreateSpace( item );
    else if(action == "recentSpace")            return recentSpace( item );
    else if(action == "clearUndoRedoHistory")   return clearUndoRedoHistory( item );
    else if(action == "doExternalEditor")       return doExternalEditor( item );
    else if(action == "doReloadAllTextures")    return doReloadAllTextures( item );
    else if(action == "doReloadAllChunks")      return doReloadAllChunks( item );
    else if(action == "doExit")                 return doExit( item );
    else if(action == "setLanguage")            return setLanguage( item );
    else if(action == "recalcCurrentChunk")     return recalcCurrentChunk( item );

    return false;
}

bool WorldManager::changeSpace( GUI::ItemPtr item )
{
    BW_GUARD;
    
    bool foundSpace = false;
    while (!foundSpace)
    {

        if (!canClose( LocaliseUTF8(
            L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHANGE_SPACE") ))
        {
            break;
        }

        BW::string space = spaceManager_->browseForSpaces( hwndApp_ );
        space = BWResource::dissolveFilename( space );
        if (space.empty())
        {
            break;
        }

        foundSpace = this->changeSpace( space, false );

        // Cannot open space
        if (!foundSpace)
        {
            // Flush errors to messages panel
            this->postPendingErrorMessages();

            // Display error message
            MsgBox mb(
Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_TITLE"),
Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/BAD_SPACE_VERSION_TEXT",
    space ),
Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OK") );

            MainFrame* mainFrame = static_cast< MainFrame * >(
                WorldEditorApp::instance().mainWnd() );
            mb.doModal( mainFrame->m_hWnd );

            // Go back to old space if we had a space open before
            // First time World Editor starts up, current space will be empty
            if (!currentSpace_.empty())
            {
                const BW::string oldSpace = currentSpace_;
                currentSpace_ = space;
                MF_ASSERT( this->changeSpace( oldSpace, false ) );
                foundSpace = true;
                break;
            }
        }
    }

    // If we found a space, assert current space name is not empty
    MF_ASSERT( foundSpace ? !currentSpace_.empty() : true );
    return foundSpace;
}

bool WorldManager::newSpace( GUI::ItemPtr item )
{
    BW_GUARD;

    if( !canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHANGE_SPACE") ) )
        return false;
    NewSpaceDlg dlg;
    bool result = ( dlg.DoModal() == IDOK );
    if( result )
    {
        BW::string nspace;
        bw_wtoutf8( (LPCTSTR)dlg.createdSpace(), nspace );
        result = changeSpace( nspace, false );
    }
    return result;
}


bool WorldManager::editSpace( GUI::ItemPtr item )
{
    BW_GUARD;

    if( !canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/EDIT_SPACE") ) )
        return false;

    SpaceSettings spaceSettings( WorldManager::instance().getCurrentSpace() );

    EditSpaceDlg dlg( spaceSettings );
    dlg.DoModal();
    return true;
}


bool WorldManager::recreateSpace( GUI::ItemPtr item )
{
    BW_GUARD;

    if( !canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/ACTION") ) )
        return false;
    RecreateSpaceDlg dlg;
    bool result = ( dlg.DoModal() == IDOK );
    if( result )
    {
        BW::string nspace;
        bw_wtoutf8( (LPCTSTR)dlg.createdSpace(), nspace );
        result = changeSpace( nspace, false );
    }
    return result;
}

bool WorldManager::recentSpace( GUI::ItemPtr item )
{
    BW_GUARD;

    if( !canClose( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CHANGE_SPACE") ) )
        return false;
    BW::string spaceName = (*item)[ "spaceName" ];
    bool ok = changeSpace( spaceName, false );
    if (!ok)
    {
        ERROR_MSG
        (
            LocaliseUTF8(
                L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/CANNOT_CHANGE_SPACE", 
                spaceName
            ).c_str()
        );
        spaceManager_->removeSpaceFromRecent( spaceName );
        updateRecentFile();
    }
    return ok;
}

bool WorldManager::setLanguage( GUI::ItemPtr item )
{
    BW_GUARD;

    BW::string languageName = (*item)[ "LanguageName" ];
    BW::string countryName = (*item)[ "CountryName" ];

    // Do nothing if we are not changing language
    if (currentLanguageName_ == languageName && currentCountryName_ == countryName)
    {
        return true;
    }

    unsigned int result;
    if (isDirty())
    {
        result = MsgBox( Localise(L"RESMGR/CHANGING_LANGUAGE_TITLE"), Localise(L"RESMGR/CHANGING_LANGUAGE"),
            Localise(L"RESMGR/SAVE_AND_RESTART"), Localise(L"RESMGR/DISCARD_AND_RESTART"),
            Localise(L"RESMGR/RESTART_LATER"), Localise(L"RESMGR/CANCEL") ).doModal();
    }
    else
    {
        result = MsgBox( Localise(L"RESMGR/CHANGING_LANGUAGE_TITLE"), Localise(L"RESMGR/CHANGING_LANGUAGE"),
            Localise(L"RESMGR/RESTART_NOW"), Localise(L"RESMGR/RESTART_LATER"), Localise(L"RESMGR/CANCEL") ).doModal() + 1;
    }
    switch (result)
    {
    case 0:
        Options::setOptionString( "currentLanguage", languageName );
        Options::setOptionString( "currentCountry", countryName );
        quickSave();
        startNewInstance();
        AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
        break;
    case 1:
        Options::setOptionString( "currentLanguage", languageName );
        Options::setOptionString( "currentCountry", countryName );
        forceClean();
        startNewInstance();
        AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
        break;
    case 2:
        Options::setOptionString( "currentLanguage", languageName );
        Options::setOptionString( "currentCountry", countryName );
        currentLanguageName_ = languageName;
        currentCountryName_ = countryName;
        break;
    case 3:
        break;
    }
    return true;
}

bool WorldManager::doReloadAllTextures( GUI::ItemPtr item )
{
    BW_GUARD;

    AfxGetApp()->DoWaitCursor( 1 );
    Moo::TextureManager::instance()->reloadAllTextures();
    AfxGetApp()->DoWaitCursor( 0 );
    return true;
}

bool WorldManager::recalcCurrentChunk( GUI::ItemPtr item )
{
    BW_GUARD;

    Chunk* chunk = ChunkManager::instance().cameraSpace()->
        findChunkFromPoint( Moo::rc().invView().applyToOrigin() );

    if (chunk && EditorChunk::chunkWriteable( *chunk ))
    {
        AfxGetApp()->DoWaitCursor( 1 );

        for (int i = 0; i < ChunkCache::cacheNum(); ++i)
        {
            if (chunk->cache( i ))
            {
                chunk->cache( i )->onChunkChanged( 
                    InvalidateFlags::FLAG_NAV_MESH |
                    InvalidateFlags::FLAG_SHADOW_MAP |
                    InvalidateFlags::FLAG_THUMBNAIL,
                    this );
            }
        }

        SyncMode syncMode;

        processChunk( chunk );

        AfxGetApp()->DoWaitCursor( 0 );
    }

    return true;
}

bool WorldManager::doReloadAllChunks( GUI::ItemPtr item )
{
    BW_GUARD;

    AfxGetApp()->DoWaitCursor( 1 );
    reloadAllChunks( true );
    AfxGetApp()->DoWaitCursor( 0 );
    return true;
}

bool WorldManager::doExit( GUI::ItemPtr item )
{
    BW_GUARD;

    AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
    return true;
}

void WorldManager::updateRecentFile()
{
    BW_GUARD;

    GUI::ItemPtr recentFiles = GUI::Manager::instance()( "/MainMenu/File/RecentFiles" );
    if( recentFiles )
    {
        while( recentFiles->num() )
            recentFiles->remove( 0 );
        for( unsigned int i = 0; i < spaceManager_->num(); ++i )
        {
            BW::stringstream name, displayName;
            name << "mru" << i;
            displayName << '&' << i << "  " << spaceManager_->entry( i );
            GUI::ItemPtr item = new GUI::Item( "ACTION", name.str(), displayName.str(),
                "", "", "", "recentSpace", "", "" );
            item->set( "spaceName", spaceManager_->entry( i ) );
            recentFiles->add( item );
        }
    }
}

void WorldManager::updateLanguageList()
{
    BW_GUARD;

    GUI::ItemPtr languageList = GUI::Manager::instance()( "/MainMenu/Languages/LanguageList" );
    if( languageList )
    {
        while( languageList->num() )
            languageList->remove( 0 );
        for( unsigned int i = 0; i < StringProvider::instance().languageNum(); ++i )
        {
            LanguagePtr l = StringProvider::instance().getLanguage( i );
            BW::wstringstream wname, wdisplayName;
            wname << L"language" << i;
            wdisplayName << L'&' << l->getLanguageName();
            BW::string name, displayName;
            bw_wtoutf8( wname.str(), name );
            bw_wtoutf8( wdisplayName.str(), displayName );
            GUI::ItemPtr item = new GUI::Item( "CHILD", name, displayName,
                "", "", "", "setLanguage", "updateLanguage", "" );
            item->set( "LanguageName", l->getIsoLangNameUTF8() );
            item->set( "CountryName", l->getIsoCountryNameUTF8() );
            languageList->add( item );
        }
    }
}


bool WorldManager::clearUndoRedoHistory( GUI::ItemPtr item )
{
    BW_GUARD;

    UndoRedo::instance().clear();
    return true;
}


unsigned int WorldManager::handleGUIUpdate( GUI::ItemPtr item )
{
    BW_GUARD;

    const BW::string& updater = item->updater();;

    if (updater == "updateRecreateSpace")       return updateRecreateSpace( item );
    else if (updater == "updateUndo")           return updateUndo( item );
    else if (updater == "updateRedo")           return updateRedo( item );
    else if (updater == "updateExternalEditor") return updateExternalEditor( item );
    else if (updater == "updateLanguage")       return updateLanguage( item );

    return 0;
}


/**
 *  This method checks if we should enable or disable the "Save space as" menu.
 *  @param item
 *  @return 1 if it should be enabled, 0 if it should be disabled.
 */
unsigned int WorldManager::updateRecreateSpace( GUI::ItemPtr item )
{
    BW_GUARD;
    return (this->pTerrainSettings()&&
        this->pTerrainSettings()->version() ==
        Terrain::TerrainSettings::TERRAIN2_VERSION) ? 1 : 0;
}


unsigned int WorldManager::updateUndo( GUI::ItemPtr item )
{
    BW_GUARD;

    return UndoRedo::instance().canUndo();
}


unsigned int WorldManager::updateRedo( GUI::ItemPtr item )
{
    BW_GUARD;

    return UndoRedo::instance().canRedo();
}


bool WorldManager::doExternalEditor( GUI::ItemPtr item )
{
    BW_GUARD;

    if( selectedItems_.size() == 1 )
        selectedItems_[ 0 ]->edExecuteCommand( "", 0 );
    return true;
}


unsigned int WorldManager::updateExternalEditor( GUI::ItemPtr item )
{
    return selectedItems_.size() == 1 && !selectedItems_[ 0 ]->edCommand( "" ).empty();
}


unsigned int WorldManager::updateLanguage( GUI::ItemPtr item )
{
    BW_GUARD;

    if (currentLanguageName_.empty())
    {
        currentLanguageName_ = StringProvider::instance().currentLanguage()->getIsoLangNameUTF8();
        currentCountryName_ = StringProvider::instance().currentLanguage()->getIsoCountryNameUTF8();
    }
    return currentLanguageName_ == (*item)[ "LanguageName" ] && currentCountryName_ == (*item)[ "CountryName" ];
}


BW::string WorldManager::get( const BW::string& key ) const
{
    BW_GUARD;

    return Options::getOptionString( key );
}


bool WorldManager::exist( const BW::string& key ) const
{
    BW_GUARD;

    return Options::optionExists( key );
}


void WorldManager::set( const BW::string& key, const BW::string& value )
{
    BW_GUARD;

    Options::setOptionString( key, value );
}


/**
 *  This function draws the terrain texture LOD for the given chunk.
 *
 *  @param chunk        The chunk to update.
 *  @param markDirty    If true then the chunk is added to the changed chunk
 *                      list, if false then it isn't.
 *  @returns            True if the terrain texture LOD could be updated.
 */
bool WorldManager::drawMissingTextureLOD(Chunk *chunk, bool markDirty)
{
    BW_GUARD;

    bool updateOk = false;

    ChunkCache& cc = EditorChunkTerrainLodCache::instance(*chunk);

    if (cc.readyToCalculate( this ))
    {
        updateOk = cc.recalc( this, *this );
    }

    return updateOk;
}


/**
 *  This function draws missing texture LODs.  These can be missing if the
 *  device is lost.
 *
 *  @param complainIfNotDone    If true and redrawing failed then print an
 *                      error message.  If false then no messages are printed.
 *  @param doAll        If true then do all of the missing texture LODs.
 *                      If false then only do one.
 *  @param markDirty    If true then the chunk is added to the dirty list,
 *                      if false then it isn't.
 *  @param progress     If true then a progress bar is displayed, if false then
 *                      no progress bar is displayed.  This can only be set to
 *                      true if doAll is also true.
 */
void WorldManager::drawMissingTextureLODs
    (
    bool        complainIfNotDone, 
    bool        doAll,
    bool        markDirty,
    bool        progress
    )
{
    BW_GUARD;

    const DirtyChunkLists & dirtyChunkLists = this->dirtyChunkLists();
    // Show the progress bar if it has been requested:
    ProgressBarTask *paintTask = NULL;
    if (progress && doAll)
    {
        paintTask = 
            new ProgressBarTask
            ( 
                LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/UPDATE_TEXTURE_LODS"), 
                (float)dirtyChunkLists[ TERRAIN_LOD ].num(),
                false
            );
    }

    // Update texture LODS
    size_t numChunksToDo = doAll ? std::numeric_limits<size_t>::max() : 1;
    bool allChunksDone = false;
    RecursiveMutexHolder lock( dirtyChunkListsMutex_ );

    while (numChunksToDo > 0 && !allChunksDone)
    {
        const DirtyChunkList & dirtyChunkList = dirtyChunkLists[ TERRAIN_LOD ];

        allChunksDone = true;

        for (DirtyChunkList::const_iterator it = dirtyChunkList.begin();
            it != dirtyChunkList.end(); 
            ++it)
        {
            Chunk *chunk = it->second;

            if (chunk && chunk->loaded())
            {
                --numChunksToDo;

                if (drawMissingTextureLOD( chunk, markDirty ))
                {
                    // we've invalidated the entire collection here,
                    // so we cannot rely on iterator comparisons as they
                    // have been invalidated
                    allChunksDone = false;
                    break;
                }

                if (paintTask != NULL)
                {
                    paintTask->step();
                }
            }
        }
    }

    // Cleanup:
    bw_safe_delete(paintTask); 
}


BW::string WorldManager::getCurrentSpace() const
{
    return currentSpace_;
}   


void WorldManager::showBusyCursor()
{
    BW_GUARD;

    // Set the cursor to the arrow + hourglass if there are not yet any loaded
    // chunks, or reset it to the arrow cursor if we were displaying the wait
    // cursor.
    EditorChunkCache::lock();
    bool loadedChunk = EditorChunkCache::chunks_.size() > 0;
    EditorChunkCache::unlock();
    if (waitCursor_ || !loadedChunk)
    {
        WorldManager::instance().setCursor
        (
            loadedChunk 
                ? ::LoadCursor(NULL, IDC_ARROW)
                : ::LoadCursor(NULL, IDC_APPSTARTING)
        );
        waitCursor_ = !loadedChunk;
    }
}


float WorldManager::getMaxFarPlane() const
{
    BW_GUARD;

    return Options::getOptionFloat( "render/maxFarPlane", 5000 );
}

void WorldManager::flushDelayedChanges()
{
    BW_GUARD;

    if (Moo::isRenderThread() && pendingChangedChunks_.size())
    {
        changeMutex_.grab();
        if (pendingChangedChunks_.size())
        {
            BW::list< PendingChangedChunk > tmp = pendingChangedChunks_;
            pendingChangedChunks_.clear();
            changeMutex_.give();

            try
            {
                for (BW::list< PendingChangedChunk >::iterator it = tmp.begin();
                    it != tmp.end(); it ++)
                {
                    changedChunk( (*it).pChunk_, (*it).flags_, *(*it).pChangedItem_ );
                }
            }
            catch( ... )
            {
                ;
            }
        }
        else
        {
            changeMutex_.give();
        }
    }
}


bool WorldManager::changedPostProcessing() const
{
    BW_GUARD;

    return changedPostProcessing_;
}


void WorldManager::changedPostProcessing( bool changed )
{
    BW_GUARD;

    changedPostProcessing_ = changed;
}


bool WorldManager::userEditingPostProcessing() const
{
    BW_GUARD;

    return userEditingPostProcessing_;
}


void WorldManager::userEditingPostProcessing( bool editing )
{
    BW_GUARD;

    userEditingPostProcessing_ = editing;
}


bool WorldManager::createBlankChunkData( BW::string& retPath, bool withTerrain )
{
    BW_GUARD;

    retPath =  BWResource::resolveFilename( s_blankCDataFname );


    DataSectionPtr cdata = new BinSection( retPath, NULL );
    cdata = cdata->convertToZip( retPath );
    if (cdata == NULL)
    {
        return false;
    }

    if (!withTerrain)
    {
        ChunkCleanFlags flags( cdata );
        flags.terrainLOD_ = 1;
        flags.save();

        return cdata->save( retPath );
    }

    Terrain::TerrainSettingsPtr pTerrainSetting = pTerrainSettings();
    uint32 blendSize = pTerrainSetting->blendMapSize();
    // create a new terrain2 block
    Terrain::EditorTerrainBlock2* pTemplateTerrainBlock = new Terrain::EditorTerrainBlock2( pTerrainSetting );
    BW::string error;
    if ( !pTemplateTerrainBlock->create( NULL, &error ) )
    {
        if ( !error.empty() )
            ERROR_MSG( "Failed to create new space: %s\n", error.c_str() );
        return false;
    }

    // add a default texture layer
    int32 idx = static_cast<int32>(pTemplateTerrainBlock->insertTextureLayer( blendSize, blendSize ));
    if ( idx == -1 )
    {
        ERROR_MSG( "Couldn't create default texture for new space.\n" );
        return false;
    }
    else
    {
        Terrain::TerrainTextureLayer& layer = pTemplateTerrainBlock->textureLayer( idx );
        layer.textureName( s_blankTerrainTexture );

        { // Scope for holder
            // set the blends to full white
            Terrain::TerrainTextureLayerHolder holder( &layer, false );
            layer.image().fill(
                std::numeric_limits<Terrain::TerrainTextureLayer::PixelType>::max() );
        }
        // rebuild the texture layers
        pTemplateTerrainBlock->rebuildCombinedLayers();
        pTemplateTerrainBlock->rebuildNormalMap( Terrain::NMQ_NICE );
    }

    Matrix m( Matrix::identity );
    m.translation( Vector3( 0.0f, 0.0f, 0.0f ) );
    // terrain2 needs the texture lod to be calculated here.
    Moo::rc().effectVisualContext().initConstants();

    if ( pTemplateTerrainBlock->rebuildLodTexture( m ) && 
        pTemplateTerrainBlock->saveToCData( cdata ) )
    {
        ChunkCleanFlags flags( cdata );
        flags.terrainLOD_ = 1;
        flags.save();
        cdata->save( retPath );
    }

    bw_safe_delete( pTemplateTerrainBlock );
    
    return true;
}

/**
 * This function expands the current space a specific new size,
 * expanded Chunks would be empty.
 */
bool WorldManager::expandSpace( int westCnt, int eastCnt, int northCnt, int southCnt, bool chunkWithTerrain )
{
    BW_GUARD;

    uint32 curTerrainVer = pTerrainSettings()->version();
    if (curTerrainVer < Terrain::TerrainSettings::TERRAIN2_VERSION)
    {
        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/READ_ONLY_WARN_TEXT"),
            MessageBox( *WorldEditorApp::instance().mainWnd(),
            Localise( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/EXPAND_SPACE_NOT_SUPPORTED" ),
            Localise( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/EXPAND_SPACE_NOT_SUPPORTED_TITLE" ),
            MB_ICONINFORMATION | MB_OK );
        return false;
    }

    if (westCnt < 0 || eastCnt < 0 || northCnt < 0 || southCnt < 0 ||
        ( mapping_->maxGridX() - mapping_->minGridX() + westCnt + eastCnt + 1 >= MAX_CHUNK_SIZE ) ||
            ( mapping_->maxGridY() - mapping_->minGridY() + northCnt + southCnt + 1 >= MAX_CHUNK_SIZE ))
    {
        this->addCommentaryMsg( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/EXPAND_SPACE_SIZE_ERROR" ) );
        return false;
    }

    BW::string spacePath = BWResource::resolveFilename( mapping_->path() );
    BW::string cdataFilePath = "";

    if ( !this->createBlankChunkData( cdataFilePath, chunkWithTerrain ) ) 
    {
        this->addCommentaryMsg( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/EXPAND_SPACE_CDATA_ERROR" ) );
        return false;
    }


    WorldEditorApp::instance().pythonAdapter()->expandSpace( spacePath, cdataFilePath, westCnt, eastCnt, northCnt, southCnt, chunkWithTerrain );
    if (!cdataFilePath.empty())
    {
        std::remove( cdataFilePath.c_str() );
    }
    
    this->quickSave();

    SpaceMap::instance().spaceExpanded();
    this->reloadAllChunks( false );

    this->addCommentaryMsg( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SPACE_EXPANDED" ) );

    return true;
}

void WorldManager::expandSpaceWithDialog()
{
    uint32 curTerrainVer = pTerrainSettings()->version();
    if (curTerrainVer < Terrain::TerrainSettings::TERRAIN2_VERSION)
    {
        Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/READ_ONLY_WARN_TEXT"),
        MessageBox( *WorldEditorApp::instance().mainWnd(),
            Localise( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/EXPAND_SPACE_NOT_SUPPORTED" ),
            Localise( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/EXPAND_SPACE_NOT_SUPPORTED_TITLE" ),
            MB_ICONINFORMATION | MB_OK );
        return;
    }
    ExpandSpaceDlg dlg;
    bool result = ( dlg.DoModal() == IDOK );

    if (result)
    {
        const ExpandSpaceControl::ExpandInfo& info = dlg.expandInfo();
        this->expandSpace( info.west_, info.east_, info.north_, info.south_, info.includeTerrainInNewChunks_ );
    }
}

/**
 * This function process messages in current message queue. But all mouse events,
 * keyboard events and menu events will be discarded.
 * This is used for preventing window from losing responding during some long time
 * calculation
 */
/*static */void WorldManager::processMessagesForTask( bool* isEscPressed )
{
    BW_GUARD;

    MSG msg;
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
        {
            if( msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE )
            {
                if ( isEscPressed != NULL )
                {
                    (*isEscPressed) = true;
                }
            }

            continue;
        }
        if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
        {
            continue;
        }
        if (msg.message == WM_COMMAND)
        {
            continue;
        }
        TranslateMessage( &msg );
        DispatchMessage(  &msg );
    }
}

FencesToolView* WorldManager::getFenceToolView()
{
    return fencesToolView_.getObject();
}


class WEMetaDataEnvironment : public MetaData::Environment
{
    virtual void changed( void* param )
    {
        BW_GUARD;

        EditorChunkCommonLoadSave* eccl = (EditorChunkCommonLoadSave*)param;

        eccl->edCommonChanged();
    }
};

static WEMetaDataEnvironment s_WEMetaDataEnvironment;

void WorldManager::enableEditorRenderables(bool enable)
{
    enableEditorRenderables_ = enable;

    Tool::enableRenderTools(enable);
}

BW_END_NAMESPACE
