#ifndef CLEAR_AUTO_LOAD_HPP
#define CLEAR_AUTO_LOAD_HPP

#include "db_storage_mysql/database_tool_app.hpp"

#include "db_storage_mysql/mappings/entity_type_mappings.hpp"


BW_BEGIN_NAMESPACE

class ClearAutoLoad : public DatabaseToolApp
{
public:
	ClearAutoLoad();
	virtual ~ClearAutoLoad();

	bool init( bool isVerbose );

	virtual bool run();

private:
	bool checkTablesExist();
	void deleteAutoLoadEntities();

	EntityTypeMappings entityTypeMappings_;
};

BW_END_NAMESPACE


#endif // CLEAR_AUTO_LOAD_HPP
