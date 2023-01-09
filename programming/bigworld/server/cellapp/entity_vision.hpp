#ifndef ENTITY_VISION_HPP
#define ENTITY_VISION_HPP

#include "entity.hpp"
#include "entity_extra.hpp"

BW_BEGIN_NAMESPACE

class VisibilityController;
class VisionController;


/**
 *	This class has extra entity stuff for the implementation of vision
 *	on the server
 */
class EntityVision : public EntityExtra
{
	Py_EntityExtraHeader( EntityVision )

public:
	EntityVision( Entity & e );
	~EntityVision();

	// Attributes pertaining to ghosts
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, seeingHeight, seeingHeight );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, visibleHeight, visibleHeight );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, canBeSeen, canBeSeen );

	// Attributes pertaining to reals
	PY_RW_ATTRIBUTE_DECLARE( shouldDropVision_, shouldDropVision );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, fieldOfView, fieldOfView );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, visionAngle, visionAngle );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, visionRange, visionRange );


	PY_AUTO_METHOD_DECLARE( RETOK, setVisionRange,
					ARG( float, ARG( float, END ) ) )
	bool setVisionRange( float visionAngle, float range );


	PY_AUTO_METHOD_DECLARE( RETOWN, addVision,
			ARG( float, ARG( float, ARG( float,
					OPTARG( int, 10, OPTARG( int, 0,  END ) ) ) ) ) )
	PyObject* addVision( float fov, float range, float seeingHeight,
					int period = 10, int userArg = 0 );

	PY_AUTO_METHOD_DECLARE( RETOWN, addScanVision,
			ARG( float,	ARG( float, ARG( float,
			ARG( float, ARG( float,	ARG( float,
			OPTARG( int, 1, OPTARG( int, 0,  END )))))))) )
	PyObject* addScanVision( float fov, float range, float seeingHeight,
		float amplitude, float scanPeriod, float timeOffset,
		int updatePeriod = 1, int userArg = 0 );

	PY_AUTO_METHOD_DECLARE( RETOWN, entitiesInView, END )
	PyObject* entitiesInView() const;

	const EntitySet & updateVisibleEntities( float seeingHeight,
			float yawOffset );

	void triggerVisionEnter( Entity * who );
	void triggerVisionLeave( Entity * who );

	VisionController * getVision() const		{ return vision_; }
	void setVision( VisionController * vc );

	VisibilityController * getVisibility() const	{ return visibility_; }
	void setVisibility( VisibilityController * vc );

	//Height from others can look at you
	float visibleHeight() const;
	void visibleHeight( float h );

	bool canBeSeen() const;
	void canBeSeen( bool v );

	//Height from which you look from
	float seeingHeight() const;
	void seeingHeight( float h );

	static bool canBeSeen( Entity * pEntity );

	float fieldOfView() const;
	void fieldOfView( float v );

	float visionAngle() const;
	void visionAngle( float v );

	float visionRange() const;
	void visionRange( float v );

	const EntitySet& entitiesInVisionRange() const
										{ return entitiesInVisionRange_; }
	const EntitySet& visibleEntities() const	{ return visibleEntities_; }
#ifdef DEBUG_VISION
	EntitySet& seenByEntities()	{ return seenByEntities_; }
#endif

	Position3D getDroppedPosition();

	void setVisibleEntities( const BW::vector< EntityID > & visibleEntities );

	static const Instance<EntityVision> instance;

private:
	void callback( const char * methodName, Entity * pEntity ) const;

	bool isNeeded() const { return visibility_ || vision_; }

	Position3D		lastDropPosition_;

	EntitySet entitiesInVisionRange_;
	EntitySet visibleEntities_;
	//ControllerID visionController_;

	VisibilityController * visibility_;
	VisionController * vision_;
	bool	shouldDropVision_;
	bool	iterating_;
	bool	iterationCancelled_;

#ifdef DEBUG_VISION
	EntitySet seenByEntities_;
#endif
};

BW_END_NAMESPACE

#endif // ENTITY_VISION_HPP

