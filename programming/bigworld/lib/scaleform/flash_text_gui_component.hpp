#ifndef FLASH_TEXT_GUI_COMPONENT_HPP
#define FLASH_TEXT_GUI_COMPONENT_HPP
#if SCALEFORM_SUPPORT

#include "cstdmf/stdmf.hpp"
#include "config.hpp"
#include "ashes/simple_gui_component.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{

/*~ class GUI.FlashTextGUIComponent
 *
 *	The FlashTextGUIComponent is a GUI component used to display text on the screen.
 *	It inherits from SimpleGUIComponent.
 *
 *	It uses the Scaleform DrawTextAPI to display the text.
 *	It can display text in various fonts, and can have its text assigned 
 *	dynamically.
 *
 *	A new FlashTextGUIComponent is created using the GUI.FlashText function.
 *
 *	For example:
 *	@{
 *	tx = GUI.FlashText( "hello there" )
 *	GUI.addRoot( tx )
 *	tx.text = "goodbye"
 *	@}
 *	This example creates and displays a FlashTextGUIComponent with the text
 *	"hello there", and then changes it to say "goodbye" immediately
 *	afterwards.
 */
/**
 *	This class is a scriptable GUI component that displays a line of text
 *	using the Scaleform DrawText API
 */
class FlashTextGUIComponent : public SimpleGUIComponent
{
	Py_Header( FlashTextGUIComponent, SimpleGUIComponent )

public:
	FlashTextGUIComponent( const BW::string& font = "", PyTypePlus * pType = &s_type_ );
	~FlashTextGUIComponent();

	ScriptObject		pyGetAttribute( const ScriptString & attrObj );
	bool				pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::wstring, label, text )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, font, font )
	PY_RW_ATTRIBUTE_DECLARE( explicitSize_, explicitSize );
	PY_METHOD_DECLARE( py_reset )

	void				label( const BW::wstring& l );
	const BW::wstring&	label( void );

	void				font( const BW::string& fontName );
	const BW::string	font() const;

	void				update( float dTime, float relParentWidth, float relParentHeight );
	void				drawSelf( bool reallyDraw, bool overlay );

	void				size( const Vector2 & size );
	void				width( float w );
	void				height( float h );

	float				fontSize() const	{ return textParams_.FontSize; }
	void				fontSize( float s )	{ textParams_.FontSize = s; dirty_ = true; }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, fontSize, fontSize );

	bool				underline() const	{ return textParams_.Underline; }
	void				underline( bool u ) { textParams_.Underline = u; dirty_ = true; }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, underline, underline );

	bool				wordWrap() const	{ return textParams_.WordWrap; }
	void				wordWrap( bool w ) { textParams_.WordWrap = w; dirty_ = true; }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, wordWrap, wordWrap );

	bool				multiline() const	{ return textParams_.Multiline; }
	void				multiline( bool m ) { textParams_.Multiline = m; dirty_ = true; }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, multiline, multiline );

	uint				stringWidth( const BW::wstring& theString );
	PY_AUTO_METHOD_DECLARE( RETDATA, stringWidth, ARG( BW::wstring, END ) );

	PyObject*			stringDimensions( const BW::wstring& theString );
	PY_AUTO_METHOD_DECLARE( RETOWN, stringDimensions, ARG( BW::wstring, END ) );

	// GRectF member has 16 bit alignment requirements, thus need to do an aligned alloc
	static void * operator new(std::size_t size);
	static void operator delete(void * ptr);

protected:
	FlashTextGUIComponent(const FlashTextGUIComponent&);
	FlashTextGUIComponent& operator=(const FlashTextGUIComponent&);

	BW::wstring		label_;
	bool				dirty_;
	bool				explicitSize_;
	Ptr<GFx::DrawText>	gfxText_;
	GFx::DrawTextManager::TextParams textParams_;
	Vector2				corners_[4];

	GRectF				stringRect_;

	virtual bool		load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings );
	virtual void		save( DataSectionPtr pSect, SaveBindings & bindings );
	virtual void		reset();
private:
	void	recalculate();
	void	textureName( const BW::string& name )	{};	//hide textureName, this is invalid for text components.

	COMPONENT_FACTORY_DECLARE( FlashTextGUIComponent( "Arial" ) )
};

};	//namespace ScaleformBW

BW_END_NAMESPACE

#endif // #if SCALEFORM_SUPPORT
#endif // FLASH_TEXT_GUI_COMPONENT_HPP
