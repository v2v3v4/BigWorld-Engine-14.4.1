#include "script/first_include.hpp"

#include "controllers.hpp"

#include "cellapp_config.hpp"
#include "entity.hpp"
#include "real_entity.hpp"

#include "cstdmf/watcher.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Controllers::Controllers() : lastAllocatedID_( 0 )
{
}


/**
 *	Destructor.
 */
Controllers::~Controllers()
{
	// Get rid of any controllers
	for (Container::iterator it = container_.begin();
		it != container_.end();
		it++)
	{
		MF_ASSERT( it->second->domain() & DOMAIN_GHOST );
		it->second->stopGhost();
		it->second->disowned();
	}
}


// -----------------------------------------------------------------------------
// Section: Streaming
// -----------------------------------------------------------------------------

/**
 *	This method reads ghost controllers from a stream.
 */
void Controllers::readGhostsFromStream( BinaryIStream & data, Entity * pEntity )
{
	ControllerID numControllers;
	ControllerType controllerType;
	ControllerPtr pController;
	ControllerID cid;
	int userArg;

	data >> numControllers;

	for (ControllerID c = 0; c < numControllers; c++)
	{
		data >> cid >> controllerType >> userArg;

		pController = Controller::create( controllerType );
		pController->init( pEntity, cid, userArg );

		MF_ASSERT( pController->domain() & DOMAIN_GHOST );

		container_.insert( Container::value_type( cid, pController ) );

		pController->readGhostFromStream( data );

		pController->startGhost();
	}
}



/**
 *	This method reads real controllers from a stream.
 */
void Controllers::readRealsFromStream( BinaryIStream & data, Entity * pEntity )
{
	data >> lastAllocatedID_;

	// Now create any controllers
	ControllerID numOffloaded;
	ControllerType controllerType;
	ControllerPtr pController;
	ControllerID cid;
	int userArg;

	data >> numOffloaded;

	for (ControllerID c = 0; c < numOffloaded; c++)
	{
		data >> cid;
		Container::const_iterator found = container_.find( cid );
		if (found != container_.end())
		{
			pController = found->second;
			MF_ASSERT( pController->isAttached() );
			MF_ASSERT( pController->domain() & DOMAIN_GHOST );
		}
		else
		{
			data >> controllerType >> userArg;
			pController = Controller::create( controllerType );
			pController->init( pEntity, cid, userArg );

			MF_ASSERT( !(pController->domain() & DOMAIN_GHOST) );

			container_.insert( Container::value_type(
				cid, pController ) );
		}

		pController->readRealFromStream( data );

		// Now called later with a call to startRealControllers. This is so that
		// the call is after the entity becomes real. (i.e. has pReal_ set).
		// pController->startReal( false /* Not initial */ );
	}
}


/**
 *	This method writes ghost controllers to a stream.
 */
void Controllers::writeGhostsToStream( BinaryOStream & data )
{
	// Note: We do this in two passes because the passed in BinaryOStream data
	// can be a MemoryOStream, and it is not safe to reserve space for
	// numControllers and then stream the controller data, as this can
	// invalidate the reserved pointer.

	// And do dynamic controllers that operate on ghosts

	ControllerID numControllers = 0;
	for (Container::const_iterator it = container_.begin();
			it != container_.end();
			++it)
	{
		ControllerPtr pController = it->second;
		ControllerDomain cd = pController->domain();
		if (cd & DOMAIN_GHOST)
		{
			++numControllers;
		}
	}
	data << numControllers;

	for (Container::const_iterator it = container_.begin();
			it != container_.end();
			++it)
	{
		ControllerPtr pController = it->second;
		ControllerDomain cd = pController->domain();
		if (cd & DOMAIN_GHOST)
		{
			data << pController->id() << pController->type() <<
				pController->userArg();

			it->second->writeGhostToStream( data );

		}
	}

}


/**
 *	This method writes real controllers to a stream.
 */
void Controllers::writeRealsToStream( BinaryOStream & data )
{
	data << lastAllocatedID_;

	// Note: We do this in two passes because the passed in BinaryOStream data
	// can be a MemoryOStream, and it is not safe to reserve space for
	// numControllers and then stream the controller data, as this can
	// invalidate the reserved pointer.

	// Offload our controllers
	ControllerID numOffloaded = 0;
	for (Container::const_iterator iter = container_.begin();
			iter != container_.end();
			++iter)
	{
		ControllerPtr pController = iter->second;
		ControllerDomain cd = pController->domain();
		if (cd & DOMAIN_REAL)
		{
			++numOffloaded;
		}
	}

	data << numOffloaded;

	for (Container::const_iterator iter = container_.begin();
			iter != container_.end();
			++iter)
	{
		ControllerPtr pController = iter->second;
		ControllerDomain cd = pController->domain();
		if (cd & DOMAIN_REAL)
		{
			data << pController->id();
			if (!(cd & DOMAIN_GHOST))
			{
				data << pController->type() << pController->userArg();
			}

			pController->writeRealToStream( data );
		}
	}
}


/**
 *	This method handles a message to create a ghost entity.
 */
void Controllers::createGhost( BinaryIStream & data, Entity * pEntity )
{
	ControllerID cid;
	ControllerType ctype;
	data >> cid >> ctype;
	int userArg;
	data >> userArg;
	ControllerPtr pController = Controller::create( ctype );
	pController->init( pEntity, cid, userArg );

	container_.insert( Container::value_type(
		pController->id(), pController ) );

	pController->readGhostFromStream( data );

	pController->startGhost();
}


/**
 *	This method handles a message to delete a ghost entity.
 */
void Controllers::deleteGhost( BinaryIStream & data, Entity * pEntity )
{
	ControllerID cid;
	data >> cid;
	this->delController( cid, pEntity, true );
}


/**
 *	This method handles a message to update a ghost controller.
 */
void Controllers::updateGhost( BinaryIStream & data )
{
	ControllerID cid;
	data >> cid;

	Container::iterator found = container_.find( cid );
	MF_ASSERT( found != container_.end())

	found->second->readGhostFromStream( data );
}


// -----------------------------------------------------------------------------
// Section: Modification
// -----------------------------------------------------------------------------

/**
 *	This method adds a controller to the collection.
 */
ControllerID Controllers::addController(
		ControllerPtr pController, int userArg, Entity * pEntity )
{
	int count = container_.size();
	if (count >= CellAppConfig::absoluteMaxControllers())
	{
		ERROR_MSG( "Controllers::addController(%u): "
				"Too many controllers on this entity, #controllers = %d "
				"type=%s-%d, userArg = %d\n",
			pEntity->id(), count, pController->typeName(), pController->type(),
			userArg );

		INFO_MSG( "Controllers::addController(%u): Current controllers:\n",
			pEntity->id() );
		for (Container::const_iterator it = container_.begin();
				it != container_.end(); ++it)
		{
			Controller& controller = *it->second;
			INFO_MSG( "\t%s-%d(%d):, domain = %d, exclusive id = %d\n",
					controller.typeName(), pController->type(),
					int( controller.id() ), controller.domain(),
					controller.exclusiveID() );
		}

		return 0;
	}
	else if (count > CellAppConfig::expectedMaxControllers())
	{
		WARNING_MSG( "Controllers::addController(%u): "
				"#controllers = %d type=%s-%d, userArg = %d\n",
			pEntity->id(), count, pController->typeName(),
				pController->type(), userArg );
	}

	ControllerID controllerID = pController->exclusiveID();

	if (controllerID != 0)
	{
		// Deleting exclusive controller. For example, if another movement
		// controller already exists, stop it now before adding the new one.
		this->delController( controllerID, pEntity, /*warnOnFailure:*/false );
	}
	else
	{
		controllerID = this->nextControllerID();
	}

	pController->init( pEntity, controllerID, userArg );

	container_.insert( Container::value_type(
		pController->id(), pController ) );

	// If we operate in the ghost domain, create ourselves on all our ghosts
	if (pController->domain() & DOMAIN_GHOST)
	{
		RealEntity * pReal = pEntity->pReal();

		RealEntity::Haunts::iterator iter = pReal->hauntsBegin();

		while (iter != pReal->hauntsEnd())
		{
			Mercury::Bundle & bundle = iter->bundle();

			bundle.startMessage( CellAppInterface::ghostControllerCreate );
			bundle << pEntity->id();
			bundle << pController->id() << pController->type();
			bundle << pController->userArg();
			pController->writeGhostToStream( bundle );

			++iter;
		}
	}

	// Note: This was moved to be after the controller has been streamed on to
	// be sent to the ghosts. The reason for this was for the vehicles. In
	// startGhost, the entity's local position changes so that the global
	// position does not. The ghost entity needs to know that it is on a
	// vehicle before it gets this update.

	if (pController->domain() & DOMAIN_GHOST)
	{
		pController->startGhost();
	}

	if (pController->domain() & DOMAIN_REAL)
	{
		pController->startReal( true /* Is initial start */ );
	}

	return pController->id();
}


/**
 *	This method removes a controller from the collection.
 */
bool Controllers::delController( ControllerID id,
		Entity * pEntity,
		bool warnOnFailure )
{
	Container::iterator iter = container_.find( id );

	if (iter == container_.end())
	{
		if (warnOnFailure)
		{
			WARNING_MSG( "Controllers::delController(%u): "
					"Could not find controller id = %d.\n",
				pEntity->id(), id );
		}

		return false;
	}

	bool isRealEntity = pEntity->isReal();

	ControllerPtr pController = iter->second;
	container_.erase( iter );

	// If we operate in the ghost domain, delete ourselves on all our ghosts
	if (isRealEntity && (pController->domain() & DOMAIN_GHOST))
	{
		RealEntity * pReal = pEntity->pReal();
		RealEntity::Haunts::iterator iter = pReal->hauntsBegin();

		while (iter != pReal->hauntsEnd())
		{
			Mercury::Bundle & bundle = iter->bundle();

			bundle.startMessage( CellAppInterface::ghostControllerDelete );
			bundle << pEntity->id();
			bundle << id;

			++iter;
		}
	}

	// Note: This was moved to be after the deletion message has been
	// streamed on to be sent to the ghosts. The reason for this was for the
	// vehicles. In stopGhost, the entity's local position changes so that
	// the global position does not. The ghost entity needs to know that it
	// not on a vehicle before it gets this update.

	if ((pController->domain() & DOMAIN_REAL) && isRealEntity)
	{
		pController->stopReal( true /* Final stop */ );
	}

	if (pController->domain() & DOMAIN_GHOST)
	{
		pController->stopGhost();
	}

	pController->disowned();

	return true;
}


/**
 *	This method modifies an existing controller.
 */
void Controllers::modController( ControllerPtr pController, Entity * pEntity )
{
	MF_ASSERT( container_.find( pController->id() ) != container_.end() );

	MF_ASSERT( pController->domain() & DOMAIN_GHOST );

	RealEntity * pReal = pEntity->pReal();
	RealEntity::Haunts::iterator iter = pReal->hauntsBegin();

	while (iter != pReal->hauntsEnd())
	{
		Mercury::Bundle & bundle = iter->bundle();

		bundle.startMessage( CellAppInterface::ghostControllerUpdate );
		bundle << pEntity->id();
		bundle << pController->id();
		pController->writeGhostToStream( bundle );

		++iter;
	}
}


/**
 *	This method starts all real controllers.
 */
void Controllers::startReals()
{
	for (Container::iterator iter = container_.begin();
			iter != container_.end();
			++iter)
	{
		ControllerPtr pController = iter->second;

		if (pController->domain() & DOMAIN_REAL)
		{
			pController->startReal( /*isInitialStart:*/false );
		}
	}
}


/**
 *	This method stops all real controllers.
 */
void Controllers::stopReals( bool isFinalStop )
{
	// stop all controllers operating in the domain of reals
	// and erase them if they operate only in that domain
	BW::vector<ControllerPtr> toStop;
	Container::iterator it = container_.begin();
	while (it != container_.end())
	{
		// NOTE: This assumes that the iterator is not affected by the erase.
		// It looks like this is okay for maps.
		Container::iterator current = it++;

		if (current->second->domain() & DOMAIN_REAL)
		{
			toStop.push_back( current->second );
		}

		if (current->second->domain() == DOMAIN_REAL)
		{
			container_.erase( current );
		}
	}

	for (uint i = 0; i < toStop.size(); i++)
	{
		ControllerPtr pController = toStop[i];
		if (pController->isAttached())
		{
			pController->stopReal( isFinalStop );
			if (!(pController->domain() & DOMAIN_GHOST))
			{
				// Only disown DOMAIN_REAL ONLY controllers. There may be
				// controllers are both DOMAIN_REAL and DOMAIN_GHOST, and we
				// only want to stop the real part here.
				pController->disowned();
			}
		}
	}

	// NOTE: We have erase before we stop in case delController is called
	// on it due to a script callback from stopping the controller.
}


/**
 *	This method implements the Entity.cancel Python function. On failure, it
 *	returns NULL and sets the Python exception state.
 */
PyObject * Controllers::py_cancel( PyObject * args, Entity * pEntity )
{
	int controllerID;
	bool deleteByID = true;

	if (PyTuple_Size( args ) != 1)
	{
		PyErr_Format( PyExc_TypeError,
				"cancel takes exactly 1 argument (%" PRIzd " given)",
				PyTuple_Size( args ) );
		return NULL;
	}

	ScriptObject arg = ScriptObject( PyTuple_GET_ITEM( args, 0 ),
		ScriptObject::FROM_BORROWED_REFERENCE );

	if (!arg.convertTo( controllerID, ScriptErrorClear() ))
	{
		if (PyString_Check( arg.get() ))
		{
			deleteByID = false;
			controllerID =
				Controller::getExclusiveID(
						PyString_AsString( arg.get() ), /*createIfNecessary:*/ false );
			if (controllerID == 0)
			{
				PyErr_Format( PyExc_TypeError,
						"invalid exclusive controller category '%s'",
						PyString_AsString( arg.get() ) );
				return NULL;
			}
		}
		else
		{
			PyErr_SetString( PyExc_TypeError,
					"argument must be a ControllerID or a string" );
			return NULL;
		}
	}

	Container::iterator found = container_.find( controllerID );
	if (found != container_.end())
	{
		ControllerPtr pController = found->second;
		pController->cancel();
	}
	else if (deleteByID)
	{
		WARNING_MSG( "Entity.cancel(%u): Cancelling an unknown "
				"controller ID %d\n",
			pEntity->id(), controllerID );
	}

	Py_RETURN_NONE;
}


/**
 *	This method gets the next controller id for this entity
 */
ControllerID Controllers::nextControllerID()
{
	do
	{
		lastAllocatedID_ =
			(lastAllocatedID_ + 1) & Controller::EXCLUSIVE_CONTROLLER_ID_MASK;
	}
	while ((container_.find( lastAllocatedID_ ) != container_.end()) ||
		(lastAllocatedID_ == 0));

	return lastAllocatedID_;
}


/**
 *	This method calls the visitor's visit method for each Controller in
 *	our container.
 *
 *	@returns true on success, false if visiting has been terminated early. 
 */
bool Controllers::visitAll( ControllersVisitor & visitor )
{
	Container::iterator it = container_.begin();
	bool shouldContinue = true;

	while (shouldContinue && (it != container_.end()))
	{
		shouldContinue = visitor.visit( it->second );
		++it;
	}

	return shouldContinue;
}

BW_END_NAMESPACE

// controllers.cpp
