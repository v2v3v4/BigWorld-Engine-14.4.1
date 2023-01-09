#include <algorithm>
#include "cstdmf/bw_string.hpp"

#include "categories.hpp"
#include "constants.hpp"
#include "types.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
Categories::Categories() :
	maxCategoryID_( 0 )
{
}


/**
 *
 */
void Categories::clear()
{
	idMap_.clear();
	nameMap_.clear();
}


/**
 *
 */
MessageLogger::CategoryID Categories::getNextAvailableCategoryID()
{
	return ++maxCategoryID_;
}


/**
 *	This method resolves the provided category name into a category ID, or
 *	creates a new category ID for it if one does not already exist.
 *
 *	@param categoryName	The category name to resolve an ID for.
 *
 *	@return A category ID that uniquely maps to the category name.
 */
MessageLogger::CategoryID Categories::resolveOrCreateNameToID(
	const BW::string & categoryName )
{

	NameToIdMap::const_iterator iter = nameMap_.find( categoryName );

	if (iter != nameMap_.end())
	{
		return iter->second;
	}

	MessageLogger::CategoryID newCategoryID =
			this->getNextAvailableCategoryID();


	idMap_[ newCategoryID ] = categoryName;
	nameMap_[ categoryName ] = newCategoryID;

	this->writeCategoryToDB( newCategoryID, categoryName );

	return newCategoryID;
}


/**
 *	This method obtains the ID associated with a categoryName (if one exists).
 *
 *	@param categoryName  The category name to find an ID mapping for.
 *	@param categoryID    The category ID that maps to the provided name. Only
 *	                     modified if this method returns true.
 *
 *	@return true if a category name to ID mapping exists, false otherwise.
 */
bool Categories::resolveNameToID( const BW::string & categoryName,
	MessageLogger::CategoryID & categoryID ) const
{
	NameToIdMap::const_iterator iter = nameMap_.find( categoryName );

	if (iter != nameMap_.end())
	{
		categoryID = iter->second;
		return true;
	}

	return false;
}


/**
 *	This method resolves the provided category ID into a category name.
 *
 *	@param categoryID The category ID to resolve into a category name.
 *
 *	@return A category name that uniquely mapped to the category ID.
 */
const char * Categories::resolveIDToName(
	MessageLogger::CategoryID categoryID ) const
{
	IDToNameMap::const_iterator iter = idMap_.find( categoryID );

	if (iter == idMap_.end())
	{
		WARNING_MSG( "Categories::resolveIDToName: "
				"Unknown category ID requested for resolution: %d\n",
			categoryID );
		return NULL;
	}

	return iter->second.c_str();
}


/**
 *	This method invokes onCategory on the visitor for all the known category
 *	names stored in the logs.
 *
 *	@returns true on success, false on error.
 */
bool Categories::visitAllWith( CategoriesVisitor & visitor ) const
{
	NameToIdMap::const_iterator iter = nameMap_.begin();
	bool status = true;

	while ((iter != nameMap_.end()) && (status == true))
	{
		status = visitor.onCategory( iter->first );
		++iter;
	}

	return status;
}


/**
 *	This method add the new category to the map.
 */
void Categories::addCategoryToMap( MessageLogger::CategoryID categoryID,
		BW::string & categoryString )
{
	maxCategoryID_ = std::max( maxCategoryID_, categoryID );

	idMap_[ categoryID ] = categoryString;
	nameMap_[ categoryString ] = categoryID;
}


BW_END_NAMESPACE

// categories.cpp
