#ifndef MOVE_CONTROLLER_HPP
#define MOVE_CONTROLLER_HPP

#include "controller.hpp"
#include "network/basictypes.hpp"
#include "server/updatable.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer< Entity > EntityPtr;

/**
 *	Abstract base class from which other controllers are derived.
 *	It just knows how to move towards a given destination by a given
 *	velocity in metres per second.
 */
class MoveController : public Controller, public Updatable
{
public:
	MoveController( float velocity, bool faceMovement = true,
		bool moveVertically = false );

	virtual void		startReal( bool isInitialStart );
	virtual void		stopReal( bool isFinalStop );

	bool				move( const Position3D & destination,
							float* pRemainingTicks = NULL );

	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );

protected:
	bool faceMovement() const				{ return faceMovement_; }
	bool moveVertically() const				{ return moveVertically_; }

	virtual void setEntityPosition( const Position3D & position,
								const Direction3D & direction );

private:
	float				metresPerTick_;
	bool faceMovement_;
	bool moveVertically_;
};



/**
 *	This controller moves an entity towards a given point. It registers itself
 *	to be called every game tick. The velocity is measured in metres per second.
 */
class MoveToPointController : public MoveController
{
	DECLARE_CONTROLLER_TYPE( MoveToPointController )

public:
	MoveToPointController(const Position3D & destination = Position3D(0, 0, 0),
		const BW::string & destinationChunk = "",
		int32 destinationWaypoint = 0,
		float velocity = 0.0f,
		bool faceMovement = true,
		bool moveVertically = false );

	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );
	void				update();

private:
	Position3D			destination_;
	BW::string			destinationChunk_;
	int32				destinationWaypoint_;
};


/**
 *	This controller is used by navigateStep to move an entity towards a given
 *	point. It registers itself to be called every game tick. The velocity is
 *	measured in metres per second.
 *	The controller will register itself to the Navigator of the entity.
 */
class NavigateStepController : public MoveController
{
	DECLARE_CONTROLLER_TYPE( NavigateStepController )

public:
	NavigateStepController();
	NavigateStepController( const Position3D & destination,
		float velocity, bool faceMovement );

	void				startReal( bool isInitialStart );
	void				stopReal( bool isFinalStop );

	void				setDestination( const Position3D& destination );

	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );
	void				update();

protected:
	virtual void setEntityPosition( const Position3D & position,
								const Direction3D & direction );

private:
	Position3D			destination_;
};


/**
 * 	This controller moves an entity towards a given entity. It will stop
 * 	when it gets within a certain range of the entity.
 */
class MoveToEntityController : public MoveController
{
	DECLARE_CONTROLLER_TYPE( MoveToEntityController )

public:
	MoveToEntityController( EntityID destEntityID = 0,
			float velocity = 0.0f,
			float range = 0.0f,
			bool faceMovement = true,
			bool moveVertically = false );

	virtual void		stopReal( bool isFinalStop );
	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );
	void				update();

private:

	void				recalcOffset();

	EntityID			destEntityID_;
	EntityPtr			pDestEntity_;
	Position3D			offset_;
	float				range_;
};

BW_END_NAMESPACE

#endif // MOVE_CONTROLLER_HPP
