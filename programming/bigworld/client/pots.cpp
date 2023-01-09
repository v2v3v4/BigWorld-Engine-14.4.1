#include "pch.hpp"
#include "pots.hpp"
#include "script_player.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"
#include "duplo/pymodelnode.hpp"

DECLARE_DEBUG_COMPONENT2( "App", 0 )


BW_BEGIN_NAMESPACE

//the singleton
Pots Pots::s_instance_;

int Pot_token = 0;


/**
 * Constructor.
 */
Pots::Pots() :
	nextHandle_( 1 )
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Pots", ">Script", NULL );
}


/**
 *	Destructor.
 */
Pots::~Pots()
{
	BW_GUARD;
	// Check to see if we are leaking. We should not have any allocations now,
	// and if we ever did, all should have been freed in the "fini" method.
	MF_ASSERT_DEV( pots_.empty() && mats_.empty() );
}


void Pots::checkPots()
{
	BW_GUARD;
	if ( !ScriptPlayer::entity() )
		return;

	Vector3 playerPos( ScriptPlayer::entity()->position() );
	Matrix m;

	//check each pot for a breach.
	PotMap::iterator it = pots_.begin();
	PotMap::iterator end = pots_.end();

	while ( it != end )
	{
		Pot& pot = *(it)->second;

		if ( pot.matrixProvider_ )
		{
			pot.matrixProvider_->matrix(m);
			float dSq = Vector3(m.applyToOrigin() - playerPos).lengthSquared();
			if ( (dSq<pot.distanceSq_) != pot.inside_ )
			{
				pot.inside_ = !pot.inside_;
				Py_INCREF( pot.function_.getObject() );
				Script::call( pot.function_.getObject(), Py_BuildValue( "(ii)", pot.inside_?1:0, it->first ), "Pots::tick" );
			}
		}

		it++;
	}


	//now safely kill dead pots ( one per frame )
	it = pots_.begin();
	end = pots_.end();

	while ( it != end )
	{
		Pot* pot = it->second;
		if ( !pot->matrixProvider_ )
		{
			pots_.erase( it );
			bw_safe_delete(pot);
			break;
		}
		it++;
	}
}


void Pots::checkMats()
{
	BW_GUARD;
	Vector3 camPos( Moo::rc().invView().applyToOrigin() );
	Vector3 checkPos;
	Matrix src;
	Matrix tgt;

	//check each mat for a breach.
	PotMap::iterator it = mats_.begin();
	PotMap::iterator end = mats_.end();

	while ( it != end )
	{
		Pot& mat = *(it)->second;

		if ( mat.matrixProvider_ )
		{
			mat.matrixProvider_->matrix(src);
			src.invert();

			if (mat.target_.hasObject())
			{
				mat.target_->matrix(tgt);
				checkPos = src.applyPoint(tgt.applyToOrigin());
			}
			else
			{
				checkPos = src.applyPoint(camPos);
			}

			if ( (checkPos.lengthSquared()<1.f) != mat.inside_ )
			{
				mat.inside_ = !mat.inside_;
				Py_INCREF( mat.function_.getObject() );
				Script::call( mat.function_.getObject(), Py_BuildValue( "(ii)", mat.inside_?1:0, it->first ), "Pots::tick" );
			}
		}

		it++;
	}


	//now safely kill dead mats ( one per frame )
	it = mats_.begin();
	end = mats_.end();

	while ( it != end )
	{
		Pot* mat = it->second;
		if ( !mat->matrixProvider_ )
		{
			mats_.erase( it );
			bw_safe_delete(mat);
			break;
		}
		it++;
	}
}


/**
 *	This method is called during via the MainLoopTasks mechanism, and checks
 *	all active pots against the player's position.
 */
void Pots::tick( float /* dGameTime */, float /* dRenderTime */ )
{
	BW_GUARD;
	SCOPED_DISABLE_FRAME_BEHIND_WARNING;
	checkPots();
	checkMats();
}

/*~ function BigWorld.addPot
 *
 *	This function adds a player-only trap to the active list of traps.  This
 *	trap will trigger if the player entity moves either into or out of the 
 *	sphere of specified radius and centre.  When it triggers, it will call
 *	the specified callable object, with two arguments, the first of which
 *	is 0 if the player moved out of the sphere, and 1 if the player moved
 *	into the sphere.  The second is the handle of the trap, which is the
 *	number returned by the addPot function.
 *	See Entity.addTrap for a trap that works on all entities.
 *
 *	For example:
 *	@{
 *	# the callback function
 *	def hitPot( enteredTrap, handle ):
 *		if enteredTrap:
 *			print "The player entered the sphere for trap number ", handle
 *		else:
 *			print "The player left the sphere for trap number ", handle
 *
 *	# Note: player.matrix is the players current position
 *	BigWorld.addPot( BigWorld.player().matrix, 5.0, hitPot )
 *	@}
 *	In this example, the hitPot function will get called whenever the player enters
 *	or leaves a stationary sphere with a radius of 5 metres (the position of the
 *	sphere is centred around where the player was when the addPot function was called).
 *
 *	@param	centre	a MatrixProvider.  This is the centre of the sphere.
 *	@param	radius	a float.  This is the radius of the sphere to trap.
 *	@param	callback	a callable object.  This is the function that gets
 *			called when the trap is triggered.
 *
 *	@return an integer.  The handle of the trap.
 */
/**
 *	This method adds a player-only-trap to the active list.
 *
 *	@param	source	the location, as given by a matrix provider
 *	@param	dist	the threshold distance
 *	@param	callback	the script function to be called ( must take two int params (in/out flag and potID) )
 *
 *	@return handle	a unique identifier for the pot
 */
uint32 Pots::addPot( MatrixProviderPtr source, float dist, SmartPointer<PyObject> callback )
{
	BW_GUARD;
	uint32 id = s_instance_.nextHandle_++;

	IF_NOT_MF_ASSERT_DEV( source.hasObject() )
	{
		ERROR_MSG( "Assert failure 'source.hasObject()' - pot not created %i\n", id );
		return id;
	}

	IF_NOT_MF_ASSERT_DEV( callback )
	{
		ERROR_MSG( "Assert failure 'callback' - pot not created %i\n", id );
		return id;
	}

	IF_NOT_MF_ASSERT_DEV( PyCallable_Check( callback.getObject() ) )
	{
		ERROR_MSG( "Assert failure 'PyCallable_Check( callback.getObject() )' - pot not created %i\n", id );
		return id;
	}

	s_instance_.pots_[ id ] = new Pot( source, dist, callback );
	return id;
}

PY_MODULE_STATIC_METHOD( Pots, addPot, BigWorld )

/*~ function BigWorld.delPot
 *
 *	This method deletes a trap from the active list.  This stops it responding
 *	to the player entity entering or leaving its sphere.
 *	
 *	The trap is specified by its handle, which was returned by addPot when
 *	the trap was created.
 *
 *	@param handle	an integer.  The handle of the trap to remove.
 */
/**
 *	This method deletes a player-only-trap from the active list.
 */
void Pots::delPot( uint32 handle )
{
	BW_GUARD;
	PotMap::iterator it = s_instance_.pots_.find( handle );

	if (it != s_instance_.pots_.end())
	{
		Pot* ppot = it->second;

		//flag pending deletion
		ppot->matrixProvider_ = NULL;
		ppot->function_ = NULL;
		ppot->target_ = NULL;
	}
}

PY_MODULE_STATIC_METHOD( Pots, delPot, BigWorld )


/**
 *	This method deletes the instance. It's called from MainLoopTasks when the
 *	App is about to finish.
 */
void Pots::fini()
{
	BW_GUARD;
	PotMap::iterator it = pots_.begin();
	PotMap::iterator end= pots_.end();
	while (it != end)
	{
		Pot * pot = (it++)->second;
		bw_safe_delete(pot);
	}

	pots_.clear();


	it = mats_.begin();
	end= mats_.end();
	while (it != end)
	{
		Pot * mat = (it++)->second;
		bw_safe_delete(mat);
	}

	mats_.clear();
}


/**
 *	This method adds a matrix trap to the active list.
 *
 *	@param	source	the location, as given by a matrix provider
 *	@param	callback	the script function to be called ( must take two int params (in/out flag and matID) )
 */
uint32 Pots::addMat( MatrixProviderPtr source, SmartPointer<PyObject> callback, MatrixProviderPtr target )
{
	BW_GUARD;
	uint32 id = s_instance_.nextHandle_++;

	IF_NOT_MF_ASSERT_DEV( source.hasObject() )
	{
		ERROR_MSG( "Assert failure 'source.hasObject()' - pot not created %i\n", id );
		return id;
	}

	IF_NOT_MF_ASSERT_DEV( callback )
	{
		ERROR_MSG( "Assert failure 'callback' - pot not created %i\n", id );
		return id;
	}

	IF_NOT_MF_ASSERT_DEV( PyCallable_Check( callback.getObject() ) )
	{
		ERROR_MSG( "Assert failure 'PyCallable_Check( callback.getObject() )' - pot not created %i\n", id );
		return id;
	}

	s_instance_.mats_[ id ] = new Pot( source, 0.f, callback, target );
	return id;
}

/*~	function BigWorld.addMat
 *
 *	This method adds a matrix trap to the active list.  Rather than having a Player Entity trigger the trap, as in 
 *	a pot (Player-Only Trap) it uses a MatrixProvider to trigger the trap.  It could use the Player's matrix to 
 *	perform the exact same task as a pot, however a mat has the added flexibility of being able to use any point in 
 *	the world as the triggering element (be it an Entity or otherwise).  If the target matrix is not provided, the 
 *	MatrixProvider of the current Camera will be used.  The scale of the source MatrixProvider represents the size 
 *	of the mat.
 *	
 *	When the matrix trap is triggered, it will notfiy the given callback function, defined as;
 *
 *		def callback( self, in, matID ):
 *
 *	where in will be 1 if the target MatrixProvider has entered the mat and 0 if it has left.  The ID of the mat 
 *	that was triggered is also provided
 *
 *	@param	source		The centre and size of the mat, as given by a MatrixProvider (size = scale of matrix)
 *	@param	callback	The script function to be called when the mat is triggered (to receive in flag and mat ID)
 *	@param	target		The optional MatrixProvider that will trigger the mat (uses current Camera if not provided)
 *
 *	@return	integer ID of the created and active matrix trap
 */
PY_MODULE_STATIC_METHOD( Pots, addMat, BigWorld )


/**
 *	This method deletes a matrix-trap from the active list.
 */
void Pots::delMat( uint32 handle )
{
	BW_GUARD;
	PotMap::iterator it = s_instance_.mats_.find( handle );

	if (it != s_instance_.mats_.end())
	{
		Pot* pmat = it->second;

		//flag pending deletion
		pmat->matrixProvider_ = NULL;
		pmat->function_ = NULL;
		pmat->target_ = NULL;
	}
}

/*~	function BigWorld.delMat
 *
 *	Removes the mat from active duty as described by the given mat handle.
 *
 *	@param	matID	The integer ID of the mat to remove.  Corresponds to the handle returned by BigWorld.addMat()
 */
PY_MODULE_STATIC_METHOD( Pots, delMat, BigWorld )


BW_END_NAMESPACE

// pots.cpp
