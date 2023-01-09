#include "cellapp_death_listener.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellAppDeathListeners
// -----------------------------------------------------------------------------

CellAppDeathListeners * CellAppDeathListeners::s_pInstance_ = NULL;

/**
 *  Static instance accessor.
 */
CellAppDeathListeners & CellAppDeathListeners::instance()
{
	if (s_pInstance_ == NULL)
	{
		s_pInstance_ = new CellAppDeathListeners();
	}

	return *s_pInstance_;
}


/**
 *  This method is called on CellApp death and notifies all listeners of the
 *  tragedy.
 */
void CellAppDeathListeners::handleCellAppDeath( const Mercury::Address & addr )
{
	// Be careful with iteration as any of these objects could be deleted and
	// disappear from the list at any time.
	iterator iter = listeners_.begin();

	while (iter != listeners_.end())
	{
		CellAppDeathListener & listener = **iter; ++iter;
		listener.handleCellAppDeath( addr );
	}
}


/**
 *  This method adds a listener to the collection and returns an iterator to it.
 */
CellAppDeathListeners::iterator CellAppDeathListeners::addListener(
	CellAppDeathListener * pListener )
{
	return listeners_.insert( listeners_.end(), pListener );
}


/**
 *  This method removes a listener from the collection.
 */
void CellAppDeathListeners::delListener( iterator & iter )
{
	listeners_.erase( iter );
}


// -----------------------------------------------------------------------------
// Section: CellAppDeathListener
// -----------------------------------------------------------------------------

/**
 *  Constructor.  Just adds this object to the static collection, and remembers
 *  where it is.
 */
CellAppDeathListener::CellAppDeathListener() :
	iter_( CellAppDeathListeners::instance().addListener( this ) )
{}


/**
 *  Destructor.  Removes this object from the static collection.
 */
CellAppDeathListener::~CellAppDeathListener()
{
	CellAppDeathListeners::instance().delListener( iter_ );
}

BW_END_NAMESPACE

// cellapp_death_listener.cpp

