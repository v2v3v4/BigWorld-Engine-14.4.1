
#include "pch.hpp"
#include "chunk_tree.hpp"
#include "chunk_obstacle.hpp"
#include "geometry_mapping.hpp"
#include "umbra_config.hpp"

#include "cstdmf/guard.hpp"

#include "resmgr/auto_config.hpp"

#include "moo/render_context.hpp"
#include "moo/shadow_manager.hpp"
#include "moo/renderer.hpp"

#include "physics2/bsp.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "romp/water_scene_renderer.hpp"

#if UMBRA_ENABLE
#include <ob/umbraObject.hpp>
#include "umbra_chunk_item.hpp"
#endif

#include "space/client_space.hpp"
#include "space/space_manager.hpp"
#include "scene/scene.hpp"
#include "scene/change_scene_view.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkTree
// -----------------------------------------------------------------------------

static AutoConfigString s_treeToModelMapping( "environment/treeToModelMapping" );


int ChunkTree_token;

/**
 *  Constructor.
 */
ChunkTree::ChunkTree() :
    BaseChunkTree(),
#if SPEEDTREE_SUPPORT
    tree_(NULL),
#endif
    reflectionVisible_(false),
    errorInfo_(NULL),
    castsShadow_( true )
{}


/**
 *  Destructor.
 */
ChunkTree::~ChunkTree()
{}


/**
 *  The overridden load method
 *  @param  pSection    pointer to the DataSection that contains the tree information
 *  @param  pChunk  the chunk contains the tree
 *  @param  errorString the string to return the loading error if there is any
 *  @return true if load successfully, otherwise false
 */
bool ChunkTree::load(DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString )
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;

    // we want transform and reflection to be set even
    // if the tree fails to load. They may be used to
    // render a place holder model in case load fails

    // rotation inverse will be 
    // updated automatically
    this->setTransform(pSection->readMatrix34("transform", Matrix::identity));
    this->reflectionVisible_ = pSection->readBool(
        "reflectionVisible", reflectionVisible_);

    uint seed = pSection->readInt("seed", 1);
    BW::string filename = pSection->readString("spt");

    bool result = this->loadTree(filename.c_str(), seed, pChunk);
    if ( !result && errorString )
    {
        *errorString = "Failed to load tree " + filename;
    }

    castsShadow_ = pSection->readBool( "castsShadow", castsShadow_ );
    castsShadow_ = pSection->readBool( "editorOnly/castsShadow", castsShadow_ );

#ifndef MF_SERVER
    sceneObject_.flags().isCastingStaticShadow( castsShadow_ );
    sceneObject_.flags().isCastingDynamicShadow( castsShadow_ );
#endif // MF_SERVER

    return result;
#else
    return false;
#endif
}

/**
 *  this method loads the speedtree
 *  @param  filename    the speedtree filename
 *  @param  seed    the seed of the speed tree
 *  @param  chunk   the chunk contains the speedtree
 *  @return true if load successfully, otherwise false
 */
bool ChunkTree::loadTree(const char * filename, int seed, Chunk * chunk)
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;
    try
    {
        // load the speedtree
        using speedtree::SpeedTreeRenderer;
        std::auto_ptr< SpeedTreeRenderer > speedTree(new SpeedTreeRenderer);

        Matrix world = chunk ? chunk->transform() : Matrix::identity;
        world.preMultiply( this->transform() );

        speedTree->load( filename, seed, world );       
        this->tree_ = speedTree;

        // update collision data
        this->BaseChunkTree::setBoundingBox(this->tree_->boundingBox());
        this->BaseChunkTree::setBSPTree( tree_->bsp() );
        this->errorInfo_.reset(NULL);
    }
    catch (const std::runtime_error &err)
    {
        this->errorInfo_.reset(new ErrorInfo);
        this->errorInfo_->what     = err.what();
        this->errorInfo_->filename = filename;
        this->errorInfo_->seed     = seed;

        ERROR_MSG( "Error loading tree: %s\n", err.what() );
    }
    return this->errorInfo_.get() == NULL;
#else
    return false;
#endif // SPEEDTREE_SUPPORT
}

/**
 * This function generates the list of textures used by the tree
 */
void ChunkTree::generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup )
{
#if SPEEDTREE_SUPPORT
    // Iterate over the tree's materials fetching textures
    if (this->tree_.get())
    {
        this->tree_->generateTextureUsage( usageGroup );
    }
#endif
}

/**
 *  The overridden addYBounds method
 */
bool ChunkTree::addYBounds(BoundingBox& bb) const
{   
    BW_GUARD;
    BoundingBox treeBB = this->boundingBox();
    treeBB.transformBy(this->transform());
    bb.addYBounds(treeBB.minBounds().y);
    bb.addYBounds(treeBB.maxBounds().y);

    return true;
}

/**
 *  The overridden syncInit method
 */
void ChunkTree::syncInit()
{
    BW_GUARD;   
#if UMBRA_ENABLE && SPEEDTREE_SUPPORT
    // Delete any old umbra draw item
    bw_safe_delete(pUmbraDrawItem_);
    bw_safe_delete(pUmbraShadowCasterTestItem_);

    Matrix m = this->pChunk_->transform();
    m.preMultiply( transform() );

    UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
    BoundingBox bb;
    localBB( bb );
    pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell() );

    // Do not use trees as occluders
    pUmbraChunkItem->pUmbraObject()->object()->set( Umbra::OB::Object::ENABLED, true );
    pUmbraChunkItem->pUmbraObject()->object()->set( Umbra::OB::Object::UNBOUNDED, false );
    pUmbraChunkItem->pUmbraObject()->object()->set( Umbra::OB::Object::OCCLUDER, false );
    
    pUmbraDrawItem_ = pUmbraChunkItem;
    this->updateUmbraLenders();

    {
        UmbraChunkItemShadowCaster* pUmbraChunkItem = new UmbraChunkItemShadowCaster;
        BoundingBox bb;
        localBB( bb );
        pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell() );
        pUmbraShadowCasterTestItem_ = pUmbraChunkItem;
    }
#endif
}


CollisionSceneProviderPtr ChunkTree::getCollisionSceneProvider( Chunk* ) const
{
    const BSPTree* tree = this->bspTree();

    if (tree)
    {
        Matrix transform;

        transform.multiply( this->transform(), chunk()->transform() );

        return new BSPTreeCollisionSceneProvider( tree, transform );
    }

    return NULL;
}


//bool use_water_culling = true;

/**
 *  The overridden draw method
 */
void ChunkTree::draw( Moo::DrawContext& drawContext )
{
#if SPEEDTREE_SUPPORT
    BW_GUARD_PROFILER( ChunkTree_draw );

    if (Moo::rc().dynamicShadowsScene() && !castsShadow_)
    {
        return;
    }

    if (Moo::rc().reflectionScene() && !reflectionVisible_)
    {
        return;
    }

#if ENABLE_BSP_MODEL_RENDERING
    if (this->drawBspInternal())
    {
        // If we drew a BSP, don't draw the object
        return;
    }
#endif // ENABLE_BSP_MODEL_RENDERING

    if (this->tree_.get() != NULL)
    {   
        if (Moo::rc().reflectionScene())
        {
            float height = WaterSceneRenderer::currentScene()->waterHeight();

            Matrix world( pChunk_->transform() );   
            BoundingBox bounds( this->boundingBox() );

            world.preMultiply(this->transform());
            bounds.transformBy( world );

            bool maxBelowPlane = bounds.maxBounds().y < height;
            if (maxBelowPlane)
                return;
        }

        Matrix world = Moo::rc().world();
        world.preMultiply(this->transform());
        this->tree_->draw( world );
    }
#endif
}


/**
 *  Add this model to (or remove it from) this chunk
 */
void ChunkTree::toss( Chunk * pChunk )
{
    BW_GUARD;
    BaseChunkTree::toss( pChunk );

#if SPEEDTREE_SUPPORT
    // add it to new chunk
    if (pChunk_ != NULL)
    {
        Matrix world( pChunk_->transform() );
        world.preMultiply( transform() );
    }
#endif
}


#if ENABLE_BSP_MODEL_RENDERING
/**
 *  Draw the tree's BSP.
 *  @return true if the BSP was drawn.
 */
bool ChunkTree::drawBspInternal()
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;

    const bool drawBsp =
        ChunkBspHolder::getDrawBsp() && ChunkBspHolder::getDrawBspSpeedTrees();

    // Load up the bsp tree if needed
    if (drawBsp && (tree_.get() != NULL) && !this->isBspCreated())
    {
        // no vertices loaded yet, create some
        const BSPTree * tree = this->bspTree();

        if (tree != NULL)
        {
            BW::vector< Moo::VertexXYZL > verts;
            Moo::Colour colour;
            this->getRandomBspColour( colour );
            Moo::BSPTreeHelper::createVertexList( *tree, verts, colour );

            // If BSP has collideable vertices
            if (!verts.empty())
            {
                this->addBsp( verts, this->filename() );
                MF_ASSERT( this->isBspCreated() );
            }
        }
    }

    // Draw the bsp
    if (drawBsp && (tree_.get() != NULL) && this->isBspCreated())
    {
        Matrix transform;
        transform.multiply( this->transform(), this->chunk()->transform() );
        this->batchBsp( transform );
        return true;
    }
#endif

    return false;
}
#endif // ENABLE_BSP_MODEL_RENDERING


/**
 *  The overridden lend method
 */
void ChunkTree::lend(Chunk * pLender)
{
    BW_GUARD;
    if (pChunk_ != NULL)
    {
        Matrix world(pChunk_->transform());
        world.preMultiply(this->transform());

        BoundingBox bb = this->boundingBox();
        bb.transformBy(world);

        this->lendByBoundingBox(pLender, bb);
    }
}

/**
 *  We support depth only
 */
uint32 ChunkTree::typeFlags() const
{
//  return speedtree::SpeedTreeRenderer::depthOnlyPass()
//          ? ChunkItemBase::TYPE_DEPTH_ONLY : 0;
    return 0;
}

/**
 *  This method sets the world transform of the tree
 *  @param  transform   the new transform
 */
void ChunkTree::setTransform(const Matrix & transform)
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;
    this->BaseChunkTree::setTransform(transform);
    if (this->tree_.get() != NULL)
    {
        Matrix world = pChunk_ ? pChunk_->transform() : Matrix::identity;
        world.preMultiply( this->transform() );
        this->tree_->resetTransform( world );

#if UMBRA_ENABLE
        if (pUmbraDrawItem_ != NULL)
            pUmbraDrawItem_->updateTransform(world);

        if (pUmbraShadowCasterTestItem_ != NULL)
            pUmbraShadowCasterTestItem_->updateTransform(world);
#endif

#if (!defined MF_SERVER) && (!defined _NAVGEN) && !defined(PROCESS_DEFS)
        ClientSpacePtr cs = SpaceManager::instance().space( pChunk_->space()->id() );
        if (cs)
        {
            ChangeSceneView* pView = cs->scene().getView<ChangeSceneView>();
            pView->notifyAreaChanged( this->boundingBox() );
        }
#endif // (!defined MF_SERVER) && (!defined _NAVGEN) && !defined(PROCESS_DEFS)

    }
#endif
}

/**
 *  This method returns the seed of the tree
 *  @return the seed of the tree
 */
uint ChunkTree::seed() const
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;
    MF_ASSERT_DEV(this->tree_.get() != NULL || this->errorInfo_.get() != NULL);
    return this->tree_.get() != NULL
        ? this->tree_->seed()
        : this->errorInfo_->seed;
#else
    return 0;
#endif
}

/**
 *  This method sets the seed of the tree
 *  @return true if successfully set, otherwise false
 */
bool ChunkTree::seed(uint seed)
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;
    bool result = false;
    if (this->tree_.get() != NULL)
    {
        result = this->loadTree(
            this->tree_->filename(), seed,
            this->ChunkItemBase::chunk());
    }
    return result;
#else
    return false;
#endif
}

/**
 *  This method returns the tree filename
 *  @return the tree filename
 */
const char * ChunkTree::filename() const
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;
    MF_ASSERT_DEV(this->tree_.get() != NULL || this->errorInfo_.get() != NULL);
    return this->tree_.get() != NULL
        ? this->tree_->filename()
        : this->errorInfo_->filename.c_str();
#else
    return "";
#endif
}

/**
 *  This method sets the tree filename
 *  @return true if successfully set, otherwise false
 */
bool ChunkTree::filename(const char * filename)
{
#if SPEEDTREE_SUPPORT
    BW_GUARD;
    MF_ASSERT_DEV(this->tree_.get() != NULL || this->errorInfo_.get() != NULL);
    int seed = this->tree_.get() != NULL 
        ? this->tree_->seed() 
        : this->errorInfo_->seed;

    return this->loadTree(filename, seed, this->ChunkItemBase::chunk());
#else
    return false;
#endif
}

/**
 *  This method returns the reflection visible flag
 *  @return true if the tree will be reflected by water, otherwise false
 */
bool ChunkTree::getReflectionVis() const
{
    return this->reflectionVisible_;
}

/**
 *  This method sets the reflection visible flag
 *  @return true if set successfully, otherwise false
 */
bool ChunkTree::setReflectionVis(const bool & reflVis) 
{ 
    this->reflectionVisible_= reflVis;
    return true;
}

/**
 *  This method returns if the load is failed
 *  @return true if load is FAILED, otherwise false
 */
bool ChunkTree::loadFailed() const
{
    return this->errorInfo_.get() != NULL;
}

/**
 *  This method returns the last error
 *  @return a C string describes the last error
 */
const char * ChunkTree::lastError() const
{
    return this->errorInfo_.get() != NULL
        ? this->errorInfo_->what.c_str()
        : "";
}


/**
 *  This method returns whether or not this tree should cast a shadow.
 *
 *  @return     Returns whether or not this tree should cast a shadow
 */
bool ChunkTree::affectShadow() const
{
    return castsShadow_;
}


/// Static factory initialiser
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)

#if SPEEDTREE_SUPPORT

IMPLEMENT_CHUNK_ITEM(ChunkTree, speedtree, 0)

#else

BW_END_NAMESPACE

#include "chunk_model.hpp"

BW_BEGIN_NAMESPACE

ChunkItemFactory ChunkTree::factory_( "speedtree", 0, ChunkTree::create );

ChunkItemFactory::Result ChunkTree::create( Chunk * pChunk, DataSectionPtr pSection )                                       \
{
    static BW::map< BW::string, BW::string > treeToModel;
    static BW::string defaultModel;
    static bool doneInit;
    static bool mapValid;

    // Try to load the mappings
    if ( !doneInit )
    {
        doneInit = true;

        DataSectionPtr mapSection = BWResource::openSection( s_treeToModelMapping );

        if ( mapSection )
        {
            defaultModel = mapSection->readString( "default" );

            for ( DataSectionIterator it = mapSection->begin(); it != mapSection->end(); it++ )
            {
                if ( (*it)->sectionName() == "mapping" )
                {
                    BW::string tree = (*it)->readString( "spt" );
                    BW::string model = (*it)->readString( "model" );
                    treeToModel[ tree ] = model;
                }
            }

            mapValid = true;
        }
    }

    // If there is no map we just don't create any items
    BW::string errorString;
    if ( !mapValid )
    {
        return ChunkItemFactory::Result( NULL, errorString );
    }

    // Load speedtree section and map it to a model resource
    Matrix transform = pSection->readMatrix34("transform", Matrix::identity);
    bool reflectionVisible = pSection->readBool( "reflectionVisible", false );
    BW::string treeName = pSection->readString("spt");
    BW::string modelName =
        treeToModel.count( treeName ) ? treeToModel[ treeName ] : defaultModel;

    // Exit if nothing to map to
    if ( modelName.empty() )
    {
        return ChunkItemFactory::Result( NULL, errorString );
    }

    // Create a model section from the tree section and mapping
    DataSectionPtr modelSection = BWResource::openSection( "model.chunk", true );
    modelSection->writeMatrix34( "transform", transform );
    modelSection->writeBool( "reflectionVisible", reflectionVisible );
    modelSection->writeString( "resource", modelName );

    // Create and load the chunk item from the section
    ChunkModel * pItem = new ChunkModel();

    if ( pItem->load( modelSection, pChunk ) )
    {
        if (!pChunk->addStaticItem( pItem ))
        {
            ASSET_MSG( "ChunkModel::create: "
                    "error in section %s of %s in mapping %s\n",
                pSection->sectionName().c_str(),
                pChunk->identifier().c_str(),
                pChunk->mapping()->path().c_str() );
        }
        return ChunkItemFactory::Result( pItem );
    }

    bw_safe_delete(pItem);
    return ChunkItemFactory::Result( NULL, errorString );
}

#endif

BW_END_NAMESPACE

// chunk_tree.cpp
