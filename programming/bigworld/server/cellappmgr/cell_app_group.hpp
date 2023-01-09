#ifndef CELL_APP_GROUP_HPP
#define CELL_APP_GROUP_HPP

#include "cstdmf/bw_namespace.hpp"
#include "server/common.hpp"

#include <cstddef>
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class CellApp;
class SpaceChooser;
class SpaceVisitor;
class Space;

/**
 *	This class is used to represent a group of CellApps that are in a
 *	meta-balance group. This is, a set of CellApps that can balance their
 *	load through the normal load balancing. For this to occur, there must
 *	be multi-cell Spaces that cover the CellApps.
 */
class CellAppGroup
{
public:
	CellAppGroup() : avgLoad_( -1.f ) {}
	~CellAppGroup();

	void addCell();
	void checkForUnderloaded( float loadLowerBound );

	void insert( CellApp * );
	void join( CellAppGroup * pMainGroup );

	size_t size() const				{ return set_.size(); }

	float avgLoad( int ifNumRemoved = 0 ) const;

	BW::string asString() const;

private:
	bool cancelRetiringCellApp();

	typedef BW::set< CellApp * > Set;

	typedef Set::iterator iterator;
	typedef Set::const_iterator const_iterator;

	const_iterator begin() const	{ return set_.begin(); }
	iterator begin()				{ return set_.begin(); }

	const_iterator end() const		{ return set_.end(); }
	iterator end()					{ return set_.end(); }

	bool isEmpty() const			{ return set_.empty(); }

	Space * chooseConnectionSpace() const;
	Space * chooseConnectionSpace( SpaceChooser & chooser ) const;
	void visitSpaces( SpaceVisitor & visitor ) const;

	void updateCellAppsPerIP();

	typedef BW::map< uint32, int > CellAppsPerIPMap;
	CellAppsPerIPMap cellAppsPerIP_;

	float avgLoad_;
	Set set_;
};

BW_END_NAMESPACE

#endif // CELL_APP_GROUP_HPP
