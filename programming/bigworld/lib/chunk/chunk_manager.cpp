#include "pch.hpp"
#include "cstdmf/guard.hpp"
#include "chunk_manager.hpp"

#if UMBRA_ENABLE
#include <ob/umbraCamera.hpp>
#include <ob/umbraCell.hpp>
#include <ob/umbraCommander.hpp>

#include "chunk_umbra.hpp"
#endif

#include <queue>

#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"

#include "chunk_overlapper.hpp"
#include "chunk_boundary.hpp"
#include "chunk_loader.hpp"
#include "chunk_space.hpp"
#include "chunk_exit_portal.hpp"
#include "chunk_vlo.hpp"
#include "chunk.hpp"
#include "chunk_terrain.hpp"
#include "chunk_obstacle.hpp"
#include "geometry_mapping.hpp"

#include "moo/line_helper.hpp"
#include "romp/ecotype_generators.hpp"

#include "math/mathdef.hpp"
#include "math/portal2d.hpp"
#include "moo/draw_context.hpp"
#include "moo/render_context.hpp"
#include "moo/renderer.hpp"

#include "network/space_data_mapping.hpp"

#include "speedtree/speedtree_renderer.hpp"
#include "terrain/terrain_settings.hpp"

#ifndef MF_SERVER
#include "moo/geometrics.hpp"
#endif

#include "space/space_manager.hpp"

#include "cstdmf/log_msg.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

// Force linking with chunktrees
extern int ChunkTree_token;
const int s_chunkTokenSet = ChunkTree_token;

// Class statics
BW::string ChunkManager::s_specialConsoleString;

namespace 
{
    // The maximum number of chunks that can be scheduled for loading:
    const uint32 MAX_LOADING_CHUNKS = 100;

    // The maximum number of chunks that the scan can find to schedule for
    // loading during one call:
    const uint32 MAX_WORTHY_CHUNKS  = 50;
}

/**
 *  Constructor.
 */
ChunkManager::ChunkManager() :
    initted_( false ),
    workingInSyncMode_( 0 ),
    waitingForTerrainLoad_( 0 ),
    cameraTrans_( Matrix::identity ),
    pCameraSpace_( NULL ),
    cameraChunk_( NULL ),
    fringeHead_( NULL ),
    maxLoadPath_( 750.f ),
    minUnloadPath_( 1000.f ),
    scanSkippedFor_( 60.f ),
    cameraAtLastScan_( 0.f, -1000000.f, 0.f ),
    noneLoadedAtLastScan_( false ),
    canLoadChunks_( true ),

#ifdef EDITOR_ENABLED
    maxUnloadChunks_( 4 ),
#else//EDITOR_ENABLED
    maxUnloadChunks_( 1 ),
#endif//EDITOR_ENABLED

#if UMBRA_ENABLE
    umbraCamera_( NULL ),
#endif
    tickMark_( 0 ),
    totalTickTimeInMS_( 0 ),
    dTime_( 0.f ),
#if ENABLE_RELOAD_MODEL
    floraVisualManager_( NULL ),
#endif//RELOAD_MODEL_SUPPORT
    scanEnabled_( true ),
    timingState_( NOT_TIMING )
#if UMBRA_ENABLE
    ,umbra_( NULL )
#endif
{
    REGISTER_SINGLETON( ChunkManager )
}


/**
 *  Destructor.
 */
ChunkManager::~ChunkManager()
{
    if (initted_) fini();
}


/**
 *  This method initialises the Chunk manager.
 */
bool ChunkManager::init( DataSectionPtr configSection )
{
    BW_GUARD;

    // People, when you're adding to this method, please try to keep
    // the same order of variable initialisation as they are defined
    // in the class. It makes it easier to see if anything is missing.

    //g_chunkSize += 64*1024;   // stack size guess

    // start the camera off at the origin
    cameraTrans_.setIdentity();
    pCameraSpace_ = NULL;
    cameraChunk_ = NULL;

    MF_WATCH( "Chunks/Counters/Traversed Chunks", ChunkManager::s_chunksTraversed, 
        Watcher::WT_READ_ONLY, "Number of chunks traversed to draw the scene" );
    MF_WATCH( "Chunks/Counters/Visible Chunks", ChunkManager::s_chunksVisible, 
        Watcher::WT_READ_ONLY, "Number of chunks actually drawn after culling" );
    MF_WATCH( "Chunks/Counters/Reflected Chunks", ChunkManager::s_chunksReflected, 
        Watcher::WT_READ_ONLY, "Number of chunks drawn for the reflection passes" );
    MF_WATCH( "Chunks/Counters/Visible Items", ChunkManager::s_visibleCount, 
        Watcher::WT_READ_ONLY, "Number of chunk items drawn to compose the whole scene" );
    MF_WATCH( "Chunks/Counters/Draw passes", ChunkManager::s_drawPass, 
        Watcher::WT_READ_ONLY, "Number of draw passes (1-main scene + n-reflections)" );
    MF_WATCH( "Chunks/Visibility Bounding Boxes", ChunkManager::s_drawVisibilityBBoxes, 
        Watcher::WT_READ_WRITE, "Toggles chunks visibility bounding boxes" );

#if UMBRA_ENABLE
    // Init umbra
    umbra_ = new ChunkUmbra( configSection );

    // Create the default umbra camera and the umbra cell used for outdoors
    umbraCamera_ = Umbra::OB::Camera::create();

    if (LogMsg::automatedTest())
    {
        char log[80];
        bw_snprintf( log, sizeof( log ), "Umbra Latent Occlusion Queries: %s",
            useLatentOcclusionQueries_ ? "On" : "Off" );
        LogMsg::logToFile( log );
    }
#endif // UMBRA_ENABLE

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::init();
#endif 

    Chunk::init();

#if ENABLE_RELOAD_MODEL
    floraVisualManager_ = new FloraVisualManager;
    floraVisualManager_->init();
#endif//RELOAD_MODEL_SUPPORT

    // and we're done
    initted_ = true;

    return true;
}


/**
 *  This method finalises the ChunkManager.
 */
bool ChunkManager::fini()
{
    BW_GUARD;
    if (!initted_) return false;

    cancelLoadingChunks();

    // get rid of loading chunks (presumably loaded by now)
    while (!loadingChunks_.empty())
    {
        // loadingChunks_.back()->mapping()->decRef();
        loadingChunks_.back()->loading( false );
        loadingChunks_.pop_back();
    }

    if (findSeedTask_)
    {
        findSeedTask_->release();
    }
    findSeedTask_ = NULL;

    // take the camera away
    pCameraSpace_ = NULL;

    // clear any old spaces
    while (!spaces_.empty())
    {
        ChunkSpace * pCS = spaces_.begin()->second;
        pCS->clear();
        // if someone still has references to the space, then erase it anyway
        if (!spaces_.empty() && spaces_.begin()->second == pCS)
            spaces_.erase( spaces_.begin() );
    }

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::fini();
#endif

    // We destruct the dynamic shadow object here since it can be holding onto
    // chunk items for the semi-dynamic shadow rendering which in turn can try
    // to call in to ChunkUmbra when destructing
    if (rp().dynamicShadow())
    {
        rp().dynamicShadow()->fini();
    }

#if UMBRA_ENABLE
    // Clean up our umbra resources
    umbraCamera_->release();

    // Shut down umbra
    bw_safe_delete( umbra_ );
#endif

    Chunk::fini();
    ChunkVLO::fini();

    ChunkBoundary::fini();  

#if ENABLE_RELOAD_MODEL
    floraVisualManager_->fini();
    bw_safe_delete( floraVisualManager_ );
#endif//RELOAD_MODEL_SUPPORT

    initted_ = false;

    return true;
}

/**
 *  Set the camera position (we don't use Moo's camera)
 *  If you're taking it from Moo this should be the
 *  cameraToWorld transform, i.e. invView
 */
void ChunkManager::camera( const Matrix & cameraTrans, ChunkSpacePtr pSpace, Chunk* pOverride )
{
    BW_GUARD;
    if (pSpace && pSpace->isMapped())
    {
        cameraTrans_ = cameraTrans;
        pCameraSpace_ = pSpace;
        cameraChunk_ = NULL;    // no camera chunk over focus (could be stale)

        if (pOverride)
            cameraChunk_ = pOverride;
        else
        {
            pCameraSpace_->focus( cameraTrans_.applyToOrigin() );

            cameraChunk_ =
                pCameraSpace_->findChunkFromPoint( this->cameraNearPoint() );
        }
        MF_ASSERT_DEV( !cameraChunk_ || cameraChunk_->isBound() );

        if (pCameraSpace_ && pCameraSpace_->terrainSettings())
        {
            pCameraSpace_->terrainSettings()->setActiveRenderer();
        }

        if (cameraChunk_)
        {
            checkCameraBoundaries();

            // If the current camera chunk is an outside chunk,
            // do a check to see if the real camera position is
            // in a different outside chunk
            if (cameraChunk_->isOutsideChunk())
            {
                Chunk* pRealCameraChunk = 
                    pCameraSpace_->findChunkFromPoint( cameraTrans_.applyToOrigin() );
                if (pRealCameraChunk &&
                    pRealCameraChunk->isOutsideChunk())
                {
                    cameraChunk_ = pRealCameraChunk;
                }
            }
        }

#if UMBRA_ENABLE
        Matrix cameraToCell = Matrix::identity;

        if (cameraChunk_)
        {
            // Set the cell of the chunk the camera is in
            umbraCamera_->setCell(cameraChunk_->space()->umbraCell());

            // all cells are in world space now
            cameraToCell = Matrix::identity;
        }
        else
        {
            // If there is no camera chunk set the camera cell to NULL
            umbraCamera_->setCell( NULL );
        }

        // Multiply with the camera transform
        cameraToCell.preMultiply( cameraTrans );

        // Make sure the last column of the matrix uses good
        // values as umbra is quite picky about its matrices
        Matrix m = Matrix::identity;
        m[0] = cameraToCell[0];
        m[1] = cameraToCell[1];
        m[2] = cameraToCell[2];
        m[3] = cameraToCell[3];

        // Set the matrix
        umbraCamera_->setCameraToCellMatrix((Umbra::Matrix4x4&)m);

        // Set up the umbra camera and frustum
        Umbra::Frustum f;

        const Moo::Camera& camera = Moo::rc().camera();
        float hh = tanf(camera.fov() / 2.0f) * camera.nearPlane();
        f.zNear  = camera.nearPlane();
        f.zFar   = camera.farPlane();
        f.right  = hh*camera.aspectRatio();
        f.left   = -f.right;
        f.top    = hh;
        f.bottom = -f.top;
        umbraCamera_->setFrustum(f);

        // Set up the type of culling, this is determined by the
        // umbra helper class
        uint32 properties = Umbra::OB::Camera::VIEWFRUSTUM_CULLING;
        if (umbra_->occlusionCulling())
        {
            properties |= Umbra::OB::Camera::OCCLUSION_CULLING;
        }

        if (umbra_->distanceEnabled())
        {
            properties |= Umbra::OB::Camera::DISTANCE_CULLING;
        }

        if (umbra_->umbraEnabled() && 
                umbra_->latentQueries())
        {
            properties |= Umbra::OB::Camera::LATENT_QUERIES;
        }

        umbraCamera_->setProperties( properties );
        umbraCamera_->setBitmask( umbra_->cullMask() );
#endif
    }
    else
    {
        cameraChunk_ = NULL;
        pCameraSpace_ = NULL;
    }
}

namespace
{
/*
 *  This helper function checks whether a 3d line intersects with
 *  portal, the line is assumed to me on the plane of the portal.
 */
bool lineOnPlaneIntersectsPortal( const ChunkBoundary::Portal* p, 
        const Vector3& start, const Vector3& end )
{
    BW_GUARD;
    // calculate the outcode of the two points
    uint32 oc1 = p->outcode( start );
    uint32 oc2 = p->outcode( end );

    // if either of the two points are completely inside
    // the portal, the line and portal are intersecting
    if (oc1 == 0 || oc2 == 0)
        return true;

    // if the two lines do not share an outcode the line and portal
    // may intersect
    if (!(oc1 & oc2))
    {
        // Calculate a line equation based on the two points
        Vector3 diff = start - end;
        Vector3 op1 = start - p->origin;
        Vector2 p2d( op1.dotProduct( p->uAxis ), op1.dotProduct( p->vAxis ) );
        Vector2 diff2d( -diff.dotProduct( p->vAxis ), diff.dotProduct( p->uAxis ) );

        float d = diff2d.dotProduct( p2d );

        bool allLess = true;
        bool allGreater = true;

        // Iterate over the points in the portal and update flags
        // depending on which side of the line the point is
        for (uint32 i = 0; i < p->points.size();i++)
        {
            if (d < diff2d.dotProduct( p->points[i] ))
            {
                allGreater = false;
            }
            else
            {
                allLess = false;
            }
        }

        // If we have points on both sides of the line, the portal
        // and line are intersecting.
        return allLess == false && allGreater == false;
    }
    return false;
}

};


/*
 * In boundary conditions we need to check if the centre of the 
 * near plane and the camera cross a portal boundary.
 * If it does we need to check if the near plane rect intersects
 * with the portal, if it does, we need to render from the
 * chunk the camera position is in, otherwise the chunk the
 * near plane position is in will be the start of rendering.
 */
void ChunkManager::checkCameraBoundaries()
{
    BW_GUARD;

    // Get the camera transform in chunk space
    Matrix localCamera = cameraChunk_->transformInverse();
    localCamera.preMultiply( cameraTrans_ );

    // Iterate over all chunk boundaries
    ChunkBoundaries& cb = cameraChunk_->bounds();
    for (size_t i = 0; i < cb.size(); i++)
    {
        // Iterate over bound portals in chunk boundaries
        for (uint32 j = 0; j < cb[i]->boundPortals_.size(); j++)
        {
            // Grab the current portal
            ChunkBoundary::Portal* p = cb[i]->boundPortals_[j];

            // Make sure we have a chunk, if both the camera chunk 
            // and the portal chunk are outside we will ignore this
            // portal as outside chunks don't do portal clipping
            if (p->hasChunk() &&
                !(p->pChunk->isOutsideChunk() && 
                    cameraChunk_->isOutsideChunk()))
            {
                // If the camera position is on the other side
                // of the portal, check if the near plane intersects
                // with the portal
                if (!p->plane.isInFrontOf( localCamera.applyToOrigin() ))
                {
                    // Get the near plane rectangle
                    const Moo::Camera& camera = Moo::rc().camera();
                    float nearUp = (float)tan(camera.fov() / 2.0) * 
                        camera.nearPlane();
                    float nearRight = nearUp * camera.aspectRatio();
                    float nearZ = camera.nearPlane();

                    bool inFront[4];
                    Vector3 points[4];
                    points[0] = localCamera.applyPoint( 
                        Vector3( -nearRight, nearUp, nearZ ));
                    points[1] = localCamera.applyPoint( 
                        Vector3( nearRight, nearUp, nearZ ));
                    points[2] = localCamera.applyPoint( 
                        Vector3( nearRight, -nearUp, nearZ ));
                    points[3] = localCamera.applyPoint( 
                        Vector3( -nearRight, -nearUp, nearZ ));

                    // Intersect the nearplane rectangle with the
                    // portal plane
                    uint32 nInFront = 0;
                    for (uint32 k = 0; k < 4; k++)
                    {
                        inFront[k] =
                            p->plane.isInFrontOf( points[k] );
                        nInFront = inFront[k] ? nInFront + 1 : nInFront;
                    }

                    // if all the points of the near plane are on one side
                    // of the portal don't bother checking if it intersects
                    if (nInFront != 0 && nInFront != 4)
                    {
                        // Get the line that makes up the intersection
                        // between the portal plane and the near plane
                        Vector3 pointsOnPlane[2] = {};
                        uint32 pointIndex = 0;
                        for (uint32 k = 0; k < 4; k++)
                        {
                            if (inFront[k] != inFront[(k + 1) % 4])
                            {
                                Vector3 dir = points[(k + 1) % 4] - points[k];
                                dir.normalise();
                                pointsOnPlane[pointIndex++] =
                                    p->plane.intersectRay( points[k], dir );
                            }
                        }
                        // If the line intersects the portal we put the camera
                        // on the other side of the portal
                        MF_ASSERT( pointIndex == 2 );
                        if (lineOnPlaneIntersectsPortal( p,
                            pointsOnPlane[0], pointsOnPlane[1] ))
                        {
                            cameraChunk_ = p->pChunk;
                            i = cb.size();
                            break;
                        }
                    }
                }
            }
        }
    }
}


/**
 *  Private method to get the camera point for the purposes of determining
 *  which chunk it is in.
 */
Vector3 ChunkManager::cameraNearPoint() const
{
    return cameraTrans_.applyToOrigin() +
        Moo::rc().camera().nearPlane() *
            cameraTrans_.applyToUnitAxisVector( Z_AXIS );
}


Vector3 ChunkManager::cameraAxis(int axis) const
{
    return cameraTrans_.applyToUnitAxisVector( axis );
}


/**
 *  Perform periodic duties, and call everyone else's tick
 */
void ChunkManager::tick( float dTime )
{
    BW_GUARD_PROFILER( ChunkManager_tick );

    // Update the tickMark_, totalTickTimeInMS_ and dTime
    ++tickMark_;
    totalTickTimeInMS_ += uint64(dTime * 1000.f);
    dTime_ = dTime;

    // Reset chunk statistics
    ChunkManager::s_chunksTraversed = 0;
    ChunkManager::s_chunksVisible   = 0;
    ChunkManager::s_chunksReflected = 0;
    ChunkManager::s_visibleCount    = 0;
    ChunkManager::s_drawPass        = 0;

    // make sure we have been initialised
    if (!initted_) return;

    updateTiming(); // tick the state machine which manages the loading timer
    {
        SimpleMutexHolder smh( pendingChunkPtrsMutex_ );
        while( !pendingChunkPtrs_.empty() )
        {
            Chunk* pChunk = pendingChunkPtrs_.begin()->first;
            if (deletedChunks_.find(pChunk) == deletedChunks_.end())
            {
                addChunkToSpace( pChunk, pendingChunkPtrs_.begin()->second );
            }
            pendingChunkPtrs_.erase( pendingChunkPtrs_.begin() );
        }

        deletedChunks_.clear();
    }

    processPendingChunks();

    if( workingInSyncMode_ )
        return;

    // update far plane for load/unload purposes
    float farPlane = Moo::rc().camera().farPlane();

#if SPEEDTREE_SUPPORT
    // tick the speedtrees
    speedtree::SpeedTreeRenderer::tick( dTime );
#endif

    // first see if any waiting chunks are ready
    size_t preLoadingChunksSize = loadingChunks_.size();
    bool anyChanges = this->checkLoadingChunks();

    // put the camera where it belongs
    if (cameraChunk_ != NULL && !cameraChunk_->isBound())
        cameraChunk_ = NULL;
    if (cameraChunk_ == NULL && pCameraSpace_)
    {
        cameraChunk_ =
            pCameraSpace_->findChunkFromPoint( this->cameraNearPoint() );
        MF_ASSERT_DEV( !cameraChunk_ || cameraChunk_->isBound() );
    }

    static DogWatch chunkScan( "ChunkScan" );
    chunkScan.start();
    // now fill the chunk graph outwards from there until
    //  we've covered the drawable distance and then some
    if (cameraChunk_ != NULL)
    {
        scanSkippedFor_ += dTime;
        bool noGo = false;
        // Don't bother scanning if we are in a stable state
        if ((cameraTrans_.applyToOrigin()-cameraAtLastScan_).
            lengthSquared() < pCameraSpace_->gridSize())
        {
            // first check if we're loading anyway
            if (loadingChunks_.size() >= MAX_LOADING_CHUNKS) noGo = true;
            // now see if we couldn't find anything to load
            if (preLoadingChunksSize == 0 && noneLoadedAtLastScan_ &&
                scanSkippedFor_ < 1.f) noGo = true;
        }
        if (!noGo && scanEnabled_)
        {
            anyChanges |= this->scan();
            scanSkippedFor_ = 0.f;
        }

        // if we have an unwanted seed get rid of it
        if (findSeedTask_ && findSeedTask_->isComplete())
        {
            findSeedTask_->release();
            findSeedTask_ = NULL;
        }
    }
/*
    else if (pCameraSpace_ && !pCameraSpace_->chunks().empty())
    {
        // uh-oh, we can't find the chunk that the camera is in!
        // help! I'm blind! whatever we do, let's not panic!!!
        anyChanges |= this->blindpanic();
    }
*/
    else if (pCameraSpace_)
    {
        anyChanges |= this->autoBootstrapSeedChunk();
    }
    chunkScan.stop();

    // if there were any changes to chunks loaded then we'd better see if
    // the camera wants to go somewhere different - and (very importantly)
    // update any stale columns - we don't want to go a whole frame with
    // stale data hanging around in them (especially if chunks were unloaded)
    if (pCameraSpace_ && anyChanges)
        this->camera( cameraTrans_, pCameraSpace_ );

    static DogWatch s_dwTickSpaces("Spaces");
    s_dwTickSpaces.start();
    // cool, now call everyone's tick as has been the custom of old
    for (ChunkSpaces::iterator it = spaces_.begin(); it != spaces_.end(); it++)
    {
        it->second->tick( dTime );
    }
    s_dwTickSpaces.stop();

#if UMBRA_ENABLE
    // Do umbra tick
    umbra_->tick();
#endif

    VeryLargeObject::tickAll( dTime );
}


/**
 *  Updates animated objects within each space.
 */
void ChunkManager::updateAnimations()
{
    BW_GUARD_PROFILER( ChunkManager_updateAnimations );
    
    static DogWatch s_dwUpdateAnimations( "Spaces update animations" );
    ScopedDogWatch sdw( s_dwUpdateAnimations );

    for (ChunkSpaces::iterator it = spaces_.begin(); it != spaces_.end(); ++it)
    {
        it->second->updateAnimations();
    }

    Chunk::nextVisibilityMark();
}


void ChunkManager::startTimeLoading(
    void ( *callback )( const char* ),
    bool waitForFinish )
{
    if (timingState_ == NOT_TIMING)
    {
        loadingTimes_[timingState_] = timestamp();

        if (waitForFinish)
        {
            timingState_ = WAITING_TO_FINISH_LOADING;
        }
        else
        {
            timingState_ = WAITING_TO_START_LOADING;
            DEBUG_MSG("Start loading timing\n");
        }

        timeLoadingCallback_ = callback;
    }
}


/**
 * statemachine which starts the timer when loading first starts and stops
 * it when loading has finished (note that it will wait for nFrames
 * to be sure loading has finished and return the time up until the last 
 * loading operation finished).
 */
void ChunkManager::updateTiming()
{
    static int framesSinceFinished = 0;
    const int numFrames = 120; // how many frames to we wait with no loading before we're sure loading has finished?

    switch (timingState_)
    {
    case NOT_TIMING:
        // not doing anything, just return
        return;
    case WAITING_TO_START_LOADING:
        // wait until the loading actually starts...
        if(loadingChunks_.size() > 0)
        {
            DEBUG_MSG( "Loading has begun!\n" );
            loadingTimes_[timingState_] = timestamp();
            timingState_ = WAITING_TO_FINISH_LOADING;
        }
        break;
    case WAITING_TO_FINISH_LOADING:
        if (noneLoadedAtLastScan_ && loadingChunks_.size() == 0)
        {
            // wait for framesSinceFinished frames to see if we really have finished loading.
            if (framesSinceFinished < numFrames) 
            {
                // grab the time stamp the first time we call this.
                if (framesSinceFinished == 0)
                { 
                    loadingTimes_[timingState_] = timestamp();
                }
                framesSinceFinished++;
                return;
            }
            // no loading for framesSinceFinished frames.
            timingState_ = NOT_TIMING;
            framesSinceFinished = 0;

            float timeTaken = (float)stampsToSeconds( 
                loadingTimes_[WAITING_TO_FINISH_LOADING] - loadingTimes_[WAITING_TO_START_LOADING] );

            // display timing output
            char buffer[256];
            sprintf( buffer, "loading took %f seconds\n", timeTaken );
            DEBUG_MSG( "%s", buffer );
            timeLoadingCallback_( buffer );
        }
        else
        {
            framesSinceFinished = 0;
        }

        break;
    default:
        break;
    }
}


/**
 *  Helper struct for drawing
 */
struct PortalDrawState
{
    PortalDrawState( Chunk * pChunk, Portal2DRef pClipPortal, Chunk* parent ) :
        pChunk_( pChunk ), pClipPortal_( pClipPortal ), pParent_( parent ) { }

    Chunk* pChunk_;
    Portal2DRef pClipPortal_;
    Chunk* pParent_;
};


#if UMBRA_ENABLE
void ChunkManager::addChunkShadowCaster( Chunk* item )
{
    umbraChunkShadowCasters_.push_back( item );
}

void ChunkManager::clearChunkShadowCasters()
{
    BW::vector<Chunk*>::iterator chunkIt;
    for ( chunkIt = umbraChunkShadowCasters_.begin(); chunkIt != umbraChunkShadowCasters_.end(); ++chunkIt )
    {
        Chunk* pChunk = *chunkIt;
        pChunk->clearShadowCasters();
    }

    umbraChunkShadowCasters_.clear();
}

/**
 *  This method traverses and draws the scene using umbra for outdoor objects
 *  and uses the regular portal traversal for indoor objects
 */
void ChunkManager::umbraDraw( Moo::DrawContext& drawContext )
{
    BW_GUARD_PROFILER( ChunkManager_draw );
    GPU_PROFILER_SCOPE( umbraDraw );
    ++ChunkManager::s_drawPass;

    ChunkExitPortal::seenExitPortals().clear(); 
    umbraInsideChunks_.clear();

    if (cameraChunk_ != NULL && !cameraChunk_->isBound())
        cameraChunk_ = NULL;
    if (cameraChunk_ == NULL)
    {
        return;
    }

    if (umbraCamera_ == NULL)
    {
        return;
    }

    this->clearChunkShadowCasters();

    // Get the current enviro minder
    EnviroMinder * envMinder = NULL;
    if (pCameraSpace_.getObject() != NULL)
    {
        envMinder = &ChunkManager::instance().cameraSpace()->enviro();
    }

#if SPEEDTREE_SUPPORT
    // Start speedtree rendering
    speedtree::SpeedTreeRenderer::beginFrame(
        envMinder, Moo::RENDERING_PASS_COLOR, Moo::rc().invView()
        );
#endif

    // Keep a list of portals between inside and outside 
    static PortalBoundsVector outsidePortals;
    outsidePortals.clear();

    // Check whether the camera is inside or outside
    if (!cameraChunk_->isOutsideChunk())
    {
        // Add the camera chunk to the draw list
        cameraChunk_->traverseMark( Chunk::s_nextMark_ );
        umbraInsideChunks_.push_back( cameraChunk_ );

        // We are inside so we start by culling inside chunks
        cullInsideChunks( cameraChunk_, NULL, Portal2DRef(), umbraInsideChunks_, outsidePortals, false );

        // Iterate over all visible chunks and draw them
        ChunkVector::iterator it = umbraInsideChunks_.begin();
        while (it != umbraInsideChunks_.end())
        {
            (*it)->drawBeg( drawContext );
            ++it;
        }
    }
    else
    {
        PortalBounds pb;
        pb.init(Portal2DRef(), Moo::rc().camera().nearPlane());
        outsidePortals.push_back( pb );

        cameraChunk_->drawCaches( drawContext );    
    }

    // The umbra chunk list
    static ChunkVector s_umbraChunks;

    // If the camera is in an outside chunk or we have visible portals
    // from inside to outside, render the umbra scene.
    if (cameraChunk_->isOutsideChunk() || outsidePortals.size())
    {
        Chunk::setUmbraChunks( &s_umbraChunks );
        static DogWatch s_umbraTime( "UMBRA" );
        s_umbraTime.start();
        {
            PROFILER_SCOPED( UMBRA_resolveVisibility );
            umbraCamera_->resolveVisibility();
            umbra_->drawContext( &drawContext );
            umbraCamera_->processVisibility( umbra_->pCommander() );
            umbra_->drawContext( NULL );
        }
        s_umbraTime.stop();

        Chunk::setUmbraChunks( NULL );
    }

    // If we rendered outside chunks using umbra, check if we have portals
    // from those chunks to inside chunks
    if (s_umbraChunks.size())
    {
        size_t firstChunk = umbraInsideChunks_.size();
        // We iterate over all the chunks to find chunks that have portals to inside chunks
        for (uint32 i = 0; i < s_umbraChunks.size(); i++)
        {
            Chunk* pChunk = s_umbraChunks[i];

            // Iterate over the portals in the chunk and find portals between 
            // outside and inside chunks
            Chunk::piterator it = pChunk->pbegin();
            Chunk::piterator end = pChunk->pend();
            while (it != end)
            {
                if (it->hasChunk() && !it->pChunk->isOutsideChunk())
                {
                    // Iterate over all portals to the outside world and traverse 
                    // the outside-to-inside portals through them
                    for (uint32 j = 0; j < outsidePortals.size(); j++)
                    {
                        cullInsideChunks( pChunk, &*it, outsidePortals[j].portal2D_, umbraInsideChunks_, outsidePortals, true );
                    }
                }
                it++;
            }
        }

        s_umbraChunks.clear();

        // Iterate over all visible chunks and draw them
        ChunkVector::iterator it = umbraInsideChunks_.begin() + firstChunk;
        while (it != umbraInsideChunks_.end())
        {
            (*it)->drawBeg( drawContext );
            ++it;
        }
    }

    ChunkVector::iterator it = umbraInsideChunks_.begin();
    while (it != umbraInsideChunks_.end())
    {
        (*it)->drawEnd();
        ++it;
    }


    // draw any fringe chunks too
    Chunk * pFringe = fringeHead_;
    fringeHead_ = (Chunk*)1;
    while (pFringe != NULL)
    {
        // draw it, but only the appropriate lent items
        pFringe->drawSelf( drawContext, true );

        // and take it out of the list
        Chunk * pNext = pFringe->fringeNext();
        pFringe->fringePrev( NULL );
        pFringe->fringeNext( NULL );

        //g_specialConsoleString += "Fringe chunk: " + pFringe->identifier() + "\n";

        pFringe = pNext;
    }
    MF_ASSERT_DEV( fringeHead_ == (Chunk*)1 );
    fringeHead_ = NULL;

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::endFrame();
#endif

    // move on the mark
    Chunk::nextMark();
}


/**
 *  This method traverses and draws the scene using umbra.
 */
void ChunkManager::umbraRepeat( Moo::DrawContext& drawContext )
{
    BW_GUARD;

    if (umbraCamera_ == NULL)
    {
        return;
    }

    // Get the current enviro minder
    EnviroMinder * envMinder = NULL;
    if (pCameraSpace_.getObject() != NULL)
    {
        envMinder = &ChunkManager::instance().cameraSpace()->enviro();
    }

#if SPEEDTREE_SUPPORT
    // Start speedtree rendering
    speedtree::SpeedTreeRenderer::beginFrame(
        envMinder, Moo::RENDERING_PASS_COLOR, Moo::rc().invView()
        );
#endif 


    ChunkExitPortal::seenExitPortals().clear(); 

    if (cameraChunk_)
        cameraChunk_->drawCaches( drawContext );

    umbra_->repeat( drawContext );

    // Iterate over all visible chunks and draw them
    ChunkVector::iterator it = umbraInsideChunks_.begin();
    while (it != umbraInsideChunks_.end())
    {
        (*it)->drawBeg( drawContext );
        ++it;
    }

    // Iterate over all visible chunks and tell them
    // that we are finished drawing, this sets up the
    // lent items
    it = umbraInsideChunks_.begin();
    while (it != umbraInsideChunks_.end())
    {
        (*it)->drawEnd();
        ++it;
    }

    // draw any fringe chunks too
    Chunk * pFringe = fringeHead_;
    fringeHead_ = (Chunk*)1;
    while (pFringe != NULL)
    {
        // draw it, but only the appropriate lent items
        pFringe->drawSelf( drawContext, true );

        // and take it out of the list
        Chunk * pNext = pFringe->fringeNext();
        pFringe->fringePrev( NULL );
        pFringe->fringeNext( NULL );

        //g_specialConsoleString += "Fringe chunk: " + pFringe->identifier() + "\n";

        pFringe = pNext;
    }
    MF_ASSERT_DEV( fringeHead_ == (Chunk*)1 );
    fringeHead_ = NULL;


#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::endFrame();
#endif
}
#endif

/**
 *  Draw the chunky scene, from the point of view of the
 *  camera set in the last call to our 'camera' method.
 */
void ChunkManager::draw( Moo::DrawContext& drawContext )
{
    BW_GUARD_PROFILER( ChunkManager_draw );

    ++ChunkManager::s_drawPass;

    ChunkExitPortal::seenExitPortals().clear(); 

    if (cameraChunk_ != NULL && !cameraChunk_->isBound())
        cameraChunk_ = NULL;
    if (cameraChunk_ == NULL)
    {
        // err, we're having some technical problems here...
        return;
    }

    EnviroMinder * envMinder = NULL;
    if (pCameraSpace_.getObject() != NULL)
    {
        envMinder = &ChunkManager::instance().cameraSpace()->enviro();
    }

#if SPEEDTREE_SUPPORT
    // Start speedtree rendering
    speedtree::SpeedTreeRenderer::beginFrame(
        envMinder, drawContext.renderingPassType(), Moo::rc().invView()
        );
#endif

    ChunkBoundary::Portal::updateFrustumBB();

    // The list of chunks to draw
    static ChunkVector s_drawList;

    // Add the camera chunk to the draw list
    cameraChunk_->traverseMark( Chunk::s_nextMark_ );
    s_drawList.push_back( cameraChunk_ );

    // Keep a list of portals between inside and outside 
    static PortalBoundsVector outsidePortals;
    outsidePortals.clear();

    size_t firstOutside = 0;

    // Check whether the camera is inside or outside
    if (cameraChunk_->isOutsideChunk())
    {
        // We are outside so start by culling the outside chunks to the entire screen
        cullOutsideChunks( s_drawList, outsidePortals );

        // Init the portal between the inside and the outside with the full screen portal
        PortalBounds pb;
        pb.init( Portal2DRef(), Moo::rc().camera().nearPlane() );
        outsidePortals.push_back( pb );
    }
    else
    {
        // We are inside so we start by culling inside chunks
        cullInsideChunks( cameraChunk_, NULL, Portal2DRef(), s_drawList, outsidePortals, false );

        // The index in the draw list of the first outside chunk
        firstOutside = s_drawList.size();

        // If we have some portals between inside and outside, we can see outside and
        // need to render the outside
        if (outsidePortals.size())
        {
            cullOutsideChunks( s_drawList, outsidePortals );
        }
    }

    // We iterate over all the chunks to find chunks that have portals to inside chunks
    for (size_t i = firstOutside; i < s_drawList.size(); i++)
    {
        Chunk* pChunk = s_drawList[i];

        // Check if the chunk has internal chunks
        if (pChunk->hasInternalChunks())
        {
            // Iterate over the portals in the chunk and find portals between 
            // outside and inside chunks
            Chunk::piterator it = pChunk->pbegin();
            Chunk::piterator end = pChunk->pend();
            while (it != end)
            {
                if (it->hasChunk() && !it->pChunk->isOutsideChunk())
                {
                    // Iterate over all portals to the outside world and traverse 
                    // the outside-to-inside portals through them
                    for (uint32 j = 0; j < outsidePortals.size(); j++)
                    {
                        cullInsideChunks( pChunk, &*it, outsidePortals[j].portal2D_, s_drawList, outsidePortals, true );
                    }
                }
                it++;
            }
        }
    }

    // Iterate over all visible chunks and draw them
    ChunkVector::iterator it = s_drawList.begin();
    while (it != s_drawList.end())
    {
        (*it)->drawBeg( drawContext );
        ++it;
    }

    // Iterate over all visible chunks and tell them
    // that we are finished drawing, this sets up the
    // lent items
    it = s_drawList.begin();
    while (it != s_drawList.end())
    {
        (*it)->drawEnd();
        ++it;
    }

    // draw any fringe chunks too
    Chunk * pFringe = fringeHead_;
    fringeHead_ = (Chunk*)1;
    while (pFringe != NULL)
    {
        // draw it, but only the appropriate lent items
        pFringe->drawSelf( drawContext, true );

        // and take it out of the list
        Chunk * pNext = pFringe->fringeNext();
        pFringe->fringePrev( NULL );
        pFringe->fringeNext( NULL );

        //g_specialConsoleString += "Fringe chunk: " + pFringe->identifier() + "\n";

        pFringe = pNext;
    }
    MF_ASSERT_DEV( fringeHead_ == (Chunk*)1 );
    fringeHead_ = NULL;

    // move on the mark
    Chunk::nextMark();

    s_drawList.clear();

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::endFrame();
#endif
}

/**
 *  Draw the chunky scene for use in a reflection.
 *  @param pVisibleChunks the indoor chunks the reflection is visible in
 *  @param outsideChunks whether or not the reflection is visible in outside chunks
 *  @param nearPoint the near distance the reflection will be visible from
 */
void ChunkManager::drawReflection( Moo::DrawContext& drawContext,
                                  const BW::vector<Chunk*>& pVisibleChunks,
                                  bool outsideChunks, float nearPoint )
{
    BW_GUARD_PROFILER( ChunkManager_drawReflection );

    ++ChunkManager::s_drawPass;

    ChunkExitPortal::seenExitPortals().clear(); 

    if (cameraChunk_ != NULL && !cameraChunk_->isBound())
        cameraChunk_ = NULL;
    if (cameraChunk_ == NULL)
    {
        return;
    }

    static DogWatch s_dwCullReflection( "ReflectionCull" );
    s_dwCullReflection.start();

    EnviroMinder * envMinder = NULL;
    if (pCameraSpace_.getObject() != NULL)
    {
        envMinder = &ChunkManager::instance().cameraSpace()->enviro();
    }

#if SPEEDTREE_SUPPORT
    // Start speedtree rendering
    speedtree::SpeedTreeRenderer::beginFrame(
        envMinder, Moo::RENDERING_PASS_REFLECTION, Moo::rc().invView()
        );
#endif

    ChunkBoundary::Portal::updateFrustumBB();

    // The list of chunks to draw
    static ChunkVector s_drawList;

    // Keep a list of portals between inside and outside 
    static PortalBoundsVector outsidePortals;
    outsidePortals.clear();

    ChunkVector::const_iterator vit = pVisibleChunks.begin();
    for (; vit != pVisibleChunks.end(); vit++)
    {
        (*vit)->traverseMark( Chunk::s_nextMark_ );
        s_drawList.push_back( *vit );
        // We are inside so we start by culling inside chunks
        cullInsideChunks( *vit, NULL, Portal2DRef(), s_drawList, outsidePortals, false );
    }

    size_t firstOutside = s_drawList.size();

    // Check whether the camera is inside or outside
    if (outsideChunks || outsidePortals.size())
    {
        if (outsideChunks)
        {
            PortalBounds pb;
            pb.init( Portal2DRef(), nearPoint );
            outsidePortals.push_back( pb );
        }

        // We are outside so start by culling the outside chunks to the entire screen
        cullOutsideChunks( s_drawList, outsidePortals );
    }

    // We iterate over all the chunks to find chunks that have portals to inside chunks
    for (size_t i = firstOutside; i < s_drawList.size(); i++)
    {
        Chunk* pChunk = s_drawList[i];

        // Check if the chunk has internal chunks
        if (pChunk->hasInternalChunks())
        {
            // Iterate over the portals in the chunk and find portals between 
            // outside and inside chunks
            Chunk::piterator it = pChunk->pbegin();
            Chunk::piterator end = pChunk->pend();
            while (it != end)
            {
                if (it->hasChunk() && !it->pChunk->isOutsideChunk())
                {
                    // Iterate over all portals to the outside world and traverse 
                    // the outside-to-inside portals through them
                    for (uint32 j = 0; j < outsidePortals.size(); j++)
                    {
                        cullInsideChunks( pChunk, &*it, outsidePortals[j].portal2D_, s_drawList, outsidePortals, true );
                    }
                }
                it++;
            }
        }
    }
    s_dwCullReflection.stop();

    static DogWatch s_dwDrawReflection( "ReflectionDraw" );
    s_dwDrawReflection.start();

    // Iterate over all visible chunks and draw them
    ChunkVector::iterator it = s_drawList.begin();
    while (it != s_drawList.end())
    {
        (*it)->drawBeg( drawContext );
        ++it;
    }

    // Iterate over all visible chunks and tell them
    // that we are finished drawing, this sets up the
    // lent items
    it = s_drawList.begin();
    while (it != s_drawList.end())
    {
        (*it)->drawEnd();
        ++it;
    }

    // draw any fringe chunks too
    Chunk * pFringe = fringeHead_;
    fringeHead_ = (Chunk*)1;
    while (pFringe != NULL)
    {
        // draw it, but only the appropriate lent items
        pFringe->drawSelf( drawContext, true );

        // and take it out of the list
        Chunk * pNext = pFringe->fringeNext();
        pFringe->fringePrev( NULL );
        pFringe->fringeNext( NULL );

        //g_specialConsoleString += "Fringe chunk: " + pFringe->identifier() + "\n";

        pFringe = pNext;
    }
    MF_ASSERT_DEV( fringeHead_ == (Chunk*)1 );
    fringeHead_ = NULL;

    // move on the mark
    Chunk::nextMark();

    s_drawList.clear();

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::endFrame();
#endif

    s_dwDrawReflection.stop();
}

/**
 *  This method calculates which inside chunks are visible
 *  @para pChunk the chunk to start traversal from
 *  @param pPortal the portal to start traversal through
 *  @param portal2D the portal2D to start traversal through
 *  @param chunks the list we return the visible chunks in
 *  @param outsidePortals the list we return the portals from inside to outside in
 *  @param ignoreOutsidePortals true if we don't want to return inside to outside portals
 */
void ChunkManager::cullInsideChunks( Chunk* pChunk, ChunkBoundary::Portal *pPortal, Portal2DRef portal2D, 
                              ChunkVector& chunks, PortalBoundsVector& outsidePortals, bool ignoreOutsidePortals )
{
    BW_GUARD;
    ChunkBoundary::TraversalData traversalData( Chunk::s_nextMark_ );

    static VectorNoDestructor<PortalDrawState>  stack;
    MF_ASSERT_DEV( stack.empty() ); // can't clear 'coz dtors should be called

    Chunk* pParent = NULL;

    // If a portal is passed in, traverse from pChunk through this portal before 
    // starting indoor traversal
    if (pPortal)
    {
        portal2D = pPortal->traverse( pChunk->transform(), 
            pChunk->transformInverse(), portal2D, traversalData );
        if (!portal2D.valid())
        {
            return;
        }
        pParent = pChunk;
        pChunk = pPortal->pChunk;
        if (pChunk->traverseMark() != traversalData.nextMark_)
        {
            pChunk->traverseMark( traversalData.nextMark_ );
            chunks.push_back( pChunk );
        }
    }

    stack.push_back( PortalDrawState( pChunk, portal2D, pParent ) );

    while (!stack.empty())
    {
        PortalDrawState cur = stack.back();
        stack.back().pClipPortal_ = Portal2DRef();  // clear since no dtor
        stack.pop_back();
        Chunk * pChunk = cur.pChunk_;

        // go through all bound portals
        for (Chunk::piterator it = pChunk->pbegin(); it != pChunk->pend(); it++)
        {
            switch (uintptr_t( it->pChunk ))
            {
            case ChunkBoundary::Portal::NOTHING:
                // do nothing (why is a portal defined?)
                break;
            case ChunkBoundary::Portal::HEAVEN:
                // heavens are always drawn ... unless we are an inside chunk
                if (!pChunk->isOutsideChunk())
                {
                    if (it->traverseMark_ == traversalData.nextMark_)
                        break;
                    ChunkSpace::Column * pCol =
                        pChunk->space()->column( it->centre, false );
                    if (pCol == NULL) break;
                    Chunk * pOutChunk = pCol->pOutsideChunk();
                    if (pOutChunk == NULL || pOutChunk->traverseMark() == traversalData.nextMark_)
                        break;

                    float nearDepth = 0.f;

                    it->pChunk = pOutChunk; // very dodgy!
                    Portal2DRef pWindow = it->traverse(
                        pChunk->transform(),
                        pChunk->transformInverse(),
                        cur.pClipPortal_,
                        traversalData,
                        &nearDepth );
                    it->pChunk = (Chunk*)ChunkBoundary::Portal::HEAVEN;

                    if (pWindow.valid() && !ignoreOutsidePortals)
                    {
                        PortalBounds pb;
                        if (pb.init( pWindow, nearDepth ))
                        {
                            outsidePortals.push_back( pb );
                        }
                    }
                }
                break;
            case ChunkBoundary::Portal::EARTH:
                // connection to earth could be used to draw terrain ... but
                // now terrain is added as a chunk item so this is unnecessary
                break;
            default:
                // ok, see if we can draw it then
                if (!it->pChunk->isBound() ||
                    it->traverseMark_ == traversalData.nextMark_ ||
                    it->pChunk == cur.pParent_)
                {
                    break;
                }

                float nearDepth = 0.f;

                // we can - draw away
                Portal2DRef pWindow = it->traverse(
                    pChunk->transform(),
                    pChunk->transformInverse(),
                    cur.pClipPortal_,
                    traversalData,
                    &nearDepth );
                if (pWindow.valid())
                {
                    if (!it->pChunk->isOutsideChunk())
                    {
                        if (it->pChunk->traverseMark() != traversalData.nextMark_)
                        {
                            it->pChunk->traverseMark( traversalData.nextMark_ );
                            chunks.push_back( it->pChunk );
                        }

                        //here, we could set up scissors rectangles or clip planes to
                        //confine drawing to just the region seen through the portal.
                        stack.push_back( PortalDrawState( it->pChunk, pWindow, pChunk ) );
                    }
                    else if (!ignoreOutsidePortals)
                    {
                        PortalBounds pb;
                        if (pb.init( pWindow, nearDepth ))
                        {
                            outsidePortals.push_back( pb );
                        }
                    }
                }
                break;
            }
        }
    }
}


/**
 *  This method culls the outside chunks
 *  @param chunks visible chunks are apended to this vector
 *  @param outsidePortals portals that the outside world are visible through
 */
void ChunkManager::cullOutsideChunks( ChunkVector& chunks, const PortalBoundsVector& outsidePortals )
{
    BW_GUARD_PROFILER( ChunkManager_cullOutsideChunks );
    // Create a temporary copy of the portal boundaries looking at the 
    // outside world
    static PortalBoundsVector portalBounds;
    portalBounds.assign( outsidePortals.begin(), outsidePortals.end() );

    // If there are no boundaries looking out, add one covering the entire 
    // screen and starts at the near plane
    if (outsidePortals.empty())
    {
        PortalBounds pb;
        pb.init( Portal2DRef(), Moo::rc().camera().nearPlane() );
        portalBounds.push_back( pb );
    }

    for (uint32 i = 0; i < portalBounds.size(); i++)
    {

        // Get the first portal boundary
        const PortalBounds& pb = portalBounds[i];

        // Get the projection matrix
        Matrix viewProj = Moo::rc().projection();

        // First we adjust the near plane distance to the near point on
        // the portal we are looking through
        const Moo::Camera& camera = Moo::rc().camera();

        float rcp = 1.f / (camera.farPlane() - pb.minDepth_);
        viewProj[2][2] = rcp * camera.farPlane();
        viewProj[3][2] = -rcp * camera.farPlane() * pb.minDepth_;

        // Make the view projection matrix
        viewProj.preMultiply( Moo::rc().view() );

        // Next, adjust the projection matrix to cover the 
        // rectangle of the portal
        Vector2 offset = (pb.max_ + pb.min_) * -0.5f;

        Matrix t;

        // Centre the projection matrix on the portal

        t.setIdentity();
        t[3][0] = offset.x;
        t[3][1] = offset.y;
        viewProj.postMultiply( t );


        // Scale the projection matrix to the portal bounds
        Vector2 scale( 2.f / (pb.max_.x - pb.min_.x), 
            2.f / (pb.max_.y - pb.min_.y) );

        t.setScale( scale.x , scale.y, 1 );

        viewProj.postMultiply( t );

        // Calculate the visible portions of the outside scene
        pCameraSpace_->getVisibleOutsideChunks( viewProj, chunks );
    }
}


/**
 *  This method inits the PortalBounds struct
 *  @param portal the 2d portal to create the bounds from
 *  @param minDepth the minimum depth of the points in the portal
 *  @return true if the portal bounds are valid
 */
bool ChunkManager::PortalBounds::init(Portal2DRef portal, float minDepth)
{
    minDepth_ = minDepth;

    portal2D_ = portal;

    // Get the min/max of the portal, clipped to the min/max of the viewport
    bool res = portal.valid();
    if (portal.valid() && portal.pVal())
    {
        Portal2D::V2Vector::const_iterator it = portal->points().begin();
        min_.set( 1, 1 );
        max_.set( -1, -1 );
        while (it != portal->points().end())
        {
            min_.x = std::min( min_.x, (*it).x );
            min_.y = std::min( min_.y, (*it).y );
            max_.x = std::max( max_.x, (*it).x );
            max_.y = std::max( max_.y, (*it).y );
            ++it;
        }

        res = min_.x < max_.x && min_.y < max_.y;
    }
    else if (portal.valid())
    {
        min_.set( -1, -1 );
        max_.set( 1, 1 );
    }
    return res;
}


/**
 *  Add this chunk which is on the fringe of the traversal
 *   (and should be drawn due to items that may be visible
 *   lent into other chunks which are in the traversal proper)
 */
void ChunkManager::addFringe( Chunk * pChunk )
{
    BW_GUARD;
    // don't allow fringe chunks to be added if we're drawing the fringe
    if (fringeHead_ == (Chunk*)1) return;

    // the furthest chunks will tend to be added first
    // (because they're added after the traversal recursion call)
    // so it is good to put newly added chunks on the head of the list
    pChunk->fringePrev( (Chunk*)1 );
    pChunk->fringeNext( fringeHead_ );
    if (fringeHead_ != NULL) fringeHead_->fringePrev( pChunk );
    fringeHead_ = pChunk;
}


/**
 *  Remove this chunk from the fringe of the traversal
 *
 *  @see addFringe
 */
void ChunkManager::delFringe( Chunk * pChunk )
{
    BW_GUARD;
    // don't allow fringe chunks to be removed if we're drawing the fringe
    if (fringeHead_ == (Chunk*)1) return;

    // fix up the list
    Chunk * pPrev = pChunk->fringePrev();
    Chunk * pNext = pChunk->fringeNext();
    if (pPrev != (Chunk*)1) pPrev->fringeNext( pNext );
    if (pNext != NULL)      pNext->fringePrev( pPrev );

    // fix up our head pointer
    if (pPrev == (Chunk*)1) fringeHead_ = pNext;

    // and clear the chunk's pointers
    pChunk->fringePrev( NULL );
    pChunk->fringeNext( NULL );
}


/**
 *  Explicitly load a given chunk (needed for bootstrapping,
 *  teleportation, and other unexplained phenomena)
 */
void ChunkManager::loadChunkExplicitly(
    const BW::string & identifier, GeometryMapping * pMapping, bool isOverlapper /*= false*/ )
{
    BW_GUARD_PROFILER( ChunkManager_loadChunkExplicitly );
    if( workingInSyncMode_ )
    {
        loadChunkNow( identifier, pMapping );
    }
    else
    {
        if( MainThreadTracker::isCurrentThreadMain() && !isOverlapper )
        {
            Chunk* chunk = findChunkByName( identifier, pMapping );

            // make sure it's not already loaded or being loaded
            if (chunk != NULL && !chunk->loaded() && std::find( loadingChunks_.begin(),
                loadingChunks_.end(), chunk) == loadingChunks_.end())
            {
                // ok, schedule it for loading ahead of time then
                this->loadChunk( chunk, false );
            }
        }
        else
        {
            SimpleMutexHolder smh( pendingChunksMutex_ );
            pendingChunks_.insert( std::make_pair( identifier, pMapping ) );
        }
    }
}

/**
 *  Adds a chunk to the given space. If called from the background thread,
 *  this is queued up to be added later on the main thread.
 */
void ChunkManager::addChunkToSpace( Chunk * pChunk, ChunkSpaceID spaceID )
{
    BW_GUARD;
    if (pChunk->isAppointed())
    {
        // It's already been added.
        return;
    }

    if( MainThreadTracker::isCurrentThreadMain() || workingInSyncMode_ )
    {
        ChunkSpacePtr pSpace = space(spaceID);
        if (pSpace)
        {
            // We don't use findOrAddChunk because we don't want to delete 
            // pChunk if it already exists (which findOrAddChunk does). It
            // is up to the caller (i.e. ChunkOverlapper) to make sure it 
            // cleans up in this case (i.e. ChunkOverlapper::bind).
            if (!pSpace->findChunk( pChunk->identifier(), pChunk->mapping()->name() ))
            {
                pSpace->addChunk( pChunk );
            }
        }
    }
    else
    {
        SimpleMutexHolder smh( pendingChunkPtrsMutex_ );
        pendingChunkPtrs_.insert( std::make_pair( pChunk, spaceID ) );
    }
}


Chunk* ChunkManager::findChunkByGrid( int16 x, int16 z, GeometryMapping * pMapping )
{
    BW_GUARD;
    IF_NOT_MF_ASSERT_DEV( pMapping )
    {
        return NULL;
    }

    ChunkSpacePtr pSpace = pMapping->pSpace();
    std::pair< int, int > coord( x, z );

    if ( !pSpace->gridChunks().count( coord ) )
        return NULL;


    GridChunkMap & gridChunks = pSpace->gridChunks();
    const BW::vector< Chunk* > & mappings = gridChunks[coord];
    Chunk * chunk = NULL;

    for (uint i = 0; i < mappings.size(); i++)
    {
        if (mappings[i]->mapping() == pMapping)
        {
            chunk = mappings[i];
            break;
        }
    }

    return chunk;
}


Chunk* ChunkManager::findOutdoorChunkByPosition( float x, float z, GeometryMapping * pMapping )
{
    ChunkSpacePtr pSpace = pMapping->pSpace();
    int16 gridX = pSpace->pointToGrid( x );
    int16 gridZ = pSpace->pointToGrid( z );

    return findChunkByGrid( gridX, gridZ, pMapping );
}


Chunk* ChunkManager::findChunkByName( const BW::string & identifier, 
    GeometryMapping * pMapping, bool createIfNotFound /*= true*/ )
{
    BW_GUARD;
    IF_NOT_MF_ASSERT_DEV( pMapping )
    {
        return NULL;
    }

    return pMapping->findChunkByName( identifier, createIfNotFound );
}

void ChunkManager::loadChunkNow( Chunk* chunk )
{
    BW_GUARD;
    if (!chunk)
    {
        CRITICAL_MSG( "Trying to load NULL chunk at %p\n", this );
    }

    if (chunk->loaded())
    {
        ERROR_MSG( "Trying to load loaded chunk %s at %p\n",
            chunk->identifier().c_str(), this );
        return;
    }

    loadingChunks_.push_back( chunk );

    ChunkLoader::loadNow( chunk );
}

void ChunkManager::loadChunkNow( const BW::string & identifier, GeometryMapping * pMapping )
{   
    BW_GUARD_PROFILER( ChunkManager_loadChunkNow );
    Chunk* chunk = findChunkByName( identifier, pMapping );

    // make sure it's not already loaded or being loaded
    if (chunk != NULL && !chunk->loaded() && std::find( loadingChunks_.begin(),
        loadingChunks_.end(), chunk) == loadingChunks_.end())
    {
        // ok, schedule it for loading ahead of time then
        this->loadChunkNow( chunk );
    }
}

void ChunkManager::processPendingChunks()
{
    SimpleMutexHolder smh( pendingChunksMutex_ );

    while( !pendingChunks_.empty() )
    {
        loadChunkExplicitly( pendingChunks_.begin()->first, pendingChunks_.begin()->second );
        pendingChunks_.erase( pendingChunks_.begin() );
    }
}

/**
 *  See if any chunks waiting to be loaded have been
 */
bool ChunkManager::checkLoadingChunks()
{
    BW_GUARD;
    bool anyChanges = false;

    for (uint i=0; i<loadingChunks_.size(); i++)
    {
        Chunk * pChunk = loadingChunks_[i];

        // Chunk has finished loading
        if (pChunk->loaded())
        {
            // ok, we have a new chunk, so stop
            // polling it to see if it's done
            loadingChunks_.erase( loadingChunks_.begin() + (i--) );

            bool isCondemned = pChunk->mapping()->condemned();

            // Note: pChunk->mapping() may be deleted by this call
            pChunk->loading( false );

            // make sure its mapping has not been condemned
            if (isCondemned)
            {
                pChunk->space()->unloadChunkBeforeBinding( pChunk );
                bw_safe_delete(pChunk);
                continue;
            }

            // otherwise bind it in
            if( !pChunk->isOutsideChunk() && pCameraSpace_ )
                pCameraSpace_->focus( cameraTrans_.applyToOrigin() );
            pChunk->bind( true );

            if (pChunk->isOutsideChunk() && pCameraSpace_)
            {
                const ChunkOverlappers::Overlappers & overlappers =
                    ChunkOverlappers::instance( *pChunk ).overlappers();

                if (!overlappers.empty())
                {
                    pCameraSpace_->focus( cameraTrans_.applyToOrigin() );

                    for (ChunkOverlappers::Overlappers::const_iterator iter = overlappers.begin();
                        iter != overlappers.end(); ++iter)
                    {
                        if ((*iter)->pOverlappingChunk()->isBound())
                        {
                            (*iter)->pOverlappingChunk()->bindPortals( true, true );
                        }
                    }
                }
            }

            if (waitingForTerrainLoad_)
            {
                ChunkTerrain* terrain = ChunkTerrainCache::instance( *pChunk ).pTerrain();

                while (terrain && terrain->doingBackgroundTask())
                {
                    BgTaskManager::instance().tick();
                    FileIOTaskManager::instance().tick();
                }
            }

            anyChanges = true;
        }

        // Chunk loading was cancelled
        else if (!pChunk->loading())
        {
            // ok, we have a new chunk, so stop
            // polling it to see if it's done
            loadingChunks_.erase( loadingChunks_.begin() + i );
            --i;
        }
    }

    return anyChanges;
}


/**
 *  This method cancels any chunks we can that are currently being loaded and
 *  pending chunks, that have yet to start loading.
 *  It waits for loading chunks to finish as there is currently no way to stop
 *  them. Can unload them without having to bind though.
 *  Cannot cancel marked/locked/non-removable chunks, they will be loaded
 *  and bound.
 *  There should only be non-removable chunks in pendingChunks_ or
 *  loadingChunks_ by the end of this function.
 */
void ChunkManager::cancelLoadingChunks()
{
    BW_GUARD;

    // Clear pending chunks - just don't load them
    {
        SimpleMutexHolder smh( pendingChunksMutex_ );

        BW::set<StrMappingPair> pendingChunksCopy;

        // Go through pending chunks and save non-removable ones
        BW::set<StrMappingPair>::const_iterator itr;
        for (itr = pendingChunks_.begin(); itr != pendingChunks_.end(); ++itr)
        {
            // Get chunk and check it's removable
            const BW::string& identifier = itr->first;
            GeometryMapping* pMapping = itr->second;
            Chunk* pChunk = this->findChunkByName( identifier, pMapping );
            if (!pChunk->removable())
            {
                pendingChunksCopy.insert( *itr );
            }
        }

        // Clear out removable chunks, keep non-removable ones
        pendingChunks_.swap( pendingChunksCopy );
    }
    // The only chunks left in pendingChunks_ are non-removable

    // If in sync mode, there should be no loading chunks
    if (workingInSyncMode_)
    {
        MF_ASSERT( loadingChunks_.empty() );
        return;
    }

    // Async mode, wait for loading chunks
    // Cancel loading chunks
    // - if the chunk is removable:
    //   Currently there is no way to actually cancel a LoadChunkTask,
    //   so we wait for the loading task to finish and then
    //   unload the chunk without binding.
    // - if the chunk is non-removable, leave it
    for (ChunkVector::size_type i = 0; i < loadingChunks_.size(); ++i)
    {
        Chunk * pChunk = loadingChunks_[i];

        // It's removable, wait for load, then unload
        if (pChunk->removable())
        {
            // Wait for loading task to finish
            while (!pChunk->loaded())
            {
                // Give some time to loading threads
                Sleep( 50 );
            }

            // Remove from loading list
            loadingChunks_.erase( loadingChunks_.begin() + i );
            --i;

            // Note: pChunk->mapping() may be deleted by this call
            pChunk->loading( false );

            // Unload without binding
            MF_ASSERT( !pChunk->isBound() );
            pChunk->space()->unloadChunkBeforeBinding( pChunk );
        }
    }
    // Only chunks left in loadingChunks_ are non-removable

}


/**
 *  This method switches to synchronous chunk loading
 *  (prevents background chunk loading).
 *  When in sync mode, all calls to load chunks will be done on the thread
 *  that called load.
 *  When switching to sync mode, wait for all background loading tasks to 
 *  finish. Any pending background tasks will be loaded in the foreground in
 *  the next tick.
 *  switchToSyncMode reference counts how many calls there have been.
 *  
 *  @param sync true to switch into sync mode, false to switch to async mode.
 */
void ChunkManager::switchToSyncMode( bool sync )
{
    BW_GUARD;
    if( !sync )
    {
        MF_ASSERT_DEV( workingInSyncMode_ > 0 );
        --workingInSyncMode_;
        return;
    }
    else if( sync && workingInSyncMode_ > 0 )
    {
        ++workingInSyncMode_;
        return;
    }
    ++workingInSyncMode_;
    while( busy() )
    {
        checkLoadingChunks();
        Sleep( 50 );
    }
}

void ChunkManager::switchToSyncTerrainLoad( bool sync )
{
    BW_GUARD;
    if( !sync )
    {
        MF_ASSERT_DEV( waitingForTerrainLoad_ > 0 );
        --waitingForTerrainLoad_;
    }
    else
    {
        ++waitingForTerrainLoad_;
    }
}

/// Bias in metres when determining whether or not a camera is inside an
/// overlapper in an outside chunk, which should cause that chunk to be
/// loaded before any other.
static const float CAMERA_INSIDE_OVERLAPPER_BIAS = 10.f;

/// Bias in metres when determining whether a chunk should remain 'wired'
/// in memory. Wired chunks are not removable even if they would otherwise
/// be, due to being near to the camera. This should probably be set a least
/// a little higher than CAMERA_INSIDE_OVERLAPPER_BIAS so it does not thrash.
static const float CAMERA_NEARBY_WIRED_BIAS = 20.f;

// Helper function to find an overlapper in a chunk
static Chunk * findOverlapper( Chunk * pInChunk, const Vector3 & pos,
    uint32 mark )
{
    BW_GUARD;   
#ifndef EDITOR_ENABLED
    if (ChunkOverlappers::instance.exists( *pInChunk ))
    {
        const ChunkOverlappers::Overlappers & overlappers =
            ChunkOverlappers::instance( *pInChunk ).overlappers();
        for (uint i = 0; i < overlappers.size(); i++)
        {
            ChunkOverlapper * pOverlapper = &*overlappers[i];
            if (pOverlapper->pOverlappingChunk()->isBound()) continue;        // online
            if (pOverlapper->pOverlappingChunk()->traverseMark() == mark) continue; // loading
            if (pOverlapper->bb().intersects( pos,
                CAMERA_INSIDE_OVERLAPPER_BIAS ))
            {
                Chunk * pChunk = pOverlapper->pOverlappingChunk();
                pChunk->traverseMark( mark );
                pChunk->pathSum( 0.f );
                return pChunk;
            }
        }
    }
#endif
    return NULL;
}

struct GridComp
{
    bool operator()( std::pair<int, int> grid1, std::pair<int, int> grid2 )
    {
        return grid1.first * grid1.first + grid1.second * grid1.second <
            grid2.first * grid2.first + grid2.second * grid2.second;
    }
};


/**
 *  Scan over the graph from the camera's chunk and see if
 *  there's anything else we want to load.
 *
 *  Returns true if any changes affecting focus grids were made.
 */
bool ChunkManager::scan()
{
    BW_GUARD_PROFILER( ChunkManager_scan );

    static int savedMaxLoadGrid = 0;
    static BW::vector<std::pair<int, int> > sortedGridBounds;

    ChunkSpacePtr pSpace = cameraChunk_->space();
    if( savedMaxLoadGrid < pSpace->pointToGrid( maxLoadPath_ ) )
    {
        savedMaxLoadGrid = pSpace->pointToGrid( maxLoadPath_ );

        sortedGridBounds.resize( 0 );
        sortedGridBounds.reserve( ( savedMaxLoadGrid * 2 + 1 ) * ( savedMaxLoadGrid * 2 + 1 ) );

        for( int i = -savedMaxLoadGrid; i < savedMaxLoadGrid + 1; ++i )
        {
            for( int j = -savedMaxLoadGrid; j < savedMaxLoadGrid + 1; ++j )
            {
                sortedGridBounds.push_back( std::make_pair( i, j ) );
            }
        }

        std::sort( sortedGridBounds.begin(), sortedGridBounds.end(), GridComp() );
    }

    cameraAtLastScan_ = cameraTrans_.applyToOrigin();
    noneLoadedAtLastScan_ = true;   

    uint32  mark = Chunk::nextMark();

    Chunk *     pMostWorthy[MAX_WORTHY_CHUNKS] = { NULL };
    size_t      pMostWorthSize = 0;

    int cameraGridX = pSpace->pointToGrid( cameraAtLastScan_.x );
    int cameraGridY = pSpace->pointToGrid( cameraAtLastScan_.z );

    if (canLoadChunks_)
    {
        PROFILER_SCOPED( ChunkManager_scan2 );

        // Load chunks only if we're allowed to.
        const ChunkSpace::GeometryMappings& dirMappings  = pSpace->getMappings();
        bool continueIterating = true;

        for( BW::vector<std::pair<int, int> >::iterator iter = sortedGridBounds.begin();
            iter != sortedGridBounds.end() && continueIterating; ++iter )
        {
            if( pSpace->gridToPoint( iter->first ) * pSpace->gridToPoint( iter->first ) +
                pSpace->gridToPoint( iter->second ) * pSpace->gridToPoint( iter->second ) >
                maxLoadPath_ * maxLoadPath_ )
                break;

            int x = cameraGridX + iter->first;
            int z = cameraGridY + iter->second;

            for (ChunkSpace::GeometryMappings::const_iterator mappingIter = dirMappings.begin();
                mappingIter != dirMappings.end(); mappingIter++)
            {
                GeometryMapping* mapping = mappingIter->second;

                if ( !mapping->inWorldBounds(x,z) )
                    continue;

                int lx,lz;
                mapping->gridToLocal( x,z,lx,lz );

                Chunk* chunk = findChunkByGrid( lx, lz, mapping );

                if ( !chunk )
                {
                    BW::string chunkName = mapping->outsideChunkIdentifier( lx, lz, false );

                    // make the chunk
                    chunk = new Chunk( chunkName, mapping );

                    // add it to its space's map of chunks
                    pSpace->addChunk( chunk );
                }

                if( chunk && !chunk->loading() && !chunk->loaded() )
                {
                    pMostWorthy[pMostWorthSize++] = chunk;
                    if (pMostWorthSize >= MAX_WORTHY_CHUNKS)
                    {
                        continueIterating = false;
                        break;
                    }
                }
            }
        }
    }

    // Get the position of the grid square the camera is in
    Vector3 cameraGridPos( 
        pSpace->gridToPoint( cameraGridX ),
        0,
        pSpace->gridToPoint( cameraGridY ) );

    static BW::vector< Chunk * > furthestChunks;
    furthestChunks.clear();
    furthestChunks.reserve( maxUnloadChunks_ );

    for (ChunkMap::iterator it = pCameraSpace_->chunks().begin();
        it != pCameraSpace_->chunks().end() && furthestChunks.size() < maxUnloadChunks_;
        ++it)
    {
        BW::vector<Chunk*>::reverse_iterator i;
        for (i = it->second.rbegin();
            i != it->second.rend() && furthestChunks.size() < maxUnloadChunks_; ++i)
        {
            if (!(*i)->isBound()) continue;
            if (!(*i)->removable()) continue;
            Vector3 origin = (*i)->transform().applyToOrigin();
            if( ( origin.x - cameraGridPos.x ) * ( origin.x - cameraGridPos.x ) + 
                ( origin.z - cameraGridPos.z ) * ( origin.z - cameraGridPos.z ) <=
                minUnloadPath_ * minUnloadPath_ )
                continue;
            if (!(*i)->isOutsideChunk())
            {
                Vector3 pos = (*i)->transform().applyToOrigin();
                BW::string identifier = (*i)->mapping()->outsideChunkIdentifier( pos );
                Chunk* chunk = findChunkByName( identifier, (*i)->mapping(), false );
                if (chunk != NULL && chunk->loaded())
                    continue;
            }
            else
            {
                bool allShellsLoaded = true;
                const ChunkOverlappers::Overlappers & overlappers =
                    ChunkOverlappers::instance( **i ).overlappers();
                for (uint i = 0; i < overlappers.size(); i++)
                {
                    ChunkOverlapper * pOverlapper = &*overlappers[i];
                    if (pOverlapper->pOverlappingChunk()->loading())
                    {
                        allShellsLoaded = false;
                        break;
                    }
                }
                if( !allShellsLoaded )
                    continue;
            }

            if ((*i)->boundingBox().intersects(
                cameraGridPos, CAMERA_NEARBY_WIRED_BIAS )) continue;

            furthestChunks.push_back( *i );
        }
        if (i != it->second.rend()) break;
    }

    // ok, we found some to load! finally! yay!
    for 
    (
        size_t pMostWorthIdx = 0;
        pMostWorthIdx < pMostWorthSize;
        ++pMostWorthIdx
    )
        {
            // only load it if we're not already loading too many
            // (we still do the scan in case others expect it)
            if (loadingChunks_.size() < MAX_LOADING_CHUNKS)
            {
                Chunk *chunk = pMostWorthy[pMostWorthIdx];
                this->loadChunk( chunk, almostZero( chunk->pathSum() ) );
                noneLoadedAtLastScan_ = false;
            }
            else
            {
                break;
        }
    }

    // Work out which loading chunk is the closest for the camera space
    if (pCameraSpace_)
    {
        float closestUnloadedChunk = FLT_MAX;
        float existing = pCameraSpace_->closestUnloadedChunk();

        for (uint i=0; i<loadingChunks_.size(); i++)
        {
            Chunk * pChunk = loadingChunks_[i];

            if ( pChunk->space() == pCameraSpace_ )
            {
                if (pChunk->boundingBoxReady())
                {
                    Vector3 origin = pChunk->boundingBox().centre();
                    float dist = sqrtf( ( origin.x - cameraAtLastScan_.x ) * ( origin.x - cameraAtLastScan_.x ) + 
                                        ( origin.z - cameraAtLastScan_.z ) * ( origin.z - cameraAtLastScan_.z ) );

                    if (dist < closestUnloadedChunk)
                    {
                        //DEBUG_MSG( "Replace closest unloaded chunk with %s (old dist=%f, new dist=%f).\n", 
                        //  pChunk->identifier().c_str(), currClosest, dist );
                        closestUnloadedChunk = dist;
                    }
                }
                else
                {
                    // if we don't have a bounding box, just set it to the existing.
                    if (existing < closestUnloadedChunk)
                    {
                        closestUnloadedChunk = existing;
                    }
                }
            }
        }

        pCameraSpace_->closestUnloadedChunk( closestUnloadedChunk );
    }

    bool anyChanges = false;

    // also get rid of the furthest one if it is far enough away
    if (!furthestChunks.empty())
    {
        noneLoadedAtLastScan_ = false;

        for (BW::vector< Chunk * >::iterator it = furthestChunks.begin();
            it != furthestChunks.end(); ++it)
        {
            IF_NOT_MF_ASSERT_DEV( (*it)->removable() )
            {
                continue;
            }

            // cut it free from its bindings
            (*it)->unbind( false );

            // and clean it out
            (*it)->unload();
        }

        // it is now unloaded but still appointed by its chunk space
        anyChanges = true;
    }

    return anyChanges;
}



/**
 *  Ah, we seem to have misplaced the camera. So what we do
 *  is load the chunk closest to the camera, and we keep doing
 *  this until the camera can find itself. Since we always
 *  load one chunk here (unless it's already loading), we're
 *  guaranteed to eventually find the camera even if it's
 *  hidden deep underground in a warren of chunks. And for
 *  most cases we should find it pretty quickly.
 *
 *  @note: This method shouldn't need to be called in the normal
 *  course of things - it is for emergencies only.
 *
 *  @note: 'Closest' above means as the crow flies, not visibility,
 *  because we can't calculate visibility if we haven't loaded
 *  those chunks yet!
 */
bool ChunkManager::blindpanic()
{
    BW_GUARD;
    uint32  mark = Chunk::nextMark();

    // if something's loading then make sure we exclude them
    for (ChunkVector::iterator it = loadingChunks_.begin();
        it != loadingChunks_.end();
        it++)
    {
        (*it)->traverseMark( mark );
        (*it)->pathSum( -1 );
    }

    const Vector3 & cameraPoint = cameraTrans_.applyToOrigin();

    // ok, now find the closest one
    Chunk * pFurthest = NULL;
    Chunk * pClosest = NULL;

    for (ChunkMap::iterator it = pCameraSpace_->chunks().begin();
        it != pCameraSpace_->chunks().end();
        it++)
    {
        for (uint i = 0; i < it->second.size(); i++)
        {
            Chunk * pChunk = it->second[i];
            if (!pChunk->isBound()) continue;
            if (pChunk->traverseMark() == mark) continue;

            // record some sort of distance to it
            pChunk->traverseMark( mark );
            pChunk->pathSum( (pChunk->centre() - cameraPoint).lengthSquared() );

            // see if it's the furthest loaded chunk
            if ((pFurthest == NULL || pChunk->pathSum() > pFurthest->pathSum())
                && pChunk->removable() && !pChunk->boundingBox().intersects(
                    cameraPoint, CAMERA_NEARBY_WIRED_BIAS ))
            {
                pFurthest = pChunk;
            }

            // see if it's connected to the closest unloaded chunk
            float   canDist = 0.f;
            Chunk * pCandidate = pChunk->findClosestUnloadedChunkTo(
                cameraPoint, &canDist );

            if (pCandidate != NULL && pCandidate->traverseMark() != mark &&
                (pClosest == NULL || canDist < pClosest->pathSum()))
            {
                MF_ASSERT_DEV( !pCandidate->isBound() );
                pClosest = pCandidate;
                pClosest->pathSum( canDist );
            }
        }
    }

    bool anyChanges = false;

    // get rid of the furthest one if it is far enough away
    if (pFurthest != NULL && pFurthest->pathSum() > minUnloadPath_*minUnloadPath_)
    {
        MF_ASSERT_DEV( pFurthest->removable() );

        if( pFurthest->removable() )
        {
            noneLoadedAtLastScan_ = false;

            // cut it free from its bindings
            pFurthest->unbind( false );

            // and clean it out
            pFurthest->unload();

            // it is now unloaded but still appointed by its chunk space
            anyChanges = true;
        }
    }

    // if we still can't find anything it's because none of the
    // loaded chunks reference _ANY_ unloaded chunks.
    // unlikely, but it happened the very first time this
    // algorithm executed :)
    // [actually, happens when still loading say first chunk]
    if (pClosest == NULL)
    {
        return anyChanges;
    }

    // ok, load that one then!
    if (loadingChunks_.size() < 3)
    {
        // as long as there aren't too many already loading
        this->loadChunk( pClosest, true );
    }

    return anyChanges;
}


/**
 *  This method automatically bootstraps a seed chunk into the space
 *  containing the camera. It does this by assuming a naming format
 *  for chunks and selecting one to load accordingly.
 */
bool ChunkManager::autoBootstrapSeedChunk()
{
    BW_GUARD;
    // first see if we are already finding a seed
    if (!findSeedTask_)
    {
        TRACE_MSG( "ChunkManager::autoBootstrapSeedChunk: "
            "seed chunk NULL so starting search\n" );

        // no, so get the loader to do it in its own thread
        findSeedTask_ = ChunkLoader::findSeed( &*pCameraSpace_, this->cameraNearPoint() );
    }
    // see if the operation has completed
    else if (findSeedTask_->isComplete())
    {
        bool destroyChunk = false;
        Chunk* pChunk = findSeedTask_->foundSeed();
        if (pChunk)
        {
            TRACE_MSG( "ChunkManager::autoBootstrapSeedChunk: "
            "seed chunk determined: %s\n", 
            pChunk->identifier().c_str() );

            bool isCondemned = pChunk->mapping()->condemned();
            
            // make sure its mapping has not been condemned
            if (isCondemned)
            {
                destroyChunk = true; // it's just a stub so ok to delete it
            }
            else
            {
                // see if it was for our space
                if (pChunk->space() == pCameraSpace_)
                {
                    Chunk * pFoundChunk = pCameraSpace_->findChunk( 
                        pChunk->identifier(), 
                        pChunk->mapping()->name() );
                    if (pFoundChunk)
                    {
                        pChunk = pFoundChunk; // use existing chunk
                        destroyChunk = true;  // new one should be destroyed
                    }
                    else
                    {
                        pCameraSpace_->addChunk(pChunk);
                    }
                    if (!pChunk->loading() && !pChunk->loaded())
                    {
                        this->loadChunk( pChunk, true );
                        TRACE_MSG( "ChunkManager::autoBootstrapSeedChunk: "
                            "seed chunk submitted for loading\n" );
                    }

                    pCameraSpace_->closestUnloadedChunk( 0.f );
                }
                else
                {
                    // otherwise just delete it and wait to be called again
                    destroyChunk = true;
                }
            }
        }
        else
        {
            destroyChunk = true; // no chunk there actually.
        }

        findSeedTask_->release( destroyChunk );
        findSeedTask_ = NULL;

        // note that if the operation completed but did not find any chunk
        // then pFoundSeed_ will be set to NULL, which is exactly what we want
    }
    // otherwise just wait for the find seed operation to complete

    /*
    // see what the space has to suggest
    Chunk * pChunk = pCameraSpace_->guessChunk( this->cameraNearPoint() );

    // if it came up with something then use that
    if (pChunk != NULL)
    {
        MF_ASSERT_DEV( !pChunk->ratified() );
        pCameraSpace_->addChunk( pChunk );
        MF_ASSERT_DEV( !pChunk->loaded() );
        this->loadChunk( pChunk, true );
    }

    // otherwise do nothing ... there's probably nothing mapped into the space
    */

    return false;
}



/**
 *  Load the given unloaded chunk
 */
void ChunkManager::loadChunk( Chunk * pChunk, bool highPriority )
{
    BW_GUARD_PROFILER( ChunkManager_loadChunk );

    if( workingInSyncMode_ )
    {
        loadChunkNow( pChunk );
        return;
    }
    if (!pChunk)
    {
        CRITICAL_MSG( "Trying to load NULL chunk at %p\n", this );
    }
    /*
    if (pChunk->spaceID() != 0)
    {
        CRITICAL_MSG( "Trying to load corrupt chunk at %p\n", this );
    }
    */
    if (pChunk->loaded())
    {
        ERROR_MSG( "Trying to load loaded chunk %s at %p\n",
            pChunk->identifier().c_str(), this );
        return;
    }
    //MF_ASSERT_DEV( pChunk && !pChunk->loaded() );

    loadingChunks_.push_back( pChunk );

    ChunkLoader::load( pChunk, highPriority ? BgTaskManager::HIGH : BgTaskManager::DEFAULT );
}


/**
 *  This method returns the space with the given id, creating it if necessary.
 */
ChunkSpacePtr ChunkManager::space( ChunkSpaceID id, bool createIfMissing )
{
    BW_GUARD;
    // Spaces are only every created or deleted (through last ref thrown away)
    // from the main thread, so we don't need to worry about concurrency here.
    // The loading thread can increment references when it creates a stub chunk,
    // but only from an existing held reference.
    ChunkSpaces::iterator it = spaces_.find( id );
    if (it != spaces_.end()) return it->second;

    if (createIfMissing)
    {
        if ( id != NULL_CHUNK_SPACE )
        {
            return new ChunkSpace( id );
        }
        else
        {           
            ERROR_MSG("Somebody tried to create space with NULL Space ID %d\n",
                NULL_CHUNK_SPACE );
        }
    }
    return NULL;
};

/**
 *  This method returns the space that the camera is currently in.
 *  Note that it can return NULL
 *
 *  @return The chunk the camera is in, or NULL if there is no current space.
 */
ChunkSpacePtr ChunkManager::cameraSpace() const
{
    return pCameraSpace_;
}

/**
 *  This method clears out all spaces
 *
 *  @param  keepClientOnlySpaces    If true this function will leave spaces
 *                                  created locally intact.
 */
void ChunkManager::clearSpace( ChunkSpace * pSpace )
{
    {
        SimpleMutexHolder smh( pendingChunksMutex_ );
        pendingChunks_.clear();
    }

    pSpace->clear();

    // Remove pending ptrs for this space, before the pSpace smartpointer kills it
    SimpleMutexHolder smh( pendingChunkPtrsMutex_ );
    for (BW::set<ChunkPtrSpaceIDPair>::iterator it = pendingChunkPtrs_.begin();
        it != pendingChunkPtrs_.end();)
    {
        Chunk* pendingChunk = (*it).first;
        bool wasDeleted = deletedChunks_.find( pendingChunk ) != deletedChunks_.end();

        if (wasDeleted || pendingChunk->space() == pSpace)
        {
            if (wasDeleted)
            {
                deletedChunks_.erase( pendingChunk );
            }
            else
            {
                bw_safe_delete(pendingChunk);
            }
            it = pendingChunkPtrs_.erase( it );
        }
        else
        {
            ++it;
        }
    }
}

/**
 *  This is called when a chunk is being deleted. This allows us to
 *  clean up any references we may have to the chunk.
 *
 *  @param  pChunk  The mapping that is being deleted
 *
 */
void ChunkManager::chunkDeleted( Chunk* pChunk )
{
    deletedChunks_.insert( pChunk );

#if UMBRA_ENABLE
    ChunkVector::iterator chunIt = std::find( 
        umbraChunkShadowCasters_.begin(), 
        umbraChunkShadowCasters_.end(), 
        pChunk 
    );

    if ( chunIt != umbraChunkShadowCasters_.end() ) 
    {
        umbraChunkShadowCasters_.erase( chunIt );
    }
#endif // UMBRA_ENABLE
}

/**
 *  This is called when a GeometryMapping has been condemned.
 *
 *  @param  pMapping    The mapping that has been condemned.
 *
 */
void ChunkManager::mappingCondemned( GeometryMapping* pMapping )
{
    // Clear out chunks pending for this mapping.
    {
        SimpleMutexHolder smh( pendingChunkPtrsMutex_ );

        BW::set<ChunkPtrSpaceIDPair>::iterator it = pendingChunkPtrs_.begin();
        while (it != pendingChunkPtrs_.end())
        {
            BW::set<Chunk*>::iterator itDel = deletedChunks_.find( it->first );

            const bool wasDeleted = itDel != deletedChunks_.end();
            if (wasDeleted)
            {
                // if it was deleted then we don't know what the mapping
                // was anyway because the chunk no longer exists
                deletedChunks_.erase( itDel );
                it = pendingChunkPtrs_.erase( it );
            }
            else if (it->first->mapping() == pMapping)
            {
                Chunk* chunk = it->first;
                bw_safe_delete( chunk );
                it = pendingChunkPtrs_.erase( it );
            }
            else
            {
                ++it;
            }
        }
    }

    {
        SimpleMutexHolder smh( pendingChunksMutex_ );

        BW::set<StrMappingPair>::iterator it = pendingChunks_.begin();
        while (it != pendingChunks_.end())
        {
            if (it->second == pMapping)
            {
                it = pendingChunks_.erase( it );
            }
            else
            {
                ++it;
            }
        }
    }
}


/**
 *  This static method returns the singleton ChunkManager instance.
 */
ChunkManager & ChunkManager::instance()
{
    SINGLETON_MANAGER_WRAPPER( ChunkManager )
    static  ChunkManager    chunky;
    return chunky;
}


static int g_treeLevel = 0;
static BW::string g_str;

void ChunkManager::drawTreeBranch( Chunk * pChunk, const BW::string & why )
{
    if (!g_treeLevel) g_str = "";

    //for (int i=0; i < g_treeLevel; i++) g_str += "\t";
    //g_str += pChunk->identifier() + why + "\n";
    //when enabling above also enable s_specialConsoleString_ concat above
    g_treeLevel++;
}

void ChunkManager::drawTreeReturn()
{
    g_treeLevel--;
}

const BW::string & ChunkManager::drawTree()
{
    return g_str;
}

/**
 * Set suitable values for maxLoadPath and minUnloadPath based on a given far
 * plane
 */
void ChunkManager::autoSetPathConstraints( float farPlane )
{
    BW_GUARD;
    if (pCameraSpace_ == NULL)
    {
        // I'm not sure if this can happen... But we probably don't want to die if it does.
        WARNING_MSG( "ChunkManager::autoSetPathConstraints: Called without pCameraSpace_\n" );
        return;
    }
    const float gridSize = pCameraSpace_->gridSize();
    if (gridSize == 0.f)
    {
        // Again, not sure if this case can occur...
        WARNING_MSG( "ChunkManager::autoSetPathConstraints: Called with pCameraSpace_ without gridSize\n" );
        return;
    }
    const float diagonalGridResolution = 
        sqrtf(gridSize*gridSize +
            gridSize*gridSize);

    //need to load up one more chunk than the hypotenuse of the far plane,
    //so walking forward doesn't reveal unloaded chunks.
    maxLoadPath_ = 
        sqrtf( farPlane*farPlane*2 ) + diagonalGridResolution;

    // Unload path epsilon as we are using quantised values
    // the unload path distance between two chunks is modified
    // a bit so that we don't get conflicts as a result of 
    // floating point inaccuracies
    const float UNLOAD_PATH_EPSILON = 1.01f;

    //minUnloadPath is one more chunk, for antihysteresis
    //TODO : allow configurable antihysteresis radius.
    minUnloadPath_ = 
        maxLoadPath_ + diagonalGridResolution * UNLOAD_PATH_EPSILON;
}

float ChunkManager::closestUnloadedChunk( ChunkSpacePtr pSpace ) const
{
    return pSpace->closestUnloadedChunk();
}

int ChunkManager::s_chunksTraversed = 0;
int ChunkManager::s_chunksVisible   = 0;
int ChunkManager::s_chunksReflected = 0;
int ChunkManager::s_visibleCount    = 0;
int ChunkManager::s_drawPass        = 0;
bool ChunkManager::s_drawVisibilityBBoxes = false;


/**
 *  This method is called by a chunk space to add itself to our list
 */
void ChunkManager::addSpace( ChunkSpace * pSpace )
{
    BW_GUARD;
    IF_NOT_MF_ASSERT_DEV( spaces_.find( pSpace->id() ) == spaces_.end() )
    {
        return;
    }

    spaces_.insert( std::make_pair( pSpace->id(), pSpace ) );
}


/**
 *  This method is called by a chunk space to remove itself from our list
 */
void ChunkManager::delSpace( ChunkSpace * pSpace )
{
    BW_GUARD;
    if (!initted_) return;

    // As we are operating on the pending chunks, grab the mutex
    pendingChunksMutex_.grab();

    // if one mapping belongs to 2 spaces, this code will not work
    // but if this will ever happen, it will break everything
    const ChunkSpace::GeometryMappings& mappings = pSpace->getMappings();
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (BW::set<std::pair<BW::string, GeometryMapping*> >::iterator iter =
                    pendingChunks_.begin();
            iter != pendingChunks_.end() && !changed; ++iter)
        {
            for (ChunkSpace::GeometryMappings::const_iterator mpiter = mappings.begin();
                mpiter != mappings.end(); ++mpiter)
            {
                if (iter->second == mpiter->second)
                {
                    pendingChunks_.erase( iter );
                    changed = true;
                    break;
                }
            }
        }
    }

    // We are done with the pending chunks, give mutex
    pendingChunksMutex_.give();

    ChunkSpaces::iterator found = spaces_.find( pSpace->id() );
    MF_ASSERT_DEV( found != spaces_.end() );
    MF_ASSERT_DEV( pSpace->refCount() == 0 );
    if( found != spaces_.end() )
        spaces_.erase( found );
}

/**
 *  This constructor adds one to the synced mode reference count.
 */
ScopedSyncMode::ScopedSyncMode()
{
    ChunkManager::instance().switchToSyncMode(true);
}


/**
 *  This destructor decreases the synced mode reference count by one.
 */
ScopedSyncMode::~ScopedSyncMode()
{
    ChunkManager::instance().switchToSyncMode(false);
}

BW_END_NAMESPACE

// chunk_manager.cpp
