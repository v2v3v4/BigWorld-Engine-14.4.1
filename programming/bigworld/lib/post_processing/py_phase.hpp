#ifndef PY_PHASE_HPP
#define PY_PHASE_HPP

#include "phase.hpp"
#include "phase_factory.hpp"
#include "filter_quad.hpp"
#include "romp/py_material.hpp"
#include "romp/py_render_target.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	#ifdef EDITOR_ENABLED
	class PyPhaseEditor;
	typedef SmartPointer< PyPhaseEditor > PyPhaseEditorPtr;
	#endif // EDITOR_ENABLED


	/**
	 *	This class derives from PostProcessing::Phase, and
	 *	provides a generic phase designed to be configured
	 *	entirely via python.
	 *
	 *	It draws a FilterQuad out to a RenderTarget using a
	 *	material.
	 */
	class PyPhase : public Phase
	{
		Py_Header( PyPhase, Phase )
		DECLARE_PHASE( PyPhase )
	public:
		PyPhase( PyTypeObject *pType = &s_type_ );
		~PyPhase();

		void tick( float dTime );
		void draw( class Debug*, RECT* = NULL );

		bool load( DataSectionPtr );
		bool save( DataSectionPtr );

		bool renderTargetFromString( const BW::StringRef & resourceID );

#ifdef EDITOR_ENABLED
		void edChangeCallback( PhaseChangeCallback pCallback );	
		void edEdit( GeneralEditor * editor );

		PyMaterialPtr material() const;
		void material( const PyMaterialPtr & newVal );

		FilterQuadPtr filterQuad() const;
		void filterQuad( const FilterQuadPtr & newVal );

		const bool clearRenderTarget() const;
		void clearRenderTarget( bool newVal );

		void callCallback( bool refreshProperties );

#endif // EDITOR_ENABLED

		PY_RW_ATTRIBUTE_DECLARE( pRenderTarget_, renderTarget )
		PY_RW_ATTRIBUTE_DECLARE( pMaterial_, material )
		PY_RW_ATTRIBUTE_DECLARE( pFilterQuad_, filterQuad )
		PY_RW_ATTRIBUTE_DECLARE( clearRenderTarget_, clearRenderTarget )
		PY_RW_ATTRIBUTE_DECLARE( name_, name )

		PY_FACTORY_DECLARE()

	private:
		bool			clearRenderTarget_;
		FilterQuadPtr	pFilterQuad_;
		PyMaterialPtr	pMaterial_;
		PyRenderTargetPtr pRenderTarget_;
		BW::string		name_;

#ifdef EDITOR_ENABLED
		PhaseChangeCallback pCallback_;
		bool calcPreview_;
		PyPhaseEditorPtr pPropertyEditor_;
#endif // EDITOR_ENABLED
	};
};


#ifdef CODE_INLINE
#include "py_phase.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_PHASE_HPP