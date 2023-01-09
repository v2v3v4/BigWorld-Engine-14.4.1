#include "pch.hpp"
#include "cstdmf/stdmf.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_model.hpp"
#include "managed_texture.hpp"
#include "sys_mem_texture.hpp"
#include "resmgr/bin_section.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "physics2/bsp.hpp"
#include "physics2/worldtri.hpp" // For WorldTriangle::Flags
#include "terrain/terrain_collision_callback.hpp"

#include "sticker_decal.hpp"
#include "material_kinds_constants.hpp"
#include "deferred_decals_manager.hpp"
#include "renderer.hpp"
#include "forward_decal.hpp"

#include "duplo/pymodel.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PyStickerModel )

PY_BEGIN_METHODS( PyStickerModel )
PY_METHOD( addSticker )
PY_METHOD( delSticker )
PY_METHOD( clear )
PY_METHOD( setLODDistances )
PY_METHOD( setAlphas )
PY_METHOD( setTextureData )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyStickerModel )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PyStickerModel, "StickerModel", BigWorld )

bool PyStickerModel::s_enabledDrawing = true;
bool PyStickerModel::s_bFirstInit = true;
float PyStickerModel::s_criticalAngle = 20.0f;

class StickerDecalTrianglesCollector : public CollisionCallback
{
public:
    BW::vector<WorldTriangle> tr_;
    StickerDecalTrianglesCollector() {}
private:
    virtual int operator()( const CollisionObstacle & obstacle,
        const WorldTriangle & triangle, float dist )
    {
        BW_GUARD;
        if(triangle.flags() & BW_TRIANGLE_NOCOLLIDE) 
            return COLLIDE_ALL;

        tr_.push_back(WorldTriangle(
            obstacle.transform().applyPoint( triangle.v0() ),
            obstacle.transform().applyPoint( triangle.v1() ),
            obstacle.transform().applyPoint( triangle.v2() ),
            triangle.flags() ));
        return COLLIDE_ALL;
    }
};

bool lineRayTriangleIntersect( const Vector3 &p0, const Vector3 &dir, const Vector3 &v0, const Vector3 &v1, const Vector3 &v2, float& prevR, Vector3 &I)
{
    Vector3 w0, w,u,v,n; // ray vectors
    float r, a, b; // params to calc ray-plane intersect
    float uu, uv, vv, wu, wv, D;
    float s, t;
    const float SMALL_NUM=0.00001f;
    // get triangle edge vectors and plane normal
    u = v1-v0;
    v = v2-v0;
    n = u.crossProduct(v);
    if (n.lengthSquared() < SMALL_NUM) return false;
    //dir = p1-p0;         // ray direction vector
    w0 = p0-v0;
    a = -n.dotProduct(w0);
    b = n.dotProduct(dir);
    if ( fabsf( b ) < SMALL_NUM )
    {// ray is parallel to triangle plane
        if ( fabsf( a ) < SMALL_NUM )
        {// ray lies in triangle plane
            return false;
        } else
        {
            return false;
        }// ray disjoint from plane
    }
// get intersect point of ray with triangle plane
    r = a / b;

    if(r >= prevR)
        return false;

    if (r < 0.0f/* || r > 1.0f*/) 
    {// ray goes away from triangle// for a segment, also test if (r > 1.0) => no intersect
        return false;              // => no intersect
    }
    I = p0+dir*r;// intersect point of ray and plane
// is I inside T?
    uu = u.dotProduct(u);
    uv = u.dotProduct(v);
    vv = v.dotProduct(v);
    w = I-v0;
    wu = w.dotProduct(u);
    wv = w.dotProduct(v);
    D = uv * uv - uu * vv;
    // get and test parametric coords
    s = (uv * wv - vv * wu) / D;
    if (s < 0.0 || s > 1.0) return false;// I is outside T
    t = (uv * wu - uu * wv) / D;
    if (t < 0.0 || (s + t) > 1.0) return false;// I is outside T
    prevR = r;
    return true;                  // I is in T
}

float findClosestTriangleInsideWorldBox(const Vector3& start, const Vector3& dir, 
                                       Vector3 *worldV, Vector3& point, Vector3& normal, const PyModel* model)
{
    BW_GUARD;

    const BSPTree* bsp = model->pSuperModel()->topModel(0)->decompose();
    if (bsp == NULL)
        return -1;

    BSPFlagsMap bspMap;
    bspMap.push_back( TRIANGLE_DOUBLESIDED);
    const_cast<BSPTree*>(bsp)->remapFlags( bspMap );
    Matrix wT = model->worldTransform();
    BoundingBox BB(Vector3(-1e10,-1e10,-1e10),Vector3(1e10,1e10,1e10));
    ChunkBSPObstacle *l_obstacle = new ChunkBSPObstacle(bsp,wT,&BB,NULL);

    StickerDecalTrianglesCollector collector;
    CollisionState state(collector);
    Matrix wTi;
    wTi.invert(wT);

    WorldTriangle tri1( wTi.applyPoint(worldV[0]), wTi.applyPoint(worldV[1]), wTi.applyPoint(worldV[3]) );
    WorldTriangle tri2( wTi.applyPoint(worldV[1]), wTi.applyPoint(worldV[2]), wTi.applyPoint(worldV[3]) );
    Vector3 extent = (wTi.applyPoint(worldV[4])-wTi.applyPoint(worldV[0]));
    //collector.tris_.clear();
    //collector.normal_ = extent;
    //collector.normal_.normalise();
    //StickerDecalTrianglesCollector collector1;
    //CollisionState state1(collector1);
    l_obstacle->collide( tri1, tri1.v0()+extent, state );
    l_obstacle->collide( tri2, tri2.v0()+extent, state );
    float prevR = 100000000.0f;
    //l_obstacle->collide(start, dir*3.0f, state);
    for(BW::vector<WorldTriangle>::iterator it = collector.tr_.begin(); it != collector.tr_.end(); it++)
    {
        if(lineRayTriangleIntersect(start, dir, it->v0(), it->v1(), it->v2(), prevR, point))
        {
            normal = it->normal();
        }
    }
    
    delete l_obstacle;

    if(prevR == 100000000.0f)
        return -1.0f;
    return prevR;
}


int PyStickerModel::addSticker( BW::string texture, BW::string bumpTexture, const PyModelPtr& model_, 
                               Vector3 start_, Vector3 end_, Vector2 size, Vector3 up_, bool isInscription )
{
    BW_GUARD;
    return -1;

#if 0

    bool bUseSrcTex = texture.empty() && pSrcTex_.exists();
    if( bUseSrcTex )
    {
        static int s_nameIdx = 0;
        char name[64];
        sprintf_s(name, sizeof(name), "TempStickerTexture%d", ++s_nameIdx);
        texture = name;
    }

    //-- Forward rendering pipeline works
    if(pForwardModel)
    {
        return pForwardModel->addSticker(texture, model_, start_, end_, size, up_, bUseSrcTex ? pSrcTex_.get() : NULL, isInscription );
    }

    owner = model_.getObject();

    Matrix wTr = owner->worldTransform();
    Vector3 start = wTr.applyPoint(start_);
    Vector3 end = wTr.applyPoint(end_); 
    Vector3 up = wTr.applyVector(up_);

    DecalsManager::Decal::Desc desc;
    desc.m_materialType = (bumpTexture == "") ? DecalsManager::Decal::MATERIAL_DIFFUSE : DecalsManager::Decal::MATERIAL_BUMP;
    desc.m_influenceType = DecalsManager::Decal::APPLY_TO_STATIC | DecalsManager::Decal::APPLY_TO_DYNAMIC;
    desc.m_priority = 10;
    desc.m_map1 = texture;
    desc.m_map2 = bumpTexture;
    if( bUseSrcTex )
        desc.m_map1Src = pSrcTex_;

    // find collision ray direction.
    Vector2 halfSize = size * 0.5f;
    Vector3 direction = end - start;
    float depth = direction.length();
    direction.normalise();

    ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
    if ( !pSpace.exists() )
        return -1;

    Vector3 zAxis(end-start);
    zAxis.normalise();
    Vector3 xAxis(up.crossProduct(zAxis));
    xAxis.normalise();
    Vector3 yAxis(zAxis.crossProduct(xAxis));
    Vector3 hS(size.x / 2.0f, size.y / 2.0f, max(size.x, size.y)*0.5f);
    Vector3 extent = end-start;
    Vector3 worldBox[8];
    {
        worldBox[0] = start-xAxis*hS.x+yAxis*hS.y;
        worldBox[1] = start+xAxis*hS.x+yAxis*hS.y;
        worldBox[2] = start+xAxis*hS.x-yAxis*hS.y;
        worldBox[3] = start-xAxis*hS.x-yAxis*hS.y;
        worldBox[4] = worldBox[0]+extent;
        worldBox[5] = worldBox[1]+extent;
        worldBox[6] = worldBox[2]+extent;
        worldBox[7] = worldBox[3]+extent;
    }

    Vector3 p;
    Vector3 normal;
    float dist = findClosestTriangleInsideWorldBox(start, direction, worldBox, p, normal, owner);

    // find hit point.
    Vector3 pos = start + direction*dist;

    // find perpendicular vector with desired yaw angle to direction.
    //Vector3 up(s_closesttri.hit_.v1() - s_closesttri.hit_.v0());
    Vector3 hit_dir = normal * -1.0f;
    hit_dir.normalise();

    if (almostZero(1.0f - abs(hit_dir.dotProduct(up)), 0.01f))
        up = Vector3(up.z, up.x, up.y);

    Matrix wti;
    wti.invert( wTr );
    //pos = wti.applyPoint(pos);
    //hit_dir = wti.applyPoint(hit_dir);

    desc.m_transform.lookAt(pos, hit_dir, up);
    desc.m_transform.invert();
    Matrix scale;
    scale.setScale(size.x, size.y, depth);
    desc.m_transform.preMultiply(scale);
    desc.m_transform.postMultiply(wti);

    int32 idx = DecalsManager::instance().createStaticDecal(desc);
    if (idx != -1)
    {
        decals_.insert(std::make_pair(idx, StickerDecal(desc, idx)));
    }
    return idx;
#endif
}

PyStickerModel::PyStickerModel( PyTypePlus * pType /*= &s_type_ */ ) :
PyAttachment( pType ),
owner(NULL),
pForwardModel(NULL),
pSrcTex_(NULL),
updateTimestamp_(0)
{
    lodDistances[0] = lodDistances[1] = 1e10;
    alphas[0] = alphas[1] = 1;
    pForwardModel = new ForwardStickerModel();

    if(s_bFirstInit)
    {
        s_bFirstInit = false;
        MF_WATCH( "Visibility/Stickers", PyStickerModel::s_enabledDrawing, Watcher::WT_READ_WRITE,
            "Draw stickers on vehicles");
        MF_WATCH( "Render/decalCriticalAngle", PyStickerModel::s_criticalAngle, Watcher::WT_READ_WRITE,
            "Critical hit angle. When sticker direction-surface angle is less than this, it's normalized.");
    }
}

PyStickerModel * PyStickerModel::New( )
{
    BW_GUARD;
    return new PyStickerModel();
}

void PyStickerModel::tossed( bool outside )
{

}

void PyStickerModel::clear()
{
#if 0
    if(pForwardModel)
        pForwardModel->clear();
    else
    {
        for(BW::map<uint32, StickerDecal>::iterator it = decals_.begin(); it != decals_.end(); it++)
            DecalsManager::instance().removeStaticDecal(it->first);
        decals_.clear();
    }
    owner = NULL;
#endif
}

void PyStickerModel::delSticker( int stickerID )
{
#if 0   
    if(stickerID < 0)
    {   // TODO: it's an error case! MUST fix if appear!
        ERROR_MSG("PyStickerDecal: Invalid sticker index(%d) on delete", stickerID);
        return;
    }
    if(pForwardModel)
        pForwardModel->delSticker(stickerID);
    else
    {
        decals_.erase(stickerID);
        DecalsManager::instance().removeStaticDecal(stickerID);
        if(decals_.empty())
            owner = NULL;
    }
#endif
}

void PyStickerModel::setLODDistances(float texture0distance,float texture1distance)
{
    if(pForwardModel)
        pForwardModel->setLODDistances(texture0distance, texture1distance);
    else
    {
        lodDistances[0] = texture0distance;
        lodDistances[1] = texture1distance;
    }
}

void PyStickerModel::setAlphas(float texture0alpha,float texture1alpha)
{
    if(pForwardModel)
        pForwardModel->setAlphas(texture0alpha, texture1alpha);
    else
    {
        alphas[0] = texture0alpha;
        alphas[1] = texture1alpha;
    }
}

bool PyStickerModel::needsUpdate() const
{
    return pForwardModel != NULL && 
        s_enabledDrawing && 
        updateTimestamp_ != Moo::rc().frameTimestamp();
}

void PyStickerModel::updateAnimations( const Matrix & worldTransform, float lod )
{
    if(pForwardModel)
        pForwardModel->prepareDraw( lod );
    updateTimestamp_ = Moo::rc().frameTimestamp();
}

void PyStickerModel::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
#if 0
    if(!s_enabledDrawing || Moo::rc().reflectionScene())
        return;

    if (!pForwardModel && owner && !decals_.empty())
    {
        DecalsManager &dmi = DecalsManager::instance();
        const Matrix &wTr = owner->worldTransform();
        for(BW::map<uint32, StickerDecal>::iterator it = decals_.begin(); it != decals_.end(); it++)
        {
            dmi.updateStaticDecal(it->first, (Matrix&)(it->second.transform()*wTr));
            dmi.markVisible(it->first);
        }
    }
#endif
}

PyStickerModel::~PyStickerModel()
{
    if(pForwardModel)
        delete pForwardModel;
}

void PyStickerModel::tick( float dTime )
{
    if(pForwardModel)
        pForwardModel->tick(dTime);
}

void PyStickerModel::setTextureData( PyObjectPtr data )
{
    BW_GUARD;
    PyObject *pData = data.get();
    if (pData == NULL || !PyString_Check(pData))
    {
        PyErr_Format(PyExc_RuntimeError, "PyStickerModel::setTextureData - invalid parameter type of data (must be string).");
        return;
    }

    char *buf = NULL;
    Py_ssize_t size = 0;
    if (PyString_AsStringAndSize(pData, &buf, &size)
        || size == 0 || buf == NULL)
    {
        PyErr_Format(PyExc_RuntimeError, "PyStickerModel::setTextureData - failed to get texture contents (buffer size is %d).", size);
        return;
    }

    BinaryPtr binBlock = new BinaryBlock(buf, size, "");
    //TODO: asynchronous load?
    pSrcTex_ = new Moo::SysMemTexture(binBlock, D3DFMT_DXT3 );
    
    if (pSrcTex_->status() == Moo::BaseTexture::STATUS_FAILED_TO_LOAD)
    {
        PyErr_Format(PyExc_RuntimeError, "PyStickerModel::setTextureData - failed to load texture from input data, texture contents may be corrupt or incomplete.");
        pSrcTex_ = NULL;
        return;
    }
}

BW_END_NAMESPACE
