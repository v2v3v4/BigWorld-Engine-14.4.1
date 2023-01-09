#ifndef WORLD_MANGER_HPP
#define WORLD_MANGER_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "worldeditor/world/editor_chunk_lock_visualizer.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/misc/editor_tickable.hpp"
#include "worldeditor/misc/editor_renderable.hpp"
#include "worldeditor/gui/scene_browser/scene_browser.hpp"
#include "worldeditor/gui/dialogs/take_chunk_photo_dlg.hpp"
#include "worldeditor/project/forced_lod_map.hpp"

#include "common/space_editor.hpp"
#include "common/bwlockd_connection.hpp"

#include "guimanager/gui_action_maker.hpp"
#include "guimanager/gui_updater_maker.hpp"
#include "guimanager/gui_functor_option.hpp"

#include "gizmo/snap_provider.hpp"
#include "gizmo/coord_mode_provider.hpp"
#include "gizmo/tool.hpp"

#include "math/vector3.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/slow_task.hpp"

#include "chunk/chunk_processor_manager.hpp"
#include "chunk/invalidate_flags.hpp"

#include "romp/progress.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

struct WorldEditorDebugMessageCallback : public DebugMessageCallback
{
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );
};


struct WorldEditorCriticalMessageCallback : public CriticalMessageCallback
{
	virtual void handleCritical( const char * msg );
};


class WEChunkFinder : public ChunkFinder
{
public:
	void operator()( ToolPtr tool, float buffer = 0.f );
};


class ChunkManager;
class RompHarness;
class SpaceNameManager;
class WorldEditorProgressBar;
class FencesToolView;
class HeightMap;

namespace Terrain
{
	class Manager;
}

namespace Moo
{
	class DrawContext;
}


/**
 *	This class is the highest authority in the WorldEditor
 */
class WorldManager :
	public Singleton< WorldManager >,
	public SnapProvider,
	public CoordModeProvider,
	public ReferenceCount,
	SlowTaskHandler,
	GUI::ActionMaker<WorldManager>,
	GUI::OptionMap,
	GUI::UpdaterMaker<WorldManager>,
	public ChunkProcessorManager,
	public SpaceEditor
{
public:
	typedef WorldManager This;

	static WorldManager& instance();
	WorldManager();
    ~WorldManager();

	bool	init( HINSTANCE hInst, HWND hwndApp, HWND hwndGraphics );
	void	fini();
	void	forceClean();
	bool	isScriptDirty() const;
	bool	isDirty() const;
	bool	isChunkProcessed( Chunk* chunk ) const;
	bool	canClose( const BW::string& action );
	void	cleanupTools() const;
	Chunk*	getChunk( const BW::string & chunkID, bool forceIntoMemory,
		bool * wasForcedIntoMemory = NULL );

	// tool mode updater called by the tool pages
	void updateUIToolMode( const BW::wstring& pyID );

    //commands and properties.  In fini, all of these will
	//have a gui item associated with them.  All gui items
	//will call through to these properties and functions.
    void	timeOfDay( float t );
	void	farPlane( float f );
	float	farPlane() const;
	float	dTime() const	{ return dTime_; }
    void	focus( bool state );
    void	globalWeather( bool state )	{ globalWeather_ = state; }

	void	addCommentaryMsg( const BW::string& msg, int id = 0 );
	// notify user of an error (gui errors pane & commentary)
	void	addError(Chunk* chunk, ChunkItem* item, const char * format, ...);

	void	onDeleteVLO( const BW::string& id );

	bool	isChunkWritable( Chunk* chunk ) const;
	bool	isChunkEditable( Chunk* pChunk ) const;

	void	reloadAllChunks( bool askBeforeProceed );
	
	virtual void	changedChunk( Chunk* pPrimaryChunk,
						InvalidateFlags flags = InvalidateFlags::FLAG_THUMBNAIL );
	virtual void	changedChunks( BW::set<Chunk*>& primaryChunks,
						InvalidateFlags flags = InvalidateFlags::FLAG_THUMBNAIL );

	virtual void	changedChunk( Chunk* pPrimaryChunk,
						EditorChunkItem& changedItem );
	virtual void	changedChunks( BW::set<Chunk*>& primaryChunks,
						EditorChunkItem& changedItem );

	virtual void	changedChunk( Chunk* pPrimaryChunk,
						InvalidateFlags flags,
						EditorChunkItem& changedItem );
	virtual void	changedChunks( BW::set<Chunk*>& primaryChunks,
						InvalidateFlags flags,
						EditorChunkItem& changedItem );

	void	collectOccupiedChunks( const BoundingBox & bb,
		Chunk * pResidentChunk, BW::set< Chunk * > & retChunks );

	void	dirtyThumbnail( Chunk * pChunk, bool justLoaded = false );
	void	changedTerrainBlock( Chunk * pChunk,
		bool collisionSceneChanged = true );
	void	chunkShadowUpdated( Chunk * pChunk );
	void	resaveAllTerrainBlocks();
	void	restitchAllTerrainBlocks();
	void	createSpaceImage();
	void	regenerateThumbnailsOffline();
	void	regenerateLODsOffline();
	void	convertSpaceToZip();
	void	optimiseTerrainTextures();
	void	chunkThumbnailUpdated( Chunk * pChunk );
	void	resetChangedLists();
	bool	checkForReadOnly() const;

	void	tickSavingChunks();
	void	save();
	void	quickSave();

	bool hasEnvironmentChanged() const
	{ return changedEnvironment_; }

    void environmentChanged( bool environmentChanged = true )
		{ changedEnvironment_ = environmentChanged; }

	bool	snapsEnabled() const;
	bool	freeSnapsEnabled() const;
	bool	terrainSnapsEnabled() const;
	bool	obstacleSnapsEnabled() const;
	Vector3	movementSnaps() const;
	float	angleSnaps() const;

	int	drawBSP() const;
	int	drawBSPSpeedTrees() const;
	int	drawBSPOtherModels() const;

	virtual SnapMode snapMode( ) const;
	virtual bool snapPosition( Vector3& v );
	virtual Vector3 snapNormal( const Vector3& v );
	virtual void snapPositionDelta( Vector3& v );
	virtual void snapAngles( Matrix& v );
	virtual float angleSnapAmount();

	virtual void startSlowTask();
	virtual void stopSlowTask();

	virtual CoordMode getCoordMode() const;

	const ChunkWatcher &chunkWatcher() const { return *chunkWatcher_; }

	//utility methods
	void	update( float dTime );
	bool	isMemoryLow( bool testNow = false ) const;
	bool	isMemoryLoadWarningNeeded() const;
	void	checkMemoryLoad();
	BW::string describeDirtyChunkStatus() const;
	void	writeStatus();
	static double getTotalTime();

	//app modules can call any of these methods in whatever order they desire.
	//must call beginRender() and endRender() though ( for timing info )
	void	beginRender();
    void	renderRompPreScene();
	void	renderChunksInUpdate();
	void	renderChunks();
	void	renderTerrain();
	void	renderEditorGizmos( Moo::DrawContext& drawContext );
	void	tickEditorTickables();
	void	renderEditorRenderables();
	void	renderDebugGizmos();	
	void    renderRompScene();
	void	renderRompPostScene( Moo::DrawContext& drawContext );
	void	renderRompDelayedScene();
	void	renderRompPostScene();
	void	renderRompPostProcess();

	void	endRender();
	//or app modules can call this on its own
	void	render( float dTime );

	// these methods add/remove custom tickable objects
	void addTickable( EditorTickablePtr tickable );
	void removeTickable( EditorTickablePtr tickable );

	// these methods add/remove custom renderable objects
	void addRenderable( EditorRenderablePtr renderable );
	void removeRenderable( EditorRenderablePtr renderable );

	/** Mark the chunk as it's being edited, to avoid doing background calculations on it. */
	void	lockChunkForEditing( Chunk * pChunk, bool editing );

	/**
	 * Mark the chunk as dirty for terrain and lighting if it wasn't fully
	 * saved last session
	 */
	void	onLoadChunk( Chunk * pChunk );

	/**
	 * The chunk is about to be unloaded.
	 */
	void	onUnloadChunk( Chunk * pChunk );

    /**
     * Temporarily stop background processing.
     */
    void stopBackgroundCalculation();

	//saving, adding, writing, erasing, removing, deleting low level fns
	struct SaveableObjectBase
	{
		virtual bool save( const BW::string & ) const = 0;
	};

	bool	saveAndAddChunkBase( const BW::string & resourceID,
		const SaveableObjectBase & saver, bool add, bool addAsBinary );

	void	eraseAndRemoveFile( const BW::string & resourceID );

	//2D - 3D mouse mapping
	const Vector3& worldRay() const		{ return worldRay_; }

	HWND hwndGraphics() const			{ return hwndGraphics_; }

	// Set world editor to monitor for escape from an escapable process
	void setEscapableProcess(bool escapable);
	// Test to see if the escape key was pressed, and update information accordingly.
	bool escapePressed();
	// Test to see if process was escaped during an escapable process
	bool processWasEscaped();
	// Test for valid mouse position
	bool cursorOverGraphicsWnd() const;
	// Current mouse position in the window
	POINT currentCursorPosition() const;
	// Calculate a world ray from the given position
	Vector3 getWorldRay(POINT& pt) const;
	Vector3 getWorldRay(int x, int y) const;

	// The connection to bigbangd, used for locking etc
#ifndef BIGWORLD_CLIENT_ONLY
	BWLock::BWLockDConnection& connection();
#endif // BIGWORLD_CLIENT_ONLY

	// Add the block to the list that will be rendered read only
	void addReadOnlyBlock( const Matrix& transform, Terrain::BaseTerrainBlockPtr pBlock );

	void setColourFog( DWORD colour );

	void showReadOnlyRegion(const BoundingBox& bbox, const Matrix & boxTransform );
	void showFrozenRegion(const BoundingBox& bbox, const Matrix & boxTransform );

	bool isPointInWriteableChunk( const Vector3& pt ) const;	
	bool isBoundingBoxInWriteableChunk( const BoundingBox& box, const Vector3& offset ) const;	

	bool warnSpaceNotLocked();

	DebugMessageCallback * getDebugMessageCallback() { return &s_debugMessageCallback_; }
	CriticalMessageCallback * getCriticalMessageCallback() { return &s_criticalMessageCallback_; }
	static bool criticalMessageOccurred() { return s_criticalMessageOccurred_; }
	static void criticalMessageOccurred( bool occurred ) { s_criticalMessageOccurred_ = occurred; }

	static bool messageHandler( DebugMessagePriority messagePriority,
		const char * pFormat, va_list argPtr );

	// check if a particular item is selected
	bool isItemSelected( ChunkItemPtr item ) const;
	// if the shell model for the given chunk is in the selection
	bool isChunkSelected( Chunk * pChunk ) const;
	// if there is a chunk in the selection
	bool isChunkSelected( bool forceCalculateNow = false ) const;
	// if any items in the given chunk are selected
	bool isChunkSelectable( Chunk * pChunk ) const;

	bool isInPlayerPreviewMode() const;
	void setPlayerPreviewMode( bool enable );

	bool touchAllChunks();

	bool performingModalOperation();

	BW::string getCurrentSpace() const;

	void markChunks();
	void unloadChunks();

	void replaceSelection( const ChunkItemPtr & itemToRemove, 
		const ChunkItemPtr & itemToAdd );

	bool hasPendingSelections();
	// Set Add Undo Barrier flag when defer selection mechanism happened
	void needsDeferSelectionAddUndoBarrier(bool addUndoBarrier ) { addUndoBarrier_ = addUndoBarrier; }

	class ScopedDeferSelectionReplacement
	{
	public:
		ScopedDeferSelectionReplacement();
		~ScopedDeferSelectionReplacement();

	private:
		static int s_count_;
		static SimpleMutex s_mutex_;

		friend class WorldManager;
	};

private:
	void deferReplaceSelection( const ChunkItemPtr & itemToRemove, 
		const ChunkItemPtr & itemToAdd );
	void flushSelectionReplacement();

public:

	void setSelection( const BW::vector<ChunkItemPtr>& items, bool updateSelection = true );
	void getSelection();

	bool drawSelection() const;
	void drawSelection( bool drawingSelection );
	void registerDrawSelectionItem( ChunkItem* item );
	ChunkItem * getDrawSelectionItem( DWORD index ) const;

	bool recalcThumbnail( Chunk* pChunk );
	bool recalcThumbnail( const BW::string& chunkID, bool forceIntoMemory );
	
	const BW::vector<ChunkItemPtr>& selectedItems() const { return selectedItems_; }

	GeometryMapping* geometryMapping();
	const GeometryMapping* geometryMapping() const;

	ForcedLODMap& forcedLodMap() { return forcedLodMap_; }

	// notify BB that n prim groups were just drawn in the given chunk
	// used to calculate data for the status bar
	void addPrimGroupCount( Chunk* chunk, uint n );

	// get time of day and environment minder and original seconds per hour
	TimeOfDay* timeOfDay();
    EnviroMinder &enviroMinder();
    float secondsPerHour() const { return secsPerHour_; }
    void secondsPerHour( float value ) { secsPerHour_ = value; }
	void refreshWeather();
	void setTimeFromOptions();

	void setStatusMessage( unsigned int index, const BW::string& value );
	const BW::string& getStatusMessage( unsigned int index ) const;

	// you can save anything!
	template <class C> struct SaveableObjectPtr : public SaveableObjectBase
	{
		SaveableObjectPtr( C & ob ) : ob_( ob ) { }
		bool save( const BW::string & chunkID ) const { return ob_->save( chunkID ); }
		C & ob_;
	};
	template <class C> bool saveAndAddChunk( const BW::string & resourceID,
		C saver, bool add, bool addAsBinary )
	{
		return this->saveAndAddChunkBase( resourceID, SaveableObjectPtr<C>(saver),
			add, addAsBinary );
	}

	void setCursor( HCURSOR cursor );
	void resetCursor();
	HCURSOR cursor() const
	{
		return cursor_;
	}

	// Methods related to terrain painting
	void chunkTexturesPainted( Chunk* chunk, bool rebuiltLodTexture );
	void chunkTexturesContextMenu( Chunk* chunk );

	uint32								getTerrainVersion() const;
	const TerrainUtils::TerrainFormat&	getTerrainInfo();
	Terrain::TerrainSettingsPtr			pTerrainSettings();
	const Terrain::TerrainSettingsPtr	pTerrainSettings() const;

	WorldEditorProgressBar*	progressBar() { return progressBar_; }

	EditorChunkItemLinkableManager& linkerManager() { return linkerManager_; }

	// Methods related to the Scene Browser
	void sceneBrowserSelChange( const SceneBrowser::ItemList & selection );

	// Timeslicing Interface for ChunkProcessor control
	static void allowChunkProcessorThreadsToRun( bool allow );
	static void checkThreadNeedsToBeBlocked();
		// Override from BgTaskManager
	virtual void setStaticThreadData( const BackgroundTaskPtr pTask );

	//-------------------------------------------------
	//Python Interface
	//-------------------------------------------------

	PY_MODULE_STATIC_METHOD_DECLARE( py_worldRay )
	PY_MODULE_STATIC_METHOD_DECLARE( py_repairTerrain )
	PY_MODULE_STATIC_METHOD_DECLARE( py_farPlane )

	PY_MODULE_STATIC_METHOD_DECLARE( py_save )
	PY_MODULE_STATIC_METHOD_DECLARE( py_quickSave )
	PY_MODULE_STATIC_METHOD_DECLARE( py_update )
	PY_MODULE_STATIC_METHOD_DECLARE( py_render )
	PY_MODULE_STATIC_METHOD_DECLARE( py_pause )
	PY_MODULE_STATIC_METHOD_DECLARE( py_showEditorRenderables )
	PY_MODULE_STATIC_METHOD_DECLARE( py_showUDOLinks )

	PY_MODULE_STATIC_METHOD_DECLARE( py_revealSelection )
	PY_MODULE_STATIC_METHOD_DECLARE( py_isChunkSelected )
	PY_MODULE_STATIC_METHOD_DECLARE( py_selectAll )
	PY_MODULE_STATIC_METHOD_DECLARE( py_isSceneBrowserFocused )

	PY_MODULE_STATIC_METHOD_DECLARE( py_cursorOverGraphicsWnd )
    PY_MODULE_STATIC_METHOD_DECLARE( py_importDataGUI )
    PY_MODULE_STATIC_METHOD_DECLARE( py_exportDataGUI )

	PY_MODULE_STATIC_METHOD_DECLARE( py_rightClick )

	static bool setReadyForScreenshotCallback( const ScriptObject& callback );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETOK,
		setReadyForScreenshotCallback, ARG( ScriptObject, END ) )

	// Constants
	static const int TIME_OF_DAY_MULTIPLIER = 10;

	float	getMaxFarPlane() const;

	typedef BW::set<BW::string> ChunkSet;

	void flushDelayedChanges();

	void expandSpaceWithDialog();
	bool expandSpace( int westCnt, int eastCnt, int northCnt, int southCnt, bool chunkWithTerrain );
	bool createBlankChunkData( BW::string& retPath, bool withTerrain );

	bool changedPostProcessing() const;
	void changedPostProcessing( bool changed );

	bool userEditingPostProcessing() const;
	void userEditingPostProcessing( bool editing );

	static void processMessagesForTask( bool* isEscPressed = NULL );

	void disableClose() { disableClose_ = true; }
	void enableClose() { disableClose_ = false; }

	// Indicates if something is preventing the user from editing
	// the space in the UI (e.g., modal dialog or progress bar)
	bool uiBlocked() { return uiBlocked_; }
	void beginModalOperation() {
		uiBlocked_ = true;
	}
	void endModalOperation() {
		uiBlocked_ = false;
	}

	void enableEditorRenderables( bool enable );
	bool enableEditorRenderables() const { return enableEditorRenderables_; }
	FencesToolView* getFenceToolView();

	bool wasExitRequested() const { return exitRequested_; }

	Moo::DrawContext& colourDrawContext() const { return *colourDrawContext_; }

private:
	static void readyForScreenshotCallback();
	static void shadowsRenderedCallback();
	static void shadowsStartedCallback();
	static void floraRenderedCallback();
	static void chunksLoadedCallback( const char* outputString );
	static ScriptObject readyForScreenshotCallback_;
	
	void (*frameDelayCallback_)();
	unsigned int framesToDelayBeforeCallback_;

	void updateAnimations( ChunkManager& chunkManager );

	HANDLE	spaceLock_;
	bool	inited_;
	bool	updating_;
    bool	chunkManagerInited_;
	bool	exitRequested_;

    bool	initRomp();

    RompHarness * romp_;
    float	dTime_;
	static double	s_totalTime_;
	bool	canSeeTerrain_;
	bool	enableEditorRenderables_;

    BW::string	projectPath_;
	bool	isInPlayerPreviewMode_;
    bool	globalWeather_;
    HWND	hwndApp_;
    HWND	hwndGraphics_;

    bool changedEnvironment_;
    float secsPerHour_;

	bool changedPostProcessing_;
	bool userEditingPostProcessing_;

	bool saveChangedFiles();

	/** Which outside chunks need to have their terrain shadows recalculated */
	BW::set<Chunk*> chunksBeingEdited_;

	/**
	 *	Set of custom tickable objects.
	 */
	BW::list<EditorTickablePtr> editorTickables_;

	/**
	 *	Set of custom renderable objects.
	 */
	BW::set<EditorRenderablePtr> editorRenderables_;

	Vector3	worldRay_;

	float angleSnaps_;
	Vector3 movementSnaps_;
	Vector3 movementDeltaSnaps_;
	void calculateSnaps();

	EditorChunkLockVisualizer chunkLockVisualizer_;

	bool settingSelection_;
	bool revealingSelection_;

	void doBackgroundUpdating();
#ifndef BIGWORLD_CLIENT_ONLY
	BWLock::BWLockDConnection conn_;
#endif // BIGWORLD_CLIENT_ONLY

	typedef std::pair<Matrix, Terrain::BaseTerrainBlockPtr >	BlockInPlace;
	BW::vector< BlockInPlace >	readOnlyTerrainBlocks_;

	typedef BW::vector< BW::string > StringVector;

	static SimpleMutex pendingMessagesMutex_;
	static StringVector pendingMessages_;
	static void postPendingErrorMessages();

	BW::vector<ChunkItemPtr> selectedItems_;
	BW::vector<ChunkItemPtr> itemsToRemove_;
	BW::vector<ChunkItemPtr> itemsToAdd_;

	SmartPointer<WorldEditorCamera> worldEditorCamera_;

	GeometryMapping* mapping_;

	ForcedLODMap forcedLodMap_;

	// Current chunk that we're counting prim groups for, used for status bar display
	Chunk* currentMonitoredChunk_;
	// Chunk amount of primitive groups in the locator's chunk, used for status bar display
	uint currentPrimGroupCount_;

	// handle the debug messages
	static WorldEditorDebugMessageCallback s_debugMessageCallback_;
	static WorldEditorCriticalMessageCallback s_criticalMessageCallback_;
	static bool s_criticalMessageOccurred_;

	EditorChunkItemLinkableManager linkerManager_;

	SpaceNameManager* spaceManager_;
	BW::string currentSpace_;

	bool changeSpace( const BW::string& space, bool reload );

	bool handleGUIAction( GUI::ItemPtr item );
	bool changeSpace( GUI::ItemPtr item );
	bool newSpace( GUI::ItemPtr item );
	bool editSpace( GUI::ItemPtr item );
	bool recreateSpace( GUI::ItemPtr item );
	bool recentSpace( GUI::ItemPtr item );
	bool setLanguage( GUI::ItemPtr item );
	bool clearUndoRedoHistory( GUI::ItemPtr item );
	bool doReloadAllChunks( GUI::ItemPtr item );
	bool doExit( GUI::ItemPtr item );
	bool doReloadAllTextures( GUI::ItemPtr item );
	bool recalcCurrentChunk( GUI::ItemPtr item );

	unsigned int handleGUIUpdate( GUI::ItemPtr item );
	unsigned int updateRecreateSpace( GUI::ItemPtr item );
	unsigned int updateUndo( GUI::ItemPtr item );
	unsigned int updateRedo( GUI::ItemPtr item );
	void updateRecentFile();
	void updateLanguageList();

	bool doExternalEditor( GUI::ItemPtr item );
	unsigned int updateExternalEditor( GUI::ItemPtr item );
	unsigned int updateLanguage( GUI::ItemPtr item );

	bool initPanels();
	bool loadDefaultPanels( GUI::ItemPtr item = 0 );

	virtual BW::string get( const BW::string& key ) const;
	virtual bool exist( const BW::string& key ) const;
	virtual void set( const BW::string& key, const BW::string& value );

	bool drawMissingTextureLOD(Chunk *chunk, bool markDirty);
	void drawMissingTextureLODs(bool complainIfNotDone, bool doAll, 
			bool markDirty, bool progress = false);


	void internalChangedChunk( Chunk * pChunk, InvalidateFlags flags, 
			EditorChunkItem* pChangedItem);

	void setCursor();

	void updateSelection( const BW::vector< ChunkItemPtr > & items );

	void createSpaceImageInternal( const BW::string& targetPath, const TakePhotoDlg::PhotoInfo& photoInfo );
	bool takePhotoOfChunk( const BW::string& chunkID, const BW::string& folderPath, int photoPosX, int photoPosY, float pixelsPerMetre, int& imgSize );

	BW::vector<BW::string> statusMessages_;
	DWORD lastModifyTime_;
	bool drawSelection_;
	BW::vector< ChunkItem * > drawSelectionItems_;

	HCURSOR cursor_;

    bool waitCursor_;
    void showBusyCursor();

	// used to store the state of the last save/quickSave
	bool saveFailed_;
	bool inEscapableProcess_;
	bool processWasEscaped_;
	bool warningOnLowMemory_;
	bool insideQuickSave_;

	ChunkWatcherPtr chunkWatcher_;
	
	TerrainUtils::TerrainFormatStorage terrainFormatStorage_;
	bool renderDisabled_;

	float timeLastUpdateTexLod_;

	WorldEditorProgressBar *progressBar_;
	BW::string currentLanguageName_;
	BW::string currentCountryName_;

	SimpleMutex changeMutex_;

	bool disableClose_;
	bool uiBlocked_;

	LONG slowTaskCount_;
	HCURSOR savedCursor_;
	SimpleMutex savedCursorMutex_;
	SmartPointer<FencesToolView> fencesToolView_;
	typedef std::auto_ptr<HeightMap> HeightMapPtr;
	HeightMapPtr pHeightMap_;

	typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
	TerrainManagerPtr pTerrainManager_;

	class InitFailureCleanup;
	Moo::DrawContext*		colourDrawContext_;
	Moo::DrawContext*		shadowDrawContext_;

private:
	struct PendingChangedChunk
	{
		Chunk* pChunk_;
		InvalidateFlags flags_;
		EditorChunkItem* pChangedItem_;
	};

	BW::list< PendingChangedChunk > pendingChangedChunks_;

private:
	// Write lock is held by WorldManager except when
	// allowChunkProcessorThreads_ is true
	// Read lock is held by active ChunkProcessors
	static ReadWriteLock s_chunkProcessorThreadsTimeSliceLock_;
	static volatile bool s_allowChunkProcessorThreads_;
	static uint64 s_presentTimeSliceTimeStamp_;

	static uint32 s_minimumAllowTimeSliceMS_;
	static uint32 s_timeSlicePauseWarnThresholdMS_;

	static const int NUM_BACKGROUND_CHUNK_PROCESSING_THREADS;
	class SelectionOverrideBlock*	selectionOverride_;
	bool addUndoBarrier_;
};

/**
 *	Helper to prevent reentry to WorldEditor operations such as saving.
 */
class ScopedReentryAssert
{
public:
	ScopedReentryAssert()
	{
		MF_ASSERT( !entered_ );
		entered_ = true;
	}

	~ScopedReentryAssert()
	{
		entered_ = false;
	}
private:
	static bool entered_;
};

BW_END_NAMESPACE

#endif // WORLD_MANGER_HPP
