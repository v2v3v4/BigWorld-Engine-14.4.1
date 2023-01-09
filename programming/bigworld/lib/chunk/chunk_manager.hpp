#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"

#include "moo/moo_math.hpp"

#ifndef MF_SERVER
#include "network/space_data_mappings.hpp"
#endif // MF_SERVER

#include "chunk_boundary.hpp"

#include "umbra_config.hpp"

#if UMBRA_ENABLE
namespace Umbra
{
	namespace OB 
	{
		class Camera;
		class Cell;
	}
}
#endif


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkSpace;
class ChunkItem;
class ChunkUmbra;
typedef uint32 ChunkSpaceID;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;
class GeometryMapping;
class FindSeedTask;
#if ENABLE_RELOAD_MODEL
class FloraVisualManager;
#endif//RELOAD_MODEL_SUPPORT

namespace Moo
{
	class DrawContext;
}
/**
 *	This singleton class manages most aspects of the chunky scene graph.
 *	It contains most of the API that classes outside the chunk library will
 *	need to use. It manages the universe that the game runs.
 *
 *	A universe defines the world for a whole game - both the client and
 *	the server run only one universe. Each universe is split up into
 *	a number of named spaces.
 */
class ChunkManager
{
public:
	ChunkManager();
	~ChunkManager();

	bool init( DataSectionPtr configSection = NULL );
	bool fini();

	// set the camera position
	void camera( const Matrix & cameraTransform, ChunkSpacePtr pSpace, Chunk* pOverride = NULL );
	// get camera transform
	const Matrix & cameraTrans() const { return cameraTrans_; }

	// call everyone's tick method, plus scan for
	// new chunks to load and old chunks to dispose
	void tick( float dTime );
	void updateAnimations();

	// draw the scene from the set camera position
	void draw( Moo::DrawContext& drawContext );
	void drawReflection( Moo::DrawContext& reflectionDrawContext,
		const BW::vector<Chunk*>& pVisibleChunks,
		bool outsideChunks, float nearPoint );

#if UMBRA_ENABLE
	void addChunkShadowCaster( Chunk* item );
	void clearChunkShadowCasters();
	const BW::vector<Chunk*> & umbraChunkShadowCasters() const { return umbraChunkShadowCasters_; }
	void umbraDraw( Moo::DrawContext& drawContext );
	void umbraRepeat( Moo::DrawContext& drawContext );
#endif

	// add/del fringe chunks from this draw call
	void addFringe( Chunk * pChunk );
	void delFringe( Chunk * pChunk );

	// add the given chunk pointer into the space, can be called from background thread.
	void addChunkToSpace( Chunk * pChunk, ChunkSpaceID spaceID );

	// append the given chunk to the load list
//	void loadChunkExplicitly( const BW::string & identifier, int spaceID );
	void loadChunkExplicitly( const BW::string & identifier,
		GeometryMapping * pMapping, bool isOverlapper = false );

	Chunk* findChunkByName( const BW::string & identifier, 
        GeometryMapping * pMapping, bool createIfNotFound = true );
	Chunk* findChunkByGrid( int16 x, int16 z, GeometryMapping * pMapping );
	Chunk* findOutdoorChunkByPosition( float x, float z, GeometryMapping * pMapping );

	void loadChunkNow( Chunk* chunk );
	void loadChunkNow( const BW::string & identifier, GeometryMapping * pMapping );

	// accessors
	ChunkSpacePtr space( ChunkSpaceID spaceID, bool createIfMissing = true );

	ChunkSpacePtr cameraSpace() const;
	Chunk * cameraChunk() const			{ return cameraChunk_; }

	void clearSpace( ChunkSpace * pSpace );

	bool busy() const			{ return !loadingChunks_.empty(); }
	bool loadPending()			{ return busy() || findSeedTask_; }

	void enableScan()			{ scanEnabled_ = true; }
	void disableScan()			{ scanEnabled_ = false; }
	bool scanEnabled() const	{ return scanEnabled_; }
	float maxLoadPath() const	{ return maxLoadPath_; }
	float minUnloadPath() const	{ return minUnloadPath_; }
	void maxLoadPath( float v )		{ maxLoadPath_ = v; }
	void minUnloadPath( float v )	{ minUnloadPath_ = v; }
	void maxUnloadChunks( unsigned int maxUnloadChunks)	{ maxUnloadChunks_ = maxUnloadChunks; }
	void autoSetPathConstraints( float farPlane );
	float closestUnloadedChunk( ChunkSpacePtr pSpace ) const;

	void addSpace( ChunkSpace * pSpace );
	void delSpace( ChunkSpace * pSpace );

	static ChunkManager & instance();

	static void drawTreeBranch( Chunk * pChunk, const BW::string & why );
	static void drawTreeReturn();
	static const BW::string & drawTree();

	static int      s_chunksTraversed;
	static int      s_chunksVisible;
	static int      s_chunksReflected;
	static int      s_visibleCount;
	static int      s_drawPass;

	static bool     s_drawVisibilityBBoxes;

	void processPendingChunks();
	bool checkLoadingChunks();
	void cancelLoadingChunks();

	bool canLoadChunks() const { return canLoadChunks_; }
	void canLoadChunks( bool canLoadChunks ) { canLoadChunks_ = canLoadChunks; }

	void switchToSyncMode( bool sync );
	void switchToSyncTerrainLoad( bool sync );

	void chunkDeleted( Chunk* pChunk );
	void mappingCondemned( GeometryMapping* pMapping );

#if UMBRA_ENABLE
	Umbra::OB::Camera*	getUmbraCamera() { return umbraCamera_; }
	void			setUmbraCamera( Umbra::OB::Camera* pCamera ) { umbraCamera_ = pCamera; }

	ChunkUmbra*			umbra() const { return umbra_; }
#endif	

	Vector3 cameraNearPoint() const;
	Vector3 cameraAxis(int axis) const;

	/**
	 * Allow debug_app to write this to "special" console. 
	 */
	static BW::string s_specialConsoleString;

	/**
	 *	Access the tickMark it us incremented every time tick is called
	 *	so that objects that tick on draw can work out whether tick has
	 *	been called since last time it was ticked
	 */
	uint32	tickMark() const { return tickMark_; }
	/**
	 *	Access the total tick time in ms, this is the number of milliseconds
	 *	that have elapsed since the chunk manager was started.
	 */
	uint64	totalTickTimeInMS() const { return totalTickTimeInMS_; }
	/**
	 *	Access the last delta time passed into the tick method
	 */
	float	dTime() const { return dTime_; }

	/**
	 * Start the loading timer and pass in the function to call
	 * when loading has finished. The callback function takes the 
	 * const char* which is printed to the Python console.
	 * waitForFinish skips waiting for loading to start and instead
	 * starts waiting for the load to complete.
	 */
	void startTimeLoading(
		void ( *callback )( const char* ),
		bool waitForFinish = false );

private:
	bool		initted_;

	typedef BW::map<ChunkSpaceID,ChunkSpace*>	ChunkSpaces;
	ChunkSpaces			spaces_;

	Matrix			cameraTrans_;
	ChunkSpacePtr	pCameraSpace_;
	Chunk			* cameraChunk_;

	typedef BW::vector<Chunk*>		ChunkVector;
	ChunkVector		loadingChunks_;
	SmartPointer<FindSeedTask> findSeedTask_;
	Chunk			* fringeHead_;

	typedef std::pair<BW::string, GeometryMapping*> StrMappingPair;
	BW::set<StrMappingPair>	pendingChunks_;
	SimpleMutex					pendingChunksMutex_;

	typedef std::pair<Chunk*, ChunkSpaceID> ChunkPtrSpaceIDPair;
	BW::set<ChunkPtrSpaceIDPair>	pendingChunkPtrs_;
	BW::set<Chunk*>				deletedChunks_;
	SimpleMutex				pendingChunkPtrsMutex_;
#if ENABLE_RELOAD_MODEL
	FloraVisualManager*		floraVisualManager_;
#endif//RELOAD_MODEL_SUPPORT

	bool scan();
	bool blindpanic();
	bool autoBootstrapSeedChunk();

	void loadChunk( Chunk * pChunk, bool highPriority );

	struct PortalBounds
	{
		bool init( Portal2DRef portal, float minDepth );
		Vector2 min_;
		Vector2 max_;
		float	minDepth_;
		Portal2DRef portal2D_;
	};

	typedef BW::vector<PortalBounds> PortalBoundsVector;

	void cullInsideChunks( Chunk* pChunk, ChunkBoundary::Portal *pPortal, Portal2DRef portal2D, 
								  ChunkVector& chunks, PortalBoundsVector& outsidePortals, bool ignoreOutsidePortals );

	void cullOutsideChunks( ChunkVector& chunks, const PortalBoundsVector& outsidePortals );


	void			checkCameraBoundaries();

	float			maxLoadPath_;	// bigger than sqrt(500^2 + 500^2)
	float			minUnloadPath_;

	float			scanSkippedFor_;
	Vector3			cameraAtLastScan_;
	bool			noneLoadedAtLastScan_;

	bool			canLoadChunks_;

	// The maximum number of chunks that can be scheduled for unloading. It
	// should unload chunks aggressively in the tool to free memory.
	unsigned int	maxUnloadChunks_;

	unsigned int	workingInSyncMode_;
	unsigned int	waitingForTerrainLoad_;
#if UMBRA_ENABLE
	// umbra
	ChunkUmbra*			umbra_;
	Umbra::OB::Camera*	umbraCamera_;
	ChunkVector    umbraChunkShadowCasters_;
	bool useLatentOcclusionQueries_;
	ChunkVector		umbraInsideChunks_;
#endif


	// These values are used for objects that tick without having their tick
	// method called. 

	// tickMark_ is incremented every time tick is called so that objects
	// that draw multiple times in a frame can identify whether tick has been
	// called since last time it was drawn, this is useful for objects that
	// do their tick when they are drawn (i.e. animating chunk models)
	// See ChunkModel::draw for an example of how this is implemented
	uint32	tickMark_;

	// totalTickTimeInMS_ in the number of milliseconds that have elapsed since
	// the chunk manager was started, this is useful for objects that tick 
	// themselves when they are drawn
	uint64	totalTickTimeInMS_;

	// dTime_ is the last time delta that was passed into the tick method
	float	dTime_;

	// do we do automatically chunk loading/unloading in tick?
	bool	scanEnabled_;

	void updateTiming();

	enum TimingState
	{
		NOT_TIMING,
		WAITING_TO_START_LOADING,
		WAITING_TO_FINISH_LOADING,
		FINISHED_LOADING,
		NUM_TIMING_STATES,
	} timingState_;

	uint64 loadingTimes_[NUM_TIMING_STATES];
	// function to call when loading finishes
	void ( *timeLoadingCallback_ )( const char* ); 
};


/**
 *  This class is used to turn on/off sync mode of the ChunkManager in a 
 *  scoped fashion.
 */
class ScopedSyncMode
{
public:
    ScopedSyncMode();
    ~ScopedSyncMode();

private:
    ScopedSyncMode(const ScopedSyncMode &);             // not allowed
    ScopedSyncMode &operator=(const ScopedSyncMode &);  // not allowed
};

BW_END_NAMESPACE

#endif // CHUNK_MANAGER_HPP

