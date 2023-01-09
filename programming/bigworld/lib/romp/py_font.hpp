#ifndef PY_FONT_HPP
#define PY_FONT_HPP

#include "romp/font.hpp"
#include "pyscript/pyobject_plus.hpp"

#include "font_metrics.hpp"
#include "glyph_cache.hpp"

BW_BEGIN_NAMESPACE

/*~ class BigWorld.PyFont
 *	@components{ client, tools }
 *
 *	This class exposes a reference to a font.
 */
/**
 *	This class allows a font to be passed through Python.
 */
class PyFont : public PyObjectPlus
{
	Py_Header( PyFont, PyObjectPlus )

public:
	PyFont( CachedFontPtr pFont, PyTypeObject * pType = &s_type_ );
	~PyFont();

	const BW::string name() const;
	PY_RO_ATTRIBUTE_DECLARE( name(), name )

	uint stringWidth(
		const BW::wstring& theString,
		bool multiline = false,
		bool colourFormatting = false ) const;

	PY_AUTO_METHOD_DECLARE(
		RETDATA,
		stringWidth,
		ARG( BW::wstring, OPTARG( bool, false, OPTARG( bool, false, END ) ) )
	);

	PyObject* stringDimensions(
		const BW::wstring& theString,
		bool multiline,
		bool colourFormatting ) const;

	PY_AUTO_METHOD_DECLARE(
		RETOWN,
		stringDimensions,
		ARG( BW::wstring, OPTARG( bool, false, OPTARG( bool, false, END ) ) )
	);

	PY_FACTORY_DECLARE()
protected:
	CachedFontPtr pFont_;
};

#ifdef CODE_INLINE
#include "py_font.ipp"
#endif

BW_END_NAMESPACE

#endif // PY_FONT_HPP
