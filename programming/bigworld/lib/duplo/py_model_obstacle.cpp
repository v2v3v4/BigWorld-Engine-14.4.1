#include "pch.hpp"

#include "py_model_obstacle.hpp"
#include "pydye.hpp"

#include "model/super_model.hpp"
#include "model/super_model_dye.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyModelObstacle
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyModelObstacle )

PY_BEGIN_METHODS( PyModelObstacle )
	PY_METHOD( node )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyModelObstacle )

	/*~	attribute PyModelObstacle.sources
	 *	@type Read-only tuple of strings
	 *
	 *	This is the set of .model resources specified upon creation
	 *	@see BigWorld.PyModelObstacle
	 */
	PY_ATTRIBUTE( sources )

	/*~	attribute PyModelObstacle.matrix
	 *	@type MatrixProviderPtr
	 *
	 *	This is the transform matrix for the PyModelObstacle.  As the 
	 *	MatrixProvider is updated, the PyModelObstacle will follow it.
	 */
	PY_ATTRIBUTE( matrix )

	/*~	attribute PyModelObstacle.dynamic
	 *	@type bool
	 *
	 *	If true, PyModelObstacle will be treated as dynamic, otherwise static.
	 *	Note:  This value should be set upon creation.  Once it is attached/assigned, 
	 *	it cannot be changed.
	 */
	PY_ATTRIBUTE( dynamic )

	/*~	attribute PyModelObstacle.attached
	 *	@type Read-only bool
	 *
	 *	This flag is set to true when the PyModelObstacle is assigned to an Entity.  Once this occurs, it is placed 
	 *	in the collision scene and no further changes can be made to its dynamic nature.
	 */
	PY_ATTRIBUTE( attached )

	/*~	attribute PyModelObstacle.vehicleID
	 *	@type uint32
	 *
	 *	To place PyModelObstacle on a vehicle, set the vehicleID to the EntityID of the vehicle.
	 *	To alight from the vehicle, set it back to 0. This parameter is used to identify platforms 
	 *  when applying gravity e.g. MovingPlatform.py.
	 */
	PY_ATTRIBUTE( vehicleID )

	/*~	attribute PyModelObstacle.bounds
	 *	@type Read-only MatrixProvider
	 *
	 *	Stores the bounding box of the PyModelObstacle as a MatrixProvider.
	 */
	PY_ATTRIBUTE( bounds )

PY_END_ATTRIBUTES()

/*~	function BigWorld.PyModelObstacle
 *
 *	Creates and returns a new PyModelObstacle object, which is used to integrate a model into the collision scene.
 *
 *	Example:
 *	@{
 *		self.model = BigWorld.PyModelObstacle("models/body.model", "models/head.model", self.matrix, 1)
 *	@}
 *
 *	Once created and assigned (self.model = PyModelObstacle), the PyObstacleModel becomes attached, hence its 
 *	dynamic nature cannot be changed.  If left as static, the position of the PyObstacleModel will not be updated 
 *	as the MatrixProvider changes.  PyModelObstacles are reasonably expensive, hence should only be used when truly 
 *	required.
 *
 *	@param modelNames Any number of ModelNames can be used to build a SuperModel, 
 *					which is a PyModel made of other PyModels ( see BigWorld.Model() ).
 *					Each modelName should be a complete .model filename, including 
 *					resource path.
 *	@param matrix		(optional) The transform Matrix for the resultant PyModelObstacle, 
 *					used to update (if dynamic) or set (if static) the SuperModel's 
 *					position.
 *	@param dynamic	(optional) If true, PyModelObstacle will be treated as dynamic, 
 *					otherwise static.  Defaults to false (static).
 */
PY_FACTORY( PyModelObstacle, BigWorld )

PY_SCRIPT_CONVERTERS( PyModelObstacle )


/**
 *	Constructor
 */
PyModelObstacle::PyModelObstacle( SuperModel * pSuperModel,
		MatrixProviderPtr pMatrix, bool dynamic, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pSuperModel_( pSuperModel ),
	pMatrix_( pMatrix ),
	dynamic_( dynamic ),
	attached_( false ),
	inWorld_( false )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pSuperModel_ != NULL )
	{
		return;
	}

	pSuperModel_->localBoundingBox( localBoundingBox_ );
}

/**
 *	Destructor
 */
PyModelObstacle::~PyModelObstacle()
{
	BW_GUARD;
	bw_safe_delete(pSuperModel_);
}


void PyModelObstacle::localBoundingBox( BoundingBox& bb, bool skinny ) const
{
	//Note we ignore the skinny parameter for now, since PyModelObstacles can't
	//have attachments.  But as soon as they can have attachments, skinny needs
	//to be taken into account.
	bb = localBoundingBox_;	
}


void PyModelObstacle::worldBoundingBox( BoundingBox& bb, const Matrix& world, bool skinny ) const
{
	//Note we ignore the skinny parameter for now, since PyModelObstacles can't
	//have attachments.  But as soon as they can have attachments, skinny needs
	//to be taken into account.
	bb = localBoundingBox_;
	bb.transformBy( worldTransform() );
}


void PyModelObstacle::attach()
{
	BW_GUARD;
	MF_ASSERT_DEV( !attached_ );
	attached_ = true;
}

void PyModelObstacle::detach()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( attached_ )
	{
		return;
	}

	attached_ = false;
}

void PyModelObstacle::enterWorld()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( attached_ )
	{
		return;
	}

	inWorld_ = true;
}

void PyModelObstacle::leaveWorld()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( attached_ )
	{
		return;
	}

	inWorld_ = false;
}

/**
 *	Tick our dyes
 */
void PyModelObstacle::tick( float dTime )
{
	BW_GUARD;
	for (Dyes::iterator it = dyes_.begin(); it != dyes_.end(); it++)
	{
		it->second->tick( dTime, 0 );
	}
}


/**
 *	Re-calculate LOD and apply animations to this model for this frame.
 *	Should be called once per frame.
 */
void PyModelObstacle::updateAnimations( const Matrix & worldTransform )
{
	BW_GUARD;
	MF_ASSERT( pSuperModel_ != NULL );

	pSuperModel_->updateAnimations( worldTransform,
		NULL, // pPreTransformFashions
		NULL, // pPostTransformFashions
		Model::LOD_AUTO_CALCULATE ); // atLod
}


/**
 *	Draw ourselves using our dyes if we have any
 */
void PyModelObstacle::draw( Moo::DrawContext& drawContext,
	const Matrix & worldTransform )
{
	BW_GUARD;
	MF_ASSERT( pSuperModel_ != NULL );

	Moo::rc().push();
	Moo::rc().world( worldTransform );

	MaterialFashionVector materialFashions;
	materialFashions.reserve( dyes_.size() );
	for (Dyes::iterator it = dyes_.begin(); it != dyes_.end(); ++it)
	{
		materialFashions.push_back( it->second->materialFashion() );
	}

	pSuperModel_->draw( drawContext, worldTransform, &materialFashions );

	Moo::rc().pop();
}


/**
 *	Get this model's world transform
 */
const Matrix & PyModelObstacle::worldTransform() const
{
	BW_GUARD;
	static Matrix transform;
	if (pMatrix_)
		pMatrix_->matrix( transform );
	else
		transform = Matrix::identity;
	return transform;
}



/**
 *	Python get attribute method
 */
ScriptObject PyModelObstacle::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	// try to find the dye matter or other fashion named 'attr'
	Dyes::iterator found2 = dyes_.find( attr );
	if (found2 != dyes_.end())
	{
		return ScriptObjectPtr<PyDye>( found2->second );
	}
	SuperModelDyePtr pDye = pSuperModel_->getDye( attr, "" );
	if (pDye)	// some question as to whether we should do this...
	{	// but I want to present illusion that matters are always there
		SmartPointer<PyDye> pPyDye = new PyDye( pDye, attr, "Default" );
		dyes_.insert( std::make_pair( attr, pPyDye ) );
		return ScriptObject( pPyDye ); // ref count comes from constructor
	}

	return this->PyObjectPlus::pyGetAttribute( attrObj );
}

/** 
 *	Python set attribute method
 */
bool PyModelObstacle::pySetAttribute( const ScriptString & attrObj,
									const ScriptObject & value )
{
	BW_GUARD;
	const char * attr = attrObj.c_str();

	// check if it's a PyFashion
	if (PyDye::Check( value ))
	{
		// some fashions must be copied when set into models
		PyDye * pPyFashion = static_cast<PyDye*>( value.get() );

		// we always have to make a copy of the dye object here, because
		// it may have been created from a different model/supermodel.
		pPyFashion = pPyFashion->makeCopy( *pSuperModel_, attr );
		if (pPyFashion == NULL)
		{
			return false; // makeCopy sets exception
		}

		Dyes::iterator found = dyes_.find( attr );
		if (found != dyes_.end())
		{
			found->second->disowned();
		}
		dyes_[attr] = pPyFashion;
		Py_DECREF( pPyFashion );
		return true;
	}

	// try to find the dye matter named 'attr'
	if (ScriptString::check( value ) || value.isNone())
	{
		const char * valStr = (value.isNone() ? "" :
			ScriptString( value ).c_str() );

		SuperModelDyePtr pDye = pSuperModel_->getDye( attr, valStr );
		if (pDye)	// only NULL if no such matter
		{
			SmartPointer<PyDye> newPyDye(
				new PyDye( pDye, attr, valStr ), true );

			Dyes::iterator found = dyes_.find( attr );
			if (found != dyes_.end())
			{
				found->second->disowned();
			}
			dyes_[attr] = newPyDye;
			return true;
		}
	}

	// check if it's none and in our list of fashions (and not a dye)
	if (value.isNone())
	{
		Dyes::iterator found = dyes_.find( attr );
		if (found != dyes_.end())
		{
			found->second->disowned();
			dyes_.erase( found );
			return true;
		}
	}

	return this->PyObjectPlus::pySetAttribute( attrObj, value );
}


/**
 *	Get sources of this model
 */
PyObject * PyModelObstacle::pyGet_sources()
{
	BW_GUARD;
	int nModels = pSuperModel_->nModels();
	PyObject * pTuple = PyTuple_New( nModels );
	for (int i = 0; i < nModels; i++)
	{
		PyTuple_SetItem( pTuple, i, Script::getData(
			pSuperModel_->topModel(i)->resourceID() ) );
	}
	return pTuple;
}

/**
 *	Helper class for the bounding box of a PyModelObstacle
 */
class ModelObstacleBBMProv : public MatrixProvider
{
public:
	ModelObstacleBBMProv( PyModelObstacle * pModel ) :
		MatrixProvider( false, &s_type_ ),
		pModel_( pModel ) { }

	~ModelObstacleBBMProv() { }

	virtual void matrix( Matrix & m ) const
	{
		BW_GUARD;	
/*		if (!pModel_->isInWorld())
		{
			m = Matrix::identity;
			m[3][3] = 0.f;
			return;
		}*/

		BoundingBox bb;
		pModel_->localBoundingBox(bb,true);
		m = pModel_->worldTransform();

		Vector3 bbMin = m.applyPoint( bb.minBounds() );
		m.translation( bbMin );

		Vector3 bbMax = bb.maxBounds() - bb.minBounds();
		m[0] *= bbMax.x;
		m[1] *= bbMax.y;
		m[2] *= bbMax.z;
	}

private:
	SmartPointer<PyModelObstacle>	pModel_;
};

/**
 *	Get the bounding box in a matrix provider
 */
PyObject * PyModelObstacle::pyGet_bounds()
{
	BW_GUARD;
	return new ModelObstacleBBMProv( this );
}


/**
 *	Set whether or not this model is dynamic
 */
int PyModelObstacle::pySet_dynamic( PyObject * val )
{
	BW_GUARD;
	bool dynamic;
	int ret = Script::setData( val, dynamic, "PyModelObstacle.dynamic" );
	if (ret != 0) return ret;

	if (attached_)
	{
		PyErr_SetString( PyExc_TypeError, "PyModelObstacle.dynamic "
			"cannot be set when obstacle is attached." );
		return -1;
	}

	dynamic_ = dynamic;
	return 0;
}


/**
 *	Helper class for a node in a ModelObstacle.
 *	Currently can only get its matrix not attach stuff to it.
 */
class ModelObstacleNode : public MatrixProvider
{
public:
	ModelObstacleNode( PyModelObstacle * pModel, const Matrix & local ) :
		MatrixProvider( false, &s_type_ ),
		pModel_( pModel ),
		local_( local )
	{
	}

	~ModelObstacleNode() { }

	virtual void matrix( Matrix & m ) const
	{
		m.multiply( local_, pModel_->worldTransform() );
	}

private:
	// Note: This packs better if Matrix is second. sizeof=80 vs sizeof=96
	SmartPointer<PyModelObstacle>	pModel_;
	Matrix							local_;
};

/**
 *	Get the matrix of our node
 */
PyObject * PyModelObstacle::node( const BW::string & nodeName )
{
	BW_GUARD;
	Moo::NodePtr pNode = pSuperModel_->findNode( nodeName );
	if (!pNode)
	{
		PyErr_Format( PyExc_ValueError, "PyModelObstacle.node(): "
			"No node named %s in this Model.", nodeName.c_str() );
		return NULL;
	}

	pSuperModel_->dressInDefault();

	return new ModelObstacleNode( this, pNode->worldTransform() );
}


/**
 *	Static factory method
 */
PyObject * PyModelObstacle::pyNew( PyObject * args )
{
	BW_GUARD;
	bool bad = false;
	Py_ssize_t nargs = PyTuple_Size( args );
	int i;

	BW::vector<BW::string> modelNames;
	for (i = 0; i < nargs; i++)
	{
		PyObject * ana = PyTuple_GetItem( args, i );
		if (!PyString_Check( ana )) break;
		modelNames.push_back( PyString_AsString( ana ) );
	}
	bad |= modelNames.empty();

	MatrixProviderPtr pMatrix = NULL;
	if (i < nargs)
	{
		int ret = Script::setData( PyTuple_GetItem( args, i ), pMatrix );
		if (ret != 0) bad = true;
		i++;
	}

	bool dynamic = false;
	if (i < nargs)
	{
		int ret = Script::setData( PyTuple_GetItem( args, i ), dynamic );
		if (ret != 0) bad = true;
		i++;
	}

	if (bad || i < nargs)
	{
		PyErr_SetString( PyExc_TypeError, "PyModelObstacle() "
			"expects some model names optionally followed by "
			"a MatrixProvider and a boolean dynamic flag" );
		return NULL;
	}

	SuperModel * pSuperModel = new SuperModel( modelNames );
	if (pSuperModel->nModels() != modelNames.size())
	{
		PyErr_Format( PyExc_ValueError, "PyModelObstacle() "
			"only found %d out of %d models requested",
			pSuperModel->nModels(), modelNames.size() );

		bw_safe_delete(pSuperModel);
		return NULL;
	}

	return new PyModelObstacle( pSuperModel, pMatrix, dynamic );
}

BW_END_NAMESPACE
