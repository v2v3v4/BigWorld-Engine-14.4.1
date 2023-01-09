#ifndef CELL_APP_GROUPS_HPP
#define CELL_APP_GROUPS_HPP

#include "cell_app_group.hpp"
#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

class CellApps;

/**
 *	This class is a collection of all the CellApp::Group objects.
 */
class CellAppGroups
{
public:
	CellAppGroups( const CellApps & cellApps );

	void checkForOverloaded( float addCellThreshold );
	void checkForUnderloaded( float retireCellThreshold );

	BW::string asString() const;

private:
	typedef BW::list< CellAppGroup > List;
	List list_;
};

BW_END_NAMESPACE

#endif // CELL_APP_GROUPS_HPP
