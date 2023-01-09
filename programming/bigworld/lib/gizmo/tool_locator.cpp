#include "pch.hpp"
#include "tool_locator.hpp"
#include "tool.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "cstdmf/debug.hpp"

/*~ module Locator
 *	@components{ tools }
 *
 *	The Locator Module is a Python module that provides an interface to
 *	the various tool locators for positioning tools in the world.
 */

DECLARE_DEBUG_COMPONENT2( "Locator", 0 )

BW_BEGIN_NAMESPACE

//------------------------------------------------------------
//Section : ToolLocator
//------------------------------------------------------------

/// static factory map initialiser
template<> LocatorFactory::ObjectMap * LocatorFactory::pMap_;


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ToolLocator::

PY_TYPEOBJECT( ToolLocator )

PY_BEGIN_METHODS( ToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ToolLocator )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
ToolLocator::ToolLocator( PyTypeObject * pType ):
	PyObjectPlus( pType ),
	transform_( Matrix::identity )
{
}


/**
 *	Destructor.
 */
ToolLocator::~ToolLocator()
{
}


PY_SCRIPT_CONVERTERS( ToolLocator )			


//------------------------------------------------------------
//Section : OriginLocator
//------------------------------------------------------------


//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE OriginLocator::

PY_TYPEOBJECT( OriginLocator )

PY_BEGIN_METHODS( OriginLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( OriginLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( OriginLocator, "OriginLocator", Locator )

LOCATOR_FACTORY( OriginLocator )
/**
 *	Constructor.
 */
OriginLocator::OriginLocator( PyTypeObject * pType ):
	ToolLocator( pType )
{
}


/**
 *	This method calculates the tool's position.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool for this locator.
 */
void OriginLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	transform_.setIdentity();
}


/**
 *	Static python factory method
 */
PyObject * OriginLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	return new OriginLocator;
}


//------------------------------------------------------------
//Section : PlaneToolLocator
//------------------------------------------------------------


//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE PlaneToolLocator::

PY_TYPEOBJECT( PlaneToolLocator )

PY_BEGIN_METHODS( PlaneToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PlaneToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PlaneToolLocator, "PlaneToolLocator", Locator )

LOCATOR_FACTORY( PlaneToolLocator )
/**
 *	Constructor.
 */
PlaneToolLocator::PlaneToolLocator( PlaneEq * pPlane, PyTypeObject * pType ):
	ToolLocator( pType ),
	planeEq_( Vector3(0,1,0), 0 )
{
	BW_GUARD;

	if (pPlane != NULL) planeEq_ = *pPlane;
}


/**
 *	This method calculates the tool's position, by intersecting it with
 *	the XZ plane.
 *
 *	@TODO	multiple planes, dependent on the camera angle, and an offset
 *			from the plane.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool for this locator.
 */
void PlaneToolLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	transform_.translation( planeEq_.intersectRay(
		Moo::rc().invView().applyToOrigin(), worldRay ) );
}


/**
 *	Static python factory method
 */
PyObject * PlaneToolLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	// see if it's the default plane
	if (PyTuple_Size( args ) == 0)
	{
		return new PlaneToolLocator();
	}
	
	// ok, parse the arguments then
	PyObject * pFirst, * pOther;
	Vector3	vFirst, vOther;
	float fOther;
	int voRet;
	if (!PyArg_ParseTuple( args, "OO", &pFirst, &pOther ) ||
		!Script::setData( pFirst, vFirst ) ||
		(!(voRet = Script::setData( pOther, vOther )) &&
			!Script::setData( pOther, fOther )))
	{
		PyErr_SetString( PyExc_TypeError, "PlaneToolLocator() expects "
			"no args, a point and a normal, or a normal and distance" );
		return NULL;
	}


	PlaneEq	peq;

	// if the vector3 conversion worked, go for that
	if (voRet == 0)
	{
		peq = PlaneEq( vFirst, vOther );
	}
	else
	{
		PyErr_Clear();	// clear the error from the first attempt
		peq = PlaneEq( vFirst, fOther );
	}

	return new PlaneToolLocator( &peq );
}


//------------------------------------------------------------
//Section : TriPlaneToolLocator
//------------------------------------------------------------


//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE TriPlaneToolLocator::

PY_TYPEOBJECT( TriPlaneToolLocator )

PY_BEGIN_METHODS( TriPlaneToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TriPlaneToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TriPlaneToolLocator, "TriPlaneToolLocator", Locator )

Vector3 TriPlaneToolLocator::s_lastPos( 0.f, 0.f, 0.f );
/**
 *	Constructor.
 */
TriPlaneToolLocator::TriPlaneToolLocator( const Vector3& pos, PyTypeObject * pType ):
	ToolLocator( pType ),
	fulcrum_( pos )
{
	BW_GUARD;

	planeEq_[0] = PlaneEq( Vector3(1,0,0), pos.x );
	planeEq_[1] = PlaneEq( Vector3(0,1,0), pos.y );
	planeEq_[2] = PlaneEq( Vector3(0,0,1), pos.z );
}


/**
 *	This method calculates the tool's position, by finding out which plane
 *	is most perpendicular to the camera to plane fulcra, and intersecting
 *	with that plane.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool for this locator.
 */
void TriPlaneToolLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	//static int lastPlane = -1;

	//Calculate which plane.
	PlaneEq* use = NULL;

	Vector3 toCamera = Moo::rc().invView().applyToOrigin();
	toCamera -= fulcrum_;
	float dist = toCamera.length();
	
	if ( dist > 0.f )
	{
		//normalise to get the vector from the fulcrum to the camera
		toCamera /= dist;
		//take the dotproduct with each of the planes normals.
		//what we want to avoid here is taking the plane that
		//the camera is looking along, we want the plane the
		//camera is looking AT.
		float maxAngle = 0.f;
		for ( int i=0; i<3; i++ )
		{
			float dotp = toCamera.dotProduct( planeEq_[i].normal() );
			if ( fabsf(dotp) > maxAngle )
			{
				use = &planeEq_[i];
				maxAngle = fabsf(dotp);
			}
		}
	}

	/*if ( lastPlane != ( use - planeEq_ ) )
	{
		lastPlane = ( use - planeEq_ );
		DEBUG_MSG( "Plane is now %s\n", lastPlane == 0 ? "x" : lastPlane == 1 ? "y" : "z" );
	}*/

	if ( use )
	{
		transform_.setTranslate( use->intersectRay(
			Moo::rc().invView().applyToOrigin(), worldRay ) );
	}
}


/**
 *	Static python factory method
 */
PyObject * TriPlaneToolLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	// make sure we've got a position
	float x,y,z;
	y = -30001.f;
	
	if (!PyArg_ParseTuple( args, "|fff", &x, &y, &z ))
	{
		PyErr_SetString( PyExc_TypeError, "TriPlaneToolLocator() expects "
			"an optional 3-tuple ( point in world space )" );
		return NULL;
	}

	if ( y < -30000.f )
		return new TriPlaneToolLocator;
	else
		return new TriPlaneToolLocator( Vector3(x,y,z) );
}


//------------------------------------------------------------
//Section : AdaptivePlaneToolLocator
//------------------------------------------------------------


//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE AdaptivePlaneToolLocator::

PY_TYPEOBJECT( AdaptivePlaneToolLocator )

PY_BEGIN_METHODS( AdaptivePlaneToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( AdaptivePlaneToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( AdaptivePlaneToolLocator, "AdaptivePlaneToolLocator", Locator )
/**
 *	Constructor.
 */
AdaptivePlaneToolLocator::AdaptivePlaneToolLocator( const Vector3& pos, PyTypeObject * pType ):
	ToolLocator( pType ),
	fulcrum_( pos )
{
}


/**
 *	This method calculates the tool's position, by finding out which plane
 *	is most perpendicular to the camera to plane fulcra, and intersecting
 *	with that plane.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool for this locator.
 */
void AdaptivePlaneToolLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	//Calculate which plane.
	Vector3 toCamera = Moo::rc().invView().applyToOrigin();
	toCamera -= fulcrum_;
	float dist = toCamera.length();
	
	if ( dist > 0.f )
	{
		//normalise to get the vector from the fulcrum to the camera
		toCamera /= dist;
	
		PlaneEq peq( fulcrum_, toCamera );
		
		transform_.setTranslate( peq.intersectRay(
			Moo::rc().invView().applyToOrigin(), worldRay ) );
	}
}


/**
 *	Static python factory method
 */
PyObject * AdaptivePlaneToolLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	// make sure we've got a position
	float x,y,z;
	
	if (!PyArg_ParseTuple( args, "fff", &x, &y, &z ))
	{
		PyErr_SetString( PyExc_TypeError, "AdaptivePlaneToolLocator() expects "
			"a 3-tuple ( point in world space )" );
		return NULL;
	}

	return new AdaptivePlaneToolLocator( Vector3(x,y,z) );
}

//------------------------------------------------------------
//Section : LineToolLocator
//------------------------------------------------------------


//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE LineToolLocator::

PY_TYPEOBJECT( LineToolLocator )

PY_BEGIN_METHODS( LineToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( LineToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( LineToolLocator, "LineToolLocator", Locator )

LOCATOR_FACTORY( LineToolLocator )
/**
 *	Constructor.
 */
LineToolLocator::LineToolLocator( const Vector3& origin, const Vector3& direction, PyTypeObject * pType ):
	ToolLocator( pType ),
//	planeEq_( Vector3(0,0,0), 0 )
	origin_( origin ),
	direction_( direction )
{
	BW_GUARD;

	direction_.normalise();
//	if (pPlane != NULL) planeEq_ = *pPlane;
}


/**
 *	This method calculates the tool's position, by intersecting it with
 *	the XZ plane.
 *
 *	@TODO	multiple planes, dependent on the camera angle, and an offset
 *			from the plane.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool for this locator.
 */
void LineToolLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	// build a plane that contains our line, and is pointing towards the camera.
	Vector3 cameraPos = Moo::rc().invView().applyToOrigin();
	Vector3 normal = direction_.crossProduct( worldRay );
	normal = normal.crossProduct( direction_ );
	if (normal.length() != 0)
	{
		normal.normalise();
		PlaneEq plane( normal, normal.dotProduct( origin_ ) );
		Vector3 posOnPlane = plane.intersectRay( cameraPos, worldRay );
		transform_.translation( origin_ + 
			direction_.dotProduct( posOnPlane - origin_ ) * direction_ );
	}
	else
		transform_.translation( origin_ );

/*	transform_.setTranslate( planeEq_.intersectRay(
		Moo::rc().invView().applyToOrigin(), worldRay ) );*/
}


/**
 *	Static python factory method
 */
PyObject * LineToolLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	// see if it's the default plane
/*	if (PyTuple_Size( args ) == 0)
	{
		return new LineToolLocator();
	}*/
	
	// ok, parse the arguments then
	PyObject * pFirst, * pOther;
	Vector3	vFirst, vOther;
	if (!PyArg_ParseTuple( args, "OO", &pFirst, &pOther ) ||
		!Script::setData( pFirst, vFirst ) ||
		!Script::setData( pOther, vOther ))
	{
		PyErr_SetString( PyExc_TypeError, "LineToolLocator() expects "
			"a point and a direction" );
		return NULL;
	}

/*	PlaneEq	peq;

	// if the vector3 conversion worked, go for that
	if (voRet == 0)
	{
		peq = PlaneEq( vFirst, vOther );
	}
	else
	{
		PyErr_Clear();	// clear the error from the first attempt
		peq = PlaneEq( vFirst, fOther );
	}*/

	return new LineToolLocator( vFirst, vOther );
}


//------------------------------------------------------------
//Section : ChunkToolLocator
//------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ChunkToolLocator::

PY_TYPEOBJECT( ChunkToolLocator )

PY_BEGIN_METHODS( ChunkToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkToolLocator )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
ChunkToolLocator::ChunkToolLocator( PyTypeObject * pType ):
	ToolLocator( pType )
{
}


//------------------------------------------------------------
//Section : PlaneChunkLocator
//------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PlaneChunkLocator::

PY_TYPEOBJECT( PlaneChunkLocator )

PY_BEGIN_METHODS( PlaneChunkLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PlaneChunkLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PlaneChunkLocator, "PlaneChunkLocator", Locator )

LOCATOR_FACTORY( PlaneChunkLocator )
/**
 *	Constructor.
 */
PlaneChunkLocator::PlaneChunkLocator( PyTypeObject * pType ):
	ChunkToolLocator( pType )
{
}



/**
 *	This method calculates the position of a tool, defined as the chunk
 *	found by intersecting the camera's world ray with the XZ plane at y=0.
 *
 *	@param worldRay		The camera's world ray.
 *	@param tool			The tool this locator applies to.
 */
void PlaneChunkLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	PlaneEq pe( Vector3( 0.f, 1.f, 0.f ), 1.f );
	Vector3 pos = pe.intersectRay( Moo::rc().invView().applyToOrigin(), worldRay );

	if ( ChunkManager::instance().cameraSpace() )
	{
		ChunkSpace::Column* pColumn =
			ChunkManager::instance().cameraSpace()->column( pos, false );

		if ( pColumn )
		{
			if ( pColumn->pOutsideChunk() )
			{
				transform_ = pColumn->pOutsideChunk()->transform();
				transform_[3] += Vector3( 50.f, 0.f, 50.f );
				tool.relevantChunks().push_back( pColumn->pOutsideChunk() );
			}
		}
	}
}


/**
 *	Static python factory method
 */
PyObject * PlaneChunkLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	return new PlaneChunkLocator;
}

BW_END_NAMESPACE

