#include "pch.hpp"

#include "chunk_space.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/dogwatch.hpp"

#include "physics2/hulltree.hpp"
#include "physics2/quad_tree.hpp"
#include "physics2/worldtri.hpp"

#include "resmgr/bwresource.hpp"

#include "chunk.hpp"
#include "chunk_obstacle.hpp"

#include "grid_traversal.hpp"

#include "chunk_manager.hpp"
#include "romp/time_of_day.hpp"

#include "moo/deferred_decals_manager.hpp"
#include "moo/renderer.hpp"

#include "chunk_quad_tree.hpp"

#if UMBRA_ENABLE
#include <ob/umbraCell.hpp>
#endif

#include <algorithm>

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: ChunkSpace
// ----------------------------------------------------------------------------

/**
 *  Constructor.
 */
ClientChunkSpace::ClientChunkSpace( ChunkSpaceID id ) :
    BaseChunkSpace( id ),
    ticking_( false ),
    pOutLight_( NULL ),
    sunLight_( NULL ),
    ambientLight_( 0.1f, 0.1f, 0.1f, 1.f ),
    lights_( new Moo::LightContainer() ),
    enviro_(id)
#if UMBRA_ENABLE
    ,umbraCell_( NULL )
#endif
{
}


/**
 *  Destructor.
 */
ClientChunkSpace::~ClientChunkSpace()
{
    BW_GUARD;

    // We have to call the base class' "fini" method early because it results
    // in calls to our "delTickItem". If we don't do this here, "delTickItem"
    // will be called from the base class destructor AFTER this derived object
    // has been deleted.
    fini();

    this->blurSpace();
#if UMBRA_ENABLE
    if (umbraCell_)
    {
        umbraCell_->release();
    }
#endif
}



/**
 *  We have received some settings from a mapping.
 *  If we haven't set anything up, then now's the time to use them.
 */
void ClientChunkSpace::mappingSettings( DataSectionPtr pSS )
{
    BW_GUARD;
    if (sunLight_) return;

    enviro_.load( pSS );

    //-- ToDo (b_sviglo): reconsider.
    if(Renderer::pInstance()->pipelineType() != IRendererPipeline::TYPE_FORWARD_SHADING)
        DecalsManager::instance().setFadeRange( pSS->readFloat( "decals/fadeStart", 100.0f),
        pSS->readFloat( "decals/fadeEnd", 120.0f));

    sunLight_ = new Moo::DirectionalLight(
        Moo::Colour( 0.8f, 0.5f, 0.1f, 1.f ),
        Vector3( 0, 1 ,0 ) );
    lights_->addDirectional( sunLight_ );
    lights_->ambientColour( ambientLight_ );

    if (enviro_.timeOfDay() != NULL)
    {
        this->heavenlyLightSource( &enviro_.timeOfDay()->lighting() );
    }
}




/**
 *  Blur the whole space
 */
void ClientChunkSpace::blurSpace()
{
    // move the focus miles away so it'll drop all its references
    currentFocus_.origin( currentFocus_.originX() + 10000, 0 );
}


/**
 *  Clear out all loaded stuff from this space
 */
void ClientChunkSpace::clear()
{
    BW_GUARD;
    this->blurSpace();

    this->homeless_.clear();

    this->BaseChunkSpace::clear();
#if UMBRA_ENABLE
    if (umbraCell_)
    {
        umbraCell_->release();
        umbraCell_ = NULL;
    }
#endif
}


extern BW::string g_specialConsoleString;
//#define HULL_DEBUG(x) g_specialConsoleString += x
#define HULL_DEBUG(x) (void)0


uint64 g_CSCTimeLimit;

#ifndef MF_SERVER
//extern uint64 g_hullTreeAddTime, g_hullTreePlaneTime, g_hullTreeMarkTime;


/**
 *  This method sets the focus point for this space
 */
void ClientChunkSpace::focus( const Vector3 & point )
{
    BW_GUARD_PROFILER(ClientChunkSpace_focus);
    static DogWatch dwFocus( "Focus Chunk" );
    ScopedDogWatch sdw( dwFocus );

    // see also max scan path in chunk_manager.cpp
    const int focusRange = ColumnGrid::SPANH;

    // figure out the grid square this is in
    const float gridSize = this->gridSize();
    int cx = int(point.x / gridSize);       if (point.x < 0.f) cx--;
    int cz = int(point.z / gridSize);       if (point.z < 0.f) cz--;

    if (currentFocus_.originX() == cx && currentFocus_.originZ() == cz
        && blurred_.empty())
    {
        bool changed = false;

        for (int z = cz - ColumnGrid::SPANH; z <= cz + ColumnGrid::SPANH && !changed; z++)
        {
            for (int x = cx - ColumnGrid::SPANH; x <= cx + ColumnGrid::SPANH && !changed; x++)
            {
                if (Column* pCol = currentFocus_( x, z ))
                {
                    bool soft = x <= cx-(focusRange-1) || x >= cx+(focusRange-1) ||
                        z <= cz-(focusRange-1) || z >= cz+(focusRange-1);

                    changed = pCol->isStale() || pCol->soft() != soft;
                }
            }
        }

        if (!changed)
        {
            return;
        }
    }

    // tell it to the grid
    currentFocus_.origin( cx, cz );

    // also delete any columns that are stale
    for (int x = cx - ColumnGrid::SPANH; x <= cx + ColumnGrid::SPANH; x++)
    {
        for (int z = cz - ColumnGrid::SPANH; z <= cz + ColumnGrid::SPANH; z++)
        {
            Column * & rpCol = currentFocus_( x, z );
            if (rpCol != NULL && rpCol->isStale())
            {
                bw_safe_delete(rpCol);
            }
        }
    }

    size_t bs = blurred_.size();
    //uint64 timeTaken = timestamp();
    g_CSCTimeLimit = timestamp() + stampsPerSecond() * 2 / 1000;
    bool hitTimeLimit = false;

    //g_hullTreeAddTime = 0;
    //g_hullTreePlaneTime = 0;
    //g_hullTreeMarkTime = 0;

    // cache any added obstacles for later sorting
//  Column::cacheControl( true );

    // focus any chunks that are now in range
    for (uint i = 0; i < blurred_.size(); i++)
    {
        Chunk * pChunk = blurred_[i];

        const Vector3 & cen = pChunk->centre();
        int nx = int(cen.x / gridSize); if (cen.x < 0.f) nx--;
        int nz = int(cen.z / gridSize); if (cen.z < 0.f) nz--;
        nx -= cx;
        nz -= cz;

        if (-focusRange <= nx && nx <= focusRange &&
            -focusRange <= nz && nz <= focusRange)
        {
            // skip this one if we're outta time
            if (hitTimeLimit && (
                nx <= -(focusRange-1) || nx >= (focusRange-1) ||
                nz <= -(focusRange-1) || nz >= (focusRange-1))) continue;

            // this chunk is no longer blurred
            blurred_.erase( blurred_.begin() + i );
            i--;

            // see if this chunk is new to (nx,nz) and adjacent columns,
            // close them if it isn't
            for (int x = nx - 1; x <= nx + 1; x++) for (int z = nz - 1; z <= nz + 1; z++)
            {
                Column * pCol = currentFocus_( cx + x, cz + z );
                if (pCol != NULL) pCol->shutIfSeen( pChunk );
            }

            // do the actual focussing work
            pChunk->focus();
            // it's ok for a chunk to re-add itself on failure to focus,
            // 'coz it'll go to the end of our vector (not that chunks
            // currently ever fail to focus)

            // open all the columns, and mark them as having seen this chunk
            for (int x = nx - 1; x <= nx + 1; x++) for (int z = nz - 1; z <= nz + 1; z++)
            {
                Column * pCol = currentFocus_( cx + x, cz + z );
                if (pCol != NULL) pCol->openAndSee( pChunk );
            }

            // get out now if we're an edge chunk
            if (timestamp() > g_CSCTimeLimit) hitTimeLimit = true;
            // time taken to cache data should be somewhat proportional to
            // time taken to construct hull tree.
#ifdef EDITOR_ENABLED
            hitTimeLimit = false;
#endif//
        }

    }

    // let every column know if it's soft (this sucks I know!)
    for (int x = cx - ColumnGrid::SPANH; x <= cx + ColumnGrid::SPANH; x++)
    {
        for (int z = cz - ColumnGrid::SPANH; z <= cz + ColumnGrid::SPANH; z++)
        {
            Column * pCol = currentFocus_( x, z );
            if (pCol != NULL) pCol->soft(
                x <= cx-(focusRange-1) || x >= cx+(focusRange-1) ||
                z <= cz-(focusRange-1) || z >= cz+(focusRange-1) );
        }
    }

    // if we focussed any chunks then see if any homeless items
    // would prefer to live in them now instead
    if (bs != blurred_.size())
    {
        for (int i = int(homeless_.size()-1); i >= 0; i--)
        {
            i = std::min( i, int(homeless_.size()-1) );
            ChunkItemPtr pHomelessItem = homeless_[i];
            pHomelessItem->nest( static_cast<ChunkSpace*>( this ) );
        }
    }
}

#if UMBRA_ENABLE
Umbra::OB::Cell* ClientChunkSpace::umbraCell()
{
    if (umbraCell_ == NULL)
    {
        umbraCell_ = Umbra::OB::Cell::create();
    }

    return umbraCell_;
}
#endif

/**
 *  Everyone's favourite function - tick!
 */
void ClientChunkSpace::tick( float dTime )
{
    BW_GUARD_PROFILER( ClientChunkSpace_tick );

    // start the embargo on chunk item changes
    ticking_ = true;

    SimpleMutexHolder smh( tickMutex_ );

    {
        PROFILER_SCOPED( ClientChunkSpace_tickPendingItems );
        static DogWatch dwTickItems( "Tick Pending Items" );
        ScopedDogWatch sdw( dwTickItems );

        // Get items just marked as tickItems in the loading thread.
        for (BW::list<ChunkItemPtr>::iterator it =
            pendingTickItems_.begin();
            it != pendingTickItems_.end(); )
        {
            if ((*it)->chunk() && (*it)->chunk()->isBound())
            {
                tickItems_.push_back( *it );
                it = pendingTickItems_.erase( it );
            }
            else
            {
                ++it;
            }
        }
    }

    {
        PROFILER_SCOPED( ClientChunkSpace_tickItems );
        static DogWatch dwTickItems( "Tick Chunk Items" );
        ScopedDogWatch sdw( dwTickItems );
    
        for (BW::list<ChunkItemPtr>::iterator it = tickItems_.begin();
            it != tickItems_.end();
            ++it)
        {
            (*it)->tick( dTime );
        }
    }

    {
        PROFILER_SCOPED( ClientChunkSpace_tickHomelessItems );
        static DogWatch dwHomeless( "Tick Homeless Chunk Items" );
        ScopedDogWatch sdw( dwHomeless );

        // and any homeless items
        for (BW::vector<ChunkItemPtr>::iterator it = homeless_.begin();
            it != homeless_.end();
            ++it)
        {
            (*it)->tick( dTime );
        }
    }

    // ok, chunk items can move around again now
    ticking_ = false;
}


/**
 *  This method re-calculates the LOD and animates ChunkItems within
 *  the ChunkSpace. It also updates the sun light and chunk visibility boxes.
 */
void ClientChunkSpace::updateAnimations()
{
    BW_GUARD_PROFILER( ClientChunkSpace_updateAnimations );

    // first update our lighting
    this->updateHeavenlyLighting();
    
    {
        SimpleMutexHolder smh( updateMutex_ );

        {
            PROFILER_SCOPED( ClientChunkSpace_updatePendingItems );
            static DogWatch dwUpdateItems( "Update Pending Items" );
            ScopedDogWatch sdw( dwUpdateItems );

            // Get items just marked as updateItems in the loading thread.
            for (BW::list<ChunkItem*>::iterator it =
                pendingUpdateItems_.begin();
                it != pendingUpdateItems_.end(); )
            {
                if ((*it)->chunk() && (*it)->chunk()->isBound())
                {
                    updateItems_.push_back( *it );
                    it = pendingUpdateItems_.erase( it );
                }
                else
                {
                    ++it;
                }
            }
        }

        {
            PROFILER_SCOPED( ClientChunkSpace_updateChunkItems );
            static DogWatch dwUpdateItems( "Update Chunk Items" );
            ScopedDogWatch sdw( dwUpdateItems );

            for (BW::list<ChunkItem*>::iterator it = updateItems_.begin();
                it != updateItems_.end();
                ++it)
            {
                // put our world transform on the render context
                ChunkItem* chunkItem = (*it);
                MF_ASSERT( chunkItem != NULL );
                MF_ASSERT( chunkItem->chunk() != NULL );
                MF_ASSERT( chunkItem->wantsUpdate() );

                // Chunk item might not need an animation update if it's
                // lodded out
                // Always update as we need to calculate new lod

                chunkItem->updateAnimations();
            }
        }

        {
            PROFILER_SCOPED( ClientChunkSpace_updateHomelessItems );
            static DogWatch dwHomeless( "Update Homeless Chunk Items" );
            ScopedDogWatch sdw( dwHomeless );

            // and any homeless items
            for (BW::vector<ChunkItemPtr>::iterator it = homeless_.begin();
                it != homeless_.end();
                ++it)
            {
                // put our world transform on the render context
                ChunkItemPtr chunkItem = (*it);
                MF_ASSERT( chunkItem.exists() );
                MF_ASSERT( chunkItem->chunk() == NULL );

                if (chunkItem->wantsUpdate())
                {
                    chunkItem->updateAnimations();
                }
            }
        }
    }

    {
        PROFILER_SCOPED(ClientChunkSpace_updateVisibility);
        // Update the visibility bounding boxes for outside chunks
        // in the quad tree
        Chunk ** tmpVisibilityUpdates = 0;
        size_t tmpVisibilityUpdatesCount = 0;
        // We have to prevent visibilityUpdates_ changes while working with it,
        // so we can simply make local copy of the vector under the mutex
        SimpleMutexHolder smh( visibilityMutex_ );
        tmpVisibilityUpdatesCount = visibilityUpdates_.size();
        if (tmpVisibilityUpdatesCount)
        {
            tmpVisibilityUpdates = (Chunk **)bw_alloca( sizeof( Chunk*) * tmpVisibilityUpdatesCount );
            memcpy( tmpVisibilityUpdates, &visibilityUpdates_[0], sizeof( Chunk* ) * tmpVisibilityUpdatesCount );
            visibilityUpdates_.clear();
        }
        for (size_t i = 0; i < tmpVisibilityUpdatesCount; ++i)
        {
            Chunk* pChunk = tmpVisibilityUpdates[i];
            MF_ASSERT(pChunk);
            if (pChunk->updateVisibilityBox())
            {
                addOutsideChunkToQuadTree( pChunk );
            }
        }
    }
}

/**
 *  This method is called by our update, and occasionally by the world editor.
 */
void ClientChunkSpace::updateHeavenlyLighting()
{
    BW_GUARD;
    if (pOutLight_ != NULL && sunLight_)
    {
        sunLight_->colour( Moo::Colour( pOutLight_->sunColour ) );

        //the dawn/dusk sneaky swap changes moonlight for
        //sunlight when the moon is brighter

        Vector3 lightPos = pOutLight_->mainLightTransform().applyPoint( Vector3( 0, 0, -1 ) );
        Vector3 dir = Vector3( 0, 0, 0 ) - lightPos;
        dir.normalise();
        sunLight_->direction( dir );
        sunLight_->worldTransform( Matrix::identity );//a hack to set worldDirection_ as direction_

        ambientLight_ = Moo::Colour( pOutLight_->ambientColour );
        lights_->ambientColour( ambientLight_ );
    }
}
#endif // MF_SERVER


/**
 *  This method adds a tickable chunk item to the tick list
 */
void ClientChunkSpace::addTickItem( ChunkItemPtr pItem )
{
    BW_GUARD;
    SimpleMutexHolder smh( tickMutex_ );

    if (!MainThreadTracker::isCurrentThreadMain())
    {
        pendingTickItems_.push_back( pItem );
    }
    else
    {
        tickItems_.push_back( pItem );
    }
}


void ClientChunkSpace::addUpdateItem( ChunkItem* pItem )
{
    BW_GUARD;
    MF_ASSERT( pItem->wantsUpdate() );

    SimpleMutexHolder smh( updateMutex_ );

    if (!MainThreadTracker::isCurrentThreadMain())
    {
        pendingUpdateItems_.push_back( pItem );
    }
    else
    {
        updateItems_.push_back( pItem );
    }
}


/**
 *  This method removes a tickable chunk item from the tick list
 */
void ClientChunkSpace::delTickItem( ChunkItemPtr pItem )
{
    BW_GUARD_PROFILER( ClientChunkSpace_delTickItem );
    SimpleMutexHolder smh( tickMutex_ );

    BW::list<ChunkItemPtr>::iterator found =
        std::find( tickItems_.begin(), tickItems_.end(), pItem );
    if (found != tickItems_.end())
    {
        tickItems_.erase( found );
    }
    else
    {
        BW::list<ChunkItemPtr>::iterator pendingFound =
            std::find( pendingTickItems_.begin(), pendingTickItems_.end(), pItem );
        if (pendingFound != pendingTickItems_.end())
        {
            pendingTickItems_.erase( pendingFound );
        }
    }
}


void ClientChunkSpace::delUpdateItem( ChunkItem* pItem )
{
    BW_GUARD_PROFILER( ClientChunkSpace_delUpdateItem );
    MF_ASSERT( pItem->wantsUpdate() );

    SimpleMutexHolder smh( updateMutex_ );

    BW::list<ChunkItem*>::iterator found =
        std::find( updateItems_.begin(), updateItems_.end(), pItem );
    if (found != updateItems_.end())
    {
        updateItems_.erase( found );
    }
    else
    {
        BW::list<ChunkItem*>::iterator pendingFound =
            std::find( pendingUpdateItems_.begin(), pendingUpdateItems_.end(), pItem );
        if (pendingFound != pendingUpdateItems_.end())
        {
            pendingUpdateItems_.erase( pendingFound );
        }
    }
}


/**
 *  This method adds a homeless chunk item
 */
void ClientChunkSpace::addHomelessItem( ChunkItemPtr pItem )
{
    BW_GUARD;
    SimpleMutexHolder smh( tickMutex_ );
    MF_ASSERT( pItem->chunk() == NULL );
    homeless_.push_back( pItem );
}


/**
 *  This method removes a homeless chunk item
 */
void ClientChunkSpace::delHomelessItem( ChunkItemPtr pItem )
{   
    BW_GUARD_PROFILER( ClientChunkSpace_delHomelessItem );
    SimpleMutexHolder smh( tickMutex_ );
    BW::vector<ChunkItemPtr>::iterator found =
        std::find( homeless_.begin(), homeless_.end(), pItem );
    if (found != homeless_.end()) homeless_.erase( found );
}


/**
 *  This method sets the colour of the ambient light from the heavens
 */
void ClientChunkSpace::ambientLight( Moo::Colour col )
{
    BW_GUARD;
    ambientLight_ = col;
    lights_->ambientColour( ambientLight_ );
}


/**
 *  This method gets the visible outside chunks
 *  @param viewProjection the projection matrix used to 
 *      determine which chunks are visible
 *  @param chunks the list that is populated with the visible chunks
 */
void ClientChunkSpace::getVisibleOutsideChunks( const Matrix& viewProjection, BW::vector<Chunk*>& chunks )
{
    BW_GUARD;
    // Set the traverse order for the quad tree
    ChunkQuadTree::setTraverseOrder( Moo::rc().invView()[2] );

    // Iterate over the quadtrees in this space and draw them
    ChunkQuadTreeMap::iterator it = chunkQuadTrees_.begin();

    while (it != chunkQuadTrees_.end())
    {
        it->second->calculateVisible( viewProjection, chunks );
        ++it;
    }
}


/**
 *  This method adds an outside chunk to the QuadTree
 *  @param pChunk the chunk to add
 */
void ClientChunkSpace::addOutsideChunkToQuadTree( Chunk* pChunk )
{
    BW_GUARD;
    if (pChunk->isOutsideChunk())
    {
        uint32 qtToken = ChunkQuadTree::token( pChunk->centre(), gridSize() );

        ChunkQuadTree* pQuadTree = NULL;
        ChunkQuadTreeMap::iterator qtIt = chunkQuadTrees_.find( qtToken );
        if (qtIt == chunkQuadTrees_.end())
        {
            pQuadTree = new ChunkQuadTree( qtToken, gridSize() );
            chunkQuadTrees_.insert( std::make_pair( qtToken, pQuadTree ) );
        }
        else
        {
            pQuadTree = qtIt->second;
        }

        pQuadTree->addChunk( pChunk );
    }
}


/**
 *  This method removed an outside chunk from the QuadTree
 *  @param pChunk the chunk to remove from the QuadTree
 */
void ClientChunkSpace::removeOutsideChunkFromQuadTree( Chunk* pChunk )
{
    BW_GUARD;
    if (pChunk->isOutsideChunk())
    {
        uint32 qtToken = ChunkQuadTree::token( pChunk->centre(), gridSize() );

        ChunkQuadTreeMap::iterator qtIt = chunkQuadTrees_.find( qtToken );
        if (qtIt != chunkQuadTrees_.end())
        {
            if (!qtIt->second->removeChunk( pChunk ))
            {
                bw_safe_delete(qtIt->second);
                chunkQuadTrees_.erase( qtIt );
            }
        }

        {
            SimpleMutexHolder smh( visibilityMutex_ );

            // If the chunk is currently in the visibility update list, remove it
            BW::vector<Chunk*>::iterator it =
                std::find( visibilityUpdates_.begin(), visibilityUpdates_.end(), pChunk );

            // In some instances the chunk can appear mutliple times in the
            // Visibility updates list, so remove all entries
            while (it != visibilityUpdates_.end())
            {
                visibilityUpdates_.erase( it );
                it = std::find( visibilityUpdates_.begin(),
                    visibilityUpdates_.end(),
                    pChunk );
            }
        }
    }
}


/**
 *  This method tells the space to update the quad tree bounding
 *  box of an outside chunk
 */
void ClientChunkSpace::updateOutsideChunkInQuadTree( Chunk* pChunk )
{
    BW_GUARD;
    // Make some checks first to see if we need to update this chunk
    if (pChunk->isOutsideChunk() &&
        pChunk->visibilityBoxMark() != Chunk::visibilityMark())
    {
        // Update the visibility mark and add the chunk to the
        // update list
        pChunk->visibilityBoxMark( Chunk::visibilityMark() );

        SimpleMutexHolder smh( visibilityMutex_ );
        visibilityUpdates_.push_back( pChunk );
    }
}

BW::SceneProvider* ClientChunkSpace::getChunkSceneProvider() 
{ 
    return sceneProvider_; 
}

void ClientChunkSpace::setChunkSceneProvider( BW::SceneProvider* sceneProvider ) 
{ 
    sceneProvider_ = sceneProvider; 
}


// ----------------------------------------------------------------------------
// Section: ClientChunkSpace::Column
// ----------------------------------------------------------------------------

/**
 *  Column constructor
 */
ClientChunkSpace::Column::Column( ClientChunkSpace * pSpace, int x, int z ) :
    BaseChunkSpace::Column( pSpace, x, z ),
    soft_( false )
{
}


/**
 *  Column destructor
 */
ClientChunkSpace::Column::~Column()
{
}

uint ClientChunkSpace::Column::nHoldings() const
{ 
    size_t nHoldings = holdings_.size() + heldObstacles_.size();
    MF_ASSERT( nHoldings <= UINT_MAX );
    return ( uint )nHoldings;
}


// ----------------------------------------------------------------------------
// Section: FocusGrid
// ----------------------------------------------------------------------------


/**
 *  FocusGrid constructor
 */
template <class T, int ISPAN>
FocusGrid<T,ISPAN>::FocusGrid() :
    cx_( 0 ),
    cz_( 0 )
{
    BW_GUARD;
    for (int z = 0; z < SPANX; z++) for (int x = 0; x < SPANX; x++)
    {
        grid_[z][x] = NULL;
    }
}


/**
 *  FocusGrid destructor
 */
template <class T, int ISPAN>
FocusGrid<T,ISPAN>::~FocusGrid()
{
    BW_GUARD;
    for (int z = 0; z < SPANX; z++) for (int x = 0; x < SPANX; x++)
        bw_safe_delete(grid_[z][x]);
}


/**
 *  This method sets the origin of the grid. The grid cell at
 *  the given location becomes the new centre of the grid.
 *  Cells are accessible for +/- SPAN/2, inclusive, around
 *  the cell at (x,y). (Since SPAN is odd, SPAN/2 always
 *  rounded down)
 */
template <class T, int ISPAN>
void FocusGrid<T,ISPAN>::origin( int cx, int cz )
{
    BW_GUARD;
    // note: could make this condition: (cz_>>1) < (z>>1), because there's
    // always one row unused, but I won't for now (stability, sanity)

    // move z to the right position
    while (cz_ < cz)
    {
        for (int x = 0; x < SPANX; x++) this->eraseEntry( x, cz_ - SPANH );
        cz_++;
    }
    while (cz_ > cz)
    {
        for (int x = 0; x < SPANX; x++) this->eraseEntry( x, cz_ + SPANH );
        cz_--;
    }

    // move x to the right position
    while (cx_ < cx)
    {
        for (int z = 0; z < SPANX; z++) this->eraseEntry( cx_ - SPANH, z );
        cx_++;
    }
    while (cx_ > cx)
    {
        for (int z = 0; z < SPANX; z++) this->eraseEntry( cx_ + SPANH, z );
        cx_--;
    }
}


/**
 *  This private method erases the indicated entry
 */
template <class T, int ISPAN>
void FocusGrid<T,ISPAN>::eraseEntry( int x, int z )
{
    BW_GUARD;
    T * & pT = this->operator()( x, z );
    bw_safe_delete(pT);
}

BW_END_NAMESPACE

// client_chunk_space.cpp
