// -----------------------------------------------------------------------------
// Section: CircularQueue
// -----------------------------------------------------------------------------

/**
 *	Default Constructor
 */
template<class T, class CONTAINER>
CircularQueue<T, CONTAINER>::CircularQueue() :
	start_( 0 ),
	count_( 0 )
{
	BW_GUARD;

}


/**
 *	Constructor
 */
template<class T, class CONTAINER>
CircularQueue<T, CONTAINER>::CircularQueue( size_t size ) :
	start_( 0 ),
	count_( 0 )
{
	BW_GUARD;

	this->resize( size );
}


/**
 *	This method sets the container size destructively.
 */
template<class T, class CONTAINER>
void CircularQueue<T, CONTAINER>::resize( size_t size )
{
	BW_GUARD;

	container_.resize( size );
	start_ = 0;
	count_ = 0;
}


/**
 *	This method removes all the elements
 */
template<class T, class CONTAINER>
void CircularQueue<T, CONTAINER>::clear()
{
	BW_GUARD;

	start_ = 0;
	count_ = 0;
}


/**
 *	This method adds a new element to the end of the CircularQueue if not full
 *	
 *	@return	A reference to the new element
 */
template<class T, class CONTAINER>
T & CircularQueue<T, CONTAINER>::push()
{
	BW_GUARD;

	MF_ASSERT( size() < capacity() );

	T & element = container_[ (start_ + count_) % capacity() ];

	++count_;

	return element;
}


/**
 *	This method adds the given element to the end of the CircularQueue if not
 *	full
 *	
 *	@return	A reference to the new element
 */
template<class T, class CONTAINER>
T & CircularQueue<T, CONTAINER>::push( const T & v )
{
	BW_GUARD;

	T &element = this->push();

	element = v;

	return element;
}


/**
 *	This method removes an element from the front of the CircularQueue
 */
template<class T, class CONTAINER>
void CircularQueue<T, CONTAINER>::pop()
{
	BW_GUARD;

	MF_ASSERT( size() );

	count_--;
	start_++;		

	if(start_ >= capacity())
	{
		start_ = 0;
	}
}


/**
 *	This method retrieves a reference to the element at the front of the
 *	CircularQueue
 *	
 *	@return	A reference to the front element
 */
template<class T, class CONTAINER>
T & CircularQueue<T, CONTAINER>::peek()
{
	BW_GUARD;

	MF_ASSERT( size() );

	return *begin();
}

/**
 *	This method retrieves a reference to the element at the front of the
 *	CircularQueue
 *	
 *	@return	A reference to the front element
 */
template<class T, class CONTAINER>
const T & CircularQueue<T, CONTAINER>::peek() const
{
	BW_GUARD;

	MF_ASSERT( size() );

	return *begin();
}


/**
 *	This method retrieves a reference to the element at the back of the
 *	CircularQueue
 *	
 *	@return	A reference to the back element
 */
template<class T, class CONTAINER>
T & CircularQueue<T, CONTAINER>::peek_back()
{
	BW_GUARD;

	MF_ASSERT( size() );

	return container_[ (start_ + count_ - 1) % capacity() ];
}

/**
 *	This method retrieves a reference to the element at the back of the
 *	CircularQueue
 *	
 *	@return	A reference to the back element
 */
template<class T, class CONTAINER>
const T & CircularQueue<T, CONTAINER>::peek_back() const
{
	BW_GUARD;

	MF_ASSERT( size() );

	return container_[ (start_ + count_ - 1) % capacity() ];
}


/**
 *	This method returns the number of elements in the CircularQueue
 *	
 *	@return	The number of elements in the CircularQueue
 */
template<class T, class CONTAINER>
size_t CircularQueue<T, CONTAINER>::size() const
{
	BW_GUARD;

	return count_;
}


/**
 *	This method returns the number of elements the CircularQueue can hold
 *	
 *	@return	The number of elements the CircularQueue can hold
 */
template<class T, class CONTAINER>
size_t CircularQueue<T, CONTAINER>::capacity() const
{
	BW_GUARD;

	return container_.size();
}


/**
 *	This operator access an element by index
 *	
 *	@return	A reference to the index'd element of the CircularQueue
 */
template<class T, class CONTAINER>
T & CircularQueue<T, CONTAINER>::operator[]( size_t index )
{
	BW_GUARD;

	return container_[ (start_ + index) % capacity() ];
}


/**
 *	This operator access an element by index
 *	
 *	@return	A reference to the index'd element of the CircularQueue
 */
template<class T, class CONTAINER>
const T & CircularQueue<T, CONTAINER>::operator[]( size_t index ) const
{
	BW_GUARD;

	return container_[ (start_ + index) % capacity() ];
}


/**
 *	This method returns whether this CircularQueue is full
 *	
 *	@return	True if this CircularQueue is full, otherwise false
 */
template<class T, class CONTAINER>
bool CircularQueue<T, CONTAINER>::full() const
{
	BW_GUARD;

	return size() == capacity();
}


/**
 *	This method returns whether this CircularQueue is empty
 *	
 *	@return	True if this CircularQueue is empty, otherwise false
 */
template<class T, class CONTAINER>
bool CircularQueue<T, CONTAINER>::empty() const
{
	BW_GUARD;

	return size() == 0;
}


/**
 *	This method returns an iterator pointing to the first element
 *	
 *	@return	end() if this CircularQueue is empty, otherwise a interator pointing
 *		to the first element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::iterator
	CircularQueue<T, CONTAINER>::begin()
{
	BW_GUARD;

	return iterator( *this, 0 );
}


/**
 *	This method returns an iterator pointing to the first element
 *	
 *	@return	end() if this CircularQueue is empty, otherwise a interator pointing
 *		to the first element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::const_iterator
	CircularQueue<T, CONTAINER>::begin() const
{
	BW_GUARD;

	return const_iterator( *this, 0 );
}


/**
 *	This method returns an iterator pointing past the last element
 *	
 *	@return	begin() if this CircularQueue is empty, otherwise a interator
 *		pointing past the last element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::iterator
	CircularQueue<T, CONTAINER>::end()
{
	BW_GUARD;

	return iterator( *this, this->size() );
}


/**
 *	This method returns an iterator pointing past the last element
 *	
 *	@return	begin() if this CircularQueue is empty, otherwise a interator
 *		pointing past the last element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::const_iterator
	CircularQueue<T, CONTAINER>::end() const
{
	BW_GUARD;

	return const_iterator( *this, this->size() );
}


/**
 *	This method returns a reverse iterator pointing to the last element
 *	
 *	@return	rend() if this CircularQueue is empty, otherwise a interator
 *		pointing to the last element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::reverse_iterator
	CircularQueue<T, CONTAINER>::rbegin()
{
	BW_GUARD;

	return reverse_iterator( this->end() );
}


/**
 *	This method returns a reverse iterator pointing to the last element
 *	
 *	@return	rend() if this CircularQueue is empty, otherwise a interator
 *		pointing to the last element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::const_reverse_iterator
	CircularQueue<T, CONTAINER>::rbegin() const
{
	BW_GUARD;

	return const_reverse_iterator( this->end() );
}


/**
 *	This method returns a reverse iterator pointing past the first element
 *	
 *	@return	rbegin() if this CircularQueue is empty, otherwise a interator
 *		pointing past the first element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::reverse_iterator
	CircularQueue<T, CONTAINER>::rend()
{
	BW_GUARD;

	return reverse_iterator( this->begin() );
}


/**
 *	This method returns a reverse iterator pointing past the first element
 *	
 *	@return	rbegin() if this CircularQueue is empty, otherwise a interator
 *		pointing past the first element
 */
template<class T, class CONTAINER>
typename CircularQueue<T, CONTAINER>::const_reverse_iterator
	CircularQueue<T, CONTAINER>::rend() const
{
	BW_GUARD;

	return const_reverse_iterator( this->begin() );
}


// -----------------------------------------------------------------------------
// Section: CircularQueue::IteratorBase
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
template<class T, class CONTAINER>
template<bool ISCONST>
CircularQueue<T, CONTAINER>::IteratorBase<ISCONST>::IteratorBase(
		owner_type & owner, size_t position ) :
	pOwner_( &owner ),
	position_( position )
{
	BW_GUARD;
}


/**
 *	Default Constructor
 */
template<class T, class CONTAINER>
template<bool ISCONST>
CircularQueue<T, CONTAINER>::IteratorBase<ISCONST>::IteratorBase() :
	pOwner_( NULL ),
	position_( -1 )
{
	BW_GUARD;
}


/**
 *	Copy-Constructor, and helpful implicit iterator->const_iterator conversion
 */
template<class T, class CONTAINER>
template<bool ISCONST>
CircularQueue<T, CONTAINER>::IteratorBase<ISCONST>::IteratorBase(
	const IteratorBase<false> & other ) :
	pOwner_( other.pOwner_ ),
	position_( other.position_ )
{
	BW_GUARD;
}


// circular_queue.ipp
