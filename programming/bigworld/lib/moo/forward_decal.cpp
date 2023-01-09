#include "pch.hpp"
#include "forward_decal.hpp"
#include "utils.hpp"
#include "renderer.hpp"
#include "sticker_decal.hpp"

#include "chunk\chunk_manager.hpp"
#include "chunk\chunk_obstacle.hpp"
#include "chunk\chunk_model.hpp"
#include "physics2\bsp.hpp"
#include "moo\managed_texture.hpp"
#include "moo\effect_visual_context.hpp"
#include "moo\vertex_format_cache.hpp"
#include "moo/render_event.hpp"
#include "resmgr\bin_section.hpp"
#include "moo\lod_settings.hpp"
#include "terrain\terrain_collision_callback.hpp"
#include "duplo/pymodel.hpp"

#define SAFE_DELETE(a) if((a)) { delete (a); (a) = NULL; }

BW_BEGIN_NAMESPACE

//-- Warning: size should be power of two value.
Vector2 g_decalAtlasSize(2048, 1024);
Vector2 g_inscriptionAtlasSize(2048, 1024);

//#define DEBUG_STICKERS

//-- use shortcuts.
class superDecalTrianglesCollector : public CollisionCallback
{
public:
    VectorNoDestructor<WorldTriangle> tris_;
    Vector3 normal_;
private:
    virtual int operator()( const CollisionObstacle & obstacle,
        const WorldTriangle & triangle, float dist )
    {
        BW_GUARD;
        if(triangle.flags() & BW_TRIANGLE_NOCOLLIDE) return COLLIDE_ALL;
        WorldTriangle wt(
            obstacle.transform().applyPoint( triangle.v0() ),
            obstacle.transform().applyPoint( triangle.v1() ),
            obstacle.transform().applyPoint( triangle.v2() ),
            triangle.flags() );
        Vector3 norm = triangle.normal();
        norm.normalise();
        const float fDot = normal_.dotProduct(norm);
        if ( fDot < -0.035f ) // ~93 degrees
        {
            // only add it if we don't already have it
            // (not sure why this happens, but it does)
            const size_t sz = tris_.size();
            uint i = 0;
            for (; i < sz; i++)
            {
                if ((tris_[i].v0() == wt.v0()) &&
                    (tris_[i].v1() == wt.v1()) &&
                    (tris_[i].v2() == wt.v2()))
                {
                    break;
                }
            }
            if (i >= sz)
            {
                tris_.push_back( wt );
            }
        }
        return COLLIDE_ALL;
    }
};

superDecalTrianglesCollector collector;


#define CLIP_SEGMENT(tmpP1,tmpP2) \
{\
    cp1 = tmpP1;\
    cp2 = tmpP2;\
    if(b.clip(cp1,cp2))\
    {\
        points.push_back(cp1);\
        points.push_back(cp2);\
    };\
}

bool lineSegmentTriangleIntersect( const Vector3 &p0, const Vector3 &p1, const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,Vector3 &I)
{
    Vector3 dir, w0, w,u,v,n; // ray vectors
    float r, a, b; // params to calc ray-plane intersect
    float uu, uv, vv, wu, wv, D;
    float s, t;
    const float SMALL_NUM=0.00001f;
    // get triangle edge vectors and plane normal
    u = v1-v0;
    v = v2-v0;
    n = u.crossProduct(v);
    if (n.lengthSquared() < SMALL_NUM) return false;
    dir = p1-p0;         // ray direction vector
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
    if (r < 0.0f || r > 1.0f) 
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
    return true;                  // I is in T
}

BW_END_NAMESPACE

#include "decal_utils.ipp"

BW_BEGIN_NAMESPACE


float clipTrianglesAgainstBox(Vector3* locBox,Vector3* worldBox,Matrix toWorld,Matrix toLoc,BW::vector< CPUSpotVertex > *vertices,bool getMinDist, const Vector4 *texCoord)
{
    BW_GUARD;

    Vector3 *locV = locBox;
    BoundingBox b = makeBoundingBoxFromLocalVertices(locV);
    BW::vector< CPUSpotVertex > v;
    BW::vector<Vector3> points;
    BW::vector<Vector3> points2;
    Vector3 p1,p2,p3;
    Vector3 cp1,cp2;
    for(size_t i = 0; i < vertices->size()/3; ++i)
    {
        points.clear();
        // translate points to BBox space
        p1 = toLoc.applyPoint((*vertices)[i*3+0].pos_);
        p2 = toLoc.applyPoint((*vertices)[i*3+1].pos_);
        p3 = toLoc.applyPoint((*vertices)[i*3+2].pos_);
        // clip triangle
        CLIP_SEGMENT(p1,p2);
        CLIP_SEGMENT(p2,p3);
        CLIP_SEGMENT(p3,p1);
        // add points of bbox intersection with triangle
        Vector3 iP;
        if(lineSegmentTriangleIntersect(locV[0],locV[1],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[1],locV[2],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[2],locV[3],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[3],locV[0],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[4],locV[5],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[5],locV[6],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[6],locV[7],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[7],locV[4],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[0],locV[4],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[1],locV[5],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[2],locV[6],p1,p2,p3,iP)) points.push_back(iP);
        if(lineSegmentTriangleIntersect(locV[3],locV[7],p1,p2,p3,iP)) points.push_back(iP);
        sort3DPoints(points);

        if(points.size() < 2)
            continue;

        points2.clear();
        points2.push_back(points[0]);
        for(size_t i = 1; i < points.size(); ++i)
        {
            if(!isEqual(points[i],points2.back()))
                points2.push_back(points[i]);
        }
        points = points2;

        // translate vertices back to world
        for(size_t i = 0; i < points.size(); ++i)
            points[i] = toWorld.applyPoint(points[i]);

        Vector2 uv;
        // triangulate
        if(!points.empty())
        {
            for(size_t i = 1; i < points.size()-1; ++i)\
            {
                v.push_back(CPUSpotVertex(points[0],*distanceFromPointToWorldBoxNormalized(worldBox,&points[0],&uv,texCoord)));
                v.push_back(CPUSpotVertex(points[i],*distanceFromPointToWorldBoxNormalized(worldBox,&points[i],&uv,texCoord)));
                v.push_back(CPUSpotVertex(points[i+1],*distanceFromPointToWorldBoxNormalized(worldBox,&points[i+1],&uv,texCoord)));
            }
        }
    }
    vertices->clear();
    *vertices = v;
    float minDist = 1e10;
    if(getMinDist)
    {
        for(size_t i = 0; i < vertices->size(); ++i)
        {
            Vector3 p = worldBox[0];
            Vector3 n = worldBox[4]-worldBox[0];
            n.normalise();
            float D = n.dotProduct(p);
            float dist = (*vertices)[i].pos_.dotProduct(n)-D;
            if(dist < minDist) minDist = dist;
        }
    }
    return minDist; 
}

#ifdef DEBUG_STICKERS
static int g_iObjFileIndex = 0;

void verticesToObj( const BW::vector< stickerCPU::VertexType > &vertices, const Matrix &transform, const char *szFileName )
{
    FILE *f;
    if ( fopen_s( &f, szFileName, "wt" ) != 0 ) return;

    for( size_t i = 0; i < vertices.size(); i++ )
    {
        Vector3 v = transform.applyPoint( vertices[i].pos_ );
        fprintf( f, "v %f %f %f\n", v.x, v.y, v.z );
    }

    for( size_t i = 0; i < vertices.size(); i += 3 )
        fprintf( f, "f %i %i %i\n", i + 1, i + 2, i + 3 );

    fclose( f );
}

void trianglesToObj( const RealWTriangleSet &tris, const char *szFileName )
{
    FILE *f;
    if ( fopen_s( &f, szFileName, "wt" ) != 0 ) return;

    for( size_t i = 0; i < tris.size(); i++ )
    {
        const Vector3 &v0 = tris[i].v0();
        fprintf( f, "v %f %f %f\n", v0.x, v0.y, v0.z );
        const Vector3 &v1 = tris[i].v1();
        fprintf( f, "v %f %f %f\n", v1.x, v1.y, v1.z );
        const Vector3 &v2 = tris[i].v2();
        fprintf( f, "v %f %f %f\n", v2.x, v2.y, v2.z );
    }

    for( size_t i = 0; i < tris.size(); i++ )
        fprintf( f, "f %i %i %i\n", i * 3 + 1, i * 3 + 2, i * 3 + 3 );

    fclose( f );
}
#endif


bool findTrianglesInsideWorldBox(const PyModel* model, Vector3 *worldBox,BW::vector< CPUSpotVertex > *vertices,bool closestOnly)
{
    BW_GUARD;

    const BSPTree* bsp = model->pSuperModel()->topModel(0)->decompose();
    if (bsp == NULL)
        return false;

    BSPFlagsMap bspMap;
    bspMap.push_back( TRIANGLE_DOUBLESIDED);
    const_cast<BSPTree*>(bsp)->remapFlags( bspMap );
    Matrix wT = model->worldTransform();
    BoundingBox BB(Vector3(-1e10,-1e10,-1e10),Vector3(1e10,1e10,1e10));
    ChunkBSPObstacle *l_obstacle = new ChunkBSPObstacle(bsp,wT,&BB,NULL);
    CollisionState state(collector);
    Matrix wTi;
    wTi.invert(wT);

#ifdef DEBUG_STICKERS
    {
        char strFileName[64];
        _snprintf( strFileName, 64, "model%03d.bsp", g_iObjFileIndex );
        bsp->save( strFileName );

        _snprintf( strFileName, 64, "d:\\worldbox%03d.obj", g_iObjFileIndex );
        FILE *f;
        if ( fopen_s( &f, strFileName, "w" ) == 0 )
        {
            for( size_t i = 0; i < 8; i++ )
            {
                Vector3 v = wTi.applyPoint( worldBox[i] );
                fprintf( f, "v %f %f %f\n", v.x, v.y, v.z );
            }
            fprintf( f, "f %d %d %d %d\n", 1, 2, 3, 4 );
            fprintf( f, "f %d %d %d %d\n", 5, 6, 7, 8 );
            fprintf( f, "f %d %d %d %d\n", 1, 2, 6, 5 );
            fprintf( f, "f %d %d %d %d\n", 2, 3, 7, 6 );
            fprintf( f, "f %d %d %d %d\n", 3, 4, 8, 7 );
            fprintf( f, "f %d %d %d %d\n", 4, 1, 1, 8 );
            fclose( f );
        }
    }
#endif

#ifdef DEBUG_STICKERS
    {
        const RealWTriangleSet &tris = bsp->triangles();
        char strFileName[64];
        _snprintf( strFileName, 64, "d:\\bsp%03d.obj", g_iObjFileIndex );
        trianglesToObj( tris, strFileName );
    }
#endif

    Vector3* worldV = worldBox;
    WorldTriangle tri1( wTi.applyPoint(worldV[0]), wTi.applyPoint(worldV[1]), wTi.applyPoint(worldV[3]) );
    WorldTriangle tri2( wTi.applyPoint(worldV[1]), wTi.applyPoint(worldV[2]), wTi.applyPoint(worldV[3]) );
    Vector3 extent = (wTi.applyPoint(worldV[4])-wTi.applyPoint(worldV[0]));
    collector.tris_.clear();
    collector.normal_ = extent;
    collector.normal_.normalise();
    l_obstacle->collide( tri1, tri1.v0()+extent, state );
    l_obstacle->collide( tri2, tri2.v0()+extent, state );
    vertices->clear();
    for(size_t i = 0; i < collector.tris_.size(); ++i)
    {
        WorldTriangle& tri = collector.tris_[i];
        Vector3 normal = tri.normal();
        vertices->push_back(CPUSpotVertex(tri.v0(),Vector2(0,0)));
        vertices->back().normal_ = normal;
        vertices->push_back(CPUSpotVertex(tri.v1(),Vector2(0,0)));
        vertices->back().normal_ = normal;
        vertices->push_back(CPUSpotVertex(tri.v2(),Vector2(0,0)));
        vertices->back().normal_ = normal;
    }
    delete l_obstacle;

    return true;
}

void stickerCPU::initialize( const Vector3 &start_, const Vector3 &end_, const Vector2 &size_, const PyModel* model, const Vector3 &up_)
{
    BW_GUARD;

    Vector3 size(size_.x,size_.y,0);

    Matrix wTr = model->worldTransform();
    Vector3 start = wTr.applyPoint(start_);
    Vector3 end = wTr.applyPoint(end_);
    Vector3 up(up_);
    up = wTr.applyVector(up);

#ifdef DEBUG_STICKERS
    g_iObjFileIndex++;
#endif

    Vector3 worldV[8],locV[8];
    Matrix toWorld,toLoc;
    const static Vector4 texCoords(0,0,1.0f,1.0f);
    makeBoxFromStartEndAndSize(start, end, size, up, worldV,locV,&toWorld,&toLoc);
    if ( findTrianglesInsideWorldBox(model, worldV,&m_vertices,false) )
    {
        Vector3 hitNormal(0,0,0);
        size_t vertLength = m_vertices.size();

        if(vertLength == 0)
            return;

        for(size_t i = 0; i < vertLength; ++i)
            hitNormal += m_vertices[i].normal_;
        hitNormal.normalise();
        Vector3 direction = start - end;
        direction.normalise();

        float curAngle = acosf(hitNormal.dotProduct(direction));
        float criticalAngle = PI* 0.5f - PyStickerModel::criticalAngle();
        if(curAngle > criticalAngle)
        {
            rotateSegmentInPlane(start, end, hitNormal, curAngle - criticalAngle );         
            makeBoxFromStartEndAndSize(start, end, size, up, worldV,locV,&toWorld,&toLoc);
            if( !findTrianglesInsideWorldBox(model, worldV,&m_vertices,false) )
                return;
        }


    #ifdef DEBUG_STICKERS
        {
            char strFileName[64];
            _snprintf( strFileName, 64, "d:\\model%03d.obj", g_iObjFileIndex );
            Matrix wti;
            wti.invert( model->worldTransform() );
            verticesToObj( vertices_, wti, strFileName );
        }
    #endif

        clipTrianglesAgainstBox(locV,worldV,toWorld,toLoc,&m_vertices,false,&texCoords);

    #ifdef DEBUG_STICKERS
        {
            char strFileName[64];
            _snprintf( strFileName, 64, "d:\\sticker%03d.obj", g_iObjFileIndex );
            Matrix wti;
            wti.invert( model->worldTransform() );
            verticesToObj( vertices_, wti, strFileName );
        }
    #endif
        // transform vertices to local model space
        Matrix initialOwnerTransform = model->worldTransform();
        initialOwnerTransform.invert();
        for(size_t i = 0; i < m_vertices.size(); ++i)
            m_vertices[i].pos_ = initialOwnerTransform.applyPoint(m_vertices[i].pos_);

        Vector3 displacement(start_-end_);
        displacement.normalise();
        displacement *= 0.0025f;

        eliminateGaps(displacement);
        calculateNormals(m_vertices);
    }
}


void stickerCPU::eliminateGaps( const Vector3 &displacement )
{
    BW_GUARD;

    bool isModified;

    for(size_t i = 0; i < m_vertices.size(); ++i)
    {
        m_vertices[i].pos_ = m_vertices[i].pos_+displacement;
    }

    if(m_vertices.size() > 1)
    {
        for(size_t i = 0; i < m_vertices.size()-1; ++i)
        {
            isModified = false;
            for(size_t j = i+1; j < m_vertices.size(); ++j)
            {
                if(m_vertices[i].pos_ != m_vertices[j].pos_ && (m_vertices[i].pos_-m_vertices[j].pos_).length() < 0.01)//isEqual(vertices[i].pos_,vertices[j].pos_,0.01f))
                {
                    m_vertices[j].pos_ = m_vertices[i].pos_;
                }
            }
        }
    }
}

stickerCPU::stickerCPU(int id, 
           const BW::string& textureID, 
           Moo::BaseTexture* pTexSrc, 
           float alpha, 
           bool isInscription):
m_id(id), 
m_textureID(textureID), 
m_alpha(alpha), 
m_pTexSrc(pTexSrc),
m_isInscription(isInscription)
{
}

stickerGPU::~stickerGPU()
{
    SAFE_DELETE(vb);
}

//---------------------------------------------------------------------------------------------------

ForwardDecalsManager* ForwardDecalsManager::s_pInstance = NULL;

ForwardDecalsManager::ForwardDecalsManager():
m_material(NULL)
, m_stickerVertexDeclaration(NULL)

{

    for(int i = 0; i < NUM_DECAL_TYPES; ++i)
    {
        m_decalAtlasSize[i] = Vector4(1,1,1,1);
    }
    createManagedObjects();
    createUnmanagedObjects();

    const BW::StringRef declName = 
        Moo::VertexFormatCache::getTargetName<stickerCPU::VertexType>( 
            Moo::VertexDeclaration::getTargetDevice(), true );
    
    m_stickerVertexDeclaration = Moo::VertexDeclaration::get( declName );
    MF_ASSERT( m_stickerVertexDeclaration );

}

ForwardDecalsManager::~ForwardDecalsManager()
{

}


void ForwardDecalsManager::init()
{
    if (!s_pInstance)
    {
        s_pInstance = new ForwardDecalsManager();
    }
}


void ForwardDecalsManager::fini()
{
    if ( s_pInstance )
    {
        delete s_pInstance;
        s_pInstance = NULL;
    }
}

ForwardDecalsManager* ForwardDecalsManager::getInstance()
{
    return s_pInstance;
}

void ForwardDecalsManager::addSticker( ForwardStickerModel *pModel )
{
    if(pModel)
    {
        removeSticker(pModel);
        m_stickerModels.push_back(pModel);
    }
}

void ForwardDecalsManager::removeSticker( ForwardStickerModel *pModel)
{
    for(BW::list<ForwardStickerModel*>::iterator iter = m_stickerModels.begin(); iter != m_stickerModels.end(); ++iter)
    {
        if(*iter == pModel)
        {
            m_stickerModels.erase(iter);
            break;
        }
    }
    m_stickersToDraw.clear();
    m_inscriptionsToDraw.clear();
}

void ForwardDecalsManager::addForwardStickerToDraw( stickerGPU* pSticker, const Matrix & worldTransform, float lod, float distance, float alpha, bool isInscription )
{
    if(isInscription)
        m_inscriptionsToDraw.push_back(ForwardStickerDrawInfo(pSticker, worldTransform, distance, lod, alpha));
    else
        m_stickersToDraw.push_back(ForwardStickerDrawInfo(pSticker, worldTransform, distance, lod, alpha));
}

void ForwardDecalsManager::draw()
{
    BW_GUARD;
    BW_SCOPED_RENDER_PERF_MARKER("Forward Decals");

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        if (Moo::rc().pushRenderTarget())
        {
            Moo::rc().setRenderTarget(0, Renderer::instance().pipeline()->gbufferSurface(2));

            drawPriv();

            Moo::rc().popRenderTarget();
        }
    }
    else
    {
        drawPriv();
    }
    m_stickersToDraw.clear();
    m_inscriptionsToDraw.clear();
}

void ForwardDecalsManager::drawPriv()
{
    beginZBIASDraw(1.0005f);

    drawStickers( m_stickersToDraw, m_decalAtlasMap[STICKER].get(), m_decalAtlasSize[STICKER] );
    drawStickers( m_inscriptionsToDraw, m_decalAtlasMap[INSCRIPTION].get(), m_decalAtlasSize[INSCRIPTION] );

    endZBIASDraw();
}

void ForwardDecalsManager::drawStickers(BW::list<ForwardStickerDrawInfo>& stickers, TextureAtlas* pAtlas, const Vector4& atlasSize)
{
    m_material->pEffect()->pEffect()->SetTexture("g_atlasMap",  pAtlas->pTexture());
    m_material->pEffect()->pEffect()->SetVector ("g_atlasSize", &atlasSize);
    m_material->pEffect()->pEffect()->CommitChanges();
    for(BW::list<ForwardStickerDrawInfo>::iterator it = stickers.begin(); it != stickers.end(); ++it)
    {
        drawSingleSticker(*it);
    }
}


TextureAtlas* ForwardDecalsManager::decalAtlas( EDecalType type ) const
{
    BW_GUARD;
    MF_ASSERT(m_decalAtlasMap[type].get());

    return m_decalAtlasMap[type].get();
}

Moo::EffectMaterial* ForwardDecalsManager::pMaterial() const
{
    BW_GUARD;
    MF_ASSERT(m_material.get());

    return m_material.get();
}

bool ForwardDecalsManager::recreateForD3DExDevice() const
{
    return true;
}

void ForwardDecalsManager::createManagedObjects()
{
    BW_GUARD;

    //-- create texture atlases.
    m_decalAtlasMap[STICKER] = new TextureAtlas(
        static_cast<uint>(g_decalAtlasSize.x), static_cast<uint>(g_decalAtlasSize.y), "forward_decal_atlas_map"
        );

    m_decalAtlasMap[INSCRIPTION] = new TextureAtlas(
        static_cast<uint>(g_inscriptionAtlasSize.x), static_cast<uint>(g_inscriptionAtlasSize.y), "forward_inscription_atlas_map"
        );

    for(int i = 0; i < NUM_DECAL_TYPES; ++i)
    {
        m_decalAtlasSize[i].set(
            (float)m_decalAtlasMap[i]->width(), (float)m_decalAtlasMap[i]->height(),
            1.0f / (float)m_decalAtlasMap[i]->width(), 1.0f / (float)m_decalAtlasMap[i]->height()
            );
    }

    m_material = new Moo::EffectMaterial();
    DataSectionPtr p = BWResource::openSection("system/materials/sticker.mfm");
    if(m_material->load(p))
    {
        if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
            m_material->hTechnique("DEFERRED");
        else
            m_material->hTechnique("FORWARD");
    }
}

void ForwardDecalsManager::deleteManagedObjects()
{
    for(int i = 0; i < NUM_DECAL_TYPES; ++i)
    {
        m_decalAtlasMap[i] = NULL;
    }
    m_material = NULL;
}

void ForwardDecalsManager::createUnmanagedObjects()
{
    for(BW::list<ForwardStickerModel*>::iterator iter = m_stickerModels.begin(); iter != m_stickerModels.end(); ++iter)
        (*iter)->createUnmanagedObjects();

    for(int i = 0; i < NUM_DECAL_TYPES; ++i)
    {
        m_decalAtlasMap[i]->createUnmanagedObjects();
    }
}

void ForwardDecalsManager::deleteUnmanagedObjects()
{
    for(BW::list<ForwardStickerModel*>::iterator iter = m_stickerModels.begin(); iter != m_stickerModels.end(); ++iter)
        (*iter)->deleteUnmanagedObjects();

    for(int i = 0; i < NUM_DECAL_TYPES; ++i)
    {
        m_decalAtlasMap[i]->deleteUnmanagedObjects();
    }
}
// -----------------------------------------------------------------------------
// Section: ForwardStickerModel
// -----------------------------------------------------------------------------

//#define LOG_STICKERS

ForwardStickerModel::ForwardStickerModel()
{
    newDecalIndex = 1;
    lodDistances[0] = lodDistances[1] = 1e10;
    alphas[0] = alphas[1] = 1;
    ForwardDecalsManager::getInstance()->addSticker(this);
}

ForwardStickerModel::~ForwardStickerModel()
{
    ForwardDecalsManager::getInstance()->removeSticker(this);
}



bool stickerCPU::refillGPUSticker()
{
    EDecalType decalType = m_isInscription ? INSCRIPTION : STICKER;
    m_stickerGPU.texture = ForwardDecalsManager::getInstance()->decalAtlas(decalType)->subTexture(m_textureID, m_pTexSrc, false);

    if(!m_stickerGPU.texture.exists())
    {
        ERROR_MSG("ForwardStickerModel: Cannot add texture [%s] to atlas! Check it's existence and format (must be DXT3).", m_textureID.c_str());
        return false;
    }

    const Rect2D& r1 = m_stickerGPU.texture->m_rect;

    const Vector4& atlasSize = ForwardDecalsManager::getInstance()->decalAtlasSize(decalType);

    m_stickerGPU.uv.x = ( r1.m_right - r1.m_left + 0.3f ) * atlasSize.z;
    m_stickerGPU.uv.y = ( r1.m_bottom - r1.m_top + 0.3f ) * atlasSize.w;
    m_stickerGPU.uv.z = ( r1.m_left + 0.3f ) * atlasSize.z;
    m_stickerGPU.uv.w = ( r1.m_top + 0.3f ) * atlasSize.w;

    m_stickerGPU.verticesCount = 0;
    int current = 0;
    m_stickerGPU.verticesCount = static_cast<int>(m_vertices.size());

    SAFE_DELETE(m_stickerGPU.vb);
    if(m_stickerGPU.verticesCount > 0)
    {
        // Setup vertex format conversion
        Moo::VertexFormat::ConversionContext vfConversion = 
            Moo::VertexFormatCache::createTargetConversion<stickerCPU::VertexType>( 
                Moo::VertexDeclaration::getTargetDevice() );

        MF_ASSERT( vfConversion.isValid() );

        // create a vertex buffer
        m_stickerGPU.vb = new Moo::VertexBuffer;
        if ( FAILED( m_stickerGPU.vb->create(m_stickerGPU.verticesCount*vfConversion.dstVertexSize(),D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT) ) )
        {
            SAFE_DELETE(m_stickerGPU.vb);
            return false;
        }
        // lock the vertex buffer and copy/convert the source data into it
        Moo::SimpleVertexLock vertexLock(*m_stickerGPU.vb,0,0,D3DLOCK_NOOVERWRITE);
        if (vertexLock)
        {
            const void * pSrcVertices = &m_vertices[0];
            bool converted = 
                vfConversion.convertSingleStream( vertexLock, pSrcVertices, m_stickerGPU.verticesCount );
            MF_ASSERT( converted );
        }
    }
    return true;
}

void ForwardDecalsManager::drawSingleSticker( const ForwardStickerDrawInfo& drawInfo )
{
    BW_GUARD;

    ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();

    float dist = 0;
    if(pSpace.exists())
    {
        dist = drawInfo.distance;
    }

    const stickerGPU& sticker = *drawInfo.pSticker;
    // texture #1 for damage stickers, #0 and #2..N for other stickers
    float lodDist = LodSettings::instance().applyLodBias(drawInfo.lod);

    if (sticker.verticesCount && (lodDist > dist) && sticker.texture != NULL)
    {
        // calculate transformation to accomodate new owner position
        uint32 vertexSize = m_stickerVertexDeclaration->format().streamStride(0);
        MF_ASSERT(vertexSize);
        sticker.vb->set(0,0,vertexSize);

        if(m_material->begin())
        {
            for(uint32 i = 0; i < m_material->numPasses(); ++i)
            {
                m_material->beginPass(i);

                m_material->pEffect()->pEffect()->SetMatrix( "worldSticker", &drawInfo.worldTransform );
                m_material->pEffect()->pEffect()->SetFloat( "alpha", drawInfo.alpha);
                m_material->pEffect()->pEffect()->SetVector("g_uv", &sticker.uv);
                m_material->pEffect()->pEffect()->CommitChanges();

                // backup cull-mode state.
                // Note: This is not expensive, because BigWorld automatically backups render states,
                //       and this call is the only access to the cache of variables.
                Moo::rc().pushRenderState(D3DRS_CULLMODE);
                Moo::rc().setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
                Moo::rc().drawPrimitive(D3DPT_TRIANGLELIST, 0, sticker.verticesCount/3);
                Moo::rc().popRenderState();

                m_material->endPass();
            }
            m_material->end();
        }
        if (Moo::rc().mixedVertexProcessing())
            Moo::rc().device()->SetSoftwareVertexProcessing(TRUE);
    }
}

void ForwardStickerModel::tossed( bool outside )
{

}

void ForwardStickerModel::setLODDistances(float texture0distance,float texture1distance)
{
    lodDistances[0] = texture0distance;
    lodDistances[1] = texture1distance;
}

void ForwardStickerModel::setAlphas(float texture0alpha,float texture1alpha)
{
    alphas[0] = texture0alpha;
    alphas[1] = texture1alpha;
}

int ForwardStickerModel::addSticker( BW::string texture, 
                                     const SmartPointer<PyModel> model_, 
                                     Vector3 segmentStart, 
                                     Vector3 segmentEnd, 
                                     Vector2 size_, 
                                     Vector3 up_, 
                                     Moo::BaseTexture* pSrcTex, 
                                     bool isInscription)
{
    BW_GUARD;

    owner = model_.getObject();
    decals.push_back(stickerCPU(newDecalIndex++,texture, pSrcTex, alphas[0], isInscription));
    decals.back().initialize(segmentStart,segmentEnd,size_,owner, up_);
    decals.back().refillGPUSticker();
    return decals.back().id();
}

void ForwardStickerModel::delSticker( int stickerID )
{
    BW_GUARD;
    BW::list<stickerCPU>::iterator end = decals.end();
    BW::list<stickerCPU>::iterator iter = decals.begin();
    for( ; iter != end; ++iter)
    {
        if(iter->id() == stickerID)
        {
            decals.erase(iter);
            break;
        }
    }
}

void ForwardStickerModel::clear()
{
    decals.clear();
}

void ForwardStickerModel::deleteUnmanagedObjects()
{
    BW::list<stickerCPU>::iterator end = decals.end();
    BW::list<stickerCPU>::iterator iter = decals.begin();
    for( ; iter != end; ++iter)
    {
        stickerGPU& sgpu = iter->gpuSticker();
        SAFE_DELETE(sgpu.vb);
        sgpu.verticesCount = 0;
        sgpu.texture = NULL;
    }
}

void ForwardStickerModel::createUnmanagedObjects()
{
    BW::list<stickerCPU>::iterator end = decals.end();
    BW::list<stickerCPU>::iterator iter = decals.begin();
    for( ; iter != end; ++iter)
    {
        iter->refillGPUSticker();
    }
}

void ForwardStickerModel::draw(class Matrix const &,float)
{
}

void ForwardStickerModel::prepareDraw( float distance )
{
    ForwardDecalsManager* fdm = ForwardDecalsManager::getInstance();
    if(owner && fdm)
    {
        BW::list<stickerCPU>::iterator end = decals.end();
        BW::list<stickerCPU>::iterator iter = decals.begin();
        for( ; iter != end; ++iter)
        {
            fdm->addForwardStickerToDraw( &iter->gpuSticker(), owner->worldTransform(), lodDistances[0], distance, alphas[0], iter->isInscription() );
        }
    }
}

BW_END_NAMESPACE
