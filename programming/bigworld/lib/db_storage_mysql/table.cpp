#include "table.hpp"


BW_BEGIN_NAMESPACE

namespace
{

class HasSubTablesVisitor : public TableVisitor
{
public:
	virtual bool onVisitTable( TableProvider& table )
	{
		return false;
	}
};

} // anonymous namespace


bool TableProvider::hasSubTables()
{
	HasSubTablesVisitor visitor;

	return !this->visitSubTablesWith( visitor );
}


namespace
{

/**
 *	This helper function visits the table and all it's sub-tables using the
 * 	provided visitor i.e. visitor.onVisitTable() will be called for
 * 	this table and all it's sub-tables.
 */
class RecursiveVisitor : public TableVisitor
{
public:
	RecursiveVisitor( TableVisitor & origVisitor ) :
		origVisitor_( origVisitor )
	{ }

	virtual bool onVisitTable( TableProvider& table )
	{
		return origVisitor_.onVisitTable( table ) &&
				table.visitSubTablesWith( *this );
	}

private:
	TableVisitor & origVisitor_;
};

} // anonymous namespace


/**
 *	This method visits this table and all sub-tables recursively.
 */
bool TableProvider::visitSubTablesRecursively( TableVisitor & visitor )
{
	RecursiveVisitor proxyVisitor( visitor );
	return proxyVisitor.onVisitTable( *this );
}

BW_END_NAMESPACE


// table.cpp
