#ifndef BOUNCE_GRENADE_EXTRA_HPP
#define BOUNCE_GRENADE_EXTRA_HPP

#include "cellapp/entity_extra.hpp"

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

class BounceGrenadeExtra : public EntityExtra
{
	Py_EntityExtraHeader( BounceGrenadeExtra );

public:
	BounceGrenadeExtra( Entity & e );

	~BounceGrenadeExtra();

	PY_AUTO_METHOD_DECLARE( RETOWN,
			bounceGrenade, ARG( Vector3, ARG( Vector3, ARG( float, ARG( float,
					ARG( float, ARG( int, OPTARG( int, -1, END ) ) ) ) ) ) ) )
	PyObject * bounceGrenade( const Vector3 & sourcePos,
								const Vector3 & velocity, float elasticity,
								float radius, float timeSample,
								int maxSamples, int maxBounces ) const;

	static const Instance< BounceGrenadeExtra > instance;
};

BW_END_NAMESPACE

#endif
