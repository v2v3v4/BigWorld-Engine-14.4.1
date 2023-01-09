#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: VectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This is the destructor for Vector Generator.
 */
INLINE VectorGenerator::~VectorGenerator()
{
}


// -----------------------------------------------------------------------------
// Section: PointVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for PointVectorGenerator.
 *
 *	@param	point	Point from where the vectors are generated.
 */
INLINE PointVectorGenerator::PointVectorGenerator( const Vector3 &point )
{
	this->position( point );
}

/**
 *	This method generates the vectors for PointVectorGenerator.
 *
 *	@param	vector	Vector to be over-written with the new vector.
 */
INLINE void PointVectorGenerator::generate( Vector3 &vector ) const
{
	vector = position_;
}


// -----------------------------------------------------------------------------
// Section: LineVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for LineVectorGenerator.
 *
 *	@param start	The start point of the line interval.
 *	@param end		The end point of the line interval.
 */
INLINE LineVectorGenerator::LineVectorGenerator( const Vector3 &start,
		const Vector3 &end )
{
	this->start( start );
	this->end( end );
}


/**
 *	This is the Get-Accessor for the start position of the line vector.
 *
 *	@return The start position of the line.
 */
INLINE Vector3 LineVectorGenerator::start() const
{
	return origin_;
}


/**
 *	This is the Set-Accessor for the start position of the line vector.
 *
 *	@param pos The start position of the line.
 */
INLINE void LineVectorGenerator::start( const Vector3 & pos )
{
	// get the original end point
	Vector3 endPoint( this->end() );

	// set the new origin
	origin_ = pos;

	// restore original end point
	this->end( endPoint );
}


/**
 *	This is the Get-Accessor for the end position of the line vector.
 *
 *	@return The end position of the line.
 */
INLINE Vector3 LineVectorGenerator::end() const
{
	return origin_ + direction_;
}


/**
 *	This is the Set-Accessor for the end position of the line vector.
 *
 *	@param end The end position of the line.
 */
INLINE void LineVectorGenerator::end( const Vector3 & end )
{
	direction_ = end - origin_;
}


/**
 *	This method generates the vectors for LineVectorGenerator.
 *
 *	@param	vector	Vector to be over-written with the new vector.
 */
INLINE void LineVectorGenerator::generate( Vector3 &vector ) const
{
	vector = origin_ + unitRand() * direction_;
}


// -----------------------------------------------------------------------------
// Section: BoxVectorGenerator
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for BoxVectorGenerator.
 *
 *	@param corner			One corner of the box.
 *	@param oppositeCorner	The opposite corner of the box.
 */
INLINE BoxVectorGenerator::BoxVectorGenerator( const Vector3 &corner,
		const Vector3 &oppositeCorner )
{
	this->corner( corner );
	this->oppositeCorner( oppositeCorner );
}


/* vector_generator.ipp */
