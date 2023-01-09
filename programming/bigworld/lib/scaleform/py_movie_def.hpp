#ifndef SCALEFORM_PY_MOVIE_DEF_HPP
#define SCALEFORM_PY_MOVIE_DEF_HPP
#if SCALEFORM_SUPPORT

#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"

#include "config.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	/*~	class Scaleform.PyMovieDef
	 *
	 *	This class wraps Scaleform's Movie Definition and provides
	 *	lifetime control and python access to its API.
	 *
	 *	A Movie Definition contains the resources for an .swf file,
	 *	and allows creation of instanced views of the movie.
	 *
	 *	Movie Definitions can also be loaded for the sole purpose
	 *	of providing fonts for other movies and for flash text components.
	 */
	class PyMovieDef : public PyObjectPlus
	{
		Py_Header( PyMovieDef, PyObjectPlus )
	public:
		PyMovieDef( GFx::MovieDef* impl, PyTypePlus *pType = &s_type_ );
		~PyMovieDef();

		PyObject * createInstance( bool initFirstFrame = true );
		PY_AUTO_METHOD_DECLARE( RETOWN, createInstance, OPTARG( bool, true, END ) )

		void setAsFontMovie();
		PY_AUTO_METHOD_DECLARE( RETVOID, setAsFontMovie, END )

		void addToFontLibrary( bool pin);
		PY_AUTO_METHOD_DECLARE( RETVOID, addToFontLibrary, OPTARG( bool, true, END ) )

		PY_FACTORY_DECLARE()

		static GFx::FontLib* GetFontLib(PyMovieDef* self, PyObject* args);
	private:
		Ptr<GFx::MovieDef> pMovieDef_;
	};
	
} // namespace ScaleformBW

PY_SCRIPT_CONVERTERS_DECLARE( ScaleformBW::PyMovieDef )

BW_END_NAMESPACE

#endif // #if SCALEFORM_SUPPORT
#endif // SCALEFORM_PY_MOVIE_DEF_HPP