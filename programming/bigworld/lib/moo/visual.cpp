#include "pch.hpp"

#include "cstdmf/bw_unordered_set.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/primitive_file.hpp"

#include "render_context.hpp"
#include "directional_light.hpp"
#include "omni_light.hpp"
#include "spot_light.hpp"
#include "texture_manager.hpp"
#include "texture_streaming_manager.hpp"
#include "primitive_manager.hpp"
#include "primitive_helper.hpp"
#include "vertices_manager.hpp"
#include "animation_manager.hpp"
#include "math/blend_transform.hpp"
#include "vertex_formats.hpp"
#include "vertex_format_cache.hpp"
#include "effect_material.hpp"
#include "visual.hpp"
#include "effect_constant_value.hpp"
#include "effect_visual_context.hpp"
#include "visual_manager.hpp"
#include "visual_common.hpp"
#include "fog_helper.hpp" 
#include "vertex_streams.hpp"
#include "resource_load_context.hpp"
#include "texture_usage.hpp"
#include "draw_context.hpp"
#include "moo_utilities.hpp"

#include "physics2/worldtri.hpp"
#include "physics2/bsp.hpp"

#include "shadow_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "visual.ipp"
#endif

namespace Moo
{

// File statics
namespace 
{
    typedef StringHashMap< BSPProxyPtr > BSPMap;

    BSPMap      s_premadeBSPs;
    SimpleMutex s_premadeBSPsLock;

    // Create an object that registers a watcher to read the size of the
    // premade BSP cache.
    class WatchOwner
    {
    public:
        uint32 premadeBSPCount() const { return (uint32)s_premadeBSPs.size(); }

        WatchOwner()
        {
            BW_GUARD;
            MF_WATCH("Render/Visual/premadeBSP_Count", *this,
                &WatchOwner::premadeBSPCount, "The size of the premade BSP Cache");
        }
    };

    WatchOwner wo;
}

/**
 *  BSPProxy constructor.
 */
BSPProxy::BSPProxy( BSPTree * pTree ) :
    pTree_( pTree )
{
    BW_GUARD;
}

/**
 *  BSPProxy destructor.
 */
BSPProxy::~BSPProxy()
{
    BW_GUARD;
    bw_safe_delete(pTree_);
}

// -----------------------------------------------------------------------------
// Section: Methods used by VisualLoader
// The following methods were created because they are needed by VisualLoader.
// Most of them are inline here because VisualLoader is only instantiated here.
// -----------------------------------------------------------------------------

static BSPProxyPtr NULL_BSPPROXYPTR( NULL ); 

// Finds the material with the specified identifier.
inline const ComplexEffectMaterialPtr Visual::Materials::find(const BW::string& identifier ) const
{
    BW_GUARD;
    for ( const_iterator ppMat = this->begin(); ppMat != this->end(); ++ppMat )
    {
        if (identifier == (*ppMat)->identifier())
            return *ppMat;
    }

    return NULL;
}

// Adds a BSP into our BSP cache.
// Needed by VisualLoader
inline void Visual::BSPCache::add( const BW::string& resourceID, BSPProxyPtr& pBSP )
{
    BW_GUARD;
    s_premadeBSPsLock.grab();
    s_premadeBSPs.insert(std::make_pair( resourceID, pBSP ));
    s_premadeBSPsLock.give();
}

// Finds a BSP into our BSP cache.
// Needed by VisualLoader
inline BSPProxyPtr& Visual::BSPCache::find( const BW::string& resourceID )
{
    BW_GUARD;
    s_premadeBSPsLock.grab();
    BSPMap::iterator bspit = s_premadeBSPs.find( resourceID );
    BSPProxyPtr& pFoundBSP = (bspit != s_premadeBSPs.end()) ?
        bspit->second : NULL_BSPPROXYPTR;
    s_premadeBSPsLock.give();
    return pFoundBSP;
}

// delete a BSP from our BSP cache.
// Needed when doing visual reload
inline void Visual::BSPCache::del( const BW::string& resourceID )
{
    BW_GUARD;
    s_premadeBSPsLock.grab();
    BSPMap::iterator bspit = s_premadeBSPs.find( resourceID );
    if (bspit != s_premadeBSPs.end())
    {
        s_premadeBSPs.erase( bspit );
    }
    s_premadeBSPsLock.give();
}

// Returns singleton instance of BSPCache. Needed by VisualLoader.
inline Visual::BSPCache& Visual::BSPCache::instance()
{
    BW_GUARD;
    REGISTER_SINGLETON( Visual::BSPCache )
    SINGLETON_MANAGER_WRAPPER( Visual::BSPCache )
    static BSPCache bspCache;
    return bspCache;
}

// Loads BSP from  _bsp.visual file.
inline BSPProxyPtr Visual::loadBSPVisual( const BW::string& resourceID )
{
    BW_GUARD;
    VisualPtr bspVisual = VisualManager::instance()->get( resourceID );
    return (bspVisual && bspVisual->pBSP_) ? bspVisual->pBSP_ : NULL_BSPPROXYPTR;
}

Visual::ReadGuard::ReadGuard( Visual* visual )
    : visual( visual )
{
#if ENABLE_RELOAD_MODEL
    visual->beginRead();
#endif
}

Visual::ReadGuard::~ReadGuard()
{
#if ENABLE_RELOAD_MODEL
    visual->endRead();
#endif
}


/**
 *  This function is a concurrency lock between
 *  me reloading myself and our client pulling info from
 *  myself, client begin to read.
*/
void Visual::beginRead()
{
    BW_GUARD;
#if ENABLE_RELOAD_MODEL
    reloadRWLock_.beginRead();
#endif//ENABLE_RELOAD_MODEL
}



/**
 *  This function is a concurrency lock between
 *  me reloading myself and our client pulling info from
 *  myself, client end read.
*/
void Visual::endRead()
{
    BW_GUARD;
#if ENABLE_RELOAD_MODEL
    reloadRWLock_.endRead();
#endif//ENABLE_RELOAD_MODEL
}


#if ENABLE_RELOAD_MODEL
/**
 *  This function reload the current visual from the disk.
 *  This function isn't thread safe. It will only be called
 *  For reload an externally-modified visual
 *  @param  doValidateCheck do we check if the file is valid first?
 *  @return if succeed
*/
bool Visual::reload( bool doValidateCheck )
{
    BW_GUARD;

    if (doValidateCheck)
    {
        // delete the bsp from the cache.
        BSPCache::instance().del( resourceID_ );
        BSPCache::instance().del( resourceID_ + ".processed" );

        if (!Visual::validateFile( resourceID_ ))
        {
            return false;
        }
    }
    // delete the bsp from the cache, we delete twice here because 
    // Visual::validateFile can cache the bsp model, and we want the bsp reload.
    BSPCache::instance().del( resourceID_ );
    BSPCache::instance().del( resourceID_ + ".processed" );

    this->onPreReload();

    reloadRWLock_.beginWrite();
    this->load();
    reloadRWLock_.endWrite();

    this->onReloaded();
    return true;
}


/**
 *  This function check if the load file is valid
 *  We are currently doing a fully load and check,
 *  but we might want to optimize it in the future 
 *  if it's needed.
 *  @param resourceID   the visual file path
 */
bool Visual::validateFile( const BW::string& resourceID )
{
    BW_GUARD;
    DataSectionPtr pFile = BWResource::openSection( resourceID );
    if (!pFile)
    {
        ASSET_MSG( "Visual::reload: "
            "%s is not a valid visual file\n",
            resourceID.c_str() );
        return false;
    }

    Visual tempVisual(resourceID);
    tempVisual.isInVisualManager( false );
    if (!tempVisual.isOK())
    {
        return false;
    }
    return true;
}
#endif//ENABLE_RELOAD_MODEL


namespace
{
    /**
     *  Adds identifier strings for each section in @a dataSections to
     *  @a identifiers.
     *  @return True if any identifiers couldn't be added into @a identifiers
     *      because they already existed (i.e., true if we encounter
     *      non-unique identifiers) or no identifier was found. If true is
     *      returned, a warning was printed for each issue.
     */
    bool addIKConstraintIdentifiers(
        BW::unordered_set<BW::string> & identifiers,
        const BW::vector<DataSectionPtr> & dataSections )
    {
        BW_GUARD;
        
        bool result = false;
        const char IDENTIFIER_NAME[] = "identifier";
        const char DEFAULT_IDENTIFIER[] = "Identifier not found. ";

        BW::string identifier;

        for (BW::vector<DataSectionPtr>::const_iterator itr =
                dataSections.begin();
            itr != dataSections.end();
            ++itr)
        {
            identifier =
                (* itr)->readString( IDENTIFIER_NAME, DEFAULT_IDENTIFIER );

            if (identifier.compare( DEFAULT_IDENTIFIER ) == 0)
            {
                result = true;
                WARNING_MSG( "An IK/constraint data section was encountered "
                    "with no '%s'.\n", IDENTIFIER_NAME );
                continue;
            }

            if ( identifiers.find( identifier ) != identifiers.end() )
            {
                result = true;
                WARNING_MSG( "A non-unique IK/constraint data section "
                    "%s was encountered ('%s').\n",
                    IDENTIFIER_NAME, identifier.c_str() );
                continue;
            }

            identifiers.insert( identifier );
        }

        return result;
    }


    /**
     *  Sets @a translationOffset and/or @a rotationOffset from the data
     *  in @a dataSection as appropriate for @a constraintType.
     */
    void getOffsets(
        Vector3 & translationOffset, Vector3 & rotationOffset,
        const DataSectionPtr & dataSection,
        ConstraintData::ConstraintType constraintType )
    {
        BW_GUARD;
        
        switch( constraintType )
        {
        case ConstraintData::CT_AIM:
        case ConstraintData::CT_ORIENT:
            rotationOffset =
                dataSection->readVector3( "offset", rotationOffset );
            break;
                
        case ConstraintData::CT_POINT:
        case ConstraintData::CT_POLE_VECTOR:
            translationOffset =
                dataSection->readVector3( "offset", translationOffset );
            break;

        case ConstraintData::CT_PARENT:
            translationOffset = dataSection->readVector3(
                "translateOffset", translationOffset );
            rotationOffset = dataSection->readVector3(
                "rotateOffset", rotationOffset );
            break;

        default:
            MF_ASSERT( false && "Unhandled ConstraintType. " );
            break;
        }
    }


    /**
     *  @param constraints Constraints loaded will be appended to this.
     *  @param constraintSections Sections from the input containing details
     *      for constraints to append to @a constraints.
     *  @param constraintType The type of constraints described in
     *      @a constraintSections.
     *  @param constraintIdentifiers/ikHandleIdentifiers Gives the identifiers
     *      for all constraints and IK handles in the system.
     *  @pre All constraint identifiers in @a constraintSections are
     *      in @a constraintIdentifiers.
     *  @post Appended elements to @a constraints for elements in
     *      @a constraintSections (perhaps multiple per item). Used default
     *      values if elements of @a constraintSections have missing data.
     */
    void loadConstraintData(
        BW::vector<ConstraintData> & constraints,
        const BW::vector<DataSectionPtr> & constraintSections,
        ConstraintData::ConstraintType constraintType,
        const BW::unordered_set<BW::string> & constraintIdentifiers,
        const BW::unordered_set<BW::string> & ikHandleIdentifiers )
    {
        BW_GUARD;
        
        for (BW::vector<DataSectionPtr>::const_iterator cItr =
                constraintSections.begin();
            cItr != constraintSections.end();
            ++cItr)
        {
            //Id
            const BW::string identifier = (*cItr)->readString( "identifier" );
            MF_ASSERT( constraintIdentifiers.find( identifier ) !=
                constraintIdentifiers.end() );

            //Target
            const BW::string targetIdentifier = (*cItr)->readString( "target" );
            ConstraintData::TargetType targetType =
                ConstraintData::TT_MODEL_NODE;
            MF_ASSERT( (constraintIdentifiers.find( targetIdentifier ) ==
                    constraintIdentifiers.end()) &&
                "Constraint constraint target unsupported. " );
            if ( ikHandleIdentifiers.find( targetIdentifier ) !=
                ikHandleIdentifiers.end() )
            {
                targetType = ConstraintData::TT_IK_HANDLE;
            }

            //Offsets
            Vector3 translationOffset( Vector3::ZERO );
            Vector3 rotationOffset( Vector3::ZERO );
            getOffsets( translationOffset, rotationOffset,
                *cItr, constraintType );

            //Aim vectors.
            Vector3 aimVector;
            Vector3 upVector;
            Vector3 worldUpVector;
            if (constraintType == ConstraintData::CT_AIM)
            {
                aimVector = (*cItr)->readVector3(
                    "aimVector", Vector3::K );
                upVector = (*cItr)->readVector3(
                    "upVector", Vector3::J );
                worldUpVector = (*cItr)->readVector3(
                    "worldUpVector", Vector3::J );
            }

            //Sources.
            BW::vector<DataSectionPtr> sourceSections;
            (*cItr)->openSections( "source", sourceSections );
            BW::string thisSrcIdentifier = identifier;
            if (sourceSections.size() > 1)
            {
                MF_ASSERT( constraintIdentifiers.find( identifier ) !=
                    constraintIdentifiers.end() );

                //If there are multiple sources, we need to choose a different
                //identifier for each constraint (since the runtime constraints
                //map one source to one target).
                //The issue with this is that if the original constraint was
                //an input to another constraint, it will now not be found.
                //This can only be resolved by supporting a type of constraint
                //that's actually a group of other constraints.
                thisSrcIdentifier += "_ExtraSrc";
            }
            for (BW::vector<DataSectionPtr>::const_iterator sItr =
                    sourceSections.begin();
                sItr != sourceSections.end();
                ++sItr)
            {
                //Offset could be defined for the source.
                getOffsets( translationOffset, rotationOffset,
                    *sItr, constraintType );

                //Weight
                const float weight = (*sItr)->readFloat( "weight" );

                //Source
                const BW::string sourceIdentifier =
                    (*sItr)->readString( "identifier" );
                ConstraintData::SourceType sourceType =
                    ConstraintData::ST_MODEL_NODE;
                MF_ASSERT( (ikHandleIdentifiers.find( sourceIdentifier ) ==
                        ikHandleIdentifiers.end()) &&
                    "IK handle constraint source unsupported. " );
                if ( constraintIdentifiers.find( sourceIdentifier ) !=
                    constraintIdentifiers.end() )
                {
                    sourceType = ConstraintData::ST_CONSTRAINT;
                }

                if (sourceSections.size() > 1)
                {
                    //Choose identifier that's not in use for this constraint
                    //tied to the next source.
                    do 
                    {
                        thisSrcIdentifier += "_";
                    } while( constraintIdentifiers.find( thisSrcIdentifier ) !=
                        constraintIdentifiers.end() );
                }

                constraints.push_back( ConstraintData(
                    constraintType, thisSrcIdentifier,
                    targetType, targetIdentifier,
                    sourceType, sourceIdentifier,
                    weight, rotationOffset, translationOffset,
                    aimVector, upVector, worldUpVector ) );
            }
        }
    }


    /**
     *  @param ikHandles IK handles loaded will be appended to this.
     *  @param ikSections Sections from the input containing details
     *      for IK handles to append to @a ikHandles.
     *  @pre True.
     *  @post Appended one element to @a ikHandles for each element in
     *      @a ikSections. Used default values if elements of @a ikSections
     *      have missing data.
     */
    void loadIKHandleData(
        BW::vector<IKHandleData> & ikHandles,
        const BW::vector<DataSectionPtr> & ikSections )
    {
        BW_GUARD;
        
        for (BW::vector<DataSectionPtr>::const_iterator itr =
                ikSections.begin();
            itr != ikSections.end();
            ++itr)
        {
            const BW::string identifier = (*itr)->readString( "identifier" );
            const BW::string startJoint = (*itr)->readString( "startJoint" );
            const BW::string endJoint = (*itr)->readString( "endJoint" );
            const Vector3 poleVector =
                (*itr)->readVector3( "poleVector/vector" );
            const Vector3 targetPos =
                (*itr)->readVector3( "source/vector", Vector3::ZERO );

            ikHandles.push_back( IKHandleData( identifier,
                startJoint, endJoint, poleVector, targetPos ) );
        }
    }
}

/**
*   This function load the current visual from the disk.
*   @param loadingFlags Flags for partial loading (geometry-only for flora for example).
*/
void Visual::load( uint32 loadingFlags /*= LOAD_ALL*/ )
{
    BW_GUARD;

    this->release();

    BW::string baseName =
        resourceID_.substr( 0, resourceID_.find_last_of( '.' ) );
    VisualLoader< Visual > loader( baseName );

    // open the resource
    DataSectionPtr root = loader.getRootDataSection();
    if (!root)
    {
        ASSET_MSG( "Couldn't open visual %s\n", resourceID_.c_str() );
        isOK_ = false;
        return;
    }

    Moo::ScopedResourceLoadContext resLoadCtx( BWResource::getFilename( resourceID_ ) );

    // Load BSP Tree
    BSPMaterialIDs  bspMaterialIDs;
    BSPMaterialIDs  missingBspMaterialIDs;
    BSPTreePtr      pFoundBSP;
    bool bspRequiresMapping = false;

    if (loadingFlags == LOAD_ALL)
    {
        bspRequiresMapping = loader.loadBSPTree( pFoundBSP, bspMaterialIDs );
    }

    // Load our primitives
    const BW::string primitivesFileName = loader.getPrimitivesResID() + '/';

    // load the hierarchy
    DataSectionPtr nodeSection = root->openSection( "node" );
    if (nodeSection)
    {
        rootNode_ = new Node;
        rootNode_->loadRecursive( nodeSection );
    }
    else
    {
        // If there are no nodes in the resource create one.
        rootNode_ = new Node;
        rootNode_->identifier( "root" );
        NodeCatalogue::instance().findOrAdd( rootNode_ );
    }

    // open the renderesets
    BW::vector< DataSectionPtr > renderSets;
    root->openSections( "renderSet", renderSets );

    if (isOK_ && (renderSets.size() == 0))
    {
        ASSET_MSG( "Moo::Visual: No rendersets in %s\n", resourceID_.c_str() );
        isOK_ = false;
    }

    const float maxFloat = std::numeric_limits<float>::max();
    const Vector3 maxVector( maxFloat, maxFloat, maxFloat);

    // iterate through the rendersets and create them
    BW::vector< DataSectionPtr >::iterator rsit = renderSets.begin();
    BW::vector< DataSectionPtr >::iterator rsend = renderSets.end();
    while (rsit != rsend)
    {
        DataSectionPtr renderSetSection = *rsit;
        ++rsit;
        RenderSet renderSet;

        // Read worldspace flag
        renderSet.treatAsWorldSpaceObject_ = renderSetSection->readBool( "treatAsWorldSpaceObject" );
        BW::vector< BW::string > nodes;

        // Read the node identifiers of all nodes that affect this renderset.
        renderSetSection->readStrings( "node", nodes );

        // Are there any nodes?
        if (nodes.size())
        {
            // Iterate through the node identifiers.
            BW::vector< BW::string >::iterator it = nodes.begin();
            BW::vector< BW::string >::iterator end = nodes.end();
            while (it != end)
            {
                // Find the current node.
                NodePtr node = rootNode_->find( *it );
                if (isOK_ && (!node))
                {
                    ASSET_MSG( "Couldn't find node %s in %s\n", (*it).c_str(), resourceID_.c_str() );
                    isOK_ = false;
                }

                // Add the node to the rendersets list of nodes
                if (node)
                {
                    renderSet.transformNodes_.push_back( node );                
                }

                ++it;
            }
        }

        if (renderSet.transformNodes_.empty())
        {
            // The renderset is not affected by any nodes, so we will force it to be affected by the rootNode.
            renderSet.transformNodes_.push_back( rootNode_ );           
        }

        // Calculate the first node's static world transform
        //  (for populateWorldTriangles)
        NodePtr pMainNode = renderSet.transformNodes_.front();
        renderSet.firstNodeStaticWorldTransform_ = pMainNode->transform();

        while (pMainNode != rootNode_)
        {
            pMainNode = pMainNode->parent();
            renderSet.firstNodeStaticWorldTransform_.postMultiply(
                pMainNode->transform() );
        }

        // Open the geometry sections
        BW::vector< DataSectionPtr > geometries;
        renderSetSection->openSections( "geometry", geometries );

        if (isOK_ && (geometries.size() == 0))
        {
            ASSET_MSG( "No geometry in renderset in %s\n", resourceID_.c_str() );
            isOK_ = false;
        }       

        BW::vector< DataSectionPtr >::iterator geit = geometries.begin();
        BW::vector< DataSectionPtr >::iterator geend = geometries.end();

        // iterate through the geometry sections
        while (geit != geend)
        {
            DataSectionPtr geometrySection = *geit;
            ++geit;
            Geometry geometry;

            // Get a reference to the vertices used by this geometry
            BW::string verticesName = geometrySection->readString( "vertices" );
            if (verticesName.find_first_of( '/' ) == BW::string::npos)
                verticesName = primitivesFileName + verticesName;

            //NOTE: this streamName is not used yet, it's holding it's spot for when 
            // it could be used in any refactoring of our vertex data.
            //BW::string streamName = geometrySection->readString( "stream", "" );
            //if (streamName.find_first_of( '/' ) >= streamName.size())
            //  streamName = primitivesFileName + streamName;

            int numNodesValidate = 0;
            // only validate number of nodes if we have to.
            if ( validateNodes_ )
                numNodesValidate = (int)renderSet.transformNodes_.size();
            geometry.vertices_ = VerticesManager::instance()->get( verticesName, numNodesValidate );

            if (isOK_ && (geometry.vertices_->nVertices() == 0))
            {
                ASSET_MSG( "No vertex information in \"%s\".\nUnable to load \"%s\".\n",
                    verticesName.c_str(), resourceID_.c_str() );
                isOK_ = false;
            }

            // Get a reference to the indices used by this geometry
            BW::string indicesName = geometrySection->readString( "primitive" );
            if (indicesName.find_first_of( '/' ) == BW::string::npos)
                indicesName = primitivesFileName + indicesName;
            geometry.primitives_ = PrimitiveManager::instance()->get( indicesName );
            
            if (isOK_ && (geometry.primitives_->nIndices() == 0))
            {
                ASSET_MSG( "No indices information in \"%s\".\nUnable to load \"%s\".\n",
                    indicesName.c_str(), resourceID_.c_str() );
                isOK_ = false;
            }

            // Open the primitive group descriptions
            BW::vector< DataSectionPtr > primitiveGroups;
            geometrySection->openSections( "primitiveGroup", primitiveGroups );

            BW::vector< DataSectionPtr >::iterator prit = primitiveGroups.begin();
            BW::vector< DataSectionPtr >::iterator prend = primitiveGroups.end();

            bool validProcessedOrigins = true;
            LookUpTable<Vector3> primitiveGroupOrigins;
            primitiveGroupOrigins.storage().reserve( primitiveGroups.size() );
            bool bumped=false;
            // Iterate through the primitive group descriptions
            while (prit != prend)
            {
                DataSectionPtr primitiveGroupSection = *prit;
                ++prit;
                // Read the primitve group data.
                PrimitiveGroup primitiveGroup;
                primitiveGroup.groupIndex_ = primitiveGroupSection->asInt();

                Vector3 processedGroupOrigin = 
                    primitiveGroupSection->readVector3( "groupOrigin", maxVector );
                primitiveGroupOrigins[primitiveGroup.groupIndex_] = 
                    processedGroupOrigin;
                if (processedGroupOrigin == maxVector)
                {
                    validProcessedOrigins = false;
                }

                if (loadingFlags == LOAD_ALL)
                {
                    ComplexEffectMaterialPtr complexMat = new ComplexEffectMaterial();
                    complexMat->load(primitiveGroupSection->openSection("material"));

                    if (complexMat->baseMaterial()->pEffect() &&
                        !complexMat->skinned() && renderSet.treatAsWorldSpaceObject_)
                    {
                        // report an error and disable this material
                        ASSET_MSG( 
                            "Using non skinned material on skinned vertices: %s, effect:%s\n",
                            geometry.vertices_->resourceID().c_str(),
                            complexMat->baseMaterial()->pEffect()->resourceID().c_str() );

                        complexMat = new ComplexEffectMaterial();
                    }

                    ownMaterials_.push_back(complexMat);
                    primitiveGroup.material_ = complexMat;
                }
                geometry.primitiveGroups_.push_back( primitiveGroup );
            }

            if (isOK_ && (loadingFlags == LOAD_ALL) && bumped && !geometry.vertices_->hasBumpedFormat())
            {
                //warning: using bumped shader on non-bumped data.
                WARNING_MSG( "Geometry using a bumped material without having bumped information generated,"
                    " re-save the model in ModelEditor: \"%s\"\n", resourceID_.c_str() );
            }

            // If we have all the origins, the push the data over to the
            // primitives
            if (validProcessedOrigins)
            {
                validProcessedOrigins = geometry.primitives_->adoptGroupOrigins( 
                    primitiveGroupOrigins.storage() );
            }
            
            // If origins didn't load correctly, fallback to calculating them
            if (!validProcessedOrigins)
            {
                geometry.primitives_->calcGroupOrigins( geometry.vertices_ );
            }
            
            renderSet.geometry_.push_back( geometry );
        }

        renderSets_.push_back( renderSet );
    }

    // Load the bounding box
    loader.setBoundingBox( bb_ );

    // Load the uvSpaceDensity if it exists
    uvSpaceDensity_ = root->readFloat( "uvSpaceDensity", ModelTextureUsage::INVALID_DENSITY );

    // Add the BSP's bounds if one was loaded up separately
    if ((loadingFlags == LOAD_ALL) && pFoundBSP && !bspRequiresMapping )
    {
        bb_.addBounds( pFoundBSP->pTree()->boundingBox() );
    }

    // Load constraints and IK handles.
    DataSectionPtr constraintsSection = root->openSection( "constraints" );
    if (constraintsSection)
    {
        //Load constraints.
        BW::vector<DataSectionPtr> orientSections;
        constraintsSection->openSections( "orientConstraint", orientSections );

        BW::vector<DataSectionPtr> pointSections;
        constraintsSection->openSections( "pointConstraint", pointSections );

        BW::vector<DataSectionPtr> parentSections;
        constraintsSection->openSections( "parentConstraint", parentSections );

        BW::vector<DataSectionPtr> aimSections;
        constraintsSection->openSections( "aimConstraint", aimSections );

        BW::vector<DataSectionPtr> poleVectorSections;
        constraintsSection->openSections(
            "poleVectorConstraint", poleVectorSections );

        const std::size_t numConstraintDescriptions = static_cast<int64>(
            orientSections.size() + pointSections.size() +
            parentSections.size() + aimSections.size() +
            poleVectorSections.size());


        //Load IK handles.
        BW::vector<DataSectionPtr> ikSections;
        constraintsSection->openSections( "ikHandle", ikSections );

        const std::size_t numIKHandleDescriptions =
            static_cast<int64>( ikSections.size() );


        //We need to get all of the constraint names and all of the IK handle
        //names. This is so we can correctly configure the constraint source
        //and target types. We need to be able to check whether they're
        //referring to an IK handle or a constraint, and if not we assume
        //they're referring to a model node.
        BW::unordered_set<BW::string> constraintNames(
            numConstraintDescriptions );
        if (addIKConstraintIdentifiers( constraintNames, orientSections ) ||
            addIKConstraintIdentifiers( constraintNames, pointSections ) ||
            addIKConstraintIdentifiers( constraintNames, parentSections ) ||
            addIKConstraintIdentifiers( constraintNames, aimSections ) ||
            addIKConstraintIdentifiers( constraintNames, poleVectorSections ))
        {
            //Data error, warnings printed above.
            CRITICAL_MSG(
                "Visual contains non-unique constraint identifiers.\n" );
        }

        BW::unordered_set<BW::string> ikHandleNames(
            numIKHandleDescriptions );
        if (addIKConstraintIdentifiers( ikHandleNames, ikSections ))
        {
            //Data error, warnings printed above.
            CRITICAL_MSG(
                "Visual contains non-unique IK handle identifiers.\n" );
        }
        

        //Create ConstraintData objects.
        constraints_.clear();
        constraints_.reserve( numConstraintDescriptions );
        
        loadConstraintData(
            constraints_, orientSections, ConstraintData::CT_ORIENT,
            constraintNames, ikHandleNames );
        loadConstraintData(
            constraints_, pointSections, ConstraintData::CT_POINT,
            constraintNames, ikHandleNames  );
        loadConstraintData(
            constraints_, parentSections, ConstraintData::CT_PARENT,
            constraintNames, ikHandleNames  );
        loadConstraintData(
            constraints_, aimSections, ConstraintData::CT_AIM,
            constraintNames, ikHandleNames  );
        loadConstraintData(
            constraints_, poleVectorSections, ConstraintData::CT_POLE_VECTOR,
            constraintNames, ikHandleNames  );
        MF_ASSERT( constraints_.size() >= numConstraintDescriptions );


        //Create IKHandleData objects.
        ikHandles_.clear();
        ikHandles_.reserve( numIKHandleDescriptions );

        loadIKHandleData( ikHandles_, ikSections );
        MF_ASSERT( ikHandles_.size() == numIKHandleDescriptions );
    }

    // Load portal info
    BW::vector< DataSectionPtr > portals;

    root->openSections( "portal", portals );

    BW::vector< DataSectionPtr >::iterator it = portals.begin();
    BW::vector< DataSectionPtr >::iterator end = portals.end();

    while (it != end)
    {
        BW::vector< Vector3 > portalPoints;
        (*it)->readVector3s( "point", portalPoints );
        if ( portalPoints.size() )
        {
            PortalData pd;

            int pdflags = 0;    // ordered as in chunk_boundary.hpp
            pdflags |= int((*it)->asString() == "heaven") << 1;
            pdflags |= int((*it)->asString() == "earth") << 2;
            pdflags |= int((*it)->asString() == "invasive") << 3;
            pd.flags( pdflags );

            for( uint32 i = 0; i < portalPoints.size(); i++ )
            {
                pd.push_back( Vector3( portalPoints[ i ] ) );
            }

            portals_.push_back( pd );
        }
        it++;
    }

    if ((loadingFlags == LOAD_ALL) && pFoundBSP)
    {
        pBSP_ = pFoundBSP;

        if (bspRequiresMapping)
        {
            loader.remapBSPMaterialFlags( *pBSP_->pTree(), ownMaterials_, bspMaterialIDs, missingBspMaterialIDs );
        }
    }

#ifdef EDITOR_ENABLED
    if ( missingBspMaterialIDs.size() > 0 )
    {
        // add materials to the first render set geometry since we don't really know where it belongs
        BSPMaterialIDs::iterator it = missingBspMaterialIDs.begin();
        BSPMaterialIDs::iterator end = missingBspMaterialIDs.end();
        for ( ; it != end; ++it )
        {
            // write to the visual the missing BSP identifiers
            DataSectionPtr missingBspMaterial = root->newSection( "missingBspMaterial" );
            missingBspMaterial = missingBspMaterial->newSection( "material" );
            missingBspMaterial->writeString( "identifier", *it );

            // create new a material, the material will be added to the visual in the ME
            DataSectionPtr bspMaterial = new XMLSection( "renderSet" );
            bspMaterial = bspMaterial->newSection( "geometry" ); 
            bspMaterial = bspMaterial->newSection( "primitiveGroup" );
            DataSectionPtr material = bspMaterial->newSection( "material" );
            material->writeString( "identifier", *it );
            material->writeInt( "collisionFlags", 0 );
            material->writeInt( "materialKind", 0 );

            // Read the primitve group data.
            PrimitiveGroup primitiveGroup;
            primitiveGroup.groupIndex_ = 0;

            if (loadingFlags == LOAD_ALL)
            {
                ComplexEffectMaterialPtr pMat = new ComplexEffectMaterial();
                ownMaterials_.push_back( pMat );
                primitiveGroup.material_ = pMat;
                pMat->load( bspMaterial->openSection( "material" ) );
            }

            renderSets_[0].geometry_[0].primitiveGroups_.push_back( primitiveGroup );

        }
    }
#endif

    this->useExistingNodes( NodeCatalogue::instance() );    
}


/**
 * This method releases all low level resources.
 */
void Visual::release()
{
    BW_GUARD;
    renderSets_.clear();
    portals_.clear();
    ownMaterials_.clear();
    isInVisualManager_ = true;
    pBSP_ = NULL;
    isOK_ = true;
}


/**
 * Constructor for the visual.
 *
 * @param resourceID the identifier of the visuals resource.
 * @param validateNodes Boolean indicating whether to validate that indices
 *                      used mesh creation refer to valid vertices.
 * @param loadingFlags  Flags for partial loading (geometry-only for flora for example).
 */
Visual::Visual( const BW::StringRef& resourceID, bool validateNodes /*= true */, uint32 loadingFlags /*= LOAD_ALL*/ ) : 
    isInVisualManager_( true ),
    pBSP_( NULL ),
    validateNodes_( validateNodes ),
    isOK_( true ), 
    resourceID_( resourceID.data(), resourceID.size() ),
    uvSpaceDensity_( ModelTextureUsage::INVALID_DENSITY )
{
    this->load( loadingFlags );
}


// Destruct the visual
Visual::~Visual()
{
    BW_GUARD;

    if (isInVisualManager_)
    {
        // get rid of the bsp
        {
            SimpleMutexHolder lock( s_premadeBSPsLock );
            BSPMap::iterator it = s_premadeBSPs.find( resourceID_ );
            if (it != s_premadeBSPs.end())
            {
                s_premadeBSPs.erase(it);
            }
        }

        // let the visual manager know we're gone
        isInVisualManager_ = false;
        VisualManager::del( this );
    }

    // dispose any materials we allocated
    ownMaterials_.clear();
}

#ifdef EDITOR_ENABLED

bool Visual::updateBSP()
{
    BW_GUARD;
    BW::string baseName = resourceID_.substr(
        0, resourceID_.find_last_of( '.' ) );

    VisualLoader< Visual > loader( baseName );
    BSPTreePtr      pFoundBSP;
    BSPMaterialIDs  materialIDs;
    BSPMaterialIDs  missingBspMaterialIDs;
    bool bspRequiresMapping = loader.loadBSPTreeNoCache( pFoundBSP, materialIDs );

    if (!pFoundBSP) return false;

    BSPCache::instance().add( resourceID_, pFoundBSP );
    pBSP_ = pFoundBSP;

    if (bspRequiresMapping)
        loader.remapBSPMaterialFlags( *pBSP_->pTree(), ownMaterials_, materialIDs, missingBspMaterialIDs );

    return true;
}

#endif // EDITOR_ENABLED

/*
 *  This class is a helper class for the visual draw methods to set
 *  up common properties before drawing.
 */
VisualHelper::VisualHelper()
{
    BW_GUARD;
    pLC_ = new LightContainer;
    pSLC_ = new LightContainer;
}

/*
 *  This method works out wether this visual should draw or not.
 *  @param ignoreBoundingBox if true the bounding box is assumed to be in the view volume.
 *  @param bb the bounding box of the visual
 *  @return true if the visual should be drawn.
 */
bool VisualHelper::shouldDraw( bool ignoreBoundingBox, const BoundingBox& bb )
{
    BW_GUARD;
    bool shouldDraw = true;

    worldViewProjection_.multiply( rc().world(), rc().viewProjection() );
    if (!ignoreBoundingBox)
    {
        bb.calculateOutcode( worldViewProjection_ );
        shouldDraw = !bb.combinedOutcode();
    }
    return shouldDraw;
}

/*
*   This method sets up the lighting and the common renderstates used by visuals.
*   @param dynamicOnly wether to set up only dynamic lights
*   @param bb the bounding box for the visual
*   @param rendering whether this is an actual rendering pass or not
*/
void VisualHelper::start( const BoundingBox& bb, bool rendering )
{
    BW_GUARD;
    // Transform the bounding box to world space
    wsBB_ = bb;
    wsBB_.transformBy( rc().world() );

    // Set up the light containers
    pRCLC_ = rc().lightContainer();
    pLC_->init( pRCLC_, wsBB_, true );
    rc().lightContainer( pLC_ );

    pRCSLC_ = rc().specularLightContainer();
    pSLC_->init( pRCSLC_, wsBB_, true );
    rc().specularLightContainer( pSLC_ );
}


/*
*   This method resets lighting and common renderstates.
*   @param rendering whether this is an actual rendering pass or not
*/
void VisualHelper::fini( bool rendering )
{
    BW_GUARD;
    // reset the light containers
    rc().lightContainer( pRCLC_ );
    rc().specularLightContainer( pRCSLC_ );
}

namespace 
{

DrawContext::InstanceData*  captureInstanceData( DrawContext& drawContext,
                            const Visual::RenderSet& renderSet )
{
    const bool isRenderSetSkinned = renderSet.treatAsWorldSpaceObject_;
    DrawContext::InstanceData* data = NULL;
    if (isRenderSetSkinned)
    {
        // allocate matrix palette
        uint32 paletteSize = (uint32)renderSet.transformNodes_.size();
        data = drawContext.allocInstanceData( (uint32)paletteSize );
        Vector4* pMatrixPalette = data->palette();
        MF_ASSERT( paletteSize > 0 );

        // capture palette from transform nodes
        NodePtrVector::const_iterator it = renderSet.transformNodes_.begin();
        NodePtrVector::const_iterator end = renderSet.transformNodes_.end();
        Vector4* dst = pMatrixPalette;
        while (it != end)
        {
            Matrix m;
            XPMatrixTranspose( &m, &(*it)->worldTransform() );
            *dst++ = m.row(0);
            *dst++ = m.row(1);
            *dst++ = m.row(2);
            it++;
        }
        MF_ASSERT( dst == pMatrixPalette +
            renderSet.transformNodes_.size() * 
            DrawContext::InstanceData::NUM_VECTOR4_PER_SKIN_MATRIX );
    }
    else
    {
        // allocate matrix and capture it from transform nodes
        data = drawContext.allocInstanceData( 0 );
        *data->matrix() = renderSet.transformNodes_.front()->worldTransform();
    }
    return data;
}
}
/**
 *  This method draws the visual.
 *  @param ignoreBoundingBox if this value is true, this
        method will not cull the visual based on its bounding box.
 *  @param useDefaultPose set this value to false if you have
 *      manually updated the node catalogue for this visual.
 *  @param pMorphState The current tween state between morphTargets
 *  @return S_OK if succeeded
 */
HRESULT Visual::draw( Moo::DrawContext& drawContext,
                     bool ignoreBoundingBox,
                     bool useDefaultPose )
{
    BW_GUARD_PROFILER( Visual_draw );

    if (!isOK_) return S_FALSE;

    static DogWatch visual( "Visual" );
    visual.start();

    static VisualHelper helper;

    if (helper.shouldDraw(ignoreBoundingBox, bb_))
    {
        // If the world transforms in our node catalogue haven't been updated,
        // then do so by traversing our original hierarchy and copying the
        // results into the global node catalogue.
        if (useDefaultPose)
        {
            rootNode_->traverse();
            rootNode_->loadIntoCatalogue();             
        }
        helper.start( bb_ );

        std::vector< Visual::RenderSet >::iterator rsit = renderSets_.begin();
        std::vector< Visual::RenderSet >::iterator rsend = renderSets_.end();

        // Iterate through our rendersets.
        while (rsit != rsend)
        {
            Visual::RenderSet& renderSet = *rsit;
            ++rsit;

            Visual::GeometryVector::iterator git = renderSet.geometry_.begin();
            Visual::GeometryVector::iterator gend = renderSet.geometry_.end();

            DrawContext::InstanceData* instanceData = captureInstanceData( drawContext, renderSet );

            // Iterate through the geometry
            while (git != gend)
            {
                Visual::Geometry& geometry = *git;
                ++git;

                Visual::PrimitiveGroupVector::iterator pgit = 
                    geometry.primitiveGroups_.begin();
                Visual::PrimitiveGroupVector::iterator pgend = 
                    geometry.primitiveGroups_.end();

                // Iterate through our primitive groups.
                while (pgit != pgend)
                {
                    Visual::PrimitiveGroup& primitiveGroup = *pgit;
                    ++pgit;

                    drawContext.drawRenderOp( primitiveGroup.material_.get(),
                        geometry.vertices_.get(), geometry.primitives_.get(),
                        primitiveGroup.groupIndex_, instanceData, helper.worldSpaceBB() );
                }
            }
        }
    }

    visual.stop();

    return S_OK;
}




/**
 *  This method tells the visual that its nodes will already have been
 *  traversed when it gets drawn, and that the transforms it should use
 *  for drawing them will not be stored in its own copy of the nodes,
 *  but rather in those ones provided by the given catalogue.
 *  The identifiers of the nodes currently used by each RenderSet
 *  is used to look up which node they should be sourced from in
 *  the provided node catalogue.
 *
 *  @note: After this call, the model's node hierarchy is still available
 *  through the rootNode accessor, however the transforms in those nodes
 *  will no longer be used to draw the visual - only the nodes' names will
 *  be the same.
 */
void Visual::useExistingNodes( NodeCatalogue& nodeCatalogue )
{
    BW_GUARD;
    NodeCatalogue::grab();  

    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        RenderSet& renderSet = *rsit;
        ++rsit;

        for (NodePtrVector::iterator nit = renderSet.transformNodes_.begin();
            nit != renderSet.transformNodes_.end();
            ++nit)
        {
            *nit = nodeCatalogue[ (*nit)->identifier() ];
            MF_ASSERT_DEV( *nit );
        }
    }

    NodeCatalogue::give();
}

/**
 *  Add an animation to the visuals animation list.
 *
 *  @param animResourceID the resource identifier of the animation to load.
 *
 *  @return pointer to the animation that was added
 *
 */
AnimationPtr Visual::addAnimation( const BW::string& animResourceID )
{
    BW_GUARD;
    AnimationPtr anim = AnimationManager::instance().get( animResourceID, rootNode_ );
    if (anim)
    {
        animations_.push_back( anim );
    }
    return anim;
}


/**
 *  Find an animation in the visuals animation list.
 *
 *  @param identifier the animation identifier
 *
 *  @return a pointer to the animation
 *
 */
AnimationPtr Visual::findAnimation( const BW::string& identifier ) const
{
    BW_GUARD;
    AnimationVector::const_iterator it = animations_.begin();
    AnimationVector::const_iterator end = animations_.end();
    while (it != end)
    {
        if ((*it)->identifier() == identifier)
        {
            return *it;
        }

        if ((*it)->internalIdentifier() == identifier)
        {
            return *it;
        }
        ++it;
    }
    return NULL;
}
    

/**
 *  @return The number of constraints for this visual data.
 */
uint32 Visual::nConstraints() const
{
    return static_cast<uint32>(constraints_.size());
}


/**
 *  @pre i is in the range [0, nConstraints()).
 *  @post Returned the constraint at index @a i. It's guaranteed that the
 *      identifier for the result is not the same as for other constraints in
 *      this visual.
 */
const ConstraintData & Visual::constraint( uint32 i ) const
{
    MF_ASSERT( i < nConstraints() );
    return constraints_[i];
}


/**
 *  @return The number of IK handles for this visual data.
 */
uint32 Visual::nIKHandles() const
{
    return static_cast<uint32>(ikHandles_.size());
}


/**
 *  @pre i is in the range [0, nIKHandles()).
 *  @post Returned the IK handle at index @a i. It's guaranteed that the
 *      identifier for the result is not the same as for other IK handles in
 *      this visual.
 */
const IKHandleData & Visual::ikHandle( uint32 i ) const
{
    MF_ASSERT( i < nIKHandles() );
    return ikHandles_[i];
}


/**
 *  This method returns the number of triangles used by this geometry.
 */
uint32 Visual::Geometry::nTriangles() const
{
    BW_GUARD;
    uint32 nTriangles = 0;

    for( uint32 i = 0; i < primitives_->nPrimGroups(); i++ )
    {
        const Moo::PrimitiveGroup& pg = primitives_->primitiveGroup( i );
        nTriangles += pg.nPrimitives_;
    }
    return nTriangles;
}

/**
 *  This method overrides all materials that have the given materialIdentifier
 *  with the given material.
 */
void Visual::overrideMaterial( const BW::string& materialIdentifier, const ComplexEffectMaterial& mat )
{
    BW_GUARD;
    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    // Iterate through our rendersets.
    while (rsit != rsend)
    {
        RenderSet& renderSet = *rsit;
        ++rsit;

        GeometryVector::iterator git = renderSet.geometry_.begin();
        GeometryVector::iterator gend = renderSet.geometry_.end();

        // Iterate through our geometries
        while (git != gend)
        {
            Geometry& geometry = *git;
            git++;

            PrimitiveGroupVector::iterator pgit = geometry.primitiveGroups_.begin();
            PrimitiveGroupVector::iterator pgend = geometry.primitiveGroups_.end();

            // Iterate through our primitive groups.
            while (pgit != pgend)
            {
                PrimitiveGroup& primitiveGroup = *pgit;
                ++pgit;

                if (primitiveGroup.material_->identifier() == materialIdentifier)
                {
                    *primitiveGroup.material_ = mat;
                }
            }
        }
    }

}


/**
 *  This method gathers all the uses of the given material identifier in
 *  this visual, and puts pointers to their pointers into the given vector.
 *  It also optionally sets a pointer to the initial material of the given
 *  identifier into the ppOriginal argument (when non-NULL)
 *  It returns the number of material pointers found (the size of the vector)
 *
 *  If materialIdentifier is a wildcard "*", this function returns all materials.
 */
int Visual::gatherMaterials(
        const BW::string& materialIdentifier,
        BW::vector<PrimitiveGroup*>& primGroups,
        ConstComplexEffectMaterialPtr* ppOriginal
    )
{
    BW_GUARD;
    int mindex = 0;
    primGroups.clear();

    bool everyMaterial = (materialIdentifier == "*");

    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    // Iterate through our rendersets.
    while (rsit != rsend)
    {
        RenderSet& renderSet = *rsit;
        ++rsit;

        GeometryVector::iterator git = renderSet.geometry_.begin();
        GeometryVector::iterator gend = renderSet.geometry_.end();

        // Iterate through our geometries
        while (git != gend)
        {
            Geometry& geometry = *git;
            git++;

            PrimitiveGroupVector::iterator pgit = geometry.primitiveGroups_.begin();
            PrimitiveGroupVector::iterator pgend = geometry.primitiveGroups_.end();

            // Iterate through our primitive groups.
            while (pgit != pgend)
            {
                PrimitiveGroup& primitiveGroup = *pgit;
                ++pgit;

                if (everyMaterial ||
                    ownMaterials_[mindex]->identifier() == materialIdentifier)
                {
                    primGroups.push_back( &primitiveGroup );
                    if (ppOriginal)
                    {
                        *ppOriginal = ownMaterials_[mindex];
                        ppOriginal = NULL;
                    }
                }

                mindex++;
            }
        }
    }

    return static_cast<int>(primGroups.size());
}


/**
 *  This method collates a list of all the visual's original material ptrs.
 *
 *  @param materials    a vector of material pointers to fill.
 *
 *  @return int The number of materials found
 */
int Visual::collateOriginalMaterials(BW::vector<ComplexEffectMaterialPtr>& materials)
{
    BW_GUARD;
    materials.clear();

    BW::vector<ComplexEffectMaterialPtr>::iterator it  = ownMaterials_.begin();
    BW::vector<ComplexEffectMaterialPtr>::iterator end = ownMaterials_.end();

    while ( it != end )
    {
        materials.push_back( *it++ );
    }

    return static_cast<int>(materials.size());
}

/**
 *  This method is used to fetch the cached minimum uvSpaceDensity of 
 *  the visual's geometry. If the value was not stored in the file,
 *  or it has not been calculated yet, it will perform the calculation.
 */
float Visual::uvSpaceDensity()
{
    // Check if we have a valid value
    if (uvSpaceDensity_ < ModelTextureUsage::MIN_DENSITY)
    {
        uvSpaceDensity_ = calculateUVSpaceDensity();
    }
    return uvSpaceDensity_;
}

/**
 *  This method calculates the minimum uvSpaceDensity of the visual's
 *  geometry. It does this by calculating the uvSpace size and modelSpace
 *  size of each triangle of the visual, and recording the minimum density.
 */
float Visual::calculateUVSpaceDensity()
{
    float minUvDensity = FLT_MAX;
    
    beginRead();

    // Iterate over every primitive group:
    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    // Iterate through our rendersets.
    while (rsit != rsend)
    {
        RenderSet & renderSet = *rsit;
        ++rsit;

        GeometryVector::iterator git = renderSet.geometry_.begin();
        GeometryVector::iterator gend = renderSet.geometry_.end();

        // Iterate through our geometries
        while (git != gend)
        {
            Geometry & geometry = *git;
            git++;

            Moo::VerticesPtr verts = geometry.vertices_;
            const VertexDeclaration* pVertDecl = verts->pDecl();
            if (!pVertDecl)
            {
                ERROR_MSG( "Visual::calculateTexelDensity: could not get "
                    "vertex declaration from vertices in %s\n", 
                    verts->resourceID().c_str() );
                return ModelTextureUsage::MIN_DENSITY;
            }
            VertexBuffer vertBuffer = verts->vertexBuffer();
            SimpleVertexLock vertLock( vertBuffer, 0, 0, D3DLOCK_READONLY );

            // Prepare functor for texel density calculation
            UvDensityTriGenFunctor densityFunct(
                pVertDecl->format(), vertLock, vertLock.size(), 0, 0);
            if (!densityFunct.isValid())
            {
                continue;
            }

            PrimitiveGroupVector::iterator pgit = geometry.primitiveGroups_.begin();
            PrimitiveGroupVector::iterator pgend = geometry.primitiveGroups_.end();

            // Iterate through our primitive groups.
            while (pgit != pgend)
            {
                PrimitiveGroup & primitiveGroup = *pgit;
                ++pgit;

                // Ignore additional primgroups as we allow
                // materialless primgroups for bsp materials
                if (primitiveGroup.groupIndex_ >= 
                    geometry.primitives_->nPrimGroups())
                {
                    continue;
                }

                const Moo::PrimitiveGroup & prims = 
                    geometry.primitives_->primitiveGroup( primitiveGroup.groupIndex_ );
                const Moo::IndicesHolder & indices = geometry.primitives_->indices();

                // Iterate over triangles, calculating texel density, updating functor's min
                PrimitiveHelper::generateTrianglesFromIndices( indices, 
                    prims.startIndex_, prims.nPrimitives_, 
                    PrimitiveHelper::TRIANGLE_LIST, densityFunct, 
                    indices.size() );
            }

            // Update top-level minimum
            minUvDensity = min(minUvDensity, densityFunct.getMinUvDensity());

        } // per geometry
    } // per render set

    endRead();

    if (minUvDensity == FLT_MAX)
    {
        return ModelTextureUsage::MIN_DENSITY;
    }
    return minUvDensity;
}

/**
 * This method populates the usage group with the texture usage of this
 * visual.
 */
void Visual::generateTextureUsage(Moo::ModelTextureUsageGroup& usageGroup)
{
    beginRead();

    float density = uvSpaceDensity();

    // Iterate over each material used by the visual and extract it's textures
    BW::vector<ComplexEffectMaterialPtr>::iterator it = ownMaterials_.begin();
    BW::vector<ComplexEffectMaterialPtr>::iterator end = ownMaterials_.end();

    for (; it != end; ++it)
    {
        EffectMaterialPtr pMaterial = (*it)->baseMaterial();
        pMaterial->generateTextureUsage( usageGroup, density );
    }

    endRead();
}

namespace
{
    template< int skipSize >
    void memcpy_strided( void* dest, void* src, size_t elements, size_t elementSize )
    {
        BW_GUARD;
        uint8* d_ptr = static_cast<uint8*>( dest );
        uint8* s_ptr = static_cast<uint8*>( src );
        uint8* end = d_ptr + (elementSize * elements);

        while ( d_ptr != end )
        {
            memcpy( d_ptr, s_ptr, elementSize );

            d_ptr += elementSize;
            s_ptr += (elementSize + skipSize);
        }
    }

    template<>
    void memcpy_strided<0>( void* dest, void* src, size_t elements, size_t elementSize )
    {
        BW_GUARD;
        memcpy( dest, src, elements * elementSize );
    }

    bool copyData( const VertexFormat & format, PrimitivePtr& prims, VerticesPtr& verts,
                   const VertexFormat & retFormat, void*& retPVertices, IndicesHolder& retPIndices,
                    uint32 & retNumVertices, uint32 & retNumIndices )
    {
        BW_GUARD;
        MF_ASSERT( format.streamCount() == 1 &&
                   retFormat.streamCount() == 1 );

        Moo::VertexBuffer vb = verts->vertexBuffer();

        if ( vb.valid() )
        {
            uint32 nVertices = verts->nVertices();
            const uint32 sourceStreamIndex = 0;
            const uint32 srcSize = nVertices * format.streamStride( sourceStreamIndex );
            Moo::SimpleVertexLock vl( vb, 0, srcSize, 0 );
            if( vl )
            {
                uint32 nIndices = 0;
                D3DPRIMITIVETYPE pt = prims->primType();


                for (uint32 i = 0; i < prims->nPrimGroups(); i++)
                {
                    nIndices += prims->primitiveGroup( i ).nPrimitives_ * 3;
                }

                const uint32 destStreamIndex = 0;
                const uint32 dstSize = verts->nVertices() * retFormat.streamStride( destStreamIndex );
                retPVertices = new uint8[ dstSize ];
                retPIndices.setSize( nIndices, prims->indicesFormat() );
                retNumVertices = verts->nVertices();
                retNumIndices = nIndices;

                // perform vertex format conversion on vertex data
                VertexFormat::ConversionContext vfConversion(&retFormat, &format);
                MF_ASSERT( vfConversion.isValid() );
                bool converted = vfConversion.convertSingleStream( 
                    retPVertices, destStreamIndex, vl, sourceStreamIndex, nVertices );
                MF_ASSERT( converted );

                prims->indicesIB( retPIndices );
                return retPIndices.valid();
            }
        }
        return false;
    }

}

/**
 *  This method creates a copy of this visual.
 *  The copy is stored in a newly allocated array of vertices and indices.
 *
 *  @param retPVertices     the address of a pointer to vertices.  This will
 *                          be set to point to the new array of vertices.
 *  @param retPIndices      the address of a pointer to indices.  This will
 *                          be set to point to the new array of indices.
 *  @param retNumVertices   an integer reference that will be set to the number
 *                          of vertices in the new array of vertices
 *  @param retNumIndices    an integer reference that will be set to the number
 *                          of indices in the new array of indices
 *  @param material         The primary material associated with the copied
 *                          Visual.
 *
 *  @return true if the copy could be made, of false if the method fails.
 */
bool Visual::createCopy( const VertexFormat & retFormat, void* & retPVertices, IndicesHolder& retPIndices,
                        uint32 & retNumVertices, uint32 & retNumIndices, EffectMaterialPtr & material )
{
    BW_GUARD;
    bool success = false;

    retPVertices = NULL;
    retNumVertices = 0;
    retNumIndices = 0;

    // Iterate through our rendersets.
    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::iterator git = renderSet.geometry_.begin();
        GeometryVector::iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            Geometry& geometry = *git;
            ++git;

            // Just get the one primitive group
            VerticesPtr verts = geometry.vertices_;
            PrimitivePtr prims = geometry.primitives_;

            if (geometry.primitiveGroups_[0].material_)
                material = geometry.primitiveGroups_[0].material_->baseMaterial();
            else
                material = NULL;

            const VertexFormat& pFormat = geometry.vertices_->pDecl()->format();
            success = copyData( pFormat, prims, verts, 
                retFormat, retPVertices, retPIndices, 
                retNumVertices, retNumIndices );
        }
    }

    return success;
}

/**
 *  This method return the vertices number as flora
 *  since Flora visual don't use indices buffer, so
 *  it equal to the number of indices. it also count 
 *  on if it's valid format.
 *
 *  @return the number of vertices as a flora.
 */
uint32 Visual::nVerticesAsFlora() const
{
    BW_GUARD;
    uint32 retNumVertices = 0;

    // Iterate through our rendersets.
    RenderSetVector::const_iterator rsit = renderSets_.begin();
    RenderSetVector::const_iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        const RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::const_iterator git = renderSet.geometry_.begin();
        GeometryVector::const_iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            const Geometry& geometry = *git;
            ++git;
            if (geometry.vertices_->sourceFormat() == "xyznuv" || 
                geometry.vertices_->sourceFormat() == "xyznuvtb")
            {
                PrimitivePtr prims = geometry.primitives_;
                for (uint32 i = 0; i < prims->nPrimGroups(); i++)
                {
                    retNumVertices +=
                        prims->primitiveGroup( i ).nPrimitives_ * 3;
                }
            }
        }
    }

    return retNumVertices;
}


/**
 *  This method returns the number of vertices in a visual.
 */
uint32 Visual::nVertices() const
{
    BW_GUARD;
    uint32 retNumVertices = 0;

    // Iterate through our rendersets.
    RenderSetVector::const_iterator rsit = renderSets_.begin();
    RenderSetVector::const_iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        const RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::const_iterator git = renderSet.geometry_.begin();
        GeometryVector::const_iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            const Geometry& geometry = *git;
            ++git;
            retNumVertices += geometry.vertices_->nVertices();
        }
    }

    return retNumVertices;
}


/**
 *  This method returns the number of triangles in a visual.
 */
uint32 Visual::nTriangles() const
{
    BW_GUARD;
    uint32 num = 0;

    // Iterate through our rendersets.
    RenderSetVector::const_iterator rsit = renderSets_.begin();
    RenderSetVector::const_iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        const RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::const_iterator git = renderSet.geometry_.begin();
        GeometryVector::const_iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            const Geometry& geometry = *git;
            ++git;
            num += geometry.nTriangles();
        }
    }

    return num;
}


/**
 *  This method returns vital information about primitive group 0.  That is,
 *  The vertices, primitives and the material.
 *
 *  Currently this method is only used by MeshParticleRenderer's BonedVisualMPT
 *  class.
 */
bool Visual::primGroup0( Moo::VerticesPtr & retVerts, Moo::PrimitivePtr & retPrim, PrimitiveGroup* &retPPrimGroup )
{
    BW_GUARD;
    // get render set #0
    if ( renderSets_.size() > 0 )
    {
        RenderSet& renderSet = renderSets_[0];

        // get geometry #0
        if ( renderSet.geometry_.size() > 0 )
        {
            // Just get the one primitive group
            Geometry& geometry = renderSet.geometry_[0];
            retVerts = geometry.vertices_;
            retPrim = geometry.primitives_;
            retPPrimGroup = &geometry.primitiveGroups_[0];
            return true;
        }
    }

    return false;
}


/**
 *  Draw the visual's primitives and do nothing else.
 *  Assumes : vertex / pixel shader set up
 *            no nodes involved (i.e. not skinned)
 */
void Visual::justDrawPrimitives()
{
    BW_GUARD;
    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    // Iterate through our rendersets.
    while (rsit != rsend)
    {
        RenderSet& renderSet = *rsit;
        ++rsit;

        GeometryVector::iterator git = renderSet.geometry_.begin();
                GeometryVector::iterator gend = renderSet.geometry_.end();

        // Iterate through the geometry
        while( git != gend )
        {
            Geometry& geometry = *git;
            ++git;

            // Set our vertices.
            if (FAILED( geometry.vertices_->setVertices(false) ))
            {
                ASSET_MSG( "Moo::Visual: Couldn't set vertices for %s\n", resourceID_.c_str() );
                continue;
            }

            // Set our indices.
            if (FAILED( geometry.primitives_->setPrimitives() ))
            {
                ASSET_MSG( "Moo::Visual: Couldn't set primitives for %s\n", resourceID_.c_str() );
                continue;
            }

            PrimitiveGroupVector::iterator pgit = geometry.primitiveGroups_.begin();
            PrimitiveGroupVector::iterator pgend = geometry.primitiveGroups_.end();

            // Iterate through our primitive groups.
            while (pgit != pgend)
            {
                PrimitiveGroup& primitiveGroup = *pgit;
                ++pgit;

                geometry.primitives_->drawPrimitiveGroup( primitiveGroup.groupIndex_ );
            }
        }
    }
}

//-- instanced version of the justDrawPrimitives.
//--------------------------------------------------------------------------------------------------
void Visual::justDrawPrimitivesInstanced(uint offset, uint numInstances)
{
    BW_GUARD;
    MF_ASSERT(numInstances > 0 && "Number of instances has to be at least 1.");
    MF_ASSERT(instancingStream_ && "Instancing buffer has to exist.");

    RenderSetVector::iterator rsit = renderSets_.begin();
    RenderSetVector::iterator rsend = renderSets_.end();

    //-- set up the geometry and instance data streams.
    Moo::rc().device()->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | numInstances));
    Moo::rc().device()->SetStreamSourceFreq(InstancingStream::STREAM_NUMBER, (D3DSTREAMSOURCE_INSTANCEDATA | 1));
    instancingStream_->set(
        Moo::InstancingStream::STREAM_NUMBER, offset * InstancingStream::ELEMENT_SIZE, InstancingStream::ELEMENT_SIZE
        );

    // Iterate through our rendersets.
    while (rsit != rsend)
    {
        RenderSet& renderSet = *rsit;
        ++rsit;

        GeometryVector::iterator git = renderSet.geometry_.begin();
        GeometryVector::iterator gend = renderSet.geometry_.end();

        // Iterate through the geometry
        while( git != gend )
        {
            Geometry& geometry = *git;
            ++git;

            // Set our vertices.
            if (FAILED( geometry.vertices_->setVertices(false, true) ))
            {
                ERROR_MSG( "Moo::Visual: Couldn't set vertices for %s\n", resourceID_.c_str() );
                continue;
            }

            // Set our indices.
            if (FAILED( geometry.primitives_->setPrimitives() ))
            {
                ERROR_MSG( "Moo::Visual: Couldn't set primitives for %s\n", resourceID_.c_str() );
                continue;
            }

            PrimitiveGroupVector::iterator pgit = geometry.primitiveGroups_.begin();
            PrimitiveGroupVector::iterator pgend = geometry.primitiveGroups_.end();

            // Iterate through our primitive groups.
            while (pgit != pgend)
            {
                PrimitiveGroup& primitiveGroup = *pgit;
                ++pgit;

                geometry.primitives_->drawInstancedPrimitiveGroup(
                    primitiveGroup.groupIndex_, numInstances
                    );
            }
        }
    }
    
    //-- restore instancing state.
    Moo::rc().device()->SetStreamSourceFreq(0, 1);
    Moo::rc().device()->SetStreamSourceFreq(InstancingStream::STREAM_NUMBER, 1);
    VertexBuffer::reset(InstancingStream::STREAM_NUMBER);
}

uint32 Visual::vertexBufferMemory() const
{
    BW_GUARD;
    uint32 num = 0;

    // Iterate through our rendersets.
    RenderSetVector::const_iterator rsit = renderSets_.begin();
    RenderSetVector::const_iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        const RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::const_iterator git = renderSet.geometry_.begin();
        GeometryVector::const_iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            const Geometry& geometry = *git;
            ++git;
            num += geometry.vertices_->vertexBuffer().size();
        }
    }

    return num;
}

void Visual::getVertices( BW::map<void*, int>& vertices )
{
    BW_GUARD;

    // Iterate through our rendersets.
    RenderSetVector::const_iterator rsit = renderSets_.begin();
    RenderSetVector::const_iterator rsend = renderSets_.end();

    while (rsit != rsend)
    {
        const RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::const_iterator git = renderSet.geometry_.begin();
        GeometryVector::const_iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            const Geometry& geometry = *git;
            ++git;
            vertices.insert(std::make_pair(geometry.vertices_.get(), geometry.vertices_->vertexBuffer().size()));
        }
    }
}

// Clamps all vertices to bound value
void Visual::clampVerticesToBound( float bound )
{
    BW_GUARD;

    if(clampedToBound_ <= bound)
        return;

    clampedToBound_ = bound;

    // Iterate through our rendersets.
    RenderSetVector::const_iterator rsit = renderSets_.begin();
    RenderSetVector::const_iterator rsend = renderSets_.end();

    Vector3 bounds(bound, bound, bound);
    Vector3 minus_bounds(-bound, -bound, -bound);

    while (rsit != rsend)
    {
        const RenderSet& renderSet = *rsit;
        ++rsit;

        // Iterate through the geometry
        GeometryVector::const_iterator git = renderSet.geometry_.begin();
        GeometryVector::const_iterator gend = renderSet.geometry_.end();

        while( git != gend )
        {
            const Geometry& geometry = *git;
            ++git;
            if(geometry.vertices_)
            {
                Moo::VertexBuffer vb = geometry.vertices_->vertexBuffer();
                void* pData = NULL;
                if( SUCCEEDED( vb.lock(0, vb.size(), &pData, 0) ) )
                {
                    size_t nVetrs = geometry.vertices_->nVertices();
                    for (size_t i = 0; i < nVetrs; ++i)
                    {
                        Vector3& pos = *(Vector3*)pData;
                        pos.clamp(minus_bounds, bounds);
                        pData = (char*)pData + geometry.vertices_->vertexStride();
                    }
                    vb.unlock();
                }
            }
        }
    }
}

/**
 * EffectVisualContextSetter implementation
 */
EffectVisualContextSetter::EffectVisualContextSetter( Visual::RenderSet* pRenderSet )
{
    if (pRenderSet->treatAsWorldSpaceObject_)
    {
        // calculate shader ready matrix palette
        NodePtrVector::const_iterator it = pRenderSet->transformNodes_.begin();
        NodePtrVector::const_iterator end = pRenderSet->transformNodes_.end();
        Matrix m;
        uint32 index = 0;
        uint32 paletteSize = (uint32)pRenderSet->transformNodes_.size();
        MF_ASSERT( paletteSize * EffectVisualContext::NUM_VECTOR4_PER_PALETTE_ENTRY < MAX_WORLD_PALETTE_SIZE );
        while (it != end)
        {
            XPMatrixTranspose( &m, &(*it)->worldTransform() );
            matrixPalette_[index++] = m.row(0);
            matrixPalette_[index++] = m.row(1);
            matrixPalette_[index++] = m.row(2);
            it++;
        }

        rc().effectVisualContext().worldMatrixPalette( matrixPalette_, paletteSize );
    }
    else
    {
        // assign a single world matrix
        rc().effectVisualContext().worldMatrix( &pRenderSet->transformNodes_.front()->worldTransform() );
    }
}

EffectVisualContextSetter::~EffectVisualContextSetter()
{
    rc().effectVisualContext().worldMatrix( NULL );
}

namespace BSPTreeHelper
{
    /**
     * Create array of vertices from given BSP tree.
     */
    void createVertexList( 
        const BSPTree &             source,
        BW::vector< Moo::VertexXYZL >& verts, 
        uint32                          colour )
    {
        BW_GUARD;
        // Count triangles, ignoring NOCOLLIDE triangles
        uint32 nVerts = 0;

        uint numTriangles = source.numTriangles();
        WorldTriangle* pTriangles = source.triangles();

        for( uint i = 0; i < numTriangles; i++)
        {
            if ( !(pTriangles[i].flags() & TRIANGLE_NOCOLLIDE) )
                nVerts += 3;
        }

        if ( nVerts > 0 )
        {
            // Resize vector to fit
            verts.resize( nVerts );

            // Copy triangles in
            uint vi = 0;

            for( uint i = 0; i < numTriangles; i++)
            {
                const WorldTriangle & tri = pTriangles[i];

                if ( !( tri.flags() & TRIANGLE_NOCOLLIDE ) )
                {
                    verts[vi].colour_   = colour;
                    verts[vi++].pos_    = tri.v0();

                    verts[vi].colour_   = 0xFFFFFFFF;
                    verts[vi++].pos_    = tri.v1();

                    verts[vi].colour_   = colour;
                    verts[vi++].pos_    = tri.v2();
                }
            }

            MF_ASSERT_DEBUG( vi == verts.size() );
        }
    }
}

}   // namespace Moo

BW_END_NAMESPACE

/*visual.cpp*/
