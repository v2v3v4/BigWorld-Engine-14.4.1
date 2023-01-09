#include "script/first_include.hpp"

#include "sequence_mapping.hpp"

#include "result_to_stream_helper.hpp"
#include "stream_to_query_helper.hpp"

#include "../comma_sep_column_names_builder.hpp"
#include "../namer.hpp"
#include "../query.hpp"
#include "../result_set.hpp"
#include "../utils.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

namespace
{
const int MAX_NUM_ELEMENTS = 100000;
}


/**
 *	Constructor.
 *
 *	@param namer	The object responsible for naming
 *	@param propName	The name of the property this represents
 *	@param pChild	The mapping used for the child elements.
 *	@param size		If not 0, this sequence is always this size. If 0, the size
 *					can vary.
 */
SequenceMapping::SequenceMapping( const Namer & namer,
		const BW::string & propName,
		PropertyMappingPtr pChild,
		int size ) :
	PropertyMapping( propName ),
	tblName_( namer.buildTableName( propName ) ),
	pChild_( pChild ),
	size_( size ),
	childHasTable_( false )
{
}


/**
 *	Destructor.
 */
SequenceMapping::~SequenceMapping()
{
}


/*
 *	Override from PropertyMapping.
 */
void SequenceMapping::prepareSQL()
{
	// NOTE: pChild->createSequenceBuffer() can't be initialised in the
	// constructor because UserTypeMapping doesn't not have its
	// children set up yet.

	childHasTable_ = pChild_->hasTable();

	BW::string stmt;

	CommaSepColNamesBuilder colNamesBuilder( *pChild_ );
	BW::string	childColNames = colNamesBuilder.getResult();
	int			childNumColumns = colNamesBuilder.getCount();

	MF_ASSERT( childHasTable_ || (childNumColumns > 0) );

	stmt = "SELECT ";

	if (childHasTable_)
	{
		stmt += "id";
	}

	if (childNumColumns)
	{
		if (childHasTable_)
		{
			stmt += ",";
		}

		stmt += childColNames;
	}

	stmt += " FROM " + tblName_ + " WHERE parentID=? ORDER BY id";
	selectQuery_.init( stmt );

	stmt = "SELECT id FROM " + tblName_ + " WHERE parentID=? ORDER BY "
			"id FOR UPDATE";
	selectChildrenQuery_.init( stmt );

	stmt = "INSERT INTO " + tblName_ + " (";

	if (childNumColumns)
	{
		stmt += childColNames + ",";
	}

	stmt += "parentID) VALUES (" +
		buildCommaSeparatedQuestionMarks( childNumColumns + 1 ) + ")";
	insertQuery_.init( stmt );

	stmt = "UPDATE " + tblName_ + " SET parentID=?";

	if (childNumColumns)
	{
		CommaSepColNamesBuilderWithSuffix
				updateColNamesBuilder( *pChild_, "=?" );
		BW::string	updateCols = updateColNamesBuilder.getResult();

		stmt += "," + updateCols;
	}

	stmt += " WHERE id=?";
	updateQuery_.init( stmt );

	stmt = "DELETE FROM " + tblName_ + " WHERE parentID=?";
	deleteQuery_.init( stmt );
	stmt += " AND id >= ?";
	deleteExtraQuery_.init( stmt );

	pChild_->prepareSQL();
}


/**
 *	This method gets the number of elements present in the stream.
 *
 *	@param strm  The stream containing the sequence to be read.
 *
 *	@returns The number of elements to be read from the stream.
 */
int SequenceMapping::getNumElemsFromStrm( BinaryIStream & strm ) const
{
	if (this->isFixedSize())
	{
		return this->getFixedSize();
	}

	int numElems = strm.readPackedInt();

	return numElems;
}


/*
 *	Override from PropertyMapping.
 */
void SequenceMapping::fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
{
	int numElems = this->getNumElemsFromStrm( strm );

	if (numElems > MAX_NUM_ELEMENTS)
	{
		ERROR_MSG( "SequenceMapping::fromStreamToDatabase: "
				"Failed to destream property '%s'. Number of sequence elements "
				"reported in the stream exceeds maximum allowed %d > %d.\n",
			this->propName().c_str(), numElems, MAX_NUM_ELEMENTS );

		strm.error( true );

		return;
	}

	MySql & connection = helper.connection();
	const DatabaseID parentID = helper.parentID();

	if (numElems == 0)	// Optimisation for empty arrays
	{
		this->deleteChildren( connection, parentID );
	}
	else
	{
		// TODO: Split this into more methods

		ResultSet resultSet;
		int numRows = 0;

		if (parentID != 0)
		{
			selectChildrenQuery_.execute( connection, parentID, &resultSet );
			numRows = resultSet.numRows();
		}

		int numUpdates = std::min( numRows, numElems );

		// Update existing rows
		for (int i = 0; i < numUpdates; ++i)
		{
			QueryRunner updateQueryRunner( connection, updateQuery_ );

			// If we are updating existing rows we must have a parentID
			MF_ASSERT( parentID != 0 );

			updateQueryRunner.pushArg( parentID );

			DatabaseID childID;
			resultSet.getResult( childID );

			StreamToQueryHelper childHelper( connection, childID );

			pChild_->fromStreamToDatabase( childHelper, strm,
											updateQueryRunner );
			if (strm.error())
			{
				ERROR_MSG( "SequenceMapping::fromStreamToDatabase: "
							"Failed to stream property '%s'.\n",
						pChild_->propName().c_str() );
			}

			MF_ASSERT( !childHelper.hasBufferedQueries() );

			updateQueryRunner.pushArg( childID );

			updateQueryRunner.execute( NULL );
		}

		// Delete any extra rows (i.e. array has shrunk).
		if (numRows > numElems)
		{
			DatabaseID nextID;
			resultSet.getResult( nextID );

			deleteExtraQuery_.execute( connection, parentID, nextID, NULL );

			if (childHasTable_)
			{
				do
				{
					pChild_->deleteChildren( connection, nextID );
				}
				while (resultSet.getResult( nextID ));
			}
		}
		// Insert any extra rows (i.e. array has grown)
		else if ((numElems > numRows) && !strm.error())
		{
			int numToAdd = numElems - numRows;

			// If we already have the parentID, the child table can be written
			// now, otherwise this query is buffered until the parentID is
			// known.
			if (parentID != 0)
			{
				for (int i = 0; i < numToAdd; ++i)
				{
					ChildQuery childQuery( connection, insertQuery_ );

					pChild_->fromStreamToDatabase( childQuery.helper(),
							strm, childQuery.queryRunner() );
					if (strm.error())
					{
						ERROR_MSG( "SequenceMapping::fromStreamToDatabase: "
									"Failed to stream property '%s' to child "
									"query. Parent record id=%" FMT_DBID ".\n",
								pChild_->propName().c_str(), parentID );
					}
					else
					{
						childQuery.execute( parentID );
					}
				}
			}
			else
			{
				for (int i = 0; i < numToAdd; ++i)
				{
					ChildQuery & childQuery =
						helper.createChildQuery( insertQuery_ );

					pChild_->fromStreamToDatabase( childQuery.helper(),
							strm, childQuery.queryRunner() );
					if (strm.error())
					{
						ERROR_MSG( "SequenceMapping::fromStreamToDatabase: "
									"Failed to stream property '%s' to child "
									"query. No parent record yet.\n",
								pChild_->propName().c_str() );
					}
				}
			}
		}
	}
}


/*
 *	Override from PropertyMapping.
 */
void SequenceMapping::fromDatabaseToStream( ResultToStreamHelper & helper,
			ResultStream & results,
			BinaryOStream & strm ) const
{
	ResultSet resultSet;
	selectQuery_.execute( helper.connection(), helper.parentID(), &resultSet );

	int numRows = resultSet.numRows();
	int numExtraRows = 0;

	if (this->isFixedSize())
	{
		if (numRows != this->getFixedSize())
		{
			WARNING_MSG( "SequenceMapping::fromDatabaseToStream: "
					"Number of rows in the database does not match Entity def. "
					"Expected %d, found %d\n",
				this->getFixedSize(), numRows );


			if (numRows > this->getFixedSize())
			{
				WARNING_MSG( "SequenceMapping::fromDatabaseToStream: "
						"Ignoring %d rows from the database beyond entity def "
						"size.\n",
					numRows - this->getFixedSize() );
				numRows = this->getFixedSize();
			}
			else // if (numRows < this->getFixedSize())
			{
				numExtraRows = this->getFixedSize() - numRows;
				WARNING_MSG( "SequenceMapping::fromDatabaseToStream: "
					"Adding %d extra elements to fixed size sequence.\n",
					numExtraRows );
			}
		}
	}
	else
	{
		strm.writePackedInt( numRows );
	}

	for (int i = 0; i < numRows; ++i)
	{
		ResultRow resultRow;

		MF_VERIFY( resultRow.fetchNextFrom( resultSet ) );

		ResultStream childResultStream( resultRow );
		DatabaseID childID = 0;

		if (childHasTable_)
		{
			childResultStream >> childID;
		}

		ResultToStreamHelper childHelper( helper.connection(), childID );

		pChild_->fromDatabaseToStream( childHelper, childResultStream, strm );
	}

	for (int i = 0; i < numExtraRows; ++i)
	{
		pChild_->defaultToStream( strm );
	}
}


/**
 *	This method sets the number of elements to be expected in a sequence stream.
 *	For fixed size sequences the number of elements is not placed on the stream.
 *
 *	@param strm      The stream where the number of elements will be set to.
 *	@param numElems  The number of elements to place onto the stream.
 *
 *	@returns The number of elements that will be placed in the stream.
 */
int SequenceMapping::setNumElemsInStrm( BinaryOStream & strm,
		int numElems ) const
{
	if (this->isFixedSize())
	{
		return this->getFixedSize();
	}

	strm.writePackedInt( numElems );

	return numElems;
}


/*
 *	Override from PropertyMapping.
 */
void SequenceMapping::defaultToStream( BinaryOStream & strm ) const
{
	defaultSequenceToStream( strm, size_, pChild_ );
}


/*
 *	Override from PropertyMapping.
 */
void SequenceMapping::deleteChildren( MySql & connection,
		DatabaseID databaseID ) const
{
	if (childHasTable_)
	{
		ResultSet resultSet;
		selectChildrenQuery_.execute( connection, databaseID, &resultSet );

		DatabaseID childID;

		while (resultSet.getResult( childID ))
		{
			pChild_->deleteChildren( connection, childID );
		}
	}

	deleteQuery_.execute( connection, databaseID, NULL );
}


/*
 *	Override from PropertyMapping.
 */
bool SequenceMapping::visitParentColumns( ColumnVisitor & visitor )
{
	// We don't add any columns to our parent's table.
	return true;
}


/*
 *	Override from PropertyMapping.
 */
bool SequenceMapping::visitTables( TableVisitor & visitor )
{
	return visitor.onVisitTable( *this );
}


/*
 *	Override from TableProvider.
 */
const BW::string & SequenceMapping::getTableName() const
{
	return tblName_;
}


/*
 *	Override from TableProvider.
 */
bool SequenceMapping::visitColumnsWith( ColumnVisitor & visitor )
{
	ColumnDescription parentIdColumn(
			PARENTID_COLUMN_NAME_STR, PARENTID_COLUMN_TYPE,
			INDEX_TYPE_PARENT_ID );

	if (!visitor.onVisitColumn( parentIdColumn ))
	{
		return false;
	}

	return pChild_->visitParentColumns( visitor );
}


/*
 *	Override from TableProvider.
 */
bool SequenceMapping::visitIDColumnWith( ColumnVisitor & visitor )
{
	ColumnDescription idColumn( ID_COLUMN_NAME_STR, ID_COLUMN_TYPE,
			INDEX_TYPE_PRIMARY );

	return visitor.onVisitColumn( idColumn );
}


/*
 *	Override from TableProvider.
 */
bool SequenceMapping::visitSubTablesWith( TableVisitor & visitor )
{
	return pChild_->visitTables( visitor );
}

BW_END_NAMESPACE

// sequence_mapping.cpp
