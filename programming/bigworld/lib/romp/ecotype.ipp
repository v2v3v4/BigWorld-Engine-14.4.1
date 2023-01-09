#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// INLINE void Ecotype::inlineFunction()
// {
// }


INLINE float
Ecotype::getRelativeDensity( void ) const
{
	return relativeDensity_;
}


INLINE
void
Ecotype::relativeDensity( float d )
{
	relativeDensity_ = d;
}


INLINE void
Ecotype::alignToSurface( bool state )
{
	alignToSurface_ = state;
}


INLINE bool
Ecotype::alignToSurface( void ) const
{
	return alignToSurface_;
}


///Retrieves the next available object
INLINE DetailObject * Ecotype::allocate( void )
{
	MF_ASSERT( this->idx_ < numUnits_-1 );
	this->idx_++;

	DetailObject * unit = unitArray_[ this->idx_ ];

	unit->setIdx_ = this->idx_;
	unit->allocated_ = true;
	unit->dirtyWorldTransform_ = true;

	//update subset usage
	if ( unit->subSet_ != -1 )
	{
		unitTypes_[ unit->subSet_ ].usage_++;
	}

	return unit;
}


///Deallocates a previously allocated object.
INLINE void Ecotype::deallocate( DetailObject * unit )
{
	MF_ASSERT( unit );
	MF_ASSERT( unit->setIdx_ <= this->idx_ );
	MF_ASSERT( this->idx_ >= 0 );

	unit->allocated_ = false;
	unit->dirtyWorldTransform_ = true;

	//update subset usage
	if ( unit->subSet_ != -1 )
	{
		unitTypes_[ unit->subSet_ ].usage_--;
	}

	//trivial case
	if ( unit->setIdx_ == this->idx_ )
	{
		this->idx_--;
	}
	else
	{
		//we are removing from the middle of our array
		//copy the last valid element over this element
		DetailObjectPtr temp = unitArray_[ unit->setIdx_ ];
		temp->dirtyWorldTransform_ = true;
		unitArray_[ unit->setIdx_ ] = unitArray_[ this->idx_ ];
		//copy the middle one to the end
		unitArray_[ this->idx_ ] = temp;
		//make sure we set the set index for the new valid one!
		unitArray_[ unit->setIdx_ ]->setIdx_ = unit->setIdx_;
		this->idx_--;
	}
}


INLINE bool	Ecotype::animates( uint detailType ) const
{
	MF_ASSERT( detailType < unitTypes_.size() );
	MF_ASSERT( detailType >= 0 );

	return unitTypes_[ detailType ].animate_;
}



INLINE void	Ecotype::windAmount( float windX, float windZ )
{
	windAmount_ = max( windX, windZ );
	windAmount_ = min( windAmount_, 1.f );
	windAmount_ = max( windAmount_, 0.f );
}


/*ecotype.ipp*/