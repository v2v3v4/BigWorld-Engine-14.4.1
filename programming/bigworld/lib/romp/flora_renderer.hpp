#ifndef FLORA_RENDERER_HPP
#define FLORA_RENDERER_HPP


#include "flora_vertex_type.hpp"
#include "animation_grid.hpp"
#include "effect_parameter_cache.hpp"
#include "moo/vertex_buffer.hpp"


BW_BEGIN_NAMESPACE

class EnviroMinder;
class FloraBlock;
class Flora;
class FloraVertexContainer;
class Ecotype;

namespace Moo
{
	class VertexDeclaration;
}
/**
 * This class defines the vertex container interface for the flora, it's used
 * to abstract the vertices for different device types. It works by passing
 * through intermediate vertices, which get translated to whatever format the
 * renderer uses internally.
 */
class FloraVertexContainer
{
public:
	FloraVertexContainer( FloraVertex* pVerts = NULL, uint32 nVerts = 0 );
	void addVertices( const FloraVertex* vertices, uint32 count, const Matrix* pTransform );
	void clear( uint32 nVertices );
	void init( FloraVertex* pVerts, uint32 nVerts );
	void blockNum( int blockNum ) { blockNum_ = blockNum; };
	void uvOffset( float uOffset, float vOffset ) { uOffset_ = (UV_TYPE)(uOffset * PACK_UV); vOffset_ = (UV_TYPE)(vOffset * PACK_UV); };
protected:
	FloraVertex* pVertsBase_;
	FloraVertex* pVerts_;
	uint32 nVerts_;
	int blockNum_;
	UV_TYPE uOffset_;
	UV_TYPE vOffset_;

#ifdef EDITOR_ENABLED
public:
	FloraBlock* blocks_;
	FloraBlock* pCurBlock_;
	Ecotype* pCurEcotype_;

	uint32 curOffset() { return uint32(pVerts_ - pVertsBase_); }
	FloraVertex* pVerts( uint32 offset ) { return pVertsBase_ + offset; }
	void offsetUV( FloraVertex*& pVerts );
#endif
};


/**
 * This class defines the interface and implements common methods for the flora renderer
 */
class FloraRenderer : public Moo::DeviceCallback
{
public:
	FloraRenderer( Flora* flora );
	~FloraRenderer();

	bool init( uint32 bufferSize );	
	void fini( );	

	FloraVertexContainer* lock( uint32 firstVertex, uint32 nVertices );
	bool unlock( FloraVertexContainer* );

	void update( float dTime, EnviroMinder& enviro );	

	bool preDraw(EnviroMinder& enviro, float tineStart, float tineEnd);
	void beginAlphaTestPass( float drawDistance, float fadePercent, uint32 alphaTestRef );
	void drawBlock( uint32 firstVertex, uint32 nVertices,
		const FloraBlock& block );
	void postDraw();	

	uint32 vertexSize() { return sizeof(FloraVertex); };
	uint32 capacity();

	//methods inherited from DeviceCallback
	void deleteUnmanagedObjects( );
	void createUnmanagedObjects( );

	void deactivate();
	void activate();
protected:
	bool createVB( uint32 bufferSize, DWORD fvf, DWORD lockFlags );
	void delVB();
	void clear( uint32 offset = 0, uint32 size = 0 );

	DWORD fvf_;
	DWORD lockFlags_;
	uint32 capacity_;
	Moo::EffectMaterialPtr material_;
	Moo::EffectMaterialPtr shadowMaterial_;
	Moo::VertexDeclaration* pDecl_;
	BYTE* pLocked_;
	FloraVertexContainer container_;
	FloraVertexContainer* pContainer_;	
	FloraAnimationLocal		m_localAnimation;
	FloraAnimationGlobal	m_globalAnimation;
	Moo::VertexBuffer	pVB_;
	DWORD bufferSize_;
	EffectParameterCache* parameters_;
	EffectParameterCache  standardParams_;
	EffectParameterCache  shadowParams_;
	int lastBlockID_;

	Flora* flora_;
};

BW_END_NAMESPACE

#endif // FLORA_RENDERER_HPP
