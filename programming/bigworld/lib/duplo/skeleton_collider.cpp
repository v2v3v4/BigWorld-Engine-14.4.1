#include "pch.hpp"
#include "skeleton_collider.hpp"


DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: SkeletonCollider
// -----------------------------------------------------------------------------

/*~ class BigWorld.SkeletonCollider
 *	This class is a utility class designed to help
 *	implement collisions on dynamic skeletal objects.
 *
 *	It has a list of BoxAttachment objects, and provides
 *	a C++ and python interface to coordinate collision
 *	testing for that list as a whole.
 */
PY_TYPEOBJECT( SkeletonCollider )

PY_BEGIN_METHODS( SkeletonCollider )
	/*~	function SkeletonCollider.addCollider
	 *
	 *	This method adds a box attachment to the skeleton collider
	 *	@param collider the box attachment to add
	 */
	PY_METHOD( addCollider )
	/*~	function SkeletonCollider.getCollider
	 *
	 *	This method gets a BoxAttachment from the skeleton collider
	 *	@param n the index of the BoxAttachment
	 *	@return the BoxAttachment
	 */
	PY_METHOD( getCollider )
	/*~	function SkeletonCollider.nColliders
	 *
	 *	returns the number of BoxAttachments in this skeleton collider
	 *	@return the number of BoxAttachments in the skeleton collider
	 */
	PY_METHOD( nColliders )
	/*~	function SkeletonCollider.doCollide
	 *
	 *	Attempts to collide with this collider. Tests the collection of colliders 
	 *	against the ray that begins at the position defined by the "start" argument, 
	 *	and goes all the way down to the far plane in the direction defined by the 
	 *	"dir" argument.
	 *
	 *	@param start the start point of the collision ray
	 *	@param direction the direction of the collision ray. 
	 *
	 *	@return 	 	True if a hit was detected, false otherwise. Check the 
	 *					impactCollider, impactPoint and impactReflection 
	 *					attibutes for details on the intersection detected.	
	 */
	PY_METHOD( doCollide )
	/*~	function SkeletonCollider.save
	 *
	 *	save this collider
	 *	@param pSection the datasection to save to
	 */
	PY_METHOD( save )
	/*~	function SkeletonCollider.load
	 *
	 *	load this collider
	 *	@param pSection the datasection to load from
	 */
	PY_METHOD( load )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( SkeletonCollider )
	/*~ attribute SkeletonCollider.impactPoint
	 *  The point in world space of the impact between the last collision ray 
	 *	and the skeleton collider
	 */
	PY_ATTRIBUTE( impactPoint )
	/*~ attribute SkeletonCollider.impactReflection
	 *	The collision ray reflected off the last hit object
	 */
	PY_ATTRIBUTE( impactReflection )
	/*~ attribute SkeletonCollider.impactCollider
	 *	The last collider object that was hit
	 */
	PY_ATTRIBUTE( impactCollider )
PY_END_ATTRIBUTES()

PY_FACTORY( SkeletonCollider, BigWorld )

/**
 *	Constructor.
 */
SkeletonCollider::SkeletonCollider( PyTypeObject * pType ) : 
	PyObjectPlus( pType )
{
}


/**
 *	Destructor.
 */
SkeletonCollider::~SkeletonCollider()
{
}

/**
 *	This method performs a collision test with all the box attachments in the skelton collider
 *	@param start the start of the collision ray
 *	@param direction the direction of the collision ray
 *	@param distance in/out the length of the collision ray 
 */
bool SkeletonCollider::collide( const Vector3& start, const Vector3& direction, float& distance )
{
	BW_GUARD;
	impactCollider_ = NULL;
	bool ret = false;
	Colliders::iterator it = colliders_.begin();
	Colliders::iterator end = colliders_.end();
	Vector3 normalisedDir = direction / direction.length();
	while (it != end)
	{
		if ((*it)->collide( start, normalisedDir, distance ))
		{
			ret = true;
			impactCollider_ = *it;
		}
        it++;
	}
	if (ret)
	{
		impactReflection_ = Vector3(0,0,0) - normalisedDir;
		impactPoint_ = start + normalisedDir * distance;
	}
	return ret;
}

void SkeletonCollider::save( DataSectionPtr pSection )
{
	BW_GUARD;
	DataSectionPtr newSection = pSection->newSection( "SkeletonCollider" );
	if (newSection.hasObject())
	{
		Colliders::iterator it = colliders_.begin();
		Colliders::iterator end = colliders_.end();
		while (it != end)
		{
			(*it)->save( newSection );
			it++;
		}
	}
	else
	{
		ERROR_MSG( "SkeletonCollider::save - unable to create SkeletonCollider section" );
	}
}

void SkeletonCollider::load( DataSectionPtr pSection )
{
	BW_GUARD;
	DataSectionPtr pSect = pSection->openSection( "SkeletonCollider" );
	if (pSect.hasObject())
	{
		DataSectionIterator it = pSect->begin();
		DataSectionIterator end = pSect->end();
		while (it != end)
		{
			DataSectionPtr pDS = *it;

			if (pDS->sectionName() == "BoxAttachment")
			{
				SmartPointer<BoxAttachment> collider(new BoxAttachment(), true);
				collider->load( pDS );
				colliders_.push_back( collider );
			}
			++it;
		}
	}
	else
	{
		ERROR_MSG( "SkeletonCollider::load - unable to open SkeletonCollider section" );
	}
}


/**
 *	Static python factory method
 */
PyObject * SkeletonCollider::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.SkeletonCollider brooks no arguments (%d given)",
			PyTuple_Size( args ) );
		return NULL;
	}

	return new SkeletonCollider( );
}

BW_END_NAMESPACE

// skeleton_collider.cpp
