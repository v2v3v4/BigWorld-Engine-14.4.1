#ifndef CATEGORIES_HPP
#define CATEGORIES_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

#include "types.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This abstract class provides a template which can be used to retrieve the
 *	names of categories that are known about in logs.
 */
class CategoriesVisitor
{
public:
	virtual bool onCategory( const BW::string & categoryName ) = 0;
};


/**
 *	This class handles the mapping of Category ID's and Category Names.
 */
class Categories
{
public:
	Categories();

	MessageLogger::CategoryID resolveOrCreateNameToID(
			const BW::string & categoryName );

	bool resolveNameToID( const BW::string & categoryName,
			MessageLogger::CategoryID & categoryID ) const;

	const char * resolveIDToName( MessageLogger::CategoryID categoryID ) const;

	bool visitAllWith( CategoriesVisitor & visitor ) const;

	virtual bool writeCategoryToDB(
		MessageLogger::CategoryID newCategoryID,
		const BW::string & categoryName) = 0;

protected:
	void clear();

	void addCategoryToMap( MessageLogger::CategoryID categoryID,
		BW::string & categoryString );

private:
	MessageLogger::CategoryID getNextAvailableCategoryID();

	typedef BW::map< MessageLogger::CategoryID, BW::string > IDToNameMap;
	IDToNameMap idMap_;

	typedef BW::map< BW::string, MessageLogger::CategoryID > NameToIdMap;
	NameToIdMap nameMap_;

	MessageLogger::CategoryID maxCategoryID_;
};

BW_END_NAMESPACE

#endif // CATEGORIES_HPP
