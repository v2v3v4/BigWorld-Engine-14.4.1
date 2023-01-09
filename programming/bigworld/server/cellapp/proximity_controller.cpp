#include "script/first_include.hpp"

#include "proximity_controller.hpp"

#include "cellapp.hpp"
#include "entity.hpp"
#include "entity_range_list_node.hpp"
#include "range_trigger.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/**
 *	This class is used for proximity range triggers or traps. When an entity
 *	crosses within a range of the source entity it will call the Python script
 *	method onEnterTrap and when an entity leaves the range, it will call the
 *	method onLeaveTrap.
 */
class ProximityRangeTrigger : public RangeTrigger
{
public:
	ProximityRangeTrigger( Entity & around,
			float range, ControllerID id, int userArg ) :
		RangeTrigger( around.pRangeListNode(), range,
				/* lowerWants: */ RangeListNode::FLAG_ENTITY_TRIGGER,
				/* upperWants: */ RangeListNode::FLAG_ENTITY_TRIGGER,
				/* lowerMakes: */ RangeListNode::FLAG_NO_TRIGGERS,
				/* upperMakes: */ RangeListNode::FLAG_NO_TRIGGERS ),
		id_( id ),
		pAlternateHandler_( NULL ),
		inited_( false ),
		userArg_( userArg )
	{
	}
	~ProximityRangeTrigger()
	{
		this->pEntity()->delTrigger( this );
		this->remove();
	}

	void init()
	{
		if (!inited_)
		{
			inited_ = true;
			this->insert();
			this->pEntity()->addTrigger( this );
		}
	}

	void setRangeAndMod( float r )
	{
		this->setRange( r );
		this->pEntity()->modTrigger( this );
	}

	virtual BW::string debugString() const;

	virtual void triggerEnter( Entity & entity );
	virtual void triggerLeave( Entity & entity );
	virtual Entity * pEntity() const { return
		EntityRangeListNode::getEntity( this->pCentralNode() ); }

	void standardEnter( Entity * pWho );
	void standardLeave( Entity * pWho );
	void nonstandardLeave( EntityID who );

	/**
	 *	This interface is implemented by classes that handle the enter and leave
	 *	events.
	 */
	class Handler
	{
	public:
		virtual ~Handler() {};

		virtual void handle( Entity * pEntity, bool isEnter ) = 0;
		virtual void attached( ProximityRangeTrigger * pTo ) {}
	};

	void pAlternateHandler( Handler * pHandler )
	{
		pAlternateHandler_ = pHandler;
	}

private:
	ControllerID id_;
	Handler * pAlternateHandler_;
	bool inited_;
	int userArg_;
};


/*~ function Entity addProximity
 *  @components{ cell }
 *	The addProximity function creates a trap which notifies this Entity whenever
 *	other entities enter or exit an area which surrounds it. This area is a
 *	square (done for efficiency). Another entity is deemed to be within this
 *	range if it is within a given distance on both the x and z world axis. The
 *	Entity is notified via calls to its onEnterTrap and onLeaveTrap functions,
 *	which can be defined as follows:
 *
 *	@{
 *		def onEnterTrap( self, entityEntering, range, controllerID, userArg = 0 ):
 *		def onLeaveTrap( self, entityLeaving, range, controllerID, userArg = 0 ):
 *	@}
 *	There is also a rare case where a message is triggered when the entity
 *	is onloaded to a new cell and a "trapped" entity which was present before
 *	is not present on the new cell. In this situation, getting a reference to 
 *	the trapped entity object is not possible - only the entity ID is available.
 *
 *	@{
 *		def onLeaveTrapID( self, entityID, range, controllerID, userArg = 0 ):
 *	@}
 *
 *	Since the trap is a controller, use Entity.cancel with the controller ID 
 *	to delete the trap.
 *
 *	Note that it is possible for the callbacks to be triggered immediately,
 *	even before the addProximity() call has returned.
 *
 *  @see cancel
 *
 *  @param range range is a float which gives the new size of the trap area. The
 *  range must be greater than or equal to 0.
 *  @param userArg userArg is an optional integer common to all controllers.
 *  This is only passed to the callbacks if the value is not 0. It is
 *	recommended to set a default value of 0 for userArg in the callback
 *	prototypes.
 *  @return addProximity returns the id of the created controller.
*/
IMPLEMENT_CONTROLLER_TYPE_WITH_PY_FACTORY( ProximityController, DOMAIN_REAL )

Controller::FactoryFnRet ProximityController::New(
	float range, int userArg )
{
	if( range <= 0.f )
	{
		PyErr_SetString( PyExc_AttributeError,
			"Can't add proximity controller 0 or negative range" );
		return NULL;
	}

	return FactoryFnRet( new ProximityController( range ), userArg );
}


/**
 * 	ProximityController constructor
 *
 *	@param range This is the range of the proximity check
 */
ProximityController::ProximityController( float range ):
	range_( range ),
	pProximityTrigger_( NULL ),
	pOnloadedSet_( NULL )
{
}


/**
 *	Destructor.
 */
ProximityController::~ProximityController()
{
}


namespace
{
class OnloadHandler : public ProximityRangeTrigger::Handler
{
public:
	OnloadHandler( ProximityRangeTrigger * pTrigger,
			BW::vector< EntityID > & onloadSet ) :
		pTrigger_( pTrigger ),
		set_( onloadSet ),
		matched_( onloadSet.size() )
	{
	}

	virtual ~OnloadHandler()
	{
		for (size_t i = 0; i < matched_.size(); ++i)
		{
			if (!matched_[i])
			{
				Entity * pEntity =
					CellApp::instance().findEntity( set_[i] );

				if (pEntity)
				{
					pTrigger_->standardLeave( pEntity );
				}
				else
				{
					pTrigger_->nonstandardLeave( set_[i] );
				}
			}
		}
	}

private:
	void handle( Entity * pEntity, bool isEnter )
	{
		MF_ASSERT( isEnter );

		BW::vector< EntityID >::iterator iter =
			std::find( set_.begin(), set_.end(), pEntity->id() );

		if (iter != set_.end())
		{
			int index = iter - set_.begin();
			matched_[ index ] = true;
		}
		else
		{
			pTrigger_->standardEnter( pEntity );
		}
	}

	ProximityRangeTrigger * pTrigger_;
	BW::vector< EntityID > & set_;
	BW::vector< bool > matched_;
};
}


/**
 *	This method overrides the Controller method.
 */
void ProximityController::startReal( bool isInitialStart )
{
	if (!isInitialStart && (pOnloadedSet_ != NULL))
	{
		// keep a reference to ourselves in case we are cancelled by a
		// script callback at the close of the scope below
		ControllerPtr pThis = this;

		pProximityTrigger_ = new ProximityRangeTrigger( this->entity(),
				range_, this->id(), this->userArg() );

		// This scope is important. The handler's destructor must be called
		// before deleting pOnloadedSet changes.
		{
			OnloadHandler handler( pProximityTrigger_, *pOnloadedSet_ );
			pProximityTrigger_->pAlternateHandler( &handler );
			pProximityTrigger_->init();
			pProximityTrigger_->pAlternateHandler( NULL );
		}
		// we might now be cancelled (but not deleted due to smart pointer)
		delete pOnloadedSet_;
		pOnloadedSet_ = NULL;
	}
	else
	{
		this->setRange( range_ );
		// we might now be deleted
	}
}


namespace
{
class NoOpHandler : public ProximityRangeTrigger::Handler
{
public:
	void handle( Entity * pEntity, bool isEnter )
	{
		MF_ASSERT( !isEnter );
	}
};
}


/**
 *	This method overrides the Controller method.
 */
void ProximityController::stopReal( bool isFinalStop )
{
	NoOpHandler handler;

	if (pProximityTrigger_ != NULL)
	{
		// TODO: Could just remove triggers in this case instead of
		// shuffling them back to range 0 and ignoring the results.
		pProximityTrigger_->pAlternateHandler( &handler );
	}

	this->setRange( 0.f );
	// we might now be deleted
}


/**
 *	This method write our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void ProximityController::writeRealToStream( BinaryOStream & stream )
{
	this->Controller::writeRealToStream( stream );
	stream << range_;

	// TODO: It may be an idea to support proximity controllers that are only
	// triggered by a subset of entities (say by entity type).

	// Add all of the entities in our proximity.
	RangeListNode * pOurNode = pProximityTrigger_->pCentralNode();
	RangeListNode * pCurr = pProximityTrigger_->pLowerTrigger()->nextX();
	const RangeListNode * pEnd = pProximityTrigger_->pUpperTrigger();

	int32 count = 0;

	while (pCurr != pEnd)
	{
		if (pCurr->isEntity() &&
			pProximityTrigger_->containsInZ( pCurr ) &&
			pCurr != pOurNode)
		{
			++count;
		}

		pCurr = pCurr->nextX();
	}

	stream << count;

	pCurr = pProximityTrigger_->pLowerTrigger()->nextX();

	while ((pCurr != pEnd) && (count > 0))
	{
		if (pCurr->isEntity() &&
			pProximityTrigger_->containsInZ( pCurr ) &&
			pCurr != pOurNode)
		{
			Entity * pEntity = EntityRangeListNode::getEntity( pCurr );
			stream << pEntity->id();
			--count;
		}

		pCurr = pCurr->nextX();
	}

	MF_ASSERT( count == 0 );
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool ProximityController::readRealFromStream( BinaryIStream & stream )
{
	this->Controller::readRealFromStream( stream );
	stream >> range_;
	int count;
	stream >> count;

	if (count > 0)
	{
		pOnloadedSet_ = new BW::vector< EntityID >( count );

		for (int i = 0; i < count; ++i)
		{
			stream >> (*pOnloadedSet_)[i];
		}

		std::sort( pOnloadedSet_->begin(), pOnloadedSet_->end() );
	}

	return true;
}


/**
 *	Set the range of proximity. Note: We may be cancelled or deleted when
 *	this function returns.
 */
void ProximityController::setRange( float range )
{
	range_ = range;

	// all three of the cases below can call script
	Entity::callbacksPermitted( false );

	START_PROFILE( SHUFFLE_TRIGGERS_PROFILE );
	if (range <= 0.f)
	{
		delete pProximityTrigger_;	// could be NULL
		pProximityTrigger_ = NULL;
		range_ = 0.f;
	}
	else if (pProximityTrigger_ == NULL)
	{
		pProximityTrigger_ = new ProximityRangeTrigger( this->entity(),
				range, this->id(), this->userArg() );
		pProximityTrigger_->init();
	}
	else
	{
		pProximityTrigger_->setRangeAndMod( range );
	}
	STOP_PROFILE( SHUFFLE_TRIGGERS_PROFILE );

	Entity::callbacksPermitted( true );
	// we might now be cancelled or deleted
}


// -----------------------------------------------------------------------------
// Section: ProximityRangeTrigger
// -----------------------------------------------------------------------------

/**
 *	This method returns the identifier for the RangeTriggerNode.
 *
 *	@return the string identifier of the node
 */
BW::string ProximityRangeTrigger::debugString() const
{
	char buf[80];
	bw_snprintf( buf, sizeof(buf), "%d for script", this->pEntity()->id() );

	return buf;
}


/**
 *	This method is called when an entity triggers the node. It forwards the call
 *	to the entity script method onEnterTrap.
 *
 * @param entity	The entity that triggered this proximity trigger.
 */
void ProximityRangeTrigger::triggerEnter( Entity & entity )
{
	Entity * pThis = this->pEntity();
	Entity * pWho = &entity;

	if (pThis->isDestroyed())
	{
		// TODO: This is not really an error but we may want to look at handling
		// these differently.
		// ERROR_MSG( "ProximityRangeTrigger::triggerEnter: "
		//			"Called on destroyed entity %d\n", pThis->id() );
		return;
	}

	if (pAlternateHandler_ != NULL)
	{
		pAlternateHandler_->handle( pWho, true );
	}
	else
	{
		this->standardEnter( pWho );
	}
}


/*~ callback Entity.onEnterTrap
 *  @components{ cell }
 *	This method is associated with the Entity.addProximity method. It is called
 *	when an entity enters a proximity trap of this entity.
 *	@param entity		The entity that has entered.
 *	@param range		The range of the trigger.
 *	@param controllerID	The id of the proximity trap controller.
 *	@param userArg		If userArg is not 0, this is also passed to the
 *		callback. The prototype should assign a default value of 0 if it is
 *		possible that addProximity() can be called with a userArg value of 0.
 */
/**
 *	This method handles the case where an entity has entered our proximity.
 */
void ProximityRangeTrigger::standardEnter( Entity * pWho )
{
	PyObject * pArgs = (userArg_ != 0) ?
		Py_BuildValue( "(Ofii)", pWho, this->range(), id_, userArg_ ) :
		Py_BuildValue( "(Ofi)", pWho, this->range(), id_ );

	this->pEntity()->callback( "onEnterTrap", pArgs,
		"RealEntity::triggerEnter: ", false );
}


/**
 *	This method is called when an entity untriggers the node. It forwards the
 *	call to the entity script method onLeaveTrap.
 *
 * @param entity	The entity that left the proximity trigger.
 */
void ProximityRangeTrigger::triggerLeave( Entity & entity )
{
	Entity * pThis = this->pEntity();
	Entity * pWho = &entity;

	if (pThis->isDestroyed())
	{
		// TODO: This is not really an error but we may want to look at handling
		// these differently.
		// ERROR_MSG( "ProximityRangeTrigger::triggerLeave: "
		//			"Called on destroyed entity %d\n", pThis->id() );
		return;
	}

	if (pAlternateHandler_ != NULL)
	{
		pAlternateHandler_->handle( pWho, false );
	}
	else
	{
		this->standardLeave( pWho );
	}
}

/*~ callback Entity.onLeaveTrap
 *  @components{ cell }
 *	This method is associated with the Entity.addProximity method. It is called
 *	when an entity leaves a proximity trap of this entity.
 *	@param entity		The entity that has left.
 *	@param range		The range of the trigger.
 *	@param controllerID	The id of the proximity trap controller.
 *	@param userArg		If userArg is not 0, this is also passed to the
 *		callback. The prototype should assign a default value of 0 if it is
 *		possible that addProximity() can be called with a userArg value of 0.
 */
/**
 *	This method handles the case where an entity has left our proximity.
 */
void ProximityRangeTrigger::standardLeave( Entity * pWho )
{
	PyObject * pArgs = (userArg_ != 0) ?
		Py_BuildValue( "(Ofii)", pWho, this->range(), id_, userArg_ ) :
		Py_BuildValue( "(Ofi)", pWho, this->range(), id_ );

	this->pEntity()->callback( "onLeaveTrap", pArgs,
		"RealEntity::triggerLeave: ", true );
}

/*~ callback Entity.onLeaveTrapID
 *  @components{ cell }
 *	This method is associated with the Entity.addProximity method. It is called
 *	when an entity leaves a proximity trap of this entity. Getting this method
 *	called should be rare. It can only occur if this entity is onloaded to a new
 *	cell that does not have a currently trapped entity.
 *	@param entityID		The id of the entity that has left.
 *	@param range		The range of the trigger.
 *	@param controllerID	The id of the proximity trap controller.
 *	@param userArg		If userArg is not 0, this is also passed to the
 *		callback. The prototype should assign a default value of 0 if it is
 *		possible that addProximity() can be called with a userArg value of 0.
 */
/**
 *	This method handles the case where we know that an entity has left our
 *	proximity but we cannot get a pointer to it.
 */
void ProximityRangeTrigger::nonstandardLeave( EntityID who )
{
	PyObject * pArgs = (userArg_ != 0) ?
		Py_BuildValue( "(ifii)", who, this->range(), id_, userArg_ ) :
		Py_BuildValue( "(ifi)", who, this->range(), id_ );

	// TODO: This is pretty ugly and we should get rid of it somehow. We should
	// at least document it well.
	this->pEntity()->callback( "onLeaveTrapID", pArgs,
		"RealEntity::triggerLeave: ", true );
}

BW_END_NAMESPACE

// proximity_controller.cpp
