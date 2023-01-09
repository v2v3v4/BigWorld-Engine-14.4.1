#pragma once

#include "custom_mesh.hpp"
#include "moo/vertex_formats.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class allows you to wrap debug
 *	geometry calls into blocks, identified
 *	by markers.
 *
 *	If you then use this class to draw
 *	the debug geometry, you have the option
 *	of seeing only those draw calls for the
 *	specified draw mark.
 *
 *	The draw mark can be set in the watchers
 *	under render/debugGeometry/drawMark
 *
 *	The geometry will be cleared every frame, unless
 *	freeze is enabled.  Freeze is enabled by clicking
 *	the middle mouse button.  Once freeze is enabled,
 *	all geometry add / delete calls are ignored.  This
 *	allows you to examine a single frame of debug
 *	geometry.
 *	
 */
class GeometryDebugMarker
{
public:
	static GeometryDebugMarker& instance()
	{
		return s_instance;
	}

	void increment();
	void reset();
	void draw();
private:
	GeometryDebugMarker();
private:
	static GeometryDebugMarker s_instance;
	int	debugMarker_;
	int drawMark_;
	BW::vector<int> meshMarks_[2];
};


/** 
 *	This class allows you to visually
 *	debug geometric operations.
 *
 *	Shapes will be drawn as additive blocks,
 *	and will ignore the z-buffer.
 *
 *	The geometry will be cleared every frame, unless
 *	freeze is enabled.  Freeze is enabled by clicking
 *	the middle mouse button.  Once freeze is enabled,
 *	all geometry add / delete calls are ignored.  This
 *	allows you to examine a single frame of debug
 *	geometry.
 */
class DebugGeometry
{
public:
	static DebugGeometry& instance()
	{
		return s_instance;
	}

	//Add a triangle
	void add( const WorldTriangle& wt, uint32 colour );
	//Add a triangular prism made from one triangle and an extrusion vector
	void add( const WorldTriangle& wt, const Vector3& extent, uint32 colour );
	//Add a box made from two triangles and an extrusion vector
	void add( const WorldTriangle& wt, const WorldTriangle& wt2, const Vector3& extent, uint32 colour );
	//Add a local-space triangle, and transform the vertices to world space
	void add( const WorldTriangle& wt, const Matrix& m, uint32 colour );
	//Add a line
	void add( const Vector3& v1, const Vector3& v2, uint32 colour );
	//Add a triangle
	void add( const Vector3& v1, const Vector3& v2, const Vector3& v3, uint32 colour );

	void draw();
protected:
	//Draw range : useful in drawing debug tris only within certain debug markers.
	void drawRange( int from, int to );

	DebugGeometry( bool readZ = false):
	readZ_(readZ)
	{
	};

	void setMaterial();

	CustomMesh<Moo::VertexXYZL> mesh_;
	bool	readZ_;
	static DebugGeometry s_instance;
	friend class GeometryDebugMarker;
};


/** 
 *	This class allows you to visually
 *	debug geometric operations.
 *
 *	Shapes will be drawn as additive blocks,
 *	but will read from the z-buffer, so you can
 *	see how the debug triangles intersect with
 *	world geometry.
 *
 *	The geometry will be cleared every frame, unless
 *	freeze is enabled.  Freeze is enabled by clicking
 *	the middle mouse button.  Once freeze is enabled,
 *	all geometry add / delete calls are ignored.  This
 *	allows you to examine a single frame of debug
 *	geometry.
 */
class DebugGeometryZRead : public DebugGeometry
{
public:
	static DebugGeometryZRead& instance()
	{
		return s_instance;
	}
private:
	DebugGeometryZRead():
		DebugGeometry(true)
	{
	};

	static DebugGeometryZRead s_instance;
	friend class GeometryDebugMarker;
};

BW_END_NAMESPACE

// debug_geometry.hpp
