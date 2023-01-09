#ifndef ENTITY_NAVIGATE_HPP
#define ENTITY_NAVIGATE_HPP

#include "entity.hpp"
#include "entity_extra.hpp"
#include "navmesh_navigation_system.hpp"

#include "chunk/base_chunk_space.hpp"

BW_BEGIN_NAMESPACE

class NavigateStepController;


/**
 *	This class is an entity extra that supports navigation with a navmesh.
 */
class EntityNavigate : public EntityExtra
{
	Py_EntityExtraHeader( EntityNavigate )

public:
	EntityNavigate( Entity & e );
	~EntityNavigate();

	void registerNavigateStepController( NavigateStepController* pController );
	void deregisterNavigateStepController( NavigateStepController* pController );

	PY_AUTO_METHOD_DECLARE( RETOWN, moveToPoint,
		ARG( Vector3, ARG( float, OPTARG( int, 0,
		OPTARG( bool, true, OPTARG( bool, false, END ) ) ) ) ) )
	PyObject * moveToPoint( Vector3 destination,
							float velocity,
							int userArg = 0,
							bool faceMovement = true,
							bool moveVertically = false );

	PY_AUTO_METHOD_DECLARE( RETOWN, moveToEntity,
		ARG( int, ARG( float, ARG( float, OPTARG( int, 0,
		OPTARG( bool, true, OPTARG( bool, false, END ) ) ) ) ) ) )
	PyObject * moveToEntity( int destEntityID,
								float velocity,
								float range,
								int userArg = 0,
								bool faceMovement = true,
								bool moveVertically = false );

	PY_AUTO_METHOD_DECLARE( RETOWN, accelerateToPoint,
		ARG( Position3D,
		ARG( float,
		ARG( float,
		OPTARG( int, 1,
		OPTARG( bool, false,
		OPTARG( int, 0, END ) ) ) ) ) ) );
	PyObject * accelerateToPoint(	Position3D destination,
									float acceleration,
									float maxSpeed,
									int facing = 1,
									bool stopAtDestination = true,
									int userArg = 0);

	PY_AUTO_METHOD_DECLARE( RETOWN, accelerateAlongPath,
		ARG( BW::vector<Position3D>,
		ARG( float,
		ARG( float,
		OPTARG( int, 1,
		OPTARG( int, 0, END ) ) ) ) ) );
	PyObject * accelerateAlongPath(	BW::vector<Position3D> waypoints,
									float acceleration,
									float maxSpeed,
									int facing = 1,
									int userArg = 0);

	PY_AUTO_METHOD_DECLARE( RETOWN, accelerateToEntity,
		ARG( EntityID,
		ARG( float,
		ARG( float,
		ARG( float,
		OPTARG( int, 1,
		OPTARG( int, 0, END ) ) ) ) ) ) );
	PyObject * accelerateToEntity(	EntityID destinationEntity,
									float acceleration,
									float maxSpeed,
									float range,
									int facing = 1,
									int userArg = 0);


	PY_AUTO_METHOD_DECLARE( RETOWN, navigate,
		ARG( Vector3, ARG( float,
		OPTARG( bool, true, OPTARG( float, -1.f, OPTARG( float, 0.5f, 
		OPTARG( float, 0.01f, OPTARG(int, 0, END ) ) ) ) ) ) ) )
	PyObject * navigate( const Vector3 & dstPosition, float velocity,
		bool faceMovement = true, float maxDistance = -1.f, float girth = 0.5f, float closeEnough = 0.01f, int userArg = 0 );

	PY_AUTO_METHOD_DECLARE( RETOWN, navigateStep,
		ARG( Vector3, ARG( float, ARG( float, OPTARG( float, -1.f,
		OPTARG( bool, true, OPTARG( float, 0.5f, OPTARG( int, 0, END ) ) ) ) ) ) ) )
	PyObject * navigateStep( const Vector3 & dstPosition, float velocity,
		float maxMoveDistance, float maxSearchDistance = -1.f, bool faceMovement = true,
		float girth = 0.5f, int userArg = 0 );


	PY_AUTO_METHOD_DECLARE( RETOWN, navigateFollow,
			ARG( PyObjectPtr, ARG( float, ARG( float,
			ARG( float, ARG( float, OPTARG( float, -1.f, OPTARG( bool, true,
			OPTARG ( float, 0.5f, OPTARG( int, 0, END ) ) ) ) ) ) ) ) ) )
	PyObject * navigateFollow( PyObjectPtr pEntity, float angle, float offset,
		float velocity, float maxMoveDistance, float maxSearchDistance = -1.f, 
		bool faceMovement = true, float girth = 0.5f, int userArg = 0 );


	PY_AUTO_METHOD_DECLARE( RETOWN, canNavigateTo,
		ARG( Vector3, OPTARG( float, -1.f, OPTARG( float, 0.5f, END ) ) ) )
	PyObject * canNavigateTo( const Vector3 & dstPosition,
		float maxDistance = -1.f, float girth = 0.5f );

	PY_AUTO_METHOD_DECLARE( RETDATA, waySetPathLength, END )
	int waySetPathLength();

	PY_AUTO_METHOD_DECLARE( RETOWN, getStopPoint,
		ARG( Vector3, ARG( bool, OPTARG( float, -1.f, OPTARG( float, 0.5f,
		OPTARG( float, 1.5f, OPTARG( float, 1.8f, END ) ) ) ) ) ) )
	PyObject * getStopPoint( const Vector3 & dstPosition,
			bool ignoreFirstStopPoint, float maxDistance = -1.f,
			float girth = 0.5f, float stopDistance = 1.5f, 
			float nearPortalDistance = 1.8f );

	PY_AUTO_METHOD_DECLARE( RETOWN, navigatePathPoints, 
		ARG( Vector3, 
		OPTARG( float, -1.f, OPTARG( float, 0.5f, END ) ) ) )
	PyObject * navigatePathPoints( const Vector3 & dstPosition, 
		float maxSearchDistance = -1.f, float girth = 0.5 );

	static const Instance< EntityNavigate > instance;

	NavmeshEntityNavigationSystem & navigationSystem();

private:
	NavmeshEntityNavigationSystem * pNavigationSystem_;

	NavigateStepController * pNavigateStepController_;
};


BW_END_NAMESPACE

#endif // ENTITY_NAVIGATE_HPP
