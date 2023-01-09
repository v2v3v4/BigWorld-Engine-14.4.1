#include "pch.hpp"
#include "editor_chunk_deferred_decal.hpp"
#include "resmgr/resource_cache.hpp"
#include "model/super_model.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/editor/item_properties.hpp"
#include "moo/geometrics.hpp"
#include "math/oriented_bbox.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "resmgr/auto_config.hpp"

BW_BEGIN_NAMESPACE

static AutoConfigString s_chunkDeferredDecalNotFoundModel( "system/notFoundModel" );

//-- Note: Another example of the BigWorld's macro magic.
#undef  IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk)

//-- implement chunk item type factory.
IMPLEMENT_CHUNK_ITEM(EditorStaticChunkDecal, staticDecal, 1)

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //--
    const float g_defaultDecalSize = 20.0f;
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


//--------------------------------------------------------------------------------------------------
EditorStaticChunkDecal::EditorStaticChunkDecal() : map1Changed(false)
{
    wantFlags_  = WantFlags(wantFlags_ | WANTS_UPDATE);
}

//--------------------------------------------------------------------------------------------------
EditorStaticChunkDecal::~EditorStaticChunkDecal()
{

}

//--------------------------------------------------------------------------------------------------
/*bool EditorStaticChunkDecal::needsUpdate( uint32 frameTimestamp ) const
{
    BW_GUARD;

    if (edHidden() || !isLoaded())
    {
        return false;
    }

    return EditorChunkSubstance<StaticChunkDecal>::needsUpdate(
        frameTimestamp );
}*/


void EditorStaticChunkDecal::updateAnimations( /*uint32 frameTimestamp */)
{
    BW_GUARD;

    if (edHidden() || !isLoaded())
    {
        return;
    }

    return EditorChunkSubstance<StaticChunkDecal>::updateAnimations(
        /*frameTimestamp*/ );
}


void EditorStaticChunkDecal::draw( Moo::DrawContext& drawContext )
{
    BW_GUARD;

    if (WorldManager::instance().drawSelection())
    {
        return;
    }

    if (edHidden())
        return;

    if (!isLoaded() || !DecalsManager::instance().getTexturesValid( m_decal ))
    {
        Matrix translate;
        translate.setTranslate(Vector3(0,-0.5f,0));

        Moo::rc().push();
        Moo::rc().preMultiply(edTransform());
        Moo::rc().preMultiply(translate);
        m_missingDecalModel->dress( Moo::rc().world() );
        m_missingDecalModel->draw( drawContext, true);
        Moo::rc().pop();

        return;
    }

    //-- ToDo (b_sviglo): Ugly solution. Move to the decals options panel.
    bool drawOverdraw       = (Options::getOptionInt( "render/decals/showOverdraw", 0 ) != 0);
    bool showTextureAtlas   = (Options::getOptionInt( "render/decals/showTextureAtlas", 0 ) != 0);

    //--
    if(drawOverdraw)
        DecalsManager::instance().enableDebugDraw(DecalsManager::DEBUG_OVERDRAW);
    else 
        DecalsManager::instance().disableDebugDraw();

    //--
    DecalsManager::instance().showStaticDecalsAtlas(showTextureAtlas);

    EditorChunkSubstance<StaticChunkDecal>::draw( drawContext );
    if (edIsSelected())
    {
        Vector3 dir = m_decalDesc.m_transform.applyToUnitAxisVector(2);
        dir.normalise();
        Vector3 pos = pChunk_->transform().applyPoint( 
            m_decalDesc.m_transform.applyToOrigin() );
        Geometrics::addLineWorld( pos, pos - dir * 3.0f, 0xffff0000 );
    }
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::load(DataSectionPtr iData)
{
    BW_GUARD;

    m_model             = Model::get("system/models/fx_unite_cube.model");
    m_missingDecalModel = Model::get(s_chunkDeferredDecalNotFoundModel.value());

    ResourceCache::instance().addResource(m_model);
    ResourceCache::instance().addResource(m_missingDecalModel);
    
    if ( !EditorChunkSubstance<StaticChunkDecal>::load(iData) )
    {
        bool map1IsBlank = m_decalDesc.m_map1.length() == 0;
        bool map2IsBlank = m_decalDesc.m_map2.length() == 0;
        bool mapsNotProvided =
            m_decalDesc.m_materialType == DecalsManager::Decal::MATERIAL_DIFFUSE &&
            map1IsBlank || map1IsBlank && map2IsBlank;

        if (mapsNotProvided)
        {
            WorldManager::instance().addError( this->chunk(), this,
                "[DECAL] Decal not loaded: No map was specified.");
        }
        else
        {
            BW::string map1 = map1IsBlank ? "Unspecified" : m_decalDesc.m_map1;
            BW::string map2 = map2IsBlank ? "Unspecified" : m_decalDesc.m_map2;
            BW::string maps =
                m_decalDesc.m_materialType == DecalsManager::Decal::MATERIAL_DIFFUSE ?
                map1 : map1 + ", " + map2;
            WorldManager::instance().addError( this->chunk(), this,
                "[DECAL] Decal not loaded: %s", maps.c_str());
        }
    }
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::load(DataSectionPtr iData, Chunk* /*pChunk*/)
{
    BW_GUARD;

    load(iData);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::reload()
{
    BW_GUARD;

    //-- remove old static decal.
    if (isLoaded())
    {
        DecalsManager::instance().removeStaticDecal(m_decal);
        m_decal = -1;
    }

    //-- load a new one.
    load(pOwnSect_);

    //-- change aspect ratio of decal based on the map1 texture aspect ratio.
    if (isLoaded() && map1Changed)
    {
        Matrix& transform = m_decalDesc.m_transform;

        float aspectRatio = DecalsManager::instance().getMap1AspectRatio(m_decal);

        Vector3 scale(1,1,1);
        scale.x = transform[1].length() * aspectRatio / transform[0].length();

        Matrix m;
        m.setScale(scale);
        transform.preMultiply(m);
    }
    map1Changed = false;

    //-- update.
    update(m_decalDesc.m_transform);
    syncInit();

    return true;
}

//--------------------------------------------------------------------------------------------------
const Matrix& EditorStaticChunkDecal::edTransform()
{
    BW_GUARD;

    return m_decalDesc.m_transform;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::edTransform(const Matrix& transform, bool transient)
{
    BW_GUARD;

    //-- if this is only a temporary change, keep it in the same chunk.
    if (transient)
    {
        update(transform);
        syncInit();
        return true;
    }

    //-- it's permanent, so find out where we belong now.
    Chunk* pOldChunk = pChunk_;
    Chunk* pNewChunk = edDropChunk(transform.applyToOrigin());

    if (pNewChunk == NULL)
        return false;

    //-- make sure the chunks aren't read-only.
    if (!EditorChunkCache::instance(*pOldChunk).edIsWriteable() || !EditorChunkCache::instance(*pNewChunk).edIsWriteable())
        return false;

    //-- ok, accept the transform change then
    m_decalDesc.m_transform.multiply(transform, pOldChunk->transform());
    m_decalDesc.m_transform.postMultiply(pNewChunk->transformInverse());

    if(Options::getOptionInt( "render/decals/autoSize", 0 ) != 0)
        minimizeBoxSize();

    //-- note that both affected chunks have seen changes.
    WorldManager::instance().changedChunk(pOldChunk);
    WorldManager::instance().changedChunk(pNewChunk);

    //-- and move ourselves into the right chunk. we have to do this even if it's the same chunk so
    //-- the col scene gets recreated.
    pOldChunk->delStaticItem(this);
    pNewChunk->addStaticItem(this);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::edEdit(GeneralEditor& editor)
{
    BW_GUARD;

    if (edFrozen() || !edCommonEdit(editor))
        return false;

    MatrixProxyPtr pMP(new ChunkItemMatrix(this));

    //-- position, rotation and scale properties.
    //------------------------------------------------------------------------------------------
    editor.addProperty(new ChunkItemPositionProperty(
        LocaliseStaticUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_DECAL/POSITION"), pMP, this
        ));
    editor.addProperty(new GenRotationProperty(
        LocaliseStaticUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_DECAL/ROTATION"), pMP
        ));
    editor.addProperty(new GenScaleProperty(
        LocaliseStaticUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_DECAL/SCALE"), pMP
        ));

    //-- material type property.
    //------------------------------------------------------------------------------------------
    {
        DataSectionPtr materialTypeDS = new XMLSection("materialType");

        materialTypeDS->writeInt("DIFFUSE",     0);
        materialTypeDS->writeInt("BUMP",        1);
        materialTypeDS->writeInt("PARALLAX",    2);
        materialTypeDS->writeInt("ENV_MAPPING", 3);

        ChoiceProperty* materialType = new ChoiceProperty(
            LocaliseUTF8("material type"),
            new AccessorDataProxy< EditorStaticChunkDecal, UIntProxy >(
            this, "material type",
            &EditorStaticChunkDecal::getMaterialType,
            &EditorStaticChunkDecal::setMaterialType),
            materialTypeDS
            );
        editor.addProperty(materialType);
    }

    //-- influence type property.
    //------------------------------------------------------------------------------------------
    {
        editor.addProperty(new GenBoolProperty(
            LocaliseStaticUTF8(L"apply to terrain"),
            new AccessorDataProxy< EditorStaticChunkDecal, BoolProxy >(
            this, "apply to terrain",
            &EditorStaticChunkDecal::getApplyTo<DecalsManager::Decal::APPLY_TO_TERRAIN>,
            &EditorStaticChunkDecal::setApplyTo<DecalsManager::Decal::APPLY_TO_TERRAIN>
            )));

        editor.addProperty(new GenBoolProperty(
            LocaliseStaticUTF8(L"apply to tree"),
            new AccessorDataProxy< EditorStaticChunkDecal, BoolProxy >(
            this, "apply to tree",
            &EditorStaticChunkDecal::getApplyTo<DecalsManager::Decal::APPLY_TO_TREE>,
            &EditorStaticChunkDecal::setApplyTo<DecalsManager::Decal::APPLY_TO_TREE>
            )));

        editor.addProperty(new GenBoolProperty(
            LocaliseStaticUTF8(L"apply to dynamic"),
            new AccessorDataProxy< EditorStaticChunkDecal, BoolProxy >(
            this, "apply to dynamic",
            &EditorStaticChunkDecal::getApplyTo<DecalsManager::Decal::APPLY_TO_DYNAMIC>,
            &EditorStaticChunkDecal::setApplyTo<DecalsManager::Decal::APPLY_TO_DYNAMIC>
            )));

        editor.addProperty(new GenBoolProperty(
            LocaliseStaticUTF8(L"apply to static"),
            new AccessorDataProxy< EditorStaticChunkDecal, BoolProxy >(
            this, "apply to static",
            &EditorStaticChunkDecal::getApplyTo<DecalsManager::Decal::APPLY_TO_STATIC>,
            &EditorStaticChunkDecal::setApplyTo<DecalsManager::Decal::APPLY_TO_STATIC>
            )));

        editor.addProperty(new GenBoolProperty(
            LocaliseStaticUTF8(L"apply to flora"),
            new AccessorDataProxy< EditorStaticChunkDecal, BoolProxy >(
            this, "apply to flora",
            &EditorStaticChunkDecal::getApplyTo<DecalsManager::Decal::APPLY_TO_FLORA>,
            &EditorStaticChunkDecal::setApplyTo<DecalsManager::Decal::APPLY_TO_FLORA>
            )));
    }

    editor.addProperty(new GenUIntProperty(
        LocaliseStaticUTF8(L"priority"),
        new AccessorDataProxy< EditorStaticChunkDecal, UIntProxy >(
            this, "priority",
            &EditorStaticChunkDecal::getPriority,
            &EditorStaticChunkDecal::setPriority
        )));

    TextProperty* diffTexProp = new TextProperty(
        LocaliseStaticUTF8(L"map #1"),
        new SlowPropReloadingProxy<EditorStaticChunkDecal, StringProxy>(
            this, "map1", 
            &EditorStaticChunkDecal::getMap1, 
            &EditorStaticChunkDecal::setMap1
            )
        );
    diffTexProp->fileFilter(Name("Texture files(*.dds)|*.dds||"));
    editor.addProperty(diffTexProp);

    TextProperty* bumpTexProp = new TextProperty(
        LocaliseStaticUTF8(L"map #2"),
        new SlowPropReloadingProxy<EditorStaticChunkDecal, StringProxy>(
            this, "map2", 
            &EditorStaticChunkDecal::getMap2, 
            &EditorStaticChunkDecal::setMap2
            )
        );
    bumpTexProp->fileFilter(Name("Texture files(*.dds)|*.dds||"));
    editor.addProperty(bumpTexProp);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::edSave(DataSectionPtr oData)
{
    BW_GUARD;

    if (!edCommonSave(oData))
        return false;
    
    oData->writeMatrix34("transform",   m_decalDesc.m_transform);
    oData->writeUInt    ("priority",    m_decalDesc.m_priority);
    oData->writeUInt    ("type",        m_decalDesc.m_materialType);
    oData->writeUInt    ("influence",   m_decalDesc.m_influenceType);
    oData->writeString  ("diffTex",     m_decalDesc.m_map1);
    oData->writeString  ("bumpTex",     m_decalDesc.m_map2);

    return true;
}

//--------------------------------------------------------------------------------------------------
const char* EditorStaticChunkDecal::sectName() const
{
    BW_GUARD;

    return "staticDecal";
}


bool EditorStaticChunkDecal::isDrawFlagVisible() const
{
    return true;
}


//--------------------------------------------------------------------------------------------------
const char* EditorStaticChunkDecal::drawFlag() const
{
    BW_GUARD;

    return "render/decals/drawChunkStaticDeferredDecals";
}

//--------------------------------------------------------------------------------------------------
ModelPtr EditorStaticChunkDecal::reprModel() const
{
    BW_GUARD;

    return m_model; 
}

//--------------------------------------------------------------------------------------------------
void EditorStaticChunkDecal::addAsObstacle()
{
    BW_GUARD;

    Matrix world(pChunk_->transform());
    world.preMultiply(this->edTransform());

    if (ModelPtr model = this->reprModel())
    {
        ChunkModelObstacle::instance(*pChunk_).addModel(model, world, this, true);
    }
}

//--------------------------------------------------------------------------------------------------
void EditorStaticChunkDecal::updateDecalProps()
{
    BW_GUARD;

    if (isLoaded())
    {
        DecalsManager::Decal::Desc desc = m_decalDesc;
        desc.m_transform.postMultiply(pChunk_->transform());

        DecalsManager::instance().updateStaticDecal(m_decal, desc);
    }
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::setGenericApplyTo(uint type, bool enable)
{
    BW_GUARD;

    if (enable) m_decalDesc.m_influenceType |= type;
    else        m_decalDesc.m_influenceType &= ~type;

    updateDecalProps();
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::getGenericApplyTo(uint32 type) const
{
    BW_GUARD;

    return (m_decalDesc.m_influenceType & type) != 0;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::setMaterialType(const uint& material)
{
    BW_GUARD;

    m_decalDesc.m_materialType = (DecalsManager::Decal::EMaterialType)material;

    updateDecalProps();
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EditorStaticChunkDecal::setPriority(const uint& priority)
{
    BW_GUARD;

    m_decalDesc.m_priority = priority;

    updateDecalProps();
    return true;
}

//--------------------------------------------------------------------------------------------------
void EditorStaticChunkDecal::setMap1(const BW::string& texture)
{
    BW_GUARD;

    m_decalDesc.m_map1 = texture;
    map1Changed = true;
}

//--------------------------------------------------------------------------------------------------
void EditorStaticChunkDecal::setMap2(const BW::string& texture)
{
    BW_GUARD;

    m_decalDesc.m_map2 = texture;
}

//--------------------------------------------------------------------------------------------------
void EditorStaticChunkDecal::edGetTexturesAndVertices( BW::map<void*, int>& texturesInfo, BW::map<void*, int>& verticesInfo )
{
    BW_GUARD;

    if (!isLoaded())
        return;

    DecalsManager::instance().getTexturesUsed(m_decal, texturesInfo);
}

//--------------------------------------------------------------------------------------------------
int EditorStaticChunkDecal::edTextureMemory() const
{
    BW_GUARD;

    if (!isLoaded())
        return 0;

    typedef BW::map<void*, int> IDMap;
    IDMap textures;
    DecalsManager::instance().getTexturesUsed(m_decal, textures);
    int texMemory = 0;
    for(IDMap::iterator it = textures.begin(); it != textures.end(); ++it)
    {
        texMemory += it->second;
    }
    return texMemory;
}

//--------------------------------------------------------------------------------------------------
int EditorStaticChunkDecal::edVertexBufferMemory() const
{
    BW_GUARD;

    if (!isLoaded())
        return 0;

    typedef BW::map<void*, int> IDMap;
    IDMap vertices;
    DecalsManager::instance().getVerticesUsed(vertices);
    int vertMemory = 0;
    for(IDMap::iterator it = vertices.begin(); it != vertices.end(); ++it)
    {
        vertMemory += it->second;
    }
    return vertMemory;
}

//--------------------------------------------------------------------------------------------------
void GetMinDist(Chunk* pChunk, const Vector3& from, const Vector3& dir, float& prevMinDist, float& prevMaxDist)
{
    BW_GUARD;

    Vector3 pos = pChunk->transform().applyPoint(from);

    Vector3 groundPos = 
        Snap::toGround(pos, dir, 20000.f);

    float dist = (groundPos - pos).length();
    prevMinDist = min(prevMinDist, dist);
    if(dist > 0.0f && dist < 20000.0f)
        prevMaxDist = max(prevMaxDist,  dist);
}

//--------------------------------------------------------------------------------------------------
void EditorStaticChunkDecal::minimizeBoxSize()
{
    BW_GUARD;

    if (!isLoaded())
        return;

    Chunk* pChunk = chunk();
    if ( !pChunk )
        return;

    Vector3 dir = m_decalDesc.m_transform.applyToUnitAxisVector(2);
    float oldYSize = dir.length();
    dir.normalise();
    Vector3 extent = dir * g_defaultDecalSize;
    Vector3 pos = m_decalDesc.m_transform.applyToOrigin();

    BoundingBox bbox;
    bbox.setBounds(Vector3(-0.5f, -0.5f, -0.5f), Vector3(+0.5f, +0.5f, +0.5f));

    Matrix m;
    m.setScale(1.0f, 1.0f, g_defaultDecalSize / oldYSize );
    m.postMultiply(m_decalDesc.m_transform);
    Math::OrientedBBox obb(bbox, m);

    //////////////////////////////////////////////////////////////////////////
    // Direction Up-Down
    //check center and corners
    float minDownDist = 10000.0f;
    float maxDownDist = -1.0f;
    GetMinDist(pChunk, pos - extent*0.5f, dir, minDownDist, maxDownDist);
    Vector3 p = obb.point(0);
    GetMinDist(pChunk, p, dir, minDownDist, maxDownDist);
    p = obb.point(3);
    GetMinDist(pChunk, p, dir, minDownDist, maxDownDist);
    p = obb.point(4);
    GetMinDist(pChunk, p, dir, minDownDist, maxDownDist);
    p = obb.point(7);
    GetMinDist(pChunk, p, dir, minDownDist, maxDownDist);
    //////////////////////////////////////////////////////////////////////////

    float newYSize = maxDownDist - minDownDist + 0.1f;

    if(newYSize <= 0.0f)
        return;

    //pos = pos + dir * ((minDownDist - g_defaultDecalSize + maxDownDist) * 0.5f);

    //m_decalDesc.m_transform.translation(pos);
    Matrix mScale;
    mScale.setScale(1.0f, 1.0f, newYSize / oldYSize);
    m_decalDesc.m_transform.preMultiply(mScale);
}

BW::string EditorStaticChunkDecal::edFilePath() const
{
    BW_GUARD;

    return getMap1();
}

BW_END_NAMESPACE
