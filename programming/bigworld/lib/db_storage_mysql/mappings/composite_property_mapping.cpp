#include "script/first_include.hpp"

#include "composite_property_mapping.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
CompositePropertyMapping::CompositePropertyMapping(
		const BW::string & propName ) :
		PropertyMapping( propName )
{
}


/**
 *	This method adds a child property to the Composite Map.
 */
void CompositePropertyMapping::addChild( PropertyMappingPtr child )
{
	if (!child)
	{
		ERROR_MSG( "CompositePropertyMapping::addChild: "
				"child is null (ignoring)\n" );
		return;
	}

	children_.push_back( child );
}


/**
 *	This method returns the number of child properties that are mapped into
 *	the composite mapping.
 */
int CompositePropertyMapping::getNumChildren() const
{
	return int(children_.size());
}


/*
 *	Override from PropertyMapping.
 */
void CompositePropertyMapping::prepareSQL()
{
	for (Children::iterator ppChild = children_.begin();
			ppChild != children_.end(); ++ppChild)
	{
		(**ppChild).prepareSQL();
	}
}


/*
 *	Override from PropertyMapping.
 */
void CompositePropertyMapping::fromStreamToDatabase(
			StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
{
	Children::const_iterator iter = children_.begin();

	while (iter != children_.end())
	{
		(*iter)->fromStreamToDatabase( helper, strm, queryRunner );
		if (strm.error())
		{
			ERROR_MSG( "CompositePropertyMapping::fromStreamToDatabase: "
				"Error encountered while de-streaming property '%s'.\n",
					(*iter)->propName().c_str() );
		}

		++iter;
	}
}


/*
 *	Override from PropertyMapping.
 */
void CompositePropertyMapping::fromDatabaseToStream(
			ResultToStreamHelper & helper,
			ResultStream & results,
			BinaryOStream & strm ) const
{
	Children::const_iterator iter = children_.begin();

	while (iter != children_.end())
	{
		(*iter)->fromDatabaseToStream( helper, results, strm );

		++iter;
	}
}


/*
 *	Override from PropertyMapping.
 */
void CompositePropertyMapping::defaultToStream( BinaryOStream & strm ) const
{
	// Assume that data is stored on the stream in the same order as
	// the bindings are created in PyUserTypeBinder::bind().
	for (Children::const_iterator ppChild = children_.begin();
			ppChild != children_.end(); ++ppChild)
	{
		(**ppChild).defaultToStream( strm );
	}
}


/*
 *	Override from PropertyMapping.
 */
bool CompositePropertyMapping::hasTable() const
{
	for (Children::const_iterator ppChild = children_.begin();
			ppChild != children_.end(); ++ppChild)
	{
		if ((*ppChild)->hasTable())
		{
			return true;
		}
	}
	return false;
}


/*
 *	Override from PropertyMapping.
 */
void CompositePropertyMapping::deleteChildren( MySql & connection,
		DatabaseID databaseID ) const
{
	for (Children::const_iterator ppChild = children_.begin();
			ppChild != children_.end(); ++ppChild)
	{
		(**ppChild).deleteChildren( connection, databaseID );
	}
}


/*
 *	Override from PropertyMapping.
 */
bool CompositePropertyMapping::visitParentColumns( ColumnVisitor & visitor )
{
	for (Children::const_iterator ppChild = children_.begin();
			ppChild != children_.end(); ++ppChild)
	{
		if (!(**ppChild).visitParentColumns( visitor ))
		{
			return false;
		}
	}
	return true;
}


/*
 *	Override from PropertyMapping.
 */
bool CompositePropertyMapping::visitTables( TableVisitor & visitor )
{
	for (Children::const_iterator ppChild = children_.begin();
			ppChild != children_.end(); ++ppChild)
	{
		if (!(**ppChild).visitTables( visitor ))
		{
			return false;
		}
	}

	return true;
}

BW_END_NAMESPACE

// composite_property_mapping.cpp
