#include "pch.hpp"
#if SCALEFORM_SUPPORT

#include "flash_text_gui_component.hpp"
#include "manager.hpp"
#include "util.hpp"

#include "romp/font.hpp"
#include "math/boundbox.hpp"
#include "ashes/simple_gui.hpp"
#include "cstdmf/debug.hpp"
#include "moo/effect_material.hpp"
#include "moo/geometrics.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{

COMPONENT_FACTORY( FlashTextGUIComponent )

PY_TYPEOBJECT( FlashTextGUIComponent )

PY_BEGIN_METHODS( FlashTextGUIComponent )
	/*~ function FlashTextGUIComponent.reset
	 *
	 *	This method resets the FlashTextGUIComponent if it has become dirty.
	 *	This method is useful if you change the text label, and then
	 *	immediately in the python script you wish to read out the labels
	 *	new size. If you don't call reset, then the label size will only
	 *	be recalculated when the label is next updated and drawn.
	 */
	PY_METHOD( reset )
	/*~ function FlashTextGUIComponent.stringWidth
	 *
	 *	This function returns the width (in pixels) that the specified string
	 *	will take when rendered to the FlashTextGUIComponent using its current font.
	 *
	 *	@param text	The string to measure the length of.
	 *
	 *	@return		An integer, the rendered width of the string in pixels.
	 */
	PY_METHOD( stringWidth )
	/*~ function FlashTextGUIComponent.stringDimensions
	 *
	 *	This function returns a 2-tuple containing the dimensions (in pixels) that 
	 *	the specified string will take when rendered to the FlashTextGUIComponent using 
	 *	its current font.
	 *
	 *	@param text	The string to measure the length and height of.
	 *
	 *	@return		A 2-tuple of integers, the render width and height 
	 *				of the string in pixels.
	 */
	PY_METHOD( stringDimensions )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FlashTextGUIComponent )
	/*~ attribute FlashTextGUIComponent.text
	 *
	 *	The text to be displayed in the component.  It will be rendered using
	 *	the font specified in the font attribute.  The component will be
	 *	resized to fit the text, if a new string is assigned to it.  It doesn't
	 *	wrap text, so if the string is too long to fit on the screen it will
	 *	just extend off the edges.
	 *
	 *	If multiple lines of text are desired, then multiple GFxTextGUIComponents
	 *	are required.
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( text )
	/*~ attribute FlashTextGUIComponent.font
	 *
	 *	This attribute specifies the path to the font file that is used to
	 *	render the text.
	 *
	 *	The font name must be one available in the loaded GFxFont library,
	 *	this is configured along with your Scaleform initialisation.
	 *
	 *	For example:
	 *	@{
	 *	tx = GUI.FlashText( "test" )
	 *	tx.font = "default_small"
	 *	@}
	 *	This example applies the default_small font to the text
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( font )
	/*~ attribute FlashTextGUIComponent.explicitSize
	 *
	 *	This attribute specifies whether the text component will resize itself
	 *	to fit exactly within the given text dimensions, or whether the
	 *	size of the component automatically resizes to reflect the best
	 *	on-screen size (default)
	 *
	 *	If this flag is turned off (default) the text component resizes itself
	 *	such that exactly one texel equals one pixel on-screen, for the
	 *	clearest possible text.  In this instance, width and height are read-only
	 *	properties, and are correct after the text label is set, or the
	 *	screen resolution has changed.
	 *
	 *	If this flag is turned on, the text component will draw itself into
	 *	the given dimensions.  This may result in text with an incorrect aspect
	 *	ratio.  In this instance, width and height are read/write.
	 *
	 *  When the flag is turned on, setting either the width or the height to
	 *	0 will maintain the correct aspect ratio.  After setting the width or
	 *	height to 0, the width and height attributes will update to reflect
	 *	the actual size of the text.
	 *
	 *	When the flag is turned on, setting both width and height to 0 will
	 *	set the text to the optimal size (one texel = one pixel).  In order
	 *	to do this, the width and height must be set at the same time, i.e.
	 *	you must use textComponent.size = (0,0)	 
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( explicitSize )
	/*~ attribute FlashTextGUIComponent.fontSize
	 *
	 *	This attribute sets the font size in points.  Scaleform can
	 *	display a font in any size.  Larger font sizes are drawn using
	 *	geometry instead of textures, and can affect performance adversely
	 *	if not used with care.
	 */
	PY_ATTRIBUTE( fontSize )
	/*~ attribute FlashTextGUIComponent.underline
	 *
	 *	This attribute deteremines whether or not the text string should be
	 *	underlined.
	 */
	PY_ATTRIBUTE( underline )
	/*~ attribute FlashTextGUIComponent.wordWrap
	 *
	 *	This attribute deteremines whether or not the text string should
	 *	wrap.  This attribute only applies if multiline is also True.
	 */
	PY_ATTRIBUTE( wordWrap )
	/*~ attribute FlashTextGUIComponent.multiline
	 *
	 *	This attribute deteremines whether or not the text string is drawn
	 *	across multiple lines.
	 */
	PY_ATTRIBUTE( multiline )
PY_END_ATTRIBUTES()


/*~ function GUI.FlashText
 *
 *	This function creates a new FlashTextGUIComponent.  This component renders a
 *	line of text.
 *
 *	@param	text	A string containing the line of text to be rendered.
 *					This can be changed once the component is created.
 *
 *	@return			the newly created component.
 */
PY_FACTORY_NAMED( FlashTextGUIComponent, "FlashText", GUI )


void * FlashTextGUIComponent::operator new(std::size_t size)
{
	return bw_malloc_aligned(size, 16);
}


void FlashTextGUIComponent::operator delete(void * ptr)
{
	bw_free_aligned(ptr);
}


FlashTextGUIComponent::FlashTextGUIComponent( const BW::string& f, PyTypePlus * pType )
:SimpleGUIComponent( "", pType ),
 dirty_( false ),
 label_( L"FlashTextGUIComponent" ),
 gfxText_( NULL ),
 explicitSize_( false )
{
	BW_GUARD;
	this->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
	this->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );

	textParams_.TextColor = GColor(255, 255, 255, 255);
	textParams_.HAlignment = GFx::DrawText::Align_Center;
	textParams_.VAlignment = GFx::DrawText::VAlign_Center;
	textParams_.FontStyle = GFx::DrawText::Normal;
	textParams_.FontSize = 15.0;
	textParams_.FontName = f.c_str();
	textParams_.Underline = false;
	textParams_.Multiline = false;
	textParams_.WordWrap = false;
}


FlashTextGUIComponent::~FlashTextGUIComponent()
{
	BW_GUARD;
	if (blueprint_)
	{
		delete [] blueprint_;
		blueprint_ = NULL;
	}

	if (vertices_)
	{
		delete [] vertices_;
		vertices_ = NULL;
	}
}


/// Get an attribute for python
ScriptObject FlashTextGUIComponent::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;
	// if it's dirty, recalculate it so the caller gets the correct value
	if ( dirty_ )
	{
		reset();
	}

	return SimpleGUIComponent::pyGetAttribute( attrObj );
}


/// Set an attribute for python
bool FlashTextGUIComponent::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;
	// set dirty_ to true, to force a recalc of the text resources. It
	// forces the recalc always to be on the safe side.
	dirty_ = true;

	return SimpleGUIComponent::pySetAttribute( attrObj, value );
}


/**
 *	Static python factory method
 */
PyObject * FlashTextGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	PyObject * label = NULL;
	char * fontName = "Arial";
	wchar_t wBuf[256];
	if (!PyArg_ParseTuple( args, "|Os", &label, &fontName ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.FlashText: "
			"Argument parsing error: Expected an optional text string (ansi or unicode) and optional font name" );
		return NULL;
	}

	if (label && PyUnicode_Check(label))
	{
		Py_ssize_t numChars = PyUnicode_AsWideChar( (PyUnicodeObject*)label, wBuf, 255 );
		if ( numChars != -1 )
			wBuf[numChars] = L'\0';
	}
	else if (label && PyString_Check(label))
	{
		bw_snwprintf( wBuf, sizeof(wBuf)/sizeof(wchar_t), L"%S\0", PyString_AsString(label) );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "GUI.FlashText: "
			"Argument parsing error: Text string must be of type String or Unicode" );
		return NULL;
	}

	FlashTextGUIComponent* pText = new FlashTextGUIComponent( fontName );
	pText->label( wBuf );

	// all set, now force a recalc of the object
	pText->dirty_ = true;

	return pText;
}


PyObject* FlashTextGUIComponent::py_reset( PyObject * args )
{
	BW_GUARD;
	if ( dirty_ )
	{
		reset();
	}

	Py_Return;
}


/**
 *	This method recalculates the text mesh.
 *	After calling this method, the width and height of the component will be correct.
 */
void FlashTextGUIComponent::recalculate()
{
	BW_GUARD;
	if ( gfxText_.GetPtr() )
	{
		gfxText_->SetText( this->label_.c_str() );
		gfxText_->SetFont( this->textParams_.FontName );
		gfxText_->SetFontSize( this->textParams_.FontSize );
		gfxText_->SetUnderline( this->textParams_.Underline );
		gfxText_->SetMultiline( this->textParams_.Multiline );
		gfxText_->SetWordWrap( this->textParams_.WordWrap );
	}

	if ( !explicitSize_ )
	{
		eSizeMode widthMode = SimpleGUIComponent::widthMode();
		eSizeMode heightMode = SimpleGUIComponent::heightMode();
		SimpleGUIComponent::widthMode( SimpleGUIComponent::SIZE_MODE_PIXEL );
		SimpleGUIComponent::heightMode( SimpleGUIComponent::SIZE_MODE_PIXEL );
		GSizeF s = Manager::instance().pTextManager()->GetTextExtent( this->label_.c_str(), 0, &textParams_ );
		//need to adjust here for resolution override.  scaleform returns actual pixels size.
		//But setting width/height in pixels means 'adjusted for res override sized pixels'
		s.Width *= ( SimpleGUI::instance().screenResolution().x / Moo::rc().screenWidth() );
		s.Height *= ( SimpleGUI::instance().screenResolution().y / Moo::rc().screenHeight() );
		SimpleGUIComponent::width( s.Width );
		SimpleGUIComponent::height( s.Height );
		SimpleGUIComponent::widthMode( widthMode );
		SimpleGUIComponent::heightMode( heightMode );
	}

	stringRect_.SetRect( 0, 0, 
		Manager::instance().pTextManager()->GetTextExtent( this->label_.c_str(), 0, &textParams_ ));

	gfxText_.Clear();
	if ( !gfxText_.GetPtr() )
	{
		gfxText_ = *Manager::instance().pTextManager()->CreateText( this->label_.c_str(), stringRect_, &this->textParams_ );
	}
	else
	{
	
		gfxText_->SetRect( stringRect_ );
	}

	gfxText_->SetColor( GColor( this->colour() ) );
}


void FlashTextGUIComponent::reset()
{
	BW_GUARD;
	this->recalculate();
	dirty_ = false;
}


void
FlashTextGUIComponent::update( float dTime, float relativeParentWidth, float relativeParentHeight )
{
	BW_GUARD;
	// turn off mip mapping here (nowhere else to do it)
	//material_.textureStage(0).useMipMapping( false );
	runTimeColour( colour() );
	runTimeTransform( Matrix::identity );

	if ( dirty_ )
	{
		recalculate();
		dirty_ = false;
	}

	SimpleGUIComponent::updateChildren( dTime, relativeParentWidth, relativeParentHeight );
	SimpleGUIComponent::clipBounds(corners_[0], corners_[1], corners_[2], corners_[3] );
}


/**
 *	Load
 */
bool FlashTextGUIComponent::load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings )
{
	BW_GUARD;
	if (!this->SimpleGUIComponent::load( pSect, ownerName, bindings )) return false;

	this->label( pSect->readWideString( "label", this->label() ) );
	this->font( pSect->readString( "font", this->font() ) );
	this->fontSize( pSect->readFloat( "fontSize", this->fontSize() ) );
	this->explicitSize_ = pSect->readBool( "explicitSize", this->explicitSize_ );

	return true;
}

/**
 *	Save
 */
void FlashTextGUIComponent::save( DataSectionPtr pSect, SaveBindings & bindings )
{
	BW_GUARD;
	this->SimpleGUIComponent::save( pSect, bindings );

	pSect->writeWideString( "label", this->label() );
	pSect->writeString( "font", this->font() );
	pSect->writeFloat( "fontSize", this->fontSize() );
	pSect->writeBool( "explicitSize", this->explicitSize_ );
}


/**
 *	This method sets the text item's font.
 *
 *	@param	fontName	The name of the font file.
 */
void FlashTextGUIComponent::font( const BW::string& fontName )
{
	BW_GUARD;
	this->textParams_.FontName = fontName.c_str();
	dirty_ = true;
}


/**
 *	This method returns the font name.
 *
 *	@return	BW::string		The name of the font
 */
const BW::string FlashTextGUIComponent::font() const
{
	BW_GUARD;
	GString g(  textParams_.FontName );
	return BW::string(g);
}


static bool settingDimensions = false;

void FlashTextGUIComponent::size( const Vector2 & size )
{
	BW_GUARD;
	if (settingDimensions) return;
	//do this so we don't possibly recalculate.
	//this is so if explicitSize_ is true, we don't 
	//do 2 recalculations.
	settingDimensions = true;
	SimpleGUIComponent::size(size);
	settingDimensions = false;
	//and this sparks a possible recalculate
	this->width(size.x);
}


void FlashTextGUIComponent::width( float w )
{
	BW_GUARD;
	SimpleGUIComponent::width(w);
	if (settingDimensions) return;		
	settingDimensions = true;
	if (explicitSize_)
	{
		this->recalculate();
	}
	settingDimensions = false;
}


void FlashTextGUIComponent::height( float h )
{	
	BW_GUARD;
	SimpleGUIComponent::height(h);
	if (settingDimensions) return;
	settingDimensions = true;
	if (explicitSize_)
	{
		this->recalculate();
	}	
	settingDimensions = false;
}


void FlashTextGUIComponent::drawSelf( bool reallyDraw, bool overlay )
{
	BW_GUARD;

	if (!reallyDraw)
		return;

	Ptr<Render::Renderer2D> &pRenderer = Manager::instance().pRenderer();
	if (pRenderer == NULL)
		return;

	Render::TreeRootDisplayHandle &displayHandle = *const_cast<Render::TreeRootDisplayHandle*>(&Manager::instance().pTextManager()->GetDisplayHandle());

	if (overlay)
	{
		GFx::Viewport gv;
		GRectF rect;
		runtimeGUIViewport( corners_, gv, rect );

		// Calculate the scale of the text, this is because scaleform does not fit the
		// text to the rect, rather it gives us a rect for the text to fit in, and we
		// can scale the component to whatever size we want.
		float scaleWidth = rect.Width() / stringRect_.Width();
		float scaleHeight = rect.Height() / stringRect_.Height();

		// Set up the transform for the text, we assume that the text object has stringRect_ set
		// as its rect
		GFx::DrawText::Matrix transform( scaleWidth, 0, 0, scaleHeight, rect.X1(), rect.Y1() );
		gfxText_->SetMatrix( transform );

		Manager::instance().pTextManager()->SetViewport(gv);
		if (displayHandle.NextCapture(pRenderer->GetContextNotify()))
			pRenderer->Display(displayHandle);
	}
	else
	{
		// Store the viewport as Scaleform doesn't play nice with it
		D3DVIEWPORT9 vp;
		Moo::rc().device()->GetViewport( &vp );

		// If we are not rendering as an overlay it means we are rendering as part
		// of the scene, this means that we need to insert the text element into the scene.
		// As Scaleform does not deal with the zbuffer (i.e. it disables it) we use the
		// stencil buffer in stead,  we render a quad into the scene at the location of
		// the text setting the stencil buffer to 2 where the z-test succeeds and 0 where
		// it fails, this enables us to render the text at a particular depth by doing a
		// stencil comparison.

		GFx::Viewport gv;
		GRectF drect;

		Vector2 corners[4];
		Matrix wvp = Moo::rc().viewProjection();
		wvp.preMultiply( Moo::rc().world());
		Outcode combinedOC = OUTCODE_MASK;
		float depth = 1.f;

		// First we transform the corners of the text and calculate the combined outcode
		if (explicitSize_)
		{
			Vector4 clipOrigin = wvp.row(3);

			// Early out if the text component is outside the near or far plane.
			if (clipOrigin.z < 0 ||
				clipOrigin.z > clipOrigin.w )
				return;

			clipOrigin *= 1.f / clipOrigin.w;

			depth = clipOrigin.z;

			for (size_t i = 0; i < 4; i++)
			{
				Vector2 clipCorner( clipOrigin.x + corners_[i].x,
					clipOrigin.y + corners_[i].y );

				Outcode oc = 0;

				if (clipCorner.x < -1.f) oc |= OUTCODE_LEFT;
				else if (clipCorner.x > 1.f) oc |= OUTCODE_RIGHT;

				if (clipCorner.y < -1.f) oc |= OUTCODE_TOP;
				else if (clipCorner.y > 1.f) oc |= OUTCODE_BOTTOM;

				combinedOC &= oc;

				corners[i].set( (clipCorner.x + 1.f) * Moo::rc().halfScreenWidth() , 
					(1.f - clipCorner.y) * Moo::rc().halfScreenHeight() );
			}
			
		}
		else
		{
			for (size_t i = 0; i < 4; i++ )
			{
				Vector4 worldCorner;

				wvp.applyPoint( worldCorner, Vector3(corners_[i].x, corners_[i].y, 0.f) );
				combinedOC &= worldCorner.calculateOutcode();
				worldCorner *= 1.f / worldCorner.w;
				depth = min( depth, worldCorner.z );
				corners[i].set( (worldCorner.x + 1.f) * Moo::rc().halfScreenWidth() , 
					(1.f - worldCorner.y) * Moo::rc().halfScreenHeight() );
			}
		}

		// If the combined outcode is non-zero, the component is off screen and we
		// can do an early out.
		if (combinedOC != 0)
			return;

		// Create the rect for the component on screen
		drect.SetRect(
			min(min( corners[0].x, corners[1].x), min(corners[2].x, corners[3].x )),
			min(min( corners[0].y, corners[1].y), min(corners[2].y, corners[3].y )),
			max(max( corners[0].x, corners[1].x), max(corners[2].x, corners[3].x )),
			max(max( corners[0].y, corners[1].y), max(corners[2].y, corners[3].y ))
		);

		// We don't want our stencil object to render to the colour buffer, so colour writes
		// are turned off
		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, 0 );

		// Set up all stencil state, to be on the safe side
		// Write 2 to the stencil buffer if z succeeds, 0 if it fails
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
		Moo::rc().setRenderState( D3DRS_STENCILENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
		Moo::rc().setRenderState( D3DRS_TWOSIDEDSTENCILMODE, FALSE );
		Moo::rc().setRenderState( D3DRS_STENCILMASK, 0xffffffff );
		Moo::rc().setRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );
		Moo::rc().setRenderState( D3DRS_STENCILREF, 2 );
		Moo::rc().setRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
		Moo::rc().setRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_ZERO );

		// Render the stencil quad
		Geometrics::drawRect( Vector2( drect.X1(), drect.Y1()), Vector2( drect.X2(), drect.Y2() ), Moo::Colour(0,0,0,0), depth );

		// Turn colour writes back on
		Moo::rc().popRenderState();

		// Set our fullscreen viewport
		fullScreenViewport( gv );

		// Calculate the scale of the text, this is because scaleform does not fit the
		// text to the rect, rather it gives us a rect for the text to fit in, and we
		// can scale the component to whatever size we want.
		float scaleWidth = drect.Width() / stringRect_.Width();
		float scaleHeight = drect.Height() / stringRect_.Height();

		// Set up the transform for the text, we assume that the text object has stringRect_ set
		// as its rect
		GFx::DrawText::Matrix transform( scaleWidth, 0, 0, scaleHeight, drect.X1(), drect.Y1() );
		gfxText_->SetMatrix( transform );
		
		// begin text rendering
		Manager::instance().pTextManager()->SetViewport(gv);

		// Set our stencil test function
		Moo::rc().setRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
		Moo::rc().setRenderState( D3DRS_STENCILREF, 2 );
		Moo::rc().setRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
		Moo::rc().setRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );

		// draw text
		if (displayHandle.NextCapture(pRenderer->GetContextNotify()))
			pRenderer->Display(displayHandle);

		// We are done with the text

		// Set the stored viewport back
		Moo::rc().device()->SetViewport( &vp );

		// Clean up our state
		Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
		Moo::rc().setRenderState( D3DRS_STENCILENABLE, FALSE );
	}

	//TODO - take this out when we have re-enabled the D3D wrapper,
	//when we use the wrapper, scaleform will be calling via our
	//own setRenderState methods, instead of going straight to the
	//device and causing issues with later draw calls.
	Moo::rc().initRenderStates();
}


uint FlashTextGUIComponent::stringWidth( const BW::wstring& theString )
{
	BW_GUARD;
	GSizeF s;
	if ( !gfxText_ )
		return 0;
	s = Manager::instance().pTextManager()->GetTextExtent(theString.c_str(), 0, &textParams_ );
	return (uint)s.Width;
}


PyObject* FlashTextGUIComponent::stringDimensions( const BW::wstring& theString )
{
	BW_GUARD;
	GSizeF s;
	if ( !gfxText_ )
	{
		s.Width = 0;
		s.Height = 0;
	}
	else
	{
		s = Manager::instance().pTextManager()->GetTextExtent(theString.c_str(), 0, &textParams_ );
	}
	
	PyObject* r = PyTuple_New(2);
	PyTuple_SetItem( r, 0, PyInt_FromLong((uint)s.Width) );
	PyTuple_SetItem( r, 1, PyInt_FromLong((uint)s.Height) );
	return r;
}

void FlashTextGUIComponent::label( const BW::wstring& l )
{
	BW_GUARD;
	//font can't handle more than 256 sets of indices, so truncate this string.
	label_ = l.substr(0,256);
	dirty_ = true;
}

const BW::wstring& FlashTextGUIComponent::label( void )
{
	BW_GUARD;
	return label_;
}


}	//namespace ScaleformBW

BW_END_NAMESPACE
#endif	//#if SCALEFORM_SUPPORT
// flash_text_gui_component.cpp
