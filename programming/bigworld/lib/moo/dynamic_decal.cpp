#include "pch.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "duplo/decal.hpp"
#include "moo/forward_declarations.hpp"
#include "pyscript/script.hpp"
#include "physics2/collision_obstacle.hpp"

#include "material_kinds_constants.hpp"
#include "deferred_decals_manager.hpp"
#include "renderer.hpp"
#include "dynamic_decal.hpp"

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE(DynamicDecalManager);

namespace {
    class ClosestTriangle : public CollisionCallback
    {
    public:
        virtual int operator()( const CollisionObstacle & obstacle,
            const WorldTriangle & triangle, float dist )
        {
            // skip destructible materials
#ifndef _NAVGEN
            uint matKind = triangle.materialKind();
            if (matKind >= MATKIND_DESTRUCTIBLE_MIN && matKind <= MATKIND_DESTRUCTIBLE_MAX)
                return COLLIDE_ALL;
#endif

            // transform into world space
            hit_ = WorldTriangle(
                obstacle.transform().applyPoint( triangle.v0() ),
                obstacle.transform().applyPoint( triangle.v1() ),
                obstacle.transform().applyPoint( triangle.v2() ) );
            hit_.flags( WorldTriangle::packFlags( triangle.collisionFlags(), triangle.materialKind() ) );
            dist_ = dist;
            return COLLIDE_BEFORE;
        }

        float dist_;
        WorldTriangle hit_;
    };
} // unnamed namespaces

static ClosestTriangle s_closesttri;

/*~ function BigWorld.addDecal
 *
 *  This function attempts to add a decal to the scene, given a world ray.
 *  If the world ray collides with scene geometry, a decal is placed over
 *  the intersecting region (clipped against the underlying collision geometry). 
 *  Try not to have too large an extent for the decal collision ray, for 
 *  reasons of performance.
 *
 *  This function returns the texture index that is needed to pass into
 *  addDecal.  It takes the name of the texture, and returns the index,
 *  or -1 if none was found. The new decal will be added to the space
 *  currently being viewed by the camera.
 */
 /**
 *  Adds a decal to the scene.
 */
void addDecal(
    BW::string groupName,
    const Vector3& start,
    const Vector3& end,
    const Vector2& size,
    float yaw,
    int difTexIdx,
    int bumpTexIdx)
{
    BW_GUARD;
    if ( size.length() == 0.0f )
    {
        PyErr_Format( PyExc_RuntimeError, "addDecal - decal size is zero.\n" );
        return; 
    }

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_FORWARD_SHADING)
    {
        ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
        if (space.exists())
        {
            EnviroMinder & envMinder = ChunkManager::instance().cameraSpace()->enviro();
            Decal* decal = envMinder.decal();
            int groupIndex = decal->validateGroupName(groupName);
            if (groupIndex == -1)
            {
                PyErr_Format( PyExc_RuntimeError, "addDecal - invalid <groupName> parameter."
                    " groupName = %s.\n", groupName.c_str() );
                return;
            }
            decal->add( start, end, size, yaw, difTexIdx, groupIndex);
        }
    }
    else
    {
        DynamicDecalManager::addDecal(  DynamicDecalManager::instance().getLifeTime(groupName),
                                    start, end, size, yaw, 
                                    DynamicDecalManager::instance().textureName(difTexIdx), 
                                    DynamicDecalManager::instance().textureName(bumpTexIdx));
    }
}

BW_END_NAMESPACE

BW_BEGIN_NAMESPACE

PY_AUTO_MODULE_FUNCTION( RETVOID, addDecal, ARG(BW::string, ARG( Vector3, ARG( Vector3, ARG( Vector2, ARG(float, ARG( int, ARG(int, END ) ) ) ) ) ) ), BigWorld )


void DynamicDecalManager::addDecal(
    float lifeTime,
    const Vector3& start,
    const Vector3& end,
    const Vector2& size,
    float yaw,
    const BW::string& difTexName,
    const BW::string& bumpTexName,
    const BW::string blendDifTexName, 
    float textureBlendFactor,
    bool useSimpleCollision)
{
    BW_GUARD;
    ChunkSpace* pSpace = ChunkManager::instance().cameraSpace().get();
    if ( !pSpace )
        return;

    Vector3 direction = end - start;
    direction.normalise();
    Vector3 up(1.0f, 0.0f, 0.0f);
    Vector3 hit_dir(0.0f, -1.0f, 0.0f);
    bool objectImpacted = false;
    float boxDepth = 1.0f;
    //////////////////////////////////////////////////////////////////////////
    //find surface triangles into decal projection box
    BoundingBox bbox;
    bbox.addBounds(start);
    bbox.addBounds(end);
    Vector3 pos(bbox.centre());
    float dist = 100000000.0f;

    {
        dist = pSpace->collide( start, end, s_closesttri );
        if (dist == -1.0f)
            return;

        // find perpendicular vector with desired yaw angle to direction.
        hit_dir = s_closesttri.hit_.normal() * -1.0f;
        hit_dir.normalise();

        objectImpacted = (s_closesttri.hit_.flags() & TRIANGLE_TERRAIN) == 0;
        boxDepth = 0.5f;//min(size.x, size.y)*0.2f;
    }

    float boxWidthKoef = objectImpacted ? DynamicDecalManager::instance().decalObjectImpactSizeCoefficient : 1.0f;
    // find hit point.
    pos = start + direction * dist;

    if (almostZero(1.0f - abs(hit_dir.dotProduct(up)), 0.01f))
        up = Vector3(0.0f, 1.0f, 0.0f);

    //////////////////////////////////////////////////////////////////////////

    DecalsManager::Decal::Desc desc;
    // do not use bump mapped decal if we it's projected on objects 
    if(objectImpacted || bumpTexName == "")
        desc.m_materialType =  DecalsManager::Decal::MATERIAL_DIFFUSE;
    else
        desc.m_materialType =  DecalsManager::Decal::MATERIAL_BUMP;

    desc.m_influenceType = DecalsManager::Decal::APPLY_TO_TERRAIN | DecalsManager::Decal::APPLY_TO_STATIC;
#ifdef EDITOR_ENABLED
    desc.m_influenceType |= DecalsManager::Decal::APPLY_TO_DYNAMIC;
#endif
    desc.m_priority      = 0;
    desc.m_map1          = difTexName;
    desc.m_map2          = bumpTexName;

    desc.m_transform.lookAt(pos, hit_dir, up);
    desc.m_transform.postRotateZ(yaw); // consider a yaw angle.
    desc.m_transform.invert();
    Matrix scale;
    scale.setScale(size.x*boxWidthKoef, size.y*boxWidthKoef, boxDepth);
    desc.m_transform.preMultiply(scale);

    if(blendDifTexName.empty() || blendDifTexName == difTexName || textureBlendFactor == 0.0f)
        DecalsManager::instance().createDynamicDecal(desc, lifeTime);
    else
    {
        DecalsManager::instance().createDynamicDecal(desc, lifeTime, 1.0f - textureBlendFactor);
        desc.m_map1 = blendDifTexName;
        DecalsManager::instance().createDynamicDecal(desc, lifeTime, textureBlendFactor);
    }
}


/*~ function BigWorld.clearDecals
 *
 *  This function clears the current list of decals. Only the decals on 
 *  the space currently being viewed by the camera will be cleared.
 *
 *  Note: Before version 1.9, this needed to be called when changing 
 *  spaces, so that decals from another space didn't draw in 
 *  the current one. From 1.9 onwards, decals are stored per space 
 *  and calling this function is no longer a requirement.
 */
/**
 *  Clear the current list of decals.
 */
void clearDecals()
{
    BW_GUARD;

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_FORWARD_SHADING)
    {
        ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
        if (space.exists())
        {
            EnviroMinder & envMinder = ChunkManager::instance().cameraSpace()->enviro();
            envMinder.decal()->clear();
        }
    }
    else
        DecalsManager::instance().clearDynamicDecals();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, clearDecals, END, BigWorld )

// 
//------------------------------------------
int addDecalGroup( const BW::string& groupName,
                      float lifeTime,
                      int trianglesCount)
{
    BW_GUARD;

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_FORWARD_SHADING)
    {
        ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
        if (space.exists())
        {
            EnviroMinder & envMinder = ChunkManager::instance().cameraSpace()->enviro();
            return envMinder.decal()->addDecalGroup(groupName, lifeTime, trianglesCount);
        }
    }
    else
        DynamicDecalManager::instance().addDecalGroup(groupName, lifeTime);
    return 0;
}

PY_AUTO_MODULE_FUNCTION( RETERR, addDecalGroup, ARG( BW::string, ARG( float, ARG(int, END ) ) ) , BigWorld )

int decalTextureIndex( const BW::string& textureName  )
{
    BW_GUARD;

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_FORWARD_SHADING)
    {
        ChunkSpace* pChunkSpace = ChunkManager::instance().cameraSpace().get();
        if(pChunkSpace)
            return pChunkSpace->enviro().decal()->decalTextureIndex(textureName);
        return -1;
    }
    return DynamicDecalManager::instance().textureIndex( textureName );
}

PY_AUTO_MODULE_FUNCTION( RETDATA, decalTextureIndex, ARG( BW::string, END ), BigWorld )


int DynamicDecalManager::addDecalGroup( const BW::string& groupName, float lifeTime )
{
    m_groups.push_back(DecalGroup(groupName, lifeTime));
    return int(m_groups.size() - 1);
}

float DynamicDecalManager::getLifeTime( const BW::string& groupName )
{
    for(size_t i = 0; i < m_groups.size(); ++i)
        if(m_groups[i].m_name.compare(groupName) == 0)
            return m_groups[i].m_lifeTime;
    return 1000000000.0f;
}

float DynamicDecalManager::getLifeTime( int32 idx )
{
    if(idx >= 0 && idx < (int32)m_groups.size())
        return m_groups[idx].m_lifeTime;
    return 1000000000.0f;
}

int32 DynamicDecalManager::getGroupIndex( const BW::string& groupName )
{
    for(size_t i = 0; i < m_groups.size(); ++i)
        if(m_groups[i].m_name.compare(groupName) == 0)
            return int32(i);
    return -1;
}

int DynamicDecalManager::textureIndex( const BW::string& texName )
{
    for(size_t i = 0; i < m_textures.size(); ++i)
        if(m_textures[i].compare(texName) == 0)
            return int(i);
    m_textures.push_back(texName);
    return int( m_textures.size() - 1 );
}

BW::string DynamicDecalManager::textureName( int idx )
{
    if(idx >= 0 && idx < (int32)m_textures.size())
        return m_textures[idx];
    return "";
}

DynamicDecalManager::DynamicDecalManager(): cur_idx(0),
decalObjectImpactSizeCoefficient(0.5f)
{
    MF_WATCH("Render/DS/Decals/Object size coefficient",
        decalObjectImpactSizeCoefficient, Watcher::WT_READ_WRITE, "Size coefficient of decal on impact into complex geometry objects to reduce artifacts."
        );
}

BW_END_NAMESPACE
