#include "pch.hpp"

#include "action_matcher.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"
#include "duplo/pymodel.hpp"
#include "duplo/action_queue.hpp"
#include "model/super_model.hpp"

#include "app.hpp"
#include "entity.hpp"
#include "entity_manager.hpp"
#include "py_entity.hpp"
#include "script_player.hpp"


DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

bool ActionMatcher::globalEntityCollision_ = false;
bool ActionMatcher::matchBots_ = true;


PY_TYPEOBJECT( ActionMatcher )

PY_BEGIN_METHODS( ActionMatcher )
	PY_METHOD( matchInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ActionMatcher )
	/*~ attribute ActionMatcher.matchCaps
	 *
	 *	This attribute specifies which user defined capabilities the
	 *	ActionMatcher is matching at the moment.  It is a list of numbers
	 *	between 0 and 31.  It is checked against the &lt;capsOn> and
	 *	&lt;capsOff> fields (in the .model file definition of each action) for
	 *	each action, and the ActionMatcher will only match those actions for
	 *	which all caps in the &lt;capsOn >field are on in matchCaps	and all
	 *	those in &lt;capsOff> are NOT on.
	 *
	 *	Read-Write List of Integers (0-31)
	 */
	PY_ATTRIBUTE( matchCaps )
	/*~ attribute ActionMatcher.entityCollision
	 *
	 *	Specifies whether the model responds to collisions between entities.
	 *	If this is true then when the parent entity collides with another
	 *	entity, then one model, or the other, or both, will be moved away from
	 *	other model so that they dont overlap.   Which model moves is
	 *	determined by the collisionRooted flag on the ActionMatcher on both
	 *	models.  If collisionRooted is set on either model, then that model
	 *	will NOT move away from the entity, leaving the onus for movement on
	 *	the other model.  If both have it set, then neither will move.  If it
	 *	is clear on both of them then they will both move half as far.  It
	 *	defaults to 0.
	 *	Note that it is the model that moves, rather than than the entity,
	 *	making this ta client side effect only.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( entityCollision )
	/*~ attribute ActionMatcher.collisionRooted
	 *
	 *	This attribute controls how the model responds to entity to entity
	 *	collisions.  The entityCollision attribute controls whether the entity
	 *	will contribute towards collisions at all, collisionRooted controls
	 *	whether this model will move in response to collisions.  This model
	 *	will move directly away from the entity it has collided with, until its
	 *	bounding box no longer overlaps that of the model belonging to the
	 *	collided entity.  Note that it is the model	that moves, rather than the
	 *	entity, making this a client side effect only.
	 *
	 *	If this is non-zero, then it will NOT move in response to collisions,
	 *	otherwise it will.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( collisionRooted )
	/*~ attribute ActionMatcher.matcherCoupled
	 *
	 *	This attribute switches the ActionMatcher on and off. If the attribute
	 *	is false (0), then the ActionMatcher is	decoupled, otherwise, it is
	 *	coupled.  It defaults to 1.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( matcherCoupled )
	/*~ attribute ActionMatcher.inheritOnRecouple
	 *
	 *	This attribute is used for client controlling entities to smooth their motion when
	 *	the ActionMatcher is switched on after being off (which is done by
	 *	setting matcherCoupled to 1).  If inheritOnRecouple is true, then the
	 *	Entities position is set to the models position, otherwise the model is
	 *	moved immediately to the entities position, which can cause a visual
	 *	jar.  It defaults to 1.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( inheritOnRecouple )
	/*~ attribute ActionMatcher.turnModelToEntity
	 *
	 *	This attribute controls whether or not the ActionMatcher turns the
	 *	model to track direction changes in the Entity.  If it is true, then
	 *	the models yaw will be set to the Entities yaw.  If it is false, then
	 *	the action matcher doesn't control the model's yaw at all.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( turnModelToEntity )
	/*~ attribute ActionMatcher.matchModelScale
	 *
	 *	This attribute controls whether the model's scale is taken into 
	 *	account when choosing an action. Note: Only the Z scale is used 
	 *	as in the following entity.model.scale[2]
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( matchModelScale )
	/*~ attribute ActionMatcher.useEntityPitchAndRoll
	 *
	 *	This attribute controls whether the model should use the pitch and
	 *	roll of the entity.
	 *
	 *	@type Read-Write Boolean.
	 */
	PY_ATTRIBUTE( useEntityPitchAndRoll )
	/*~ attribute ActionMatcher.maxCollisionLod
	 *
	 *	If the LOD (level of detail) of the model is less than this amount,
	 *	then the model will be moved to respond to entity-entity collisions.
	 *	If it is greater than this amount, then the model won't respond to
	 *	collisions.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( maxCollisionLod )
	/*~ attribute ActionMatcher.bodyTwistSpeed
	 *
	 *	The angular speed at which the model will turn to line up with the
	 *	entity in radians per second.  Defaults to 2pi.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( bodyTwistSpeed )
	/*~ attribute ActionMatcher.footTwistSpeed
	 *
	 *	The angular speed at which the model can twist to match its
	 *	unitRotation in	radians per second. Defaults to 3pi.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( footTwistSpeed )
	/*~	attribute ActionMatcher.fallNotifier
	 *
	 *	The callback function to call when the entity transitions between
	 *	falling and not-falling.  On callback the function will receive one
	 *	argument, falling, which is 1 if the entity has started to fall, and 0
	 *	if it has just stopped falling.  The ActionMatcher decides if the
	 *	entity is falling based on the action that it chooses.  If the
	 *	minEntitySpeed of the trigger is less than zero, then it assumes that
	 *	the entity is falling. This will be the case when the falling checkbox is selected
	 *  in the model editor for an action. 
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( fallNotifier )
	/*~ attribute ActionMatcher.fallSelected
	 *
	 *	This attribute is set to true if the ActionMatcher chose a falling
	 *	action in its most recent choice.  The ActionMatcher decides if the
	 *	entity is falling based on the action that it chooses.  If the
	 *	minEntitySpeed of the trigger is less than zero, then it assumes that
	 *	the entity is falling.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( fallSelected )
	/*~ attribute ActionMatcher.lastMatch
	 *
	 *	This attribute is set to the name of the last action chosen by the
	 *	action matcher.
	 *
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( lastMatch )
	/*~ attribute ActionMatcher.actions
	 *
	 *	This attribute contains the name of all matchable actions in use
	 *	by the action matcher
	 *
	 *	@type	Read-Only Tuple
	 */
	PY_ATTRIBUTE( actions )
	/*~ attribute ActionMatcher.fuse
	 *
	 *	Fuse is the attribute that tracks how long (in seconds) a model has
	 *	been bored (playing the same action).  Once it exceeds patience, the
	 *	boredNotifier callback is called.  This works for all animations, not
	 *	just idle animations.  It is up	to the callback function to process
	 *	different types of animation.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( fuse )
	/*~ attribute ActionMatcher.patience
	 *
	 *	Patience is how long the model can continue playing the same action
	 *	for, before it becomes bored.  Fuse keeps track	of how long the same
	 *	animation has been playing. Once fuse exceeds patience, the
	 *	boredNotifier callback is called.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( patience )
	/*~ attribute ActionMatcher.boredNotifier
	 *
	 *	The boredNotifer is the callback function to call once fuse exceeds patience.
	 *	As an animation plays, fuse will increase.  When a new animation is played,
	 *	fuse will be reset.  The patience parameter is set by script.  If 0, the
	 *	boredNotifier will not be triggered.  The callback can be defined as follows:
	 *
	 *	@{
	 *		def onBored( self, actionName )
	 *	@}
	 *
	 *	The name of the action is that described in the .model file under
	 *	<action>/<name>.  It can be set in the Action panel of ModelViewer.
	 *
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( boredNotifier )
	/*~ attribute ActionMatcher.startMovingNotifier
	 *
	 *	The startMovingNotifier is a callback function to call when the entity
	 *	changes from a stationary to a moving state. This callback's purpose is
	 *	to provide the opportunity to the script programmer to clear the action
	 *	queue in the even that it had been set explicitly in script. The
	 *	callback can be defined as follows:
	 *
	 *	@{
	 *		def onStartMoving( self, actionName ):
	 *			for actionName in self.model.queue:
	 *				self.model.action(actionName).stop()
	 *			pass
	 *
	 *		self.am.startMovingNotifier = onStartMoving
	 *	@}
	 *
	 *	The name of the action is that described in the .model file under
	 *	<action>/<name>.  It can be set in the Action panel of ModelViewer.
	 *
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( startMovingNotifier )
	/*~ attribute ActionMatcher.useEntityModel
	 *
	 *	The useEntityModel attribute makes the action matcher use the entity's
	 *	model to check for movement and rotation, instead of the action
	 *	matcher's owner's model.
	 *	
	 *	This is particularly useful if the action matcher is constructed with
	 *	and entity that is different than the owner of the model that this
	 *	action matcher is used by.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( useEntityModel )
	/*~ attribute ActionMatcher.velocityProvider
	 *
	 *	The velocityProvider attribute allows you to use any Vector4Provider
	 *	to match against, instead of the entity velocity.  This is particularly
	 *	useful with player physics Vector4Providers, which allow you to remove
	 *	the effect of collision corrections.
	 */
	PY_ATTRIBUTE( velocityProvider )
	/*~ attribute ActionMatcher.debug
	 *
	 *	The debug attribute allows you to view internal action matcher
	 *	variables that are contributing towards matching, including information
	 *	about the currently matched action, if any.
	 */
	PY_ATTRIBUTE( debug )
	/*~ attribute ActionMatcher.debugWorldVelocity
	 *
	 *	The debug attribute exposes the internal world velocity
	 *	that was calculated and used by the action matcher to select
	 *	actions.
	 */
	PY_ATTRIBUTE( debugWorldVelocity )
	/*~	attribute ActionMatcher.matchNotifier
	 *
	 *	The callback function to call when the entity transitions between
	 *	one action-matched action and another.  On callback the function
	 *	will receive one argument, the name of the new action.	
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( matchNotifier )
PY_END_ATTRIBUTES()

/*~	function BigWorld.ActionMatcher
 *
 *	This function returns a new ActionMatcher Motor object, which is used to match the
 *	movements and orientation of an entity to the actions defined on its primary model.
 *	Note that the primary Model will be assigned an ActionMatcher Motor by default.
 *
 *	@return new ActionMatcher object
 */
PY_FACTORY( ActionMatcher, BigWorld )

/**
 *	Constructor
 */
ActionMatcher::ActionMatcher( PyEntity * pPyEntity, PyTypeObject * pType ) :
	Motor( pType ),
	pPyEntity_( pPyEntity ),
	lastSpeed_( 0 ),
	lastSpeedFor_( 0 ),
	lastFootTwist_( 0 ),
	smoothingTime_( 0.3f ),
	entityCollision_( false ),
	collisionRooted_( true ),
	matcherCoupled_( true ),
	inheritOnRecouple_( true ),
	turnModelToEntity_( false ),
	matchModelScale_( true ),
	useEntityModel_( false ),
	useEntityPitchAndRoll_( false ),
	maxCollisionLod_( 100.f ),
	bodyTwistSpeed_( DEG_TO_RAD( 360.0f ) ),
	footTwistSpeed_( DEG_TO_RAD( 540.0f ) ),
	fallSelected_( false ),
	velocityProvider_( NULL ),
	fuse_( 0.f ),
	patience_( 0.f ),
	lastVehicleID_( 0 ),
	lastActionName_(""),
	debug_( false ),
	debugWorldVelocity_( new Vector4Basic, true )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
ActionMatcher::~ActionMatcher()
{
	BW_GUARD;
	MF_ASSERT_DEV( pOwner_ == NULL );
}


 /**
 * if our pSuperModel_ has been reloaded, 
 * update the related data.
 */
void ActionMatcher::onOwnerReloaded()
{
	BW_GUARD;
	this->pullInfoFromSuperModel();
}

 /**
 *	This method pull the info from our pSuperModel_
 */
void ActionMatcher::pullInfoFromSuperModel()
{
	BW_GUARD;
	// gather the list of matchable actions
	if (pOwner_->pSuperModel())
	{
		pOwner_->pSuperModel()->getMatchableActions( pActions_ );
	}
	lastAction_ = pActions_.end();
}

/**
 *	This method is called by our base class after it attaches to a model.
 *
 *	We override it to get information out the model we attach to, and
 *	also set the world transform of the model to be that of the entity.
 */
void ActionMatcher::attached()
{
	BW_GUARD;
	MF_ASSERT_DEV( pActions_.empty() );

	// gather the list of matchable actions
	if (pOwner_->pSuperModel())
	{
		this->pullInfoFromSuperModel();
	}

	// restore the last action pointer.
	lastAction_ = pActions_.begin();
	SMActionVector::iterator end = pActions_.end();
	while (lastAction_ != end)
	{
		if ( (*lastAction_)->pSource_->name_ == lastActionName_ )
		{
			break;
		}
		else
		{
			++lastAction_;
		}
	}

	fuse_ = 0.f;
	if (patience_ < 0.f) patience_ = -patience_;

	// Note: we don't reinitialise all the 'lastXXX' variables here...
	//  because if we're detached from somewhere and reattached elsewhere,
	//  chances are these are exactly the things the user wants to keep.
	Matrix  currModelScale;
	currModelScale.setScale( pOwner_->worldTransform().applyToUnitAxisVector(0).length(),
								pOwner_->worldTransform().applyToUnitAxisVector(1).length(),
								pOwner_->worldTransform().applyToUnitAxisVector(2).length() );

	// set the the model's transform to the entity's
	Matrix	world;
	if (pPyEntity_.good() && pPyEntity_->pEntity())
	{
		Entity * pEntity = pPyEntity_->pEntity().get();
		world.setTranslate( pEntity->position() );
		world.preRotateY( pEntity->direction().yaw );
		if (pActions_.size() == 0)
		{
			world.preRotateX( pEntity->direction().pitch );
			world.preRotateZ( pEntity->direction().roll );
		}
	}
	else
	{
		world.setIdentity();
	}

	world.preMultiply( currModelScale );
	pOwner_->worldTransform( world );

	this->rev( App::instance().getFrameGameDelta() );
}

/**
 *	This method is called by our base class after it detaches from a model.
 *
 *	We override it to clean stuff up, releasing references to the supermodel.
 */
void ActionMatcher::detached()
{
	BW_GUARD;
	if (!pActions_.empty())	// empty <=> capacity zero for us
	{						// (i.e. no mem block if empty)
		if (lastAction_ != pActions_.end())
		{
			lastActionName_ = (*lastAction_)->pSource_->name_;
		}
		pActions_.clear();
	}
}



/**
 *	Static python factory method
 */
PyObject * ActionMatcher::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pObject;
	if (!PyArg_ParseTuple( args, "O!", &PyEntity::s_type_, &pObject ))
	{
		return NULL;
	}

	PyEntityPtr pPyEntity( pObject );

	if (pPyEntity->isDestroyed())
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.ActionMatcher: Expected a non-destroyed entity" );
		return NULL;
	}

	return new ActionMatcher( pPyEntity.get() );
}

/**
 *	Specialised set method for the 'matcherCoupled' attribute
 */
int ActionMatcher::pySet_matcherCoupled( PyObject * value )
{
	BW_GUARD;

	bool oldMC = matcherCoupled_;
	int ret = Script::setData( value, matcherCoupled_, "matcherCoupled" );
	if (ret == 0 && pOwner_ != NULL && pPyEntity_.good() &&
		pPyEntity_->pEntity() && matcherCoupled_ != oldMC)
	{
		// If we just turned ON the matcher coupling from being off,
		if (matcherCoupled_)
		{
			Entity * pEntity = pPyEntity_->pEntity().get();
			// Only do this for a locally-controlled entity, because for
			// server-controlled entities, a position update will be sent from
			// the server, overwriting this change almost immediately.
			// We also don't apply this position to entities under physics
			// correction, as the server has explicitly told us where to be.
			if (pEntity->isControlled() && inheritOnRecouple_ &&
				pEntity->isPhysicsAllowed())
			{
				// and if we're controlling this entity and want to be smooth,
				// then copy the model's world position into the entity's
				Position3D worldPosition =
					pOwner_->worldTransform().applyToOrigin();

				SpaceID spaceID;
				EntityID vehicleID;
				Position3D localPosition;
				Vector3 dummyPositionErr( Vector3::zero() );
				Direction3D localDirection;

				pEntity->getLatestMove( localPosition, vehicleID, spaceID,
					localDirection, dummyPositionErr );

				localPosition = worldPosition;
				Direction3D dummyWorldDirection( Vector3::zero() );

				if (vehicleID != NULL_ENTITY_ID)
				{
					Entity * pVehicle =
						ConnectionControl::instance().findEntity( vehicleID );
					if (pVehicle != NULL)
					{
						pVehicle->transformCommonToVehicle( localPosition,
							dummyWorldDirection );
					}
					else
					{
						// This shouldn't ever happen, it suggests we recently
						// boarded a vehicle that isn't in the world
						vehicleID = NULL_ENTITY_ID;
					}
				}

				pEntity->onMoveLocally( App::instance().getGameTimeFrameStart(),
					localPosition, vehicleID, /* is2DPosition */ false,
					localDirection );

				// Scripts should really call teleport on physics before
				// this time to give other clients the chance to have the
				// right position by the time they've finished the action.
			}
			else// either inheritOnRecouple_ is false or non-self-controlled entity
			{
				// Otherwise immediately set the model's world position to
				// the entity's, since it's going to pop next rev anyway
				// (when we will be turned on), and we don't want to match
				// some weird action to that pop (such as a deadly fall :)
				Matrix world = pOwner_->worldTransform();
				world.translation( pEntity->position() );
				pOwner_->worldTransform( world );
			}
		}
		else
		{
			// cancel matched action
			PyModel & model = *pOwner_;
			model.actionQueue().setMatch(NULL);
		}
	}

	return ret;
}








/**
 * TODO: to be documented.
 */
struct NiceV3
{
public:
	NiceV3( const Vector3 & v ) : v_(v) {}
	const Vector3 & v_;
};

std::ostream& operator <<( std::ostream& o, const NiceV3& t )
{
	return o << '(' << t.v_[0] << ',' << t.v_[1] << ',' << t.v_[2] << ')';
}


/** 
* This helper function transforms a point in the coordinate system of world 
* space into the coordinate system used by passengers that have us as their 
 * vehicle. 
 * In : vehicle pos / vehicle dir
 * In/Out: world pos -> vehicle relative pos 
 * In/Out: world yaw -> vehicle relative yaw 
*/ 
void worldToVehicle( 
    const Vector3& vehiclePos, 
    const Direction3D & vehicleDir,
    Vector3 & pos, 
    float& yaw ) 
{ 
    Matrix m; 
    m.setRotateInverse( vehicleDir.yaw, vehicleDir.pitch, vehicleDir.roll );
    pos = m.applyVector( pos - vehiclePos ); 
    yaw -= vehicleDir.yaw;
} 


/** 
 * This helper function is similar, but just transforms a vector. 
 * In : vehicle dir
 * In/Out: world space direction -> vehicle space direction 
*/ 
void worldToVehicle( 
    const Direction3D & vehicleDir,
    Vector3 & velocity ) 
{ 
    Matrix m; 
    m.setRotateInverse( vehicleDir.yaw, vehicleDir.pitch, vehicleDir.roll );
    velocity = m.applyVector( velocity ); 
} 


static DogWatch dwMatch("Match");
static DogWatch dwMatchLoop("Loop");
/*
static DogWatch dwMatchPreLoop("PreLoop");
static DogWatch dwMatchZero("Zero");
static DogWatch dwMatchStart("Start");
static DogWatch dwMatchStart2("Start2");
static DogWatch dwMatchStart3("Start3");
static DogWatch dwMatchEnd("End");
static DogWatch dwMatchBored("Bored");
static DogWatch dwMatchFall("Fall");
*/


/**
 *	This function implements the raison d'etre of this class.
 *
 *	@see ActionMatcher
 */
void ActionMatcher::rev( float dTime )
{
	BW_GUARD;
	MF_ASSERT_DEV( pOwner_ != NULL );

	if (!matcherCoupled_ || !pPyEntity_.good() || !pPyEntity_->pEntity())
	{
		return;
	}

	Entity * pEntity = pPyEntity_->pEntity().get();

	dwMatch.start();

	// dwMatchStart.start();
	PyModel & model = *pOwner_;

	// get current entity and model poses
	Vector3		wCurrPos = pEntity->position();
	float		wCurrYaw = pEntity->direction().yaw;
	float		wCurrPitch = pEntity->direction().pitch;
	float		wCurrRoll = pEntity->direction().roll;

	Matrix  currModelScale;
	currModelScale.setScale( model.worldTransform().applyToUnitAxisVector(0).length(),
							 model.worldTransform().applyToUnitAxisVector(1).length(),
							 model.worldTransform().applyToUnitAxisVector(2).length() );

	// dwMatchStart3.start();
	Matrix world;
	if ( useEntityModel_ )
	{
		if (pEntity->pPrimaryModel())
		{
			world = pEntity->pPrimaryModel()->worldTransform();
		}
		else
		{
			world.setRotate( wCurrYaw, wCurrPitch, wCurrRoll );
			world.postTranslateBy( wCurrPos );
		}
	}
	else
	{
		world = model.worldTransform();	
	}
	// dwMatchStart3.stop();

	// Check for Entity-to-Entity collision here.
	if (globalEntityCollision_ && entityCollision_ && !collisionRooted_ &&
		(model.pSuperModel() == NULL || maxCollisionLod_ <= 0.f ||
			model.pSuperModel()->lastLod() < maxCollisionLod_))
	{
		this->doEntityCollisions( wCurrPos );
	}
	// dwMatchStart.stop();

	// if this model doesn't have any movement actions, get out now
	if (pActions_.empty())
	{
		// dwMatchZero.start();
		Matrix newWorld;
		newWorld.setRotate( wCurrYaw, wCurrPitch, wCurrRoll );
		newWorld.postTranslateBy( wCurrPos );
		newWorld.preMultiply( currModelScale );

		// check this because setting the world transform is expensive
		if (newWorld != world)
		{
			model.worldTransform( newWorld );
		}

		// dwMatchZero.stop();
		dwMatch.stop();
		return;
	}

	//'w' variables are in world space.  This also happens to be what 
 	//we get from calls to the entity etc. 
 	Vector3     wLastPos = world.applyToOrigin(); 
 	float       wLastYaw = world.yaw(); 
 	float       wLastPitch = world.pitch(); 
 	float       wLastRoll = world.roll(); 

 	//'v' variables are in vehicle space.  This could also be called 
 	//local space.  If not on a vehicle, these are the same as world space. 
 	Vector3     vCurrPos( wCurrPos ); 
 	float       vCurrYaw( wCurrYaw ); 
 	Vector3     vLastPos( wLastPos ); 
 	float       vLastYaw( wLastYaw ); 

	// fiddle with the old position if we were on a vehicle
	// TODO: 1. Make it work for moving spaces.
	// TODO: 2. Do it all a better way! (e.g. store auto/selfDisplacement_ in
	//  Entity when 'pos' called? careful of transitions...)
	BWEntity * pVehicle = pEntity->pVehicle();
	if (pVehicle != NULL)
	{
		EntityID currVehicleID = pVehicle->entityID();
		Vector3 wCurrVehiclePos = pVehicle->position();
		float wCurrVehicleYaw = pVehicle->direction().yaw;
		float wCurrVehiclePitch = pVehicle->direction().pitch;
		float wCurrVehicleRoll = pVehicle->direction().roll;

 		if ( lastVehicleID_ != currVehicleID )
		{
			lastVehicleID_ = currVehicleID;
			wLastVehiclePos_ = wCurrVehiclePos; 
 			wLastVehicleYaw_ = wCurrVehicleYaw; 
 			wLastVehiclePitch_ = wCurrVehiclePitch; 
 			wLastVehicleRoll_ = wCurrVehicleRoll; 
 		} 

		// Use the last vehicle pos/yaw to calculate the old local pos/yaw in
		// vehicle coordinate of the last frame.
		Direction3D wLastVehicleDir;
		wLastVehicleDir.yaw = wLastVehicleYaw_;
		wLastVehicleDir.pitch = wLastVehiclePitch_;
		wLastVehicleDir.roll = wLastVehicleRoll_;
		worldToVehicle( wLastVehiclePos_, wLastVehicleDir, vLastPos, vLastYaw );

		// Calculate local entity pos and yaw
		worldToVehicle( wCurrVehiclePos, wLastVehicleDir, vCurrPos, vCurrYaw );

		// Calculate wLastPos and wLastYaw with respect to the current vehicle's
		// reference frame
 		wLastPos = vLastPos; 
		Direction3D aux;
		aux.yaw = vLastYaw;
		aux.pitch = 0.f;
		aux.roll = 0.f;
		// Entity::transformVehicleToCommon only changes Direction3D::yaw.
		pVehicle->transformVehicleToCommon( wLastPos, aux );
		wLastYaw = aux.yaw;

 		wLastVehiclePos_ = wCurrVehiclePos; 
 		wLastVehicleYaw_ = wCurrVehicleYaw; 
 		wLastVehiclePitch_ = wCurrVehiclePitch; 
 		wLastVehicleRoll_ = wCurrVehicleRoll; 
	}
	else
	{
		lastVehicleID_ = 0;
	}

	// initially the model isn't going anywhere
	Vector3		wNewModelPos = wLastPos;
	float		wNewModelYaw = wLastYaw;
	float		wNewModelPitch = wLastPitch;
	float		wNewModelRoll = wLastRoll;

	float		newFootTwist = 0.f;

	// find the change in speed that this entails
	Vector3		vVelocity;

	if( pEntity->localVelocity().lengthSquared() > 0 )
	{
		vVelocity = pEntity->localVelocity();
 		vVelocity.normalise(); 
 		float speed = Vector3(vCurrPos - vLastPos ).length(); 
 		//use local movement displacement (speed in vehicle space) 
 		vVelocity *= speed * ( 1.0f / dTime );  
	}
	else
	{
		//use local movement displacement (speed in vehicle space) 
 		vVelocity = ( vCurrPos - vLastPos ) * ( 1.0f / dTime ); 
	}

	if (velocityProvider_.hasObject())
	{
		//velocityProvider_ is already in vehicle space.
		Vector4 v4;
		velocityProvider_->output( v4 );
		vVelocity.set( v4.x, v4.y, v4.z ); 
 	} 


 	float   deltaFall = vVelocity[1]; 
 	Vector3 vHorizontalVelocity = vVelocity; 
 	vHorizontalVelocity[1] = 0.f; 

 	const bool startedMoving =  lastSpeed_ < 0.1f && 
 	                            vHorizontalVelocity.length() >= 0.1f;

	// if entity is player and no impacting actions are affecting its position,
	// then estimate the eventual speed as a multiplier of current speed.
	float		blendingSpeedFactorEstimate = 1.f;
	if (pEntity == ScriptPlayer::entity() &&
		model.actionQueue().lastPromotion().lengthSquared() < 0.1f)
	{
		// If this entity is the player then we compensate for its
		// velocity smoothing (i.e. momentum) to determine the speed
		// it's building up/down to, so we know what action to blend in to

		// i.e. when we detect a change in velocity (an acceleration)
		// we pretend it's going to change smoothly at that rate for
		// the standard blendInTime (0.3) seconds.

		// Another way to do this (for every entity) could be to say
		// 'OK, we're doing this set of movement actions at these blend
		// ratios which make for a certain apparent velocity. Now what's the
		// best action we can select to make that velocity look like the
		// entity's velocity, bearing in mind that if we select a different
		// action it will only add as much as it can blend in over this
		// next frame.' And no, I can't find the words to express that
		// more concisely right now :)

		lastSpeedFor_ += dTime;

		float		newSpeed = vHorizontalVelocity.length();
		float		newSpeedDelta = newSpeed - lastSpeed_;
		if (lastSpeedFor_ <= smoothingTime_)
		{
			// ok, scale this delta up as if it took smoothingTime_
			float estimateSpeed = lastSpeed_ +
				newSpeedDelta * ((smoothingTime_-lastSpeedFor_) + dTime)/dTime;
			// don't let it get too small
			if (estimateSpeed < 0) estimateSpeed = 0;

			if (newSpeed > 0.001f)
			{
				blendingSpeedFactorEstimate = estimateSpeed/newSpeed;
			}
		}

		lastSpeed_ = newSpeed;
	}
	else
	{
		lastSpeed_ = vHorizontalVelocity.length();
	}

	// dwMatchPreLoop.start();
	// express this displacement from the model's point of view
	Matrix	vRotor;
	vRotor.setRotateY( -vLastYaw );
	Vector3 vDeltaRot = vRotor.applyPoint( vHorizontalVelocity );

	float modelScaleFactor = 1.0f;
	if( matchModelScale_ && pEntity->pPrimaryModel() )
	{
		modelScaleFactor = pEntity->pPrimaryModel()->worldTransform().applyToUnitAxisVector(2).length();
	}


	float rLen = vDeltaRot.length() * blendingSpeedFactorEstimate * modelScaleFactor;
	float rYaw = vDeltaRot.yaw();

	//use delta yaw in vehicle coordinate system 
	float rAux1 = Angle( vCurrYaw - vLastYaw );
	Capabilities rCaps = matchCapabilities_;
	// dwMatchPreLoop.stop();

	dwMatchLoop.start();

	// and find a matching action!
	SMActionVector::iterator i;
	for (i = pActions_.begin(); i != pActions_.end(); i++)
	{
		const MatchInfo & mi = (*i)->pSource_->matchInfo_;

		// does this action trigger?
		if (mi.trigger.satisfies( rCaps, rLen, rYaw, rAux1 ))
		{
			break;			// yep
		}

		bool trigCapsOK = rCaps.match( mi.trigger.capsOn, mi.trigger.capsOff );
		// does it trigger by falling?
		if (trigCapsOK && mi.trigger.minEntitySpeed < 0.f &&
			deltaFall < mi.trigger.minEntitySpeed &&
			deltaFall > mi.trigger.maxEntitySpeed)
		{
			break;			// yep
		}

		// were we doing this action last time?
		if (lastAction_ != i)
		{
			continue;		// nope
		}

		// have we gone on too long?
		if (mi.oneShot)
		{
			float	matchCycleCount = model.actionQueue().matchCycleCount() +
				(*i)->pSource_->blendOutTime_ / (*i)->pFirstAnim_->duration_;

			if (matchCycleCount >= 1.f)
			{
				continue;	// yep
			}
		}

		// so does it cancel?
		if (mi.cancel.satisfies( rCaps, rLen, rYaw, rAux1 ))
		{
			continue;		// yep
		}

		// so shall we keep doing this one then?
		break;				// yep
	}
	dwMatchLoop.stop();

	// dwMatchEnd.start();

	// ok, what did we find to match that movement?
	SMActionVector::iterator	origI = i;
	bool fallSelected = false;
	float inTime = -1.0f;
	if (i == pActions_.end())
	{
		wNewModelPos = wCurrPos;		// just teleport the model
		wNewModelYaw = wCurrYaw;
		wNewModelPitch = wCurrPitch;
		wNewModelRoll = wCurrRoll;

		//DEBUG_MSG( "AM: rLen %g, rYaw %g: None\n", rLen, rYaw );
		model.actionQueue().setMatch( NULL );
	}
	else
	{
		if ((*i)->pSource_->matchInfo_.scalePlaybackSpeed)
		{
			const ModelAnimation * pAnim = (*i)->pFirstAnim_;
			float	dbLen = pAnim->flagFactorBit( 0 ).applyToOrigin().length() /
				pAnim->duration_;

			//float	drLen = desiredOffset.length();
			Vector3 speedOffset = vVelocity * dTime;
			speedOffset[1] = max(0.f,speedOffset[1]);
			float drLen = speedOffset.length();

			if (dbLen != 0.0f)
			{
				// nickl TODO

				// inTime = dTime * ( drLen / (dbLen * dTime) );
				inTime = model.moveScale() * drLen / dbLen;	// cancelled out 'dTime'
			}
		}

		wNewModelPos = wCurrPos;

		bool turnModelToEntity = turnModelToEntity_;
		if ((*i)->pSource_->matchInfo_.feetFollowDirection)
		{
			// point the feet in the direction of movement
			if ((*i)->pSource_->isMovement_)
			{
				const Vector3 naturalDir =
					model.initialItinerantContextInverse().applyVector(
					(*i)->pFirstAnim_->flagFactorBit( 0 ).applyToOrigin() )*-1.f;
				float naturalYaw = atan2f(naturalDir[0],naturalDir[2]);

				// note about coordinate systems here : the natural Dir etc are 
 				// from the animiation, and therefore are in local space.  The 
 				// rYaw variable from above is calculated in vehicle / local space 
 				// and thus you can subtract the two. 
				newFootTwist = Angle( rYaw - naturalYaw );
			}

			turnModelToEntity = true;
		}
		if (turnModelToEntity)
		{
			// turn the model to the yaw of the entity
			//  (pretty quick smart too)
			const float bodyTwistMax = bodyTwistSpeed_ * dTime;
			wNewModelYaw = wLastYaw + Math::clamp( bodyTwistMax,
				float(Angle(wCurrYaw - wLastYaw)) );

			if (useEntityPitchAndRoll_)
			{
				wNewModelPitch = wLastPitch + Math::clamp( bodyTwistMax,
					float(Angle(wCurrPitch - wLastPitch)) );

				wNewModelRoll = wLastRoll + Math::clamp( bodyTwistMax,
					float(Angle(wCurrRoll - wLastRoll)) );
			}
		}

		// smooth the foot twist, regardless of who set it
		const float footTwistMax = footTwistSpeed_ * dTime;
		newFootTwist = lastFootTwist_ + Math::clamp( footTwistMax,
			newFootTwist - lastFootTwist_ );

		//DEBUG_MSG( "AM: rLen %g, rYaw %g, rAux1 %g: %s\n", rLen, rYaw, rAux1,
		//	(*i)->pSource->name_.c_str() );
		model.actionQueue().setMatch( *i, dTime, inTime,
			(*i)->pSource_->promoteMotion() );

		if (!model.actionQueue().lastMatchAccepted())
		{
//			dprintf( "Match not accepted\n" );
			i = pActions_.end();
		}
		else
		{
			// record if we sucessfully (as far as we can tell)
			// suggested a falling action
			fallSelected =
				(*i)->pSource_->matchInfo_.trigger.minEntitySpeed < 0.f;
		}
	}

	// dwMatchFall.start();
	// if we've changed whether or not we're falling then call the notifier
	// (this system should really be a bit more generic)
	if (fallSelected_ != fallSelected)
	{
		fallSelected_ = fallSelected;
		if (fallNotifier_)
		{
			Py_INCREF( &*fallNotifier_ );
			Script::call(
				&*fallNotifier_,
				Py_BuildValue( "(i)", int( fallSelected ) ),
				"ActionMatcher fall notifier callback: " );
		}
	}
	// dwMatchFall.stop();



	if (startedMoving && startMovingNotifier_)
	{
		//MF_ASSERT_DEV(PyCallable_Check(startMovingNotifier_));

		Py_INCREF( &*startMovingNotifier_ );

		if (i != pActions_.end())
		{
			Script::call(
				&*startMovingNotifier_,
				Py_BuildValue( "(s)", (*i)->pSource_->name_.c_str() ),
				"ActionMatcher start moving notifier on: " );
		}
		else
		{
			Script::call(
				&*startMovingNotifier_,
				Py_BuildValue( "(O)", Py_None ),
				"ActionMatcher start moving notifier on: " );
		}
	}


	// dwMatchBored.start();
	// see if we're bored
	if (boredNotifier_)
	{
		if (patience_ > 0.f)
		{
			if (i == lastAction_ && i != pActions_.end())
			{
				fuse_ += dTime;
				if (fuse_ >= patience_)
				{
					patience_ = -patience_;
					boredAction_ = i;
					Py_INCREF( &*boredNotifier_ );
					Script::call(
						&*boredNotifier_,
						Py_BuildValue( "(s)", (*i)->pSource_->name_.c_str() ),
						"ActionMatcher bored notifier on: " );
				}
			}
			else
			{
				fuse_ = 0.f;
			}
		}
		else if (patience_ < 0.f)
		{
			if (origI != boredAction_)
			{
				patience_ = -patience_;
				Py_INCREF( &*boredNotifier_ );
				Script::call(
					&*boredNotifier_,
					Py_BuildValue( "(O)", Py_None ),
					"ActionMatcher bored notifier off: " );
			}
		}
	}


	if (matchNotifier_ && lastAction_ != i)
	{		
		Py_INCREF( &*matchNotifier_ );
		if (i != pActions_.end())
		{
			Script::call(
				&*matchNotifier_,
				Py_BuildValue( "(s)", (*i)->pSource_->name_.c_str() ),
				"ActionMatcher match notifier on: " );
		}
		else
		{
			Script::call(
				&*matchNotifier_,
				Py_BuildValue( "(O)", Py_None ),
				"ActionMatcher match notifier on: " );
		}
	}


	// dwMatchBored.stop();
	lastAction_ = i;


	// apply the changes to the model's pose
	float wYaw( wNewModelYaw ), wPitch( wLastPitch ), wRoll( wLastRoll ); 
	if (useEntityPitchAndRoll_)
	{
		wPitch = wNewModelPitch; 
 	    wRoll = wNewModelRoll; 
 	} 
	Matrix newWorld;
	newWorld.setRotate( wYaw, wPitch, wRoll );
	newWorld.translation( wNewModelPos );
	newWorld.preMultiply( currModelScale );
	model.worldTransform( newWorld );

	lastFootTwist_ = newFootTwist;


	model.unitRotation( newFootTwist );


	debugWorldVelocity_->set( Vector4(vVelocity, 1.f) );

	if (debug_)
	{
		BW::stringstream		amDebug;
		amDebug << NiceV3(vVelocity * dTime) << std::ends;
		model.setDebugString( "local velocity", amDebug.str() );

		BW::stringstream		amDebug2;
		amDebug2 << rLen << std::ends;
		model.setDebugString( "speed", amDebug2.str() );

		BW::stringstream		amDebug3;
		amDebug3 << rYaw << std::ends;
		model.setDebugString( "relative velocity direction", amDebug3.str() );

		BW::stringstream		amDebug4;
		amDebug4 << rAux1 << std::ends;
		model.setDebugString( "relative yaw", amDebug4.str() );

		BW::stringstream		amDebug5;
		if (i == pActions_.end())
		{
			amDebug5 << "<None>";
		}
		else if (model.actionQueue().lastMatchAccepted())
		{
			amDebug5 << (*i)->pSource_->name_ << " @ " << inTime;
		}
		else
		{
			amDebug5 << "<Overridden>";
		}
		amDebug5 << std::ends;
		model.setDebugString( "matched action", amDebug5.str() );
	}

	// dwMatchEnd.stop();
	dwMatch.stop();
}

// this function (above) gets called from tick, and tick only gets called
// when we're in the world, so its OK to add actions to the action queue.
// (and since we're not using callbacks it's ok anyway)



/**
 *	This function collides the entity that we control with other
 *	entities around it.
 *
 *	@param wCurrPos		Entity position on input and output
 */
void ActionMatcher::doEntityCollisions( Vector3 & wCurrPos )
{
	BW_GUARD;

	if (!pPyEntity_.good() || !pPyEntity_->pEntity())
	{
		return;
	}

	Entity * pEntity = pPyEntity_->pEntity().get();

	static DogWatch dwCollisions("Collide");
	dwCollisions.start();

	// TODO: Our neighbours should be attached ActionMatchers, not entities
	Entity::Neighbour iter	= pEntity->begin( 3.0f );
	Entity::Neighbour end	= pEntity->end( 3.0f );
	for ( ; iter != end; iter++ )
	{
		// Anything in the Neighbour list must also be in-world.
		MF_ASSERT( iter->isInWorld() )

		PyModel * pModel = iter->pPrimaryModel();
		if (pModel == NULL) continue;

		Motor * pMotor = pModel->motorZero();
		if (!pMotor || !ActionMatcher::Check( pMotor )) continue;

		ActionMatcher * pAM = (ActionMatcher*)pMotor;

		// For each other entity, check that it does not collide with it.
		if ( pAM->entityCollision_ )
		{
			// 1. If they are too far from the camera, don't bother with
			//	  collision detection either, since the player will not
			//	  be able to see it.
			// JWD: Actually, we can't do this due to action matching methinks

			// 2. Check if they are close enough to affect each other
			//	  in the y height.
			float otherHeight = iter->position().y;
			float newHeight = wCurrPos.y;

			// TODO: The magic eight ball says 1.8 metres is a good height.
			//		The magic eight ball is never to be trusted. Fix this.
			const float entityHeight = 1.8f;
			if ( ( otherHeight > newHeight + entityHeight ) ||
				( newHeight > otherHeight + entityHeight ) )
			{
				// The two Entities are not vertically spaced to
				// affect each other. Move on to the next Entity.
				continue;
			}

			// 3. Check if they are close enough in the x-z plane.

			// Limit the size of things to a diametre of 10 metres.
			const double maxDiametre = 10.0;
			double xDist = fabs( iter->position().x - wCurrPos.x );
			if ( xDist >= maxDiametre )
			{
				continue;
			}
			double zDist = fabs( iter->position().z - wCurrPos.z );
			if ( zDist >= maxDiametre )
			{
				continue;
			}

			// Set up the vectors in 2D for cylinders.
			Vector2 otherPos( iter->position().x, iter->position().z );
			Vector2 newPos( wCurrPos.x, wCurrPos.z );
			Vector2 lookAt = otherPos - newPos;

			// TODO: The magic eight ball says 0.6 metre is the answer.
			//		The magic eight ball is never to be trusted. Fix this.
			const float bubbleRadius = 0.6f;

			// 4. Apply the 'electro-static repulsion cylindrical' bubble
			// to the current Entity.
			float distSQ = lookAt.lengthSquared();
			if ( distSQ < bubbleRadius * bubbleRadius )
			{
				if ( distSQ == 0.0f )
				{
					lookAt = Vector2( 0.0f,
						(iter->entityID() < pEntity->entityID()) ?
							1.0f : -1.0f );
				}
				else
				{
					lookAt.normalise();
				}

				if ( pAM->collisionRooted_ )
				{
					lookAt *= ( bubbleRadius - sqrtf( distSQ ) );
				}
				else
				{
					lookAt *= ( bubbleRadius - sqrtf( distSQ ) ) / 2;
				}
				Vector3 lookAtIn3D( lookAt.x, 0.0f, lookAt.y );
				wCurrPos -= lookAtIn3D ;
			}
		}
	}

	dwCollisions.stop();
}


/**
 *	Get the string name of the last action matched, or None if no match
 */
PyObject * ActionMatcher::pyGet_lastMatch()
{
	BW_GUARD;
	if (pOwner_ == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "ActionMatcher.lastMatch: "
			"ActionMatcher is not attached to a Model" );
		return NULL;
	}

	if (lastAction_ == pActions_.end())
	{
		Py_RETURN_NONE;
	}

	return Script::getData( (*lastAction_)->pSource_->name_ );
}


/**
 *	Get a tuple of action names
 */
PyObject * ActionMatcher::pyGet_actions()
{
	BW_GUARD;

	PyObject * result = PyTuple_New( pActions_.size() );
	for (size_t i=0; i<pActions_.size(); i++)
	{
		PyTuple_SetItem( result, i, PyString_FromString(pActions_[i]->pSource_->name_.c_str()) );
	}

	return Script::getData(result);
}


PyObject * ActionMatcher::matchInfo( const BW::string& actionName )
{
	for (size_t i=0; i<pActions_.size(); i++)
	{
		if ( pActions_[i]->pSource_->name_ == actionName )
		{
			const MatchInfo& mi =  pActions_[i]->pSource_->matchInfo_;
			PyObject * result = PyTuple_New( 12 );

			PyTuple_SetItem( result, 0, Script::getData(mi.trigger.minEntitySpeed) );
			PyTuple_SetItem( result, 1, Script::getData(mi.trigger.maxEntitySpeed) );
			PyTuple_SetItem( result, 2, Script::getData(mi.trigger.minEntityAux1) );
			PyTuple_SetItem( result, 3, Script::getData(mi.trigger.maxEntityAux1) );
			PyTuple_SetItem( result, 4, Script::getData(mi.trigger.minModelYaw) );
			PyTuple_SetItem( result, 5, Script::getData(mi.trigger.maxModelYaw) );

			PyTuple_SetItem( result, 6, Script::getData(mi.cancel.minEntitySpeed) );
			PyTuple_SetItem( result, 7, Script::getData(mi.cancel.maxEntitySpeed) );
			PyTuple_SetItem( result, 8, Script::getData(mi.cancel.minEntityAux1) );
			PyTuple_SetItem( result, 9, Script::getData(mi.cancel.maxEntityAux1) );
			PyTuple_SetItem( result, 10, Script::getData(mi.cancel.minModelYaw) );
			PyTuple_SetItem( result, 11, Script::getData(mi.cancel.maxModelYaw) );

			return result;
		}
	}

	Py_RETURN_NONE;
}

BW_END_NAMESPACE

// action_manager.cpp
