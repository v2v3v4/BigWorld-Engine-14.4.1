#include "pch.hpp"
#include "worldeditor/world/items/chunk_link_segment.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/geometrics.hpp"
#include "physics2/worldtri.hpp"
#include <limits>

BW_BEGIN_NAMESPACE

namespace
{
    const float SELECTION_EXPANSION = 7.0f;

    //
    // Given v, find a set of orthonormal vectors e1, e2, e3 such that
    // e1 is v normalised.
    //
    // @param v     The vector to find an orthnormal set for.
    // @param e1    v normalised.
    // @param e2    Something orthogonal to e1.
    // @param e3    Something orthogonal to e1 and e2.
    //
    void
    orthonormalSet
    (
        Vector3     const &v,
        Vector3     &e1,
        Vector3     &e2,
        Vector3     &e3
    )
    {
		BW_GUARD;

        e1 = v;
        if (v.z != 0.0f)
            e2 = Vector3(-v.y, v.x, 0.0f);
        else
            e2 = Vector3(-v.z, 0.0f, v.x);
        e3 = e1.crossProduct(e2);
        e1.normalise();
        e2.normalise();
        e3.normalise();
    }


    //
    // Find a Matrix m such that m.applyToVector(0,0,0) is v1 and
    // m.applyToVector(1,0,0) is v2.
    //
    // @param v1    Vector 1.
    // @param v2    Vector 2.
    // @param m     This will be set to a matrix that maps the origin to v1 and
    //              (1,0,0) to v2.
    //
    void
    findXForm
    (
        Vector3     const &v1,
        Vector3     const &v2,
        Matrix      &m
    )
    {
		BW_GUARD;

        Vector3 diff = v2 - v1;
        Vector3 e1, e2, e3;
        orthonormalSet(diff, e1, e2, e3);
        m.setIdentity();
        m[0] = e1;
        m[1] = e2;
        m[2] = e3;      
        m[3] = v1;
    }
}


/**
 *  This function intialises a ChunkLinkSegment.
 *
 *  @param start            The start position.
 *  @param end              The end position.
 *  @param startU           The start texture coordinate.
 *  @param endE             The end texture coordinate.
 *  @param thickness        The thickness of the segment.
 *  @param vertexBuffer     The vertex buffer to store mesh vertices in.
 *  @param indexBuffer      The index buffer to store mesh indices in.
 *  @param vertexBase       The offset to start storing vertices.
 *  @param indexBase        The offset to start storing indices.
 *  @param lineVertexBuffer The vertex buffer to store line vertices in.
 *  @param lineIndexBuffer  The index buffer to store line indices in.
 *  @param lineVertexBase   The offset to start storing line vertices.
 *  @param lineIndexBase    The offset to start storing line indices.
 */
ChunkLinkSegment::ChunkLinkSegment
(
    Vector3         const &start,
    Vector3         const &end,
    float           startU,
    float           endU,
    float           thickness,
	Moo::VertexBuffer    vertexBuffer,
    Moo::IndexBuffer indexBuffer,
    uint16          vertexBase,
    uint16          indexBase,
	Moo::VertexBuffer    lineVertexBuffer,
    Moo::IndexBuffer lineIndexBuffer,
    uint16          lineVertexBase,
    uint16          lineIndexBase
) :
    startU_(startU),
    endU_(endU)
{
	BW_GUARD;

    findXForm(start, end, transform_);
    invTransform_ = transform_;
    invTransform_.invert();

    float dist = (end - start).length();

    Vector3 v1 = Vector3(0.0f, -0.5f*thickness, -0.5f*thickness);
    Vector3 v2 = Vector3(dist, +0.5f*thickness, +0.5f*thickness);

    min_ = Vector3(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
    max_ = Vector3(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));   

	// build mesh
	Moo::SimpleVertexLock vl
        (
			vertexBuffer,
            vertexBase*sizeof(VertexType), 
            numberVertices()*sizeof(VertexType),
            0
        );
    ASSERT(vl);
    if (!vl)
        return;
	Moo::IndicesReference indices =
        indexBuffer.lock
		(
            indexBase,
            numberIndices(),
            0
        );
    ASSERT(indices.valid());
    if (!indices.valid())
        return;

    VertexType *verts = (VertexType *)(void*)vl;
    uint16     *inds  = (uint16 *)indices.indices();

    VertexType vert1, vert2, vert3, vert4;


    vert1.normal_.set(0, 0.866025404f, -0.5f);
    vert1.pos_.set(v1.x, v1.y, 0.0f);
    vert1.uv_ .set(startU_, 0.0f);
    vert1.uv2_.set(startU_, 0.0f);
    addVertex(verts++, vert1);  
    vert2.normal_.set(0, 0.866025404f, -0.5f);
    vert2.pos_.set(v2.x, v1.y, 0.0f);
    vert2.uv_ .set(startU_, 1.0f);
    vert2.uv2_.set(startU_, 1.0f);
    addVertex(verts++, vert2);
    vert3.normal_.set(0, 0.866025404f, -0.5f);
    vert3.pos_.set(v1.x, v2.y, v1.z);
    vert3.uv_ .set(endU_, 0.0f);
    vert3.uv2_.set(endU_, 0.0f);
    addVertex(verts++, vert3);
    vert4.normal_.set(0, 0.866025404f, -0.5f);
    vert4.pos_.set(v2.x, v2.y, v1.z);
    vert4.uv_ .set(endU_, 1.0f);
    vert4.uv2_.set(endU_, 1.0f);
    addVertex(verts++, vert4);
    *inds++ = vertexBase;
    *inds++ = vertexBase + 2;
    *inds++ = vertexBase + 1;
    *inds++ = vertexBase + 1;
    *inds++ = vertexBase + 2;
    *inds++ = vertexBase + 3;
    addTriangle(vert3, vert2, vert1);
    addTriangle(vert2, vert3, vert4);

    vertexBase += 4;
    vert1.normal_.set(0, 0.866025404f, 0.5f);
    vert1.pos_.set(v1.x, v1.y, 0.0f);
    vert1.uv_ .set(startU_, 0.0f);
    vert1.uv2_.set(startU_, 0.0f);
    addVertex(verts++, vert1);
    vert2.normal_.set(0, 0.866025404f, 0.5f);
    vert2.pos_.set(v2.x, v1.y, 0.0f);
    vert2.uv_ .set(startU_, 1.0f);
    vert2.uv2_.set(startU_, 1.0f);
    addVertex(verts++, vert2);
    vert3.normal_.set(0, 0.866025404f, 0.5f);
    vert3.pos_.set(v1.x, v2.y, v2.z);
    vert3.uv_ .set(endU_, 0.0f);
    vert3.uv2_.set(endU_, 0.0f);
    addVertex(verts++, vert3);
    vert4.normal_.set(0, 0.866025404f, 0.5f);
    vert4.pos_.set(v2.x, v2.y, v2.z);
    vert4.uv_ .set(endU_, 1.0f);
    vert4.uv2_.set(endU_, 1.0f);
    addVertex(verts++, vert4);
    *inds++ = vertexBase;
    *inds++ = vertexBase + 1;
    *inds++ = vertexBase + 2;
    *inds++ = vertexBase + 1;
    *inds++ = vertexBase + 3;
    *inds++ = vertexBase + 2;
    addTriangle(vert1, vert2, vert3);
    addTriangle(vert4, vert3, vert2);

    vertexBase += 4;
    vert1.normal_.set(0, -1, 0);
    vert1.pos_.set(v1.x, v2.y, v1.z);
    vert1.uv_ .set(startU_, 0.0f);
    vert1.uv2_.set(startU_, 0.0f);
    addVertex(verts++, vert1);
    vert2.normal_.set(0, -1, 0);
    vert2.pos_.set(v2.x, v2.y, v1.z);
    vert2.uv_ .set(startU_, 1.0f);
    vert2.uv2_.set(startU_, 1.0f);
    addVertex(verts++, vert2);
    vert3.normal_.set(0, -1, 0);
    vert3.pos_.set(v1.x, v2.y, v2.z);
    vert3.uv_ .set(endU_, 0.0f);
    vert3.uv2_.set(endU_, 0.0f);
    addVertex(verts++, vert3);
    vert4.normal_.set(0, -1, 0);
    vert4.pos_.set(v2.x, v2.y, v2.z);
    vert4.uv_ .set(endU_, 1.0f);
    vert4.uv2_.set(endU_, 1.0f);
    addVertex(verts++, vert4);
    *inds++ = vertexBase;
    *inds++ = vertexBase + 1;
    *inds++ = vertexBase + 2;
    *inds++ = vertexBase + 1;
    *inds++ = vertexBase + 3;
    *inds++ = vertexBase + 2;
    addTriangle(vert3, vert2, vert1);
    addTriangle(vert2, vert3, vert4);

    HRESULT hr = indexBuffer.unlock();
	ASSERT(SUCCEEDED(hr));


	// build line
	Moo::SimpleVertexLock lvl
        (
			lineVertexBuffer,
            lineVertexBase*sizeof(VertexType), 
            2*sizeof(VertexType),
            0
        );
    ASSERT(lvl);
    if (!lvl)
        return;
	Moo::IndicesReference lindices =
        lineIndexBuffer.lock
		(
            lineIndexBase,
            2,
            0
        );
    ASSERT(lindices.valid());
    if (!lindices.valid())
        return;

    verts = (VertexType *)(void*)lvl;
    inds  = (uint16 *)lindices.indices();

    vert1.normal_.set(0.0f, 1.0f, 0.0f);
    vert1.pos_.set(v1.x, 0.0f, 0.0f);
    vert1.uv_ .set(startU_, 0.0f);
    vert1.uv2_.set(startU_, 0.0f);
    addVertex(verts++, vert1);  
    vert2.normal_.set(0.0f, 1.0f, 0.0f);
    vert2.pos_.set(v2.x, 0.0f, 0.0f);
    vert2.uv_ .set(endU_, 1.0f);
    vert2.uv2_.set(endU_, 1.0f);
    addVertex(verts++, vert2);
    *inds++ = lineVertexBase;
    *inds++ = lineVertexBase + 1;

    hr = lineIndexBuffer.unlock();
	ASSERT(SUCCEEDED(hr));
}


/**
 *  This function should be called before drawing a bunch of ChunkLinkSegments
 *  that all go in the same direction.
 * 
 *  @param rc               The render context.
 *  @param texture          The texture to draw with.
 *  @param effectMaterial   The effect to draw with.
 *  @param time             The current time.
 *  @param vSpeed           The speed to scroll the texture.
 *  @param direction        The direction of the segments.
 */
/*static*/ bool 
ChunkLinkSegment::beginDrawSegments
(
    Moo::RenderContext      &rc,
    DX::BaseTexture         *texture,
    Moo::EffectMaterialPtr	effectMaterial,
    float                   time,
    float                   vSpeed,
    ChunkLink::Direction    direction,
	const Matrix&			worldTransform
)
{
	BW_GUARD;

    DX::Device *device = rc.device();

    Moo::rc().effectVisualContext().initConstants();
    ComObjectWrap<ID3DXEffect> effect = effectMaterial->pEffect()->pEffect();

    Matrix WVP = worldTransform;
    WVP.postMultiply(rc.view());
    WVP.postMultiply(rc.projection());
    effect->SetMatrix("worldViewProjection", &WVP);

    float sgn1 = (direction & ChunkLink::DIR_START_END) != 0 ? -1.0f : +1.0f;
    float sgn2 = (direction & ChunkLink::DIR_END_START) != 0 ? +1.0f : -1.0f;

    effect->SetTexture("patrolTexture", texture);
    effect->SetFloat("vOffset1", sgn1*time*vSpeed);
    effect->SetFloat("vOffset2", sgn2*time*vSpeed);

    if (effectMaterial->begin())
	{
		MF_ASSERT( effectMaterial->beginPass(0) );
		return true;
	}
	else
	{
		// TODO: find why this happens only once sometimes.
		DEBUG_MSG( "ChunkLinkSegment::beginDrawSegments: Call to EffectMaterial::begin() failed.\n");
	}
	return false;
}


/**
 *  This is called to draw a bunch of ChunkLinkSegments.
 *
 *  @param rc               The rendering context.
 *  @param vertexBuffer     The vertex buffer common to the segments.
 *  @param indexBuffer      The index buffer common to the segments.
 *  @param lineVertexBuffer The line vertex buffer common to the segments.
 *  @param lineIndexBuffer  The line index buffer common to the segments.
 *  @param number           The number of segments to draw.
 */
/*static*/ void 
ChunkLinkSegment::draw
( 
    Moo::RenderContext      &rc,
	Moo::VertexBuffer       vertexBuffer,
    Moo::IndexBuffer        indexBuffer,
	Moo::VertexBuffer       lineVertexBuffer,
    Moo::IndexBuffer        lineIndexBuffer,
    unsigned int            number
)
{
	BW_GUARD;

    DX::Device* pDev = rc.device();

	// mesh
	vertexBuffer.set( 0, 0, sizeof(VertexType) );
    rc.setFVF(VertexType::fvf());
	indexBuffer.set();
    rc.drawIndexedPrimitive
    ( 
        D3DPT_TRIANGLELIST, 
        0, 
        0, 
        number*numberVertices(),
        0, 
        number*numberTriangles()
    );

	//line
	lineVertexBuffer.set( 0, 0, sizeof(VertexType) );
	lineIndexBuffer.set();
	rc.drawIndexedPrimitive
	( 
		D3DPT_LINELIST, 
		0, 
		0, 
		number*2,
		0, 
		number
	);
}


/**
 *  This is useful to draw triangles around the ChunkLinkSegment, for 
 *  debugging.
 *
 *  @param rc               The rendering context.
 */
void ChunkLinkSegment::drawTris(Moo::RenderContext &/*rc*/) const
{
	BW_GUARD;

    for (size_t i = 0; i < triangles_.size(); ++i)
    {
        Vector3 v0 = transform_.applyPoint(triangles_[i].v0());
        Vector3 v1 = transform_.applyPoint(triangles_[i].v1());
        Vector3 v2 = transform_.applyPoint(triangles_[i].v2());
        Vector3 n;
        n.crossProduct((v1 - v0), (v2 - v0));
        n.normalise();
        n = 0.1f*n;
        Vector3 c = (v0 + v1 + v2)/3.0f;
        Geometrics::drawLine
        (
            v0, v1,
            Moo::Colour(1.0f, 0.0f, 0.0f, 1.0f)
        );
        Geometrics::drawLine
        (
            v1, v2,
            Moo::Colour(1.0f, 0.0f, 0.0f, 1.0f)
        );
        Geometrics::drawLine
        (
            v2, v0,
            Moo::Colour(1.0f, 0.0f, 0.0f, 1.0f)
        );
        Geometrics::drawLine
        (
            c, c + n,
            Moo::Colour(1.0f, 0.0f, 0.0f, 1.0f)
        );
    }
}


/**
 *  This is used to signify the end of drawing of a bunch of ChunkLinkSegments.
 * 
 *  @param rc               The render context.
 *  @param texture          The texture to draw with.
 *  @param effectMaterial   The effect to draw with.
 *  @param time             The current time.
 *  @param vSpeed           The speed to scroll the texture.
 *  @param direction        The direction of the segments.
 */
/*static*/ void 
ChunkLinkSegment::endDrawSegments
(
    Moo::RenderContext      &rc,
    DX::BaseTexture         * /*texture*/,
    Moo::EffectMaterialPtr	effectMaterial,
    float                   /*time*/,
    float                   /*vSpeed*/,
    ChunkLink::Direction    /*direction*/
)
{
	BW_GUARD;

    effectMaterial->endPass();
    effectMaterial->end();    
}


/**
 *  This is used to determine a hit test for the ChunkLinkSegment.
 *
 *  @param start            The start of the hit test.
 *  @param dir              The direction of the hit test.
 *  @param t_value          The t-value that the collision occured.
 *  @param wt               The hit triangle.
 *
 *  @return                 True if hit, false otherwise.
 */
bool ChunkLinkSegment::intersects
(
    Vector3         const &start,
    Vector3         const &dir,
    float           &t_value,
    WorldTriangle   &wt
) const
{
	BW_GUARD;

    Vector3 lo = invTransform_.applyPoint(start);
    Vector3 ld = invTransform_.applyVector(dir);

    float dist = std::numeric_limits<float>::max();
    int hitIdx = -1;
    for (size_t i = 0; i < triangles_.size(); ++i)
    {
        float thisDist = std::numeric_limits<float>::max();
        if (triangles_[i].intersects(lo, ld, thisDist))
        {
			if ( triangles_[i].normal().dotProduct( ld ) <= 0.0f &&
				 thisDist < dist)
            {
                dist = thisDist;
                hitIdx = (int)i;
            }
        }
    }

    if (hitIdx != -1)
    {
        Vector3 hitPt = transform_.applyPoint(lo + dist*ld);
        if (dir.x != 0)
        {
            t_value = (hitPt.x - start.x)/dir.x;
        }
        else if (dir.y != 0)
        {
            t_value = (hitPt.y - start.y)/dir.y;
        }
        else
        {
            t_value = (hitPt.z - start.z)/dir.z;
        }
        Vector3 v0 = transform_.applyPoint(triangles_[hitIdx].v0());
        Vector3 v1 = transform_.applyPoint(triangles_[hitIdx].v1());
        Vector3 v2 = transform_.applyPoint(triangles_[hitIdx].v2());
        wt = WorldTriangle(v0, v1, v2);
        return true;
    }
    else
    {
        return false;
    }
}


/**
 *  This returns the number of vertices per segment.
 */
/*static*/ unsigned int ChunkLinkSegment::numberVertices()
{
    return 12;
}


/**
 *  This returns the number of indices per segment.
 */
/*static*/ unsigned int ChunkLinkSegment::numberIndices()
{
    return 18;
}


/**
 *  This returns the number of indices per segment.
 */
/*static*/ unsigned int ChunkLinkSegment::numberTriangles()
{
    return 6;
}


/**
 *  Transform and add a point to the vertex buffer.
 */
void ChunkLinkSegment::addVertex
(
    VertexType      *buffer, 
    VertexType      const &v
)
{
	BW_GUARD;

    *buffer = v;
    buffer->pos_ = transform_.applyPoint(buffer->pos_);
    buffer->normal_ = transform_.applyVector(buffer->normal_);
}


/**
 *  Add a triangle to our triangles for hit testing.
 *
 *  @param v1           Vertex 1.
 *  @param v2           Vertex 2.
 *  @param v3           Vertex 3.
 */
void 
ChunkLinkSegment::addTriangle
(
    VertexType          const &v1_,
    VertexType          const &v2_,
    VertexType          const &v3_
)
{
	BW_GUARD;

    Vector3 v1 = v1_.pos_;
    Vector3 v2 = v2_.pos_;
    Vector3 v3 = v3_.pos_;
    v1.y *= SELECTION_EXPANSION;
    v1.z *= SELECTION_EXPANSION;
    v2.y *= SELECTION_EXPANSION;
    v2.z *= SELECTION_EXPANSION;
    v3.y *= SELECTION_EXPANSION;
    v3.z *= SELECTION_EXPANSION;
    WorldTriangle wt(v1, v2, v3);
    triangles_.push_back(wt);
}
BW_END_NAMESPACE

