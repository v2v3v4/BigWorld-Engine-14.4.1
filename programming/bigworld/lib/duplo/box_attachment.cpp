#include "pch.hpp"
#include "box_attachment.hpp"
#include "moo/custom_mesh.hpp"
#include "moo/material.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/geometrics.hpp"

DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BoxAttachment
// -----------------------------------------------------------------------------

/*~	class BigWorld.BoxAttachment
 *
 *	This class implements a bounding box that can be attached to a node
 *	to perform hit testing.
 */
PY_TYPEOBJECT( BoxAttachment )

PY_BEGIN_METHODS( BoxAttachment )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BoxAttachment )
	/*~ attribute BoxAttachment.minBounds
	 *	The minimum bounds of the hit box
	 *	@type Vector3.
	 */
	PY_ATTRIBUTE( minBounds )
	/*~ attribute BoxAttachment.maxBounds
	 *	The maximum bounds of the hit box
	 *	@type Vector3.
	 */
	PY_ATTRIBUTE( maxBounds )
	/*~ attribute BoxAttachment.name
	 *	The name of the box attachment
	 *	@type String.
	 */
	PY_ATTRIBUTE( name )
	/*~ attribute BoxAttachment.worldTransform
	 *	The world transform of the box attachment
	 *	@type Matrix.
	 */
	PY_ATTRIBUTE( worldTransform )
	/*~ attribute BoxAttachment.hit
	 *	Whether the box attachment was hit in the last hit test
	 *	@type bool.
	 */
	PY_ATTRIBUTE( hit )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( BoxAttachment )

/*~ function BigWorld.BoxAttachment
 *  This method creates a BoxAttachment and returns it.
 *	@return Returns a new BoxAttachment object.
 */
PY_FACTORY( BoxAttachment, BigWorld )

static bool s_displayImpactBoxes = false;
static BW::string s_lastBox;


/**
 *	This helper class creates a list of indices mapping from
 *	names to index.
 */
class NameList
{
public:

	typedef BW::vector<BW::string> Names;

	NameList()
	{
		names_.push_back( "" );
	}
	~NameList()
	{

	}
	const BW::string& get( uint16 index )
	{
		return names_[index];
	}
	const uint16 index( const BW::string& name )
	{
		MF_ASSERT( names_.size() <= USHRT_MAX );
		uint16 ind = ( uint16 )names_.size();
		Names::iterator it = std::find( names_.begin(), names_.end(), name );
		if (it != names_.end())
		{
			ind = (uint16) ( it - names_.begin() );
		}
		else
		{
			names_.push_back( name );
		}
		return ind;
	}


private:
	Names names_;

};

static NameList s_nameList;


/**
 *	Constructor.
 */
BoxAttachment::BoxAttachment(PyTypeObject * pType) :
	PyAttachment( pType ),
	minBounds_( 0, 0, 0 ),
	maxBounds_( 0, 0, 0 ),
	hit_( false ),
	nameIndex_( 0 )
{
	BW_GUARD;
	static bool firstTime = true;
	if (firstTime)
	{
		MF_WATCH( "Client Settings/displayImpactBoxes", s_displayImpactBoxes, Watcher::WT_READ_WRITE, "Display boxes that have been hit with collision tests for this frame." );
		MF_WATCH( "Client Settings/lastBoxHit", s_lastBox, Watcher::WT_READ_WRITE, "The name of the last box hit by a collision test" );
		firstTime = false;
	}
}


/**
 *	Destructor.
 */
BoxAttachment::~BoxAttachment()
{
}


void BoxAttachment::tick( float dTime )
{
}


/**
 *	This method stores the world transform of the box attachment
 *	it also performs debug rendering of the attachment if needed
 *	@param worldTransform the world transform to render at
 *	@param lod unused parameter
 */
void BoxAttachment::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
	BW_GUARD;
	worldTransform_ = worldTransform;

	if (s_displayImpactBoxes)
	{
		Moo::rc().push();
		Moo::rc().world(worldTransform);
		Geometrics::rgbBox(BoundingBox(minBounds_, maxBounds_));
		Moo::rc().pop();
	}
}

/**
 * Collide method, works out wether the ray intersects with this box attachment
 *
 * @param start the start point in world space of the ray
 * @param direction the direction of the ray
 * @param distance if this value is > 0 when passed in, it is the distance to 
 *	the end of the ray, it is also the return value for the distance to the
 *	collision impact
 *	@return true if the ray collided
 */
bool BoxAttachment::collide( const Vector3& start, const Vector3& direction, float& distance )
{
	BW_GUARD;
	hit_ = false;
	Matrix inv = worldTransform_;
    inv.invert();

	Vector3 lStart = inv.applyPoint( start );
	Vector3 lEnd;
	if (distance > 0)
		lEnd = direction * distance + start;
	else
		lEnd = direction * Moo::rc().camera().farPlane() + start;
	lEnd = inv.applyPoint( lEnd );
	BoundingBox bb;
	bb.setBounds( minBounds_, maxBounds_ );
	if (bb.clip(lStart, lEnd))
	{
		Vector3 end = worldTransform_.applyPoint(lStart);
		distance = (end - start).length();
		hit_ = true;
		s_lastBox = name();
	}
	return hit_;
}

/**
 *	This method saves the box attachment to a datasection
 */
void BoxAttachment::save( DataSectionPtr pSection )
{
	BW_GUARD;
	DataSectionPtr pSect = pSection->newSection( "BoxAttachment" );
	pSect->writeString( "name", name() );
	pSect->writeVector3( "minBounds", minBounds_ );
	pSect->writeVector3( "maxBounds", maxBounds_ );
}

/**
 *	This method loads the box attachment from a datasection
 */
void BoxAttachment::load( DataSectionPtr pSection )
{
	BW_GUARD;
	name(pSection->readString( "name" ));
	minBounds_ = pSection->readVector3( "minBounds" );
	maxBounds_ = pSection->readVector3( "maxBounds" );
}

/**
 *	This method sets the name of this box attachment
 *	@param name the name of the attachment
 */
void BoxAttachment::name( const BW::string& name )
{
	BW_GUARD;
	nameIndex_ = s_nameList.index( name );
}

/**
 *	This method gets the name of the box attachment
 *	@return the name of the attachment
 */
const BW::string& BoxAttachment::name() const
{
	BW_GUARD;
	return s_nameList.get( nameIndex_ );
}

/*
 *	Python method to return the name
 */
PyObject * BoxAttachment::pyGet_name()
{
	BW_GUARD;
	return PyString_FromString( name().c_str() );
}

/*
 *	Python method to set the name
 */
int BoxAttachment::pySet_name( PyObject * pValue )
{
	BW_GUARD;
	if (PyString_Check( pValue ))
		name( PyString_AsString( pValue ) );
	else
	{
		PyErr_Format( PyExc_TypeError,
			"Bad type for BoxAttachment.name, string expected" );
		return -1;
	}
	return 0;
}


/**
 *	Static python factory method
 */
PyObject * BoxAttachment::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.BoxAttachment brooks no arguments (%d given)",
			PyTuple_Size( args ) );
		return NULL;
	}

	return new BoxAttachment( );
}

BW_END_NAMESPACE

// box_attachment.cpp
