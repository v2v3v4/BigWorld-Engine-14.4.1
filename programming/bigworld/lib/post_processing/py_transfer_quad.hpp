#ifndef PY_TRANSFER_QUAD_HPP
#define PY_TRANSFER_QUAD_HPP

#include "filter_quad.hpp"
#include "filter_quad_factory.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/custom_mesh.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	/**
	 *	This class implements a simple FilterQuad for use
	 *	by PyPhase objects that uses 1 sample and no offsets.
	 */
	class PyTransferQuad : public FilterQuad
	{
		Py_Header( PyTransferQuad, FilterQuad )
		DECLARE_FILTER_QUAD( PyTransferQuad )
	public:
		PyTransferQuad( PyTypeObject *pType = &s_type_ );
		~PyTransferQuad();

		void preDraw( Moo::EffectMaterialPtr pMat )	{};
		void draw();
		bool save( DataSectionPtr );
		bool load( DataSectionPtr );

#ifdef EDITOR_ENABLED
		void edEdit( GeneralEditor * editor, FilterChangeCallback pCallback ) { /*nothing to do*/ }
#endif // EDITOR_ENABLED

		PY_FACTORY_DECLARE()
	private:
		void addVert( Moo::VertexXYZNUV2& v );
		void buildMesh();

		CustomMesh< Moo::FourTapVertex > verts4tap_;
		Moo::VertexDeclaration* pDecl4tap_;
	};
};

#ifdef CODE_INLINE
#include "py_transfer_quad.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_TRANSFER_QUAD_HPP
