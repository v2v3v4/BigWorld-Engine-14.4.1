#include "pch.hpp"
#include "frame_gui_component2.hpp"

#include "simple_gui.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( FrameGUIComponent2 )

PY_BEGIN_METHODS( FrameGUIComponent2 )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FrameGUIComponent2 )
PY_END_ATTRIBUTES()


/*~ function GUI.Frame2
 *	@components{ client, tools }
 *
 *	This function creates a new FrameGUIComponent2.  This is a resizable
 *	frame created from a single texture and a single draw call.
 *	The frame requires specific artwork, please see the content creation
 *	guide for details.
 */
PY_FACTORY_NAMED( FrameGUIComponent2, "Frame2", GUI )

COMPONENT_FACTORY( FrameGUIComponent2 )

//How to satisfy Borland.  Move render_context.hpp below
//the python attributes, in this file.
//why?  don't ask.  it works.
#include "moo/render_context.hpp"

/**
 *	This constructor is used when creating a FrameGUIComponent2. 
 */
FrameGUIComponent2::FrameGUIComponent2( const BW::string& textureName, PyTypeObject * pType )
:SimpleGUIComponent( textureName, pType )
{
	BW_GUARD;
	tiled_ = true;
	this->buildMesh();
}


/**
 *	Static python factory method
 */
PyObject * FrameGUIComponent2::pyNew( PyObject * args )
{
	BW_GUARD;
	char	* textureName;	

	if (!PyArg_ParseTuple( args, "s",
		&textureName ) )
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Frame: "
			"Argument parsing error: "
			"Expected a texture name" );
		return NULL;
	}

	return new FrameGUIComponent2( textureName );
}


/**
 *	This private method sets the the positions and uvs for a particular
 *	quad.
 *
 *	The parameters are (x,y) pairs for the 4 corners, then (u,v) pairs.
 *	The order of vertices are topLeft, topRight, bottomRight, bottomLeft.
 */
GUIVertex* FrameGUIComponent2::setQuad(
	GUIVertex* pFirstVert,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3,
	float x4, float y4,
	float u1, float v1,
	float u2, float v2,
	float u3, float v3,
	float u4, float v4 )
{
	BW_GUARD;
	GUIVertex* v = pFirstVert;

	v->pos_.x = x1;
	v->pos_.y = y1;
	v->pos_.z = 1.f;
	v->uv_[0] = u1;
	v->uv_[1] = v1;
	v->colour_ = 0xffffffff;
	v++;

	v->pos_.x = x2;
	v->pos_.y = y2;
	v->pos_.z = 1.f;
	v->uv_[0] = u2;
	v->uv_[1] = v2;
	v->colour_ = 0xffffffff;
	v++;

	v->pos_.x = x3;
	v->pos_.y = y3;
	v->pos_.z = 1.f;
	v->uv_[0] = u3;
	v->uv_[1] = v3;
	v->colour_ = 0xffffffff;
	v++;

	v->pos_.x = x4;
	v->pos_.y = y4;
	v->pos_.z = 1.f;
	v->uv_[0] = u4;
	v->uv_[1] = v4;
	v->colour_ = 0xffffffff;
	v++;

	return v;
}


/**
 *	This method updates the component.  It overrides SGC::update to make sure
 *	updateVertices is called.
 */
void FrameGUIComponent2::update( float dTime, float relParentWidth, float relParentHeight )
{
	BW_GUARD;
	float x,y,w,h;
	this->layout( relParentWidth, relParentHeight, x, y, w, h );

	memcpy( vertices_, blueprint_, nVertices_ * sizeof( GUIVertex ) );	
	this->updateVertices( &vertices_[0], relParentWidth, relParentHeight );

	runTimeColour( colour() );
	runTimeTransform( Matrix::identity );

	//reset run-time clip region
	static Vector4 fullscreen( -1.f, 1.f, 1.f, -1.f );
	runTimeClipRegion_ = fullscreen;

	//now, we have a drawable set of vertices.

	updateChildren( dTime, relParentWidth, relParentHeight );
}



/**
 *	This function builds the frame mesh.
 */
void FrameGUIComponent2::buildMesh( void )
{
	BW_GUARD;
	cleanMesh();

	uint32 nQuads = 13;
	nVertices_ = nQuads * 4;
	nIndices_ = nQuads * 6;

	blueprint_ = new GUIVertex[nVertices_];
	vertices_ = new GUIVertex[nVertices_];
	indices_ = new uint16[nIndices_];

	this->updateVertices(&blueprint_[0], 2.f, 2.f);	

	//all quads have similar indices, its all made of quads
	uint32 idx = 0;
	for (uint32 i=0; i<nQuads; i++)
	{
		uint32 base = i * 4;
		indices_[idx++] = base + 0;
		indices_[idx++] = base + 1;
		indices_[idx++] = base + 2;
		indices_[idx++] = base + 0;
		indices_[idx++] = base + 2;
		indices_[idx++] = base + 3;
	}
}


/**
 *	This methods updates the positions of the existing set of vertices, to match the
 *	current width and height of the component.  See the content creation manual to
 *	fully understand the required layout of the source texture.
 */
void FrameGUIComponent2::updateVertices( GUIVertex* v, float relativeParentWidth, float relativeParentHeight )
{
	BW_GUARD;
	//layout variables.
	float x,y,w,height;
	this->layout( relativeParentWidth, relativeParentHeight, x, y, w, height );	

	float texelW, texelH;
	if (texture_.hasObject())
	{
		texelW = (float)texture_->width();
		texelH = (float)texture_->height();
	}
	else
	{
		return;
	}

	float hmargin,vmargin;
	SimpleGUI::instance().pixelRangesToClip(texelW,texelW,&hmargin,&vmargin);

	float pixelW,pixelH;
	SimpleGUI::instance().clipRangesToPixel(w,height,&pixelW,&pixelH);

	const float nRepeatsW = (pixelW - 2*texelW) / texelW;
	const float nRepeatsH = (pixelH - 2*texelW) / texelW;

	//Note on the following variables.
	//lower case are all positions
	//upper case are all UVs.
	//They are all 'rulers', or 'slices' in html parlance.

	//The general layout of these rulers.  They represent
	//vertex positions.
	//So in the following diagram, ab encompass the left
	//edge.  bcd is the top edge, which is split at c because
	//we want symmetrical tiling of the edge.  de is the
	//right edge.
	//			a b		c		d e
	//		f
	//		g
	//
	//
	//		h
	//
	//
	//		i
	//		j
	//
	//For the UV rulers, they are laid out like this :
	//
	//L								P
	//		M				O
	//				 N
	//				A B
	//			C
	//			D
	//			E
	//			F
	//			G
	//			H
	//			I
	//			J
	//			K

	//These are 5 rulers representing the 5 parts of the geometry,
	//From left to right.
	const float a = x;
	const float b = x + hmargin;
	const float c = x+w/2.f;
	const float d = x+w - hmargin;
	const float e = x+w;

	//These are 5 rulers representing the 5 parts of the geometry,
	//From top to bottom.
	const float f = y;
	const float g = y - vmargin;
	const float h = y-height/2.f;
	const float i = y-height + vmargin;
	const float j = y-height;

	//UVS
	//These are 2 rulers representing the left and right of the texture.
	const float A = 0.f;
	const float B = 1.f;

	//These rulers are each 1/8th more of the height of the texture map.
	//They correspond to the 8 parts of the texture atlas, from top to
	//bottom.
	const float C = 0.f;
	const float D = 0.125f;
	const float E = 0.25f;
	const float F = 0.375f;
	const float G = 0.5f;
	const float H = 0.625f;
	const float I = 0.75f;
	const float J = 0.875f;
	const float K = 1.f;

	//These rulers are used for the UV of the edge pieces.  They allow
	//the edges to tile correctly to the current size of the frame.
	const float L = 0.5f - nRepeatsW/2.f;
	const float M = 0.5f - nRepeatsH/2.f;
	const float N = 0.5f;
	const float O = 0.5f + nRepeatsH/2.f;
	const float P = 0.5f + nRepeatsW/2.f;

	//setQuad takes (left,top) (right,top), (right,bottom), (left,bottom), texture atlas part index	

	//center area
	//if these are the first four vertices, then some other GUI functionality like hittests work.
	v = this->setQuad( v, b,g, d,g, d,i, b,i, N,D, N,D, N,D, N,D );

	//top left corner	
	v = this->setQuad( v, a,f, b,f, b,g, a,g, A,C, B,C, B,D, A,D );
	//top right corner
	v = this->setQuad( v, d,f, e,f, e,g, d,g, A,E, B,E, B,F, A,F );
	//bottom right corner
	v = this->setQuad( v, d,i, e,i, e,j, d,j, A,F, B,F, B,G, A,G );
	//bottom left corner
	v = this->setQuad( v, a,i, b,i, b,j, a,j, A,D, B,D, B,E, A,E );

	//top edge, left half
	v = this->setQuad( v, b,f, c,f, c,g, b,g, L,G, N,G, N,H, L,H );
	//top edge, right half
	v = this->setQuad( v, c,f, d,f, d,g, c,g, N,G, P,G, P,H, N,H );

	//right edge, top half
	v = this->setQuad( v, d,g, e,g, e,h, d,h, M,J, M,I, N,I, N,J );
	//right edge, bottom half
	v = this->setQuad( v, d,h, e,h, e,i, d,i, N,J, N,I, O,I, O,J );

	//bottom edge, left half
	v = this->setQuad( v, b,i, c,i, c,j, b,j, L,H, N,H, N,I, L,I );
	//bottom edge, right half
	v = this->setQuad( v, c,i, d,i, d,j, c,j, N,H, P,H, P,I, N,I );

	//left edge, top half
	v = this->setQuad( v, a,g, b,g, b,h, a,h, M,K, M,J, N,J, N,K );
	//left edge, bottom half
	/*v =*/ this->setQuad( v, a,h, b,h, b,i, a,i, N,K, N,J, O,J, O,K );	
}

BW_END_NAMESPACE

// frame_gui_component2.cpp
