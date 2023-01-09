#ifndef TEXT_GUI_COMPONENT_HPP
#define TEXT_GUI_COMPONENT_HPP

#include "simple_gui_component.hpp"
#include "moo/custom_mesh.hpp"
#include "moo/vertex_formats.hpp"
#include "romp/font.hpp"
#include "romp/font_metrics.hpp"
#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.TextGUIComponent
 *	@components{ client, tools }
 *
 *	The TextGUIComponent is a GUI component used to display text on the screen.
 *	It inherits from SimpleGUIComponent.
 *
 *	It can display text in various fonts, and can have its text assigned 
 *	dynamically.
 *
 *	A new TextGUIComponent is created using the GUI.Text function.
 *
 *	For example:
 *	@{
 *	tx = GUI.Text( "hello there" )
 *	GUI.addRoot( tx )
 *	tx.text = "goodbye"
 *	@}
 *	This example creates and displays a TextGUIComponent with the text
 *	"hello there", and then changes it to say "goodbye" immediately
 *	afterwards.
 *
 *	Note that setting the SimpleGUIComponent.texture and 
 *	SimpleGUIComponent.textureName attributes is not supported on
 *	TextGUIComponent.
 *
 */
/**
 *	This class is a scriptable GUI component that displays a line of text
 */
class TextGUIComponent : public SimpleGUIComponent
{
	Py_Header( TextGUIComponent, SimpleGUIComponent )

public:
	TextGUIComponent( CachedFontPtr font = NULL, PyTypeObject * pType = &s_type_ );
	~TextGUIComponent();

	bool				pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::wstring, label, text )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, font, font )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, multiline, multiline )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, colourFormatting, colourFormatting )
	PY_RW_ATTRIBUTE_DECLARE( explicitSize_, explicitSize );
	PY_METHOD_DECLARE( py_reset )

	void				label( const BW::wstring& l );
	const BW::wstring&	label( void );

	void				multiline( bool b );
	bool				multiline();

	void				colourFormatting( bool b );
	bool				colourFormatting();

	///this method only to be used for debugging code from C++ only.
	///(takes an ANSI only string)
	void				slimLabel( const BW::string& l );

	virtual void		textureName( const BW::string& name );

	void				font( const BW::string& fontName );
	const BW::string	font() const;

	void				update( float dTime, float relParentWidth, float relParentHeight );

	void				size( const Vector2 & size );
	float				width() const;
	void				width( float w );
	float				height() const;
	void				height( float h );

	uint				stringWidth( const BW::wstring& theString ) const;
	PY_AUTO_METHOD_DECLARE( RETDATA, stringWidth, ARG( BW::wstring, END ) );

	PyObject*			stringDimensions( const BW::wstring& theString ) const;
	PY_AUTO_METHOD_DECLARE( RETOWN, stringDimensions, ARG( BW::wstring, END ) );

protected:
	TextGUIComponent(const TextGUIComponent&);
	TextGUIComponent& operator=(const TextGUIComponent&);

	void				recalculate();
	void				copyAndMove( float relParentWidth, float relParentHeight );
	void				calculateMeshSize();
	void				drawSelf( bool reallyDraw, bool overlay );
	bool				buildMaterial();

	CachedFontPtr		font_;
	CustomMesh<GUIVertex>* mesh_;
	Vector3				meshSize_;
	BW::wstring		label_;
	bool				dirty_;
	bool				explicitSize_;
	bool				multiline_;
	bool				colourFormatting_;
	Vector3				drawOffset_;
	uint32				lastUsedResolution_;
	D3DXHANDLE			technique_;

	virtual bool		load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings );
	virtual void		save( DataSectionPtr pSect, SaveBindings & bindings );

	virtual void		reset();

	COMPONENT_FACTORY_DECLARE( TextGUIComponent() )
};

#ifdef CODE_INLINE
#include "text_gui_component.ipp"
#endif

BW_END_NAMESPACE

#endif // TEXT_GUI_COMPONENT_HPP
