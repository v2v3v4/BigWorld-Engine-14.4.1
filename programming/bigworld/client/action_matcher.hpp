#ifndef ACTION_MATCHER_HPP
#define ACTION_MATCHER_HPP


#include "cstdmf/bw_vector.hpp"

#include "math/vector3.hpp"

#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "network/basictypes.hpp"
#include "duplo/motor.hpp"


BW_BEGIN_NAMESPACE

typedef BW::vector< SmartPointer<class SuperModelAction> > SMActionVector;


class PyEntity;
class PyModel;
typedef WeakPyPtr< PyEntity > PyEntityWPtr;


/*~ class BigWorld.ActionMatcher
 *
 *	ActionMatcher is a Motor that matches the movements and
 *	orientation of an entity to the actions defined on its primary model.
 *
 *	The ActionMatcher takes account of such information as:
 *
 *		- the speed of the Entity
 *
 *		- the angle between the Model and the Entity
 *
 *		- the angle between the Model and the Entity's velocity
 *
 *		- user-defined state flags
 *
 *	It also reads configuration information from Actions in the .model file, in
 *	order to work out which action to play and how fast to play it.
 *	The ActionMatcher starts at the top, and checks each action
 *	in order, until it finds one that matches the current criteria.
 *	The relevent parts of the action definition are within the &lt;match>
 *  section.
 *
 *		- &lt;trigger> contains the criteria for picking a particular action.
 *		  For an action to be chosen, all of the following must be true:
 *
 *			- The speed (magnitude of the velocity) of the Enitity is betweeen
 *			  &lt;minEntitySpeed> and &lt;maxEntitySpeed>.  If
 *			  &lt;minEntitySpeed> is less than zero, then the engine assumes
 *			  the entity is falling when it plays this animation.  Note that
 *			  minEntitySpeed should always be closer to zero, regardless of
 *			  sign, eg, min = -2, max = -7, min = 2, max = 7
 *
 *			- The angle (entityYaw - modelYaw)is between &lt;minEntityAux1> and
 *			  &lt;maxEntityAux1>.
 *
 *			- The angle (entityVelocity - modelYaw) is between
 *			  &lt;minEntityYaw> and &lt;maxEntityYaw>.  To match a model
 *			  walking backward, give it a a yaw range of between, 90 and 270.
 *
 *			- The ActionMatcher must have all of the caps specified in
 *			  &lt;capsOn> switched on.  These caps are bitpositions in a
 *			  bitfield, so range between 0 and 31.  The caps have no BigWorld
 *			  specific meanings, so it is up to the game to assign meaning to
 *			  specific numbers.
 *
 *			- The ActionMatcher must have all of the caps specified in
 *			  &lt;capsOff> switched off.
 *
 *		- &lt;scalePlaybackSpeed>, if set to true, specifies that the rate the
 *		  animation is played at will be scaled to match the movement rate.
 *		  If, for example, the animation was designed in 3dsmax to be played
 *		  at 30 frames a second and move at 2 m/s.  If the entity is moving at
 *		  3.5 m/s, and the valid range for this particular animation is 1 m/s
 *		  to 5 m/s then the animation will play 3.5/2.0 = 1.75 times faster
 *		  than the animator built it to play.
 *
 *		- &lt;feetFollowDirection>, if set to true, causes the model to turn to
 *		  track the DirectionCursor
 *
 *	By default, an ActionMatcher motor is automatically created for a primary
 *	Model. To create additional ActionMatchers, use BigWorld.ActionMatcher function.
 */
/**
 *	The job of the action matcher is to match the movements of an
 *	entity to the movement actions defined on its primary model.
 *
 *	If there are no actions available, it resorts to teleporting
 *	the model to the entity's current position each frame.
 *
 *	But give it a few actions to play with and it'll do its best
 *	to make things look right - the more the merrier.
 */
class ActionMatcher : public Motor
{
	Py_Header( ActionMatcher, Motor )

public:
	ActionMatcher( PyEntity * pPyEntity,
		PyTypeObject * pType = &ActionMatcher::s_type_ );
	~ActionMatcher();

	PY_FACTORY_DECLARE()

	void resetVelocitySmoothing(float time)
	{
		lastSpeedFor_=0.f;
		smoothingTime_=time;
	}

	virtual void onOwnerReloaded();


	PY_RW_ATTRIBUTE_DECLARE( matchCapabilities_, matchCaps )

	PY_RW_ATTRIBUTE_DECLARE( entityCollision_, entityCollision )
	PY_RW_ATTRIBUTE_DECLARE( collisionRooted_, collisionRooted )

	PY_READABLE_ATTRIBUTE_GET( matcherCoupled_, matcherCoupled )
	int pySet_matcherCoupled( PyObject * value );
	PY_RW_ATTRIBUTE_DECLARE( inheritOnRecouple_, inheritOnRecouple )
	PY_RW_ATTRIBUTE_DECLARE( turnModelToEntity_, turnModelToEntity )
	PY_RW_ATTRIBUTE_DECLARE( matchModelScale_, matchModelScale )
	PY_RW_ATTRIBUTE_DECLARE( useEntityModel_, useEntityModel )
	PY_RW_ATTRIBUTE_DECLARE( useEntityPitchAndRoll_, useEntityPitchAndRoll )
	PY_RW_ATTRIBUTE_DECLARE( velocityProvider_, velocityProvider )

	PY_RW_ATTRIBUTE_DECLARE( maxCollisionLod_, maxCollisionLod )

	PY_RW_ATTRIBUTE_DECLARE( bodyTwistSpeed_, bodyTwistSpeed )
	PY_RW_ATTRIBUTE_DECLARE( footTwistSpeed_, footTwistSpeed )

	PY_RW_ATTRIBUTE_DECLARE( fallNotifier_, fallNotifier )
	PY_RW_ATTRIBUTE_DECLARE( fallSelected_, fallSelected )

	PyObject * pyGet_lastMatch();
	PY_RO_ATTRIBUTE_SET( lastMatch )

	PyObject * pyGet_actions();
	PY_RO_ATTRIBUTE_SET( actions )

	PyObject * matchInfo( const BW::string& );
	PY_AUTO_METHOD_DECLARE( RETOWN, matchInfo, ARG(BW::string, END ) )

	PY_RW_ATTRIBUTE_DECLARE( fuse_, fuse )
	PY_RW_ATTRIBUTE_DECLARE( patience_, patience )
	PY_RW_ATTRIBUTE_DECLARE( boredNotifier_, boredNotifier )
	PY_RW_ATTRIBUTE_DECLARE( startMovingNotifier_, startMovingNotifier )

	PY_RW_ATTRIBUTE_DECLARE( debug_, debug )
	PY_RW_ATTRIBUTE_DECLARE( debugWorldVelocity_, debugWorldVelocity )
	PY_RW_ATTRIBUTE_DECLARE( matchNotifier_, matchNotifier )
private:
	virtual void attached();
	virtual void detached();

	virtual void rev( float dTime );

	void pullInfoFromSuperModel();


private:
	PyEntityWPtr	pPyEntity_;

	SMActionVector				pActions_;
	SMActionVector::iterator	lastAction_;
	BW::string	lastActionName_;

	float		lastSpeed_;
	float		lastSpeedFor_;
	float		smoothingTime_;
	float		lastFootTwist_;

	Capabilities	matchCapabilities_;

	bool		entityCollision_;
	bool		collisionRooted_;

	bool		matcherCoupled_;
	bool		inheritOnRecouple_;
	bool		turnModelToEntity_;
	bool		matchModelScale_;
	bool		useEntityModel_;
	bool		useEntityPitchAndRoll_;
	Vector4ProviderPtr velocityProvider_;

	float		maxCollisionLod_;

	float		bodyTwistSpeed_;
	float		footTwistSpeed_;

	SmartPointer<PyObject>	fallNotifier_;
	bool					fallSelected_;

	float		fuse_;
	float		patience_;
	SmartPointer<PyObject>		boredNotifier_;
	SMActionVector::iterator	boredAction_;

	SmartPointer<PyObject>		startMovingNotifier_;
	SmartPointer<PyObject>		matchNotifier_;

	EntityID	lastVehicleID_;
	Vector3		wLastVehiclePos_;
	float		wLastVehicleYaw_;
	float       wLastVehiclePitch_;
	float       wLastVehicleRoll_; 

	/// Used for debugging purposes only.
	bool			debug_;
	Vector4BasicPtr	debugWorldVelocity_;

	void doEntityCollisions( Vector3 & wCurrPos );

public:
	static bool				globalEntityCollision_;
	static bool				matchBots_;	///< Should bots be action matched
};


BW_END_NAMESPACE


#endif // ACTION_MATCHER_HPP

