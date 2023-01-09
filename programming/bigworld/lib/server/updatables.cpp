#include "updatables.hpp"
#include "updatable.hpp"

#include "cstdmf/debug.hpp"

#include <cstddef>


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
Updatables::Updatables( int numLevels ) :
	objects_(),
	levelSize_( numLevels ),
	inUpdate_( false ),
	deletedObjects_( 0 )
{
}


/**
 *	This method adds an Updatable object to this collection.
 */
bool Updatables::add( Updatable * pObject, int level )
{
	if ((level < 0) || (int(levelSize_.size()) <= level))
	{
		ERROR_MSG( "Updatables::add: Invalid level %d\n", level );
		return false;
	}

	// Error checking only
	if (pObject->removalHandle_ != -1)
	{
		int handle = pObject->removalHandle_;
		if (0 <= handle && uint(handle) < objects_.size() &&
				objects_[handle] == pObject)
		{
			ERROR_MSG( "Updatables::add: Already added.\n" );
		}
		else
		{
			CRITICAL_MSG( "Updatables::add: "
					"Updatable object in invalid state.\n" );
		}

		return false;
	}

	// Initially, the Updatable is placed at the end of the vector, not in any
	// valid level. The desired level is stored in removalHandle_.

	// In adjustForAddedObject, the Updatable is then placed in the appropriate
	// level.

	// Initially, a hole is made at the end of the vector. The first element of
	// the level is moved into the hole at the back of the level. This creates a
	// hole in a lower level. This is repeated until the correct level is
	// arrived at.

	pObject->removalHandle_ = -1 - level;
	objects_.push_back( pObject );

	if (!inUpdate_)
	{
		this->adjustForAddedObject();
	}

	return true;
}


/**
 *	This method adjusts this collection to remove any holes and correct the size
 *	of the levels.
 */
void Updatables::adjustForAddedObject()
{
	MF_ASSERT( levelSize_.back() < (int)objects_.size() );

	Updatable * pObject = objects_[ levelSize_.back() ];
	MF_ASSERT( pObject->removalHandle_ < 0 );
	int level = -1 - pObject->removalHandle_;
	int hole = levelSize_.back();

	int currLevel = levelSize_.size() - 1;

	// Each element of levelSize_ stores the number of Updatables at
	// that level or lower. It is like an end pointer for that level.
	++levelSize_[ currLevel ];

	// Keep moving the hole to a lower level until the right one is found.
	while (--currLevel >= level)
	{
		int levelFront = levelSize_[ currLevel ];
		++levelSize_[ currLevel ];

		if (levelFront < hole)
		{
			// Move the front to the hole (the back). The hole is now at the
			// back of the lower level.
			Updatable * pTemp = objects_[ levelFront ];
			objects_[ hole ] = pTemp;
			if (pTemp != NULL)
				pTemp->removalHandle_ = hole;
		}
		else
		{
			MF_ASSERT( levelFront == hole );
		}
		hole = levelFront;
	}

	objects_[ hole ] = pObject;
	pObject->removalHandle_ = hole;
}


/**
 *	This method removes an Updatable from this collection.
 */
bool Updatables::remove( Updatable * pObject )
{
	/* DEBUG_MSG( "Updatables::remove: "
			"inUpdate_ = %d. objects_[ %d ] = %p\n",
		inUpdate_, pObject->removalHandle_, pObject ); */

	int hole = pObject->removalHandle_;
	if (0 <= hole && uint(hole) < objects_.size() &&
			objects_[hole] == pObject)
	{
		pObject->removalHandle_ = -1;

		if (inUpdate_)
		{
			objects_[ hole ] = NULL;
			++deletedObjects_;
		}
		else
		{
			// This line is for debugging.
			objects_[ hole ] = NULL;

			// Go through all of the levels and move the hole to a higher level,
			// if necessary.
			for (uint level = 0; level < levelSize_.size(); ++level)
			{
				int levelBack = levelSize_[ level ] - 1;
				if (hole <= levelBack)
				{
					MF_ASSERT( objects_[ hole ] == NULL );
					// Move it to the back of the current level and then
					// consider it to be the front of the next level.
					--levelSize_[ level ];
					objects_[ hole ] = objects_[ levelBack ];

					if (objects_[ hole ] != NULL)
					{
						objects_[ hole ]->removalHandle_ = hole;
					}

					hole = levelBack;

					// For debugging
					objects_[ hole ] = NULL;
				}
			}

			MF_ASSERT( objects_.back() == NULL );
			objects_.pop_back();
			MF_ASSERT( levelSize_.back() == int(objects_.size()) );
		}
	}
	else
	{
		int handle = pObject->removalHandle_;
		if (handle < 0)
		{
			for (unsigned int i = levelSize_.back();
					i < objects_.size();
					++i)
			{
				if (objects_[i] == pObject)
				{
					objects_.erase( objects_.begin() + i );
					return true;
				}
			}
		}

		WARNING_MSG( "Updatables::remove: Updatable was not fully added\n" );
		return false;
	}

	return true;
}


/**
 *	This method calls the update method of each object in the collection.
 */
void Updatables::call()
{
	inUpdate_ = true;
	unsigned int size = objects_.size();

	for (unsigned int i = 0; i < size; i++)
	{
		// We go by the initial size of the array, not the current size.
		// This means that if new objects are added to it while we are
		// iterating, they will not be processed until next tick. This
		// helps prevent infinite loops.
		MF_ASSERT( objects_.size() >= size );

		if (objects_[i])
		{
			MF_ASSERT( objects_[i]->removalHandle_ == int(i) );
			objects_[i]->update();
			MF_ASSERT( size <= objects_.size() );
		}
	}

	inUpdate_ = false;

	// Adjust any objects newly registered while inUpdate_ was true.
	const int numAdded = objects_.size() - size;
	for (int i = 0; i < numAdded; ++i)
	{
		this->adjustForAddedObject();
	}

	MF_ASSERT( levelSize_.back() == (int)objects_.size() );

	if (deletedObjects_ > 0)
	{
		int currIdx = 0;
		int currLevel = 0;

		// The objects_ vector is traversed to find all of the holes.
		// For each level, the holes are moved to the end of the level which is
		// then considered as the start of the next level.

		while ((currLevel < int(levelSize_.size()) - 1) ||
				(currIdx < levelSize_.back()))
		{
			while (currIdx >= levelSize_[ currLevel ])
			{
				++currLevel;
			}

			if (objects_[ currIdx ] == NULL)
			{
				--levelSize_[ currLevel ];
				Updatable * pEnd =
					objects_[ levelSize_[ currLevel ] ];
				objects_[ levelSize_[ currLevel ] ] = NULL;
				if (pEnd)
				{
					pEnd->removalHandle_ = currIdx;
					objects_[ currIdx ] = pEnd;
					// DEBUG_MSG( "Updatables::call: "
					//		"objects_[ %d ] = 0x%08x\n",
					//	currIdx, pEnd );
				}
				else
				{
				}
			}
			else
			{
				++currIdx;
			}
		}

		MF_ASSERT( int(objects_.size()) - deletedObjects_ ==
				levelSize_.back() );
		objects_.resize( levelSize_.back() );
	}

	deletedObjects_ = 0;
}

BW_END_NAMESPACE

// updatables.cpp
