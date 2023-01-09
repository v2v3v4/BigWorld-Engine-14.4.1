#include "cell_app_groups.hpp"

#include "cellapps.hpp"
#include "space.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellAppGroups
// -----------------------------------------------------------------------------

/**
 *	Constructor. The input CellApps are partitioned into a list of Group
 *	objects. Each Group is the set of CellApps that can balance their load via
 *	normal load balancing.
 */
CellAppGroups::CellAppGroups( const CellApps & cellApps )
{
	cellApps.identifyGroups( list_ );
}


namespace
{

/**
 *	This class is used to collect the overloaded groups.
 */
class OverloadedGroups
{
public:
	void add( CellAppGroup & group )
	{
		map_.insert( std::make_pair( group.avgLoad(), &group ) );
	}

	void addCells()
	{
		// Iterate over in reverse order so that the most loaded group is merged
		// first.
		Map::reverse_iterator iter = map_.rbegin();

		while (iter != map_.rend())
		{
			CellAppGroup * pGroup = iter->second;

			pGroup->addCell();

			++iter;
		}
	}

private:
	typedef std::multimap< float, CellAppGroup * > Map;
	Map map_;
};

}


/**
 *	This method checks whether there are any overloaded CellApp groups and adds
 *	cells, if necessary, to help balance the load.
 */
void CellAppGroups::checkForOverloaded( float addCellThreshold )
{
	OverloadedGroups overloadedGroups;

	List::iterator iter = list_.begin();

	while (iter != list_.end())
	{
		CellAppGroup & group = *iter;

		if (group.avgLoad() > addCellThreshold)
		{
			overloadedGroups.add( group );
		}

		++iter;
	}

	overloadedGroups.addCells();
}


/**
 *	This method checks whether any current groups are underloaded. If so, it
 *	will remove the cells from one of the CellApps.
 */
void CellAppGroups::checkForUnderloaded( float retireCellThreshold )
{
	List::iterator iter = list_.begin();

	while (iter != list_.end())
	{
		iter->checkForUnderloaded( retireCellThreshold );

		++iter;
	}
}


/**
 *	This method generates a string representation of the set of CellApp groups
 *	with more than one CellApp.
 */
BW::string CellAppGroups::asString() const
{
	BW::stringstream str;

	List::const_iterator iter = list_.begin();

	str << '[';

	bool hasGroupBeenAdded = false;

	while (iter != list_.end())
	{
		const CellAppGroup & group = *iter;

		if (group.size() > 1)
		{
			if (hasGroupBeenAdded)
			{
				str << ',';
			}

			str << group.asString();

			hasGroupBeenAdded = true;
		}

		++iter;
	}

	str << ']';

	return str.str();
}


BW_END_NAMESPACE

// cell_app_groups.cpp
