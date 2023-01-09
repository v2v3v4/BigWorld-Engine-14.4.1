#include "categories.hpp"
#include "../constants.hpp"
#include "../types.hpp"

#include "cstdmf/bw_string.hpp"
#include <algorithm>


BW_BEGIN_NAMESPACE

static const char * CATEGORIES_FILE = "categories";


/**
 *	Constructor.
 */
CategoriesMLDB::CategoriesMLDB() :
	BinaryFileHandler()
{
}


/*
 *	Override from FileHandler.
 */
bool CategoriesMLDB::init( const char * rootDirectory, const char * mode )
{
	const char * categoriesPath = this->join( rootDirectory, CATEGORIES_FILE );
	return BinaryFileHandler::init( categoriesPath, mode );
}


/*
 *	Override from FileHandler.
 */
bool CategoriesMLDB::read()
{
	long len = pFile_->length();
	pFile_->seek( 0 );

	while (pFile_->tell() < len)
	{
		MessageLogger::CategoryID categoryID;
		BW::string categoryString;

		*pFile_ >> categoryID >> categoryString;

		this->addCategoryToMap( categoryID, categoryString );

		// This could happen if we're reading and the logger was halfway
		// through dumping a fmt when we calculated 'len'.
		if (pFile_->error())
		{
			WARNING_MSG( "CategoriesMLDB::read: "
				"Error encountered while reading categories file '%s': %s\n",
				filename_.c_str(), pFile_->strerror() );
			return false;
		}
	}

	return true;
}


/**
 *
 */
void CategoriesMLDB::flush()
{
	this->clear();
}


/**
 *  Write new categories to DB.
 */
bool CategoriesMLDB::writeCategoryToDB(
		MessageLogger::CategoryID newCategoryID,
		const BW::string & categoryName)
{
	*pFile_ << newCategoryID << categoryName;
	return pFile_->commit();
}


BW_END_NAMESPACE

// categories.cpp
