#ifndef PY_FILTER_QUAD_HPP
#define PY_FILTER_QUAD_HPP

#include "filter_quad.hpp"
#include "filter_quad_factory.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/custom_mesh.hpp"
#include "moo/dynamic_vertex_buffer.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	//{u offset, v offset, weight}
	typedef Vector3 FilterSample;

	/**
	 *	This class implements a generic FilterQuad for use
	 *	by PyPhase objects.
	 */
	class PyFilterQuad : public FilterQuad
	{
		Py_Header( PyFilterQuad, FilterQuad )
		DECLARE_FILTER_QUAD( PyFilterQuad )
	public:
		typedef BW::vector<Vector3> Samples;

		PyFilterQuad( PyTypeObject *pType = &s_type_ );
		~PyFilterQuad();

		void preDraw( Moo::EffectMaterialPtr pMat );
		void draw();
		bool save( DataSectionPtr );
		bool load( DataSectionPtr );

#ifdef EDITOR_ENABLED
		void edEdit( GeneralEditor * editor, FilterChangeCallback pCallback );
		BW::string edGetSrcTexture() const;
		bool edSetSrcTexture( const BW::string & textureName );
		Samples & getSamples();
		void setSamples( const Samples & samples );
		void onSampleChanged();
#endif // EDITOR_ENABLED

		PY_RW_ATTRIBUTE_DECLARE( samplesHolder_, samples )

		PY_FACTORY_DECLARE()
	private:
		Samples samples_;
		PySTLSequenceHolder<Samples> samplesHolder_;

		void addVert( Moo::VertexXYZNUV2& v, FilterSample* rSample, size_t tapSize, const Vector2& srcDim );
		void buildMesh();

		CustomMesh< Moo::FourTapVertex > verts4tap_;
		CustomMesh< Moo::EightTapVertex > verts8tap_;

		Moo::VertexDeclaration* pDecl4tap_;
		Moo::VertexDeclaration* pDecl8tap_;

#ifdef EDITOR_ENABLED
		FilterChangeCallback pCallback_;
#endif // EDITOR_ENABLED
	};
};

#ifdef CODE_INLINE
#include "py_filter_quad.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_FILTER_QUAD_HPP
