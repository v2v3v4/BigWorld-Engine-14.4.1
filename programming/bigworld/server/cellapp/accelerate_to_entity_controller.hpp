#ifndef ACCELERATE_TO_ENTITY_CONTROLLER_HPP
#define ACCELERATE_TO_ENTITY_CONTROLLER_HPP


#include "base_acceleration_controller.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This class forms a controller smoothly moves an entity towards a given
 *	entity. It will stop when it gets within a certain range of the entity.
 */
class AccelerateToEntityController : public BaseAccelerationController
{
	DECLARE_CONTROLLER_TYPE( AccelerateToEntityController )

public:
	AccelerateToEntityController(	EntityID destEntityID = 0,
									float acceleration = 0.0f,
									float maxSpeed = 0.0f,
									float range = 0.0f,
									Facing facing = FACING_NONE );

	virtual void		stopReal( bool isFinalStop );
	virtual void		writeRealToStream( BinaryOStream & stream );
	virtual bool		readRealFromStream( BinaryIStream & stream );
	void				update();

private:

	void				recalcOffset();

	EntityID			destEntityID_;
	EntityPtr			pDestEntity_;
	Position3D			destination_;
	Position3D			offset_;
	float				range_;
};

BW_END_NAMESPACE

#endif // ACCELERATE_TO_ENTITY_CONTROLLER_HPP
