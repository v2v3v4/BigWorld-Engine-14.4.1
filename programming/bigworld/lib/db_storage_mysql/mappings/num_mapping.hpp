#ifndef MYSQL_NUM_MAPPING_HPP
#define MYSQL_NUM_MAPPING_HPP

#include "property_mapping.hpp"

#include "../namer.hpp"
#include "../query.hpp"
#include "../string_conv.hpp"
#include "../table.hpp"

#include "cstdmf/binary_stream.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

template <class STRM_NUM_TYPE>
class NumMapping : public PropertyMapping
{
public:
	NumMapping( const BW::string & propName,
			DataSectionPtr pDefaultValue, 
			ColumnIndexType indexType = INDEX_TYPE_NONE ) :
		PropertyMapping( propName, indexType ),
		colName_( propName ),
		defaultValue_( 0 )
	{
		if (pDefaultValue)
		{
			defaultValue_ = pDefaultValue->as<STRM_NUM_TYPE>();
		}
	}

	NumMapping( const Namer & namer, const BW::string & propName,
			DataSectionPtr pDefaultValue,
			ColumnIndexType indexType = INDEX_TYPE_NONE ) :
		PropertyMapping( propName, indexType ),
		colName_( namer.buildColumnName( "sm", propName ) ),
		defaultValue_( 0 )
	{
		if (pDefaultValue)
		{
			defaultValue_ = pDefaultValue->as<STRM_NUM_TYPE>();
		}
	}

	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
	{
		STRM_NUM_TYPE i;
		strm >> i;
		queryRunner.pushArg( i );
	}

	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const
	{
		STRM_NUM_TYPE i = 0;
		results >> i;
		strm << i;
	}

	virtual void defaultToStream( BinaryOStream & strm ) const
	{
		strm << defaultValue_;
	}

	virtual bool visitParentColumns( ColumnVisitor & visitor )
	{
		ColumnType type(
				MySqlTypeTraits<STRM_NUM_TYPE>::colType,
				!std::numeric_limits<STRM_NUM_TYPE>::is_signed,
				0,
				StringConv::toStr( defaultValue_ ) );

		ColumnDescription columnDescription( colName_, type, 
			this->indexType() );

		return visitor.onVisitColumn( columnDescription );
	}

private:
	BW::string colName_;
	STRM_NUM_TYPE defaultValue_;
};

BW_END_NAMESPACE

#endif // MYSQL_NUM_MAPPING_HPP
