#ifndef CUSTOM_MESH_HPP
#define CUSTOM_MESH_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/vectornodest.hpp"
#include "moo/moo_dx.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/dynamic_vertex_buffer.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This templatised class gives the ability to
 *	create a custom mesh.
 *
 *	Template vertex format type must have an
 *	int fvf() method.
 */
template <class T>
class CustomMesh : public VectorNoDestructor< T >
{
public:
	explicit CustomMesh( D3DPRIMITIVETYPE primitiveType = D3DPT_TRIANGLELIST );
	~CustomMesh();

	D3DPRIMITIVETYPE	primitiveType() const	{ return primitiveType_; }

	int			vertexFormat() const { return T::fvf(); }
	int			nVerts() const
	{
		size_t nVerts = size();
		MF_ASSERT( nVerts <= INT_MAX );
		return ( int )nVerts;
	}

	HRESULT		draw();
	HRESULT		drawEffect( Moo::VertexDeclaration* decl = NULL );
	HRESULT		drawRange( uint32 from, uint32 to );

private:
	CustomMesh( const CustomMesh& );
	CustomMesh& operator=( const CustomMesh& );

	D3DPRIMITIVETYPE primitiveType_;
};


class StaticMesh
{
public:

	StaticMesh();

	template < typename VertexType >
	VertexType * lock( D3DPRIMITIVETYPE primitiveType, uint32 numVertices );

	void unlock();

	void release();

	HRESULT drawEffect( Moo::VertexDeclaration * decl = NULL );

private:
	StaticMesh( const StaticMesh& );
	StaticMesh& operator=( const StaticMesh& );

	D3DPRIMITIVETYPE primitiveType_;
	uint32 fvf_;
	uint32 numVertices_;
	uint32 stride_;
	Moo::VertexBuffer vertexBuffer_;
};

#include "custom_mesh.ipp"

BW_END_NAMESPACE

#endif // CUSTOM_MESH_HPP
