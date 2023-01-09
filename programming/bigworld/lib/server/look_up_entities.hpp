#ifndef LOOK_UP_ENTITIES__HEADER
#define LOOK_UP_ENTITIES__HEADER

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"
#include <utility>
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

typedef BW::vector< std::pair< BW::string, BW::string > > PropertyQueries;


/**
 *	This enumeration lists all the different filter types for when
 *	BigWorld.lookUpBasesByIndex() is called.
 */
enum LookUpEntitiesFilter
{
	/** Filter on all entities (no filter). */
	LOOK_UP_ENTITIES_FILTER_ALL,
	/** Filter on only online entities. */
	LOOK_UP_ENTITIES_FILTER_ONLINE,
	/** Filter on only offline entities. */  
	LOOK_UP_ENTITIES_FILTER_OFFLINE,

	// All other values are invalid.
	LOOK_UP_ENTITIES_FILTER_INVALID
};


/**
 *	This class represents criteria to apply when searching for entities in the
 *	database.
 */
class LookUpEntitiesCriteria
{
public:
	LookUpEntitiesCriteria();

	// ~LookUpEntitiesCriteria();
	// LookUpEntitiesCriteria( const LookUpEntitiesCriteria & );
	// LookUpEntitiesCriteria & operator=( const LookUpEntitiesCriteria & );

	void addPropertyQuery( const BW::string & propertyName, 
		const BW::string & searchValue );

	// Accessors

	LookUpEntitiesFilter filter() const
		{ return filter_; }

	void filter( LookUpEntitiesFilter value )
		{ filter_ = value; }

	bool hasSortProperty() const
		{ return !sortProperty_.empty(); }

	const BW::string & sortProperty() const
		{ return sortProperty_; }

	void sortProperty( const BW::string & sortProperty )
		{ sortProperty_ = sortProperty; }

	bool isAscending() const
		{ return isAscending_; }

	void isAscending( bool value )
		{ isAscending_ = value; }

	int limit() const
		{ return limit_; }

	void limit( int value ) 
		{ limit_ = value; }

	int offset() const
		{ return offset_; }

	void offset( int value )
		{ offset_ = value; }

	const PropertyQueries & propertyQueries() const
		{ return propertyQueries_; } 

	void initFromStream( BinaryIStream & is );

	BinaryOStream & addToStream( BinaryOStream & os ) const;



private:
	PropertyQueries 		propertyQueries_;
	LookUpEntitiesFilter 	filter_;
	BW::string 			sortProperty_;
	bool 					isAscending_;
	int						limit_;
	int						offset_;
};

/// Streaming operators

inline BinaryOStream & operator<<( BinaryOStream & os, const LookUpEntitiesCriteria & criteria )
{
	return criteria.addToStream( os );
}


inline BinaryIStream & operator>>( BinaryIStream & is, LookUpEntitiesCriteria & criteria )
{
	criteria.initFromStream( is );
	return is;
}

BW_END_NAMESPACE

#endif // LOOK_UP_ENTITIES__HEADER
