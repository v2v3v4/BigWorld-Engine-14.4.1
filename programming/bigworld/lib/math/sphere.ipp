
BW_BEGIN_NAMESPACE

inline const Vector3& Sphere::center() const
{
	return center_;
}

inline void Sphere::center( const Vector3& newCenter )
{
	center_ = newCenter;
}

inline float Sphere::radius() const
{
	return radius_;
}

inline void Sphere::radius( float newRadius )
{
	radius_ = newRadius;
}

BW_END_NAMESPACE