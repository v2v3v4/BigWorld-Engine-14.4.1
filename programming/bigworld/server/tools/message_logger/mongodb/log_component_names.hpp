#ifndef LOG_COMPONENT_NAMES_MONGODB_HPP
#define LOG_COMPONENT_NAMES_MONGODB_HPP

#include "../log_component_names.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE


typedef uint32 ComponentNameID;
typedef BW::map< BW::string, ComponentNameID > ComponentIDMap;


class LogComponentNamesMongoDB : public LogComponentNames
{
public:
	LogComponentNamesMongoDB( TaskManager & mongoDBTaskMgr,
		mongo::DBClientConnection & conn, const BW::string & collName );

	bool writeComponentNameToDB( const BW::string & componentName );
	bool init();

	ComponentNameID getIDOfComponentName( const BW::string & componentName );

private:
	ComponentNameID getNextAvailableID();

	TaskManager & mongoDBTaskMgr_;
	mongo::DBClientConnection & conn_;
	BW::string namespace_;

	ComponentNameID currMaxID_;
	ComponentIDMap componentIDMap_;
};


BW_END_NAMESPACE


#endif // LOG_COMPONENT_NAMES_MONGODB_HPP
