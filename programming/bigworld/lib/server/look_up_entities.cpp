#include "look_up_entities.hpp"

#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
LookUpEntitiesCriteria::LookUpEntitiesCriteria() :
		propertyQueries_(),
		filter_( LOOK_UP_ENTITIES_FILTER_ALL ),
		sortProperty_(),
		isAscending_( true ),
		limit_( -1 ),
		offset_( -1 )
{}


/**
 *	This method adds a property string match query to the criteria when
 *	searching for an entity in the database.
 *
 *	@param propertyName 		The name of the property to match.
 *	@param searchValue 			The search value.
 */
void LookUpEntitiesCriteria::addPropertyQuery( const BW::string & propertyName,
		const BW::string & searchValue )
{
	propertyQueries_.push_back( std::make_pair( propertyName, searchValue ) );
}


/**
 *	This method implements the streaming onto a BinaryOStream for
 *	LookUpEntitiesCriteria.
 */
BinaryOStream & LookUpEntitiesCriteria::addToStream( BinaryOStream & os ) const
{
	os << uint8( filter_ ) << sortProperty_;

	if (this->hasSortProperty())
	{
		os << isAscending_;
	}

	os << int32( limit_ ) << int32( offset_ );

	PropertyQueries::const_iterator iPropertyQuery = propertyQueries_.begin();
	while (iPropertyQuery != propertyQueries_.end())
	{
		os << iPropertyQuery->first << iPropertyQuery->second;
		++iPropertyQuery;
	}

	return os;
}


/**
 *	This method implements the streaming from a BinaryIStream for
 *	LookUpEntitiesCriteria.
 */
void LookUpEntitiesCriteria::initFromStream( BinaryIStream & is )
{
	uint8 filter;
	is >> filter >> sortProperty_;
	filter_ = LookUpEntitiesFilter( filter );

	if (this->hasSortProperty())
	{
		is >> isAscending_;
	}

	int32 limit;
	int32 offset;
	is >> limit >> offset;
	limit_ = limit;
	offset_ = offset;

	propertyQueries_.clear();

	while (is.remainingLength() > 0)
	{
		BW::string propertyName;
		BW::string searchValue;
		is >> propertyName >> searchValue;
		this->addPropertyQuery( propertyName, searchValue );
	}
}

BW_END_NAMESPACE

// look_up_entities.cpp

