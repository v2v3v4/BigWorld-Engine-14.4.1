#ifndef PASSENGERS_HPP
#define PASSENGERS_HPP

#include "entity_extra.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class manages the passengers that are riding on this entity. It extends
 *	entities that are vehicles, moving platforms, etc.
 */
class Passengers : public EntityExtra
{
public:
	Passengers( Entity & e );
	~Passengers();

	bool add( Entity & entity );
	bool remove( Entity & entity );

	void updateInternalsForNewPosition( const Vector3 & oldPosition );

	const Matrix & transform() const	{ return transform_; }

	//input global pos/dir, output local pos/dir on the vehicle space
	void transformToVehiclePos( Position3D& pos );
	void transformToVehicleDir( Direction3D& dir );

	static const Instance<Passengers> instance;

private:
	Passengers( const Passengers& );
	Passengers& operator=( const Passengers& );

	void adjustTransform();

	typedef BW::vector< Entity * > Container;

	bool inDestructor_;

	Container passengers_;
	Matrix transform_;
};

BW_END_NAMESPACE


#endif // PASSENGERS_HPP
