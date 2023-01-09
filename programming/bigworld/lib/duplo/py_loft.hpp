#ifndef LOFT_PARTICLE_RENDERER_HPP
#define LOFT_PARTICLE_RENDERER_HPP

#include "py_attachment.hpp"
#include "moo/material.hpp"
#include "pyscript/script_math.hpp"
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ControlPoint
// -----------------------------------------------------------------------------
class ControlPoint
{
public:
	ControlPoint( float age, const Vector3& startPos, const Vector3& endPos ):
		age_(age),
		st_(startPos),
		en_(endPos)
	{
	};

	ControlPoint():
		age_(-1.f),
		st_(Vector3::zero()),
		en_(Vector3::zero())
	{
	};

	float age_;
	Vector3 st_;
	Vector3 en_;
};


// -----------------------------------------------------------------------------
// Section: Path (interface)
// -----------------------------------------------------------------------------
class Path
{
public:
	virtual void age( float dTime, float maxAge ) = 0;
	virtual void add( const ControlPoint& cp ) = 0;
	virtual size_t size() const = 0;
	virtual bool begin(ControlPoint& ret) = 0;
	virtual bool next(ControlPoint& ret) = 0;
};


// -----------------------------------------------------------------------------
// Section: SimplePath
// -----------------------------------------------------------------------------
/**
 *	This class simply records the incoming control points, and exposes them for
 *	iteration.  It performs no interpolation or smoothing, and thus performs the
 *	fastest out of all the paths, but may no look as good.
 */
class SimplePath : public Path
{
public:
	virtual void age( float dTime, float maxAge );
	virtual void add( const ControlPoint& cp );
	virtual size_t size() const;	
	virtual bool begin(ControlPoint& ret);
	virtual bool next(ControlPoint& ret);
private:
	typedef BW::list< ControlPoint >	History;
	History	history_;
	History::iterator it_;
};


// -----------------------------------------------------------------------------
// Section: InterpolatedPath
// -----------------------------------------------------------------------------
/**
 * This class takes a series of control points, and rejects incoming points if
 * they are spaced closer than threshold_ metres.
 *
 * If a control point comes in that is more than threshold_ metres away, it
 * linearly interpolates the path such that each path segment is threshold_
 * metres from each other.
 *
 * It uses temporary_ control point to signify the current actual end point of
 * the path.  If the end point has not yet reached the threshold (and has thus
 * not yet stored a control point for it), the temporary end point is still used
 * for iteration.  This prevents any gap occurring between the souce point and
 * the path.
 */
class InterpolatedPath : public Path
{
public:
	InterpolatedPath();
	void age( float dTime, float maxAge );
	void add( const ControlPoint& cp );
	virtual size_t size() const;
	bool begin(ControlPoint& ret);
	bool next(ControlPoint& ret);
	void threshold( float t )	{ threshold_ = t; }
	float threshold() const	{ return threshold_; }
protected:
	virtual void addPoint( const ControlPoint& cp );
	typedef BW::list< ControlPoint >	History;
	History	history_;
	float threshold_;
private:		
	ControlPoint temporary_;
	History::iterator it_;	
};


// -----------------------------------------------------------------------------
// Section: SmoothedPath
// -----------------------------------------------------------------------------
/**
 * This class takes an interpolated series of control points, and every time a
 * new point is added, it smooths the third-most-recent-point using a
 * Catmull-Rom spline.
 *
 * This will smooth out most paths, however if the frame-rate is poor, and the
 * interpolation is relatively high, what you will see is a fairly angular path,
 * with smoothed corners.
 */
class SmoothedPath : public InterpolatedPath
{
private:
	void addPoint( const ControlPoint& basis5 );
};


// -----------------------------------------------------------------------------
// Section: ProgressiveRefinementPath
// -----------------------------------------------------------------------------
/**
 * This class takes a series of control points, and every frame scans the entire
 * list finds any two points that are further apart than the supplied threshold,
 * and inserts a new refined point using a Catmull-Rom spline.
 * Each frame it tessellates only once, making it a progressive refinement.
 */
class ProgressiveRefinementPath : public Path
{
public:
	ProgressiveRefinementPath();
	void age( float dTime, float maxAge );
	void add( const ControlPoint& cp );
	virtual size_t size() const;
	bool begin(ControlPoint& ret);
	bool next(ControlPoint& ret);
	void threshold( float t )	{ threshold_ = t; }
	float threshold() const	{ return threshold_; }
private:
	typedef BW::list< ControlPoint >	History;
	History	history_;
	void smooth();
	bool refine( History::iterator& cp0,
				History::iterator& cp1,
				History::iterator& cp2,
				History::iterator& cp3 );
	float threshold_;		
	ControlPoint temporary_;
	History::iterator it_;	
};


// -----------------------------------------------------------------------------
// Section: PyLoft.
// -----------------------------------------------------------------------------
/*~ class BigWorld.PyLoft
 *
 * The PyLoft is a PyAttachment.  It lofts a shape along a path, drawing a
 * single texture over the resultant mesh with no repeats.
 *
 * Shape : A line between the particle where it is first created, and the
 * position of a MatrixProvider.
 *
 * Path : The line between each particle in turn, in order from youngest to
 * oldest.
 *
 * The path is progressively refined down to a threshold level, meaning that
 * the longest distance between points is no more than threshold metres. 
 *
 *	Example:
 *	@{
 *	loft  = BigWorld.Loft( "optional_texture_name" )
 *	m = BigWorld.player().model
 *	m.node( "biped Head" ).attach( loft )
 *	loft.endPoint = m.node( "biped R Clavicle" )
 *	@}
 */
/**
 * The PyLoft is a PyAttachment.  It lofts a shape along a path, drawing a
 * single texture over the resultant mesh with no repeats.
 *
 * Shape : A line between the particle where it is first created, and the
 * position of a MatrixProvider.
 *
 * Path : The line between each particle in turn, in order from youngest to
 * oldest.
 *
 * The path is progressively refined down to a threshold level, meaning that
 * the longest distance between points is no more than threshold metres. 
 */
class PyLoft : public PyAttachment
{
	Py_Header( PyLoft, PyAttachment )
public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyLoft( PyTypeObject *pType = &s_type_ );
	~PyLoft();
	//@}

	/// @name PyLoft methods.
	//@{
	void textureName( const BW::string& v );
	const BW::string& textureName() const	{ return textureName_; }	

	void maxAge( float a )	{ maxAge_ = a; }
	float maxAge() const	{ return maxAge_; }

	void threshold( float t )	{ path4_.threshold(t); }
	float threshold() const	{ return path4_.threshold(); }

	void endPoint( MatrixProviderPtr ep )	{ endPoint_ = ep; }
	MatrixProviderPtr endPoint( void )	{ return endPoint_; }

	void realDraw( const Matrix & worldTransform );	
	//@}


	///	@name PyAttachment Overrides.
	//@{
	void tick( float dTime );

	virtual bool needsUpdate() const { return false; }
	virtual bool isLodVisible() const { return true; }
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod ) { /*do nothing*/ }

	void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );
	void localBoundingBox( BoundingBox & bb, bool skinny = false );
	void localVisibilityBox( BoundingBox & vbb, bool skinny = false );
	void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false );
	void worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny = false );
	//@}

	///	@name Python Interface to the PyLoft.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, textureName, textureName )	
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxAge, maxAge )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( MatrixProviderPtr, endPoint, endPoint )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, threshold, threshold )	
	//@}

protected:
	virtual void serialiseInternal(DataSectionPtr pSect, bool load);

private:
	void recordPosition( const Matrix & worldTransform );	

	Moo::Material		material_;
	BW::string			textureName_;
	MatrixProviderPtr	endPoint_;
	float				maxAge_;	
	Path*				path_;
	//SimplePath		path1_;
	//InterpolatedPath	path2_;
	//SmoothedPath		path3_;
	ProgressiveRefinementPath path4_;
	//We always store the bb_ in world coordinates.
	BoundingBox			bb_;
};

BW_END_NAMESPACE

#endif // PY_LOFT
