#ifndef PY_COPY_BACK_BUFFER_HPP
#define PY_COPY_BACK_BUFFER_HPP

#include "phase.hpp"
#include "phase_factory.hpp"
#include "filter_quad.hpp"
#include "romp/py_material.hpp"
#include "romp/py_render_target.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	/**
	 *	This class derives from PostProcessing::Phase, and
	 *	provides a way to get a copy of the back buffer in
	 *	a render target.  It does this by doing a StretchRect
	 *	from the back buffer, which allows the card to resolve
	 *	anti-aliasing at that time.
	 */
	class PyCopyBackBuffer : public Phase
	{
		Py_Header( PyCopyBackBuffer, Phase )
		DECLARE_PHASE( PyCopyBackBuffer )
	public:
		PyCopyBackBuffer( PyTypeObject *pType = &s_type_ );
		~PyCopyBackBuffer();

		void tick( float dTime );
		void draw( class Debug*, RECT* = NULL );

		bool load( DataSectionPtr );
		bool save( DataSectionPtr );

		static uint32 drawCookie();
		static void incrementDrawCookie();

#ifdef EDITOR_ENABLED
		void edChangeCallback( PhaseChangeCallback pCallback );
		void edEdit( GeneralEditor * editor );

		BW::string getOutputRenderTarget() const;
		bool setOutputRenderTarget( const BW::string & rt );
#endif // EDITOR_ENABLED

		PY_RW_ATTRIBUTE_DECLARE( pRenderTarget_, renderTarget )
		PY_RW_ATTRIBUTE_DECLARE( name_, name )

		PY_FACTORY_DECLARE()
	private:
		bool			isDirty() const;
		PyRenderTargetPtr pRenderTarget_;
		BW::string		name_;
		static uint32	s_drawCookie_;
		uint32			drawCookie_;

#ifdef EDITOR_ENABLED
		PhaseChangeCallback pCallback_;
#endif // EDITOR_ENABLED

		bool renderTargetFromString( const BW::string & resourceID );
	};	
};


#ifdef CODE_INLINE
#include "py_copy_back_buffer.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_COPY_BACK_BUFFER_HPP
