#ifndef CATEGORIES_MLDB_HPP
#define CATEGORIES_MLDB_HPP

#include "../binary_file_handler.hpp"
#include "../categories.hpp"
#include "../types.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class handles the mapping of Category ID's and Category Names.
 *
 *	The category file is shared between all users and the order of the file is
 *	based on the order in which the categories are first delivered.
 */
class CategoriesMLDB : public BinaryFileHandler, public Categories
{
public:
	CategoriesMLDB();

	bool init( const char * rootDirectory, const char * mode );

	virtual bool read();

	bool writeCategoryToDB(
		MessageLogger::CategoryID newCategoryID,
		const BW::string & categoryName);

protected:
	virtual void flush();

};

BW_END_NAMESPACE

#endif // CATEGORIES_MLDB_HPP
