#ifndef PHASE_HPP
#define PHASE_HPP


#ifdef EDITOR_ENABLED

#include "moo/moo_dx.hpp"


BW_BEGIN_NAMESPACE

class GeneralEditor;

BW_END_NAMESPACE

#endif // EDITOR_ENABLED


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	/**
	 *	This class is an abstract base class for Post-Processing Phases.
	 *	It ensures derivated classes are PyObjects.
	 */
	class Phase : public PyObjectPlus
	{
		Py_Header( Phase, PyObjectPlus )
	public:
		Phase( PyTypeObject *pType = &s_type_ );
		virtual ~Phase();

		virtual void tick( float dTime ) = 0;
		virtual void draw( class Debug*, RECT* = NULL ) = 0;

		virtual bool load( DataSectionPtr ) = 0;
		virtual bool save( DataSectionPtr ) = 0;

#ifdef EDITOR_ENABLED
		typedef void (*PhaseChangeCallback)( bool needsReload );
		virtual void edChangeCallback( PhaseChangeCallback pCallback ) = 0;
		virtual void edEdit( GeneralEditor * editor ) = 0;
#endif // EDITOR_ENABLED
	private:
	};

	typedef SmartPointer<Phase> PhasePtr;
};

PY_SCRIPT_CONVERTERS_DECLARE( PostProcessing::Phase )

#ifdef CODE_INLINE
#include "phase.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PHASE_HPP
