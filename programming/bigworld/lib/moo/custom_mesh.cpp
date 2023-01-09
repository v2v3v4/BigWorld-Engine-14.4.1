#include "pch.hpp"
#include "custom_mesh.hpp"
#include "moo/render_context.hpp"

BW_BEGIN_NAMESPACE

StaticMesh::StaticMesh() :
	numVertices_( 0 )
{
}


void StaticMesh::unlock()
{
	vertexBuffer_.unlock();
}


void StaticMesh::release()
{
	numVertices_ = 0;
	vertexBuffer_.release();
}


HRESULT StaticMesh::drawEffect( Moo::VertexDeclaration * vecDecl /*= NULL*/ )
{
	uint32 numPrimitives = Detail::nPrimitives( primitiveType_, numVertices_ );
	if (numPrimitives)
	{
		if (vecDecl)
		{
			Moo::rc().setVertexDeclaration( vecDecl->declaration() );
		}
		else
		{
			Moo::rc().setFVF( fvf_ );
		}

		if (SUCCEEDED( vertexBuffer_.set( 0, 0, stride_ ) ))
		{
			HRESULT res = Moo::rc().drawPrimitive( primitiveType_, 0,
				Detail::nPrimitives( primitiveType_, numVertices_ ) );
			vertexBuffer_.reset( 0 );
			return res;
		}
	}
	return S_OK;
}

BW_END_NAMESPACE

// custom_mesh.cpp
