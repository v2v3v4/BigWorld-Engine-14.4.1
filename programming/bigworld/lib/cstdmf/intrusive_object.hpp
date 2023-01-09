#ifndef INTRUSIVE_OBJECT_HPP
#define INTRUSIVE_OBJECT_HPP

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stdmf_minimal.hpp"
#include "debug.hpp"

#include <limits>

BW_BEGIN_NAMESPACE

/**
 *	This is the base class for all intrusive objects. An intrusive object is an
 *	object that automatically inserts itself into a collection when it is
 *	created and removes itself when it is destroyed.
 */
template < class ELEMENT >
class IntrusiveObject
{
public:
	/// This typedef specifies the container type that these intrusive objects
	/// will insert themselves into.
	typedef BW::vector< ELEMENT * > Container;
protected:
	typedef typename Container::size_type size_type;


	/**
	 *	The constructor takes reference to the a pointer that points to the
	 *	container that the object will insert itself into. If the container does
	 *	not yet exist, it will be created.
	 */
	IntrusiveObject( Container *& pContainer, bool shouldAdd = true ) :
		pContainer_( pContainer ),
		containerPos_( std::numeric_limits< size_type >::max() )
	{
		if (shouldAdd)
		{
			if (pContainer_ == NULL)
			{
				pContainer_ = new Container;
			}

			// We store the position in the container so we can do a fast 
			// deletion and erase on destruction.
			containerPos_ = pContainer_->size();

			pContainer_->push_back( pThis() );
		}
	}


	/**
	 *  The copy constructor which inserts the new object in the container.
	 */
	IntrusiveObject( const IntrusiveObject & source ) :
		pContainer_( source.pContainer_ ),
		containerPos_( source.pContainer_->size() )
	{
		pContainer_->push_back( pThis() );
	}


	/**
	 *	The destructor automatically removes this object from the list it was
	 *	inserted into. If this leaves the collection empty, the collection is
	 *	deleted.
	 */
	virtual ~IntrusiveObject()
	{
		if (pContainer_ != NULL &&
			containerPos_ != std::numeric_limits< size_type >::max())
		{
			// Check that an external class hasn't shuffled us around.  It's a
			// bit unfortunate that we need to do this, but it gives us speed
			// and keeps the code simple.
			MF_ASSERT( pContainer_->at( containerPos_ ) == pThis() );

			// For speed, we simply swap the cur element with the last one, and
			// tell the last one what its new position is.
			pContainer_->back()->containerPos_ = containerPos_;
			(*pContainer_)[ containerPos_ ] = pContainer_->back();
			pContainer_->pop_back();

			if (pContainer_->empty())
			{
				bw_safe_delete(pContainer_);
			}
		}
	}

private:
	/// This method casts to the derived class type.
	ELEMENT * pThis()
	{
		return static_cast< ELEMENT * >(
				const_cast< IntrusiveObject< ELEMENT > * >( this ) );
	}

	Container *& pContainer_;

	size_type containerPos_;
};

BW_END_NAMESPACE

#endif // INTRUSIVE_OBJECT_HPP

/* intrusive_object.hpp */
