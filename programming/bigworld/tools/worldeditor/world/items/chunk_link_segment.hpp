#ifndef CHUNK_LINK_SEGMENT_HPP
#define CHUNK_LINK_SEGMENT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_link.hpp"
#include "moo/index_buffer.hpp"
#include "moo/vertex_buffer.hpp"
#include "moo/vertex_formats.hpp"
#include "physics2/worldtri.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class represents a segment in an EditorChunkLink.
 */
class ChunkLinkSegment : public ReferenceCount
{
public:
    typedef Moo::VertexXYZNUV2                  VertexType;

    ChunkLinkSegment
    (
        Vector3                 const &start,
        Vector3                 const &end,
        float                   startU,
        float                   endU,
        float                   thickness,
		Moo::VertexBuffer       vertexBuffer,
		Moo::IndexBuffer        indexBuffer,
        uint16                  vertexBase,
        uint16                  indexBase,
		Moo::VertexBuffer       lineVertexBuffer,
		Moo::IndexBuffer        lineIndexBuffer,
        uint16                  lineVertexBase,
        uint16                  lineIndexBase
    );

    static bool 
    beginDrawSegments
    (
        Moo::RenderContext      &rc,
        DX::BaseTexture         *texture,
        Moo::EffectMaterialPtr	effectMaterial,
        float                   time,
        float                   vSpeed,
        ChunkLink::Direction    direction,
		const Matrix&			worldTransform = Matrix::identity
    );

    static void draw
    (
        Moo::RenderContext      &rc,
		Moo::VertexBuffer       vertexBuffer,
        Moo::IndexBuffer        indexBuffer,
		Moo::VertexBuffer       lineVertexBuffer,
        Moo::IndexBuffer        lineIndexBuffer,
        unsigned int            numSegments
    );

    void drawTris
    (
        Moo::RenderContext      &rc
    ) const;

    static void 
    endDrawSegments
    (
        Moo::RenderContext      &rc,
        DX::BaseTexture         *texture,
        Moo::EffectMaterialPtr	effectMaterial,
        float                   time,
        float                   vSpeed,
        ChunkLink::Direction    direction
    );

    bool intersects
    (
        Vector3                 const &start,
        Vector3                 const &dir,
        float                   &t,
        WorldTriangle           &tri
    ) const;

    static unsigned int numberVertices();

    static unsigned int numberIndices();

    static unsigned int numberTriangles();

private:
    void addVertex(VertexType *buffer, VertexType const &v);

    void 
    addTriangle
    (
        VertexType                  const &v1,
        VertexType                  const &v2,
        VertexType                  const &v3
    );

    ChunkLinkSegment(ChunkLinkSegment const &);
    ChunkLinkSegment &operator=(ChunkLinkSegment const &);

private:           
    Matrix                          transform_;
    Matrix                          invTransform_;
    Vector3                         min_;
    Vector3                         max_;
    BW::vector<WorldTriangle>      triangles_;
    float                           startU_;
    float                           endU_;
};


typedef SmartPointer<ChunkLinkSegment>  ChunkLinkSegmentPtr;

BW_END_NAMESPACE

#endif // CHUNK_LINK_SEGMENT_HPP
