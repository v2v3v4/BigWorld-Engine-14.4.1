#ifndef GEOMETRICS_HPP
#define GEOMETRICS_HPP

#include <iostream>
#include "moo/moo_math.hpp"
#include "moo/effect_material.hpp"
#include "math/boundBox.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/material.hpp"
#include "custom_mesh.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class provides utility geometrics.
 *
 *	Available are :
 *	- points
 *	- line
 *	- rectangle
 *	- textured rectangle
 *	- bounding box in wireframe
 *	- colourised/alpharised sphere
 */
class Geometrics
{
public:
	Geometrics();
	~Geometrics();

	static Geometrics & instance();

	static void clearSwizzledViewport();

	static void createRectMesh( const Vector2& topLeft,
		const Vector2& bottomRight,
		const Moo::Colour& colour,
		CustomMesh<Moo::VertexTL>& result,
		float depth = 0  );

	static void createRectMesh( const Vector2& topLeft,
		const Vector2& bottomRight,
		const Moo::Colour& colour,
		bool linearUV, 
		CustomMesh<Moo::VertexTLUV>& mesh );

	static void createRectMesh( const Vector2& topLeft,
		const Vector2& bottomRight,
		const Moo::Colour& colour,	
		CustomMesh<Moo::VertexXYZUV>& mesh );

	static void createRectMesh( const Vector2& topLeft,
		const Vector2& bottomRight,
		const Vector2& tlUV,
		const Vector2& brUV,
		const Moo::Colour& colour,
		CustomMesh<Moo::VertexTLUV>& mesh );

	static void setVertexColour( const Moo::Colour& colour );
	static void setVertexColourMod2( const Moo::Colour& colour );

	static void	drawRect( const Vector2& topLeft, 
			const Vector2& bottomRight, 
			const Moo::Colour& colour, float depth = 0 );

	static void drawRect( const Vector2& topLeft,
		const Vector2& bottomRight,
		Moo::EffectMaterial& material,
		bool linearUV = false );	

	static void	drawShadowPoly( const Moo::Colour& colour );

	static void	drawUnitSquareOnXZPlane( const Moo::Colour& colour );

	//draw a screen-space rectangle.
	//pixel/texel alignment is automatically taken care of
	static void	texturedRect( const Vector2& topLeft, 
			const Vector2& bottomRight, 
			const Moo::Colour& colour,
			bool linearUV = false );

	//draw a screen-space rectangle.
	//pixel/texel alignment is automatically taken care of
	static void	texturedRect( const Vector2& topLeft, 
			const Vector2& bottomRight,
			const Vector2& topLeftUV, 
			const Vector2& bottomRightUV,
			const Moo::Colour& colour,
			bool singleMip = false );

	static void texturedUnitWorldRectOnXZPlane( const Moo::Colour& colour );

	static void	drawPoints( const Vector3* points, 
			uint32 count, 
			float pointSize,
			const Moo::Colour& colour );

		static void	drawLine( const Vector3& start, 
		const Vector3& end, 
		const Moo::Colour& colour,
		bool alwaysDraw = true );

	static void	drawLineTwoColour( const Vector3& start, 
		const Vector3& end, 
		const Moo::Colour& colourStart,
		const Moo::Colour& colourEnd,
		bool alwaysDraw = true );

	static void drawAxes( const Matrix & worldTransform, float size = 1.0f,
		Moo::PackedColour xAxisStartColour = Moo::PackedColours::RED,
		Moo::PackedColour yAxisStartColour = Moo::PackedColours::GREEN,
		Moo::PackedColour zAxisStartColour = Moo::PackedColours::BLUE,
		Moo::PackedColour xAxisEndColour = Moo::PackedColours::RED,
		Moo::PackedColour yAxisEndColour = Moo::PackedColours::GREEN,
		Moo::PackedColour zAxisEndColour = Moo::PackedColours::BLUE,
		bool alwaysDraw = true );

	static void drawAxesCentreColoured(
		const Matrix & worldTransform, float size = 1.0f,
		Moo::PackedColour centreColour = Moo::PackedColours::WHITE,
		Moo::PackedColour xAxisEndColour = Moo::PackedColours::RED,
		Moo::PackedColour yAxisEndColour = Moo::PackedColours::GREEN,
		Moo::PackedColour zAxisEndColour = Moo::PackedColours::BLUE,
		bool alwaysDraw = true );

	static void drawStar( const Vector3 & centre, float radius = 1.0f,
		Moo::PackedColour centreColour = Moo::PackedColours::MAGENTA,
		Moo::PackedColour edgeColour = Moo::PackedColours::MAGENTA,
		bool alwaysDraw = true );

	//Always use TexturedWorldLine by enclosing many lines in a begin/end block
	static bool beginTexturedWorldLines();
	static void texturedWorldLine( const Vector3 & start,
		const Vector3 & end,
		float thickness,
		const Moo::PackedColour & colour,
		float textureRepeat );
	static void texturedWorldLine( const Vector3 & start,
		const Vector3 & end,
		float thickness,
		const Moo::PackedColour & colour,
		float uvParametricStep,
		float uvStart );
	static void endTexturedWorldLines( const Matrix& world = Matrix::identity );

	//Always use texturedWorldLoftSegment by enclosing many lines in a begin/end block
	static bool beginLoft( uint32 nSegments = 0);
	static void texturedWorldLoftSegment(
		const Vector3& shapeRay,
		const Vector3& pathPos,
		float uv,
		const Moo::PackedColour & col );
	static void endLoft( const Matrix& world );

	static void texturedLine( const Vector3& start,
			const Vector3& end,
			float clipThickness,
			const Moo::PackedColour& colour,
			float textureRepeat = -1.f,
			bool useEffect=false );

	static void drawLinesInClip( Vector2 * points, uint count,
		const Moo::Colour& colour = Moo::Colour( 1, 0, 0, 0 ) );

	static void drawLinesInWorld( Vector3 * points, uint count,
		const Moo::Colour& colour = Moo::Colour( 1, 0, 0, 0 ) );

	static void drawLinesInWorld( Vector2 * points, uint count,
		const Moo::Colour& colour = Moo::Colour( 1, 0, 0, 0 ) );

	static void wireBox( const BoundingBox& bb,
			const Moo::Colour& colour, bool drawAlways = false );

	static void texturedWireBox( const BoundingBox& bb,
			const Moo::Colour& colour, float offset, float tile,
			bool drawAlways = false, bool setStates = true );

	static void rgbBox( const BoundingBox& bb );


	//sphere methods not static because the sphere object
	//has state
	void drawSphere( const Vector3 & center,
			float radius,
			const Moo::Colour& colour );

	void wireSphere( const Vector3 & center,
			float radius,
			const Moo::Colour& colour );

	void drawCylinder( const Vector3 & start,
			const Vector3 & end,
			float radius,
			const Moo::Colour& colour );

	void wireCylinder( const Vector3 & start,
			const Vector3 & end,
			float radius,
			const Moo::Colour& colour );

public:

	static void addWireBoxWorld( const BoundingBox & bb, 
		const Moo::Colour & colour, float offset = 0.0f, float tile = 0.0f,
		bool drawAlways = false, bool setStates = true );

	static void addLineWorld( const Vector3& start, const Vector3& end, 
		const Moo::Colour& colour, bool alwaysDraw = true );

	static void flushDrawItems();


private:
	Geometrics(const Geometrics&);
	Geometrics& operator=(const Geometrics&);

	class Sphere
	{
	public:
		Sphere() :
			inited_( false ),
			nVerts_(),
			nIndices_(),
			nRows_(),
			nPts_(),
			verts_( NULL ),
			indices_( NULL )
		{
		}

		~Sphere()
		{
			bw_safe_delete_array(verts_);
			bw_safe_delete_array(indices_);
		}

		void draw( const Vector3 & position,
			float radius,
			const Moo::Colour & colour,
			bool wire = false );


		Moo::EffectMaterialPtr material_;
	private:
		void	create( bool wire );
		void	createWireIndices();
		void	createSolidIndices();

		bool	inited_;
		int		nVerts_;
		int		nIndices_;
		int		nRows_;
		int		nPts_;
		Moo::VertexXYZL * verts_;
		uint16 *indices_;
	};

	Sphere		sphere_;
	Sphere		wireSphere_;


	class Cylinder
	{
	public:
		Cylinder()
			:inited_( false ),
			 verts_( NULL ),
			 indices_( NULL ),
			 nVerts_(0),
			 nIndices_(0),
			 nRows_(0),
			 nPoints_(0),
			 circleStripLength_(0),
			 wallStripLength_(0)
		{
		}

		~Cylinder()
		{
			bw_safe_delete_array(verts_);
			bw_safe_delete_array(indices_);
		}

		void	create( bool wire );
		void	createWireIndices();
		void	createSolidIndices();
		void	draw( const Vector3 & start,
					const Vector3 & end,
					float radius,
					const Moo::Colour& colour = 0xFFFF0000,
					bool wire = false );

		Moo::EffectMaterialPtr material_;
	private:
		bool	inited_;
		int		nVerts_;
		int		nIndices_;
		int		nRows_;
		int		nPoints_;
		int		circleStripLength_;
		int		wallStripLength_;
		Moo::VertexXYZL * verts_;
		uint16 *indices_;
	};

	Cylinder		cylinder_;
	Cylinder		wireCylinder_;


	friend std::ostream& operator<<(std::ostream&, const Geometrics&);
};

#ifdef CODE_INLINE
#include "geometrics.ipp"
#endif

BW_END_NAMESPACE

#endif
/*geometrics.hpp*/
