#ifndef NAV_CONTROLLER_HPP
#define NAV_CONTROLLER_HPP

#include "controller.hpp"
#include "network/basictypes.hpp"
#include "server/updatable.hpp"
#include "waypoint/navigator.hpp"
#include "waypoint/navloc.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 * This controller moves an entity to the destination point along the
 * navigation mesh.
 */
class NavigationController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( NavigationController )
public:
	NavigationController( const Position3D & destination = Position3D( 0, 0, 0 ),
		float velocity = 0.f, bool faceMovement = true,
		float maxDistance = -1.f, float girth = 0.5f,
		float closeEnough = 0.01f );

protected:

	enum NavigationStatus
	{
		NAVIGATION_CANCELLED = -2,
		NAVIGATION_FAILED = -1,
		NAVIGATION_IN_PROGRESS = 0,
		NAVIGATION_COMPLETE = 1
	};

	virtual void	startReal( bool isInitialStart );
	virtual void	stopReal( bool isFinalStop );

	NavigationStatus move();
	void			writeRealToStream( BinaryOStream & stream );
	bool 			readRealFromStream( BinaryIStream & stream );
	void			update();

private:
	void generateTraversalPath( const NavLoc & srcLoc );

	float 		metresPerTick_;
	float 		maxDistance_;
	float 		girth_;
	float 		closeEnough_;
	bool  		faceMovement_;
	Position3D 	nextPosition_;
	Position3D 	destination_;
	NavLoc * 	pDstLoc_;

	Vector3Path path_;
	int 		currentNode_;
};

BW_END_NAMESPACE

#endif //NAV_CONTROLLER_HPP
