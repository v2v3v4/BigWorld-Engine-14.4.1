#ifndef PY_POINT_SPRITE_TRANSFER_MESH_HPP
#define PY_POINT_SPRITE_TRANSFER_MESH_HPP

#include "filter_quad.hpp"
#include "filter_quad_factory.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/vertex_buffer_wrapper.hpp"
#include "moo/vertex_formats.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	//{u offset, v offset, weight}
	typedef Vector3 FilterSample;

	/**
	 *	This class implements a FilterQuad that renders
	 *	every pixel on the screen as point sprites
	 */
	class PyPointSpriteTransferMesh : public FilterQuad
	{
		Py_Header( PyPointSpriteTransferMesh, FilterQuad )
		DECLARE_FILTER_QUAD( PyPointSpriteTransferMesh )
	public:
		PyPointSpriteTransferMesh( PyTypeObject *pType = &s_type_ );
		~PyPointSpriteTransferMesh();

		void preDraw( Moo::EffectMaterialPtr pMat )	{};
		void draw();
		bool save( DataSectionPtr );
		bool load( DataSectionPtr );

#ifdef EDITOR_ENABLED
		void edEdit( GeneralEditor * editor, FilterChangeCallback pCallback ) { /*nothing to do*/ }
#endif // EDITOR_ENABLED

		PY_FACTORY_DECLARE()
	private:
		void buildMesh();
		uint32 nPixels_;
		Moo::VertexDeclaration* pDecl_;
		typedef SmartPointer< Moo::VertexBufferWrapper< Moo::VertexXYZUV > >
			VertexBufferWrapperPtr;
		VertexBufferWrapperPtr pVertexBuffer_;
	};
};

#ifdef CODE_INLINE
#include "py_point_sprite_transfer_mesh.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_POINT_SPRITE_TRANSFER_MESH_HPP
