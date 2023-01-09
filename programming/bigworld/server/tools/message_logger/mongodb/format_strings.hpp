#ifndef FORMAT_STRINGS_MONGODB_HPP
#define FORMAT_STRINGS_MONGODB_HPP

#include "../types.hpp"
#include "../format_strings.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE

typedef uint32 FormatStringID;
typedef BW::map< BW::string, FormatStringID > FmtStrIDMap;

class FormatStringsMongoDB : public FormatStrings
{
public:
	FormatStringsMongoDB( TaskManager & mongoDBTaskMgr,
		mongo::DBClientConnection & conn, const BW::string & collName );

	bool canAppendToDB() { return true; };
	bool writeFormatStringToDB( LogStringInterpolator *pHandler );
	bool init();
	
	FormatStringID getIdOfFmtString( const BW::string & fmtString );

private:
	FormatStringID getNextAvailableID();

	TaskManager & mongoDBTaskMgr_;
	mongo::DBClientConnection & conn_;
	BW::string namespace_;

	FormatStringID currMaxID_;
	FmtStrIDMap fmtStrIDMap_;
};


BW_END_NAMESPACE

#endif // FORMAT_STRINGS_MONGODB_HPP
