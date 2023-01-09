
#include "pch.hpp"

#include "line_helper.hpp"
#include "moo/material.hpp"
#include "math/boundbox.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

LineHelper::LineHelper() :
	immediateLines_( D3DPT_LINELIST ),
	immediateLinesScreenSpace_( D3DPT_LINELIST )
{
	BW_GUARD;
}

LineHelper::~LineHelper() 
{
	BW_GUARD;
	// deallocate world space meshes
	for ( MeshListWS::iterator it = bufferedLines_.begin(); 
			it != bufferedLines_.end(); it++ )
	{
		bw_safe_delete(*it);
	}
	bufferedLines_.clear();
}

LineHelper& LineHelper::instance()
{
	static LineHelper s_instance;
	return s_instance;
}

// -----------------------------------------------------------------------------
// Section: Public methods
// -----------------------------------------------------------------------------

/**
 *	This method adds a world space line to the list and potentially draws
 *	it if the buffer has reached its limit.
 *	@param start/end The start/end of the line in world space.
 *	@param startColour/endColour Colour for the start/end of the line.
 *	@param life The amount of frames after the current one to draw this line.
 */
void LineHelper::drawLineTwoColour( const Vector3 & start, const Vector3 & end, 
	Moo::PackedColour startColour, Moo::PackedColour endColour,
	uint32 life )
{
	BW_GUARD;

	// we may build a mesh anew or render to the immediate mesh
	MeshWS* meshPtr = this->getMeshForLine( life );

	Moo::VertexXYZL vert;

	//Start
	vert.pos_ = start;
	vert.colour_ = startColour;
	meshPtr->push_back( vert );

	//End
	vert.pos_ = end;
	vert.colour_ = endColour;
	meshPtr->push_back( vert );
}


///Sames as the other drawLine, just for a single colour.
void LineHelper::drawLine( const Vector3 & start, const Vector3 & end, 
	Moo::PackedColour colour, uint32 life )
{
	BW_GUARD;

	drawLineTwoColour( start, end, colour, colour, life );
}


/**
 *	Draw coordinate axis lines (x = red, y = green, z = blue) with
 *	the specified world-space transform.
 *	@param size The length of the coordinate axis lines.
 *	@param worldTransform The world transform applied to the axes.
 *	@param xAxisStartColour/xAxisEndColour/yAxisStartColour/yAxisEndColour/
 *		zAxisStartColour/zAxisEndColour Colour to use when drawing the
 *		corresponding axis, with a gradient interpolating from start to end.
 *	@param life The amount of frames after the current one to draw this
 *		set of axes.
 *	@pre @a size is positive.
 *	@post Added axis lines to be drawn for the next @a life frames.
 */
void LineHelper::drawAxes( const Matrix & worldTransform, float size,
	Moo::PackedColour xAxisStartColour, Moo::PackedColour yAxisStartColour,
	Moo::PackedColour zAxisStartColour, Moo::PackedColour xAxisEndColour,
	Moo::PackedColour yAxisEndColour, Moo::PackedColour zAxisEndColour,
	uint32 life )
{
	BW_GUARD;
	MF_ASSERT( size >= 0.0f );

	const Vector3 origin( worldTransform.applyToOrigin() );

	//X
	drawLineTwoColour( origin,
		worldTransform.applyPoint( Vector3::I * size ),
		xAxisStartColour, xAxisEndColour, life );
	//Y
	drawLineTwoColour( origin,
		worldTransform.applyPoint( Vector3::J * size ),
		yAxisStartColour, yAxisEndColour, life );
	//Z
	drawLineTwoColour( origin,
		worldTransform.applyPoint( Vector3::K * size ),
		zAxisStartColour, zAxisEndColour, life );
}


///Same as DrawAxes, but the origin colour is the same on all axes.
void LineHelper::drawAxesCentreColoured(
	const Matrix & worldTransform, float size,
	Moo::PackedColour centreColour, Moo::PackedColour xAxisEndColour,
	Moo::PackedColour yAxisEndColour, Moo::PackedColour zAxisEndColour,
	uint32 life )
{
	BW_GUARD;
	MF_ASSERT( size >= 0.0f );

	drawAxes( worldTransform, size, centreColour, centreColour, centreColour,
		xAxisEndColour, yAxisEndColour, zAxisEndColour, life );
}


/**
 *	Draws a "star" (lots of lines centred on the specified world-space point).
 *	@param point The centre point of the star.
 *	@param radius The radius of lines from @a point.
 *	@param centreColour/edgeColour Colour to use for the lines, interpolating
 *		along each line from centreColour to edgeColour.
 *	@param life The amount of frames after the current one to draw this star.
 *	@pre @a radius is positive.
 *	@post Added axis lines to be drawn for the next @a life frames.
 */
void LineHelper::drawStar( const Vector3 & centre, float radius,
	Moo::PackedColour centreColour, Moo::PackedColour edgeColour,
	uint32 life )
{
	BW_GUARD;
	MF_ASSERT( radius >= 0.0f );

	const int NUM_SLICES = 8;

	Matrix rotY;
	Matrix rotX;
	Matrix rotFinal;
	Vector3 endPoint;
	for(int sliceY = 0; sliceY < NUM_SLICES; ++sliceY)
	{
		rotY.setRotateY( MATH_2PI * sliceY * (1.0f / NUM_SLICES) );

		for(int sliceX = 0; sliceX < NUM_SLICES; ++sliceX)
		{
			rotX.setRotateX( MATH_2PI * sliceX * (1.0f / NUM_SLICES) );

			rotFinal = rotX;
			rotFinal.postMultiply( rotY );

			endPoint = rotFinal.applyVector( Vector3::K * radius );
			endPoint += centre;

			drawLineTwoColour(
				centre, endPoint, centreColour, edgeColour, life );
		}
	}
}


/**
 *	This method adds a world space line to the list and potentially draws
 *	it if the buffer has reached its limit.
 *	@param start the start of the line
 *	@param end the end of the line
 *	@param colour the colour of the line as a 32 bit ARGB value
 */
void LineHelper::drawLineScreenSpace( const Vector4& start, const Vector4& end, 
	Moo::PackedColour colour )
{
	BW_GUARD;
	Moo::VertexTL vert;
	vert.pos_ = start;
	vert.colour_ = colour;
	immediateLinesScreenSpace_.push_back( vert );
	vert.pos_ = end;
	immediateLinesScreenSpace_.push_back( vert );

    if (immediateLinesScreenSpace_.size() > POINT_LIMIT)
		purgeScreenSpace();
}

void LineHelper::drawBoundingBox( const BoundingBox& bb, const Matrix& world,
	Moo::PackedColour colour )
{
	BW_GUARD;
	Vector3 diff = bb.maxBounds() - bb.minBounds();

	Vector3 corner = world.applyPoint( bb.minBounds() );
	Vector3 xAxis = world[0] * diff.x;
	Vector3 yAxis = world[1] * diff.y;
	Vector3 zAxis = world[2] * diff.z;

	static Vector3 points[8];
	points[0] = corner;
	points[1] = points[0] + zAxis;
	points[2] = points[1] + xAxis;
	points[3] = points[0] + xAxis;
	points[4] = corner + yAxis;
	points[5] = points[4] + zAxis;
	points[6] = points[5] + xAxis;
	points[7] = points[4] + xAxis;

	for (uint32 i = 0; i < 4; i++)
	{
		drawLine( points[i], points[i + 4], colour );
		uint32 ii = (i + 1) % 4;
		drawLine( points[i], points[ii], colour );
		drawLine( points[i + 4], points[ii + 4], colour );
	}
}

// draw a triangle
void LineHelper::drawTriangle(	const Vector3& v0, const Vector3& v1, 
								const Vector3& v2, 
								Moo::PackedColour	colour,
								uint32				life )
{
	BW_GUARD;
	Vector3 points[] = { v0, v1, v2, v0 };

	drawLineStrip( points, ARRAY_SIZE(points), colour, life );
}

// draw a strip of lines 
void LineHelper::drawLineStrip( const Vector3* points, uint32 length, 
								Moo::PackedColour colour,
								uint32 life )
{
	BW_GUARD;
	if ( length >= 2 )
	{
		// we may build a mesh anew or render to the immediate mesh
		MeshWS* meshPtr = getMeshForLine( life );

		// create start vert
		Moo::VertexXYZL vert;
		vert.colour_	= colour;

		// add each point
		meshPtr->reserve( length * 2 );
		for ( uint32 i = 1; i < length; i++ )
		{
			// p1
			vert.pos_ = points[i-1];
			meshPtr->push_back( vert );

			// p2
			vert.pos_ = points[i];
			meshPtr->push_back( vert );
		}
	}
}

/**
 *	This method purges the line buffers drawing them to the screen.
 */
void LineHelper::purge() 
{ 
	BW_GUARD;
	purgeScreenSpace(); 
	purgeWorldSpace(); 
}

// -----------------------------------------------------------------------------
// Section: Drawing
// -----------------------------------------------------------------------------

/*
 *	This method draws the screen space lines
 */
void LineHelper::purgeScreenSpace()
{
	BW_GUARD;
	if (immediateLinesScreenSpace_.size())
	{
		Moo::Material::setVertexColour();
		immediateLinesScreenSpace_.draw();
		immediateLinesScreenSpace_.clear();
	}
}

/*
 *	This method draws the world space lines
 */
void LineHelper::purgeWorldSpace()
{
	BW_GUARD;
	// set render states
	Moo::Material::setVertexColour();
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	// render immediate line entries
	immediateLines_.draw();
	immediateLines_.clear();

	// render all buffered line entries and decrease lifetime
	MeshListWS::iterator it;
	for ( it = bufferedLines_.begin(); 
		it != bufferedLines_.end(); it++ )
	{
		(*it)->mesh_.draw();
		(*it)->life_--;
	}

	// cull dead line entries
	it = bufferedLines_.begin();
	while ( it != bufferedLines_.end() )
	{
		if ( (*it)->life_ == 0 )
		{
			bw_safe_delete(*it);
			it = bufferedLines_.erase( it );
		}
		else
		{
			it++;
		}
	}
}

LineHelper::MeshWS* LineHelper::getMeshForLine( uint32 life )
{
	BW_GUARD;
	// we may build a mesh anew or render to the immediate mesh
	MeshWS* meshPtr = NULL;

	if ( life == 0 )
	{
		// set the pointer to our immediate mesh
		meshPtr = &immediateLines_;
	}
	else
	{	
		// create a line entry
		LineEntryWS* newLine = new LineEntryWS( life );

		// add line entry to list - list owns it now.
		bufferedLines_.push_back( newLine );

		// set the pointer to our new mesh
		meshPtr = &newLine->mesh_;
	}

	return meshPtr;
}

BW_END_NAMESPACE

// line_helper.cpp
