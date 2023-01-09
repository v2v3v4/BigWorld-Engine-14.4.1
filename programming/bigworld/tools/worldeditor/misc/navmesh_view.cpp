#include "pch.hpp"

#include "navmesh_view.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/geometry_mapping.hpp"

#include "navigation_recast/recast_generator.hpp"

BW_BEGIN_NAMESPACE

void NavmeshView::preDraw( float size )
{
	BW_GUARD;

	Moo::rc().pushRenderState( D3DRS_ZFUNC );
	Moo::rc().pushRenderState( D3DRS_CULLMODE );

	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setTexture( 0, NULL );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_POINTSIZE, *((DWORD*)&size) );

	Moo::Material::setVertexColour();

	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_CURRENT );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
}


void NavmeshView::postDraw()
{
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
}


template<typename T> T pop( const unsigned char* buffer, int* offset )
{
	*offset += sizeof( T );

	return *(T*)( buffer + *offset - sizeof( T ) );
}


void NavmeshView::populateGirth( const unsigned char* navmesh, int* offset,
	float yBias )
{
	pop<int>( navmesh, offset ); // skip version

	float girth = pop<float>( navmesh, offset );
	int meshCount = pop<int>( navmesh, offset );
	int edgeCount = pop<int>( navmesh, offset );
	RenderSet renderSet;

	BW::vector<Moo::VertexXYZL> edgeVertices;
	BW::vector<Moo::VertexXYZL> triangleVertices;
	Mesh* meshes = (Mesh*)( navmesh + *offset );
	Edge* edges = (Edge*)( navmesh + *offset
		+ meshCount * sizeof( Mesh ) );

	*offset += meshCount * sizeof( Mesh );

	for (int i = 0; i < meshCount; ++i)
	{
		Moo::VertexXYZL first;
		Moo::VertexXYZL prev;
		float maxHeight = meshes->maxHeight_ + yBias;

		first.pos_ = Vector3( edges[0].x_, maxHeight, edges[0].z_ );
		prev.pos_ = Vector3( edges[1].x_, maxHeight, edges[1].z_ );

		first.colour_ = 0xffffffff;
		prev.colour_ = 0xffffffff;

		edgeVertices.push_back( first );
		edgeVertices.push_back( prev );

		for (int edge = 2; edge < meshes->edgeCount_; ++edge)
		{
			Moo::VertexXYZL current;

			current.pos_ = Vector3( edges[ edge ].x_,
				maxHeight, edges[ edge ].z_ );
			current.colour_ = 0x660000ff;

			edgeVertices.push_back( prev );
			edgeVertices.push_back( current );

			first.colour_ = 0x660000ff;
			prev.colour_ = 0x660000ff;

			triangleVertices.push_back( first );
			triangleVertices.push_back( prev );
			triangleVertices.push_back( current );

			prev = current;
		}

		first.colour_ = 0xffffffff;
		prev.colour_ = 0xffffffff;

		edgeVertices.push_back( prev );
		edgeVertices.push_back( first );

		*offset += meshes->edgeCount_ * sizeof( Edge );
		edges += meshes->edgeCount_;
		++meshes;
	}

	HRESULT hr1 = renderSet.edges_.create<Moo::VertexXYZL>(
		static_cast<uint>(edgeVertices.size()), 0, D3DPOOL_MANAGED );

	HRESULT hr2 = renderSet.triangles_.create<Moo::VertexXYZL>(
		static_cast<uint>(triangleVertices.size()), 0, D3DPOOL_MANAGED );

	if (SUCCEEDED( hr1 ) && SUCCEEDED( hr2 ))
	{
		Moo::VertexLock<Moo::VertexXYZL> edges( renderSet.edges_ );
		Moo::VertexLock<Moo::VertexXYZL> triangles( renderSet.triangles_ );

		edges.fill( &edgeVertices[0], static_cast<uint32>(edgeVertices.size()
			* sizeof( Moo::VertexXYZL )) );
		triangles.fill( &triangleVertices[0], static_cast<uint32>(triangleVertices.size()
			* sizeof( Moo::VertexXYZL )) );

		renderSet.edgeCount_ = (UINT)edgeVertices.size() / 2;
		renderSet.triangleCount_ = (UINT)triangleVertices.size() / 3;

		girthRenderSets_[ girth ].push_back( renderSet );
	}
}


NavmeshView::NavmeshView( const BW::vector<unsigned char>& navmesh, float yBias )
	: active_( false ), currentGirth_( 0.f )
{
	for (int offset = 0; offset < (int)navmesh.size(); )
	{
		populateGirth( &navmesh[0], &offset, yBias );
	}
}


void NavmeshView::addToChannel( Moo::DrawContext& drawContext, const Vector3& centre, float girth )
{
	BW_GUARD;

	if (girthRenderSets_.find( girth ) != girthRenderSets_.end()
		&& !active_ && !EditorChunkItem::drawSelection())
	{
		currentGirth_ = girth;
		active_ = true;
		float distance = ( centre - Moo::rc().invView().applyToOrigin() ).length();
		drawContext.drawUserItem( this, Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );
	}
}


void NavmeshView::draw()
{
	BW_GUARD;

	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	preDraw( 1.f );

	for (RenderSets::const_iterator iter = girthRenderSets_[ currentGirth_ ].begin();
		iter != girthRenderSets_[ currentGirth_ ].end(); ++iter)
	{
		iter->triangles_.set( 0, 0, sizeof( Moo::VertexXYZL ) );
		Moo::rc().drawPrimitive( D3DPT_TRIANGLELIST, 0, iter->triangleCount_ );
		iter->edges_.set( 0, 0, sizeof( Moo::VertexXYZL ) );
		Moo::rc().drawPrimitive( D3DPT_LINELIST, 0, iter->edgeCount_ );
	}

	postDraw();

	Moo::rc().pop();
}


void NavmeshView::fini()
{
	active_ = false;
}


void RecastDebugView::render()
{
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	BW::map<Chunk*, Moo::VertexBuffer>::iterator iter;

	for (iter = chunkVBs_.begin(); iter != chunkVBs_.end(); ++iter)
	{
		iter->second.set( 0, 0, sizeof( Moo::VertexXYZL ) );

		Moo::rc().drawPrimitive( D3DPT_TRIANGLELIST, 0, iter->second.size() / sizeof( Moo::VertexXYZL ) );
	}
}


void RecastDebugView::add( Chunk* chunk )
{
	if (chunkVBs_.find( chunk ) == chunkVBs_.end())
	{
		BW::vector<Vector3> triangles;
		BW::vector<Moo::VertexXYZL> vertices;
		Moo::VertexBuffer vertexBuffer;

		RecastGenerator::collectTriangles( chunk ).appendCollisionTriangleList( triangles );

		vertices.reserve( triangles.size() );

		DWORD colours[] = { 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0xffffffff };
		int colourIndex = 0;
		BW::vector<Vector3>::iterator iter = triangles.begin();

		while (iter != triangles.end())
		{
			Moo::VertexXYZL v;

			v.pos_ = *iter;
			v.colour_ = colours[ colourIndex ];

			colourIndex = ( colourIndex + 1 ) % ( sizeof( colours ) / sizeof( *colours ) );

			vertices.push_back( v );

			++iter;
		}

		HRESULT hr = vertexBuffer.create( static_cast<uint32>(vertices.size() * sizeof( Moo::VertexXYZL )),
			0, Moo::VertexXYZL::fvf(), D3DPOOL_MANAGED );

		if (SUCCEEDED( hr ))
		{
			Moo::VertexLock<Moo::VertexXYZL> lock( vertexBuffer );

			lock.fill( &vertices[0], static_cast<uint32>(vertices.size() * sizeof( Moo::VertexXYZL )) );

			chunkVBs_[ chunk ] = vertexBuffer;
		}
	}
}


void RecastDebugView::remove( Chunk* chunk )
{
	if (chunkVBs_.find( chunk ) == chunkVBs_.end())
	{
		chunkVBs_.erase( chunkVBs_.find( chunk ) );
	}
}

BW_END_NAMESPACE

