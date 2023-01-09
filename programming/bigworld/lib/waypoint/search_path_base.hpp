#ifndef SEARCH_PATH_BASE_HPP
#define SEARCH_PATH_BASE_HPP

#include "astar.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Base class for searched A* state paths. Each instance has access to current
 *	state, the next state, and the destination state. When the path is to be
 *	queried, clients call the matches() method with their current state and
 *	their intended destination state. 
 */
template< class SearchState, class SearchData = void* >
class SearchPathBase
{
protected:
	/**
	 *	Constructor.
	 */
	SearchPathBase():
		reversePath_()
	{}

	virtual ~SearchPathBase()
	{}

public:

	/**
	 *	This method initialises this search path from the given A* search.
	 */
	virtual void init( AStar< SearchState, SearchData > & astar ) = 0;


	/**
	 *	This method returns true if the two given search states are equivalent.
	 *	Subclasses must supply this method.
	 */
	virtual bool statesAreEquivalent( const SearchState & s1, 
			const SearchState & s2 ) const = 0;

	/**
	 *	Return true if the path is empty.
	 */
	bool empty() const
	{
		return reversePath_.empty();
	}


	/**
	 *	Return true if we have a valid path. This means that we have at least a
	 *	current state and the destination state.
	 */
	bool isValid() const
		{ return reversePath_.size() >= 2; }


	/**
	 *	Return the current search state, or NULL if the path is invalid.
	 */
	const SearchState * pCurrent() const
		{ return !this->empty() ? &reversePath_.back() : NULL; }


	/**
	 *	Return the next search state (which might be the destination), or NULL
	 *	if the path is invalid.
	 */
	const SearchState * pNext() const
	{ 
		return this->isValid() ?
			&reversePath_[ reversePath_.size() - 2 ] : 
			NULL; 
	}


	/**
	 *	Return our destination search state.
	 */
	const SearchState * pDest() const
		{ return !this->empty() ? &reversePath_.front(): NULL; }


	/**
	 *	Clear the path.
	 */
	void clear()
	{
		reversePath_.clear();
	}


	/**
	 *	Proceed to the next search state. This means that the previous next
	 *	state is now our new current state.
	 */
	void pop()
	{
		if (this->isValid())
		{
			reversePath_.pop_back();
		}
	}

	bool matches( const SearchState & src, const SearchState & dst );


	/**
	 *	Return whether the given destination state is equivalent to our stored
	 *	destination state.
	 */
	bool isDestinationState( const SearchState & dest ) const
	{ 
		return this->isValid() && 
			this->statesAreEquivalent( dest, *this->pDest() );
	}


	/**
	 *	Return whether the given current state is equivalent to our stored
	 *	current state.
	 */
	bool isCurrentState( const SearchState & current ) const
	{
		return this->isValid() &&
			this->statesAreEquivalent( current, *this->pCurrent() );
	}

	/**
	 *	Returns the current reverse path
	 */
	const BW::vector< SearchState > & reversePath( ) const
		{ return reversePath_; }

private:

	/**
	 *	Return whether the given current state is equivalent to the next state
	 *	in the path.
	 */
	bool isNextState( const SearchState & current ) const
	{
		return this->isValid() &&
			this->statesAreEquivalent( current, *this->pNext() );
	}

protected:
	/**
	 *	Return whether the given state is part of the full path
	 */
	virtual bool isPartOfPath( const SearchState & current ) const
		{ return false; }

	// We use a reverse path (such that the destination is at the front, and
	// the current state is at the back) so that we can use BW::vector
	// pop_back() for pop().
	typedef BW::vector< SearchState > Path;
	Path reversePath_;

};


/**
 *	Return true if the given search states correspond to what is stored in this
 *	search path. The current node is advanced if the given source node is
 *	equivalent to the path's next node.
 */
template< class SearchState, class SearchData >
bool SearchPathBase< SearchState, SearchData >::matches(
		const SearchState & src, const SearchState & dst )
{
	// The criteria for this is that the destinations must be equivalent, and
	// the given source state must be equivalent to either the path's current
	// state or the path's next state.

	if (!this->isDestinationState( dst ))
	{
		// Destination has changed.
		return false;
	}

	// OK, this path leads to the right place.
	// Now make sure it starts from the right place too.
	if (this->isNextState( src ))
	{
		// Advance up the path.
		this->pop();

		return true;
	}
	else if (this->isCurrentState( src ))
	{
		// Looks good.
		return true;
	}
	else if (this->isPartOfPath( src ))
	{
		return true;
	}

	// nope, no good. clear it now just for sanity
	return false;
}

BW_END_NAMESPACE

#endif // SEARCH_PATH_BASE_HPP
