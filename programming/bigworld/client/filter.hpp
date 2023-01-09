#ifndef FILTER_HPP
#define FILTER_HPP

#include "py_filter.hpp"

#include "connection/movement_filter.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

typedef WeakPyPtr< PyFilter > WeakPyFilterPtr;

/**
 *	This class forms the base of all filters in the BigWorld client.
 *
 *	Filters are components attached to entities to process updates to the
 *	entity's 'volatile'  members. These include position, yaw, pitch and roll.
 *	Volatile members are different to other entity members in that their
 *	transmission to clients is 'best effort' as apposed to the 'reliable'
 *	fashion most members are updated. In addition to this the frequency of
 *	updates is affected with distance to the player. It is the responsibility
 *	of filters to take these inputs and produce a visually pleasing movement of
 *	the entity.

 *	@note:	The network layer will sometimes provided positions that are
 *	'on ground' and will need to have their \f$y\f$ coordinate sampled from the
 *	terrain at the given \f$(x,z)\f$. They will appear as positions with a
 *	\f$y\f$ component approximately equal to -13000.
 */
class Filter : public MovementFilter
{
public:
	Filter( PyFilter * pOwner );
	virtual ~Filter();

protected:
	const PyFilter * pOwner() const { return wpOwner_.get(); }

private:
	// Disable copy-constructor and copy-assignment
	Filter( const Filter & other );
	Filter & operator=( const Filter & other );

	WeakPyFilterPtr wpOwner_;
};

BW_END_NAMESPACE

#endif // FILTER_HPP
