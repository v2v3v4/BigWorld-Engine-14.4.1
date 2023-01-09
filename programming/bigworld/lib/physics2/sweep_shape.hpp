#ifndef SWEEP_SHAPE_HPP
#define SWEEP_SHAPE_HPP

#include "collision_callback.hpp"

#include "math/vector3.hpp"
#include "math/boundbox.hpp"
#include "worldtri.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This template class is a shape that can be swept through a chunk space and
 *	collided with the obstacles therein.
 *
 *	The shape may be no bigger than BaseChunkSpace::gridSize() in any dimension.
 */
template <class X> class SweepShape
{
	//	const Vector3 & leader() const;
	//	void boundingBox( BoundingBox & bb ) const;
	//	void transform( X & shape, Vector3 & extent,
	//		const Matrix & transformer,
	//		float sdist, float edist, const Vector3 & dir,
	//		const Vector3 & bbCenterAtSDistTransformed,
	//		const Vector3 & bbCenterAtEDistTransformed ) const;
	//	inline static const float transformRangeToRadius( const Matrix & trInv,
	//		const Vector3 & shapeRange )
};


/**
 *	This is the simplest instantiation of the SweepShape template,
 *	for sweeping a single point through the space (making a ray)
 */
template <>
class SweepShape<Vector3>
{
public:
	SweepShape( const Vector3 & pt ) : pt_( pt ) {}

	const Vector3 & leader() const
	{
		return pt_;
	}

	void boundingBox( BoundingBox & bb ) const
	{
		bb.setBounds( pt_, pt_ );
	}

	void transform( Vector3 & shape, Vector3 & end,
		const Matrix &, float, float, const Vector3 &,
		const Vector3 & bbCenterAtSDistTransformed,
		const Vector3 & bbCenterAtEDistTransformed ) const
	{
		shape = bbCenterAtSDistTransformed;
		end = bbCenterAtEDistTransformed;
	}

	inline static float transformRangeToRadius( const Matrix & /*trInv*/,
		const Vector3 & /*shapeRange*/ )
	{
		return 0.f;
	}

private:
	Vector3 pt_;
};


/**
 *	This is the instantiation of the SweepShape template for a WorldTriangle,
 *	which swept through the chunk space makes a prism.
 */
template <> class SweepShape<WorldTriangle>
{
public:
	SweepShape( const WorldTriangle & wt ) : wt_( wt ) {}

	const Vector3 & leader() const
	{
		return wt_.v0();
	}

	void boundingBox( BoundingBox & bb ) const
	{
		bb.setBounds( wt_.v0(), wt_.v0() );
		bb.addBounds( wt_.v1() );
		bb.addBounds( wt_.v2() );
	}

	void transform( WorldTriangle & shape, Vector3 & end,
		const Matrix & tr, float sdist, float edist, const Vector3 & dir,
		const Vector3&, const Vector3& ) const
	{
		Vector3 off = dir * sdist;
		shape = WorldTriangle(
			tr.applyPoint( wt_.v0() + off ),
			tr.applyPoint( wt_.v1() + off ),
			tr.applyPoint( wt_.v2() + off ) );
		end = tr.applyPoint( wt_.v0() + dir * edist );
	}

	inline static float transformRangeToRadius( const Matrix & trInv,
		const Vector3 & shapeRange )
	{
		const Vector3 clipRange = trInv.applyVector( shapeRange );
		return clipRange.length() * 0.5f;
	}

private:
	WorldTriangle wt_;
};


class SweepParams
{
public:
	template<typename ShapeType>
	SweepParams( const SweepShape<ShapeType> & start,
			const Vector3 &	end );

	const AABB & shapeBox() const { return shapeBox_; }
	const Vector3 & shapeRange() const { return shapeRange_; }
	float shapeRadius() const { return shapeRad_; }

	const Vector3 & bsource() const { return bsource_; }
	const Vector3 & bextent() const { return bextent_; }

	const Vector3 & csource() const { return csource_; }
	const Vector3 & cextent() const { return cextent_; }

	const Vector3 & direction() const { return dir_; }
	float fullDistance() const { return fullDist_; }

private:
	BoundingBox shapeBox_;
	Vector3 shapeRange_;
	float shapeRad_;

	Vector3 bsource_, bextent_;
	Vector3 csource_, cextent_;

	Vector3 dir_;
	float fullDist_;
};

///////////

template<typename ShapeType>
SweepParams::SweepParams( 
	const SweepShape<ShapeType> & start,
	const Vector3 &	end )
{
	// find the min and max of X and Z
	start.boundingBox( shapeBox_ );
	
	shapeRange_ = shapeBox_.maxBounds() - shapeBox_.minBounds();
	shapeRad_ = shapeRange_.length() * 0.5f;
	dir_ = end - start.leader();

	// find our real source and extent
	bsource_ = shapeBox_.minBounds();
	bextent_ = bsource_ + dir_;

	// get the source point moved to the centre of the bb
	// (for hulltree rounded cylinder collisions)
	csource_ = bsource_ + shapeRange_ * 0.5f;
	cextent_ = csource_ + dir_;

	const Vector3 destMin = shapeBox_.minBounds() + dir_;
	const Vector3 destMax = shapeBox_.maxBounds() + dir_;

	shapeBox_.addBounds( destMin );
	shapeBox_.addBounds( destMax );

	fullDist_ = dir_.length();
	dir_ /= fullDist_;
}

BW_END_NAMESPACE


#endif // SWEEP_SHAPE_HPP
