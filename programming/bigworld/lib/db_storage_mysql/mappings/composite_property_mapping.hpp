#ifndef MYSQL_COMPOSITE_PROPERTY_MAPPING_HPP
#define MYSQL_COMPOSITE_PROPERTY_MAPPING_HPP

#include "property_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class handles mapping multiple property types into a single collection
 *	such as is required for USER_DATA types.
 */
class CompositePropertyMapping : public PropertyMapping
{
public:
	CompositePropertyMapping( const BW::string & propName );

	void addChild( PropertyMappingPtr child );

	int getNumChildren() const;

	// Overrides from PropertyMapping
	virtual void prepareSQL();

	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const;


	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const;

	virtual void defaultToStream( BinaryOStream & strm ) const;

	virtual bool hasTable() const;

	virtual void deleteChildren( MySql & connection,
				DatabaseID databaseID ) const;

	virtual bool visitParentColumns( ColumnVisitor & visitor );

	virtual bool visitTables( TableVisitor & visitor );

	typedef BW::vector< PropertyMappingPtr > Children;

protected:
	Children children_;
};

typedef SmartPointer<CompositePropertyMapping> CompositePropertyMappingPtr;

BW_END_NAMESPACE

#endif // MYSQL_COMPOSITE_PROPERTY_MAPPING_HPP
