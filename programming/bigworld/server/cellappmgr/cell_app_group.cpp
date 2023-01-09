#include "cell_app_group.hpp"

#include "cellapp.hpp"
#include "cellappmgr.hpp"
#include "cellappmgr_config.hpp"
#include "space.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: SpaceVisitor
// -----------------------------------------------------------------------------

/**
 *	This class is an interface used to visit all spaces in a collection.
 */
class SpaceVisitor
{
public:
	virtual ~SpaceVisitor() {}

	/**
	 *	This method will be called for each space in a collection.
	 */
	virtual void onSpace( const Space & space ) = 0;
};


// -----------------------------------------------------------------------------
// Section: SpaceChooser
// -----------------------------------------------------------------------------

/**
 *	This class is an interface used to choose a space from a collection.
 */
class SpaceChooser : public SpaceVisitor
{
public:
	SpaceChooser() { pSpace_ = NULL; }
	const Space * pSpace() { return pSpace_; }

protected:
	const Space * pSpace_;
};


// -----------------------------------------------------------------------------
// Section: LargestSpaceChooser
// -----------------------------------------------------------------------------

/**
 *	This class chooses the largest (most cells) space in a collection.
 */
class LargestSpaceChooser : public SpaceChooser
{
public:
	LargestSpaceChooser();
	virtual void onSpace( const Space & space );

private:
	int maxCells_;
};


/**
 *	Constructor.
 */
LargestSpaceChooser::LargestSpaceChooser() :
	SpaceChooser(),
	maxCells_( 0 )
{ }


/**
 *	This method is visited every time a Space that needs to be evaluated
 *	for selection.
 */
void LargestSpaceChooser::onSpace( const Space & space )
{
	if (space.numCells() > maxCells_)
	{
		pSpace_ = &space;
		maxCells_ = space.numCells();
	}
}


// -----------------------------------------------------------------------------
// Section: SmallestSpaceChooser
// -----------------------------------------------------------------------------

/**
 *	This class chooses the smallest (fewest cells) space in a collection.
 */
class SmallestSpaceChooser: public SpaceChooser
{
public:
	SmallestSpaceChooser();
	virtual void onSpace( const Space & space );

private:
	int minCells_;
};


/**
 *	Constructor.
 */
SmallestSpaceChooser::SmallestSpaceChooser() :
	SpaceChooser(),
	minCells_( INT_MAX )
{ }


/**
 *	This method is visited every time a Space that needs to be evaluated
 *	for selection.
 */
void SmallestSpaceChooser::onSpace( const Space & space )
{
	if (space.numCells() < minCells_)
	{
		pSpace_ = &space;
		minCells_ = space.numCells();
	}
}


// -----------------------------------------------------------------------------
// Section: HybridSpaceChooser
// -----------------------------------------------------------------------------

/**
 *	This class chooses the best space in a collection to introduce a new cell
 *	to.
 */
class HybridSpaceChooser: public SpaceChooser
{
public:
	HybridSpaceChooser();
	virtual void onSpace( const Space & space );

	bool hasLargestSpace() const { return minCells_ == 0; }

private:
	int minCells_;
	float maxLoad_;
};


/**
 *	Constructor.
 */
HybridSpaceChooser::HybridSpaceChooser() :
	SpaceChooser(),
	minCells_( INT_MAX ),
	maxLoad_( FLT_MAX )
{
}


/**
 *	This method is visited every time a Space that needs to be evaluated
 *	for selection.
 */
void HybridSpaceChooser::onSpace( const Space & space )
{
	float currentLoad = space.avgLoad();

	if (space.isLarge())
	{
		if ((currentLoad > maxLoad_) || !this->hasLargestSpace())
		{
			pSpace_ = &space;
			maxLoad_ = currentLoad;

			// No longer look for min cells.
			minCells_ = 0;
		}
	}

	if ((space.numCells() < minCells_) ||
			((space.numCells() == minCells_) && (currentLoad > maxLoad_)))
	{
		pSpace_ = &space;
		minCells_ = space.numCells();
		maxLoad_ = currentLoad;
	}
}


// -----------------------------------------------------------------------------
// Section: CellAppGroup
// -----------------------------------------------------------------------------

/**
 *	Destructor.
 */
CellAppGroup::~CellAppGroup()
{
	iterator iter = this->begin();

	while (iter != this->end())
	{
		CellApp * pApp = *iter;

		MF_ASSERT( pApp->pGroup() == this );

		pApp->pGroup_ = NULL;

		++iter;
	}
}


/**
 *	This method visits all spaces that belong in this group. Note that spaces
 *	can be visited more than once.
 */
void CellAppGroup::visitSpaces( SpaceVisitor & visitor ) const
{
	const_iterator appIter = this->begin();

	while (appIter != this->end())
	{
		CellApp * pApp = *appIter;

		Cells::const_iterator cellIter = pApp->cells().begin();

		while (cellIter != pApp->cells().end())
		{
			visitor.onSpace( (*cellIter)->space() );

			++cellIter;
		}

		++appIter;
	}
}


/**
 *	This method chooses a space in this group that would be good to use to
 *	connect to another group.
 */
Space * CellAppGroup::chooseConnectionSpace( SpaceChooser & chooser ) const
{
	this->visitSpaces( chooser );

	const Space * pSpace = chooser.pSpace();

	if (!pSpace)
	{
		ERROR_MSG( "CellAppGroup::chooseConnectionSpace: "
				"No appropriate space in group\n" );
		return NULL;
	}

	return const_cast< Space *>( pSpace );
}


/**
 *	This method chooses a space in this group that would be good to use to
 *	connect to another group.
 */
Space * CellAppGroup::chooseConnectionSpace() const
{
	CellAppMgrConfig::MetaLoadBalanceScheme scheme =
		static_cast< CellAppMgrConfig::MetaLoadBalanceScheme >(
			CellAppMgrConfig::metaLoadBalanceScheme() );

	switch (scheme)
	{
		case CellAppMgrConfig::SCHEME_LARGEST:
		{
			LargestSpaceChooser chooser;
			return this->chooseConnectionSpace( chooser );
		}
		break;

		case CellAppMgrConfig::SCHEME_SMALLEST:
		{
			SmallestSpaceChooser chooser;
			return this->chooseConnectionSpace( chooser );
		}
		break;

		case CellAppMgrConfig::SCHEME_HYBRID:
		{
			HybridSpaceChooser chooser;
			return this->chooseConnectionSpace( chooser );
		}
		break;

		default:
			ERROR_MSG( "CellAppGroup::chooseConnectionSpace: "
					"Invalid scheme %d. Switching.\n", scheme );
			CellAppMgrConfig::metaLoadBalanceScheme.set(
					CellAppMgrConfig::SCHEME_HYBRID );
		break;
	}

	return NULL;
}


/**
 *	This method stops a CellApp in this group from retiring its Cells.
 *
 *	@return True if there is a CellApp with retiring cells that was stopped,
 *	otherwise false.
 */
bool CellAppGroup::cancelRetiringCellApp()
{
	iterator iter = this->begin();

	while (iter != this->end())
	{
		CellApp * pApp = *iter;

		if (!pApp->isRetiring() && pApp->hasAnyRetiringCells())
		{
			pApp->cancelRetiringCells();
			return true;
		}

		++iter;
	}

	return false;
}


/**
 *	This method returns the average load over all CellApps in this group. For
 *	CellApps that are retiring, their load is included but they are not included
 *	in the count.
 *
 *	@param ifNumRemoved	If not 0, the expected average load if this many
 *	CellApps were removed.
 *
 *	@return The average load for this group.
 */
float CellAppGroup::avgLoad( int ifNumRemoved ) const
{
	int count = -ifNumRemoved;
	float totalLoad = 0.f;

	CellAppGroup::iterator iter = this->begin();

	while (iter != this->end())
	{
		CellApp * pApp = *iter;
		totalLoad += pApp->smoothedLoad();

		if (!pApp->hasOnlyRetiringCells())
		{
			++count;
		}

		++iter;
	}

	return (count > 0) ? totalLoad/count : FLT_MAX;
}


/**
 *	This method generates a string representation of the group.
 */
BW::string CellAppGroup::asString() const
{
	BW::stringstream str;

	Set::const_iterator iter = set_.begin();

	str << '(';

	while (iter != set_.end())
	{
		if (iter != set_.begin())
		{
			str << ',';
		}

		str << (*iter)->id();

		++iter;
	}

	str << ')';

	return str.str();
}


/**
 *	This method fills a mapping of IP addresses to the number of CellApps in
 *	this group that reside on that address.
 */
void CellAppGroup::updateCellAppsPerIP()
{
	cellAppsPerIP_.clear();

	const_iterator iter = this->begin();

	while (iter != this->end())
	{
		++cellAppsPerIP_[ (*iter)->addr().ip ];
		++iter;
	}
}


/**
 *	This methods inserts a CellApp into this group. It also takes care of
 *	ensuring that the CellApp knows which group it is in.
 *
 *	@param	pApp	The CellApp to add to this group.
 */
void CellAppGroup::insert( CellApp * pApp )
{
	MF_ASSERT( pApp->pGroup() == NULL );

	set_.insert( pApp );
	pApp->pGroup_ = this;

	++cellAppsPerIP_[ pApp->addr().ip ];
}


/**
 *	This method causes this group to join the input group. At the end of this
 *	method, this group will be empty and all members will be in the input group.
 *
 *	@param pMainGroup The group that this group will be joined to.
 */
void CellAppGroup::join( CellAppGroup * pMainGroup )
{
	if (pMainGroup)
	{
		pMainGroup->set_.insert( set_.begin(), set_.end() );

		iterator iter = this->begin();

		while (iter != this->end())
		{
			(*iter)->pGroup_ = pMainGroup;

			++iter;
		}

		set_.clear();
		pMainGroup->updateCellAppsPerIP();
	}
}


/**
 *	This method attempts to add a cell to this group to help spread the group's
 *	load.
 */
void CellAppGroup::addCell()
{
	if (this->isEmpty())
	{
		// This occurs if this group was already merged with another.
		return;
	}

	if (!this->cancelRetiringCellApp())
	{
		// Which space from this group should have a Cell added?
		Space * pSpace = this->chooseConnectionSpace();

		if (pSpace)
		{
			pSpace->addCell();
		}
	}
}


/**
 *	This method checks whether this group is underloaded and removes a CellApp
 *	if necessary.
 */
void CellAppGroup::checkForUnderloaded( float loadLowerBound )
{
	// If the expected average load with one less CellApp is lower than the
	// input threshold, the least loaded CellApp is retired from the group.

	if (this->avgLoad( 1 ) < loadLowerBound)
	{
		CellApp * pLeastLoaded = NULL;
		float leastLoad = FLT_MAX;

		iterator iter = this->begin();

		while (iter != this->end())
		{
			CellApp * pApp = *iter;

			if ((!pApp->hasOnlyRetiringCells() &&
				pApp->smoothedLoad() < leastLoad))
			{
				pLeastLoaded = *iter;
				leastLoad = pLeastLoaded->smoothedLoad();
			}

			++iter;
		}

		if (pLeastLoaded)
		{
			pLeastLoaded->retireAllCells();
		}
	}
}

BW_END_NAMESPACE

// cell_app_group.cpp
