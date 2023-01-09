#ifndef FILTER_QUAD_HPP
#define FILTER_QUAD_HPP

#include "resmgr/datasection.hpp"
#include "moo/effect_material.hpp"


BW_BEGIN_NAMESPACE

#ifdef EDITOR_ENABLED

class GeneralEditor;

#endif // EDITOR_ENABLED


namespace PostProcessing
{
	/**
	 *	This class abstract class implements the interface for
	 *	filter quads, used by PyPhases.
	 */
	class FilterQuad : public PyObjectPlus
	{
		Py_Header( FilterQuad, PyObjectPlus )

	public:
		FilterQuad( PyTypeObject *pType = &s_type_ );
		virtual ~FilterQuad();

		virtual void preDraw( Moo::EffectMaterialPtr pMat ) = 0;
		virtual void draw();
		virtual bool save( DataSectionPtr ) = 0;
		virtual bool load( DataSectionPtr ) = 0;

#ifdef EDITOR_ENABLED
		typedef void (*FilterChangeCallback)( bool needsReload );
		virtual void edEdit( GeneralEditor * editor, FilterChangeCallback pCallback ) = 0;
		virtual const char * creatorName() const = 0;
#endif // EDITOR_ENABLED

	private:
	};

	typedef SmartPointer<FilterQuad> FilterQuadPtr;
};

PY_SCRIPT_CONVERTERS_DECLARE( PostProcessing::FilterQuad )

#ifdef CODE_INLINE
#include "filter_quad.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef FILTER_QUAD_HPP
