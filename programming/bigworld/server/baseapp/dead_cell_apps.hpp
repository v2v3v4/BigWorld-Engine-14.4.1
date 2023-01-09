#ifndef DEAD_CELL_APPS_HPP
#define DEAD_CELL_APPS_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/shared_ptr.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class Bases;
class BinaryIStream;
class DeadCellApp;
class DyingCellApp;

namespace Mercury
{
class Address;
class NetworkInterface;
}


/**
 *	This class handles identifying CellApps that have recently died.
 */
class DeadCellApps
{
public:
	bool isRecentlyDead( const Mercury::Address & addr ) const;
	void addApp( const Mercury::Address & addr, BinaryIStream & data );

	void tick( const Bases & bases, Mercury::NetworkInterface & intInterface );

private:
	void removeOldApps();

	typedef BW::vector< shared_ptr< DeadCellApp > > Container;
	Container apps_;

	typedef BW::list< shared_ptr< DyingCellApp > > DyingCellApps;
	DyingCellApps dyingApps_;
};

BW_END_NAMESPACE

#endif // DEAD_CELL_APPS_HPP
