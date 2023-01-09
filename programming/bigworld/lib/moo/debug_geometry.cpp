#include "pch.hpp"
#include "debug_geometry.hpp"

#include "input/input.hpp"
#include "physics2/worldtri.hpp"
#include "moo/material.hpp"


BW_BEGIN_NAMESPACE

GeometryDebugMarker GeometryDebugMarker::s_instance;
DebugGeometry DebugGeometry::s_instance;
DebugGeometryZRead DebugGeometryZRead::s_instance;
bool s_freeze = false;

namespace Util
{

/**
 *	This method checks the input devices and enables
 *	or disables freeze appropriately.
 *
 *	Debug geometry will be reset every frame, unless
 *	freeze is enabled.  Freeze is enabled by clicking
 *	the middle mouse button.  Once freeze is enabled,
 *	all geometry add / delete calls are ignored.  This
 *	allows you to examine a single frame of debug
 *	geometry.
 *
 *	To unfreeze the debug geometry, press the escape key.
 */
void checkForFreezeCondition()
{
	BW_GUARD;
	if ( InputDevices::isKeyDown( KeyCode::KEY_MIDDLEMOUSE ) )
	{
		s_freeze = true;
	}
	if ( InputDevices::isKeyDown( KeyCode::KEY_ESCAPE ) )
	{
		s_freeze = false;
	}
}

} // namespace Util

/**
 *	Constructor.
 */
GeometryDebugMarker::GeometryDebugMarker():
	debugMarker_(0),
	drawMark_(0)
{
	BW_GUARD;
	reset();
	MF_WATCH( "Render/DebugGeometry/nMarks",
		debugMarker_,
		Watcher::WT_READ_WRITE,
		"upper range of displyed debug marks." );
	MF_WATCH( "Render/DebugGeometry/drawMark",
		drawMark_,
		Watcher::WT_READ_WRITE,
		"lower range of displayed debug marks (0 to display all)" );
};


/**
 *	Destructor.
 */
void GeometryDebugMarker::increment()
{
	BW_GUARD;
	if ( s_freeze )
		return;

	debugMarker_++;
	meshMarks_[0].push_back( (int)DebugGeometryZRead::instance().mesh_.size() );
	meshMarks_[1].push_back( (int)DebugGeometry::instance().mesh_.size() );
}


/**
 *	This method resets the all of the markers.
 *
 *	This method is called by GeometryDebugMarker::draw.
 *
 *	This method must be called manually if you call draw()
 *	on either of the DebugGeometry classes and not through
 *	the marker::draw interface, or else the meshMarks will
 *	build up and become irrelevant.
 */
void GeometryDebugMarker::reset()
{
	BW_GUARD;
	if ( s_freeze )
		return;

	debugMarker_ = 0;
	drawMark_ = 0;
	meshMarks_[0].clear();
	meshMarks_[1].clear();
	meshMarks_[0].push_back( 0 );
	meshMarks_[1].push_back( 0 );
}


/**
 * This method draws both DebugGeometry classes,
 * selecting only the geometry between the selected
 * debug marker, if any is selected.
 *
 * If you are using markers at all, then you should
 * always call this draw method.
 */
void GeometryDebugMarker::draw()
{
	BW_GUARD;
	Util::checkForFreezeCondition();

	if ( drawMark_ < 1 )
	{
		DebugGeometryZRead::instance().draw();
		DebugGeometry::instance().draw();
	}
	else if ( drawMark_ < debugMarker_ )
	{
		int from, to;

		to = meshMarks_[0][drawMark_];
		from = meshMarks_[0][drawMark_-1];
		DebugGeometryZRead::instance().drawRange( from, to );

		to = meshMarks_[1][drawMark_];
		from = meshMarks_[1][drawMark_-1];
		DebugGeometry::instance().drawRange( from, to );
	}

	if ( !s_freeze )
	{
		reset();
	}
}



/**
 *	This method adds a world triangle to the debug geometry.
 *
 *	@param wt		WorldTriangle to draw
 *	@param colour	uint32 colour for the triangle.	
 */
void DebugGeometry::add( const WorldTriangle& wt, uint32 colour )
{
	BW_GUARD;
	add( wt.v0(), wt.v1(), wt.v2(), colour );
}


/**
 *	This method adds an extruded world triangle as a solid prism
 *	to the debug geometry.
 *
 *	@param wt		WorldTriangle to draw
 *	@param extent	Extent of the extrusion
 *	@param colour	uint32 colour for the triangular prism.	
 */
void DebugGeometry::add( const WorldTriangle& wt, const Vector3& extent, uint32 colour )
{
	BW_GUARD;
	//create back triangles
	Vector3 bt[3];
	bt[0] = wt.v0() + extent;
	bt[1] = wt.v1() + extent;
	bt[2] = wt.v2() + extent;

	//draw front + back
	add( wt, colour );
	add( bt[0], bt[1], bt[2], colour );

	//create sides
	add( wt.v0(), wt.v1(), bt[0], colour );
	add( wt.v1(), bt[0], bt[1], colour );

	add( wt.v1(), wt.v2(), bt[1], colour );
	add( wt.v2(), bt[1], bt[2], colour );

	add( wt.v2(), wt.v0(), bt[2], colour );
	add( wt.v0(), bt[2], bt[0], colour );
}


/**
 *	This method adds an solid box made up from two world triangles
 *	and an extrusion vector to the debug geometry.
 *
 *	@param wt		WorldTriangle1 to draw
 *	@param wt2		WorldTriangle2 to draw
 *	@param extent	Extent of the extrusion
 *	@param colour	uint32 colour for the box.	
 */
void DebugGeometry::add( const WorldTriangle& wt, const WorldTriangle& wt2, const Vector3& extent, uint32 colour )
{
	BW_GUARD;
	//create back triangles
	Vector3 bt[2][3];
	bt[0][0] = wt.v0() + extent;
	bt[0][1] = wt.v1() + extent;
	bt[0][2] = wt.v2() + extent;
	bt[1][0] = wt2.v0() + extent;
	bt[1][1] = wt2.v1() + extent;
	bt[1][2] = wt2.v2() + extent;

	add( wt, colour );
	add( wt2, colour );
	add( bt[0][0], bt[0][1], bt[0][2], colour );
	add( bt[1][0], bt[1][1], bt[1][2], colour );

	//create sides
	add( wt.v0(), wt.v1(), bt[0][0], colour );
	add( wt.v1(), bt[0][0], bt[0][1], colour );

	add( wt2.v0(), wt2.v1(), bt[1][0], colour );
	add( wt2.v1(), bt[1][0], bt[1][1], colour );

	add( wt.v1(), wt.v2(), bt[0][1], colour );
	add( wt.v2(), bt[0][1], bt[0][2], colour );

	add( wt2.v1(), wt2.v2(), bt[1][1], colour );
	add( wt2.v2(), bt[1][1], bt[1][2], colour );
}


/**
 *	This method adds a world triangle in local space to 
 *	the debug geomtetry.  A transformation matrix is
 *	passed in so the triangle can be drawn in world space.
 *
 *	@param wt		WorldTriangle1 to draw
 *	@param m		ObjectToWorld matrix for the triangle
 *	@param colour	uint32 colour for the triangle.	
 */
void DebugGeometry::add( const WorldTriangle& wt, const Matrix& m, uint32 colour )
{
	BW_GUARD;
	Vector3 v[3];
	v[0] = m.applyPoint( wt.v0() );
	v[1] = m.applyPoint( wt.v1() );
	v[2] = m.applyPoint( wt.v2() );
	add( v[0], v[1], v[2], colour );
}


/**
 *	This method adds a triangle to  the debug geomtetry.
 *	This method takes three individual vectors, instead of
 *	a WorldTriangle object.
 *
 *	@param v0		Vertex number 1 of the triangle
 *	@param v1		Vertex number 2 of the triangle
 *	@param v2		Vertex number 3 of the triangle
 *	@param colour	uint32 colour for the triangle.	
 */
void DebugGeometry::add( const Vector3& v0, const Vector3& v1, const Vector3& v2, uint32 colour )
{
	BW_GUARD;
	if ( s_freeze )
		return;

	Moo::VertexXYZL v;
	v.colour_ = colour;
	v.pos_ = v0;
	mesh_.push_back( v );
	v.pos_ = v1;
	mesh_.push_back( v );
	v.pos_ = v2;
	mesh_.push_back( v );
}


/**
 *	This method draws the DebugGeometry ( z-disabled only ).
 *	If you call this method, and also have marker calls, then
 *	make sure you call GeometryDebugMarker::reset() as well.
 */
void DebugGeometry::draw()
{	
	BW_GUARD;
	Util::checkForFreezeCondition();
	setMaterial();
	mesh_.draw();
	if ( !s_freeze )
		mesh_.clear();
}


/**
 *	This method draws the DebugGeometry, but only between
 *	the specified vertices.
 *
 *	This method is private to the class, and is only called
 *	by GeometryDebugMarker::draw, if a marker range is selected. 
 */
void DebugGeometry::drawRange( int from, int to )
{
	BW_GUARD;
	setMaterial();
	mesh_.drawRange(from,to);
	if ( !s_freeze )
		mesh_.clear();
}


/**
 *	This method is called internally by the DebugGeometry clases,
 *	and sets up an appropriate material to draw with.
 */
void DebugGeometry::setMaterial()
{
	BW_GUARD;
	Moo::Material::setVertexColour();
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	if ( readZ_ )
	{
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
	}
	else
	{
		Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
	}
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
}

BW_END_NAMESPACE

// debug_geomentry.cpp
