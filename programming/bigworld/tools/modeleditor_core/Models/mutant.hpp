#ifndef MUTANT_HPP
#define MUTANT_HPP

#include "cstdmf/bw_set.hpp"
#include "chunk/chunk_bsp_holder.hpp"
// This is temporary as mutant will be made into a pymodel for 1.10
#include "duplo/py_attachment.hpp"
#include "duplo/action_queue.hpp"
#include "gizmo/meta_data.hpp"
#include "moo/reload.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/visual.hpp"
#include "moo/shadow_manager.hpp"
#include "moo/reload.hpp"

BW_BEGIN_NAMESPACE

class GeneralEditor;
class SuperModel;

typedef SmartPointer< class AnimLoadCallback > AnimLoadCallbackPtr;
typedef SmartPointer< class SuperModelAnimation > SuperModelAnimationPtr;
typedef SmartPointer< class SuperModelAction > SuperModelActionPtr;
typedef SmartPointer< class SuperModelDye > SuperModelDyePtr;
typedef SmartPointer< class Model > ModelPtr;

typedef SmartPointer< class XMLSection > XMLSectionPtr;

typedef std::pair < BW::string , BW::string > StringPair;
typedef std::pair < StringPair , BW::vector < BW::string > > TreeBranch;
typedef BW::vector < TreeBranch > TreeRoot;

typedef std::pair < StringPair , float > LODEntry;
typedef BW::vector < LODEntry > LODList;

typedef BW::map < BW::string, BW::string > Dyes;
typedef BW::set< Moo::ComplexEffectMaterialPtr > ComplexEffectMaterialSet;

class MatrixLiaisonIdentity : public MatrixLiaison
{
public:
    virtual const Matrix & getMatrix() const { return Matrix::identity; }
    virtual bool setMatrix( const Matrix & m ) { return true; };
};

struct ChannelsInfo : public ReferenceCount
{
    BW::vector< Moo::AnimationChannelPtr > list;
};
typedef SmartPointer<ChannelsInfo> ChannelsInfoPtr;

class AnimationInfo
{
public:
    AnimationInfo();
    AnimationInfo(
        DataSectionPtr cData,
        DataSectionPtr cModel,
        SuperModelAnimationPtr cAnimation,
        BW::map< BW::string, float >& cBoneWeights,
        DataSectionPtr cFrameRates,
        ChannelsInfoPtr cChannels,
        float animTime,
        bool cIsReadOnly
    );
    ~AnimationInfo();

    Moo::AnimationPtr getAnim();
    Moo::AnimationPtr backupChannels();
    Moo::AnimationPtr restoreChannels();

    void uncompressAnim( Moo::AnimationPtr anim, BW::vector<Moo::AnimationChannelPtr>& oldChannels );
    void restoreAnim( Moo::AnimationPtr anim, BW::vector<Moo::AnimationChannelPtr>& oldChannels );

    DataSectionPtr data;
    DataSectionPtr model;
    SuperModelAnimationPtr animation;
    BW::map< BW::string, float > boneWeights;
    DataSectionPtr frameRates;
    ChannelsInfoPtr channels;
    bool isReadOnly;
};

class ActionInfo
{
public:
    ActionInfo();
    ActionInfo(
        DataSectionPtr cData,
        DataSectionPtr cModel
    );

    DataSectionPtr data;
    DataSectionPtr model;
};

class MaterialInfo
{
public:
    MaterialInfo();
    MaterialInfo(
        const BW::string& cName,
        DataSectionPtr cNameData,
        ComplexEffectMaterialSet cEffect,
        BW::vector< DataSectionPtr > cData,
        BW::string cFormat,
        bool cColours,
        bool cDualUV
    );

    BW::string name;
    DataSectionPtr nameData;
    ComplexEffectMaterialSet effect;
    BW::vector< DataSectionPtr > data;
    BW::string format;
    bool colours;
    bool dualUV;
};

class TintInfo
{
public:
    TintInfo();
    TintInfo(
        Moo::ComplexEffectMaterialPtr cEffect,
        DataSectionPtr cData,
        SuperModelDyePtr cDye,
        BW::string cFormat,
        bool cColours,
        bool cDualUV
    );

    Moo::ComplexEffectMaterialPtr effect;
    DataSectionPtr data;
    SuperModelDyePtr dye;
    BW::string format;
    bool colours;
    bool dualUV;
};

class ModelChangeCallback : public ReferenceCount
{
public:
    ModelChangeCallback() {}

    virtual const bool execute() = 0;

    virtual const void* parent() const = 0;
};

template< class C >
class ModelChangeFunctor: public ModelChangeCallback
{
public:
    typedef bool (C::*Method)();
    
    ModelChangeFunctor( C* parent, Method method ):
        ModelChangeCallback(),
        parent_(parent),
        method_(method)
    {}

    const bool execute()
    {
        return (parent_->*method_)();
    }

    const void* parent() const
    {
        return parent_;
    }
private:
    C* parent_;
    Method method_;
};

/*class MaterialHighlighter : public MaterialFashion
{
public:
    MaterialHighlighter(SuperModel *pSuperModel);

    virtual void dress(SuperModel &model);
    virtual void undress(SuperModel &model);

    void setSelectedMaterial(const BW::string &matName);

private:
    struct Binding
    {
        Binding(Moo::Visual::PrimitiveGroup *_pPrimGroup, const Moo::ComplexEffectMaterialPtr& _effectMat)
            : pPrimGroup(_pPrimGroup), initialMaterial(_effectMat)
        { }

        Moo::Visual::PrimitiveGroup *pPrimGroup;
        Moo::ComplexEffectMaterialPtr initialMaterial;
    };

    typedef BW::vector<Binding> Bindings;

    SuperModel *pSuperModel_;
    Bindings bindings_;
    Moo::ComplexEffectMaterialPtr highlightedMaterial_;
    Moo::ComplexEffectMaterialPtr replacementMaterial_;
};*/


class Mutant:
    /*
    *   Note: ChunkModel is always listening if pSuperModel_ has 
    *   been reloaded, if you are pulling info out from the pSuperModel_
    *   then please update it again in onReloaderReloaded which is 
    *   called when pSuperModel_ is reloaded so that related data
    *   will need be update again.and you might want to do something
    *   in onReloaderPriorReloaded which happen right before the pSuperModel_ reloaded.
    */
    public ReloadListener
#if ENABLE_BSP_MODEL_RENDERING
    , public ChunkBspHolder
#endif // ENABLE_BSP_MODEL_RENDERING
{
public:
    Mutant( bool groundModel, bool centreModel );
    ~Mutant();

    virtual void onReloaderReloaded( Reloader* pReloader );

    void registerModelChangeCallback( SmartPointer <ModelChangeCallback> mcc );
    void unregisterModelChangeCallback( void* parent );

    void groundModel( bool lock );
    void centreModel( bool lock );

    bool hasVisibilityBox( BW::string modelPath = "" );
    int animCount( BW::string modelPath = "" );
    bool nodefull( BW::string modelPath = "" );

    size_t blendBoneCount() const;

    bool hasModel() const { return (superModel_ != NULL); }
    const BW::string& modelName() const { return modelName_; }

    void numNodes ( unsigned numNodes ) { numNodes_ = numNodes; }
    unsigned numNodes () { return numNodes_; }

    BW::string editorProxyName();
    void editorProxyName( BW::string editorProxyName );
    void removeEditorProxy();

    bool texMemUpdate()
    {
        bool val = texMemDirty_;
        texMemDirty_ = false;
        return val;
    }
    
    uint32 texMem() { return texMem_; }
        
    void bumpMutant( DataSectionPtr visual, const BW::string& primitivesName );

    void revertModel();
    void reloadModel( bool inParentOrders = false );
    bool loadModel( const BW::string& name, bool reload = false,  bool inParentOrders = false );
    void postLoad();
    bool addModel( const BW::string& name, bool reload = true );
    
    bool hasAddedModels();
    void removeAddedModels();

    void buildNodeList( DataSectionPtr data, BW::set< BW::string >& nodes );
    bool canAddModel( BW::string modelPath );
    
    void reloadAllLists( bool inParentOrders = false );

    void regenAnimLists();
    void regenMaterialsList();

    DataSectionPtr model( const BW::string& modelPath = "" )
    {
        if (models_.find( modelPath ) == models_.end())
            return currModel_;

        return models_[modelPath];
    }
    DataSectionPtr visual() { return currVisual_; }

    void recreateFashions( bool dyesOnly = false );

    void recreateModelVisibilityBox( AnimLoadCallbackPtr pAnimLoadCallback,
        bool undoable );

    void updateAnimation( const StringPair& animID, float dTime );

    Vector3 getCurrentRootNode();
    BoundingBox zoomBoundingBox();
    BoundingBox modelBoundingBox();

    BoundingBox& transformBoundingBox( BoundingBox& inout );
    BoundingBox visibililtyBox();
    BoundingBox shadowVisibililtyBox();

    void updateFrameBoundingBox();
    void updateVisibilityBox();
    void updateActions(float dTime);
    bool visibilityBoxDirty() const { return visibilityBoxDirty_; }

    Matrix transform( bool grounded, bool centred = true );

    TreeRoot* animTree() { return &animList_; }
    TreeRoot* actTree() { return &actList_; }
    LODList*  lodList() { return &lodList_; }
    TreeRoot* materialTree() { return &materialList_; }
    
    void dirty( DataSectionPtr data );
    void forceClean();
    bool dirty();
    bool isReadOnly() const { return isReadOnly_; }

    void save();
    bool saveAs( const BW::string& newName );

    int updateCount( const BW::string& name )
    {
        return updateCount_[name];
    }

    void triggerUpdate( const BW::string& name )
    {
        updateCount_[name]++;
    }

    void normalsLength( int length ) { normalsLength_ = length; }
    int normalsLength() { return normalsLength_; }
    
    // Render related functions
    void updateModelAnimations( float atDist );
    void drawModel( Moo::DrawContext& drawContext );
    void drawOriginalModel( Moo::DrawContext& drawContext );
    void drawBoundingBoxes();
    void drawSkeleton();
    void drawPortals();
    void reloadBSP();
    bool drawBspInternal();
    void drawNormals( bool showNormals, bool showBinormals );
    void drawHardPoints( Moo::DrawContext& drawContext );
    void calcCustomHull();
    void drawCustomHull();
    void render(  Moo::DrawContext& drawContext, float dTime, int renderStates );
    void switchChannels( BW::map< size_t, BW::vector<Moo::AnimationChannelPtr> >& oldChannels, bool toCompressedChannel );

    // Validation related functions
    
    void clipToDiffRoot( BW::string& strA, BW::string& strB );

    void fixTexAnim( const BW::string& texRoot, const BW::string& oldRoot, const BW::string& newRoot );
    bool fixTextures( DataSectionPtr mat, const BW::string& oldRoot, const BW::string& newRoot );

    bool locateFile( BW::string& fileName, BW::string modelName, const BW::string& ext,
        const BW::string& what, const BW::string& criticalMsg = "" );

    bool isFileReadOnly ( const BW::string& file ) const;
    bool testReadOnly( const BW::string& modelName, const BW::string& visualName, const BW::string& primitivesName ) const;
    bool testReadOnlyAnim( const DataSectionPtr& pData ) const;

    bool ensureModelValid( const BW::string& name, const BW::string& what,
        DataSectionPtr* model = NULL, DataSectionPtr* visual = NULL,
        BW::string* vName = NULL, BW::string* pName = NULL,
        bool* readOnly = NULL);

    bool isFormatDepreciated( DataSectionPtr visual, const BW::string& primitivesName );

    static void clearFilesMissingList();

    // Animation related functions
    
    bool canAnim( const BW::string& modelPath );
    bool hasAnims( const BW::string& modelPath );
        
    StringPair createAnim( const StringPair& animID, const BW::string& animPath );
    void changeAnim( const StringPair& animID, const BW::string& animPath );
    void removeAnim( const StringPair& animID );
    void cleanAnim( const StringPair& animID );

    Moo::AnimationPtr getMooAnim( const StringPair& animID );
    Moo::AnimationPtr restoreChannels( const StringPair& animID );
    Moo::AnimationPtr backupChannels( const StringPair& animID );
    
    void setAnim( size_t pageID, const StringPair& animID );
    const StringPair& getAnim( size_t pageID ) const;
    void stopAnim( size_t pageID );

    bool animName( const StringPair& animID, const BW::string& animName );
    BW::string animName( const StringPair& animID );

    void firstFrame( const StringPair& animID, int frame );
    int firstFrame( const StringPair& animID );

    void lastFrame( const StringPair& animID, int frame );
    int lastFrame( const StringPair& animID );

    void localFrameRate( const StringPair& animID, float rate, bool final = false );
    float localFrameRate( const StringPair& animID );
    void lastFrameRate( float rate ) { lastFrameRate_ = rate; }
    
    void saveFrameRate( const StringPair& animID );
    float frameRate( const StringPair& animID );

    void frameNum( const StringPair& animID, int num );
    int frameNum( const StringPair& animID );

    int numFrames( const StringPair& animID );

    void animFile( const StringPair& animID, const BW::string& animFile );
    BW::string animFile( const StringPair& animID );

    void animBoneWeight( const StringPair& animID, const BW::string& boneName, float val );
    float animBoneWeight( const StringPair& animID, const BW::string& boneName );

    void removeAnimNode( const StringPair& animID, const BW::string& boneName );

    bool isReadOnlyAnim( const StringPair& animID );

    void playAnim( bool play );
    bool playAnim() { return playing_; }

    void loopAnim( bool loop) { looping_ = loop; }
    bool loopAnim() { return looping_; }

    // Action related functions

    bool hasActs( const BW::string& modelPath );
    
    StringPair createAct( const StringPair& actID, const BW::string& animName, const StringPair& afterAct );
    void removeAct( const StringPair& actID );

    void swapActions( const BW::string& what, const StringPair& actID, const StringPair& act2ID, bool reload = true );
    
    void setAct ( const StringPair& actID );
    const StringPair& getAct() const    {   return currAct_;    }
    void stopAct();

    void setupActionMatch( SuperModelActionPtr action );
    bool alreadyLooping( const BW::vector< SuperModelActionPtr >& actionsInUse, SuperModelActionPtr action );
    void takeOverOldAction( const BW::vector< SuperModelActionPtr >& actionsInUse, SuperModelActionPtr action );

    BW::string actName( const StringPair& actID );
    bool actName( const StringPair& actID, const BW::string& actName );

    BW::string actAnim( const StringPair& actID );
    void actAnim( const StringPair& actID, const BW::string& animName );

    float actBlendTime( const StringPair& actID, const BW::string& fieldName );
    void actBlendTime( const StringPair& actID, const BW::string& fieldName, float val );

    bool actFlag( const StringPair& actID, const BW::string& flagName );
    void actFlag( const StringPair& actID, const BW::string& flagName, bool flagVal );

    int actTrack( const StringPair& actID );
    void actTrack( const StringPair& actID, int track );

    float actMatchFloat( const StringPair& actID, const BW::string& typeName, const BW::string& flagName, bool& valSet );
    bool actMatchVal( const StringPair& actID, const BW::string& typeName, const BW::string& flagName, bool empty, float val, bool& setUndo );
    
    BW::string actMatchCaps( const StringPair& actID, const BW::string& typeName, const BW::string& capsType );
    void actMatchCaps( const StringPair& actID, const BW::string& typeName, const BW::string& capsType, const BW::string& newCaps );

    void actMatchFlag( const StringPair& actID, const BW::string& flagName, bool flagVal );

    // LOD functions
    
    float lodExtent( const BW::string& modelFile );
    void lodExtent( const BW::string& modelFile, float extent );

    void lodParents( BW::string modelName, BW::vector< BW::string >& parents );
    bool hasParent( const BW::string& modelName );

    bool isHidden( const BW::string& modelFile );
        
    BW::string lodParent( const BW::string& modelFile );
    void lodParent( const BW::string& modelFile, const BW::string& parent );

    void lodList( LODList* newList );

    void virtualDist( float dist = -1.f );

    // Material functions

    BW::string materialDisplayName( const BW::string& materialName );
    
    void setDye( const BW::string& matterName, const BW::string& tintName, Moo::ComplexEffectMaterialPtr & material  );

    void getMaterial( const BW::string& materialName, ComplexEffectMaterialSet & material );

    BW::string getTintName( const BW::string& matterName );

    bool setMaterialProperty( const BW::string& materialName, const BW::string& descName, const BW::string& uiName, const BW::string& propType, const BW::string& val );
    
    void instantiateMFM( DataSectionPtr data );
    void overloadMFM( DataSectionPtr data, DataSectionPtr mfmData );

    void cleanMaterials();
    void cleanTints();
    void cleanMaterialNames();
        
    bool setTintProperty( const BW::string& matterName, const BW::string& tintName, const BW::string& descName, const BW::string& uiName, const BW::string& propType, const BW::string& val );

    bool materialName( const BW::string& materialName, const BW::string& new_name );

    bool matterName( const BW::string& matterName, const BW::string& new_name, bool undoRedo = true );

    bool tintName( const BW::string& matterName, const BW::string& tintName, const BW::string& new_name, bool undoRedo = true );

    BW::string newTint( const BW::string& materialName, const BW::string& matterName, const BW::string& oldTintName, const BW::string& newTintName, const BW::string& fxFile, const BW::string& mfmFile );
    
    bool saveMFM( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& mfmFile );
    
    void deleteTint( const BW::string& matterName, const BW::string& tintName );
    
    bool ensureShaderCorrect( const BW::string& fxFile, const BW::string& format, bool dualUV );
        
    bool effectHasNormalMap( const BW::string& effectFile );
    bool doAnyEffectsHaveNormalMap();
    
    bool isSkyBox() const { return isSkyBox_; }
    bool effectIsSkybox( const BW::string& effectFile ) const;
    void checkMaterials();

    bool materialShader( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& fxFile, bool undoable = true );
    BW::string materialShader( const BW::string& materialName, const BW::string& matterName = "", const BW::string& tintName = "" );

    bool materialMFM( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& mfmFile, BW::string* fxFile /* = NULL */ );

    void tintFlag( const BW::string& matterName, const BW::string& tintName, const BW::string& flagName, uint32 val );
    uint32 tintFlag( const BW::string& matterName, const BW::string& tintName, const BW::string& flagName );

    void materialFlag( const BW::string& materialName, const BW::string& flagName, uint32 val );
    uint32 materialFlag( const BW::string& materialName, const BW::string& flagName );

    void tintNames( const BW::string& matterName, BW::vector< BW::string >& names );

    int modelMaterial () const;
    void modelMaterial ( int id );

    BW::string materialPropertyVal( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& propName, const BW::string& dataType );
    BW::string materialTextureFeedName( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& propName );
    void changeMaterialFeed( DataSectionPtr data, const BW::string& propName, const BW::string& feedName );
    void materialTextureFeedName( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& propName, const BW::string& feedName );

    BW::string exposedToScriptName( const BW::string& matterName, const BW::string& tintName, const BW::string& propName );
    void toggleExposed( const BW::string& matterName, const BW::string& tintName, const BW::string& propName );

    Vector4 getExposedVector4( const BW::string& matterName, const BW::string& tintName, const BW::string& descName, const BW::string& propType, const BW::string& val );

    uint32 recalcTextureMemUsage();

    Moo::EffectMaterialPtr getEffectForTint(    const BW::string& matterName, const BW::string& tintName, const BW::string& materialName );

    void updateTintFromEffect( const BW::string& matterName, const BW::string& tintName );

    void edit( GeneralEditor& editor );
    void updateModificationInfo();

    void canCaptureModel( bool val );
    bool canCaptureModel() const { return canCaptureModel_; }

    bool captureModel( const BW::string& fileName ) const;

#ifdef ENABLE_ASSET_PIPE
    void setAssetClient( AssetClient* assetClient ) { assetClient_ = assetClient; }
#endif

private:
    BW::string modelName_;
    BW::string visualName_;
    BW::string primitivesName_;

    BW::string editorProxyName_;

    DataSectionPtr extraModels_;

    Moo::EffectMaterialPtr pPortalMat_;

    unsigned numNodes_;
    
    bool texMemDirty_;
    uint32 texMem_;
    
    SuperModel* superModel_;
    SuperModel* editorProxySuperModel_;
    ActionQueue actionQueue_;
    MatrixLiaisonIdentity* matrixLI_;

private:
    BoundingBox modelBB_;
    BoundingBox visibilityBB_;

    // length of the vertex normals
    int normalsLength_;

    // for drawing the custom hull
    BW::vector< BW::vector<Vector3> > customHullPoints_;

    // for drawing the bsp
    BW::vector<Moo::VertexXYZL> verts_;

    DataSectionPtr currModel_;
    DataSectionPtr currVisual_;

    BW::map < BW::string , DataSectionPtr > modelHist_;

    BW::map < BW::string , DataSectionPtr > models_;
    BW::map < DataSectionPtr , BW::string > dataFiles_;

    BW::map < StringPair , AnimationInfo > animations_;
    BW::map < StringPair , ActionInfo > actions_;

    Dyes currDyes_;

    BW::map< BW::string, MaterialInfo > materials_;
    BW::map< BW::string, DataSectionPtr > dyes_;

    typedef BW::map< BW::string, BW::map < BW::string, TintInfo > > TintMap;
    TintMap tints_;

    BW::map< BW::string, BW::string > tintedMaterials_;

    BW::vector< DataSectionPtr > materialNameDataToRemove_;
    
    TreeRoot animList_;
    TreeRoot actList_;
    LODList  lodList_;
    TreeRoot materialList_;

    typedef BW::map< size_t, StringPair >::iterator AnimIt;
        
    bool animMode_;
    bool clearAnim_;
    BW::map< size_t, StringPair > currAnims_;
    StringPair currAct_;

    MaterialFashionVector materialFashions_;
    TransformFashionVector transformFashions_;

    bool playing_;
    bool looping_;

    float lastFrameRate_;
    
    BW::set< DataSectionPtr > dirty_;

    BW::map< BW::string, int > updateCount_;

    float virtualDist_;

    bool groundModel_;
    bool centreModel_;

    bool hasRootNode_;
    bool foundRootNode_;
    Vector3 initialRootNode_;

    bool visibilityBoxDirty_;

    typedef BW::vector< SmartPointer < ModelChangeCallback > > ModelChangeCallbackVect;
    ModelChangeCallbackVect modelChangeCallbacks_;

    bool isReadOnly_;

    bool isSkyBox_;
    
    static BW::vector< BW::string > s_missingFiles_;

    uint32 materialSectionTextureMemUsage( DataSectionPtr data, BW::set< BW::string >& texturesDone );

    bool executeModelChangeCallbacks();

    void onModelLoaded( bool reload );

    MetaData::MetaData metaData_;

    bool canCaptureModel_;
    int canCaptureModelCount_;

#ifdef ENABLE_ASSET_PIPE
    AssetClient* assetClient_;
#endif
};

class MutantShadowRender : public Moo::IShadowRenderItem
{
public:
    MutantShadowRender() 
        : mutant_( NULL )
        , time_( 0.0f )
        , states_( 0 )
    {
    }

    void addToScene();
    void update( Mutant* mutant, float time, int states );
    virtual void render( Moo::DrawContext& drawContext );
    virtual void release() {}

private:
    Mutant* mutant_;
    float   time_;
    int     states_;
};

BW_END_NAMESPACE

#endif // MUTANT_HPP
