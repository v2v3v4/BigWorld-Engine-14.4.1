#include "pch.hpp"

#include "text_gui_component.hpp"
#include "romp/font_manager.hpp"
#include "romp/glyph_cache.hpp"
#include "simple_gui.hpp"

#include "cstdmf/debug.hpp"
#include "moo/effect_material.hpp"
#include "resmgr/auto_config.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

AutoConfigString s_defaultFontName( "system/defaultFontName", "default_medium.font" );


#ifndef CODE_INLINE
#include "text_gui_component.ipp"
#endif

PY_TYPEOBJECT( TextGUIComponent )

PY_BEGIN_METHODS( TextGUIComponent )
	/*~ function TextGUIComponent.reset
	 *	@components{ client, tools }
	 *
	 *	This method redraws the TextGUIComponent if it has become dirty.  It
	 *	shouldn't be neccessary to call this function.
	 */
	PY_METHOD( reset )
	/*~ function TextGUIComponent.stringWidth
	 *	@components{ client, tools }
	 *
	 *	This function returns the width (in pixels) that the specified string
	 *	will take when rendered to the TextGUIComponent using its current font.
	 *
	 *	@param text	The string to measure the length of.
	 *
	 *	@return		An integer, the rendered width of the string in pixels.
	 */
	PY_METHOD( stringWidth )
	/*~ function TextGUIComponent.stringDimensions
	 *	@components{ client, tools }
	 *
	 *	This function returns a 2-tuple containing the dimensions (in pixels) that 
	 *	the specified string will take when rendered to the TextGUIComponent using 
	 *	its current font.
	 *
	 *	@param text	The string to measure the length and height of.
	 *
	 *	@return		A 2-tuple of integers, the render width and height 
	 *				of the string in pixels.
	 */
	PY_METHOD( stringDimensions )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TextGUIComponent )
	/*~ attribute TextGUIComponent.text
	 *	@components{ client, tools }
	 *
	 *	The text to be displayed in the component.  It will be rendered using
	 *	the font specified in the font attribute.  The component will be
	 *	resized to fit the text, if a new string is assigned to it.  It doesn't
	 *	wrap text, so if the string is too long to fit on the screen it will
	 *	just extend off the edges.
	 *
	 *	If multiple lines of text are desired, then multiple TextGUIComponents
	 *	are required.
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( text )
	/*~ attribute TextGUIComponent.font
	 *	@components{ client, tools }
	 *
	 *	This attribute specifies the path to the font file that is used to
	 *	render the text.  Assigning to it causes the new font file to be
	 *	loaded. If an invalid file is specified, then a Python Error occurs.
	 *
	 *	By convention, font files are stored in the fonts subdirectory.  They
	 *	are an xml file describing which image to obtain the characters from,
	 *	and the size and location of each character.
	 *
	 *	The default font is default_medium.font".
	 *
	 *	For example:
	 *	@{
	 *	tx = GUI.Text( "test" )
	 *	tx.font = "default_small.font"
	 *	@}
	 *	This example applies the default_small font to the text
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( font )
	/*~ attribute TextGUIComponent.explicitSize
	 *	@components{ client, tools }
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
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( explicitSize )
	/*~ attribute TextGUIComponent.multiline
	 *	@components{ client, tools }
	 *
	 *	If set to True, the text component will consider all newline (\n) 
	 *	characters contained within the 'text' property and wrap them 
	 *	accordingly when rendering and calculating text dimensions.
	 *	
	 *	Newlines are treated as a line separator.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( multiline )
	/*~ attribute TextGUIComponent.colourFormatting
	 *	@components{ client, tools }
	 *
	 *	If set to True, the text component will look for colour formatting tags
	 *	embedded within the text. Formatting tags are defined inline and a 
	 *	be of the form:
	 *
	 *	@{
	 *	\cRRGGBBAA;
	 *	@}
	 *
	 *	where RRGGBBAA is a 32-bit hex representation of the desired colour
	 *	(which must include the alpha value). If a tag is not of this form
	 *	then it is treated as visible text. Use \\ to escape a tag if neccessary
	 *	(e.g. to sanitise user inputted text).
	 *
	 *	The colour property of the component will still be used, and will tint
	 *	all text within the component by multiplying against the formatted
	 *	vertex colour.
	 *
	 *	Note that formatting tags are also parsed when calculating text
	 *	dimensions.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( colourFormatting )
PY_END_ATTRIBUTES()

/*~ function GUI.Text
 *	@components{ client, tools }
 *
 *	This function creates a new TextGUIComponent.  This component renders a
 *	line of text.
 *
 *	@param	text	A string containing the line of text to be rendered.
 *					This can be changed once the component is created.
 *
 *	@return			the newly created component.
 */
PY_FACTORY_NAMED( TextGUIComponent, "Text", GUI )

COMPONENT_FACTORY( TextGUIComponent )



TextGUIComponent::TextGUIComponent( CachedFontPtr f, PyTypeObject * pType )
:SimpleGUIComponent( "", pType ),
 dirty_( false ),
 label_( L"" ),
 mesh_( NULL ),
 font_( f ),
 drawOffset_( 0.f, 0.f, 0.f ),
 explicitSize_( false ),
 multiline_( false ),
 colourFormatting_( false ),
 lastUsedResolution_( 0 )
{
	BW_GUARD;

	if ( !font_.hasObject() )
	{
		font_ = FontManager::instance().getCachedFont( s_defaultFontName.value() );
	}
	mesh_ = new CustomMesh<GUIVertex>;
	if ( font_ && font_->pTexture() )
		SimpleGUIComponent::textureName( font_->pTexture()->resourceID() );
	materialFX( FX_BLEND );

	this->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
	this->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
}


TextGUIComponent::~TextGUIComponent()
{
	BW_GUARD;

	bw_safe_delete(mesh_);
	bw_safe_delete_array(blueprint_);
	bw_safe_delete_array(vertices_);

	if (font_)
	{
		font_->releaseRefs();
	}
}


/// Set an attribute for python
bool TextGUIComponent::pySetAttribute( const ScriptString & attrObj,
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
PyObject * TextGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	PyObject * label = NULL;
	char * fontName = NULL;
	wchar_t wBuf[256];
	if (!PyArg_ParseTuple( args, "|Os", &label, &fontName ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Text: "
			"Argument parsing error: Expected an optional text string (ansi or unicode) and optional font name" );
		return NULL;
	}

	if (label && PyUnicode_Check(label))
	{
		size_t numChars = PyUnicode_AsWideChar( (PyUnicodeObject*)label, wBuf, 255 );
		if ( numChars != -1 )
			wBuf[numChars] = L'\0';
	}
	else if (label && PyString_Check(label))
	{
		bw_snwprintf( wBuf, sizeof(wBuf)/sizeof(wchar_t), L"%S\0", PyString_AsString(label) );
	}
	else if (label)
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Text: "
			"Argument parsing error: Text string must be of type String or Unicode" );
		return NULL;
	}

	CachedFontPtr pFont;
	if ( fontName )
		pFont = FontManager::instance().getCachedFont( fontName );
	else
		pFont = FontManager::instance().getCachedFont( s_defaultFontName.value() );


	if ( !pFont )
	{
		PyErr_SetString( PyExc_AttributeError, "GUI.Text: "
			"The requested font does not exist." );
		ERROR_MSG( "Invalid font %s\n", fontName ? fontName : s_defaultFontName.value().c_str() );
		return NULL;
	}

	TextGUIComponent* pText = new TextGUIComponent(	pFont );
	pText->label( wBuf );
	pText->dirty_ = true;

	return pText;
}


PyObject* TextGUIComponent::py_reset( PyObject * args )
{
	BW_GUARD;
	if ( dirty_ )
	{
		reset();
	}

	Py_RETURN_NONE;
}


void TextGUIComponent::reset()
{
	BW_GUARD;
	dirty_ = false;
	this->recalculate();
	lastUsedResolution_ = SimpleGUI::instance().realScreenResolutionCounter();

	//and move and colour the vertices.
	//we have to do this here in case this was called from
	//a script call that was itself called during the update
	//of another gui component.
	//
	//i.e. TimerText callsback a script in its update, which
	//in turn adds an alert, which calls otherText.text = blah
	//and otherText.reset().  In this case, the otherText needs
	//a fully realised vertex buffer for when it draws (it may
	//have already had update() called )

	float relativeParentWidth, relativeParentHeight;
	nearestRelativeDimensions( relativeParentWidth, relativeParentHeight );
	this->copyAndMove( relativeParentWidth, relativeParentHeight );
}


void
TextGUIComponent::update( float dTime, float relativeParentWidth, float relativeParentHeight )
{
	BW_GUARD;
	// turn off mip mapping here (nowhere else to do it)
	//material_.textureStage(0).useMipMapping( false );
	runTimeColour( colour() );
	runTimeTransform( Matrix::identity );

	// We store our own lastUsedResolution instead of using hasResolutionChanged,
	// because hasResolutionChanged is only valid for the frame when the res changed.
	// This means that if a GUI component is not updated that frame it misses the
	// res change.
	if ( SimpleGUI::instance().realScreenResolutionCounter() != lastUsedResolution_ )
	{
		if (!explicitSize_) 
		{
			dirty_ = true;
		}
	}


	if ( dirty_ )
	{
		recalculate();
		dirty_ = false;
		lastUsedResolution_ = SimpleGUI::instance().realScreenResolutionCounter();
	}

	this->copyAndMove( relativeParentWidth, relativeParentHeight );

	SimpleGUIComponent::updateChildren( dTime, relativeParentWidth, relativeParentHeight );
}


/**
 *	This method copies the blueprint vertices to the run-time vertices and
 *	moves them into place.
 */
void TextGUIComponent::copyAndMove( float relativeParentWidth, float relativeParentHeight )
{
	BW_GUARD;
	if ( nVertices_ )
	{
		//copy to temporary buffer
		memcpy( vertices_, blueprint_, nVertices_ * sizeof( GUIVertex ) );

		//move temporary vertices to current position
		float x,y,w,h;

		if (mesh_)
		{
			float relativeParentWidth = 0;
			float relativeParentHeight = 0;
			nearestRelativeDimensions( relativeParentWidth, relativeParentHeight );
			w = widthInClip( relativeParentWidth );
			h = heightInClip( relativeParentHeight );
		}
		else
		{
			w = vertices_[nVertices_-2].pos_.x - vertices_[0].pos_.x;
			h = vertices_[0].pos_.y - vertices_[3].pos_.y;
		}

		float clipX, clipY;
		this->positionInClip( relativeParentWidth, relativeParentHeight, clipX, clipY );

		float anchorOffsetX, anchorOffsetY;

		this->anchorOffset( w, h, anchorOffsetX, anchorOffsetY );

		x = clipX + anchorOffsetX;
		y = clipY + anchorOffsetY;

		drawOffset_.x = ( x - vertices_[0].pos_.x );
		drawOffset_.y = ( y - vertices_[0].pos_.y );

		for ( int i = 0; i < nVertices_; i++ )
		{
			// TODO: get rid of this as well - draw text into the correct spot in the first place.
			vertices_[i].pos_.x += drawOffset_.x;
			vertices_[i].pos_.y += drawOffset_.y;
			vertices_[i].pos_.z = position().z;
		}
	}
}


/**
 *	This method recalculates the text mesh.
 *	After calling this method, the width and height of the component will be correct.
 */
void TextGUIComponent::recalculate()
{
	BW_GUARD;
	SimpleGUIComponent::cleanMesh();

	mesh_->clear();

	float w = 0.0f;
	float h = 0.0f;

	if ( font_ )
	{		
		if (explicitSize_)
		{
			//draw into mesh uses screen clip (LEGACY) coordinates
			float relativeParentWidth = 0;
			float relativeParentHeight = 0;
			nearestRelativeDimensions( relativeParentWidth, relativeParentHeight );
			float usedWidth = widthInClip( relativeParentWidth );
			float usedHeight = heightInClip( relativeParentHeight );
			font_->colour( 0xffffffff );
			font_->releaseRefs();
			font_->drawIntoMesh( *mesh_, label_, 0.f, 0.f, usedWidth, usedHeight, &w, &h, multiline_, colourFormatting_ );
		}
		else
		{
			font_->releaseRefs();
			font_->drawIntoMesh( *mesh_, label_, 0.f, 0.f, &w, &h, multiline_, colourFormatting_ );
		}		

		SimpleGUIComponent::setWidthInScreenClip( w );
		SimpleGUIComponent::setHeightInScreenClip( h );

		calculateMeshSize();
		MF_ASSERT( mesh_->size() <= INT_MAX );
		nVertices_ = ( int )mesh_->size();
		bw_safe_delete_array(blueprint_);
		blueprint_ = new GUIVertex[nVertices_];

		bw_safe_delete_array(vertices_);
		vertices_ = new GUIVertex[nVertices_];

		// TODO: use indices again if the real cause for the flickering problem
		// is found and fixed, or remove it once and for all
		// No longer using indices, since the new 'drawIntoMesh' generates the
		// vertices in the correct order to render with drawPrimitiveUP. The 
		// previous method of rendering with drawIndexedPrimitiveUP was generating
		// an unexplained flickering of text on nVidia cards.
		//nIndices_ = label_.length() * 6;
		//indices_ = new uint16[nIndices_];
		//memcpy( indices_, font_->indices().s_indices, nIndices_ * sizeof( uint16 ) );

		//now, rip out the font vertices
		if (mesh_->size() > 0)
		{
			memcpy( blueprint_, &mesh_->front(),  mesh_->size() * sizeof( (*mesh_)[0] ) );
		}
	}
	else
	{
		nVertices_ = 0;
	}
}


/**
 *	This method overrides SimpleGUIComponent::drawSelf and sets up the
 *	font cache divisor for our shader (the font cache divisor allows
 *	the font cache to grow, without invalidating pre-existing string meshes)
 */
void TextGUIComponent::drawSelf( bool reallyDraw, bool overlay )
{
	if (font_)
	{
		FontManager::instance().prepareMaterial( *font_, runTimeColour(), pixelSnap(), technique_ );
		SimpleGUIComponent::drawSelf( reallyDraw, overlay );
	}
}


void TextGUIComponent::calculateMeshSize()
{
	BW_GUARD;
	BoundingBox bb( Vector3(0,0,0), Vector3(0,0,0) );

	for ( int i = 0; i < mesh_->nVerts(); i++ )
	{
		bb.addBounds( reinterpret_cast<Vector3&>((*mesh_)[i].pos_) );
	}

	meshSize_ = bb.maxBounds() - bb.minBounds();
}


/**
 *	Load
 */
bool TextGUIComponent::load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings )
{
	BW_GUARD;
	if (!this->SimpleGUIComponent::load( pSect, ownerName, bindings )) return false;

	this->label( pSect->readWideString( "label", this->label() ) );
	this->font( pSect->readString( "font", this->font() ) );
	this->explicitSize_ = pSect->readBool( "explicitSize", this->explicitSize_ );
	this->multiline_ = pSect->readBool( "multiline", this->multiline_ );
	this->colourFormatting_ = pSect->readBool( "colourFormatting", this->colourFormatting_ );

	return true;
}

/**
 *	Save
 */
void TextGUIComponent::save( DataSectionPtr pSect, SaveBindings & bindings )
{
	BW_GUARD;
	this->SimpleGUIComponent::save( pSect, bindings );

	pSect->writeWideString( "label", this->label() );
	pSect->writeString( "font", this->font() );
	pSect->writeBool( "explicitSize", this->explicitSize_ );
	pSect->writeBool( "multiline", this->multiline_ );
	pSect->writeBool( "colourFormatting", this->colourFormatting_ );
}


/**
 *	This method sets the text item's font.
 *
 *	@param	fontName	The name of the font file.
 */
void TextGUIComponent::font( const BW::string& fontName )
{
	BW_GUARD;
	CachedFontPtr pFont = FontManager::instance().getCachedFont( fontName );
	if ( !pFont )
	{
		ERROR_MSG( "TextGUIComponent::font - font %s unknown\n", fontName.c_str() );
		pFont = FontManager::instance().getCachedFont( s_defaultFontName.value() );
	}

	font_ = pFont;
	if ( font_ && font_->pTexture() )
	{
		SimpleGUIComponent::textureName( font_->pTexture()->resourceID() );
		dirty_ = true;
	}
}


/**
 *	This method returns the font name.
 *
 *	@return	BW::string		The name of the font
 */
const BW::string TextGUIComponent::font() const
{
	BW_GUARD;
	if ( font_ )
	{
		return font_->metrics().cache().fontFileName();
	}
	else
	{
		return s_defaultFontName.value();
	}
}


static bool settingDimensions = false;

void TextGUIComponent::size( const Vector2 & size )
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


float TextGUIComponent::width() const
{
	BW_GUARD;
	if ( dirty_ )
	{
		const_cast<TextGUIComponent*>(this)->reset();
	}

	return SimpleGUIComponent::width();
}


void TextGUIComponent::width( float w )
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


float TextGUIComponent::height() const
{
	BW_GUARD;
	if ( dirty_ )
	{
		const_cast<TextGUIComponent*>(this)->reset();
	}

	return SimpleGUIComponent::height();
}


void TextGUIComponent::height( float h )
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


bool TextGUIComponent::buildMaterial()
{
	material_ = FontManager::instance().material();
	MF_ASSERT( material_ );
	technique_ = material_->pEffect()->pEffect()->GetTechnique(materialFX()-FX_ADD);
	return true;
}

BW_END_NAMESPACE

// text_gui_component.cpp
