#ifndef BOUNDBOX_HPP
#define BOUNDBOX_HPP

#include "mathdef.hpp"
#include "vector3.hpp"
#include "matrix.hpp"
#include "cstdmf/debug.hpp"

#include <float.h>

BW_BEGIN_NAMESPACE

/**
 *	BoundingBox, implementation of an axis aligned boundingbox
 */

 class AABB
 {
 public:
	AABB();
	AABB( const Vector3 & min, const Vector3 & max );
	
	bool operator==( const AABB& bb ) const;
	bool operator!=( const AABB& bb ) const;

	const Vector3 & minBounds() const;
	const Vector3 & maxBounds() const;
	void setBounds( const Vector3 & min, const Vector3 & max );

    float width() const;
    float height() const;
    float depth() const;

	void addYBounds( float y );
	void addBounds( const Vector3 & v );
	void addBounds( const AABB & bb );
    void expandSymmetrically( float dx, float dy, float dz );
    void expandSymmetrically( const Vector3 & v );

	void transformBy( const Matrix & transform );

	bool intersects( const AABB & box ) const;
	bool intersects( const Vector3 & v ) const;
	bool intersects( const Vector3 & v, float bias ) const;
	bool intersectsRay( const Vector3 & origin, const Vector3 & dir ) const;
	bool intersectsLine( const Vector3 & origin, const Vector3 & dest ) const;

	void calculateOutcode( const Matrix & m,
		Outcode &outCode, Outcode& combinedOutcode ) const;

	enum RayIntersectionType
	{
		NO_INTERSECTION,
		ORIGIN_INSIDE,
		INTERSECTION
	};

	RayIntersectionType intersectsRay( const Vector3 & origin, 
		const Vector3 & dir, Vector3 & coord ) const;

	bool clip( Vector3 & start, Vector3 & extent, float bloat = 0.f ) const;

	float distance( const Vector3& point ) const;

	INLINE Vector3 centre() const;

	INLINE bool insideOut() const;

 protected:
	Vector3 min_;
	Vector3 max_;
 };

class BoundingBox : public AABB
{
public:
	BoundingBox();
	explicit BoundingBox( const AABB& other );
	BoundingBox( const Vector3 & min, const Vector3 & max );

	bool operator==( const BoundingBox& bb ) const;
	bool operator!=( const BoundingBox& bb ) const;

	void calculateOutcode( const Matrix & m ) const;

	Outcode outcode() const;
	Outcode combinedOutcode() const;
	void outcode( Outcode oc );
	void combinedOutcode( Outcode oc );

private:
	mutable Outcode oc_;
	mutable Outcode combinedOc_;

public:
	static const BoundingBox s_insideOut_;
};

#ifdef CODE_INLINE
#include "boundbox.ipp"
#endif

BW_END_NAMESPACE


#endif //BOUNDBOX.HPP
/*BoundBox.hpp*/
