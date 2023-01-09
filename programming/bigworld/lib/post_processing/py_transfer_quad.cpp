#include "pch.hpp"
#include "py_transfer_quad.hpp"
#include "moo/render_context.hpp"
#include "moo/base_texture.hpp"
#include "moo/texture_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_transfer_quad.ipp"
#endif

namespace PostProcessing
{

// Python statics
PY_TYPEOBJECT( PyTransferQuad )

PY_BEGIN_METHODS( PyTransferQuad )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyTransferQuad )
PY_END_ATTRIBUTES()

/*~ class PostProcessing.PyTransferQuad
 *	@components{ client, tools }
 *	A PyTransferQuad can be used by a PyPhase object, and provides the geometry
 *	for the phase.  The TransferQuad fills out 4 texture coordinates for its
 *	vertices, but each uv coordinate is not offset at all.
 */
/*~ function PostProcessing.TransferQuad
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing PyTransferQuad object.
 *	@return A new PostProcessing PyTransferQuad object.
 */
PY_FACTORY_NAMED( PyTransferQuad, "TransferQuad", _PostProcessing )

IMPLEMENT_FILTER_QUAD( PyTransferQuad, PyTransferQuad )


PyTransferQuad::PyTransferQuad( PyTypeObject *pType ) :
	FilterQuad( pType ),
	pDecl4tap_()
{
}


PyTransferQuad::~PyTransferQuad()
{
}


/**
 *	This method adds a single vertex to the filter quad.
 *	@param	v		XYZNUV2 source vertex
 */
void PyTransferQuad::addVert( Moo::VertexXYZNUV2& v )
{
	Moo::FourTapVertex curr;
	curr.pos_ = v.pos_;
	for (size_t t=0; t<4; t++)
	{
		curr.uv_[t][0] = v.uv2_[0];
		curr.uv_[t][1] = v.uv2_[1];
		curr.uv_[t][2] = ( t==0 ) ? 1.f : 0.f;
	}

	Matrix view = Moo::rc().invView();
	view.translation(Vector3(0,0,0));
	curr.viewNormal_ = Moo::rc().camera().nearPlanePoint( curr.pos_.x, curr.pos_.y );
	curr.viewNormal_.normalise();		
	curr.worldNormal_ = view.applyVector( curr.viewNormal_ );

	verts4tap_.push_back(curr);
}


/**
 *	This method builds the mesh used to render with.
 */
void PyTransferQuad::buildMesh()
{	
	pDecl4tap_ = Moo::VertexDeclaration::get( "xyzuvw4" );

	float w = 1.f;
	float h = 1.f;
	Vector3 fixup( -0.5f / Moo::rc().halfScreenWidth(), 0.5f / Moo::rc().halfScreenHeight(), 0.f );

	Moo::VertexXYZNUV2 v[4];

	v[0].pos_.set(-1.f,-1.f,0.1f);
	v[0].uv_.set(0.f,0.f);
	v[0].uv2_.set(0.f,h);

	v[1].pos_.set(-1.f,1.f,0.1f);
	v[1].uv_.set(0.f,1.f);
	v[1].uv2_.set(0.f,0.f);	

	v[2].pos_.set(1.f,1.f,0.1f);
	v[2].uv_.set(1.f,1.f);
	v[2].uv2_.set(w,0.f);	

	v[3].pos_.set(1.f,-1.f,0.1f);
	v[3].uv_.set(1.f,0.f);
	v[3].uv2_.set(w,h);

	for (uint32 i=0; i<4; i++)
	{
		v[i].pos_ = v[i].pos_ + fixup;
	}

	//Now we have our 4 vertices that give us a full-screen quad.
	verts4tap_.clear();
	this->addVert( v[0] );
	this->addVert( v[1] );
	this->addVert( v[2] );
	this->addVert( v[0] );
	this->addVert( v[2] );
	this->addVert( v[3] );
}


/**
 *	This method draws the transfer quad using the current device state.
 */
void PyTransferQuad::draw()
{
	this->buildMesh();
	verts4tap_.drawEffect( pDecl4tap_ );
}


bool PyTransferQuad::save( DataSectionPtr pDS )
{
	DataSectionPtr pSect = pDS->newSection( "PyTransferQuad" );
	return true;
}


bool PyTransferQuad::load( DataSectionPtr pSect )
{
	return true;
}

PyObject* PyTransferQuad::pyNew(PyObject* args)
{
	return new PyTransferQuad;
}

} // namespace PostProcessing

BW_END_NAMESPACE

// py_transfer_quad.cpp
