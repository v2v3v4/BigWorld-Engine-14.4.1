#include "pch.hpp"

#include "pymodel.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/vectornodest.hpp"

#include "duplo/ik_constraint_system.hpp"
#include "duplo/py_material_fashion.hpp"
#include "duplo/py_transform_fashion.hpp"
#include "pydye.hpp"

#include "action_queue.hpp"
#include "motor.hpp"
#include "pymodelnode.hpp"

#include "ashes/bounding_box_gui_component.hpp"
#include "ashes/text_gui_component.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_obstacle.hpp"

#include "romp/water.hpp"
#include "moo/geometrics.hpp"
#include "model/super_model.hpp"
#include "romp/font.hpp"
#include "romp/labels.hpp"

#include "math/colour.hpp"

#include "model/super_model_dye.hpp"
#include "moo/draw_context.hpp"
#include "moo/moo_math.hpp"
#include "moo/node.hpp"
#include "moo/render_context.hpp"
#include "moo/node_catalogue.hpp"
#include "moo/visual.hpp"

#include "fmodsound/py_sound.hpp"
#include "fmodsound/py_sound_list.hpp"

#include "pyscript/py_data_section.hpp"

#include "moo/lod_settings.hpp"

DECLARE_DEBUG_COMPONENT2( "PyModel", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "pymodel.ipp"
#endif

extern bool g_crossFadeLODs;

// -----------------------------------------------------------------------------
// Section: PyModel
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyModel )

PY_BEGIN_METHODS( PyModel )
	PY_METHOD_WITH_DOC( action, "This method looks up an action by name from "
		"an internal list of actions and returns an ActionQueuer object if it "
		"finds it." )
	PY_METHOD_WITH_DOC( playSound, 
		"Plays a 3D sound at the location of the model." )
	PY_METHOD_WITH_DOC( getSound, "Returns a Sound object for a 3D sound event "
		"at the location of the model." )

	// Aliases provided for backwards compatibility
	PY_METHOD_ALIAS( playSound, playFxSound )
	PY_METHOD_ALIAS( getSound, getFxSound )

	PY_METHOD_WITH_DOC( addMotor,
		"This method adds the specified object to the model's list of Motors." )
	PY_METHOD_WITH_DOC( delMotor,
		"This method removes the specified object to the models list of Motors." )
	PY_METHOD_WITH_DOC( straighten, "This method straightens a model, i.e. "
		"removes all rotations (including yaw) from it." )
	PY_METHOD_WITH_DOC( rotate, "This method rotates a model about an "
		"arbitrary axis (and optionally an arbitrary centre)." )
	PY_METHOD_WITH_DOC( alignTriangle, "This method aligns a model to the "
		"given triangle (specified as three points in world space, taken in a "
		"clockwise direction)." )
	PY_METHOD_WITH_DOC( reflectOffTriangle, "This method reflects the Vector3 "
		"specified by the fwd argument off the specified triangle, and places "
		"the model in the centre of the triangle facing in the direction of "
		"the reflected vector." )
	PY_METHOD_WITH_DOC( zoomExtents, "This method places the model at the "
		"origin, scaled to fit within a 1 metre cube; that is, its longest "
		"dimension will be exactly 1 metre." )
	PY_METHOD_WITH_DOC( node, "Returns the named node." )
	PY_METHOD_WITH_DOC( origin, "Make the named node the origin of the model's "
		"coordinate system, if a node with that name exists.")
	PY_METHOD( getIKConstraintSystemFromData )
	PY_METHOD_WITH_DOC( cue, "This method sets the default response for the "
		"given cue identifier. Sets a python exception on error." )

	PY_METHOD_WITH_DOC( saveActionQueue, "This function is used to save the state of "
		"the pyModel's action queue." )
	PY_METHOD_WITH_DOC( restoreActionQueue, "This function is used to restore the state "
		"of the pyModel's action queue." )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyModel )
	/*~ attribute PyModel.visible
	 *
	 *	This attribute controls whether or not the model is rendered.  If it is
	 *	set to 0, the model is not rendered, anything else and it is rendered.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( visible )
	/*~ attribute PyModel.visibleAttachments
	 *
	 *	This attribute can be set to allow hiding of model but update of animation and drawing
	 *	of attachments (or allowing selective control of their individual visibility). Only meaningful
	 *	when visible flag is set to false.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( visibleAttachments )
	/*~ attribute PyModel.moveAttachments
	 *
	 *	This attribute can be set to allow motors to function on attached objects.
	 *	By default this flag is turned off, because calling move on all known nodes
	 *	on all PyModels can take a measurable amount of time, even if there are no
	 *	motors attached.
	 *	Note the action matcher is a motor, turn this flag on to allow the action
	 *	matcher to apply to attached objects.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( moveAttachments )
	/*~ attribute	PyModel.independentLod
	 *
	 *	This attribute controls whether this model recalculates the lod distance
	 *	independently of the lod distance passed in to its draw method.
	 *
	 *	@type bool
	 */
	PY_ATTRIBUTE( independentLod )
	/*~ attribute PyModel.outsideOnly
	 *
	 *	Indicates that this model is only ever positioned in outside chunks.
	 *	This allows the engine to optimise processing on this model.
	 *	There are extra processing steps that need to be done if  this model is
	 *	capable of being inside.  If it is only ever going to be outside, then
	 *	optimise by setting this attribute to 1. By default it is 0.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( outsideOnly )
	/*~ attribute PyModel.moveScale
	 *
	 *	This attribute sets the model's actions's movement speed scale.
	 *	The higher the moveScale the faster the model moves.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( moveScale )
	/*~ attribute PyModel.actionScale
	 *
	 *	This attribute sets the model's action's animation speed scale.
	 *	The higher the actionScale the faster the model's actions are played.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( actionScale )

	/*~ attribute PyModel.position
	 *
	 *	The position of the model within the world, as a 3d vector.  This is in
	 *	theory write as well as read, however if there are motors attached to
	 *	the model, then those motors will often update the models position
	 *	every tick, at which point any data written will be overwritten before
	 *	it gets rendered at the new position.  In a model with no motors
	 *	though, you can set the models position by writing to this attribute.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( position )
	/*~ attribute PyModel.scale
	 *
	 *	Used to scale the model in each of the 3 dimensions.  Takes a vector3,
	 *	each component of which specifies the scale for the model in the
	 *	corresponding dimension.  This only effects the rendering of the model,
	 *	not its bounding box for collision tests
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( scale )
	/*~ attribute PyModel.height
	 *
	 *	This is the height of the bounding box used for collision detection.
	 *	Changing this variable sets the top of the bounding box to height above
	 *	the existing bottom of the bounding box.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( height )
	/*~ attribute PyModel.yaw
	 *
	 *	The rotation of the model about its own (rather than the world's)
	 *	y-axis.  This is, in theory, write as well as read, however if there
	 *	are motors attached to the model, then those motors will often update
	 *	the models yaw every tick, at which point any data written will be
	 *	overwritten before it gets rendered at the new yaw.  In a model with no
	 *  motors though, you can set the models yaw by writing to this attribute.
	 *
	 *	@type	Float;
	 */
	PY_ATTRIBUTE( yaw )
	/*~ attribute PyModel.unitRotation
	 *
	 *	This attribute allows scripts to access the difference in yaw between
	 *	the model and the direction the ActionMatcher (if one is present) is
	 *	matching to.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( unitRotation )
	/*~ attribute PyModel.queue
	 *
	 *	This read only attribute is a list of the names of the actions which
	 *	have been queued up to play.
	 *
	 *	@type	Read-Only List of Strings
	 */
	PY_ATTRIBUTE( queue )
	/*~	attribute PyModel.motors
	 *
	 *	This attribute is the list of motors which affect the position and
	 *	rotation of the model, based on the position and rotation of the owning
	 *	entity.  It is generally manipulated using the addMotor and delMotor
	 *	functions on the model.  On creation, an entity's model has an
	 *	ActionMatcher as its only motor.
	 *
	 *	@type	List of Motors
	 */
	PY_ATTRIBUTE( motors )
	/*~	attribute PyModel.sources
	 *
	 *	This read-only attribute is a list of the resource ids of all the
	 *	models that make up the super model.  In the case of a simple model
	 *	this is a list of just one resource id.
	 *
	 *	@type	Read-Only List of Strings
	 */
	PY_ATTRIBUTE( sources )
	/*~	attribute PyModel.bounds
	 *
	 *	This read-only attribute gives the bounding box for the model.  It
	 *	returns a MatrixProvider.  The matrix specifies what transformation
	 *	would need to be applied to a 1x1x1 cube placed at the
	 *	world-space origin to scale, rotate and translate it to bound the
	 *	model.  The bounds themselves are read in from the &lt;boundingBox>
	 *	property of the .visual file that was used to create the model,
	 *	and don't update to reflect the current pose of the model.
	 *
	 *	This MatrixProvider can be used with the BoundingBoxGUIComponent to
	 *	find the smallest bounding rectangle of the model, in relation to the
	 *	current viewport on the world.
	 *
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( bounds )
	/*~ attribute PyModel.tossCallback
	 *
	 *	This attribute is the callback function that gets called every time the
	 *	model moves from one chunk to another.  There are no specific arguments
	 *	supplied to the function.
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( tossCallback )
	/*~ attribute	PyModel.root
	 *
	 *	This read-only attribute is the root node of the model.
	 *
	 *	@type Read-Only PyModelNode
	 */
	PY_ATTRIBUTE( root )

	/*~ attribute	PyModel.stopSoundsOnDestroy
	 *
	 *  Controls whether or not sounds attached to this model are stopped when
	 *  this model is destroyed.  This attribute is set to True by default,
	 *  however you may want to set this to False if you have a special effect
	 *  whose model may disappear before its sound events have finished playing.
	 *
	 *	@type bool
	 */
	PY_ATTRIBUTE( stopSoundsOnDestroy )

PY_END_ATTRIBUTES()

/*~ function BigWorld.Model
 *
 *	Model is a factory function that creates and returns a new PyModel object
 *	(None if there are problems loading any of the requested resources).
 *	PyModel@@s are renderable objects which can be moved around the world,
 *	animated, dyed (have materials programatically assigned) and contain points
 *	to which objects can be attached.
 *
 *	The parameters for the function are one or more model resources. Each of
 *	these individual models is loaded, and combined into a "supermodel".
 *
 *	This function will block the main thread to load models in the event that
 *	they were not specified as prerequisites. To load models asynchronously
 *	from script use the method BigWorld.loadResourceListBG().
 *
 *	For example, to load a body and a head model into one supermodel:
 *
 *	@{
 *	model = BigWorld.Model( "models/body.model", "models/head.model" )
 *	@}
 *
 *	@see	BigWorld.loadResourceListBG
 *
 *	@param *modelPath	String argument(s) for the model.
 *
 *	@return A reference to the PyModel specified.
 */
PY_FACTORY_NAMED( PyModel, "Model", BigWorld )


PY_SCRIPT_CONVERTERS( PyModel )

/*~ function BigWorld.fetchModel
 *
 *	Deprecated: It is recommended to use BigWorld.loadResourceListBG in the
 *	following manner instead:
 *	@{
 *	# load list of models in background
 *	myList = [ "a.model", "b.model" ]
 *	BigWorld.loadResourceListBG( myList, self.callbackFn )
 *	def callbackFn( self, resourceRefs ):
 *		if resourceRefs.failedIDs:
 *			ERROR_MSG( "Failed to load %s" % ( resourceRefs.failedIDs, ) ) 
 *		else:
 *			# construct supermodel
 *			myModel = BigWorld.Model( *resourceRefs.keys() )
 *	@}
 *
 *	fetchModel is a function that creates and returns a new PyModel object
 *	through the callback function provided (None if there are problems loading
 *	any of the requested resources). PyModel@@s are renderable objects which
 *	can be moved around the world, animated, dyed (have materials
 *	programatically assigned) and contain points to which objects can be
 *	attached.
 *
 *	The parameters for the function are one or more model resources. Each of
 *	these individual models is loaded, and combined into a "supermodel".
 *
 *	This function is similar to the BigWorld.Model function. However, it
 *	schedules the model loading as a background task so that main game script
 *	execution would not be affected. After model loading is completed, callback
 *	function will be triggered with the valid PyModel.
 *
 *	For example, to load a body and a head model into one supermodel:
 *
 *	@{
 *	BigWorld.fetchModel( "models/body.model", "models/head.model", onLoadCompleted )
 *	@}
 *
 *	@param *modelPath	String argument(s) for the model.
 *	@param onLoadCompleted The function to be called when the model resource loading is
 *						 completed. An valid PyModel will be returned. When the model
 *						 loading fails. A None is passed back.
 *
 *	@return No return values.
 */

ScheduledModelLoader::ScheduledModelLoader( PyObject * bgLoadCallback,
	const BW::vector< BW::string > & modelNames )
	:
	BackgroundTask( "ScheduledModelLoader" )
	, pBgLoadCallback_( bgLoadCallback )
	, pSuperModel_()
	, modelNames_( modelNames )
{
	BW_GUARD;
	FileIOTaskManager::instance().addBackgroundTask( this );
}

void ScheduledModelLoader::doBackgroundTask( TaskManager & mgr )

{
	BW_GUARD;
	pSuperModel_ = new SuperModel( modelNames_ );

	mgr.addMainThreadTask( this );
}


void ScheduledModelLoader::doMainThreadTask( TaskManager & mgr )
{
	BW_GUARD;
	TRACE_MSG( "ScheduledModelLoader::doMainThreadTask:\n" );

	bool successfullLoad =
		modelNames_.size() == pSuperModel_->nModels();

	if (successfullLoad)
	{
		pSuperModel_->checkBB( false );
		PyModel * model = new PyModel( pSuperModel_ );
		Py_INCREF( pBgLoadCallback_.getObject() );
		Script::call( &*(pBgLoadCallback_),
			Py_BuildValue( "(O)", (PyObject *) model ),
			"PyModel onModelLoaded callback: " );
		Py_DECREF( model );
	}
	else
	{
		ERROR_MSG( "Model(): Did not found all models "
			"Only found %d out of %d models requested\n",
			pSuperModel_->nModels(), modelNames_.size() );

		Py_INCREF( pBgLoadCallback_.getObject() );

		//following is equivalent to return a Py_None
		Script::call( &*(pBgLoadCallback_),
				Py_BuildValue( "(s)", NULL),
				"PyModel onModelLoaded callback: " );

		bw_safe_delete(pSuperModel_);
	}
}

static PyObject * py_fetchModel( PyObject * args )
{
	BW_GUARD;

	// deprecated as of: 3/1/2012
	WARNING_MSG( "BigWorld.fetchModel(): This method is deprecated. "
		"Use BigWorld.loadResourceListBG() instead.\n" );

	Py_ssize_t size = PyTuple_Size( args );
	if (size < 2)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.fetchModel requires at least 2 arguments" );
		return NULL;
	}

	Py_ssize_t sz = PyTuple_Size( args ) - 1;
	BW::vector< BW::string > modelNames( sz );
	Py_ssize_t i;
	PyObject * pItem = NULL;
	for (i = 0; i < sz; i++)
	{
		pItem = PyTuple_GetItem( args, i );	// borrowed
		if (!PyString_Check( pItem ))
		{
			PyErr_Format( PyExc_ValueError, "fetchModel(): "
				"Argument %d is not a string", i );
			return NULL;
		}
		modelNames[i] = PyString_AsString( pItem );
	}

	PyObject * bgLoadCallback = PyTuple_GetItem( args, sz );

	if (!PyCallable_Check( bgLoadCallback ))
	{
		PyErr_Format( PyExc_ValueError, "fetchModel(): "
			"Callback function is not a callable object" );
		return NULL;
	}

	// This object deletes itself.
	new ScheduledModelLoader( bgLoadCallback, modelNames );

	Py_RETURN_NONE;
}

PY_MODULE_FUNCTION( fetchModel, BigWorld )

// This is the model that is currently being ticked() or moved() or whatever.
// This replaces the SoundSourceMinder::s_pTopMinder_ thing that was used for
// letting Motors trigger sounds in rev().
//
// TODO: This is a pretty dodgy way of doing this and we should think of a
// better way of passing the PyModel* down into PyAttachment::tick() and
// PyAttachment::draw().
PyModel* PyModel::s_pCurrent_ = NULL;

/*static*/ int PyModel::ScopedPyModelWatchHolder::running_( 0 );

/**
 *	Constructor
 */
PyModel::PyModel( SuperModel * pSuperModel, PyTypeObject * pType ) :
 PyAttachment( pType ),
 pSuperModel_( pSuperModel ),
 dummyModelNode_( NULL ),
 localBoundingBox_( Vector3::zero(), Vector3::zero() ),
 localVisibilityBox_( Vector3::zero(), Vector3::zero() ),
 height_( 0.f ),
 visible_( true ),
 visibleAttachments_( false ),
 moveAttachments_( false ),
 independentLod_( false ),
 outsideOnly_( false ),
 unitOffset_( Vector3::zero() ),
 unitRotation_( 0 ),
 unitTransform_( Matrix::identity ),
 shouldDrawSkeleton_( false ),
 pDebugInfo_( NULL ),
 transformModsDirty_( false ),
 dirty_( false ),
 moveScale_( 1.f ),
 actionScale_( 1.f )
#if FMOD_SUPPORT
 ,pSounds_( NULL ),
  cleanupSounds_( true )
#endif
{
	BW_GUARD;

	if (pSuperModel_ != NULL)
	{
		// Mutually exclusive of pSuperModel_ reloading, start
		pSuperModel_->beginRead();
	}

	this->pullInfoFromSuperModel();
	if (pSuperModel_ != NULL)
	{
		pSuperModel_->registerModelListener( this );

		// Mutually exclusive of pSuperModel_ reloading, end
		pSuperModel_->endRead();
	}
	else
	{
		Moo::NodePtr dummyNode =
			Moo::NodeCatalogue::find( Moo::Node::SCENE_ROOT_NAME );
		dummyModelNode_ = SmartPointer<PyModelNode>(
			new PyModelNode( this, dummyNode, NULL ),
			SmartPointer<PyModelNode>::STEAL_REFERENCE );
	}
}


 /**
 *	This method pull the info from our pVisual_
 *	@pram visualOnly		if true, then we only update data
 *							that's related to the models' visual
 */
 void PyModel::pullInfoFromSuperModel( bool visualOnly /*= false*/)
 {
	 BW_GUARD;
	// calculate our local bounding boxes for future reference
	this->calculateBoundingBoxes();
	if (visualOnly)
	{
		return;
	}

	if (pSuperModel_ != NULL)
	{
		SuperModel::ReadGuard guard( pSuperModel_ );
		// get the initial transform matrices
		//Model::Animation * pMainDefault =
		//	pSuperModel->topModel(0)->lookupLocalDefaultAnimation();
		//pMainDefault->flagFactor( -1, initialItinerantTransform_ );
		//initialItinerantTransformInverse_.invert( initialItinerantTransform_);
		initialItinerantContext_ = Matrix::identity;
		initialItinerantContextInverse_.invert( initialItinerantContext_ );

		// and dress in default in case someone wants our nodes
		pSuperModel_->dressInDefault();
	}
}
 /**
 * if any of our models_ has been reloaded, 
 * update the related data.
 */
void PyModel::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;

	if (pReloader)
	{
		bool modelReloaded = pSuperModel_->hasModel( (Model*) pReloader, true );
		bool visualReloaded = pSuperModel_->hasVisual( (Moo::Visual*) pReloader );
		if (modelReloaded || visualReloaded)
		{
			this->pullInfoFromSuperModel( visualReloaded );

			if (modelReloaded)
			{
				//reset actionQueue_ and motors.
				actionQueue_.flush( true );
				Motors::iterator it = motors_.begin();
				for (; it!= motors_.end(); ++ it)
				{
					(*it)->onOwnerReloaded();
				}
			}
		}
	}
}

/**
 * This method generates the texture usage data for the
 * wrapped models and attachments.
 */
void PyModel::generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup ) const
{
	// First generate the super model's texture usage
	if (pSuperModel())
	{
		pSuperModel()->generateTextureUsage( usageGroup );
	}

	// Now iterate over all nodes and their attachments
	for (PyModelNodes::const_iterator nit = knownNodes_.begin(),
		nend = knownNodes_.end(); nit != nend; ++nit)
	{
		PyModelNode * pynode = *nit;

		const PyAttachments & attachments = pynode->attachments();
		for (PyAttachments::const_iterator ait = attachments.begin(),
			aend = attachments.end(); ait != aend; ++ait)
		{
			const PyAttachmentPtr& attachment = *ait;
			const PyModel * attachedModel = attachment->getPyModel();

			// attached pointer might be zero if pymodel has non model attached to it
			// non models include particles and possible other things
			if (attachedModel)
			{
				attachedModel->generateTextureUsage( usageGroup );
			}
		}
	}
}


void PyModel::textureUsageWorldBox( BoundingBox & wbb, const Matrix & worldTransform ) const
{
	BW_GUARD;	

	BoundingBox bb;
	this->localBoundingBoxInternal( bb );
	bb.transformBy( worldTransform );
	wbb.addBounds( bb );

	// Now iterate over all nodes and their attachments
	const PyModelNodes&	 knownNodes = knownNodes_;
	for (PyModelNodes::const_iterator nit = knownNodes_.begin(),
		nend = knownNodes_.end(); nit != nend; ++nit)
	{
		PyModelNode * pynode = *nit;

		const PyAttachments & attachments = pynode->attachments();
		for (PyAttachments::const_iterator ait = attachments.begin(),
			aend = attachments.end(); ait != aend; ++ait)
		{
			const PyModel * attachedModel = (*ait)->getPyModel();

			// attached pointer might be zero if pymodel has non model attached to it
			// non models include particles and possible other things
			if (attachedModel)
			{
				attachedModel->textureUsageWorldBox( wbb, worldTransform );
			}
		}
	}
}

/**
 *	Destructor
 */
PyModel::~PyModel()
{
	BW_GUARD;
	// take us out of the world
	if (this->isInWorld()) this->leaveWorld();

	// detach all our known nodes
	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
	{
		(*nit)->detach();
		Py_DECREF( *nit );
	}
	knownNodes_.clear();

	// Inform all fashions that they have been disowned
	for (PyMaterialFashions::iterator itr = pyMaterialFashionsMap_.begin();
		itr != pyMaterialFashionsMap_.end();
		++itr)
	{
		itr->second->disowned();
	}
	for (PyTransformFashions::iterator itr =
		postPyTransformFashionsMap_.begin();
		itr != postPyTransformFashionsMap_.end();
		++itr)
	{
		itr->second->disowned();
	}
	for (PyTransformFashions::iterator itr =
		prePyTransformFashionsMap_.begin();
		itr != prePyTransformFashionsMap_.end();
		++itr)
	{
		itr->second->disowned();
	}

	// Get rid of all motors
	for (Motors::iterator it = motors_.begin(); it != motors_.end(); it++)
	{
		(*it)->detach();
		Py_DECREF( (*it) );
	}

	if (dummyModelNode_ != NULL)
	{
		dummyModelNode_->detach();
		dummyModelNode_ = NULL;
	}

	// and finally delete the supermodel (whoops!)
	bw_safe_delete(pSuperModel_);

	// This may be NULL.
	bw_safe_delete(pDebugInfo_);

	// Clean up any sound events that might still be playing
#if FMOD_SUPPORT
	this->cleanupSounds();
#endif
}


/**
 *	We've changed chunks. Someone might be interested.
 */
void PyModel::tossed( bool outside )
{
	BW_GUARD;
	if (tossCallback_)
	{
		Py_INCREF( tossCallback_.getObject() );
		Script::call( &*tossCallback_, PyTuple_New(0),
			"PyModel toss callback: " );
	}

	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
		(*nit)->tossed( outside );
}


/**
 *	This method calculates the local space BoundingBoxes for the model
 */
void PyModel::calculateBoundingBoxes()
{
	BW_GUARD;
	if (pSuperModel_ != NULL)
	{
		BoundingBox bb;

		pSuperModel_->localBoundingBox( bb );

		float maxXZ = max( fabsf( bb.maxBounds().x ) , max( fabsf( bb.minBounds().x ),
			max( fabsf( bb.maxBounds().z ), fabsf( bb.minBounds().z ) ) ) ) * 2;
		float ysz = bb.maxBounds().y - bb.minBounds().y;

		localBoundingBox_ = BoundingBox(
			Vector3( -maxXZ, bb.minBounds().y - ysz*0.5f, -maxXZ ),
			Vector3( maxXZ, bb.maxBounds().y + ysz*0.5f, maxXZ ) );

		pSuperModel_->localVisibilityBox( bb );

		maxXZ = max( fabsf( bb.maxBounds().x ) , max( fabsf( bb.minBounds().x ),
			max( fabsf( bb.maxBounds().z ), fabsf( bb.minBounds().z ) ) ) ) * 2;
		ysz = bb.maxBounds().y - bb.minBounds().y;

		localVisibilityBox_ = BoundingBox(
			Vector3( -maxXZ, bb.minBounds().y - ysz*0.5f, -maxXZ ),
			Vector3( maxXZ, bb.maxBounds().y + ysz*0.5f, maxXZ ) );
	}
	else
	{
		localBoundingBox_.setBounds( Vector3(-0.5f,  0.0f, -0.5f),
								Vector3( 0.5f,  1.0f,  0.5f) );
		localVisibilityBox_ = localBoundingBox_;
	}
}


/**
 *	This method makes up a name for this model
 */
BW::string PyModel::name() const
{
	BW_GUARD;
	if (!pSuperModel_) return "";

	BW::string names;
	for (int i = 0; i < pSuperModel_->nModels(); i++)
	{
		if (i) names += ";";
		names += pSuperModel_->topModel(i)->resourceID();
	}
	return names;
}


/**
 *	Override from PyAttachment
 */
void PyModel::detach()
{
	BW_GUARD;
	this->PyAttachment::detach();

	// reset our coupling fashion since we are no longer a couple :(
	pCouplingFashion_ = NULL;
}


/**
 *	This allows scripts to get various properties of a model
 */
ScriptObject PyModel::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	if (pSuperModel_)
	{
		// try to find the hard point named 'attr'
		const Moo::NodePtr pHardPoint = pSuperModel_->findNode(
			BW::string( "HP_" ) + attr );
		if (pHardPoint)
		{
			return ScriptObject(
				this->accessNode( pHardPoint )->pyGetHardPoint(),
				ScriptObject::FROM_NEW_REFERENCE );
		}

		// try to find the action named 'attr'
		SuperModelActionPtr smap = pSuperModel_->getAction( attr );
		if (smap)
		{
			return ScriptObject(
				new ActionQueuer( this, smap, NULL ),
				ScriptObject::FROM_NEW_REFERENCE );
		}

		// Try to find the dye matter or other fashion named 'attr'
		PyObject * pPyMaterialFashion = this->getMaterial( attr );
		if (pPyMaterialFashion)
		{
			return ScriptObject(
				pPyMaterialFashion,
				ScriptObject::FROM_BORROWED_REFERENCE );
		}

		PyObject * pPyTransformFashion = this->getTransform( attr );
		if (pPyTransformFashion)
		{
			return ScriptObject( 
				pPyTransformFashion,
				ScriptObject::FROM_BORROWED_REFERENCE );
		}

		SuperModelDyePtr pDye = pSuperModel_->getDye( attr, "" );
		if (pDye)	// some question as to whether we should do this...
		{	// but I want to present illusion that matters are always there
			PyMaterialFashion* pPyDye = new PyDye( pDye, attr, "Default" );
			pyMaterialFashionsMap_.insert( std::make_pair( attr, pPyDye ) );
			materialFashions_.push_back( pPyDye->materialFashion() );
			return ScriptObject( pPyDye, ScriptObject::FROM_NEW_REFERENCE );
		}
	}

	return PyAttachment::pyGetAttribute( attrObj );
}


/**
 *	This allows scripts to set various properties of a model
 */
bool PyModel::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;

	if (pSuperModel_)
	{
		const char * attr = attrObj.c_str();

		// try to find the hard point named 'attr'
		const Moo::NodePtr pHardPoint = pSuperModel_->findNode(
			BW::string( "HP_" ) + attr, NULL, true );
		if (pHardPoint)
		{
			return this->accessNode(
					pHardPoint )->pySetHardPoint( value.get() ) == 0;
		}

		// Check if it's a BW::PyMaterialFashion
		if (PyMaterialFashion::Check( value ))
		{
			// some fashions must be copied when set into models
			PyMaterialFashion * pPyMaterialFashion =
				static_cast< PyMaterialFashion* >( value.get() );

			// We always have to make a copy of the object here, because
			// it may have been created from a different model/supermodel.
			pPyMaterialFashion = static_cast< PyMaterialFashion* >(
				pPyMaterialFashion->makeCopy( this, attr ) );
			if (pPyMaterialFashion == NULL)
			{
				// makeCopy sets exception
				return false;
			}

			this->setMaterial( attr, pPyMaterialFashion );
			return true;
		}

		// Check if it's a PyTransformFashion
		if (PyTransformFashion::Check( value ))
		{
			// some fashions must be copied when set into models
			PyTransformFashion * pPyTransformFashion =
				static_cast< PyTransformFashion* >( value.get() );

			// We always have to make a copy of the object here, because
			// it may have been created from a different model/supermodel.
			pPyTransformFashion = static_cast< PyTransformFashion* >(
				pPyTransformFashion->makeCopy( this, attr ) );
			if (pPyTransformFashion == NULL)
			{
				// makeCopy sets exception
				return false;
			}

			this->setTransform( attr, pPyTransformFashion );
			return true;
		}

		// try to find the dye matter named 'attr'
		if (ScriptString::check( value ) || value.isNone())
		{
			const char * valStr = ( value.isNone() ? "" : 
					ScriptString( value ).c_str() );

			SuperModelDyePtr pDye = pSuperModel_->getDye( attr, valStr );
			if (pDye)	// only NULL if no such matter
			{
				PyMaterialFashion * newPyDye =
					new PyDye( pDye, attr, valStr );
				this->setMaterial( attr, newPyDye );
				return true;
			}
		}

		// check if it's none and in our list of fashions (and not a dye)
		if (value.isNone())
		{
			if (this->removeMaterial( attr ))
			{
				return true;
			}

			if (this->removeTransform( attr ))
			{
				return true;
			}
		}

	}

	return PyAttachment::pySetAttribute( attrObj, value );
}


/**
 *	This allows scripts to delete some properties of a model
 */
bool PyModel::pyDelAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	// currently only fashions can be deleted
	if (this->removeMaterial( attr ))
	{
		return true;
	}

	if (this->removeTransform( attr ))
	{
		return true;
	}

	return this->PyAttachment::pyDelAttribute( attrObj );
}


/**
*	Try to find the material fashion named 'attr'.
*	Does not increment the reference count.
 *	@param attr the name of the attribute to find the fashion for.
 *	@return the material fashion that corresponds to the given attribute
 *		name, otherwise NULL.
 */
BW::PyMaterialFashion * PyModel::getMaterial( const char * attr )
{
	BW_GUARD;
	PyMaterialFashions::iterator found = pyMaterialFashionsMap_.find( attr );
	if (found != pyMaterialFashionsMap_.end())
	{
		BW::PyMaterialFashion * pPyMaterialFashion = &*found->second;
		return pPyMaterialFashion;
	}
	return NULL;
}


/**
 *	Set the attribute 'attr' to the given material fashion.
 *	Decrements the reference count.
 *	@param attr the name of the attribute to add the fashion.
 *	@param pPyMaterialFashion pointer to the material fashion to add.
 *		Should not be NULL, use removeMaterial to remove materials.
 */
void PyModel::setMaterial( const char * attr,
	BW::PyMaterialFashion * pPyMaterialFashion )
{
	BW_GUARD;
	MF_ASSERT( pPyMaterialFashion != NULL );

	// Remove old material set to this attribute
	PyMaterialFashions::iterator found = pyMaterialFashionsMap_.find( attr );
	if (found != pyMaterialFashionsMap_.end())
	{
		found->second->disowned();
	}

	MaterialFashionPtr pMaterialFashion = pPyMaterialFashion->materialFashion();
	MaterialFashionVector::iterator found2 = std::find(
		materialFashions_.begin(),
		materialFashions_.end(),
		pMaterialFashion );

	if (found2 != materialFashions_.end())
	{
		materialFashions_.erase( found2 );
	}

	// Add new material to attribute
	pyMaterialFashionsMap_[ attr ] = pPyMaterialFashion;
	materialFashions_.push_back( pMaterialFashion );

	Py_DECREF( pPyMaterialFashion );
}


/**
 *	Set the material attribute 'attr' to None.
 *	@param attr the name of the attribute to remove the material fashion.
 */
bool PyModel::removeMaterial( const char * attr )
{
	BW_GUARD;
	PyMaterialFashions::iterator found = pyMaterialFashionsMap_.find( attr );
	if (found != pyMaterialFashionsMap_.end())
	{
		found->second->disowned();
		MaterialFashionPtr materialFashion = found->second->materialFashion();
		pyMaterialFashionsMap_.erase( found );

		MaterialFashionVector::iterator found2 = std::find(
			materialFashions_.begin(),
			materialFashions_.end(),
			materialFashion );

		if (found2 != materialFashions_.end())
		{
			materialFashions_.erase( found2 );
		}

		return true;
	}
	return false;
}


/**
 *	Try to find the transform fashion named 'attr'.
 *	Does not increment the reference count.
 *	@param attr the name of the attribute to find the fashion for.
 *	@return the transform fashion that corresponds to the given attribute
 *		name, otherwise NULL.
 */
PyTransformFashion * PyModel::getTransform( const char * attr )
{
	BW_GUARD;
	PyTransformFashions::iterator found =
		prePyTransformFashionsMap_.find( attr );
	if (found != prePyTransformFashionsMap_.end())
	{
		PyTransformFashion * pPyTransformFashion = &*found->second;
		return pPyTransformFashion;
	}

	found = postPyTransformFashionsMap_.find( attr );
	if (found != postPyTransformFashionsMap_.end())
	{
		PyTransformFashion * pPyTransformFashion = &*found->second;
		return pPyTransformFashion;
	}
	return NULL;
}


/**
 *	Set the attribute 'attr' to the given transform fashion.
 *	Decrements the reference count on the old attribute.
 *	@param attr the name of the attribute to add the fashion.
 *	@param pPyTransformFashion pointer to the transform fashion to add.
 *		Should not be NULL, use removeTransform to remove transforms. 
 */
void PyModel::setTransform( const char * attr,
	PyTransformFashion * pPyTransformFashion )
{
	BW_GUARD;
	MF_ASSERT( pPyTransformFashion != NULL );

	if (pPyTransformFashion->isPreTransform())
	{
		PyTransformFashions::iterator found =
			prePyTransformFashionsMap_.find( attr );
		if (found != prePyTransformFashionsMap_.end())
		{
			found->second->disowned();
		}
		prePyTransformFashionsMap_[ attr ] = pPyTransformFashion;
		Py_DECREF( pPyTransformFashion );
	}
	else
	{
		PyTransformFashions::iterator found =
			postPyTransformFashionsMap_.find( attr );
		if (found != postPyTransformFashionsMap_.end())
		{
			found->second->disowned();
		}
		postPyTransformFashionsMap_[ attr ] = pPyTransformFashion;
		Py_DECREF( pPyTransformFashion );
	}
}


/**
 *	Set the transform attribute 'attr' to None.
 *	@param attr the name of the attribute to remove the transform fashion.
 */
bool PyModel::removeTransform( const char* attr )
{
	BW_GUARD;

	PyTransformFashions::iterator found =
		prePyTransformFashionsMap_.find( attr );
	if (found != prePyTransformFashionsMap_.end())
	{
		found->second->disowned();
		prePyTransformFashionsMap_.erase( found );
		return true;
	}

	found = postPyTransformFashionsMap_.find( attr );
	if (found != postPyTransformFashionsMap_.end())
	{
		found->second->disowned();
		postPyTransformFashionsMap_.erase( found );
		return true;
	}

	return false;
}


/**
 *	Specialised get method for the 'sources' attribute
 */
PyObject * PyModel::pyGet_sources()
{
	BW_GUARD;
	int nModels = 0;
	if (pSuperModel_ != NULL)
		nModels = pSuperModel_->nModels();
	PyObject * pTuple = PyTuple_New( nModels );
	for (int i = 0; i < nModels; i++)
	{
		PyTuple_SetItem( pTuple, i, Script::getData(
			pSuperModel_->topModel(i)->resourceID() ) );
	}
	return pTuple;
}


/**
 *	Specialised get method for the 'position' attribute
 */
PyObject * PyModel::pyGet_position()
{
	BW_GUARD;
	// TODO: It may be nice to make a type so that this property can be modified
	// instead of being read-only.
	return Script::getReadOnlyData( this->worldTransform().applyToOrigin() );
}

/**
 *	Specialised set method for the 'position' attribute
 */
int PyModel::pySet_position( PyObject * value )
{
	BW_GUARD;
	Matrix	wM( this->worldTransform() );
	Vector3	newPos = wM.applyToOrigin();
	int ret = Script::setData( value, newPos, "position" );
	if (ret == 0)
	{
		wM.translation( newPos );
		this->worldTransform( wM );
	}

#if FMOD_SUPPORT
	// NB: event velocity WONT automatically update from setPosition because dTime is unknown
	if (pSounds_ &&
		!pSounds_->update( this->worldTransform().applyToOrigin(), this->worldTransform()[2], 0))
	{
		ERROR_MSG( "PyModel::pySet_position: "
			"Failed to update sound events\n" );
	}
#endif

	return ret;
}


/**
 *	Specialised get method for the 'scale' attribute
 */
PyObject * PyModel::pyGet_scale()
{
	BW_GUARD;
	Vector3	vScale(
		this->worldTransform().applyToUnitAxisVector(0).length(),
		this->worldTransform().applyToUnitAxisVector(1).length(),
		this->worldTransform().applyToUnitAxisVector(2).length() );
	// TODO: It may be nice to make a type so that this property can be modified
	// instead of being read-only.
	return Script::getReadOnlyData( vScale );
}

/**
 *	Specialised set method for the 'scale' attribute
 */
int PyModel::pySet_scale( PyObject * value )
{
	BW_GUARD;
	Matrix	wM( this->worldTransform() );
	Vector3	newScale;
	int ret = Script::setData( value, newScale, "scale" );
	if (ret == 0)
	{
		Vector3	vScale(
			wM.applyToUnitAxisVector(0).length(),
			wM.applyToUnitAxisVector(1).length(),
			wM.applyToUnitAxisVector(2).length() );

		Matrix	scaler;
		scaler.setScale(
			newScale[0]/vScale[0],
			newScale[1]/vScale[1],
			newScale[2]/vScale[2] );

		wM.preMultiply( scaler );
		this->worldTransform( wM );
	}
	return ret;
}


/**
 *	Specialised get method for the 'yaw' attribute
 */
float PyModel::yaw() const
{
	BW_GUARD;
	const Matrix & wM = this->worldTransform();
	return atan2f( wM.m[2][0], wM.m[2][2] );
}

/**
 *	Specialised set method for the 'yaw' attribute
 */
void PyModel::yaw( float yaw )
{
	BW_GUARD;
	Matrix	wM = this->worldTransform();
	wM.preRotateY( yaw - atan2f( wM.m[2][0], wM.m[2][2] ) );
	this->worldTransform( wM );
}


/**
 *	Specialised get method for the 'height' attribute
 */
float PyModel::height() const
{
	BW_GUARD;
	float height = height_;

	if (height_ <= 0.f)
	{
		if (pSuperModel_ != NULL)
		{
			BoundingBox bb( Vector3::zero(), Vector3::zero() );
			pSuperModel_->localBoundingBox( bb );
			height = bb.maxBounds().y - bb.minBounds().y;
		}
		else
		{
			height = 1.f;
		}
	}

	return height;
}

/**
 *	Specialised set method for the 'height' attribute. This does nothing at
 *	the moment.
 */
void PyModel::height( float height )
{
	BW_GUARD;
	height_ = height;
}



/**
 *	Simple function to turn a python sequence of strings into a vector
 *	of nodes, after checking that they're in the model.
 *	Yes, a get/setData could be made out of this but I can't be bothered.
 */
int findNodes( PyObject * pSeq, SuperModel * pSM,
	BW::vector<Moo::NodePtr> & nodes, const char * varName )
{
	BW_GUARD;
	nodes.clear();

	Py_ssize_t sz = PySequence_Size( pSeq );
	for (Py_ssize_t i = 0; i < sz; i++)
	{
		BW::string varNamePlus = varName;
		char nidbuf[32];
		bw_snprintf( nidbuf, sizeof( nidbuf ), " element %" PRIzd, i );
		varNamePlus += nidbuf;

		BW::string nodeName;
		if (Script::setData( &*SmartPointer<PyObject>( PySequence_GetItem(
			pSeq, i ), true ), nodeName, varNamePlus.c_str() ) != 0)
				return -1;

		Moo::NodePtr anode = pSM->findNode( nodeName );
		if (!anode)
		{
			PyErr_Format( PyExc_ValueError,
				"%s setter can't find node %s", varName, nodeName.c_str() );
			return -1;
		}
		nodes.push_back( anode );
	}

	return 0;
}



/**
 *	Model bounding box matrix provider class
 */
class ModelBBMProv : public MatrixProvider
{
public:
	ModelBBMProv( PyModel * pModel ) :
	  MatrixProvider( false, &s_type_ ),
	  pModel_( pModel ) { }

	~ModelBBMProv() { }

	virtual void matrix( Matrix & m ) const
	{
		BW_GUARD;
		if (!pModel_->isInWorld())
		{
			m = Matrix::identity;
			m[3][3] = 0.f;
			return;
		}

		BoundingBox bb = BoundingBox::s_insideOut_;
		pModel_->localBoundingBox( bb, true );
		m = pModel_->worldTransform();

		Vector3 bbMin = m.applyPoint( bb.minBounds() );
		m.translation( bbMin );

		Vector3 bbMax = bb.maxBounds() - bb.minBounds();
		m[0] *= bbMax.x;
		m[1] *= bbMax.y;
		m[2] *= bbMax.z;
	}

private:
	SmartPointer<PyModel>	pModel_;
};


/**
 *	Get the bounding box in a matrix provider
 */
PyObject * PyModel::pyGet_bounds()
{
	BW_GUARD;
	return new ModelBBMProv( this );
}


/*~ function PyModel.node
 *
 *	Returns the named node.  If the name is empty string, the root node is
 *  returned.  If the named node doesn't exist then an exception is generated.
 *	Using this method is not valid for blank models and will raise an exception.
 *
 *	@param	nodeName	A string specifying the name of the node to look up.
 *	@param	local		(optional) A local transform for the node.  This is a
 *						matrix provider that premultiplies itself with the
 *						node's world transform, effectively creating a new
 *						node to attach things to.
 *
 *	@return				The node with the specified name. ValueError is raised
 *						if the node does not exist.
 */
/**
 *	Get the given node.
 *
 *	Note: Returns a new reference!
 */
PyObject * PyModel::node( const BW::string & nodeName,
						 MatrixProviderPtr local )
{
	BW_GUARD;
	Moo::NodePtr pNode;

	if (pSuperModel_ == NULL)
	{
		MF_ASSERT( dummyModelNode_ != NULL );

		if (!nodeName.empty() && (nodeName != Moo::Node::SCENE_ROOT_NAME))
		{
			PyErr_Format( PyExc_ValueError, "PyModel.node: "
				"No node named %s in this Model.", nodeName.c_str() );
			return NULL;
		}

		pNode = dummyModelNode_->pNode();
	}
	else
	{
		MF_ASSERT( dummyModelNode_ == NULL );

		if (nodeName.empty())
		{
			pNode = pSuperModel_->findNode( Moo::Node::SCENE_ROOT_NAME );
		}
		else
		{
			pNode = pSuperModel_->findNode( nodeName );
		}

		if (!pNode)
		{
			PyErr_Format( PyExc_ValueError, "PyModel.node: "
				"No node named %s in this Model.", nodeName.c_str() );
			return NULL;
		}
	}

	PyModelNode * pPyNode = this->accessNode( pNode, local );
	Py_INCREF( pPyNode );
	return pPyNode;
}


/**
 *	Access the given node, creating it if necessary.
 */
PyModelNode * PyModel::accessNode( Moo::NodePtr pNode, MatrixProviderPtr local )
{
	BW_GUARD;
	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); ++nit)
	{
		bool sameNode = (*nit)->pNode() == pNode;
		bool sameLocal = (*nit)->localTransform() == local;
		if ( sameNode && sameLocal ) break;
	}
	if (nit == knownNodes_.end())
	{
		knownNodes_.push_back( new PyModelNode( this, pNode, local ) );
		nit = knownNodes_.end() - 1;
	}
	return *nit;
}


/*~ function PyModel.cue
 *	This method is not yet supported.
 *
 *	This method sets the default response for the given cue identifier.
 *
 *	A cue is a named frame inside an animation, that is exported with
 *	the model.  If this function is used to set up a callback function
 *	then anytime that named frame gets played, the function will be
 *	called.
 *
 *	For example, assuming that there was a cue named Impact in the swordSwing
 *	animation, which is played using the SwordSwing ActionQueuer, then the
 *	following example will print "Sword hit target" at that point in the
 *	animation.
 *
 *	@{
 *	def onImpact( t ):  print "Sword hit target at time", t
 *	biped.cue( "Impact", onImpact )
 *	biped.SwordSwing()
 *	@}
 *
 *	It raises a TypeError if the given function parameter is not callable. It
 *	raises a ValueError if the identifier is unknown.
 *
 *	@param	identifier	A String.  The cue identifier we are setting a
 *						response for.
 *	@param	function	A callable object.  The callback for that cue
 *						identifier.
 *
 */
/**
 *	This method sets the default response for the given cue identifier.
 *	Sets a python exception on error.
 */
bool PyModel::cue( const BW::string & identifier,
	SmartPointer<PyObject> function )
{
	return actionQueue_.cue( identifier, function );
}

/*~ function PyModel.saveActionQueue
 *
 *	This function is used to save the state of the pyModel's action queue.
 *	This useful when doing a change to the pyModel's supermodel which would involve
 *	reloading the supermodel and its animation and action states.
 *
 *	@param state DataSectionPtr to save the state to.
 */
PyObject * PyModel::py_saveActionQueue( PyObject * args )
{
	BW_GUARD;
	PyObject * pPyDS;
	if (!PyArg_ParseTuple( args, "O", &pPyDS) ||
		!PyDataSection::Check(pPyDS) )
	{
		PyErr_SetString( PyExc_TypeError, "PyModel.saveActionQueue() "
			"expects a PyDataSection" );
		return NULL;
	}

	DataSectionPtr state = static_cast<PyDataSection*>( pPyDS )->pSection();
	actionQueue_.saveActionState(state);

	Py_RETURN_NONE;
}

/*~ function PyModel.restoreActionQueue
 *
 *	This function is used to restore the state of the pyModel's action queue.
 *	This useful when doing a change to the pyModel's supermodel which would involve
 *	reloading the supermodel and its animation and action states.
 *
 *	@param state DataSectionPtr to restore the state from.
 *
 *	@return Returns True (1) whether the restore operation was able to completely restore the previous
 *			state of the action queue, False (0) otherwise. The restore operation may fail if an action that was in
 *			the actionQueue is no longer part of the refreshed supermodel.
 */
PyObject * PyModel::py_restoreActionQueue( PyObject * args )
{
	BW_GUARD;
	PyObject * pPyDS;
	if (!PyArg_ParseTuple( args, "O", &pPyDS) ||
		!PyDataSection::Check(pPyDS) )
	{
		PyErr_SetString( PyExc_TypeError, "PyModel.restoreActionQueue() "
			"expects a PyDataSection" );
		return NULL;
	}

	DataSectionPtr state = static_cast<PyDataSection*>( pPyDS )->pSection();
	return PyInt_FromLong( actionQueue_.restoreActionState( state, pSuperModel_, pOwnWorld_ ) );
}

/**
 *	Static python factory method
 */
PyObject * PyModel::pyNew( PyObject * args )
{
	BW_GUARD;
	// get the arguments
	BW::vector< BW::string > modelNames;
	Py_ssize_t sz = PyTuple_Size( args );
	Py_ssize_t i;
	for (i = 0; i < sz; i++)
	{
		PyObject * pItem = PyTuple_GetItem( args, i );	// borrowed
		if (!PyString_Check( pItem )) break;
		modelNames.push_back( PyString_AsString( pItem ) );
	}

	if (!i || i < sz)
	{
		PyErr_SetString( PyExc_TypeError,
			"Model(): expected a number (>0) of strings" );
		return NULL;
	}

	// ok, make the supermodel
	SuperModel * pSM = NULL;

	// unless a blank model is requested
	if (i != 1 || !modelNames[0].empty())
	{
		pSM = new SuperModel( modelNames );
		if (!pSM->nModels() || pSM->nModels() != sz)
		{
			PyErr_Format( PyExc_ValueError, "Model(): "
				"Only found %d out of %d models requested",
				pSM->nModels(), sz );

			bw_safe_delete(pSM);

			return NULL;
		}

		pSM->checkBB( false );
	}
	return new PyModel( pSM );
}

/*~ function PyModel.straighten
 *
 *	This method straightens a model, i.e. removes all rotations (including yaw)
 *  from it.  It also removes any scaling effects, leaving nothing but the
 *  translation part of the world matrix.
 *
 *	@return			None
 */
/**
 *	This method allows scripts to straighten a model,
 *	i.e. remove all rotations (including yaw) from it
 */
void PyModel::straighten()
{
	BW_GUARD;
	Vector3 offset = this->worldTransform().applyToOrigin();
	Matrix wM;
	wM.setTranslate( offset );
	this->worldTransform( wM );
}


/*~ function PyModel.rotate
 *
 *	This method rotates a model about an arbitrary axis (and optionally an
 *  arbitrary centre).  A centre may be specified, otherwise it defaults to
 *  (0,0,0).  The centre is relative to the origin of the model.
 *
 *	@param	angle		The angle to rotate the model through.
 *	@param	vAxis		A Vector3 axis about which to rotate the model.  This
 *						axis will pass through vCentre.
 *	@param	vCentre		A Vector3 through which vAxis passes, when the rotation
 *						about it is applied.  It defaults to (0,0,0).
 *
 *	@return				None
 */
/**
 *	This method allows scripts to rotate a model about an
 *	arbitrary axis (and optionally an arbitrary centre)
 */
void PyModel::rotate( float angle, const Vector3 & vAxis,
	const Vector3 & vCentre )
{
	BW_GUARD;
	Matrix wM = this->worldTransform();

	// push the centre through the transform
	Vector3 rotCentre;
	wM.applyVector( rotCentre, vCentre );

	// set the matrix position and remember the difference
	Vector3 offsetDiff = wM.applyToOrigin() - rotCentre;
	wM.translation( rotCentre );

	// make a quaternion for that rotation
	Quaternion rotor;
	rotor.fromAngleAxis( angle, vAxis );

	// turn it into a matrix and apply it to our world matrix
	Matrix poster;
	poster.setRotate( rotor );
	wM.postMultiply( poster );

	// and add back the model position
	wM.translation( wM.applyToOrigin() + offsetDiff );

	this->worldTransform( wM );
}


/*~ function PyModel.alignTriangle
 *
 *	This method aligns a model to the given triangle (specified as three points
 *  in world space, taken in a clockwise direction).  The resultant world
 *	matrix will be in the middle of the triangle, with its z-axis along the
 *	triangles normal.  If randomYaw is true, The model's y-axis will be random;
 *	otherwise it will be along the world's positive y-axis if the y component
 *	of the triangle's normal (in world space) is less than 0.95; otherwise
 *  along the world's positive x-axis.
 *
 *	For example:
 *	@{
 *	m.alignTriangle( (1, 0, 0), (-1, 0, 0), (0,0,1), 0)
 *	@}
 *	results in the model lying on its back, with its head pointing along the
 *	world's positive x-axis.
 *	Wheras
 *	@{
 *	m.alignTriangle( (1, 0, 0), (-1, 0, 0), (0,1,0), 0)
 *	@}
 *	results in the model standing upright, facing in the world's negative
 *	z-axis.
 *
 *	@param	vertex0		A Vector3 specifying the first point of the triangle to
 *						align to.
 *	@param	vertex1		A Vector3 specifying the second point of the triangle to
 *						align to.
 *	@param	vertex2		A Vector3 specifying the third point of the triangle to
 *						align to
 *	@param	randomYaw	A boolean specifying whether or not to apply a random
 *						yaw to the model.
 *
 *	@return				None
 */
/**
 *	This method allows scripts to align a model to the given triangle.
 *	Note the resultant matrix is in the middle of the triangle - you
 *	may want to post-set the position.
 *
 */
void PyModel::alignTriangle( const Vector3 & vertex0,
	const Vector3 & vertex1, const Vector3 & vertex2, bool randomYaw )
{
	BW_GUARD;
	Vector3 normal = (vertex2 - vertex0).crossProduct(vertex2 - vertex1);
	Vector3 center = (vertex0+vertex1+vertex2) / 3;

	Matrix wM = this->worldTransform();
	wM.lookAt( center, normal, fabsf(normal.y) < 0.95f ? Vector3(0.f,1.f,0.f) : Vector3(1.f,0.f,0.f) );
	wM.invert();

	if ( randomYaw )
	{
		Matrix rot;
		rot.setRotateZ( (float)(rand() % 360) * 0.01745f );
		wM.preMultiply( rot );
	}

	this->worldTransform( wM );
}


/*~ function PyModel.reflectOffTriangle
 *
 *	This method reflects the Vector3 specified by the fwd argument off the
 *  specified triangle, and places the model in the centre of the triangle
 *	facing in the direction of the reflected vector.  The model's vertical will
 *  be will be along the world's positive y-axis if the new unit forward vector
 *  has a world space y component less than 0.95, otherwise along the world's
 *  positive z-axis.
 *
 *
 *	@param	vertex0		A Vector3 specifying the first point, in world space,
 *						of the triangle to align to.
 *	@param	vertex1		A Vector3 specifying the second point, in world space,
 *						of the triangle to align to.
 *	@param	vertex2		A Vector3 specifying the third point, in world space,
 *						of the triangle to align to.
 *	@param	fwd			The vector, in world space, to reflect off the
 *						triangle, giving the new forward direction for the
 *						model.
 *
 *	@return				A Vector3, which is the new forward direction for the
 *						model in world space.
 */
/**
 *	This method allows scripts to align a model to the given triangle.
 *	Note the resultant matrix is in the middle of the triangle - you
 *	may want to post-set the position.
 */
Vector3 PyModel::reflectOffTriangle( const Vector3 & vertex0,
	const Vector3 & vertex1, const Vector3 & vertex2, const Vector3 & fwd )
{
	BW_GUARD;
	Vector3 edge[2];
	edge[0] = vertex2 - vertex0;
	edge[1] = vertex2 - vertex1;
	edge[0].normalise();
	edge[1].normalise();
	Vector3 normal = edge[0].crossProduct(edge[1]);
	Vector3 center = (vertex0+vertex1+vertex2) / 3;

	//now we have the normal, calc the reflection vector
	Vector3 reflection = (2.f*normal.dotProduct(fwd))*normal - fwd;

	Matrix wM = this->worldTransform();

	wM.lookAt( center, reflection, fabsf(reflection.y) < 0.95f ? Vector3(0.f,1.f,0.f) : Vector3(1.f,0.f,0.f) );
	wM.invert();

	this->worldTransform( wM );

	return reflection;
}



/*~ function PyModel.zoomExtents
 *
 *	This method places the model at the origin, scaled to fit within a 1 metre
 *  cube; that is, its longest dimension will be exactly 1 metre.  Its
 *	placement within the cube depends on upperFrontFlushness. If
 *	upperFrontFlushness is non-zero it moves the model to be flush with the top
 *  front of the cube, rather than the bottom.
 *
 *	You have the option of placing a model in a box is wider than it is tall.
 *	To do this, set the xzMultiplier to the (width / height) of the box.
 *
 *	@param	upperFrontFlushness	Whether to align with the top-front or the
 *			bottom-rear.
 *	@param	xzMultiplier ( default 1.0 ) for placing a model in a non-cubic box.
 *
 *	@return						None
 */
/**
 *	This method allows scripts to place a model at the origin,
 *	with a size equalling 1 metre cubed.
 */
void PyModel::zoomExtents( bool upperFrontFlushness, float xzMutiplier )
{
	BW_GUARD;
	BoundingBox bb = BoundingBox::s_insideOut_;
	this->localBoundingBox( bb, true );
	Vector3 extents( bb.maxBounds() - bb.minBounds() );
	//the xzMultiplier is a fudge that allows zoomExtents to deal with
	//non-cubic bounds
	extents.x /= xzMutiplier;
	extents.z /= xzMutiplier;
	float size = max( max( extents.z, extents.y ), extents.x );
	Matrix sc;
	sc.setScale( 1.f / size, 1.f / size, 1.f / size );
	Matrix tr;
	tr.setTranslate( Vector3(-bb.centre()) );
	tr.postMultiply( sc );

	if ( upperFrontFlushness )
	{
		//find half of the scaled size of the object.
		extents *= (1.f / (2.f*size));
		//find the smallest move out of y,z
		float move = min( 0.5f-extents.y,0.5f-extents.z );
		//and move it to the front/top of the unit cube.
		tr.postTranslateBy( Vector3(0,move,-move) );
		//note we find the min, because if we don't move it
		//exactly towards the camera, the resultant picture will
		//not be centered.
	}

	this->worldTransform( tr );
}

/*~ function PyModel.action
 *
 *	This method looks up an action by name from an internal list of actions and
 *  returns an ActionQueuer object if it finds it.  Otherwise a Python type error is thrown.
 *
 *	@param	actionName		A string which is the name of the action to search
 *							for.
 *
 *	@return					The ActionQueuer object named actionName.
 */
/**
 *	This method looks up an action by name and returns an ActionQueuer object
 *	if it finds it
 */
PyObject * PyModel::py_action( PyObject * args )
{
	BW_GUARD;
	// get the name
	char * actionName;
	if (!PyArg_ParseTuple( args, "s", &actionName ))
	{
		PyErr_SetString( PyExc_TypeError,
			"PyModel.action: Could not parse the arguments\n" );
		return NULL;
	}

	if (pSuperModel_)
	{
		// find the action
		SuperModelActionPtr smap = pSuperModel_->getAction( actionName );
		if (smap)
		{
			return new ActionQueuer( this, smap, NULL );
		}
	}

	// set the error if we couldn't find it
	PyErr_Format( PyExc_ValueError, "Model.action() "
		"could not find action %s", actionName );
	return NULL;
}


#if FMOD_SUPPORT
PySound* PyModel::createSoundInternal( const BW::string& s )
{
	BW_GUARD;
	if (s.empty())
	{
		PyErr_Format( PyExc_ValueError,
			"Can't play unnamed sound" );
		return NULL;
	}

	if (!inWorld_)
	{
		PyErr_Format( PyExc_ValueError,
			"Can't attach sounds when not in the world" );
		return NULL;
	}

    PySound *pySound = static_cast< PySound* >( SoundManager::instance().createPySound( s ) );

    if ( pySound != SoundManager::pyError() )
    {
        this->attachSound( pySound );
        pySound->setModel( this );
    }
	else
	{
		Py_DECREF( pySound );
		PyErr_Format( PyExc_ValueError,
			"Can't create sound named %s", s.c_str() );

		return NULL;
	}

	return pySound;
}
#endif // FMOD_SUPPORT

/*~ function PyModel.getSound
 *
 *	Returns a PySound for a 3D sound event at the location of the model.  The
 *	sound can subsequently be played by calling its play() method.
 *
 *  For sound naming semantics, please see FMOD.playSound().
 *
 *	@param	s				The name of the sound
 *
 *	@return					The PySound for this sound event
 */
PyObject* PyModel::getSound( const BW::string& s )
{
	BW_GUARD;
#if FMOD_SUPPORT	
    PySound * pySound = createSoundInternal( s );    
	return pySound;
#else
	ERROR_MSG( "PyModel::getSound: FMOD sound support is disabled.\n" );
	Py_RETURN_NONE;
#endif
}


/*~ function PyModel.playSound
 *
 *	Plays a 3D sound at the location of the model.
 *
 *  For sound naming semantics, please see FMOD.playSound().
 *
 *	@param	s				The name of the sound
 *
 *	@return					The PySound for this sound event
 */
PyObject* PyModel::playSound( const BW::string& s )
{
	BW_GUARD;

#if FMOD_SUPPORT
	PySound *pySound = this->createSoundInternal( s );

	if ( pySound && !pySound->play() )
	{
        PyErr_Format( PyExc_StandardError, "Model.playSound() "
				"Error starting sound %s", s.c_str() );
		Py_DECREF( pySound );
		return NULL;
	}

	return pySound;
#else
	ERROR_MSG( "PyModel::playSound: FMOD sound support is disabled.\n" );
	Py_RETURN_NONE;
#endif
}

#if FMOD_SUPPORT
/**
 *  Returns a reference to this model's sound event list, instantiating first if
 *  necessary.
 */
PySoundList& PyModel::sounds()
{
	BW_GUARD;
	if (pSounds_ == NULL)
		pSounds_ = new PySoundList();

	return *pSounds_;
}


/**
 *  Add the given sound event to this model's sound event list and set its 3D
 *  properties to correspond to this model.
 */
bool PyModel::attachSound( PySound *pySound )
{
	BW_GUARD;
	if ( pySound->update( this->worldTransform().applyToOrigin() ) )
	{
		this->sounds().push_back( pySound );
		return true;
	}
	else
		return false;
}

/**
* remove the pySound from pSounds_
*  
*/
bool PyModel::detachSound( PySound *pySound )
{
	if (pSounds_)
	{
		return pSounds_->removeSound( pySound );
	}
	else
	{
		return true;
	}
}


/**
* set its 3D properties to correspond to this model.
*  
*/
bool PyModel::updateSound( PySound *pySound )
{
	BW_GUARD;
	return pySound->update( this->worldTransform().applyToOrigin() );
}


void PyModel::cleanupSounds()
{
	BW_GUARD;
	if (pSounds_)
	{
		pSounds_->stopOnDestroy( cleanupSounds_ );
		bw_safe_delete(pSounds_);
	}
}
#endif // FMOD_SUPPORT

/*~ function PyModel.addMotor
 *
 *	This method adds the specified object to the model's list of Motors.
 *	All motors in this list are able to influence the movement and animation of
 *  the model based on the state of the entity which owns it.
 *
 *	@param	motor	A Motor which is to be added to the model
 *
 *	@return			None
 */
/**
 *	This method allows scripts to add a motor to a model.
 */
PyObject * PyModel::py_addMotor( PyObject * args )
{
	BW_GUARD;
	// parse args
	PyObject * pObject;
	if ( !PyArg_ParseTuple( args, "O", &pObject ) || !Motor::Check( pObject ) )
	{
		PyErr_SetString( PyExc_TypeError, "Model.addMotor() expects a Motor" );
		return NULL;
	}

	Motor * pMotor = (Motor*)pObject;

	// check that no-one owns it
	if (pMotor->pOwner() != NULL)
	{
		PyErr_SetString( PyExc_ValueError, "Model.addMotor() "
			"cannot add a Motor is already attached to a Model" );
		return NULL;
	}

	// take it over
	Py_INCREF( pMotor );
	pMotor->attach( this );
	motors_.push_back( pMotor );

	Py_RETURN_NONE;
}


/*~ function PyModel.delMotor
 *
 *	This method removes the specified object to the models list of Motors.
 *	All motors in this list are able to influence the movement and animation of
 *  the model based on the state of the entity which owns it.
 *
 *	@param	motor	A Motor which is to be removed from the model
 *
 *	@return			None
 */
/**
 *	This method allows scripts to remove a motor from a model.
 */
PyObject * PyModel::py_delMotor( PyObject * args )
{
	BW_GUARD;
	// parse args
	PyObject * pObject;
	if ( !PyArg_ParseTuple( args, "O", &pObject ) || !Motor::Check( pObject ) )
	{
		PyErr_SetString( PyExc_TypeError, "Model.delMotor() expects a Motor" );
		return NULL;
	}

	Motor * pMotor = (Motor*)pObject;

	// check that we own it
	if (pMotor->pOwner() != this)
	{
		PyErr_SetString( PyExc_ValueError, "Model.delMotor() "
			"was given a Motor that is not attached to this Model" );
		return NULL;
	}

	// remove it from our list
	for (uint i = 0; i < motors_.size(); i++)
	{
		if (motors_[i] == pMotor)
		{
			motors_.erase( motors_.begin() + i );
			i--;	// continue for safety (would use std::find o/wise)
		}
	}

	// and get rid of it
	pMotor->detach();
	Py_DECREF( pMotor );

	Py_RETURN_NONE;
}

/**
 *	Get the tuple of motors
 */
PyObject * PyModel::pyGet_motors()
{
	BW_GUARD;
	PyObject * pTuple = PyTuple_New( motors_.size() );

	for (uint i = 0; i < motors_.size(); i++)
	{
		Py_INCREF( motors_[i] );
		PyTuple_SetItem( pTuple, i, motors_[i] );
	}

	return pTuple;
}


/**
 *	Set the sequence of motors
 */
int PyModel::pySet_motors( PyObject * value )
{
	BW_GUARD;
	// first check arguments...
	if (!PySequence_Check( value ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Model.motors must be set to a sequence of Motors" );
		return -1;
	}

	// ... thoroughly
	bool bad = false;
	for (int i = 0; i < PySequence_Size( value ) && !bad; i++)
	{
		PyObject * pTry = PySequence_GetItem( value, i );

		if (!Motor::Check( pTry ))
		{
			PyErr_Format( PyExc_TypeError, "Element %d of sequence replacing "
				" Model.motors is not a Motor", i );
			bad = true;
		}
		else if (((Motor*)pTry)->pOwner() != NULL)
		{
			PyErr_Format( PyExc_ValueError, "Element %d of sequence replacing "
				"Model.motors is already attached to a Model", i );
			bad = true;
		}

		Py_DECREF( pTry );
	}
	if (bad) return -1;


	// let old motors go
	for (uint i = 0; i < motors_.size(); i++)
	{
		motors_[i]->detach();
		Py_DECREF( motors_[i] );
	}
	motors_.clear();

	// fit new ones
	for (int i = 0; i < PySequence_Size( value ) && !bad; i++)
	{
		Motor * pMotor = (Motor*)PySequence_GetItem( value, i );

		pMotor->attach( this );
		motors_.push_back( pMotor );
		// We keep the reference returned by PySequence_GetItem.
	}

	return 0;
}

/**
 *	This method gives models a chance to move during entity tick.
 *
 *	We can't move during scene tick because our movement might put
 *	us in a different node which would stuff up all the
 *	iterators. And it's nice to have all of these movements done at once.
 */
void PyModel::move( float dTime )
{
	BW_GUARD;
	// tell it where the sound source is (in case the motor triggers a sound)
	ScopedCurrentModelHolder current( *this );

	// optionally call move on known nodes.  This is optional because doing
	// it on all pymodels and all nodes can take a measurable chunk of time.	
	if (moveAttachments_)
	{	
		PyModelNodes::iterator nit;
		for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
		{
			(*nit)->move( dTime );
		}
	}	

	// purposely not using iterators for stability
	for (uint i = 0; i < motors_.size(); i++)
	{
		motors_[i]->rev( dTime );
	}

#if FMOD_SUPPORT
// 	// __glenc__: Disabling this, as tick() has this too
// 	if (pSounds_ &&
// 		!pSounds_->update( this->worldTransform().applyToOrigin() ))
// 	{
// 		ERROR_MSG( "PyModel::pySet_position: "
// 			"Failed to update sound events\n" );
// 	}
#endif
}


static DogWatch	watchAQ("Action Queue");
static DogWatch watchTracker("Tracker");


/**
 * This method updates the model for this frame.  Uses the
 * underlying Action Queue to update
 *
 * @param dTime is the change in time for this frame
 */
void PyModel::tick( float dTime )
{
	BW_GUARD_PROFILER( PyModel_tick );
	if (!transformModsDirty_)
	{
		unitRotation_ = 0.f;
		unitOffset_.setZero();
	}

	watchAQ.start();
	actionQueue_.tick( actionScale()*dTime, this );
	watchAQ.stop();

	if (pSuperModel_ != NULL)
	{
		watchTracker.start();
		float lastLod = pSuperModel_->lastLod();
		for (PyTransformFashions::iterator it =
			prePyTransformFashionsMap_.begin();
			it != prePyTransformFashionsMap_.end();
			++it)
		{
			it->second->tick( dTime, lastLod );
		}
		for (PyTransformFashions::iterator it =
			postPyTransformFashionsMap_.begin();
			it != postPyTransformFashionsMap_.end();
			++it)
		{
			it->second->tick( dTime, lastLod );
		}
		for (PyMaterialFashions::iterator it = pyMaterialFashionsMap_.begin();
			it != pyMaterialFashionsMap_.end();
			++it)
		{
			it->second->tick( dTime, lastLod );
		}
		watchTracker.stop();
	}

	for (int i = (int)knownNodes_.size() - 1; i >= 0; --i)
	{
		PyModelNode* node = knownNodes_[ i ];

		node->tick( dTime );

		if (node->refCount() == 1 && !node->hasAttachments())
		{
			knownNodes_[ i ] = knownNodes_.back();
			knownNodes_.pop_back();
			node->detach();
			Py_DECREF( node );
		}
	}

#if FMOD_SUPPORT
	SCOPED_DISABLE_FRAME_BEHIND_WARNING;
	const Matrix& matrix = this->worldTransform();
	if (pSounds_ &&
		!pSounds_->update( this->worldTransform().applyToOrigin(), matrix[2], dTime))
	{
		ERROR_MSG( "PyModel::tick: "
			"Failed to update sound events\n" );
	}
#endif
}


/**
 *	Check if this PyModel needs to be updated this frame.
 *	This check's the SuperModel (or dummy if we are empty) and
 *	any attachments.
 *	
 *	A PyModel needs to be updated if:
 *		1. it has been moved during tick so the dirty flag is set
 *		2. its SuperModel or dummy model needs to update its animation
 *		3. any of its attachments need updating
 *
 *	Note: invisible models still have their animations updated because you can
 *	make invisible objects to attach things to them.
 *	
 *	@return true if we have not been updated this frame or false if
 *		we don't need to. Eg. if we were already updated or have lodded.
 */
bool PyModel::needsUpdate() const
{
	BW_GUARD;

	if (dirty_)
	{
		return true;
	}
	if (pSuperModel_ != NULL)
	{
		// SuperModel always needs updating, to re-calculate its lod
		if (pSuperModel_->needsUpdate())
		{
			return true;
		}
		// Otherwise check attachments
	}
	else
	{
		MF_ASSERT( dummyModelNode_ != NULL );
		if (dummyModelNode_->needsUpdate())
		{
			return true;
		}
		// Otherwise check attachments
	}

	// Check if any attachments need updating
	for (PyModelNodes::const_iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		++nit)
	{
		const PyModelNode* pNode = (*nit);
		if (pNode->needsUpdate())
		{
			return true;
		}
	}

	return false;
}


/**
 *	Check if this PyModel and its attachments have visible lods.
 *	@return true if it is visible, false if it is lodded out.
 */
bool PyModel::isLodVisible() const
{
	BW_GUARD;

	if (pSuperModel_ != NULL)
	{
		if (pSuperModel_->isLodVisible())
		{
			return true;
		}
		// Otherwise check attachments
	}
	else
	{
		MF_ASSERT( dummyModelNode_ != NULL );
		if (!dummyModelNode_->isLodded() &&
			!dummyModelNode_->attachments().empty())
		{
			return true;
		}
		// Otherwise check attachments
	}

	// Check if any attachments need updating
	for (PyModelNodes::const_iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		++nit)
	{
		const PyModelNode* pNode = (*nit);
		if (!pNode->isLodded() &&
			!pNode->attachments().empty())
		{
			return true;
		}
	}
	return false;
}


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	Check if this PyModel needs to be updated this frame.
 *	This check's the SuperModel (or dummy if we are empty) and
 *	any attachments.
 *	
 *	A PyModel needs to be updated if:
 *		1. it has been moved during tick so the dirty flag is set
 *		2. its SuperModel or dummy model needs to update its animation
 *		3. any of its attachments need updating
 *	
 *	@return true if we have not been updated this frame or false if
 *		we don't need to. Eg. if we were already updated or have lodded.
 */
bool PyModel::isTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;

	if (dirty_)
	{
		return true;
	}
	if (pSuperModel_ != NULL)
	{
		if (pSuperModel_->isTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
		// Otherwise check attachments
	}
	else
	{
		MF_ASSERT( dummyModelNode_ != NULL );
		if (dummyModelNode_->isTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
		// Otherwise check attachments
	}

	// Check if any attachments need updating
	for (PyModelNodes::const_iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		++nit)
	{
		const PyModelNode* pNode = (*nit);
		if (pNode->isTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
	}

	return false;
}


/**
 *	Check if this PyModel needs to be updated this frame.
 *	This check's the SuperModel (or dummy if we are empty) and
 *	any attachments.
 *	
 *	A PyModel needs to be updated if:
 *		1. it has been moved during tick so the dirty flag is set
 *		2. its SuperModel or dummy model needs to update its animation
 *		3. any of its attachments need updating
 *
 *	Use this check when checking for if we need to update.
 *	ie. want to update lods.
 *	
 *	@return true if we have not been updated this frame or false if
 *		we don't need to. Eg. if we were already updated or have lodded.
 */
bool PyModel::isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;

	if (dirty_)
	{
		return true;
	}
	if (pSuperModel_ != NULL)
	{
		if (pSuperModel_->isVisibleTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
		// Otherwise check attachments
	}
	else
	{
		MF_ASSERT( dummyModelNode_ != NULL );
		if (dummyModelNode_->isVisibleTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
		// Otherwise check attachments
	}

	// Check if any attachments need updating
	for (PyModelNodes::const_iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		++nit)
	{
		const PyModelNode* pNode = (*nit);
		if (pNode->isVisibleTransformFrameDirty( frameTimestamp ))
		{
			return true;
		}
	}
	return false;
}
#endif // ENABLE_TRANSFORM_VALIDATION


/**
 *	Update this PyModel's animations and then update its attachments'
 *	animations.
 *
 *	Note: invisible models still have their animations updated because you can
 *		make invisible objects to attach things to them.
 *	
 *	@param worldTransform the world transform to use for animating.
 *	@param lod specify this to use a specific LOD, use -1 to calculate
 *		automatically.
 */
void PyModel::updateAnimations( const Matrix & worldTransform,
	float lod )
{
	BW_GUARD_PROFILER( PyModel_update );
	static DogWatch pyModelWatch( "PyModel updateAnimations" );
	ScopedPyModelWatchHolder watch( pyModelWatch );

	TRANSFORM_ASSERT( frameTransformCache_.transformFrameTimestamp_ !=
		Moo::rc().frameTimestamp() );

#if ENABLE_TRANSFORM_VALIDATION
	frameTransformCache_.transformFrameTimestamp_ = Moo::rc().frameTimestamp();
#endif // ENABLE_TRANSFORM_VALIDATION

	frameTransformCache_.attachmentsTransform_ = worldTransform;

	// For hardpoints
	frameTransformCache_.didAnimate_ = this->animateAsAttachment();
	if (pCouplingFashion_ && !frameTransformCache_.didAnimate_)
	{
		frameTransformCache_.attachmentsTransform_.preMultiply(
			pCouplingFashion_->staticInverse() );
	}
	frameTransformCache_.thisFrameCouplingFashion_ =
		frameTransformCache_.didAnimate_ ? &*pCouplingFashion_ : NULL;

	// Change the world transform appropriately
	frameTransformCache_.thisTransform_ =
		frameTransformCache_.attachmentsTransform_;
	{
		Matrix mOffset = unitTransform_;
		mOffset.postTranslateBy( unitOffset_ );
		mOffset.postRotateY( unitRotation_ );

		frameTransformCache_.thisTransform_.preMultiply( mOffset );
	}

	ScopedCurrentModelHolder current( *this );

	TRANSFORM_ASSERT( this->isTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );

	if (independentLod_)
	{
		// setting this to force a recompute of the lod
		lod = Model::LOD_AUTO_CALCULATE;
	}

	this->updateAsAttachment( frameTransformCache_.thisTransform_,
		frameTransformCache_.thisFrameCouplingFashion_,
		lod );

	// draw all our attachments
	this->updateAttachments( frameTransformCache_.attachmentsTransform_, lod );

	TRANSFORM_ASSERT( !this->isVisibleTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );
}


/**
 *	This method draws the model.
 *	@param worldTransform the world transform to draw the model at.
 */
void PyModel::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
	BW_GUARD_PROFILER( PyModel_draw );

	if (!this->visible() && !this->visibleAttachments())
	{
		return;
	}

	if (!this->isLodVisible())
	{
		return;
	}

	if (Moo::rc().reflectionScene())
	{
		MF_ASSERT_DEV( WaterSceneRenderer::currentScene() );
		if ( WaterSceneRenderer::currentScene() &&
			WaterSceneRenderer::currentScene()->shouldReflect(
				worldTransform.applyToOrigin(), this ) )
		{
			WaterSceneRenderer::incReflectionCount();
		}
		else
		{
			return;
		}
	}

	ScopedCurrentModelHolder current( *this );

	TRANSFORM_ASSERT( frameTransformCache_.transformFrameTimestamp_ ==
		Moo::rc().frameTimestamp() );

	// Model must have either:
	// - had lod and transforms updated and frame timestamp set
	// - had lod updated and not transforms because it has lodded out
	TRANSFORM_ASSERT( !this->isVisibleTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );

	this->drawAsAttachment( drawContext, frameTransformCache_.thisTransform_ );

	// draw all our attachments
	this->drawAttachments( drawContext, frameTransformCache_.attachmentsTransform_ );

	if (pDebugInfo_ != NULL)
	{
		this->drawDebugInfo( drawContext );
	}
}


/**
 *	Accumulate our visibilty box into the given one
 */
void PyModel::worldVisibilityBox( BoundingBox & vbb, const Matrix& worldTransform, bool skinny )
{
	BW_GUARD;
	if ((!this->visible()) && (!this->visibleAttachments())) return;

	BoundingBox bb(localVisibilityBox_);
	bb.transformBy(worldTransform);
	vbb.addBounds(bb);

	if (skinny || knownNodes_.empty()) return;

	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
	{
		(*nit)->worldBoundingBox(vbb, worldTransform);
	}
}


/**
 *	Accumulate our bounding box into the given one
 */
void PyModel::worldBoundingBox( BoundingBox & wbb, const Matrix& worldTransform, bool skinny )
{
	BW_GUARD;	

	BoundingBox bb;
	this->localBoundingBoxInternal(bb);
	bb.transformBy(worldTransform);
	wbb.addBounds(bb);

	if (skinny || knownNodes_.empty()) return;

	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
	{
		(*nit)->worldBoundingBox(wbb, worldTransform);
	}
}


/**
 *	Accumulate our bounding box into the given one
 */
void PyModel::localBoundingBox( BoundingBox & lbb, bool skinny )
{
	BW_GUARD;

	BoundingBox bb;
	this->localBoundingBoxInternal(bb);
	lbb.addBounds(bb);

	if (skinny || knownNodes_.empty()) return;

	//Accumulate our children in world space
	Matrix world( this->worldTransform() );
	BoundingBox wbb;
	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
	{
		(*nit)->worldBoundingBox(wbb, this->worldTransform());
	}

	//then convert back to local space.
	if (!wbb.insideOut())
	{
		world.invert();
		wbb.transformBy(world);
		lbb.addBounds(wbb);
	}
}


void PyModel::localVisibilityBox( BoundingBox & lbb, bool skinny )
{	
	BW_GUARD;
	if ((!this->visible()) && (!this->visibleAttachments())) return;

	lbb.addBounds(localVisibilityBox_);

	if (skinny || knownNodes_.empty()) return;

	//Accumulate our children in world space, then convert back to local space.
	SCOPED_DISABLE_FRAME_BEHIND_WARNING;
	Matrix world( this->worldTransform() );
	BoundingBox wbb;
	PyModelNodes::iterator nit;
	for (nit = knownNodes_.begin(); nit != knownNodes_.end(); nit++)
	{
		(*nit)->worldBoundingBox(wbb, this->worldTransform());
	}
	if (!wbb.insideOut())
	{
		world.invert();
		wbb.transformBy(world);
		lbb.addBounds(wbb);
	}
}





bool PyModel::drawSkeletons_  = false;
bool PyModel::drawSkeletonWithCoordAxes_ = false;
bool PyModel::drawNodeLabels_ = false;

/*~ function BigWorld.debugModel
 *
 * This function is used to turn on/off the drawing of the models
 * skeleton. This can also be done through the watcher located in
 * Render/Draw Skeletons
 */
/**
 *	Python debug setting method
 */
void debugModel(bool b)
{
	PyModel::drawSkeletons_ = b;
}

PY_AUTO_MODULE_FUNCTION( RETVOID, debugModel, ARG( bool, END ), BigWorld )


/**
 *	draws the skeleton of this model
 */
void PyModel::drawSkeleton()
{
	BW_GUARD;	
#if ENABLE_DRAW_SKELETON

	if (!drawSkeletons_ && !shouldDrawSkeleton_)
	{
		return;
	}

	BoundingBox bb;
	this->localBoundingBoxInternal(bb);
	Matrix m = Moo::rc().world();
	m.postMultiply( Moo::rc().viewProjection() );
	bb.calculateOutcode( m );
	if (bb.combinedOutcode())
	{
		return;
	}

	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	// draw skeleton for each model
	for (int i=0; i<pSuperModel_->nModels(); ++i)
	{
		Model * model = pSuperModel_->curModel( i );
		if (model != NULL)
		{
			NodeTreeIterator it  = model->nodeTreeBegin();
			NodeTreeIterator end = model->nodeTreeEnd();
			while (it != end)
			{
				// skip leaf nodes
				BW::string nodeName = it->pData->pNode->identifier();
				if ( ( nodeName.substr(0, 3) != BW::string("HP_") ) && ( nodeName.find("BlendBone", 0) == nodeName.npos) )
				{
					Vector3	fromPos = it->pParentTransform->applyToOrigin();
					const Matrix & nodeTrans = it->pData->pNode->worldTransform();
					Vector3 toPos = nodeTrans.applyToOrigin();

					// draw arrow body
					Geometrics::drawLine(
						fromPos, toPos, Moo::PackedColours::GREEN );

					// draw arrow head
					Vector3 vDir  = (toPos - fromPos);
					vDir.normalise();
					Vector3 ortog = vDir.crossProduct( nodeTrans[1] );
					ortog.normalise();
					static const float length = 0.02f;
					Geometrics::drawLine(
						toPos, toPos - vDir * length + ortog * length,
						Moo::PackedColours::RED );
					Geometrics::drawLine(
						toPos, toPos - vDir * length - ortog * length,
						Moo::PackedColours::RED );

					//Draw local coordinate axes.
					if (drawSkeletonWithCoordAxes_)
					{
						const float axisSize = 0.05f;
						Geometrics::drawAxesCentreColoured(
							nodeTrans, axisSize );
					}
				}
				it++;
			}
		}
	}
	Moo::rc().pop();

#endif
}

/**
 *	draws the labels of each node in this model's skeleton
 */
void PyModel::drawNodeLabels( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	if (!drawNodeLabels_)
	{
		return;
	}

	BoundingBox bb;
	this->localBoundingBoxInternal(bb);
	Matrix m = Moo::rc().world();
	m.postMultiply( Moo::rc().viewProjection() );
	bb.calculateOutcode( m );
	if (bb.combinedOutcode())
	{
		return;
	}

	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	Labels * nodesIDs = new Labels;
	for (int i=0; i<pSuperModel_->nModels(); ++i)
	{
		Model * model = pSuperModel_->curModel( i );
		if (model != NULL)
		{
			NodeTreeIterator it  = model->nodeTreeBegin();
			NodeTreeIterator end = model->nodeTreeEnd();
			while (it != end)
			{
				const Matrix & nodeTrans = it->pData->pNode->worldTransform();
				Vector3 nodePos = nodeTrans.applyToOrigin();
				nodesIDs->add(it->pData->pNode->identifier(), nodePos);
				it++;
			}
		}
	}
	drawContext.drawUserItem( nodesIDs,
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, 0.0f );
	Moo::rc().pop();
}

/**
 *	draws the debug info of this model
 */
void PyModel::drawDebugInfo( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	if (pDebugInfo_ == NULL || pDebugInfo_->empty())
	{
		return;
	}

	BoundingBox bb;
	this->localBoundingBoxInternal(bb);
	Matrix m = Moo::rc().world();
	m.postMultiply( Moo::rc().viewProjection() );
	bb.calculateOutcode( m );
	if (bb.combinedOutcode())
	{
		return;
	}

	Vector3 position = this->worldTransform().applyToOrigin() + 
						Vector3(0, this->height() + 0.1f, 0);

	StackedLabels * labels = new StackedLabels(position);

	for (DebugStrings::iterator i = pDebugInfo_->begin();
		i != pDebugInfo_->end();
		i++)
	{
		labels->add( BW::string(i->first) + ": " + i->second );
	}

	pDebugInfo_->clear();

	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	drawContext.drawUserItem( labels,
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, 0.0f );
	Moo::rc().pop();
}



/**
 * animates this model as a single attachment
 *
 * @return the result from actionQueue.effect(), which
 *  is true if the action queue did any animating
 */
bool PyModel::animateAsAttachment()
{
	BW_GUARD;
	// Get the action queue to do its stuff and animate the model
	watchAQ.start();
	bool ret = actionQueue_.effect();
	watchAQ.stop();

	return ret;
}


/**
 *	Updates this PyModel, but not its attachments.
 *	@param pCouplingFashion a tracker or NULL if there isn't one.
 *	@param lod specify this to use a specific LOD, use -1 to calculate
 *		automatically.
 */
void PyModel::updateAsAttachment( const Matrix& thisTransform,
	TransformFashion * pCouplingFashion,
	float lod )
{
	BW_GUARD;

	if (pSuperModel_ != NULL)
	{
		this->updateSuperModelAsAttachment( thisTransform,
			pCouplingFashion,
			lod );
	}
	else
	{
		this->updateDummyNodeAsAttachment( thisTransform, lod );
	}

	// Update some state
	transformModsDirty_ = false;
	dirty_ = false;
}


/**
 *	Update this PyModel's SuperModel's animations.
 *	@param pCouplingFashion a tracker or NULL if there isn't one.
 *	@param lod specify this to use a specific LOD, use -1 to calculate
 *		automatically.
 */
void PyModel::updateSuperModelAsAttachment( const Matrix& thisTransform,
	TransformFashion * pCouplingFashion,
	float lod )
{
	BW_GUARD;
	MF_ASSERT( pSuperModel_ != NULL );

	TmpTransforms preTransformFashions;
	TmpTransforms postTransformFashions;
	this->collectTransformFashions( preTransformFashions,
		postTransformFashions,
		pCouplingFashion );

	pSuperModel_->updateAnimations( thisTransform,
		&preTransformFashions,
		&postTransformFashions,
		lod );
}


/**
 *	This method places all of the transforms currently applying to this PyModel
 *	into some vectors, ready for animating.
 *	@param preTransformFashions place to put the animations currently applying
 *		to the PyModel.
 *	@param postTransformFashions place to put the trackers currently applying
 *		to the PyModel.
 *	@param pCouplingFashion the coupling fashion or NULL if there isn't one.
 */
void PyModel::collectTransformFashions(
	TmpTransforms& preTransformFashions,
	TmpTransforms& postTransformFashions,
	TransformFashion* pCouplingFashion )
{
	BW_GUARD;

	// Temporarily add action queue
	for (TransformFashionVector::const_iterator itr =
		actionQueue_.transformFashions().begin();
		itr != actionQueue_.transformFashions().end();
		++itr)
	{
		MF_ASSERT( (*itr).hasObject() );
		preTransformFashions.push_back( (*itr).get() );
	}

	// Temporarily add animators
	for (PyTransformFashions::const_iterator fit =
		prePyTransformFashionsMap_.begin();
		fit != prePyTransformFashionsMap_.end();
		++fit)
	{
		if (!fit->second->transformFashion().exists())
		{
			continue;
		}

		MF_ASSERT( fit->second->transformFashion().hasObject() );
		MF_ASSERT( !fit->second->transformFashion()->needsRedress() );
		preTransformFashions.push_back(
			fit->second->transformFashion().get() );
	}

	// Trackers
	for (PyTransformFashions::const_iterator fit =
		postPyTransformFashionsMap_.begin();
		fit != postPyTransformFashionsMap_.end();
		++fit)
	{
		if (!fit->second->transformFashion().exists())
		{
			continue;
		}

		if (fit->second->isAffectingTransformThisFrame())
		{
			postTransformFashions.push_back(
				fit->second->transformFashion().get() );
		}
	}

	//
	// And temporarily add any passed in fashion
	if (pCouplingFashion != NULL)
	{
		postTransformFashions.push_back( pCouplingFashion );
	}
}


/**
 *	This method updates our dummy node if we are an empty PyModel.
 *	@param world the world transform.
 *	@param lod specify this to use a specific LOD, use -1 to calculate
 *		automatically.
 */
void PyModel::updateDummyNodeAsAttachment( const Matrix & world,
	float lod )
{
	BW_GUARD;
	MF_ASSERT( dummyModelNode_ != NULL );

	// Always reset scene root
	dummyModelNode_->pNode()->blendClobber(
		Model::blendCookie(), Matrix::identity );
	dummyModelNode_->pNode()->visitSelf( world );
	dummyModelNode_->latch( world );

	dummyModelNode_->updateAnimations( lod );
}


/**
 *	Draws this model as a single attachment,
 *	using a primed render context transform
 *	Does not draw this PyModel's attachments.
 */
void PyModel::drawAsAttachment( Moo::DrawContext& drawContext,
							   const Matrix& thisTransform )
{
	BW_GUARD;

	if (!this->visible())
	{
		return;
	}

	if (pSuperModel_ != NULL)
	{
		this->drawSuperModelAsAttachment( drawContext, thisTransform );
	}
	else
	{
		this->drawDummyModelAsAttachment( drawContext );
	}
}


/**
 *	Apply materials and draw this PyModel's SuperModel.
 */
void PyModel::drawSuperModelAsAttachment( Moo::DrawContext& drawContext,
										 const Matrix& thisTransform )
{
	BW_GUARD;
	MF_ASSERT( pSuperModel_ != NULL );

	// Change the RenderContext's world transform appropriately
	Moo::rc().push();
	Moo::rc().world( thisTransform );

	// This object ID is setup for masking all the
	// dynamic objects to turn off the foam effect in the
	// water. This solution feels kinda hackish but a better
	// solution would require too many framework changes.
	// (best left to a future version).
	Moo::rc().effectVisualContext().currentObjectID(1.f);

	pSuperModel_->draw( drawContext, Moo::rc().world(), &materialFashions_ );

	Moo::rc().effectVisualContext().currentObjectID(0.f);

	this->drawSkeleton();
	this->drawNodeLabels( drawContext );

	Moo::rc().pop();
}


/**
 *	If this PyModel is empty, tell its dummy node to draw its attachments.
 */
void PyModel::drawDummyModelAsAttachment( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	MF_ASSERT( dummyModelNode_ != NULL );
	dummyModelNode_->draw( drawContext );
}


/**
 *	Constructor
 */
ModelAligner::ModelAligner( SuperModel * pSuperModel, Moo::NodePtr pNode ) :
	pNode_( pNode )
{
	BW_GUARD;
	if (pSuperModel != NULL)
	{
		pSuperModel->dressInDefault();
		staticInverse_.invert( pNode->worldTransform() );
	}
}


/**
 *	Fashion dress method, only used when model is animated.
 */
void ModelAligner::dress( SuperModel & superModel, Matrix& world )
{
	BW_GUARD;
	// first invert world transform of node
	Matrix m;
	m.invert( pNode_->worldTransform() );

	// now premultiply by world transform of root node
	// (could avoid by setting world to identity earlier...
	// but in the scope of things this is not our biggest problem)
	m.preMultiply( world );

	// now premultiply the world by this calculated transform
	world.preMultiply( m );
}



/*~ function PyModel.origin
 *
 *	Make the named node the origin of the model's coordinate system, if a node
 *  with that name exists.  If it does, return 1, otherwise 0.
 *
 *	@param nodeName	The name of the node to make the origin
 *
 */
/**
 *	Make the given node the origin of the model's coordinate system.
 *	If there is no such node, then an error is logged and false is returned
 *
 *	@note	Sets python error when it returns false.
 */
bool PyModel::origin( const BW::string & nodeName )
{
	BW_GUARD;
	if (pSuperModel_ == NULL)
	{
		PyErr_SetString( PyExc_TypeError,
			"Model.origin() is not supported for blank models" );
		return false;
	}

	Moo::NodePtr pNode = pSuperModel_->findNode( nodeName );
	if (!pNode)
	{
		PyErr_Format( PyExc_ValueError, "PyModel.origin(): "
			"No node named %s in this Model.", nodeName.c_str() );
		return false;
	}

	pCouplingFashion_ = NULL;

	if (!pNode->isSceneRoot())
	{
		pCouplingFashion_ = new ModelAligner( pSuperModel_, pNode );
	}

	return true;
}


/**
 * Update attached models.
 *
 * Uses the cached objectToCamera transform from the hard point node
 * to prime the render context before calling drawAttachment
 */
void PyModel::updateAttachments( const Matrix & attachmentsTransform,
	float lod )
{
	BW_GUARD;

	// If lod is set to calculate LOD automatically
	if (isEqual( lod, Model::LOD_AUTO_CALCULATE ))
	{
		if (pSuperModel_ != NULL)
		{
			lod = pSuperModel_->lastLod();
		}
		else
		{
			lod = LodSettings::instance().calculateLod( attachmentsTransform );
		}
	}

	for (PyModelNodes::iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		++nit)
	{
		PyModelNode* pNode = (*nit);

		// Reset node for the attachment (if updateAsAttachment() has changed
		// the blend cookie)
		// Use the transform that was calculated during the
		// SuperModel animation update which sets all of the nodes/hardpoints
		// in position (SuperModel::dress)
		if (pNode->pNode()->isSceneRoot())
		{
			pNode->pNode()->blendClobber(
				Model::blendCookie(), Matrix::identity );
			pNode->pNode()->visitSelf( attachmentsTransform );
		}
		else
		{
			// Only reset node for the attachment if updateAsAttachment() has
			// changed the blend cookie)
			if (pNode->pNode()->needsReset( Model::blendCookie() ))
			{
				pNode->pNode()->blendClobber( Model::blendCookie(),
					attachmentsTransform );
			}
		}
		pNode->latch( attachmentsTransform );

		pNode->updateAnimations( lod );
	}
}


/**
 * Draws attached models.
 *
 * Uses the cached objectToCamera transform from the hard point node
 * to prime the render context before calling drawAttachment
 */
void PyModel::drawAttachments( Moo::DrawContext& drawContext, const Matrix& attachmentsTransform )
{
	BW_GUARD;
 	if (!this->visible() && !this->visibleAttachments())
 	{
 		return;
 	}

	Moo::rc().push();
	Moo::rc().world( attachmentsTransform );

	for (PyModelNodes::iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		++nit)
	{
		PyModelNode* pNode = (*nit);
		pNode->draw( drawContext );
	}

	Moo::rc().pop();
}


/**
 *	This function lets the model know that it's now in the
 *	world and thus its tick function will start being called.
 */
void PyModel::enterWorld()
{
	BW_GUARD;
	this->PyAttachment::enterWorld();

	// flush out the action queue
	actionQueue().flush();

	// tell all our attachments the same
	for (PyModelNodes::iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		nit++)
	{
		(*nit)->enterWorld();
	}
}



/**
 *	This function lets the model know that it's no longer in the
 *	world and thus its tick function will stop being called.
 */
void PyModel::leaveWorld()
{
	BW_GUARD;
	this->PyAttachment::leaveWorld();

	// flush out the action queue
	actionQueue().flush();

	// tell all our attachments the same
	for (PyModelNodes::iterator nit = knownNodes_.begin();
		nit != knownNodes_.end();
		nit++)
	{
		(*nit)->leaveWorld();
	}

#if FMOD_SUPPORT
	// stop all sounds
	this->cleanupSounds();
#endif
}


/**
 *	This method retrieves our local bounding box, adjusting for
 *	the python-overridable height attribute.
 */
void PyModel::localBoundingBoxInternal( BoundingBox& bb ) const
{
	if (pSuperModel_ != NULL)
		pSuperModel_->localBoundingBox( bb );
	else
		bb = localBoundingBox_;

	if (height_ > 0.f)
	{
		Vector3 maxBounds = bb.maxBounds();
		maxBounds.y = bb.minBounds().y + height_;
		bb.setBounds( bb.minBounds(), maxBounds );
	}
}




static WeakPyPtr<PyModel> s_dpm;

/**
 *	Static method to do any debug stuff
 */
void PyModel::debugStuff( float dTime )
{
	BW_GUARD;	
#if ENABLE_ACTION_QUEUE_DEBUGGER
	if (s_dpm.hasObject())
	{
		PyModel* model = s_dpm.get();

		if (!model)
		{
			s_dpm = NULL;

			return;
		}

		if (!model->isInWorld())
		{
			return;
		}

		model->actionQueue().debugTick( dTime );
		model->actionQueue().debugDraw();
	}
#endif
}


const Matrix & PyModel::worldTransform() const
{
	return PyAttachment::worldTransform();
}


void PyModel::worldTransform( const Matrix & worldTransform )
{
	PyAttachment::worldTransform( worldTransform );
	dirty_ = true;
}


namespace
{
	/**
	 *	Used by getIKConstraintSystemFromData. On success, does one of
	 *	two things:
	 *	 - Adds a constraint from @a data to @a ikcs.
	 *	 - Found that the constraint needs a constraint that's not yet in
	 *		@a ikcs. Added a pointer to @a data into @a delayedAdd.
	 *	
	 *	@return True on success, false on failure (due to identifier
	 *		for @a data already existing in @a ikcs). On false return, printed
	 *		a warning.
	 */
	bool addConstraintFromData(
		IKConstraintSystem & ikcs, const Moo::ConstraintData & data,
		BW::vector<const Moo::ConstraintData *> & delayedAdd )
	{
		BW_GUARD;

		//Need to make sure the identifier isn't already taken.
		if (ikcs.getIKHandle( data.identifier_ ).hasObject())
		{
			WARNING_MSG( "Model visuals have an identifier conflict for "
				"the IK handle '%s'.\n", data.identifier_.c_str() );
			return false;
		}

		ConstraintSourcePtr source;
		switch( data.sourceType_ )
		{
		case Moo::ConstraintData::ST_CONSTRAINT:
			{
				ConstraintPtr sourceConstraint =
					ikcs.getConstraint( data.sourceIdentifier_ );
				if (!sourceConstraint.hasObject())
				{
					delayedAdd.push_back( &data );
					return true;
				}
				source = ConstraintSourcePtr(
					new ConstraintConstraintSource( sourceConstraint ),
					PyObjectPtr::STEAL_REFERENCE );
			}
			break;

		case Moo::ConstraintData::ST_MODEL_NODE:
			source = ConstraintSourcePtr(
				new ModelNodeConstraintSource( data.sourceIdentifier_ ),
				PyObjectPtr::STEAL_REFERENCE );
			break;

		default:
			MF_ASSERT( false && "Unhandled ConstraintData source type. " );
			break;
		}

		ConstraintTargetPtr target;
		switch( data.targetType_ )
		{
		case Moo::ConstraintData::TT_IK_HANDLE:
			{
				//Pole vector constraint type is just a special
				//point constraint that targets the pole vector of an
				//IK handle.
				const ConstraintTarget::Type ikTargetType =
					((data.type_ == Moo::ConstraintData::CT_POLE_VECTOR) ?
						ConstraintTarget::IK_HANDLE_POLE_VECTOR
						:
						ConstraintTarget::IK_HANDLE_TARGET);
				target = ConstraintTargetPtr( new ConstraintTarget(
					ikTargetType, data.targetIdentifier_ ),
					PyObjectPtr::STEAL_REFERENCE );
			}
			break;

		case Moo::ConstraintData::TT_MODEL_NODE:
			target = ConstraintTargetPtr(
				new ConstraintTarget(
					ConstraintTarget::MODEL_NODE, data.targetIdentifier_ ),
				PyObjectPtr::STEAL_REFERENCE );
			break;

		default:
			MF_ASSERT( false && "Unhandled ConstraintData target type. " );
			break;
		}

		ConstraintPtr constraint;
		switch( data.type_ )
		{
		case Moo::ConstraintData::CT_AIM:
			{
				AimConstraint * directPtr = NULL;
				constraint = ConstraintPtr( directPtr =
					new AimConstraint( data.identifier_, source, target ),
					PyObjectPtr::STEAL_REFERENCE );
				directPtr->setWeight( data.weight_ );
				directPtr->setOffset( data.rotationOffset_ );
				directPtr->setAimVector( data.aimVector_ );
				directPtr->setUpVector( data.upVector_ );
				directPtr->setWorldUpVector( data.worldUpVector_ );
			}
			break;

		case Moo::ConstraintData::CT_ORIENT:
			{
				OrientConstraint * directPtr = NULL;
				constraint = ConstraintPtr( directPtr =
					new OrientConstraint( data.identifier_, source, target ),
					PyObjectPtr::STEAL_REFERENCE );
				directPtr->setWeight( data.weight_ );
				directPtr->setOffset( data.rotationOffset_ );
			}
			break;

		case Moo::ConstraintData::CT_PARENT:
			{
				ParentConstraint * directPtr = NULL;
				constraint = ConstraintPtr( directPtr =
					new ParentConstraint( data.identifier_, source, target ),
					PyObjectPtr::STEAL_REFERENCE );
				directPtr->setWeight( data.weight_ );
				directPtr->setTranslationOffset( data.translationOffset_ );
				directPtr->setRotationOffset( data.rotationOffset_ );
			}
			break;
			
		case Moo::ConstraintData::CT_POLE_VECTOR:
			MF_ASSERT( target->getType() ==
				ConstraintTarget::IK_HANDLE_POLE_VECTOR );

			//Fall through, pole vector constraint is a point constraint.

		case Moo::ConstraintData::CT_POINT:
			{
				PointConstraint * directPtr = NULL;
				constraint = ConstraintPtr( directPtr =
					new PointConstraint( data.identifier_, source, target ),
					PyObjectPtr::STEAL_REFERENCE );
				directPtr->setWeight( data.weight_ );
				directPtr->setOffset( data.translationOffset_ );
			}
			break;

		default:
			MF_ASSERT( false && "Unhandled ConstraintData type. " );
			return false;
		}

		ikcs.setConstraint( constraint );
		
		return true;
	}


	/**
	 *	Used by getIKConstraintSystemFromData, adds an IKHandle from @a data
	 *	to @a ikcs.
	 *	@return True on success, false on failure (due to identifier
	 *		for @a data already existing in @a ikcs). On false return, printed
	 *		a warning.
	 */
	bool addIKHandleFromData(
		IKConstraintSystem & ikcs, const Moo::IKHandleData & data )
	{
		BW_GUARD;

		//Need to make sure the identifier isn't already taken.
		if (ikcs.getIKHandle( data.identifier_ ).hasObject())
		{
			WARNING_MSG( "Model visuals have an identifier conflict for "
				"the IK handle '%s'.\n", data.identifier_.c_str() );
			return false;
		}

		CCDChainIKHandle * directPtr = NULL;
		IKHandlePtr ikHandle( directPtr =
			new CCDChainIKHandle(
				data.identifier_, data.startNodeId_, data.endNodeId_ ),
			PyObjectPtr::STEAL_REFERENCE );
		directPtr->setTarget( data.targetPosition_ );
		directPtr->setPoleVector( data.poleVector_, false );

		ikcs.setIKHandle( ikHandle );
		
		return true;
	}
}


/**
 *  @pre True.
 *	@post Created an IKConstraintSystem containing any constraint and
 *		IK handle information from the top-level LOD of all models in the
 *		associated SuperModel.
 *		Returns NULL if no such data is present or errors are encountered.
 */
SmartPointer<IKConstraintSystem> PyModel::getIKConstraintSystemFromData() const
{
	BW_GUARD;

	if (pSuperModel_ == NULL)
	{
		return SmartPointer<IKConstraintSystem>();
	}

	SmartPointer<IKConstraintSystem> result;
	
	//Need to iterate through all models, getting any associated IK handle
	//and constraint data.
	const int nModels = pSuperModel_->nModels();
	for (int modelIndex = 0; modelIndex < nModels; ++modelIndex)
	{
		//We only use the top level LOD, lower LODs will use the
		//same ik/constraint data.
		const Moo::VisualPtr visual =
			pSuperModel_->topModel( modelIndex )->getVisual();
		const uint32 numConstraints = visual->nConstraints();
		const uint32 numIKHandles = visual->nIKHandles();
		if ((numConstraints == 0) && (numIKHandles == 0))
		{
			continue;
		}

		if (!result.hasObject())
		{
			result = SmartPointer<IKConstraintSystem>(
				new IKConstraintSystem(), PyObjectPtr::STEAL_REFERENCE );
		}
		
		for (uint32 i = 0; i < numIKHandles; ++i)
		{
			if (!addIKHandleFromData( *result, visual->ikHandle( i ) ))
			{
				return SmartPointer<IKConstraintSystem>();
			}
		}

		//The addition of constraints is complicated by the fact that
		//constraints can depend on other constraints.
		//We continue adding until there are none left to add or
		//we can't add any more.
		typedef BW::vector<const Moo::ConstraintData *> DelayAddVector;
		DelayAddVector delayedAdd;
		for (uint32 i = 0; i < numConstraints; ++i)
		{
			if (!addConstraintFromData(
				*result, visual->constraint( i ), delayedAdd ))
			{
				return SmartPointer<IKConstraintSystem>();
			}
		}
		DelayAddVector::size_type numDelayedLast = delayedAdd.size() + 1;
		while (!delayedAdd.empty() && (numDelayedLast != delayedAdd.size()))
		{
			numDelayedLast = delayedAdd.size();

			DelayAddVector delayedAddAgain;

			for( DelayAddVector::const_iterator itr = delayedAdd.begin();
				itr != delayedAdd.end();
				++itr)
			{
				if (!addConstraintFromData( *result, **itr, delayedAddAgain ))
				{
					return SmartPointer<IKConstraintSystem>();
				}
			}

			delayedAdd.swap( delayedAddAgain );
		}
		if (!delayedAdd.empty())
		{
			WARNING_MSG( "There are constraint dependencies where "
				"constraint(s) are sourcing from non-existent constraints "
				"or there's a cycle of dependencies. The first such constraint "
				"has identifier '%s' with source identifier '%s'.\n",
				delayedAdd.front()->identifier_.c_str(),
				delayedAdd.front()->sourceIdentifier_.c_str() );
			return SmartPointer<IKConstraintSystem>();
		}
	}

	return result;
}


PyModel::ScopedPyModelWatchHolder::ScopedPyModelWatchHolder(
	DogWatch& pyModelWatch ) :
	pyModelWatch_( pyModelWatch )
{
	if (running_ == 0)
	{
		pyModelWatch_.start();
	}
	++running_;
}


PyModel::ScopedPyModelWatchHolder::~ScopedPyModelWatchHolder()
{
	--running_;
	if (running_ == 0)
	{
		pyModelWatch_.stop();
	}
}


PyModel::ScopedCurrentModelHolder::ScopedCurrentModelHolder(
	PyModel& pyModel ) :
	pyModel_( pyModel )
{
	if (PyModel::pCurrent() == NULL)
	{
		PyModel::pCurrent( &pyModel_ );
	}
}


PyModel::ScopedCurrentModelHolder::~ScopedCurrentModelHolder()
{
	if (PyModel::pCurrent() == &pyModel_)
	{
		PyModel::pCurrent( NULL );
	}
}


/**
 *	The constructor of PyModel is private because it's usually only created
 *	by a loader friend. This function allows unit test code to create PyModel
 *	objects if required.
 *
 *	@param superModel The SuperModel the result owns. It's deliberately passed
 *		by value as auto_ptr because this function is taking ownership.
 *	
 *	@return A PyModel that has taken ownership of @a superModel (if it's
 *		not NULL).
 */
PyModelPtr PyModel::createTestPyModel( std::auto_ptr<SuperModel> superModel )
{
	return PyModelPtr(
		new PyModel( superModel.release() ), PyModelPtr::STEAL_REFERENCE );
}


/*~ function BigWorld.debugAQ
 *
 *	This function controls the Action Queue debugging graph.  It takes one
 *	argument which is either a PyModel, or None.  None switches the debugging
 *	graph off.  If the argument is a model, then a graph is displayed on
 *	the screen.  It is a line graph, which shows what actions are currently
 *	playing on that model, and what blend weight each action has.
 *
 *	Each line represents one currently playing action.  It will have an
 *	arbitrary colour assigned to it, and have the Actions name printed below
 *	it in the same colour.  The height of the line represents what percentage
 *	of the total blended Actions this Action makes up.
 *
 *	If one model is currently being graphed, and this function is called again
 *	with another model, the graph for the first model stops, and the new model
 *	takes over.
 */
/**
 *	Python debug setting method
 */
static void debugAQ( SmartPointer<PyModel> pm )
{
	BW_GUARD;	
#if ENABLE_ACTION_QUEUE_DEBUGGER
	if (pm)
	{
		pm->actionQueue().debugTick( -1.f );
		s_dpm = pm.getObject();
	}
	else
	{
		s_dpm = NULL;
	}
#else
	ERROR_MSG("Action Queue Debugger is disabled");
#endif
}
PY_AUTO_MODULE_FUNCTION(
	RETVOID, debugAQ, ARG( SmartPointer<PyModel>, END ), BigWorld )

BW_END_NAMESPACE

// pymodel.cpp
