#ifndef CATEGORIES_MONGODB_HPP
#define CATEGORIES_MONGODB_HPP

#include "../categories.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE

class CategoriesMongoDB : public Categories
{
public:
	CategoriesMongoDB( TaskManager & mongoDBTaskMgr,
		mongo::DBClientConnection & conn, const BW::string & collName );

	bool writeCategoryToDB(	MessageLogger::CategoryID newCategoryID,
		const BW::string & categoryName );

	bool init();

private:
	TaskManager & mongoDBTaskMgr_;
	mongo::DBClientConnection & conn_;
	BW::string namespace_;
};


BW_END_NAMESPACE

#endif // CATEGORIES_MONGODB_HPP
