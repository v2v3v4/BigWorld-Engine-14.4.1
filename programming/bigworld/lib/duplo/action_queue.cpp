#include "pch.hpp"

#include "action_queue.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/debug.hpp"

#include "pyscript/script.hpp"
#include "pyscript/py_callback.hpp"
#include "pymodel.hpp"

#include "math/colour.hpp"

#include "model/super_model.hpp"

#include "moo/animation_channel.hpp"
#include "moo/cue_channel.hpp"

#include "romp/font_manager.hpp"
#include "moo/geometrics.hpp"
#include "romp/font.hpp"

DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

/**
 *	This class is what we use when we want no fashion :)
 *	It is much nicer than changing supermodel to test for a NULL pointer
 *	in each supplied fashion, or making a new fashion vector each time,
 *	or passing in a horribly-cast structure that looks like a vector
 *	but is without the first element. (The first element is reserved
 *	for the matched action. Should probably have some way of disallowing
 *	matched actions which would then not require this.... hmmmm)
 */
class NullTransformFashion : public TransformFashion
{
public:

	/**
	 *	NullTransformFashion does not require models to re-calculate their
	 *	transform state after this fashion is applied.
	 *	@return false
	 */
	bool needsRedress() const { return false; }
	void dress( SuperModel & superModel, Matrix& world ) { }
};

/**
 *	This static member is a fashionless fashion
 *	@see NullTransformFashion
 */
static TransformFashionPtr s_nullTransformFashion = new NullTransformFashion();

// -----------------------------------------------------------------------------
// Section: CueResponse
// -----------------------------------------------------------------------------


/**
 *	Constructor
 */
CueResponse::CueResponse( Moo::CueBasePtr cueBase, PyObject * function ) :
	cueBase_( cueBase ),
	function_( function )
{
}


/**
 *	This method responds to fired cues with a given vector of responses
 */
void CueResponse::respond( uint firstCue, const CueResponses & responses,
	PyModel *pModel )
{
	BW_GUARD;
	Moo::CueShots::iterator cit;
	CueResponses::const_iterator rit;

	for (cit = Moo::CueShot::s_cueShots_.begin() + firstCue;
		cit < Moo::CueShot::s_cueShots_.end();
		cit++)
	{
		char type = cit->pCue_->cueBase_->type();

		DEBUG_MSG( "Cue shot: %c %s by %f\n",
			type, cit->pCue_->cueBase_->identifier(), cit->missedBy_ );

		if (type == 's')
		{
			if (pModel != NULL)
				pModel->playSound( cit->pCue_->cueBase_->identifier() );
		}

		if (type != 'c') continue;

		for (rit = responses.begin(); rit != responses.end(); rit++)
		{
			if (rit->cueBase_ == cit->pCue_->cueBase_)
			{
				rit->respond( cit->missedBy_, &cit->pCue_->args_ );
			}
		}
	}
}


/**
 *	This method makes the intended response.
 *
 *	If the args are null, then it is called without even the missedBy argument.
 */
void CueResponse::respond( float missedBy,
	const std::basic_string<float> * args ) const
{
	BW_GUARD;
	// get fn
	PyObject * fn = &*function_;
	Py_INCREF( fn );

	// get args
	PyObject * pTuple;
	if (args == NULL)
	{
		pTuple = PyTuple_New( 0 );
	}
	else
	{
		const std::basic_string<float> & cueArgs = *args;
		pTuple = PyTuple_New( 1 + cueArgs.length() );
		PyTuple_SET_ITEM( pTuple, 0,
			PyFloat_FromDouble( missedBy ) );
		for (uint i = 0; i < cueArgs.length(); i++)
			PyTuple_SET_ITEM( pTuple, i+1,
				PyFloat_FromDouble( cueArgs[i] ) );
	}

#ifndef EDITOR_ENABLED
	// we let the script callback system sort the calls for us
	Script::callNextFrame(
		fn, pTuple, "ActionQueue Callback: ", missedBy );
#endif
}


// -----------------------------------------------------------------------------
// Section: ActionQueue
// -----------------------------------------------------------------------------


/// Constructor
ActionQueue::ActionQueue() :
	matchActionTime_( -1.f ),
	lastMatchAccepted_( false ),
	lastPromotion_( Vector3::zero() ),
	lastYawPromotion_( 0 ),
	needsMollifying_( false )
{
	BW_GUARD;
	this->addMatchedActionPlaceholder();
}


/// Destructor
ActionQueue::~ActionQueue()
{
	BW_GUARD;
	this->flush();
}




/**
 *	This method saves an action queue element to a data section.
 *
 *	@param state A AQElement to save.
 *	@param state A DataSectionPtr to save the AQElement to.
 */
void ActionQueue::saveAQElement( AQElement& aqe, DataSectionPtr actionState )
{
	actionState->writeString( "name", aqe.action_->pSource_->name_ );
	actionState->writeFloat( "finish", aqe.action_->finish_ );
	actionState->writeFloat( "passed", aqe.action_->passed_ );
	actionState->writeFloat( "played", aqe.action_->played_ );
	actionState->writeFloat( "lastPlayed", aqe.action_->lastPlayed_ );
	actionState->writeBool( "promoteMotion", aqe.promoteMotion_ );
	if ( aqe.link_ != NULL )
		actionState->writeString( "link", aqe.link_->action_->pSource_->name_ );
}


/**
 *	This function saves the current state of the action queue. This is needed
 *	as when a supermodel is recreated its action references will become invalid to the
 *	action queue.
 *
 *	@param state A DataSectionPtr to save the current state of the action queue
 */
void ActionQueue::saveActionState( DataSectionPtr state )
{
	BW_GUARD;
	state->deleteSections( "action" );
	// Save the currently matched action
	if ( actionState_[0].action_.hasObject() )
	{
		AQElement &aqe = actionState_[0];
		DataSectionPtr actionState = state->newSection( "matchedAction" );
		saveAQElement( aqe, actionState );
	}

	// save everything currently in the actionState_
	for ( uint32 i = 1; i < actionState_.size(); i++ )
	{
		AQElement &aqe = actionState_[i];
		DataSectionPtr actionState = state->newSection( "action" );
		saveAQElement( aqe, actionState );
	}

	// And save everything in the waiting list
	for ( uint32 i = 0; i < waiting_.size(); i++ )
	{
		AQElement &aqe = waiting_[i];
		DataSectionPtr actionState = state->newSection( "action" );
		saveAQElement( aqe, actionState );
	}
}

/**
 *	This function restores the current state of the action queue from the supplied
 *	saved state. Allowing the supermodel to be seemlessly changed in realtime without 
 *	it restarting all of its current actions.
 *
 *	@param state A DataSectionPtr of the action queue state to load from
 *	@param pSuperModel A pointer to the new supermodel
 *	@param pMatrix A pointer to a MatrixLiaison object
 *
 *	@return Returns True if the restore operation was successful, False otherwise. A restore
 *			operation may not be successful as the new supermodel may not have all the same 
 *			actions in the actions queue as the previous supermodel.
 */
bool ActionQueue::restoreActionState( DataSectionPtr state, SuperModel *pSuperModel, MatrixLiaison* pMatrix )
{
	BW_GUARD;
	flush();

	DataSectionPtr savedAction;
	bool success = true;

	savedAction = state->openSection( "matchedAction" );
	if ( savedAction.hasObject() )
	{
		SuperModelActionPtr action = pSuperModel->getAction( savedAction->readString( "name", "" ) );
		if ( action )
		{
			actionState_[0].action_ = action;
			actionState_[0].link_ = NULL;
			actionState_[0].responses_ = NULL;
			actionState_[0].promoteMotion_ = savedAction->readBool( "promoteMotion", false );
			action->finish_ = savedAction->readFloat( "finish", -1.f );
			action->passed_ = savedAction->readFloat( "passed", 0.f );
			action->played_ = savedAction->readFloat( "played", 0.f );
			action->lastPlayed_ = savedAction->readFloat( "lastPlayed", 0.f );
			transformFashions_[0] = action;
		}
		else
		{
			transformFashions_[0] = s_nullTransformFashion;
		}
	}

	BW::vector<DataSectionPtr> actions;
	state->openSections( "action", actions );
	for( uint32 i = 0; i < actions.size(); i++ )
	{
		savedAction = actions[i];
		SuperModelActionPtr action = pSuperModel->getAction( savedAction->readString( "name", "" ) );
		if ( action )
		{
			SuperModelActionPtr link = NULL;
			BW::string linkName = savedAction->readString( "link", "" );
			if ( linkName != "" )
			{
				link = pSuperModel->getAction( linkName );
				if ( !link )
					success = false;
			}
			add( action, 0, NULL, savedAction->readBool( "promoteMotion", false ), link );
			action->finish_ = savedAction->readFloat( "finish", -1.f );
			action->passed_ = savedAction->readFloat( "passed", 0.f );
			action->played_ = savedAction->readFloat( "played", 0.f );
			action->lastPlayed_ = savedAction->readFloat( "lastPlayed", 0.f );
		}
		else 
			success = false;
	}

	tick( 0.f, pSuperModel, pMatrix );

	// Need to clear any cues after the tick
	Moo::CueShot::s_cueShots_.clear();

	debugTick( -1.f );
	return success;
}

/**
 *	This adds the specified action to the queue, to start after time
 *	afterTime. If responses has an initial NULL cue base pointer entry,
 *	then its function is called when the action completes
 *	(even if the action's animation is never displayed on screen),
 *	otherwise they are called when their identified cues are fired.
 *	If a link action is specified, and that action is still playing
 *	and hasn't blended out, then the afterTime field is ignored and
 *	the action is played immediately on completion of the link action.
 */
void ActionQueue::add( SuperModelActionPtr action, float afterTime,
	CueResponses * responses, bool promoteMotion, SuperModelActionPtr link )
{
	BW_GUARD;
	if (link)
	{
		AQElement * aqe = NULL;
		AQElement * haqe = NULL;

		// if there's a link action, see if we can find it in the
		// waiting list or in the playing list and it's not too late.
		for (uint i = 0; i < waiting_.size(); i++)
		{
			AQElement * look = &waiting_[i];
			while (look->link_ != NULL) { look = look->link_; }
			if (look->action_ == link)
			{
				haqe = &waiting_[i];
				aqe = look;
				break;
			}
		}
		if (aqe == NULL) for (uint i = 1; i < actionState_.size(); i++)
		{
			AQElement & head = actionState_[i];
			AQElement * look = &head;
			while (look->link_ != NULL) { look = look->link_; }

			// currently this only works for non-filler actions.
			// If it's a filler, then it'll start blending out
			// immediately when we add it below anyway.
			if (look->action_ == link && head.action_->passed_ <
				head.action_->finish_ - head.action_->pSource_->blendOutTime_ )
			{
				haqe = &head;
				aqe = look;
				break;
			}
		}

		// ok, link it in then
		if (aqe != NULL)
		{
			aqe->link_ = new AQElement(
				action, responses, promoteMotion );
			haqe->action_->finish_ = action->pSource_->filler_ ?
				-1.f : haqe->action_->finish_ + action->pFirstAnim_->duration_;
			return;
		}
	}

	// ok, add it to the list of animations waiting to start then
	AQElement	newAQE( action, responses, promoteMotion );
	action->passed_ = -afterTime;
	action->played_ = 0.f;
	action->finish_ = action->pSource_->filler_ ?
		-1.f : action->pFirstAnim_->duration_;
	action->lastPlayed_ = 0.f;

	waiting_.push_back( newAQE );

	MF_ASSERT_DEV( responses == NULL || !responses->empty() );

//	DEBUG_MSG( "Queuing action %s, blended %d, filler %d\n",
//		action.actionName().c_str(), action.blended(), action.filler() );
}


/**
 *	This action stops all instances of the input action on the queue,
 *	by starting to blend them out (excepting matched actions)
 */
void ActionQueue::stop( const ModelAction * pActionSource )
{
	BW_GUARD;
	for (uint i = 1; i < actionState_.size(); i++)
	{
		AQElement & aqe = actionState_[i];
		if (aqe.action_->pSource_ == pActionSource)
		{
			float newFinish = aqe.action_->passed_ +
				aqe.action_->pSource_->blendOutTime_;
			if (aqe.action_->finish_ < 0.f || newFinish < aqe.action_->finish_)
			{
				//DEBUG_MSG( "Setting finish on action '%s' to %g\n",
				//	aqe.action->actionName().c_str(), aqe.finish );
				aqe.action_->finish_ = newFinish;
			}
		}
	}

	// don't forget the waiting queue
	for (uint i=0; i < waiting_.size(); i++)
	{
		AQElement & aqe = waiting_[i];
		if (aqe.action_->pSource_ == pActionSource)
		{
			aqe.doCallback( 0.f );
			for (AQElement * look = aqe.link_; look != NULL; look = look->link_)
			{
				look->doCallback( 0.f );
			}
			waiting_.erase( waiting_.begin() + i );
			i--;
		}
	}
}

/**
 *	This function is required for ModelEditor at the moment
 * May be removed in 1.10
 */
void ActionQueue::getCurrentActions( BW::vector<SuperModelActionPtr>& actionsInUse )
{
	BW_GUARD;
	for (uint i = 1; i < actionState_.size(); i++)
	{
		AQElement & aqe = actionState_[i];
		actionsInUse.push_back( aqe.action_ );
	}
}

/**
 *	This function is called by the Action Matcher to set the
 *	action which it thinks should be played now
 *
 *	actionTime sets how much time the action queue should pretend
 *	has elapsed since the last frame, or negative for dTime
 *
 *	Assumption: This function is called before the 'tick' on the
 *	frame in which it is to happen.
 */
void ActionQueue::setMatch( SuperModelActionPtr action, float dTime,
	float actionTime, bool promoteMotion )
{
	BW_GUARD;
	AQElement	& matchState = actionState_[0];

	// assume we accept it
	lastMatchAccepted_ = true;

	// if there are primary actions or
	// if there is already an action on same track and
	// they are not finished playing
	// then matched action must be NULL
	for (uint i=1; i < actionState_.size(); i++)
	{
		AQElement	& aqe = actionState_[i];
		SuperModelActionPtr ia = aqe.action_;

		bool isPrimaryAction = ia->pSource_->track_ == 0;
		bool actionOnTrack = action != NULL && ia->pSource_->track_ != -1 &&
			ia->pSource_->track_ == action->pSource_->track_;
		bool ignoreMatchedAction = isPrimaryAction || actionOnTrack;
		bool actionPlaying = ia->finish_ < 0.f ||
							(ia->passed_ + dTime) < 
							(ia->finish_ - ia->pSource_->blendOutTime_);

		if (ignoreMatchedAction && actionPlaying)
		{
			action = NULL;
			actionTime = -1.f;
			lastMatchAccepted_ = false;
			//TRACE_MSG( "AQ::setMatch: Action made NULL\n" );
			break;
		}
	}

	// is this the same action as last time?
	if (action == matchState.action_)
	{			// note: assumes AM uses same set of SMAP's
		// record actionTime until we know dTime
		matchActionTime_ = actionTime;
		return;
	}

	//TRACE_MSG( "ActionQueue %lx - ActionMatcher chose a different action: %s\n", this,
	//	action ? action->pSource_->name_.c_str() : "None" );

	float	cycleParam = 0.f;

	// ok, they're different, so start blending out the old one
	if (matchState.action_)
	{
		// copy the cycle position of the old animation if it was movement
		if (matchState.action_->pSource_->isMovement_ &&
			matchState.action_->pFirstAnim_->looped_)
		{
			cycleParam = fmodf( matchState.action_->played_ /
				matchState.action_->pFirstAnim_->duration_, 1.0 );
		}

		matchState.action_->finish_ = matchState.action_->passed_ +
			matchState.action_->pSource_->blendOutTime_ *
			matchState.action_->blendRatio();

		// now whack it on the back
		AQElement	matchStateCopy = matchState;
		actionState_.push_back( matchStateCopy );
		TransformFashionPtr fashionCopy = transformFashions_[0];
		transformFashions_.push_back( fashionCopy );
		// the copies above are done 'coz the memory the references use
		// could be moved underneath them while they are added.
		// take-home lesson: Don't add to vectors using references to
		// elements in it!
	}

	// re-get the reference in case it moved above. eugh.
	AQElement & newMatchState = actionState_[0];

	// set up the new action
	newMatchState.action_ = action;
	if (action)
	{
		// calculate what point in the animation it should start at
		// (movement anims match cycles, others don't)
		float playedTime = action->pSource_->isMovement_ ?
			cycleParam * action->pFirstAnim_->duration_ : 0.f;

		float passedTime = 0.f;

		// see if this matched action is still playing somewhere else.
		// this will be the case if it's 'finish' time isn't -1, since
		// we set an action's finish time back to -1 when we erase it
		uint eraseIdx = 0;
		if (action->finish_ != -1)
		{
			// it's played for the same amount of time
			playedTime = action->played_;
			// whatever blend ratio it has now, that's how far we put it
			// from the beginning (to keep the blend ratio the same)
			passedTime = action->blendRatio() * action->pSource_->blendInTime_;

			// figure out which entry to erase from the vectors
			for (eraseIdx = 1; eraseIdx < actionState_.size(); eraseIdx++)
			{
				if (actionState_[eraseIdx].action_ == action) break;
			}
		}

		matchActionTime_ = actionTime;

		newMatchState.action_->played_ = playedTime;
		newMatchState.action_->passed_ = passedTime;
		newMatchState.action_->finish_ = -1.f;	// these are all like fillers
		newMatchState.action_->lastPlayed_ = playedTime;
		newMatchState.responses_ = NULL;
		newMatchState.promoteMotion_ = promoteMotion;
		transformFashions_[0] = action;

		// if the action was already blending out, then erase it here
		// (after we're done with the reference into the vector)
		if (eraseIdx > 0)
		{
			MF_ASSERT_DEV( eraseIdx < actionState_.size() &&
				actionState_[eraseIdx].responses_ == NULL );

			if( eraseIdx < actionState_.size() )
				actionState_.erase( actionState_.begin() + eraseIdx );

			if( eraseIdx < transformFashions_.size() )
			{
				transformFashions_.erase( transformFashions_.begin() + eraseIdx );
			}
		}
	}
	else
	{
		transformFashions_[0] = s_nullTransformFashion;
	}
}


/**
 *	This function returns the number of times the current matched
 *	action has played through. The noninteger part indicates how far
 *	through the current cycle playback has progressed.
 *
 *	Assumption: This function is called before the 'tick' on the
 *	frame in which it is to report.
 */
float ActionQueue::matchCycleCount()
{
	BW_GUARD;
	SuperModelActionPtr action = actionState_[0].action_;
	if (action)
	{
		return action->played_ / action->pFirstAnim_->duration_;
	}
	return 0;
}


/**
 *	This function updates the state of the action queue
 */
void ActionQueue::tick( float dTime, SuperModel* supermodel, MatrixLiaison* pMatrix )
{
	BW_GUARD;
	lastPromotion_.setZero();
	lastYawPromotion_ = Angle( 0 );

	// see if we need to mollify the world transform a bit
	// (only happens after promoted actions)
	if (needsMollifying_)
	{
		needsMollifying_ = this->mollifyWorldTransform( pMatrix );
	}

	// see if there's anyone due to start
	for (uint i = 0; i < waiting_.size(); i++)
	{
		AQElement & aqe = waiting_[i];

		// see if it's ready to go
		if (aqe.action_->passed_ + dTime >= 0)
		{
			// pretend it's played for that long too
			aqe.action_->played_ = aqe.action_->passed_;

			// if we're not on track -1, first stop any
			//  actions already playing on the same track
			if (aqe.action_->pSource_->track_ != -1)
			{
				int track = aqe.action_->pSource_->track_;
				for (uint j = 1; j < actionState_.size(); j++)
				{
					AQElement & jaqe = actionState_[j];
					if (jaqe.action_->pSource_->track_ == track)
					{
						float newFinish = jaqe.action_->passed_ +
							jaqe.action_->pSource_->blendOutTime_;
						float & oldFinish = jaqe.action_->finish_;
						if (oldFinish < 0.f || newFinish < oldFinish)
						{
							//DEBUG_MSG( "Setting finish on action '%s' to %g\n",
							//	jaqe.action->pSource_->name_.c_str(), newFinish );
							oldFinish = newFinish;
						}
					}
				}
			}

			//DEBUG_MSG( "Starting action '%s', finish %g\n",
			//	aqe.action->pSource_->name_.c_str(),
			//	aqe.action->finish_ );

			actionState_.push_back( aqe );
			transformFashions_.push_back( aqe.action_ );

			waiting_.erase( waiting_.begin() + i );
			i--;
		}
		else
		{
			// if the action is not ready to start yet, make sure we update 
			// passed time
			aqe.action_->passed_ += dTime;
		}
	}

	MF_ASSERT_DEV( Moo::CueShot::s_cueShots_.empty() );
	uint numCueDefaults = 0;

	// see if there's anyone due to stop, or move on to their next link
	for (uint i = 1; i < actionState_.size(); i++)
	{
		AQElement & aqe = actionState_[i];

		// move on the time passed and played
		aqe.action_->passed_ += dTime;
		aqe.action_->played_ += dTime;

		// move on the links while they're being used up
		while (aqe.link_ != NULL && aqe.action_->played_ >
			aqe.action_->pFirstAnim_->duration_)
		{
			// set up link
			float odur = aqe.action_->pFirstAnim_->duration_;
			aqe.link_->action_->passed_ = aqe.action_->passed_;
			aqe.link_->action_->played_ = aqe.action_->played_ - odur;
			aqe.link_->action_->finish_ = aqe.action_->finish_;
			aqe.link_->action_->lastPlayed_ = 0.f;

			//DEBUG_MSG( "Following link from action '%s' to '%s',\n"
			//	" passed %g, finish %g, played %g\n",
			//	aqe.action->pSource_->name_.c_str(),
			//	aqe.link->action->pSource_->name_.c_str(),
			//	aqe.action->passed_, aqe.action->finish_,
			//	aqe.action->played_ - odur );

			// end old action
			this->endPlayingAction( aqe, dTime, odur, supermodel, pMatrix );

			// ActionQueue::endPlayingAction ticks the 
			// animation, so trigger any queued cues.
			if (aqe.responses_ != NULL)
			{
				CueResponse::respond( numCueDefaults, *aqe.responses_, NULL );
			}
			size_t numCueShots = Moo::CueShot::s_cueShots_.size();
			MF_ASSERT( numCueShots <= UINT_MAX );
			numCueDefaults = ( uint )numCueShots;

			// note that finish could well be -1 here,
			// if the last action in the chain is a filler.

			// copy link over old action
			aqe.followLink();
			transformFashions_[i] = aqe.action_;
		}

		// see if we're getting rid of it now
		if (aqe.action_->finish_ >= 0.f && aqe.action_->finish_ < aqe.action_->passed_)
		{
			//DEBUG_MSG( "Stopping action '%s', finish %g\n",
			//	aqe.action->pSource_->name_.c_str(), aqe.action->finish_ );

			float overShot = (aqe.action_->passed_-aqe.action_->finish_);
			this->endPlayingAction( aqe, dTime,
			//	aqe.action->pFirstAnim_->duration_ );
				aqe.action_->played_ - overShot,
				supermodel, pMatrix );
			// the last argument above is the played time that the anim
			// should have stopped at, based on how far the passed time
			// overshot the finish time.

			// ActionQueue::endPlayingAction ticks the 
			// animation, so trigger any queued cues.
			if (aqe.responses_ != NULL)
			{
				CueResponse::respond( numCueDefaults, *aqe.responses_, NULL );
			}
			size_t numCueShots = Moo::CueShot::s_cueShots_.size();
			MF_ASSERT( numCueShots <= UINT_MAX );
			numCueDefaults = ( uint )numCueShots;

			// if there are any unused links (there shouldn't be) call them now
			for (AQElement * look = aqe.link_; look != NULL; look = look->link_)
			{
				look->doCallback( overShot );
			}

			// and erase the whole element
			actionState_.erase( actionState_.begin() + i );
			transformFashions_.erase( transformFashions_.begin() + i );
			i--;
		}
	}

	// see what's up with the matched action
	if (actionState_[0].action_)
	{
		AQElement	& matchState = actionState_[0];

		matchState.action_->passed_ += dTime;

		// are we fiddling with the playback rate?
		if (matchActionTime_ >= 0.f)
		{
			//matchState.timeOffset -= dTime - matchActionTime_;
			matchState.action_->played_ += matchActionTime_;
			matchActionTime_ = -1.f;
		}
		else
		{
			matchState.action_->played_ += dTime;
		}
	}

	// promote the motion of any actions that desire it
	for (uint i = 0; i < actionState_.size(); i++)
	{
		AQElement & aqe = actionState_[i];

		if (!aqe.action_ || !aqe.promoteMotion_) continue;

		this->tickPromotion( aqe, dTime, pMatrix );
	}

	// and call the internal tick on every action
	for (uint i = !actionState_[0].action_; i < actionState_.size(); i++)
	{
		AQElement & aqe = actionState_[i];
		aqe.action_->tick( *supermodel, dTime );

		// respond to cues in accordance with the wishes of this action
		if (aqe.responses_ != NULL)
			CueResponse::respond( numCueDefaults, *aqe.responses_, NULL );
		size_t numCueShots = Moo::CueShot::s_cueShots_.size();
		MF_ASSERT( numCueShots <= UINT_MAX );
		numCueDefaults = ( uint )numCueShots;
	}


}

/**
 * This tick is specific to pyModel. 
 */
void ActionQueue::tick( float dTime, PyModel *pModel )
{
	BW_GUARD;
	this->tick( dTime, pModel->pSuperModel(), pModel->matrixLiaison() );

	// respond to all the cues with the default responses
	// note that if there is CueResponse in the aqe and in the defaults
	// for the same CueBase, then both are called (but the aqe goes first)
	// I'm not sure if this is good or not, but it's certainly easier :)
	CueResponse::respond( 0, responses_, pModel );
	Moo::CueShot::s_cueShots_.clear();
}


/**
 *	This does all the stuff that needs to happen when a playing action ends.
 *	endat is the exact value of 'played' when the action completes
 *	(note, not necessarily the same as finish, especially now with links)
 *
 *	It is OK to call this method for a promoted action that should have played
 *	but never did (i.e. never called tickPromotion before), because even though
 *	played-dTime calculation in tickPromotion will be < 0, tickPromotion will
 *	push it back up to 0, just as if this were the first time it was called
 *	during normal playback, when it also wouldn't have been going since the
 *	exact beginning of the frame (i.e. started in the middle of the frame).
 */
void ActionQueue::endPlayingAction( AQElement & aqe, float dTime, float endat, SuperModel* supermodel, MatrixLiaison* pMatrix )
{
	BW_GUARD;
	// find out how much we overshot by
	float overShot = aqe.action_->played_ - endat;

	// tick it internally up until the end
	float rplay = aqe.action_->played_;
	aqe.action_->played_ = endat;
	aqe.action_->tick( *supermodel, dTime - overShot );
	aqe.action_->played_ = rplay;

	// call the completion callback (if present)
	aqe.doCallback( overShot );

	// if this is a promoted motion action and there's no more
	// waiting to run, make sure that it's standing straight
	if (aqe.promoteMotion_)
	{
		// but let it add its last gasp first
		aqe.action_->played_ -= overShot;
		this->tickPromotion( aqe, dTime - overShot, pMatrix );
		// note: tickPromotion works on 'played'

		//dprintf( "ePA: dTime %f, overShot %f, endat %f\n",
		//	dTime, overShot, endat );

		aqe.promoteMotion_ = 0;
		needsMollifying_ |= this->mollifyWorldTransform( pMatrix );
	}

	aqe.action_->finish_ = -1.f;
};


/**
 *	This function performs the tick side of promoted actions - it
 *	pulls their matrix up into the model's world transform.
 */
void ActionQueue::tickPromotion( AQElement & aqe, float dTime, MatrixLiaison* pMatrix )
{
	BW_GUARD;
	SuperModelActionPtr action = aqe.action_;

	// what proportion had we added last time?
	float lastProp = (action->played_ - dTime) / action->pFirstAnim_->duration_;
	if (lastProp < 0) lastProp = 0;

	// what proportion do we want to make it now?
	float thisProp = action->played_ / action->pFirstAnim_->duration_;
	if (thisProp < 0) thisProp = 0;

	// make sure one of them is less than 1.
	if (thisProp > 1 && lastProp > 1)
	{
		// note: despite this, promoting movement actions
		//  no longer works with looped actions,
		//  and the promoting impacting actions never did

		int lProp = int( min( thisProp, lastProp ) );
		thisProp -= lProp;
		lastProp -= lProp;
	}

	// get the world matrix
	Matrix	world( pMatrix ? pMatrix->getMatrix() : Matrix::identity );
	Vector3 oldWorldOffset( world.applyToOrigin() );
	Angle oldWorldYaw( world.yaw() );

	// get the promoting matrix
	//const Matrix & promoter =
	//	model_.initialItinerantContextInverse();
	// we assume this is identity for now. More must change
	//  (in supermodel impact calcs) if this ever becomes false.

	if (action->pSource_->isMovement_)
	{
		Matrix move = action->pFirstAnim_->flagFactorBit( 0 );
		move.invert();
		BlendTransform eble( move );
		Matrix	padd, nadd;

		BlendTransform pble;
		pble.blend( lastProp, eble );
		pble.output( padd );
		//padd.preMultiply( promoter );

		BlendTransform nble;
		nble.blend( thisProp, eble );
		nble.output( nadd );
		//nadd.preMultiply( promoter );

		/*
		// Transforms are already inverted 'coz that makes them easier to apply
		// in ActionQueue's effect, so we actually invert nadd here instead.
		nadd.invertFast();
		// Hmmm... not exactly sure what the consequences of inverting here
		// v.s. doing it earlier are .... I still can't 'see' these well

		// and change the model's world transform
		// (actually probably safe to change its local transform,
		//  but it'd just recalculate the world transform next time anyway)

		world.preMultiply( padd );
		world.preMultiply( nadd );
		*/
		padd.invert();
		nadd.postMultiply( padd );
		world.preMultiply( nadd );
	}
	else if (action->pSource_->isImpacting_)
	{
		// promoter was initialItinerant_Transform_Inverse here before,
		// and now it's the Context inverse, because it is now taken
		// out of the channel for us for the supermodel.
		Matrix	padd, nadd;

		padd[0][0] = lastProp * action->pFirstAnim_->duration_;
		action->pFirstAnim_->flagFactor( 4, padd );
		//padd.preMultiply( promoter );

		nadd[0][0] = thisProp * action->pFirstAnim_->duration_;
		action->pFirstAnim_->flagFactor( 4, nadd );
		//nadd.preMultiply( promoter );

		/*!
		float	lastFrame = lastProp * action->pFirstAnim_->duration_ * action.frameRate() +
			action.firstFrame();

		float	thisFrame = thisProp * action->pFirstAnim_->duration_ * action.frameRate() +
			action.firstFrame();

		const Moo::AnimationChannelPtr pIAC =
			action.animation()->itinerantRoot()->channel();

		Matrix	padd = pIAC->result( lastFrame );
		padd.preMultiply( promoter );

		Matrix	nadd = pIAC->result( thisFrame );
		nadd.preMultiply( promoter );
		*/

		padd.invert();
		nadd.postMultiply( padd );
		world.preMultiply( nadd );

		//const_cast<Vector3&>(world.applyToUnitAxisVector(0)).normalise();
		//const_cast<Vector3&>(world.applyToUnitAxisVector(1)).normalise();
		//const_cast<Vector3&>(world.applyToUnitAxisVector(2)).normalise();
	}

	lastPromotion_ += world.applyToOrigin() - oldWorldOffset;
	lastYawPromotion_ += Angle(world.yaw()) - oldWorldYaw;

	if ( pMatrix )
		pMatrix->setMatrix( world );
}


/**
 *	This method makes sure that the model's world transform looks 'ok'.
 *
 *	@return	Whether or not this function needs to be called again
 */
bool ActionQueue::mollifyWorldTransform( MatrixLiaison* pMatrix )
{
	BW_GUARD;
	// Make sure there aren't any promoted motion actions running
	for (uint i=0; i < actionState_.size(); i++)
	{
		if (actionState_[i].promoteMotion_) return false;
	}

	// And straighten ourselves up
	bool again = false;

	// Get the old world
	const Matrix oldWorld = pMatrix ? pMatrix->getMatrix() : Matrix::identity;
	Vector3	pos = oldWorld.applyToOrigin();
	float	yaw = atan2f( oldWorld.m[2][0], oldWorld.m[2][2] );

	// Make up a clean new world
	Matrix newWorld;
	newWorld.setTranslate( pos );
	newWorld.preRotateY( yaw );

	// Find their difference in quaternion terms
	Quaternion	oldRot( oldWorld );
	Quaternion	newRot( newWorld );
	Quaternion	delRot = oldRot;
	delRot.invert();
	delRot.preMultiply( newRot );

	// If the rotation is a decent size, slow it down
	delRot.minimise();
	float dangle = fabsf( acosf( Math::clamp(-1.0f, delRot.w, +1.0f) ) );
		// dangle is double the angle (in radians)

	static const float MAX_DANGLE = 0.005f;
	if (dangle > MAX_DANGLE)
	{
		Quaternion	identity(0,0,0,1);
		delRot.slerp( identity, delRot, MAX_DANGLE/dangle );

		Matrix	delWorld;
		delWorld.setRotate( delRot );

		newWorld.multiply( delWorld, oldWorld );

		again = true;
	}
	else 
	{
		Matrix  oldScale;
	    oldScale.setScale( oldWorld.applyToUnitAxisVector(0).length(),
				oldWorld.applyToUnitAxisVector(1).length(),
				oldWorld.applyToUnitAxisVector(2).length() );
		newWorld.preMultiply( oldScale );
 	}

	// And set the world transform to our clean new one
	if ( pMatrix )
		pMatrix->setMatrix( newWorld );

	return again;
}

/**
 *	This function effects the state of the action queue on its model.
 *
 *	It returns true if animated differently to last time
 */
bool ActionQueue::effect()
{
	BW_GUARD;
	// all we have to do here now is figure out if we have any animations
	uint istart = actionState_[0].action_ ? 0 : 1;
	return ( istart < actionState_.size() );
}

/**
 *	This private function creates the placeholder for the
 *	matched action in the state vectors.
 */
void ActionQueue::addMatchedActionPlaceholder()
{
	BW_GUARD;
	AQElement	matchAQE( NULL, NULL, false );
	actionState_.push_back( matchAQE );
	transformFashions_.push_back( s_nullTransformFashion );
}

/**
 *	This method sets the default response for the given cue identifier.
 *	Sets a python exception on error.
 */
bool ActionQueue::cue( const BW::string & identifier,
	SmartPointer<PyObject> function )
{
	BW_GUARD;
	// check the function
	if (function && !PyCallable_Check( &*function ))
	{
		PyErr_SetString( PyExc_TypeError,
			"PyModel.cue argument 2 must be Callable" );
		return false;
	}

	// check the identifier and get a pointer to the cue base
	Moo::CueBasePtr cb = Moo::CueBase::find( 'c', identifier.c_str() );
	if (!cb)
	{
		PyErr_Format( PyExc_ValueError, "PyModel.cue() "
			"Cue identifier '%s' is unknown.", identifier.c_str() );
		return false;
	}

	// see if we already have this cue base
	for (CueResponses::iterator it = responses_.begin();
		it != responses_.end();
		it++)
	{
		if (it->cueBase_ == cb)
		{
			if (function)
				it->function_ = function;
			else
				responses_.erase( it );
			return true;
		}
	}

	// ok, if we're not trying to clear it then add an entry
	if (function)
	{
		responses_.push_back( CueResponse( cb, &*function ) );
	}
	// else nothing to do if we are clearing it

	return true;
}


/**
 *	This method returns a python object describing the state of
 *	the queue. Currently it is intended for debugging use only...
 *	when the format stabilises it may be appropriate for general use.
 *
 *	To consider: Keep as tuple or make as list? Make whole semantics
 *	sequeuence-like? Have members of sequence as own special types?
 *	Much scope for general improvement in script interface like this.
 */
PyObject * ActionQueue::queueState() const
{
	BW_GUARD;
	uint ibegin = actionState_[0].action_ ? 0 : 1;
	MF_ASSERT( actionState_.size() <= UINT_MAX );
	uint iend = ( uint )actionState_.size();

	PyObject * pTuple = PyTuple_New( iend - ibegin );

	for (uint i = ibegin; i < iend; i++)
	{
		const AQElement & aqe = actionState_[i];
		PyTuple_SetItem( pTuple, i-ibegin, PyString_FromString(
			aqe.action_->pSource_->name_.c_str() ) );
	}

	return pTuple;
}




/**
 *	Debugging struct
 */
struct AQEltHistory
{
	AQEltHistory() { }
	AQEltHistory( SuperModelActionPtr smap, uint cidx ) :
		smap_( smap ),
		last_( 0 ),
		cidx_( cidx )
	{
		if (cidx_ == uint(-1)) cidx_ = s_cidx++;
	}

	SuperModelActionPtr		smap_;
	uint					last_;
	uint					cidx_;
	BW::vector<float>		blends_;

	static uint s_cidx;
};
uint AQEltHistory::s_cidx = 0;

static BW::vector<AQEltHistory>	s_aqhist;
static BW::vector<float>			s_dtimes;

/**
 *	Tick debug stuff
 */
void ActionQueue::debugTick( float dTime )
{
	BW_GUARD;	
#if ENABLE_ACTION_QUEUE_DEBUGGER
	if (dTime == -1.f || s_dtimes.size() > 32768)
	{
		s_aqhist.clear();
		s_dtimes.clear();
		return;
	}

	s_dtimes.push_back( dTime );
	MF_ASSERT( s_dtimes.size() <= UINT_MAX );
	uint dts = ( uint )s_dtimes.size();

	uint ibegin = actionState_[0].action_ ? 0 : 1;
	MF_ASSERT( actionState_.size() <= UINT_MAX );
	uint iend = ( uint )actionState_.size();

	for (uint i = ibegin; i < iend; i++)
	{
		const AQElement & aqe = actionState_[i];
		uint ecidx = -1;
		uint j;
		for (j = 0; j < s_aqhist.size(); j++)
		{
			if (s_aqhist[j].smap_ == aqe.action_ &&
				s_aqhist[j].last_ == dts-1) break;
			if (s_aqhist[j].smap_->pSource_ == aqe.action_->pSource_)
				ecidx = s_aqhist[j].cidx_;
			if (s_aqhist[j].last_ + 600 < dts)
			{
				s_aqhist.erase( s_aqhist.begin() + j );
				j--;
			}
		}
		if (j == s_aqhist.size())
		{
			s_aqhist.push_back( AQEltHistory( aqe.action_, ecidx ) );
		}
		s_aqhist[j].last_ = dts;
		s_aqhist[j].blends_.push_back( aqe.action_->blendRatio() );
		if (s_aqhist[j].blends_.size() > 1000)
		{
			s_aqhist[j].blends_.erase(
				s_aqhist[j].blends_.begin(),
				s_aqhist[j].blends_.begin() + 300 );
		}
	}
#endif // ENABLE_ACTION_QUEUE_DEBUGGER
}

static const float AQDEBUG_XSCALE = 0.4f;
static const float AQDEBUG_YSCALE = 0.5f;

static Moo::Colour s_lineColours[8] = {
	Moo::Colour( 1.f, 0.f, 0.f, 1.f ),
	Moo::Colour( 0.f, 1.f, 0.f, 1.f ),
	Moo::Colour( 0.f, 0.f, 1.f, 1.f ),
	Moo::Colour( 1.f, 0.f, 1.f, 1.f ),
	Moo::Colour( 1.f, 1.f, 0.f, 1.f ),
	Moo::Colour( 0.f, 1.f, 1.f, 1.f ),
	Moo::Colour( 1.f,0.5f, 0.f, 1.f ),
	Moo::Colour( 1.f, 1.f, 1.f, 1.f )
};


/**
 *	Draw debug stuff
 */
void ActionQueue::debugDraw()
{
	BW_GUARD;
	Vector2	lineseg[3];
	Moo::Colour colour;
	static FontPtr myfont = FontManager::instance().get( "default_medium.font" );

	Moo::Material::setVertexColour();
	for (uint i = 0; i < s_aqhist.size(); i++)
	{
		AQEltHistory & aqeh = s_aqhist[i];
		colour = s_lineColours[aqeh.cidx_&7];
		float x = 1.f;
		size_t dti = s_dtimes.size();
		while( dti > aqeh.last_ )
		{
			dti--;
			x -= s_dtimes[dti] * AQDEBUG_XSCALE;
		}
		if (x < -1.5f) continue;

		for (int j = int(aqeh.blends_.size())-1; j > 0; j--)
		{
			dti--;
			float nx = x - s_dtimes[dti] * AQDEBUG_XSCALE;

			lineseg[0].x = x;
			lineseg[0].y = aqeh.blends_[j] * AQDEBUG_YSCALE - 0.65f;
			lineseg[1].x = nx;
			lineseg[1].y = aqeh.blends_[j-1] * AQDEBUG_YSCALE - 0.65f;
			lineseg[2] = lineseg[1];

			Geometrics::drawLinesInClip( lineseg, 3, colour );

			x = nx;
			if (x < -1.f) break;
		}

		if (x < -1.f) x = -1.f;
		myfont->colour( Colour::getUint32FromNormalised( (Vector4&)colour ) );
		if (FontManager::instance().begin( *myfont ))
		{
			myfont->drawString( aqeh.smap_->pSource_->name_,
				int((x+1.f) * Moo::rc().screenWidth()/2),
				int(Moo::rc().screenHeight()*(0.60 + 0.03*(aqeh.cidx_ & 7))) );
			FontManager::instance().end();
		}
	}
}

/// This function flushes the queue
void ActionQueue::flush( bool includeDebug/* = false */)
{
	BW_GUARD;
	waiting_.clear();

	AQElements::iterator iter;
	for (iter = actionState_.begin(); iter != actionState_.end(); iter++)
	{
		if (iter->action_) iter->action_->finish_ = -1.f;
	}
	actionState_.clear();
	transformFashions_.clear();

	this->addMatchedActionPlaceholder();

#if ENABLE_ACTION_QUEUE_DEBUGGER
	if (includeDebug)
	{
		s_aqhist.clear();
		s_dtimes.clear();
	}
#endif//ENABLE_ACTION_QUEUE_DEBUGGER
}


// -----------------------------------------------------------------------------
// Section: ActionQueue::AQElement
// -----------------------------------------------------------------------------


/**
 *	This method calls the completion callback in an action queue element,
 *	if it has one.
 */
void ActionQueue::AQElement::doCallback( float missedBy )
{
	BW_GUARD;
	if (responses_ != NULL && !responses_->front().cueBase_ &&
		responses_->front().function_)
	{		// call the completion function ... later
		responses_->front().respond( missedBy, NULL );
		responses_->front().function_ = NULL;
	}
}


/**
 *	This method replaces an action with that of its link,
 *	more efficiently than the copy operator would.
 *	It assumes that the link is not NULL.
 */
void ActionQueue::AQElement::followLink()
{
	BW_GUARD;
	action_ = link_->action_;

	bw_safe_delete(responses_);
	responses_ = link_->responses_;
	link_->responses_ = NULL;

	promoteMotion_ = link_->promoteMotion_;

	AQElement * olink = link_;
	link_ = olink->link_;
	olink->link_ = NULL;

	// we have to clear some fields in the link so it doesn't delete them here
	bw_safe_delete(olink);
}


// -----------------------------------------------------------------------------
// Section: ActionQueuer
// -----------------------------------------------------------------------------


/// Constructor
ActionQueuer::ActionQueuer( SmartPointer<PyModel> model,
		SuperModelActionPtr action, SuperModelActionPtr link,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	model_( model ),
	action_( action ),
	link_( link )
{
}

/// Destructor
ActionQueuer::~ActionQueuer()
{
}

/*~ function ActionQueuer.__call__
 *
 *	By calling the ActionQueuer itself as a function, you initiate the action
 *	that the ActionQueuer refers to.
 *
 *	For example:
 *	@{
 *  # Start the walk action immediatly.
 *	self.model.Walk()
 *	# Start the climb action after a short delay, call onClimbFinished on
 *	# completion, and apply animation transforms to the model transform.
 *	self.model.Climb(0.5, self.onClimbFinished, 1)
 *	@}
 *
 *	@param afterTime		A float which is the time in seconds to delay before
 *						starting to play the action. Default is 0.
 *
 *	@param callBack		A script functor that will be called when the action
 *						finishes playing. Default is None.
 *
 *	@param promoteMotion	An integer specifying a boolean value (0 or 1) that
 *						determines whether the animation transforms are applied
 *						to the model's transform. Default is 0 (false).
 *
 *	@return				The ActionQueuer object on success, None on failure.
 */
/**
 *	This method is called when the function object is called from Python
 */
PyObject * ActionQueuer::pyCall( PyObject * args, PyObject * kwargs )
{
	BW_GUARD;
	if (!action_)
	{
		PyErr_SetString( PyExc_TypeError,
			"ActionQueuer continuation objects are not callable" );
		return NULL;
	}

	// make sure that the model is actually in the world,
	// 'coz it won't be much fun adding actions if its tick function
	// isn't going to be called to process them
	if (!model_->isInWorld())
	{
		PyErr_SetString( PyExc_EnvironmentError, "ActionQueuer op() "
			"cannot queue actions when our Model is not in the world" );
		return NULL;
	}


	float	afterTime = 0;
	PyObject * pCompletionCallback = NULL;
	int promoteMotion = action_->pSource_->isImpacting_;
	CueResponses * pResponses = NULL;

	bool good = PyArg_ParseTuple( args, "|fOi",
		&afterTime, &pCompletionCallback, &promoteMotion ) != 0;
	bool setError = false;

	// check the time
	if (good && afterTime < 0.f)
	{
		/*
		good = false;
		PyErr_SetString( PyExc_ValueError, "ActionQueuer op()"
			" called with negative aftertime" );
		setError = true;
		*/
		afterTime = -afterTime;
	}

	// check the completion callback
	if (good)
	{
		if (pCompletionCallback != Py_None && pCompletionCallback != NULL)
		{
			if (PyCallable_Check( pCompletionCallback ))
			{
				pResponses = new CueResponses();
				pResponses->push_back(CueResponse( NULL, pCompletionCallback ));
			}
			else good = false;
		}
	}

	// parse the cue callbacks
	if (good && kwargs != NULL)
	{
		Py_ssize_t pos = 0;
		PyObject * key, * value;
		while (PyDict_Next( kwargs, &pos, &key, &value ))
		{
			if (!PyString_Check( key ))			{ good = false; break; }
			if (!PyCallable_Check( value ))		{ good = false; break; }
			const char * cueStr = PyString_AsString( key );
			Moo::CueBasePtr cb = Moo::CueBase::find( 'c', cueStr );
			if (!cb)
			{
				good = false;
				PyErr_Format( PyExc_ValueError, "ActionQueuer() "
					"Cue identifier '%s' is unknown.", cueStr );
				setError = true;
				break;
			}
			if (pResponses == NULL) pResponses = new CueResponses();
			pResponses->push_back( CueResponse( cb, value ) );
		}
	}

	// get out now if things are not good
	if (!good)
	{
		if (!setError)
		{
			PyErr_SetString( PyExc_TypeError, "ActionQueuer() expects "
				"[float afterTime = 0[, Callable completionFunction = None"
				"[, bool promoteMotion = isImpacting]]][,cueName=function]*" );
		}

		bw_safe_delete(pResponses);
		return NULL;
	}

	// ok, finally add the action then
	model_->actionQueue().add(
		action_, afterTime, pResponses, promoteMotion != 0, link_ );

	return new ActionQueuer( model_, NULL, action_ );
}



/*~ function ActionQueuer.stop
 *
 *	This function stops all instances of the input action on the queue,
 *	by starting to blend them out (excepting matched actions)
 */
PyObject * ActionQueuer::py_stop( PyObject * pArgs )
{
	BW_GUARD;
	SuperModelActionPtr smap = this->daction();

	model_->actionQueue().stop( smap->pSource_ );

	Py_RETURN_NONE;
}


/// The python get attribue function
ScriptObject ActionQueuer::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	// if we're a continuation, look for an action of this name in the model
	if (!action_ && model_->pSuperModel() != NULL)
	{
		PyObject * pNewQueuer = this->action( attrObj.c_str(), false );
		if (pNewQueuer != (PyObject*)-1)
		{
			return ScriptObject( pNewQueuer, ScriptObject::FROM_NEW_REFERENCE );
		}
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}

/*~ function ActionQueuer.action
 *
 *	This function returns the specified action as an ActionQueuer object. If an action is found
 *	that matches the action name parameter then it is added into the ActionQueue. 
 *
 *	For example:
 *	@{
 *  # Start the walk action.
 *	self.model.action("Walk")()
 *	@}
 *
 *	@param aname The name of the action to return as an ActionQueuer object.
 *	@param errIfNotFound An optional bool value specifying whether to return an error,
 *						 if no action was found. Default value is True.
 *
 *	@return Returns an ActionQueuer object of the specified action.
 */
PyObject * ActionQueuer::action( const BW::string & aname, bool errIfNotFound )
{
	BW_GUARD;
	// make sure we haven't yet got an action
	if (action_)
	{
		PyErr_Format( PyExc_TypeError, "ActionQueuer.action() "
			"not available since action %s already selected",
			action_->pSource_->name_.c_str() );
		return NULL;
	}

	// make sure we have a model (bit dubious this check)
	if (model_->pSuperModel() == NULL)
	{
		PyErr_SetString( PyExc_TypeError, "ActionQueuer.action() "
			"not available since model is blank" );
		return NULL;
	}

	// now that we can, look up the action
	SuperModelActionPtr smap = model_->pSuperModel()->getAction( aname );
	if (!smap)
	{
		if (!errIfNotFound) return (PyObject*)-1;

		PyErr_Format( PyExc_ValueError, "ActionQueuer.action() "
			"no action %s on this model", aname.c_str() );
		return NULL;
	}

	// make sure their tracks are the same
	if (link_ && smap->pSource_->track_ != link_->pSource_->track_)
	{
		PyErr_Format( PyExc_AttributeError,
			"ActionQueuer.%s cannot continue from %s "
			"because it is on track %d and the earlier one is on %d",
			aname.c_str(), link_->pSource_->name_.c_str(),
			smap->pSource_->track_, link_->pSource_->track_ );
		return NULL;
	}

	// and return the new ActionQueuer object
	return new ActionQueuer( model_, smap, link_ );
}

/** 
 *	This method gets the action's displacement as a Vector3.
 *	It is accessible from python via the displacement attribute.
 *
 *	@return Returns the action's displacement as a Vector3.
 */
PyObject * ActionQueuer::pyGet_displacement()
{
	BW_GUARD;
	SuperModelActionPtr smap = this->daction();

	Vector3	ret( 0.f, 0.f, 0.f );
	if (smap->pSource_->isMovement_)
	{
		ret = model_->initialItinerantContextInverse().applyVector(
			smap->pFirstAnim_->flagFactorBit( 0 ).applyToOrigin() )*-1.f;
	}
	else if (smap->pSource_->isImpacting_)
	{
		Matrix imEnd;
		imEnd[0][0] = smap->pFirstAnim_->duration_;
		smap->pFirstAnim_->flagFactor( 4, imEnd );

		ret = model_->initialItinerantContextInverse().applyVector(
			imEnd.applyToOrigin() );
	}
	else if (smap->pSource_->isCoordinated_)
	{
		ret = model_->initialItinerantContextInverse().applyVector(
			smap->pFirstAnim_->flagFactorBit( 1 ).applyToOrigin() );
	}

	return Script::getReadOnlyData( ret );
}

/** 
 *	This method gets the position and yaw (in world space) that another model
 *	should be at, so that when this model plays this action, and the other model plays
 *	the coordinated action, it all synchronises	properly.  Returns a vector4,
 *	with the 4th element being yaw.
 *
 *	It is accessible from python via the rotation attribute.
 *
 *	@return Returns the action's rotation as a Vector4.
 */
PyObject * ActionQueuer::pyGet_rotation()
{
	BW_GUARD;
	SuperModelActionPtr smap = this->daction();

	Quaternion	q(0,0,0,1);

	if (smap->pSource_->isMovement_)
	{
		q = Quaternion( smap->pFirstAnim_->flagFactorBit( 0 ) );
	}
	else if (smap->pSource_->isImpacting_)
	{
		Matrix imEnd;
		imEnd[0][0] = smap->pFirstAnim_->duration_;
		smap->pFirstAnim_->flagFactor( 4, imEnd );

		q = Quaternion( imEnd );
	}
	else if (smap->pSource_->isCoordinated_)
	{
		q = Quaternion( smap->pFirstAnim_->flagFactorBit( 1 ) );
	}

	Vector3	ax( q.x, q.y, q.z );	ax.normalise();
	Vector4 ret( ax[0], ax[1], ax[2], 2 * acosf( Math::clamp(-1.0f, q.w, +1.0f) ) );
	return Script::getReadOnlyData( ret );
}


/*~ function ActionQueuer.getSeek
 *
 *	This method returns the seek position and yaw (in world space )for another
 *	entity wishing to co-ordinate using this action on the provided
 *	position and yaw with this action's counterpart on this model.
 *
 *	This method is different from the seek attribute.  The seek attribute
 *	specifies the position relative to this model.  This function returns
 *	the position relative to a specified postition.
 *
 *	@param posYaw	A Vector4 which is the position and yaw that this action will
 *  be played at
 *
 *	@return		A Vector4 which is the position and rotation that the
 *				corresponding action should be played at.
 */
/**
 *	This method returns the seek position for another entity
 *	wishing to co-ordinate using this action on the provided
 *	position with this action's counterpart on this model.
 */
PyObject * ActionQueuer::py_getSeek( PyObject * pArgs )
{
	BW_GUARD;
	Vector4 worldPos;
	int ret = Script::setData( pArgs, worldPos, "ActionQueuer.getSeek()" );
	if (ret != 0) return NULL;

	Matrix world;
	world.setRotateY( worldPos[3] );
	world.translation( reinterpret_cast<Vector3&>(worldPos) );

	return Script::getData( this->getSeek( world, false ) );
}


/*~ function ActionQueuer.Get_seek
 *
 *	This attribute returns the seek position for another entity
 *	wishing to co-ordinate using this action's counterpart on its
 *	own model with this action on this model.
 *
 *	@return		A Vector4 which is the position and rotation that the
 *				corresponding action should be played at.
 */
PyObject * ActionQueuer::pyGet_seek()
{
	BW_GUARD;
	return Script::getReadOnlyData(
		this->getSeek( model_->worldTransform(), false ) );
}


/*~ function ActionQueuer.getSeekInv
 *
 *	This method returns the seek position and yaw for another entity
 *	wishing to co-ordinate using this action's counterpart on
 *	this position and yaw with this action on its own model.
 *
 *	This method is different from the seekInv attribute.  The seekInv attribute
 *	specifies the position relative to this model.  This function returns
 *	the position relative to a specified postition.
 *
 *	@param posYaw	A Vector4 which is the position and yaw that the corresponding
 *				action will be played at
 *
 *	@return		A Vector4 which is the position and rotation that this action
 *				should be played at.
 */
/**
 *	This method returns the seek position for another entity
 *	wishing to co-ordinate using this action's counterpart on
 *	this position with this action on its own model.
 */
PyObject * ActionQueuer::py_getSeekInv( PyObject * pArgs )
{
	BW_GUARD;
	Vector4 worldPos;
	int ret = Script::setData( pArgs, worldPos, "ActionQueuer.getSeekInv()" );
	if (ret != 0) return NULL;

	Matrix world;
	world.setRotateY( worldPos[3] );
	world.translation( (Vector3&)worldPos );

	return Script::getData( this->getSeek( world, true ) );
}





/*~ function ActionQueuer.Get_seekInv
 *
 *	This attribute returns the seek position for another entity
 *	wishing to co-ordinate using this action on its own model with
 *	this action's counterpart on this model (got that?).
 *
 *	@return		A Vector4 which is the position and rotation that this action
 *				should be played at.
 */
PyObject * ActionQueuer::pyGet_seekInv()
{
	BW_GUARD;
	return Script::getReadOnlyData(
		this->getSeek( model_->worldTransform(), true ) );
}


/**
 *	Private function to handle seek gets
 *
 *	@see py_getSeek
 *	@see py_getSeekInv
 */
Vector4 ActionQueuer::getSeek( const Matrix & worldIn, bool invert )
{
	BW_GUARD;
	SuperModelActionPtr smap = this->daction();

	Matrix world( worldIn );

	Matrix	coord = smap->pFirstAnim_->flagFactorBit( 1 );
	/*
	coord.setRotate( smap->pSource_->coordRotation() );
	if (invert)
	{
		coord.invert();
		coord.postTranslateBy( smap->pSource_->coordDisplacement()*-1.f );
	}
	else
	{
		coord.preTranslateBy( smap->pSource_->coordDisplacement() );
	}
	*/
	if (invert) coord.invert();

	world.preMultiply( coord );

	return Vector4( world.m[3][0], world.m[3][1], world.m[3][2],
		atan2f( world.m[2][0], world.m[2][2] ) );
}



/** 
 *	This method returns what the final position of our
 *	entity would be if our action were played on it.
 *
 *	It is accessible via python as the impact attribute.
 *
 *	@return Returns final position of the entity as a Vector4.
 */
PyObject * ActionQueuer::pyGet_impact()
{
	BW_GUARD;
	SuperModelActionPtr smap = this->daction();

	Matrix world = model_->worldTransform();

	Matrix	impact;
	impact[0][0] = smap->pFirstAnim_->duration_;
	smap->pFirstAnim_->flagFactor( 4, impact );
/*
	impact.setRotate( smap->pSource_->impactRotation() );
	impact.invert();

	impact.postTranslateBy( smap->pSource_->impactDisplacement() * -1.f );
*/
	world.preMultiply( impact );

	Vector4 impaction( world.m[3][0], world.m[3][1], world.m[3][2],
		atan2f( world.m[2][0], world.m[2][2] ) );
	return Script::getReadOnlyData( impaction );
}







PY_TYPEOBJECT_WITH_CALL( ActionQueuer )

PY_BEGIN_METHODS( ActionQueuer )
	PY_METHOD_WITH_DOC( stop, 
		"This function stops all instances of the input action on the queue" )
	PY_METHOD_WITH_DOC( action, "This function returns the specified action"
				"as an ActionQueuer object." )
	PY_METHOD_WITH_DOC( getSeek,
		"This method returns the seek position for another entity wishing to "
		"co-ordinate using this action on the provided position with this "
		"action's counterpart on this model." )
	PY_METHOD_WITH_DOC( getSeekInv,
		"This method returns the seek position for another entity"
		"wishing to co-ordinate using this action's counterpart on"
		"this position with this action on its own model." )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ActionQueuer )
	/*~ attribute ActionQueuer.duration
	 *
	 *	The time it takes to play the action (measured in seconds).  There are
	 *	a few caveats to this:
	 *
	 *	- If the &lt;ScalePlaybackSpeed> flag is set in the .model file for
	 *	this action, then it may play faster or slower, depending on how
	 *	fast the model is moving.  If a model moves over the duration of its
	 *	animation, it has a 'natural' speed which is the distance it covers,
	 *	divided by the duration.  If the model's in game velocity is different
	 *	from this, then the playback rate of the animation is scaled
	 *	accordingly, to keep it synchronised with the movement rate.  This
	 *	changing of the playback rate changes the duration.
	 *
	 *	- The blend in and blend out times are part of the duration of the
	 *	action.  If the action is short, in particular, shorter than the blend
	 *	times, the action will start to blend out from the moment it starts
	 *	blending in, resulting in the action never being fully blended in.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( duration )
	/*~ attribute ActionQueuer.blendInTime
	 *
	 *	The time, in seconds, that this action will take to blend in.  That is,
	 *	it will start playing blendInTime before the previous action has
	 *	finished, and gradually blend from the first to the second.  In order
	 *	for the blending to be smooth, it is important that the blendOutTime of
	 *	the previous action be the same as the blend in time of this action.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( blendInTime )
	/*~ attribute ActionQueuer.blendOutTime
	 *
	 *	The time, in seconds, that this action will take to blend out.  The
	 *	system will start playing the next animation blendOutTime before this
	 *	animation ends. In order for the blending to be smooth, it is fairly
	 *	important that the blendOutTime of the previous action be the same as
	 *	the blend in time of this action.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( blendOutTime )
	/*~ attribute ActionQueuer.track
	 *
	 *	Which track the action plays on.  Tracks are integers from -1 up,
	 *	with -1 having a special status.  When an action is played on a
	 *	particular track, it stops whatever action was playing on the track
	 *	before (blending the other one out, and itself in).  When an action is
	 *	played on track -1, it doesn't interfere with the playing of any
	 *	actions.  Actions which are playing concurrently on different tracks
	 *	are weighted together using the alpha values for each node in each
	 *	animation.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Integer
	 */
	PY_ATTRIBUTE( track )
	/*~ attribute ActionQueuer.filler
	 *
	 *	If this is set to non-zero then this action is available for the
	 *	ActionMatcher to choose.  See the ActionMatcher documentation for a
	 *	description of how it chooses which action to play.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type Read-Only Integer
	 */
	PY_ATTRIBUTE( filler )
	/*~ attribute ActionQueuer.blended
	 *
	 *	If this is set to non-zero, then this action will be played on track
	 *	-1.  The track system replaces the use of the blended attribute.
	 *
	 *	@type Read-Only Integer
	 */
	PY_ATTRIBUTE( blended )
	/*~ attribute ActionQueuer.displacement
	 *
	 *	The distance and direction that the model moves between the first and
	 *	last frame of this action, measured relative to the models initial
	 *	frame of reference.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type Read-Only Vector3
	 */
	PY_ATTRIBUTE( displacement )
	/*~ attribute ActionQueuer.rotation
	 *
	 *	The quarternion representing the rotation of the model between the
	 *	first and last frames of this action.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Vector4
	 */
	PY_ATTRIBUTE( rotation )
	/*~ attribute ActionQueuer.seek
	 *
	 *	This attribute is the position and yaw (in world space) that another model
	 *	should be at,
	 *	so that when this model plays this action, and the other model plays
	 *	the coordinated action, it all synchronises	properly.  It is a vector4,
	 *	with the 4th element being yaw.
	 *
	 *	@type	Read-Only Vector4
	 */
	PY_ATTRIBUTE( seek )
	/*~ attribute ActionQueuer.seekInv
	 *
	 *	This attribute is the position and yaw (in world space)that another model
	 *	should be at,
	 *	so that when the other model plays this action, and this model plays
	 *	the coordinated action, it all synchronises properly.  It is a vector4,
	 *	with the 4th element being yaw.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Vector4
	 */
	PY_ATTRIBUTE( seekInv )
	/*~ attribute ActionQueuer.impact
	 *
	 *	This atribute is the position that the model will end up at, in world
	 *	coordinates, after playing this action.  This is the current models
	 *	current position in world coordinates, plus the displacement attribute
	 *	of this action converted to world coordinates.
	 *
	 *	This attribute is read in from the model file.
	 *
	 *	@type	Read-Only Vector3
	 */
	PY_ATTRIBUTE( impact )
PY_END_ATTRIBUTES()

BW_END_NAMESPACE

// action_queue.cpp
