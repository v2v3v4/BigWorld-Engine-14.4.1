#include "pch.hpp"
#include "frame_gui_component.hpp"

#include "simple_gui.hpp"
#include "gui_shader.hpp"
#include "cstdmf/debug.hpp"
#include "moo/effect_material.hpp"

#ifndef CODE_INLINE
#include "frame_gui_component.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( FrameGUIComponent )

PY_BEGIN_METHODS( FrameGUIComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FrameGUIComponent )
	/*~ attribute FrameGUIComponent.edgeTextureName
	 *	@components{ client, tools }
	 *
	 *	This attribute is the name of the texture used on the edges of the
	 *	FrameGUIComponent.  Assigning a filename to this causes the edges to
	 *	load the named texture.
	 *
	 *	The specified image is assumed to be oriented correctly to be displayed
	 *	on the bottom edge.  It is mirrored	to be displayed on the top edge,
	 *	and rotated to be displayed on the side edges.
	 *
	 *	The edge's height on the top and bottom is specified using the
	 *	tiledHeight attribute, and it is tiled along the top and bottom using
	 *	the tiledWidth attribute.
	 *
	 *	The edge's width on the sides is specified using the tiledWidth
	 *	attribute, and it is tiled along the sides using the tiledHeight
	 *	attribute.
	 *
	 *	@type		A string, which must be the filename of a texture.
	 */
	PY_ATTRIBUTE( edgeTextureName )
	/*~ attribute FrameGUIComponent.cornerTextureName
	 *	@components{ client, tools }
	 *
	 *	This attribute is the name of the texture used on the corners of the
	 *	FrameGUIComponent.  Assigning a filename to this causes the corners to
	 *	load the named texture.
	 *
	 *	The specified texture is split into 4 equal quarters, each being a
	 *	corner.  These corners are rendered in the corresponding corner of the
	 *	FrameGUIComponent.  The size of each corner is specified by the
	 *	tiledWidth and tiledHeight attributes.
	 *
	 *	@type		A string, which must be the filename of a texture.
	 */
	PY_ATTRIBUTE( cornerTextureName )
PY_END_ATTRIBUTES()

/*~ function GUI.Frame
 *	@components{ client, tools }
 *
 *	This function creates a new FrameGUIComponent.  This is basically a tiled
 *  simpleGUIComponent with a frame around it.  The frame is specified by and
 *  edge texture and a corner texture.  The size of the frame is specified by
 *  the tiledWidth and tiledHeight, which are also used to specify the tiling
 *  of the background.
 *
 *	@param	backgroundTextureName	The filename of the background texture.
 *									This needs to be a valid texture file.
 *	@param	cornerTextureName		The filename of the corner texture.
 *									This needs to be a valid texture file.
 *	@param	edgeTextureName			The filename of the edge texture.
 *									This needs to be a valid texture file.
 *	@param	tiledWidth				The width to make the frame, and to tile
 *									the background at.
 *	@param	tiledHeight				The height to make the frame, and to tile
 *									the background at.
 */
PY_FACTORY_NAMED( FrameGUIComponent, "Frame", GUI )

COMPONENT_FACTORY( FrameGUIComponent )

//How to satisfy Borland.  Move render_context.hpp below
//the python attributes, in this file.
//why?  don't ask.  it works.
#include "moo/render_context.hpp"

/**
 *	This constructor is used when creating a FrameGUIComponent.
 *
 *	@todo Comment
 */
FrameGUIComponent::FrameGUIComponent( const BW::string& backgroundTextureName,
									const BW::string& frameTextureName,
									const BW::string& edgeTextureName,
									int tWidth, int tHeight,
									PyTypeObject * pType )
:SimpleGUIComponent( backgroundTextureName, pType )
{
	BW_GUARD;
	tiled_ = true;

	this->tileWidth( tWidth );
	this->tileHeight( tHeight );

	for ( int i = 0; i < 4; i++ )
	{
		corners_[i] = new SimpleGUIComponent( frameTextureName );
		corners_[i]->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
		corners_[i]->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
		corners_[i]->horizontalPositionMode( SimpleGUIComponent::POSITION_MODE_LEGACY );
		corners_[i]->verticalPositionMode( SimpleGUIComponent::POSITION_MODE_LEGACY );
		edges_[i] = new SimpleGUIComponent( edgeTextureName );
		edges_[i]->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
		edges_[i]->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
		edges_[i]->horizontalPositionMode( SimpleGUIComponent::POSITION_MODE_LEGACY );
		edges_[i]->verticalPositionMode( SimpleGUIComponent::POSITION_MODE_LEGACY );
	}

	Vector2 mc[4];

	mc[0].set( 0.f, 0.f );
	mc[1].set( 0.f, 0.5 );
	mc[2].set( 0.5, 0.5 );
	mc[3].set( 0.5, 0.f );
	corners_[0]->anchor( SimpleGUIComponent::ANCHOR_H_LEFT, ANCHOR_V_TOP );
	edges_[0]->anchor( SimpleGUIComponent::ANCHOR_H_LEFT, ANCHOR_V_CENTER );
	edges_[0]->angle( SimpleGUIComponent::ROT_90 );
	corners_[0]->mapping( mc );

	mc[0].set( 0.f, 0.5 );
	mc[1].set( 0.f, 1.f );
	mc[2].set( 0.5, 1.f );
	mc[3].set( 0.5, 0.5 );
	corners_[1]->anchor( SimpleGUIComponent::ANCHOR_H_LEFT, ANCHOR_V_BOTTOM );
	edges_[1]->anchor( SimpleGUIComponent::ANCHOR_H_CENTER, ANCHOR_V_BOTTOM );
	edges_[1]->angle( SimpleGUIComponent::ROT_0 );
	corners_[1]->mapping( mc );

	mc[0].set( 0.5, 0.5 );
	mc[1].set( 0.5, 1.f );
	mc[2].set( 1.f, 1.f );
	mc[3].set( 1.f, 0.5 );
	corners_[2]->anchor( SimpleGUIComponent::ANCHOR_H_RIGHT, ANCHOR_V_BOTTOM );
	edges_[2]->anchor( SimpleGUIComponent::ANCHOR_H_RIGHT, ANCHOR_V_CENTER );
	edges_[2]->angle( SimpleGUIComponent::ROT_270 );
	corners_[2]->mapping( mc );

	mc[0].set( 0.5, 0.f );
	mc[1].set( 0.5, 0.5 );
	mc[2].set( 1.f, 0.5 );
	mc[3].set( 1.f, 0.f );
	corners_[3]->anchor( SimpleGUIComponent::ANCHOR_H_RIGHT, ANCHOR_V_TOP );
	edges_[3]->anchor( SimpleGUIComponent::ANCHOR_H_CENTER, ANCHOR_V_TOP );
	edges_[3]->angle( SimpleGUIComponent::ROT_180 );
	corners_[3]->mapping( mc );
}


/**
 *	@todo Comment.
 */
FrameGUIComponent::~FrameGUIComponent()
{
	BW_GUARD;
	for (int i = 0; i < 4; i++)
	{
		Py_DECREF( corners_[i] );
		Py_DECREF( edges_[i] );
	}
}


/**
 *	Static python factory method
 */
PyObject * FrameGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	char	* backName, * frameName, * edgeName;
	int		tileWidth,	tileHeight;

	if (!PyArg_ParseTuple( args, "sssii",
		&backName, &frameName, &edgeName,
		&tileWidth, &tileHeight ) )
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Frame: "
			"Argument parsing error: "
			"Expected 3 texture names then two integers" );
		return NULL;
	}

	return new FrameGUIComponent(
		backName, frameName, edgeName, tileWidth, tileHeight );
}



/**
 * This override of update bakes texture coordinates
 * into the underlying gui component, to tile our
 * texture across the whole component but with a constant
 * tile size in pixels.
 */
void FrameGUIComponent::update( float dTime, float relParentWidth, float relParentHeight )
{
	BW_GUARD;
	SimpleGUIComponent::update( dTime, relParentWidth, relParentHeight );

	//Moo::TextureStage& ts = material_.textureStage( 0 );
	//if ( ts.pTexture() )
	{
		//ts.textureWrapMode( Moo::TextureStage::REPEAT );

		float widthInClipCoords = vertices_[2].pos_.x - vertices_[0].pos_.x;
		float heightInClipCoords = vertices_[0].pos_.y - vertices_[2].pos_.y;

		float sw, sh;	//width, height in screen coordinates( pixels )
		SimpleGUI::instance().clipRangesToPixel( widthInClipCoords, heightInClipCoords, &sw, &sh );

		float tuMin, tuMax;
		float tvMin, tvMax;

		tuMin = 0.f;
		tuMax = sw / this->tileWidth();
		tvMin = 0.f;
		tvMax = sh / this->tileHeight();

		GUIVertex* v;

		v = &vertices_[0];
		v->uv_[0] = tuMin;
		v->uv_[1] = tvMin;

		v = &vertices_[1];
		v->uv_[0] = tuMin;
		v->uv_[1] = tvMax;

		v = &vertices_[2];
		v->uv_[0] = tuMax;
		v->uv_[1] = tvMax;

		v = &vertices_[3];
		v->uv_[0] = tuMax;
		v->uv_[1] = tvMin;

		float halfHeight = heightInClipCoords / 2.f;
		float halfWidth = widthInClipCoords / 2.f;
		float singleWidth = widthInClipCoords / tuMax;
		float singleHeight = heightInClipCoords / tvMax;

		edges_[0]->position( Vector3( vertices_[0].pos_.x, vertices_[0].pos_.y - halfHeight, vertices_[1].pos_.z ) );
		edges_[0]->width( ( widthInClipCoords / tuMax ) );
		edges_[0]->height( heightInClipCoords - ( 2.f * singleHeight ) );

		edges_[1]->position( Vector3( vertices_[0].pos_.x + halfWidth, vertices_[1].pos_.y, vertices_[1].pos_.z ) );
		edges_[1]->width( widthInClipCoords - ( 2.f * singleWidth ) );
		edges_[1]->height( ( heightInClipCoords /tvMax ) * 1.f );

		edges_[2]->position( Vector3( vertices_[2].pos_.x, vertices_[0].pos_.y - halfHeight, vertices_[1].pos_.z ) );
		edges_[2]->width( ( widthInClipCoords / tuMax ) * 1.f );
		edges_[2]->height( heightInClipCoords - ( 2.f * singleHeight ) );

		edges_[3]->position( Vector3( vertices_[0].pos_.x + halfWidth, vertices_[3].pos_.y, vertices_[1].pos_.z ) );
		edges_[3]->width( widthInClipCoords - ( 2.f * singleWidth ) );
		edges_[3]->height( ( heightInClipCoords / tvMax ) * 1.f );

		for ( int i = 0; i < 4; i++ )
		{
			v = &vertices_[i];
			corners_[i]->position( Vector3(v->pos_.x, v->pos_.y, v->pos_.z) );
			corners_[i]->width( singleWidth );
			corners_[i]->height( singleHeight );
			corners_[i]->colour( colour() | 0xff000000 );
			edges_[i]->colour( colour() | 0xff000000 );
			corners_[i]->filterType( filterType() );
			edges_[i]->filterType( filterType() );
			corners_[i]->materialFX( materialFX() );
			edges_[i]->materialFX( materialFX() );
			corners_[i]->update( dTime, relParentWidth, relParentHeight );
			edges_[i]->update( dTime, relParentWidth, relParentHeight );
		}

	}
}


/**
 *	@todo Comment
 */
void FrameGUIComponent::applyShaders( float dTime )
{
	BW_GUARD;
	GUIShaderPtrVector::iterator it = shaders_.begin();
	GUIShaderPtrVector::iterator end = shaders_.end();

	while( it != end )
	{
		this->applyShader( *it->second, dTime );
		it++;
	}

	//Apply corner and edge specific shaders
	for ( int i = 0; i < 4; i++ )
	{
		corners_[i]->applyShaders( dTime );
		edges_[i]->applyShaders( dTime );
	}

	//Apply child shaders
	ChildRecVector::iterator cit = children_.begin();
	ChildRecVector::iterator cend = children_.end();

	while( cit != cend )
	{
		cit->second->applyShaders( dTime );
		cit++;
	}
}


/**
 *	@todo Comment
 */
void FrameGUIComponent::applyShader( GUIShader& shader, float dTime )
{
	BW_GUARD;
	bool traverseKids = shader.processComponent( *this, dTime );

	for ( int i = 0; i < 4; i++ )
	{
		corners_[i]->applyShader( shader, 0.f );
		edges_[i]->applyShader( shader, 0.f );
	}

	if ( traverseKids )
	{
		ChildRecVector::iterator cit = children_.begin();
		ChildRecVector::iterator cend = children_.end();

		while( cit != cend )
		{
			cit->second->applyShader( shader, 0.f );
			cit++;
		}
	}
}


/**
 *	@todo Comment
 */
void FrameGUIComponent::draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay )
{
	BW_GUARD;
	if ( visible() )
	{
		Moo::rc().push();
		Moo::rc().preMultiply( runTimeTransform() );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

		SimpleGUIComponent::drawSelf(reallyDraw, overlay);
		SimpleGUIComponent::drawChildren( drawContext, reallyDraw, overlay );

		Moo::rc().pop();
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

		for ( int i = 0; i < 4; i++ )
		{
			edges_[i]->draw( drawContext, reallyDraw, overlay );
			corners_[i]->draw( drawContext, reallyDraw, overlay );
		}
	}

	momentarilyInvisible( false );
}


/**
 *	Load
 */
bool FrameGUIComponent::load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings )
{
	BW_GUARD;
	if (!this->SimpleGUIComponent::load( pSect, ownerName, bindings )) return false;

	this->edgeTextureName(
		pSect->readString( "edgeTextureName", this->edgeTextureName() ) );
	this->cornerTextureName(
		pSect->readString( "cornerTextureName", this->cornerTextureName() ) );

	return true;
}

/**
 *	Save
 */
void FrameGUIComponent::save( DataSectionPtr pSect, SaveBindings & bindings )
{
	BW_GUARD;
	this->SimpleGUIComponent::save( pSect, bindings );

	pSect->writeString( "edgeTextureName", this->edgeTextureName() );
	pSect->writeString( "cornerTextureName", this->cornerTextureName() );
}

BW_END_NAMESPACE

// frame_gui_component.cpp
