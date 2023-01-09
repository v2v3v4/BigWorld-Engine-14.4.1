#include "pch.hpp"

#include "vector_generator.hpp"
#include "particle/particle_serialisation.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


#include <cmath>


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "vector_generator.ipp"
#endif

const BW::string PointVectorGenerator::nameID_ = "PointVectorGenerator";
const BW::string LineVectorGenerator::nameID_ = "LineVectorGenerator";
const BW::string CylinderVectorGenerator::nameID_ = "CylinderVectorGenerator";
const BW::string SphereVectorGenerator::nameID_ = "SphereVectorGenerator";
const BW::string BoxVectorGenerator::nameID_ = "BoxVectorGenerator";


// -----------------------------------------------------------------------------
// Section: VectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This method creates a Vector3Generator, given a python list object. It
 *	uses the parsePythonTuple static methods of each of its children in
 *	order to parse the details.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a list with a string for the head. The string
 *					represents the type of the vector generator desired.
 *					The tail of the list is passed to the specific generator
 *					to be parsed further.
 *
 *	@return	A pointer to the newly created vector generator if everything was
 *			created without problems. NULL, otherwise.
 */
VectorGenerator *VectorGenerator::parseFromPython( PyObject *args )
{
	BW_GUARD;
	// PySequence_Check( args )
	// PySequence_GetItem( args, i ) // new reference
	PyObject *pGeneratorType = PyList_GetItem( args, 0 );

	// Get the string containing the type of the VectorGenerator desired.
	// It should be found in the head of the arguments list.
	if ( ( pGeneratorType != NULL ) && PyString_Check( pGeneratorType ) )
	{
		// Retrieve the tail of the list. The argsAsTuple object is not a
		// borrowed reference so it has to be deferenced. The same applies to
		// the tail object.
		PyObject *argsAsTuple = PyList_AsTuple( args );
		PyObject *tail = PyTuple_GetSlice( argsAsTuple, 1,
			PyTuple_Size( argsAsTuple ) );
		Py_DECREF( argsAsTuple );

		char *generatorType = PyString_AsString( pGeneratorType );

		// Based on the type of the VectorGenerator, parse the rest of the
		// list. The base class does not know how to do this, it only knows
		// what identifies a derived class - once it identifies which class
		// is requested, it passed the information on the the derived classes.
		if ( !bw_stricmp( generatorType, "Point" ) )
		{
			return PointVectorGenerator::parsePythonTuple( tail );
		}
		else if  ( !bw_stricmp( generatorType, "Line" ) )
		{
			return LineVectorGenerator::parsePythonTuple( tail );
		}
		else if  ( !bw_stricmp( generatorType, "Cylinder" ) )
		{
			return CylinderVectorGenerator::parsePythonTuple( tail );
		}
		else if  ( !bw_stricmp( generatorType, "Sphere" ) )
		{
			return SphereVectorGenerator::parsePythonTuple( tail );
		}
		else if  ( !bw_stricmp( generatorType, "Box" ) )
		{
			return BoxVectorGenerator::parsePythonTuple( tail );
		}
		else
		{
			PyErr_Format( PyExc_TypeError,
				"VectorGenerator: Unknown or unregistered generator %s.",
				generatorType );
			return NULL;
		}
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "VectorGenerator:"
			"VectorGenerator name <string> required at head of list." );
		return NULL;
	}
}


void VectorGenerator::load( DataSectionPtr pSect )
{
	BW_GUARD;
	loadInternal( pSect );
}

void VectorGenerator::save( DataSectionPtr pSect ) const
{
	BW_GUARD;
	const BW::string & name = nameID();
	pSect = pSect->newSection( name );
	pSect->writeString( "nameID_", name );
	saveInternal( pSect );
}

VectorGenerator * VectorGenerator::createGeneratorOfType(const BW::string & type)
{
	BW_GUARD;
	if (type == PointVectorGenerator::nameID_)
	{
		return new PointVectorGenerator();
	}
	else if (type == LineVectorGenerator::nameID_)
	{
		return new LineVectorGenerator();
	}
	else if (type == CylinderVectorGenerator::nameID_)
	{
		return new CylinderVectorGenerator();
	}
	else if (type == SphereVectorGenerator::nameID_)
	{
		return new SphereVectorGenerator();
	}
	else if (type == BoxVectorGenerator::nameID_)
	{
		return new BoxVectorGenerator();
	}

	MF_EXIT( "VectorGenerator::createGeneratorOfType: requesting creation of invalid generator type" );

	return NULL;
}


// -----------------------------------------------------------------------------
// Section: PointVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This method parses a Python tuple and builds a PointVectorGenerator.
 *	The arguments accepted are a single triple of numbers. Args is not a
 *	borrowed object so it needs to have its reference count decremented
 *	after use.
 *
 *	@param args		Python tuple containing initialisation information.
 *
 *	@return A pointer to the newly created vector generator if successful;
 *			NULL, otherwise.
 */
PointVectorGenerator *PointVectorGenerator::parsePythonTuple( PyObject *args )
{
	BW_GUARD;
	float x, y, z;

	if ( PyArg_ParseTuple( args, "(fff)", &x, &y, &z ) )
	{
		Vector3 point( x, y, z );
		Py_DECREF( args );

		return new PointVectorGenerator( point );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "PointVectorGenerator:"
			"Expected (x,y,z)." );
		Py_DECREF( args );
		return NULL;
	}
}


template < typename Serialiser >
void PointVectorGenerator::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, PointVectorGenerator, position );
}


void PointVectorGenerator::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< PointVectorGenerator >( pSect,
		this ) );
}


void PointVectorGenerator::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< PointVectorGenerator >( pSect,
		this ) );
}


PointVectorGenerator * PointVectorGenerator::clone() const
{
	BW_GUARD;
	PointVectorGenerator * vg = new PointVectorGenerator();
	this->serialise( BW::CloneObject< PointVectorGenerator >( this, vg ) );
	return vg;
}


// -----------------------------------------------------------------------------
// Section: LineVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This method parses a Python tuple and builds a LineVectorGenerator.
 *	The arguments accepted are two triples of numbers. Args is not a
 *	borrowed object so it needs to have its reference count decremented
 *	after use.
 *
 *	@param args		Python tuple containing initialisation information.
 *
 *	@return A pointer to the newly created vector generator if successful;
 *			NULL, otherwise.
 */
LineVectorGenerator *LineVectorGenerator::parsePythonTuple( PyObject *args )
{
	BW_GUARD;
	float x1, y1, z1;
	float x2, y2, z2;

	if ( PyArg_ParseTuple( args, "(fff)(fff)",
		&x1, &y1, &z1,
		&x2, &y2, &z2 ) )
	{
		Vector3 point1( x1, y1, z1 );
		Vector3 point2( x2, y2, z2 );

		Py_DECREF( args );
		return new LineVectorGenerator( point1, point2 );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "LineVectorGenerator:"
			"Expected (x1,y1,z1), (x2,y2,z2)." );
		Py_DECREF( args );
		return NULL;
	}
}

/**
 *	serialise saves / restores the state of the object
 */
template < typename Serialiser >
void LineVectorGenerator::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, LineVectorGenerator, origin );
	BW_PARITCLE_SERIALISE( s, LineVectorGenerator, direction );
}


void LineVectorGenerator::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< LineVectorGenerator >( pSect,
		this ) );
}


void LineVectorGenerator::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< LineVectorGenerator >( pSect,
		this ) );
}


LineVectorGenerator * LineVectorGenerator::clone() const
{
	BW_GUARD;
	LineVectorGenerator * vg = new LineVectorGenerator();
	this->serialise( BW::CloneObject< LineVectorGenerator >( this, vg ) );
	return vg;
}


// -----------------------------------------------------------------------------
// Section: CylinderVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for CylinderVectorGenerator.
 *
 *	@param Source		The centre of the first circle endpoint.
 *	@param Destination	The centre of the second circle endpoint.
 *	@param MaxRad	The outer radius of the cylinder.
 *	@param MinRad	(Optional) The inner radius of the cylinder.
 */
CylinderVectorGenerator::CylinderVectorGenerator( const Vector3 &Source,
		const Vector3 &Destination,
		float MaxRad,
		float MinRad ) :
	origin_( Source ),
	direction_( Destination - Source ),
	maxRadius_( MaxRad ),
	minRadius_( MinRad )
{
	BW_GUARD;
	this->constructBasis();
	this->checkRadii();
}

void CylinderVectorGenerator::constructBasis()
{
	BW_GUARD;
	// Try x-basis vector or y-basis vector. The more orthogonal, the better.
	Vector3 normalisedDirection = direction_;
	normalisedDirection.normalise();
	Vector3 basis( 1.0f, 0.0f, 0.0f );
	if ( almostEqual( 1.0f, fabs( basis.dotProduct( normalisedDirection ) ), 0.0001f ) )
	{
		basis.set( 0.0f, 1.0f, 0.0f );
	}

	// Build basis vectors along plane cross-section of cylinder.
	basisU_ = basis.crossProduct( direction_ );
	basisU_.normalise();

	basisV_ = direction_.crossProduct( basisU_ );
	basisV_.normalise();
}

void CylinderVectorGenerator::checkRadii()
{
	//Bug 5075 fix: Removed the check of the minimumRadius since it isn't used.
	minRadius_ = 0.0f; // Should always be zero for cylinders

	// Check that the radius is positive, if not, set it to zero.
	if ( maxRadius_ < 0.0f )
	{
		maxRadius_ = 0.0f;
	}
}

void CylinderVectorGenerator::updateOrigin(const Vector3 & pos)
{
	Vector3 oldDestination = destination();
	origin_ = pos;
	destination(oldDestination);
}

void CylinderVectorGenerator::destination(const Vector3 & destination)
{
	direction_ = destination - origin_;
	constructBasis();
}


/**
 *	This method generates the vectors for CylinderVectorGenerator.
 *
 *	@param	vector	Vector to be over-written with the new vector.
 */
void CylinderVectorGenerator::generate( Vector3 &vector ) const
{
	BW_GUARD;
	// Get a point along the axis of the cylinder.
	vector = origin_ + unitRand() * direction_;

    // Make sure that the radii are sorted:
    float minR = minRadius_;
    float maxR = maxRadius_;
    if (minR > maxR) 
        std::swap(minR, maxR);

    // Choosing points in a disc uniformly is not as easy as you may first
    // think.  Choosing the angle and radius randomly gives a distribution
    // concentrated at the center.  A correct formula is given by:
    //    http://mathworld.wolfram.com/DiskPointPicking.html
    // Here we adjust for the fact that we are not on the unit disc, but on
    // an annulus.  To do this we convert the radius range from 
    // [minRadius_, maxRadius] to [alpha, 1.0] where 
    // alpha = minRadius_/maxRadius_, do the sqrt random distribution and
    // then convert back to the range [minRadius_, maxRadius].
    float angle = unitRand() * 2.0f * MATH_PI;
    float r;
    if (minR == maxR) // handle case where radii are equal (or both zero)
    {
        r = minR;
    }
    else
    {        
        float alpha = minR/maxR;
        float k1    = std::sqrtf( unitRand() * ( 1.0f - alpha ) + minR );
        r  = Math::lerp( k1, alpha, 1.0f, minR, maxR );
    }
    vector += r * ( std::cosf( angle ) * basisU_ + std::sinf( angle ) * basisV_ );
}

/**
 *	This method parses a Python tuple and builds a CylinderVectorGenerator.
 *	The arguments accepted are two triples of numbers and two numbers. Args
 *	is not a borrowed object so it needs to have its reference count
 *	decremented after use.
 *
 *	@param args	Python tuple containing initialisation information.
 *
 *	@return A pointer to the newly created vector generator if successful;
 *			NULL, otherwise.
 */
CylinderVectorGenerator *CylinderVectorGenerator::parsePythonTuple(
		PyObject *args )
{
	BW_GUARD;
	float x1, y1, z1;
	float x2, y2, z2;
	float maxRadius, minRadius;

	if ( PyArg_ParseTuple( args, "(fff)(fff)ff",
		&x1, &y1, &z1,
		&x2, &y2, &z2,
		&maxRadius, &minRadius ) )
	{
		Vector3 point1( x1, y1, z1 );
		Vector3 point2( x2, y2, z2 );

		Py_DECREF( args );
		return new CylinderVectorGenerator( point1, point2, maxRadius,
			minRadius );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "CylinderVectorGenerator:"
			"Expected (x1,y1,z1), (x2,y2,z2), maxRadius, minRadius." );
		Py_DECREF( args );
		return NULL;
	}
}

/**
 *	serialise saves / restores the state of the object
 */
template < typename Serialiser >
void CylinderVectorGenerator::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, CylinderVectorGenerator, origin );
	BW_PARITCLE_SERIALISE( s, CylinderVectorGenerator, direction );
	BW_PARITCLE_SERIALISE( s, CylinderVectorGenerator, maxRadius );
	BW_PARITCLE_SERIALISE( s, CylinderVectorGenerator, minRadius );

	BW_PARITCLE_SERIALISE( s, CylinderVectorGenerator, basisU );
	BW_PARITCLE_SERIALISE( s, CylinderVectorGenerator, basisV );
}


void CylinderVectorGenerator::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< CylinderVectorGenerator >( pSect,
		this ) );
}


void CylinderVectorGenerator::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< CylinderVectorGenerator >( pSect,
		this ) );
}


CylinderVectorGenerator * CylinderVectorGenerator::clone() const
{
	BW_GUARD;
	CylinderVectorGenerator * vg = new CylinderVectorGenerator();
	this->serialise( BW::CloneObject< CylinderVectorGenerator >( this, vg ) );
	return vg;
}


// -----------------------------------------------------------------------------
// Section: SphereVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for SphereVectorGenerator.
 *
 * 	@param Centre		The centre of the sphere.
 * 	@param MaxRadius	The radius of the sphere.
 * 	@param MinRadius	The radius of the hole in the centre of the sphere.
 */
SphereVectorGenerator::SphereVectorGenerator( const Vector3 &Centre,
		float MaxRadius,
		float MinRadius )
{
	centre(Centre);
	maxRadius(MaxRadius);
	minRadius(MinRadius);
}

void SphereVectorGenerator::checkRadii()
{
	BW_GUARD;
	// Check that the radii are both positive, if not, set them to zero.
	if ( maxRadius_ < 0.0f )
	{
		maxRadius_ = 0.0f;
	}
	if ( minRadius_ < 0.0f )
	{
		minRadius_ = 0.0f;
	}

	// Check that the maximum radius is greater or equal to the minimum
	// radius. If not, set the maximum radius to minimum radius.
	if ( maxRadius_ < minRadius_ )
	{
		maxRadius_ = minRadius_;
	}
}

class SphereVectorCache
{
	static const int CACHE_SIZE = 4096;
	static const int MAX_POW_INDEX = CACHE_SIZE - 1;
	Vector3 vectorCache_[ CACHE_SIZE ];
	float powCache_[ CACHE_SIZE ];
public:
	SphereVectorCache()
	{
		// Generate a random point on the unit sphere
		// (based upon http://mathworld.wolfram.com/SpherePointPicking.html)
		// and also cache the expensive pow calls because we know we only get
		// powers of a num between 0 and 1, and always to to 1/3.
		for (int i = 0; i < CACHE_SIZE; ++i)
		{
			float theta  = 2.0f * MATH_PI * unitRand();
			float phi    = std::acosf( 2.0f * unitRand() - 1.0f );
			float sinphi = std::sinf( phi );
			float x      = std::cosf( theta ) * sinphi;
			float y      = std::sinf( theta ) * sinphi;
			float z      = std::cosf( phi );

			vectorCache_[ i ] = Vector3(x, y, z);

			powCache_[ i ] =
					std::powf( float( i ) / float( CACHE_SIZE ) , 1.0f/3.0f);
		}
	}

	const Vector3& getVector() const
	{
		return vectorCache_[ bw_random() % CACHE_SIZE ];
	}

	float getPow( float num ) const
	{
		// We know num is from 0 to 1, but put guards in case someone changes
		// the formula for it.
		int idx = int( num * MAX_POW_INDEX );
		if (idx > MAX_POW_INDEX)
		{
			idx = MAX_POW_INDEX;
			WARNING_MSG( "SphereVectorCache::getPow "
									"called with number greater than 1.\n" );
		}
		else if (idx < 0)
		{
			idx = 0;
			WARNING_MSG( "SphereVectorCache::getPow "
										"called with number less than 0.\n" );
		}
		return powCache_[ idx ];
	}
}
s_sphereVectorCache;


/**
 *	This method generates the vectors for SphereVectorGenerator.
 *
 *	@param	vector	Vector to be over-written with the new vector.
 */
void SphereVectorGenerator::generate( Vector3 &vector ) const
{
	vector = s_sphereVectorCache.getVector();

    // Choose a random radius, taking into account that spherical shells
    // go as r^2 dr (integrate to give the volume of the shell and then
    // bias the choice of radius based upon that).
    float r;

	if (minRadius_ == maxRadius_) // handle case where radii are equal (or both zero)
    {
        r = minRadius_;
    }
    else
    {
        float alpha = minRadius_ / maxRadius_;
        float k1	= s_sphereVectorCache.getPow(
									unitRand() * ( 1.0f - alpha ) + alpha );
        r           = k1 * maxRadius_;
    }

	// Scale the point to the sphere we want and move it into position.
	vector = r*vector + centre_;
}

/**
 *	This method parses a Python tuple and builds a SphereVectorGenerator.
 *	The arguments accepted are a triple of numbers and two numbers. Args
 *	is not a borrowed object so it needs to have its reference count
 *	decremented after use.
 *
 *	@param args	Python tuple containing initialisation information.
 *
 *	@return A pointer to the newly created vector generator if successful;
 *			NULL, otherwise.
 */
SphereVectorGenerator *SphereVectorGenerator::parsePythonTuple(
		PyObject *args )
{
	BW_GUARD;
	float x, y, z;
	float maxRadius, minRadius;

	if ( PyArg_ParseTuple( args, "(fff)ff",
		&x, &y, &z,
		&maxRadius, &minRadius ) )
	{
		Vector3 point( x, y, z );

		Py_DECREF( args );
		return new SphereVectorGenerator( point, maxRadius, minRadius );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "SphereVectorGenerator:"
			"Expected (x,y,z), maxRadius, minRadius." );
		Py_DECREF( args );
		return NULL;
	}
}

/**
 *	serialise saves / restores the state of the object
 */
template < typename Serialiser >
void SphereVectorGenerator::serialise( const Serialiser & s ) const
{
	BW_GUARD;
	BW_PARITCLE_SERIALISE( s, SphereVectorGenerator, centre );
	BW_PARITCLE_SERIALISE( s, SphereVectorGenerator, maxRadius );
	BW_PARITCLE_SERIALISE( s, SphereVectorGenerator, minRadius );
}


void SphereVectorGenerator::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< SphereVectorGenerator >( pSect,
		this ) );
}


void SphereVectorGenerator::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< SphereVectorGenerator >( pSect,
		this ) );
}


SphereVectorGenerator * SphereVectorGenerator::clone() const
{
	SphereVectorGenerator * vg = new SphereVectorGenerator();
	this->serialise( BW::CloneObject< SphereVectorGenerator >( this, vg ) );
	return vg;
}


// -----------------------------------------------------------------------------
// Section: BoxVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This method generates the vectors for BoxVectorGenerator.
 *
 *	@param	vector	Vector to be over-written with the new vector.
 */
void BoxVectorGenerator::generate( Vector3 &vector ) const
{
	vector.set( corner_.x + unitRand() * ( opposite_.x - corner_.x ),
		corner_.y + unitRand() * ( opposite_.y - corner_.y ),
		corner_.z + unitRand() * ( opposite_.z - corner_.z ) );
}

/**
 *	This method parses a Python tuple and builds a BoxVectorGenerator.
 *	The arguments accepted are two triples of numbers. Args is not a borrowed
 *	object so it needs to have its reference count decremented after use.
 *
 *	@param args	Python tuple containing initialisation information.
 *
 *	@return A pointer to the newly created vector generator if successful;
 *			NULL, otherwise.
 */
BoxVectorGenerator *BoxVectorGenerator::parsePythonTuple( PyObject *args )
{
	BW_GUARD;
	float x1, y1, z1;
	float x2, y2, z2;

	if ( PyArg_ParseTuple( args, "(fff)(fff)",
		&x1, &y1, &z1,
		&x2, &y2, &z2 ) )
	{
		Vector3 point1( x1, y1, z1 );
		Vector3 point2( x2, y2, z2 );

		Py_DECREF( args );
		return new BoxVectorGenerator( point1, point2 );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "BoxVectorGenerator:"
			"Expected (x1,y1,z1), (x2,y2,z2)." );
		Py_DECREF( args );
		return NULL;
	}
}

/**
 *	serialise saves / restores the state of the object
 */
template < typename Serialiser >
void BoxVectorGenerator::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, BoxVectorGenerator, corner );
	// tagged "opposite_" for legacy reasons
	BW_PARITCLE_SERIALISE_NAMED( s, BoxVectorGenerator, oppositeCorner,
		"opposite_" );
}


void BoxVectorGenerator::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< BoxVectorGenerator >( pSect,
		this ) );
}


void BoxVectorGenerator::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< BoxVectorGenerator >( pSect,
		this ) );
}


BoxVectorGenerator * BoxVectorGenerator::clone() const
{
	BW_GUARD;
	BoxVectorGenerator * vg = new BoxVectorGenerator();
	this->serialise( BW::CloneObject< BoxVectorGenerator >( this, vg ) );
	return vg;
}


BW_END_NAMESPACE

/* vector_generator.cpp */
